#pragma once

#include "compiler.h"

#include <limits>
#include <set>
#include <stack>
#include <unordered_set>


#define SWM_OPT_LABEL_LOOP_BEGIN            "LoopBegin"
#define SWM_OPT_LABEL_LOOP_CHECK            "LoopCondition"
#define SWM_OPT_LABEL_LOOP_END              "LoopEnd"

#define SWM_OPT_LABEL_CONDITIONAL_IF        "ConditionalIf"
#define SWM_OPT_LABEL_CONDITIONAL_ELSE      "ConditionalElse"
#define SWM_OPT_LABEL_CONDITIONAL_END       "ConditionalEnd"

#define OPT_LABEL_FUNCTION      "Function"


enum ArithmeticOperatorDouble {
    ADDITION,
    SUBTRACTION,
    MULTIPLICATION,
    DIVISION,
    MODULUS
};
enum ArithmeticOperatorSingle {
    POSITIVE, // Usually a Positive Unary operation is NOOP
    NEGATIVE,
    INCREMENT,
    DECREMENT
};
enum FlowControl {
    BREAK,
    CONTINUE,
    RETURN
};

namespace std {
    inline std::string to_string(ArithmeticOperatorDouble op) {
        switch(op) {
            case ADDITION: return "+";
            case SUBTRACTION: return "-";
            case MULTIPLICATION: return "*";
            case DIVISION: return "/";
            case MODULUS: return "%";
            default: return "(?)";
        }
    }
    inline std::string to_string(ArithmeticOperatorSingle op) {
        switch(op) {
            case POSITIVE: return "+";
            case NEGATIVE: return "-";
            case INCREMENT: return "++";
            case DECREMENT: return "--";
            default: return "(?)";
        }
    }
    inline std::string to_string(FlowControl cntrl) {
        switch(cntrl) {
            case BREAK: return "break";
            case CONTINUE: return "continue";
            case RETURN: return "return";
            default: return "flow:?";
        }
    }
}

class OptimizeException : public std::runtime_error {
public:
    enum Type {
        VARIABLE_NOT_FOUND,
        SIZE_INVALID,
        SCOPE_CONTROL,
        OUT_OF_REGISTERS,
        UNKNOWN_COMMAND,
        INVALID_OPERATION,
        MISSING_EXPRESSION
    };

    Type type() { return _type; }

    static OptimizeException VariableNotFound(size_t id) {
        return OptimizeException(VARIABLE_NOT_FOUND,
                                 "The Variable with ID '" + std::to_string(id) + "' has not been created yet");
    }
    static OptimizeException MemorySizeInvalid(BitWidth width, size_t memSize, MemoryPrefix memPrefix) {
        return OptimizeException(SIZE_INVALID,
                                    "A Bit Width of " + std::to_string(width) + " cannot map a Memory of the size " +
                                    std::to_string(memSize) + " " + std::to_string(memPrefix));
    }

    static OptimizeException ScopeControl(const FlowControl control) {
        return OptimizeException(SCOPE_CONTROL,
                                 "Flow control '" + std::to_string(control) + "' not valid for current scope");
    }

    static OptimizeException OutOfRegisters() {
        return OptimizeException(OUT_OF_REGISTERS,
                                 "Ran out of available registers when filling out register values");
    }

    static OptimizeException UnknownCommand() {
        return OptimizeException(UNKNOWN_COMMAND,
                                 "Could not determine an appropiate compiler command");
    }

    static OptimizeException InvalidSingleOperation(const std::string &operation, bool post) {
        return OptimizeException(INVALID_OPERATION,
                                 "Operation '" + operation + "' not supported as a " + (post ? "POST" : "PRE") + " operation");
    }

    static OptimizeException MissingExpression(const std::string &statement) {
        return OptimizeException(MISSING_EXPRESSION,
                                 "No Expression set for the Statement of type '" + statement + "'");
    }

protected:
    OptimizeException(Type type, const std::string &msg) : _type(type), runtime_error(msg) {}
    Type _type;
};


class mem_map {
public:
    struct mem_spot {
        size_t index;
        size_t varID;
        BitWidth width;
    };
protected:
    std::unordered_map<size_t, mem_spot> _data;
    size_t _next = 0;
public:
    mem_spot create(size_t varID, BitWidth width) {
        mem_spot val{ _next, varID, width };
        _next += width;
        _data.insert({{ varID, val }});
        DEBUG_PRINT("Created Memory for ID " << std::to_string(varID));
        return val;
    }
    mem_spot get(size_t varID) const {
        if(_data.count(varID))
            return _data.at(varID);
        else
            throw OptimizeException::VariableNotFound(varID);
    }
    bool exists(size_t varID) const {
        return _data.count(varID) > 0;
    }
    size_t size() { return _next; }
    size_t count() { return _data.size(); }
};


