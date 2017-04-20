#pragma once

#include "compiler.h"

#include <limits>
#include <list>
#include <set>
#include <stack>
#include <unordered_set>


#define OPT_LABEL_FUNCTION "Function"



struct optimizer_settings {
    bit_width program_width;
    label_map labels;
};


//struct abstract_expression;
typedef abstract_expression* ae_ptr;
struct std::hash<ae_ptr> {
    size_t operator()(const ae_ptr &expr) const {
        return std::hash<uintptr_t>()(reinterpret_cast<uintptr_t>(expr));
    }
};

struct reg_alloc {
    reg_alloc* prev;
    reg_alloc* next;
    size_t index;
    size_t varID;
    vbyte* reg_ptr;
    reg_alloc(reg_alloc* prev, reg_alloc* next, size_t index, size_t varID, vbyte* reg_ptr)
            : prev(prev), next(next), index(index), varID(varID), reg_ptr(reg_ptr) {}
};

struct scope_struct {
private:
    size_t _nextID = 0;

protected:
    typedef std::unordered_multimap<ae_ptr, reg_alloc*> RegAllocMap;
    typedef std::pair<ae_ptr, reg_alloc*> RegAllocPair;
    RegAllocMap _reg_alloc_map;
    std::unordered_map<size_t, reg_alloc*> _last_pos_cache;

public:
    scope_struct &_parent;
    scope_struct(scope_struct &parent) : _parent(parent) {}

    virtual void actReturn()    = 0;
    virtual void actBreak()     = 0;
    virtual void actContinue()  = 0;

    size_t getID() { return _nextID++; }

    virtual ~scope_struct() {
        for(auto && iter : _reg_alloc_map)
            if(iter.second != nullptr) delete iter.second;
    }

    void requestRegister(ae_ptr key, vbyte* reg_ptr, size_t index, size_t varID) {
        reg_alloc* npos = nullptr;
        reg_alloc* cpos = _last_pos_cache[varID];
        if(cpos != nullptr) {
            if(index < cpos->index) {
                reg_alloc* last;
                while(true) {
                    last = cpos;
                    cpos = cpos->prev;
                    if(cpos == nullptr) break;
                    if(index >= cpos->index) break;
                }
                npos = new reg_alloc( cpos, last, index, varID, reg_ptr );
            } else {
                reg_alloc* last;
                while(true) {
                    last = cpos;
                    cpos = cpos->next;
                    if(cpos == nullptr) break;
                    if(index <= cpos->index) break;
                }
                npos = new reg_alloc( cpos, last, index, varID, reg_ptr );
            }
        }
        _last_pos_cache[varID] = npos;
        _reg_alloc_map.insert({{ key, npos }});
    }

    void calculateRegisters() {

    }

};

struct scope_global : public scope_struct {
    scope_global(scope_struct &parent) : scope_struct(parent) {}

    virtual void actReturn()    { throw std::invalid_argument("scope_global::actReturn()"); }
    virtual void actBreak()     { throw std::invalid_argument("scope_global::actBreak()"); }
    virtual void actContinue()  { throw std::invalid_argument("scope_global::actContinue()"); }
};

struct scope_function : public scope_struct {
    scope_function(scope_struct &parent) : scope_struct(parent) {}

    virtual void actBreak()     { throw std::invalid_argument("scope_function::actBreak()"); }
    virtual void actContinue()  { throw std::invalid_argument("scope_function::actContinue()"); }
};

struct scope_loop : public scope_struct {
    scope_loop(scope_struct &parent) : scope_struct(parent) {}

    virtual void actReturn()    { throw std::invalid_argument("scope_loop::actReturn()"); }
};


struct abstract_expression {
    enum ArithmeticOperatorDouble {
        ADDITION,
        SUBTRACTION,
        MULTIPLICATION,
        DIVISION,
        MODULUS
    };
    enum ArithmeticOperatorSingle {
        POSITIVE, // Usually NOOP
        NEGATIVE,
        INCREMENT,
        DECREMENT
    };
    /*
    struct ExprReturn {
        enum {
            REGISTER,
            VARIABLE_ID
        } type;
        union {
            const vbyte* reg;
            const size_t varID;
        } val;
        explicit ExprReturn(const vbyte* reg) : type(REGISTER), val(reg) {}
        explicit ExprReturn(const size_t varID) : type(VARIABLE_ID), val(varID) {}
    };
     */
    virtual size_t compile(std::list<compiler_command*> &output, scope_struct &scope_parent) const = 0;
    virtual ~abstract_expression() {};
};


struct exp_variable : public abstract_expression {
    const size_t _varID;
    exp_variable(size_t varID) : _varID(varID) {}
    virtual size_t compile(std::list<compiler_command*> &output, scope_struct &scope_parent) const {
        return _varID;
    }
};