struct optimizer_settings {
    BitWidth program_width;
    vbyte max_register_count;
    label_map labels;
};


struct id_map {
private:
    size_t _nextID = 0;
public:
    size_t getID() { return _nextID++; }
};

struct abstract_expression;
typedef abstract_expression* ae_ptr;
/*struct std::hash<ae_ptr> {
    size_t operator()(const ae_ptr &expr) const {
        return std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(expr));
    }
};*/

struct reg_alloc {
    reg_alloc* prev;
    reg_alloc* next;
    size_t index_hint;
    size_t varID;
    bool defined;
    vbyte* reg_ptr;
    cc_iter iter;
    reg_alloc(reg_alloc* prev, reg_alloc* next, size_t index_hint, size_t varID, bool defined, vbyte* reg_ptr, const cc_iter &iter)
            : prev(prev), next(next), index_hint(index_hint), varID(varID), defined(defined), reg_ptr(reg_ptr), iter(iter) {}
};

// Assumes non-nullptr
struct ra_cmp {
    bool operator()(reg_alloc* const& lhs, reg_alloc* const& rhs) const
    { return lhs->index_hint < rhs->index_hint; }
};

struct scope_struct {
protected:
    //typedef std::unordered_multimap<ae_ptr, reg_alloc*> RegAllocMap;
    //typedef std::pair<ae_ptr, reg_alloc*> RegAllocPair;
    //RegAllocMap _reg_alloc_map;
    std::unordered_map<size_t, reg_alloc*> _last_pos_cache;
    std::set<reg_alloc*, ra_cmp> _begin_cache;

    std::stack<std::set<size_t>> _loop_cache;

    scope_struct(scope_struct* const parent) : _parent(parent) {}

public:
    //scope_struct &_parent;
    scope_struct* const _parent;

    std::string _label_break = "";
    std::string _label_continue = "";

    virtual ~scope_struct() {
        for(reg_alloc* entry : _begin_cache) {
            reg_alloc* current = entry;
            while(current != nullptr) {
                reg_alloc* next = current->next;
                delete current;
                current = next;
            }
        }
    }

    void addRegisterEntry(vbyte* reg_ptr, size_t index_hint, size_t varID, cc_iter iter, bool defined = false) {
        reg_alloc* npos = nullptr;
        reg_alloc* cpos = _last_pos_cache[varID];

        DEBUG_PRINT("Adding Register Entry for ID " << std::to_string(varID) << " and Index Hint " << std::to_string(index_hint))

        // Previous Entry Exists
        if(cpos != nullptr) {

            // Iterate Backwards until a spot is found
            if(index_hint < cpos->index_hint) {
                reg_alloc* last;
                while(true) {
                    last = cpos;
                    cpos = cpos->prev;
                    if(cpos == nullptr) break;
                    if(index_hint >= cpos->index_hint) break;
                }
                npos = new reg_alloc( cpos, last, index_hint, varID, defined, reg_ptr, iter );
                if(cpos != nullptr) cpos->next = npos;
                last->prev = npos;
            }
            // Iterate Forwards until a spot is found
            else {
                reg_alloc* last;
                while(true) {
                    last = cpos;
                    cpos = cpos->next;
                    if(cpos == nullptr) break;
                    if(index_hint <= cpos->index_hint) break;
                }
                npos = new reg_alloc( last, cpos, index_hint, varID, defined, reg_ptr, iter );
                if(cpos != nullptr) cpos->prev = npos;
                last->next = npos;
            }

            // Check to see if the begin_cache needs to be updated
            if(npos->prev == nullptr) {
                _begin_cache.erase(npos->next);
                _begin_cache.insert( npos );
            }

        }
        // No Previous Entry, create one
        else {
            npos = new reg_alloc( nullptr, nullptr, index_hint, varID, defined, reg_ptr, iter );
            _begin_cache.insert( npos );
        }
        // Update last_pos_cache
        _last_pos_cache[varID] = npos;

        // Add to Loop Cache if necessary
        if(!_loop_cache.empty())
            _loop_cache.top().insert(varID);
    }

    void pushLoopCache() { _loop_cache.push(std::set<size_t>()); }

    void popLoopCache(size_t index_hint, cc_iter iter) {
        if(!_loop_cache.empty()) {
            std::set<size_t> &ids = _loop_cache.top();
            for (size_t id : ids)
                addRegisterEntry(nullptr, index_hint, id, iter);
            _loop_cache.pop();
        }
    }

private:
    struct UsedRegSpan {
        UsedRegSpan* prev;
        UsedRegSpan* next;
        size_t begin;
        size_t end;
        UsedRegSpan(UsedRegSpan* prev, UsedRegSpan* next, size_t begin, size_t end)
                : prev(prev), next(next), begin(begin), end(end) {}
    };

public:
    void calculateRegisters(cc_list &cmds, optimizer_settings &settings, mem_map &mem, const std::unordered_set<vbyte> &reserved_registers) {

        UsedRegSpan* used_regs[settings.max_register_count] = { nullptr };

        // Iterate through all
        for(reg_alloc* ra : _begin_cache) {

            reg_alloc* current = ra;
            reg_alloc* last = ra;
            while(current != nullptr) {
                last = current;
                current = current->next;
            }

            size_t begin = ra->index_hint;
            size_t end = last->index_hint;

            // Find register with unused space matching the required usage space
            vbyte reg = settings.max_register_count;
            for(vbyte i = 0; i < settings.max_register_count; i++) {
                if(reserved_registers.count(i)) continue;

                UsedRegSpan* c = used_regs[i];
                if(c == nullptr) {
                    // This register isn't used at all, so there is nothing but empty space
                    used_regs[i] = new UsedRegSpan( nullptr, nullptr, begin, end );
                    reg = i;
                    break;
                } else {
                    size_t last_index = 0;
                    UsedRegSpan* l = c;
                    while(c != nullptr) {
                        // Found empty space either in the beginning or in the middle
                        if( ((begin > last_index) || (c->prev == nullptr)) && end < c->begin ) {
                            UsedRegSpan* n = new UsedRegSpan( c->prev, c, begin, end );
                            c->prev = n;
                            if(c->prev == nullptr) used_regs[i] = n;
                            else c->prev->next = n;
                            reg = i;
                            break;
                        }
                        last_index = c->end;
                        l = c;
                        c = c->next;
                    }
                    // Found empty space at the end
                    if(begin > last_index) {
                        UsedRegSpan* n = new UsedRegSpan( l, nullptr, begin, end );
                        l->next = n;
                        reg = i;
                        break;
                    }
                }
            }

            // For now, throw exception if out of registers
            // TODO: Determine and store a used register temporarily in order to enable more register usage
            if(reg == settings.max_register_count)
                throw OptimizeException::OutOfRegisters();

            // Add a Load Cmd if needed
            if(mem.exists(ra->varID) && !ra->defined) {
                const mem_map::mem_spot spot = mem.get(ra->varID);
                compiler_command* cc = new cc_move_to_register_constant(reg, spot.index, settings.program_width, spot.width);
                cmds.insert(ra->iter, cc);
            }

            // Set all the register values
            current = ra;
            last = ra;
            while(current != nullptr) {
                if(current->reg_ptr != nullptr) *(current->reg_ptr) = reg;
                last = current;
                current = current->next;
            }

            // Add a Store Cmd if needed
            if(mem.exists(ra->varID)) {
                const mem_map::mem_spot spot = mem.get(ra->varID);
                compiler_command* cc = new cc_move_to_memory_constant(reg, spot.index, settings.program_width, spot.width);
                cc_iter i = last->iter;
                i++;
                cmds.insert(i, cc);
            }

        }

        // Delete Heap Created Data
        for(vbyte i = 0; i < settings.max_register_count; i++) {
            UsedRegSpan* c = used_regs[i];
            while(c != nullptr) {
                UsedRegSpan* n = c->next;
                delete c;
                c = n;
            }
        }
    }

};

struct scope_global : public scope_struct {
    scope_global() : scope_struct(nullptr) {}
};

struct scope_function : public scope_struct {
    scope_function(scope_struct &parent) : scope_struct(&parent) {}
};

struct scope_loop : public scope_struct {
    scope_loop(scope_struct &parent) : scope_struct(&parent) {}
};

struct scope_block : public scope_struct {
    scope_block(scope_struct &parent) : scope_struct(&parent) {}
};


struct abstract_expression {
    virtual size_t compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids, size_t resID) const = 0;
    virtual size_t compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids) const {
        size_t ret_id = ids.getID();
        return compile(output, scope_parent, settings, mem, ids, ret_id);
    }
    virtual std::string to_string() const = 0;
    virtual std::string to_string(size_t indent) const {
        std::string pre("");
        for(size_t i = 0; i < indent; i++)
            pre += "  ";
        return pre + to_string();
    }
    virtual ~abstract_expression() {};
};