struct exp_arithmetic_double : public abstract_expression {
    const abstract_expression* _lhs;
    const abstract_expression* _rhs;
    const ArithmeticOperatorDouble _oprtr;
    exp_arithmetic_double(abstract_expression* lhs, abstract_expression* rhs, ArithmeticOperatorDouble oprtr) : _lhs(lhs), _rhs(rhs), _oprtr(oprtr) {}
    virtual ~exp_arithmetic_double() {
        if(_lhs != nullptr) delete _lhs;
        if(_rhs != nullptr) delete _rhs;
    }
    virtual size_t compile(std::list<compiler_command*> &output, scope_struct &scope_parent) const {
        // Order matters; right to left
        size_t ret_rhs = _rhs->compile(output, scope_parent);
        size_t ret_lhs = _lhs->compile(output, scope_parent);
        size_t ret_id = scope_parent.getID();

        compiler_command* cmd = nullptr;
        vbyte* r_lhs; vbyte* r_rhs; vbyte* r_res;
        switch(_oprtr) {
            case ADDITION: {
                cc_alu_addition* c = new cc_alu_addition(0, 0, 0);
                r_lhs = &c->_in_register_a;
                r_rhs = &c->_in_register_b;
                r_res = &c->_out_register;
                cmd = c;
            } break;
            case SUBTRACTION: {
                cc_alu_subtraction* c = new cc_alu_subtraction(0, 0, 0);
                r_lhs = &c->_in_register_a;
                r_rhs = &c->_in_register_b;
                r_res = &c->_out_register;
                cmd = c;
            } break;
            case MULTIPLICATION: {
                cc_alu_multiplication* c = new cc_alu_multiplication(0, 0, 0);
                r_lhs = &c->_in_register_a;
                r_rhs = &c->_in_register_b;
                r_res = &c->_out_register;
                cmd = c;
            } break;
            case DIVISION: {
                cc_alu_division* c = new cc_alu_division(0, 0, 0);
                r_lhs = &c->_in_register_a;
                r_rhs = &c->_in_register_b;
                r_res = &c->_out_register;
                cmd = c;
            } break;
            case MODULUS: {
                cc_alu_division* c = new cc_alu_division(0, 0, 0);
                r_lhs = &c->_in_register_a;
                r_rhs = &c->_in_register_b;
                r_res = &c->_out_register;
                cmd = c;
            } break;
            default: break;
        }
        if(cmd == nullptr) throw std::runtime_error("exp_arithmetic_double::compile()");

        scope_parent._reg_alloc_map.insert(scope_struct::RegAllocPair())

    }
};

struct exp_arithmetic_single : public abstract_expression {
    const abstract_expression* _expr;
    const ArithmeticOperatorSingle _oprtr;
    const bool _post;
    exp_arithmetic_single(abstract_expression* expr, ArithmeticOperatorSingle oprtr, bool post) : _expr(expr), _oprtr(oprtr), _post(post) {}
    virtual ~exp_arithmetic_single() {
        if(_expr != nullptr) delete _expr;
    }
};



struct abstract_statement {
    const optimizer_settings &_settings;
    virtual void compile(std::list<compiler_command*> &output, const scope_struct &scope_parent) const = 0;
    virtual ~abstract_statement() {}
    abstract_statement(const optimizer_settings &settings) : _settings(settings) {}
};

struct stmt_function : public abstract_statement, public scope_function {
protected:
    std::vector<abstract_statement*> _stmts;

public:
    const vbyte _param_count;
    const std::string _label;

    stmt_function(const optimizer_settings &settings, vbyte param_count = 0)
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
        output.push_back(new cc_move_to_memory(REG_COUNTER, REG_STACK, _settings.program_width));
        output.push_back(new cc_alu_const_add(REG_STACK, REG_STACK, 8, BIT_8));

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
            output.push_back(new cc_move_to_memory(reg, REG_STACK, _settings.program_width));
            output.push_back(new cc_alu_const_add(REG_STACK, REG_STACK, 8, BIT_8));
            ordered_regs[i0++] = reg;
        }

        // Add function contents
        for(compiler_command* cmd : tmp) output.push_back(cmd);

        // Retrieve the contents of any
        for(vbyte i = (vbyte)used_regs.size(); i >= 0; i--) {
            output.push_back(new cc_alu_const_subtract(REG_STACK, REG_STACK, 8, BIT_8));
            output.push_back(new cc_move_to_register(ordered_regs[i], REG_STACK, _settings.program_width));
        }

        // Return to the original location
        output.push_back(new cc_alu_const_subtract(REG_STACK, REG_STACK, 8, BIT_8));
        output.push_back(new cc_move_to_register(REG_COUNTER, REG_STACK, _settings.program_width));

    }
};