struct exp_variable : public abstract_expression {
    const size_t _varID;
    exp_variable(size_t varID) : _varID(varID) {}
    virtual size_t compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids, size_t resID) const {
        DEBUG_PRINT("Compiling Variable Expression");
        return _varID;
    }
    virtual size_t compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids) const {
        DEBUG_PRINT("Compiling Variable Expression");
        return _varID;
    }
    virtual std::string to_string() const {
        return "{" + std::to_string(_varID) + "}";
    }
};

struct exp_constant : public abstract_expression {
    const int64_t _value;
    exp_constant(int64_t value) : _value(value) {}
    virtual size_t compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids, size_t resID) const {
        DEBUG_PRINT("Compiling Constant Value Expression");
        cc_load_constant* cmd = new cc_load_constant(0, _value, settings.program_width);
        cc_iter it = output.insert(output.end(), &*cmd);
        scope_parent.addRegisterEntry(&cmd->_target_register, output.size()-1, resID, it);
        return resID;
    }
    virtual std::string to_string() const {
        return std::to_string(_value);
    }
};

struct exp_arithmetic_double : public abstract_expression {
    const abstract_expression* _lhs;
    const abstract_expression* _rhs;
    const ArithmeticOperatorDouble _op;
    exp_arithmetic_double(const abstract_expression* lhs, const abstract_expression* rhs, ArithmeticOperatorDouble op)
            : _lhs(lhs), _rhs(rhs), _op(op) {}
    virtual ~exp_arithmetic_double() {
        if(_lhs != nullptr) delete _lhs;
        if(_rhs != nullptr) delete _rhs;
    }
    virtual size_t compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids, size_t resID) const {
        DEBUG_PRINT("Compiling Double Arithmetic Expression of Type " + std::to_string(_op));
        // Order matters; right to left
        size_t ret_rhs = _rhs->compile(output, scope_parent, settings, mem, ids);
        size_t ret_lhs = _lhs->compile(output, scope_parent, settings, mem, ids);

        cc_alu_double_operation* cmd = nullptr;
        switch(_op) {
            case ADDITION:          cmd = new cc_alu_addition(0, 0, 0); break;
            case SUBTRACTION:       cmd = new cc_alu_subtraction(0, 0, 0); break;
            case MULTIPLICATION:    cmd = new cc_alu_multiplication(0, 0, 0); break;
            case DIVISION:          cmd = new cc_alu_division(0, 0, 0); break;
            case MODULUS:           cmd = new cc_alu_modulus(0, 0, 0); break;
            default: throw OptimizeException::UnknownCommand();
        }

        cc_iter it = output.insert(output.end(), &*cmd);

        scope_parent.addRegisterEntry(&cmd->_in_register_a, output.size()-1, ret_lhs, it);
        scope_parent.addRegisterEntry(&cmd->_in_register_b, output.size()-1, ret_rhs, it);
        scope_parent.addRegisterEntry(&cmd->_out_register,  output.size()-1, resID,  it);

        return resID;
    }
    virtual std::string to_string() const {
        return "(" + _lhs->to_string() + " " + std::to_string(_op) + " " + _rhs->to_string() + ")";
    }
};

struct exp_arithmetic_single : public abstract_expression {
    const abstract_expression* _expr;
    const ArithmeticOperatorSingle _op;
    const bool _post;
    exp_arithmetic_single(const abstract_expression* expr, ArithmeticOperatorSingle op, bool post = false)
            : _expr(expr), _op(op), _post(post) {}
    virtual ~exp_arithmetic_single() {
        if(_expr != nullptr) delete _expr;
    }
    virtual size_t compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids, size_t resID) const {
        DEBUG_PRINT("Compiling Single Arithmetic Expression of Type " + std::string(_post ? "Post" : "Pre") + std::to_string(_op));
        size_t ret_expr = _expr->compile(output, scope_parent, settings, mem, ids);

        switch(_op) {
            case POSITIVE: {
                if(_post) throw OptimizeException::InvalidSingleOperation("Positive", _post);
                else return ret_expr;
            }
            case NEGATIVE: {
                if(_post) throw OptimizeException::InvalidSingleOperation("Negative", _post);
                cc_alu_move_inversion* cmd = new cc_alu_move_inversion(0, 0);
                cc_iter it = output.insert(output.end(), &*cmd);
                scope_parent.addRegisterEntry(&cmd->_in_register,  output.size()-1, ret_expr, it);
                scope_parent.addRegisterEntry(&cmd->_out_register, output.size()-1, resID,   it);
            } break;
            case INCREMENT: {
                if(_post) {
                    cc_copy_register* cmd1 = new cc_copy_register(0, 0);
                    cc_iter it = output.insert(output.end(), &*cmd1);
                    scope_parent.addRegisterEntry(&cmd1->_from_register, output.size()-1, ret_expr, it);
                    scope_parent.addRegisterEntry(&cmd1->_to_register,   output.size()-1, resID,   it);
                    cc_alu_increment* cmd2 = new cc_alu_increment(0);
                    it = output.insert(output.end(), &*cmd2);
                    scope_parent.addRegisterEntry(&cmd2->_register, output.size()-1, ret_expr, it);
                } else {
                    cc_alu_increment* cmd1 = new cc_alu_increment(0);
                    cc_iter it = output.insert(output.end(), &*cmd1);
                    scope_parent.addRegisterEntry(&cmd1->_register, output.size()-1, ret_expr, it);
                    cc_copy_register* cmd2 = new cc_copy_register(0, 0);
                    it = output.insert(output.end(), &*cmd2);
                    scope_parent.addRegisterEntry(&cmd2->_from_register, output.size()-1, ret_expr, it);
                    scope_parent.addRegisterEntry(&cmd2->_to_register,   output.size()-1, resID,   it);
                }
            } break;
            case DECREMENT: {
                if(_post) {
                    cc_copy_register* cmd1 = new cc_copy_register(0, 0);
                    cc_iter it = output.insert(output.end(), &*cmd1);
                    scope_parent.addRegisterEntry(&cmd1->_from_register, output.size()-1, ret_expr, it);
                    scope_parent.addRegisterEntry(&cmd1->_to_register,   output.size()-1, resID,   it);
                    cc_alu_decrement* cmd2 = new cc_alu_decrement(0);
                    it = output.insert(output.end(), &*cmd2);
                    scope_parent.addRegisterEntry(&cmd2->_register, output.size()-1, ret_expr, it);
                } else {
                    cc_alu_decrement* cmd1 = new cc_alu_decrement(0);
                    cc_iter it = output.insert(output.end(), &*cmd1);
                    scope_parent.addRegisterEntry(&cmd1->_register, output.size()-1, ret_expr, it);
                    cc_copy_register* cmd2 = new cc_copy_register(0, 0);
                    it = output.insert(output.end(), &*cmd2);
                    scope_parent.addRegisterEntry(&cmd2->_from_register, output.size()-1, ret_expr, it);
                    scope_parent.addRegisterEntry(&cmd2->_to_register,   output.size()-1, resID,   it);
                }
            } break;
            default: throw OptimizeException::UnknownCommand();
        }

        return resID;
    }
    virtual std::string to_string() const {
        return _post ? ( _expr->to_string() + std::to_string(_op) ) : ( std::to_string(_op) + _expr->to_string() );
    }
};



struct abstract_statement {
    virtual void compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids) const = 0;
    virtual std::string to_string() const = 0;
    virtual std::string to_string(size_t indent) const {
        std::string pre("");
        for(size_t i = 0; i < indent; i++)
            pre += "  ";
        return pre + to_string();
    }
    virtual ~abstract_statement() {}
};

struct stmt_assignment : public abstract_statement {
    const abstract_expression* _expr;
    const size_t _varID;
    const bool _define;
    stmt_assignment(const abstract_expression* expr, size_t varID, bool define = false)
            : _expr(expr), _varID(varID), _define(define) {}
    virtual ~stmt_assignment() {
        if(_expr != nullptr) delete _expr;
    }
    virtual void compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids) const {
        DEBUG_PRINT("Compiling Assignment Statement");
        if(_define && !mem.exists(_varID)) mem.create(_varID, settings.program_width);
        _expr->compile(output, scope_parent, settings, mem, ids, _varID);
        //cc_copy_register* cmd = new cc_copy_register(0, 0);
        //cc_iter it = output.insert(output.end(), &*cmd);
        //scope_parent.addRegisterEntry(&cmd->_from_register, output.size()-1, ret_expr, it);
        //scope_parent.addRegisterEntry(&cmd->_to_register,   output.size()-1, _varID,   it, _define);
    }
    virtual std::string to_string() const {
        return std::string(_define ? "init " : "") + "{" + std::to_string(_varID) + "} = " + _expr->to_string();
    }
};

struct stmt_expr : public abstract_statement {
    const abstract_expression* _expr;
    stmt_expr(const abstract_expression* expr)
            : _expr(expr) {}
    virtual ~stmt_expr() {
        if(_expr != nullptr) delete _expr;
    }
    virtual void compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids) const {
        DEBUG_PRINT("Compiling Expression Statement");
        _expr->compile(output, scope_parent, settings, mem, ids);
    }
    virtual std::string to_string() const {
        return _expr->to_string();
    }
};

struct stmt_flow_control : public abstract_statement {
    const FlowControl _control;
    const abstract_expression* _expr_ret;
    stmt_flow_control(const FlowControl control, const abstract_expression* expr_ret = nullptr)
            : _control(control), _expr_ret(expr_ret) {}
    virtual ~stmt_flow_control() {
        if(_expr_ret != nullptr) delete _expr_ret;
    }
    virtual void compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids) const {
        DEBUG_PRINT("Compiling Flow Control Statement");
        switch(_control) {
            case BREAK: {
                if(scope_parent._label_break.size() < 1)
                    throw OptimizeException::ScopeControl(_control);
                else
                    output.insert(output.end(), new cc_jump(settings.labels, scope_parent._label_break));
            } break;
            case CONTINUE: {
                if(scope_parent._label_continue.size() < 1)
                    throw OptimizeException::ScopeControl(_control);
                else
                    output.insert(output.end(), new cc_jump(settings.labels, scope_parent._label_continue));
            } break;
            case RETURN: {
                // TODO: Fill out stmt_flow_control Return operation
                throw OptimizeException::ScopeControl(_control);
            } break;
            default: break;
        }
    }
    virtual std::string to_string() const {
        return std::to_string(_control) +
                (( _control == RETURN && _expr_ret != nullptr) ? (" " + _expr_ret->to_string()) : "");
    }
};

struct stmt_loop : public abstract_statement {
protected:
    stmt_list _stmts;

public:
    const abstract_statement* _stmt_init;
    const abstract_expression* _expr_cond;
    const abstract_statement* _stmt_inc;

    stmt_loop(stmt_list stmts, const abstract_statement* expr_init, const abstract_expression* expr_cond, const abstract_statement* expr_inc)
            : _stmts(stmts), _stmt_init(expr_init), _expr_cond(expr_cond), _stmt_inc(expr_inc) {}

    virtual ~stmt_loop() {
        for(abstract_statement* stmt : _stmts)
            delete stmt;
        if(_stmt_init != nullptr) delete _stmt_init;
        if(_expr_cond != nullptr) delete _expr_cond;
        if(_stmt_inc  != nullptr) delete _stmt_inc;
    }

    virtual void compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids) const {

        scope_parent.pushLoopCache();

        // Loop initialize statement
        if(_stmt_init != nullptr) _stmt_init->compile(output, scope_parent, settings, mem, ids);

        // Load a '0' constant for comparisons
        size_t zeroConstID = ids.getID();
        cc_load_constant* cmd_lc = new cc_load_constant(0, 0, BIT_8);
        cc_iter it = output.insert(output.end(), &*cmd_lc);
        scope_parent.addRegisterEntry(&cmd_lc->_target_register, output.size()-1, zeroConstID, it);

        // Create the loop's labels
        std::string label_begin = settings.labels.uniqueLabel(SWM_OPT_LABEL_LOOP_BEGIN);
        std::string label_check = settings.labels.uniqueLabel(SWM_OPT_LABEL_LOOP_CHECK);
        std::string label_end = settings.labels.uniqueLabel(SWM_OPT_LABEL_LOOP_END);

        // Cache the previous Flow Control labels
        std::string label_old_fc_break = scope_parent._label_break;
        std::string label_old_fc_continue = scope_parent._label_continue;

        // Set the Flow Control labels
        scope_parent._label_break = label_end;
        scope_parent._label_continue = label_begin;

        // Jump to the Loop Condition Check
        output.insert(output.end(), new cc_jump(settings.labels, label_check));

        // Create the start label
        output.insert(output.end(), new cc_label(settings.labels, label_begin));

        // Evaluate loop contents
        for(abstract_statement* stmt : _stmts) {
            stmt->compile(output, scope_parent, settings, mem, ids);
        }

        // Loop Increment Statement
        if(_stmt_inc != nullptr) _stmt_inc->compile(output, scope_parent, settings, mem, ids);

        // Create the check label
        output.insert(output.end(), new cc_label(settings.labels, label_check));

        // Jump back to the start based on Loop Check Expression
        if(_expr_cond != nullptr) {
            size_t retID_cond = _expr_cond->compile(output, scope_parent, settings, mem, ids);
            cc_jump_less *cmd_jmp = new cc_jump_less(settings.labels, label_begin, 0, 0);
            it = output.insert(output.end(), &*cmd_jmp);
            scope_parent.addRegisterEntry(&cmd_jmp->_register_a, output.size() - 1, zeroConstID, it);
            scope_parent.addRegisterEntry(&cmd_jmp->_register_b, output.size() - 1, retID_cond, it);
        } else {
            // No check, always jump (infinite loop if no Flow Control exists)
            output.insert(output.end(), new cc_jump(settings.labels, label_begin));
        }

        // Create the end label
        output.insert(output.end(), new cc_label(settings.labels, label_end));

        // Reset the Flow Control labels
        scope_parent._label_break = label_old_fc_break;
        scope_parent._label_continue = label_old_fc_continue;

        scope_parent.popLoopCache(output.size() - 1, --output.end());
    }
    virtual std::string to_string(size_t indent) const {
        std::string ind("");
        for(size_t i = 0; i < indent; i++)
            ind += "  ";
        std::string result = ind + "loop ( " +
                ( _stmt_init == nullptr ? "" : _stmt_init->to_string()) + " ; " +
                ( _expr_cond == nullptr ? "" : _expr_cond->to_string()) + " ; " +
                ( _stmt_inc == nullptr ? "" : _stmt_inc->to_string()) + " )";
        for(abstract_statement* stmt : _stmts)
            result += "\n" + stmt->to_string(indent+1);
        return result;
    }
    virtual std::string to_string() const {
        return to_string(0);
    }

};

struct stmt_conditional : public abstract_statement {
    struct conditional_block {
        const abstract_expression* expr;
        stmt_list stmts;
        conditional_block(const abstract_expression* expr, stmt_list stmts)
                : expr(expr), stmts(stmts) {}
        ~conditional_block() {
            delete expr;
            for(abstract_statement* stmt : stmts)
                delete stmt;
        }
    };

    stmt_list _else_stmts;
    std::list<conditional_block*> _if_blocks;

    stmt_conditional(std::list<conditional_block*> if_blocks, stmt_list else_stmts)
            :  _if_blocks(if_blocks), _else_stmts(else_stmts)  {}

    virtual ~stmt_conditional() {
        for(abstract_statement* stmt : _else_stmts)
            delete stmt;
        for(conditional_block* block : _if_blocks)
            delete block;
    }

    virtual void compile(cc_list &output, scope_struct &scope_parent, optimizer_settings &settings, mem_map &mem, id_map &ids) const {

        // Load a '0' constant for comparisons
        size_t zeroConstID = ids.getID();
        cc_load_constant* cmd_lc = new cc_load_constant(0, 0, BIT_8);
        cc_iter it = output.insert(output.end(), &*cmd_lc);
        scope_parent.addRegisterEntry(&cmd_lc->_target_register, output.size()-1, zeroConstID, it);

        // Create the conditional's labels
        std::string label_if = settings.labels.uniqueLabel(SWM_OPT_LABEL_CONDITIONAL_IF);
        std::string label_else = settings.labels.uniqueLabel(SWM_OPT_LABEL_CONDITIONAL_ELSE);
        std::string label_end = settings.labels.uniqueLabel(SWM_OPT_LABEL_CONDITIONAL_END);

        // Check to make sure there is at least one conditional block
        if(_if_blocks.empty())
            throw OptimizeException::MissingExpression("Conditional");

        // Go through If blocks in order and check conditionals
        size_t bi = 0;
        for(conditional_block* block : _if_blocks) {
            size_t retID_cond = block->expr->compile(output, scope_parent, settings, mem, ids);
            cc_jump_less *cmd_jmp = new cc_jump_less(settings.labels, label_if + std::to_string(bi), 0, 0);
            it = output.insert(output.end(), &*cmd_jmp);
            scope_parent.addRegisterEntry(&cmd_jmp->_register_a, output.size() - 1, zeroConstID, it);
            scope_parent.addRegisterEntry(&cmd_jmp->_register_b, output.size() - 1, retID_cond, it);
            bi++;
        }

        // Add Else jump
        output.insert(output.end(), new cc_jump(settings.labels, label_else));

        // Go through If blocks in order and compile statements
        bi = 0;
        for(conditional_block* block : _if_blocks) {
            output.insert(output.end(), new cc_label(settings.labels, label_if + std::to_string(bi)));
            for(abstract_statement* stmt : block->stmts) {
                stmt->compile(output, scope_parent, settings, mem, ids);
            }
            output.insert(output.end(), new cc_jump(settings.labels, label_end));
            bi++;
        }

        // Create the Else label
        output.insert(output.end(), new cc_label(settings.labels, label_else));

        // Add Else statements
        for(abstract_statement* stmt : _else_stmts) {
            stmt->compile(output, scope_parent, settings, mem, ids);
        }

        // Create the end label
        output.insert(output.end(), new cc_label(settings.labels, label_end));
    }
    virtual std::string to_string(size_t indent) const {
        std::string ind("");
        for(size_t i = 0; i < indent; i++)
            ind += "  ";
        std::string result("");
        size_t bi = 0;
        for(conditional_block* block : _if_blocks) {
            result += ind + "conditional " + std::to_string(bi) + "( " + block->expr->to_string() + ")\n";
            for (abstract_statement *stmt : block->stmts)
                result += stmt->to_string(indent + 1) + "\n";
            bi++;
        }
        result += ind + "conditional else";
        for (abstract_statement *stmt : _else_stmts)
            result += "\n" + stmt->to_string(indent + 1);
        return result;
    }
    virtual std::string to_string() const {
        return to_string(0);
    }
};

struct stmt_function_definition : public abstract_statement {

};

/*
struct stmt_function : public abstract_statement, public scope_function {
protected:
    std::vector<abstract_statement*> _stmts;

public:
    const vbyte _param_count;
    const std::string _label;

    stmt_function(optimizer_settings &settings, vbyte param_count = 0)
            : abstract_statement(settings), _param_count(param_count), _label(settings.labels.uniqueLabel(OPT_LABEL_FUNCTION)) {}

    void add(abstract_statement* stmt) {
        _stmts.push_back(stmt);
    }

    virtual ~stmt_function() {
        for(abstract_statement* stmt : _stmts) delete stmt;
    }

    virtual void compile(std::list<compiler_command*> &output, const scope_struct &scope_parent) const {

        if(_param_count > reg_array.registerMax())
            throw std::out_of_range("stmt_function::compile() - Paramater count higher than total register count");

        // Add the label for jumping to this function
        output.push_back(new cc_label(_settings.labels, _label));

        // Add the return location to the stack
        output.push_back(new cc_move_to_memory(SWM_REG_COUNTER, SWM_REG_STACK, _settings.program_width));
        output.push_back(new cc_alu_const_add(SWM_REG_STACK, SWM_REG_STACK, 8, BIT_8));

        register_array func_reg_array(reg_array.registerMax());
        size_t func_reg_counter = 0;

        // Create the temporary function commands
        std::vector<compiler_command*> tmp;
        for(abstract_statement* stmt : _stmts) stmt->compile(tmp, *this);

        // Determine appropiate register IDs
        std::set<vbyte> used_regs = func_reg_array.compile((vbyte)(_param_count+1));
        vbyte ordered_regs[used_regs.size()];
        vbyte i0 = 0;

        // Add the contents of any used registers to the stack
        for(vbyte reg : used_regs) {
            output.push_back(new cc_move_to_memory(reg, SWM_REG_STACK, _settings.program_width));
            output.push_back(new cc_alu_const_add(SWM_REG_STACK, SWM_REG_STACK, 8, BIT_8));
            ordered_regs[i0++] = reg;
        }

        // Add function contents
        for(compiler_command* cmd : tmp) output.push_back(cmd);

        // Retrieve the contents of any
        for(vbyte i = (vbyte)used_regs.size(); i >= 0; i--) {
            output.push_back(new cc_alu_const_subtract(SWM_REG_STACK, SWM_REG_STACK, 8, BIT_8));
            output.push_back(new cc_move_to_register(ordered_regs[i], SWM_REG_STACK, _settings.program_width));
        }

        // Return to the original location
        output.push_back(new cc_alu_const_subtract(SWM_REG_STACK, SWM_REG_STACK, 8, BIT_8));
        output.push_back(new cc_move_to_register(SWM_REG_COUNTER, SWM_REG_STACK, _settings.program_width));

    }
};
*/

cc_list compileOptimizeList(const std::list<abstract_statement*> &stmts, optimizer_settings &settings, id_map &ids, size_t* req_mem_size);