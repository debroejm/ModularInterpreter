#pragma once

#include "types.h"
#include "ve_commands.h"

#include <unordered_map>
#include <vector>

class compiler_command {
public:
    virtual void compile(vbyte* result, size_t pos) const = 0;
    virtual size_t size() const = 0;
    virtual vbyte command() const = 0;
protected:
    static vbyte widthFlag(bit_width width) {
        switch(width) {
            default:
            case BIT_8:  return CMD_PRECISION_1B;
            case BIT_16: return CMD_PRECISION_2B;
            case BIT_32: return CMD_PRECISION_4B;
            case BIT_64: return CMD_PRECISION_8B;
        }
    }
};

vbyte* compileCommandList(const std::vector<compiler_command*> &cmds, size_t* size);

class cc_nop : public compiler_command {
public:
    virtual void compile(vbyte* result, size_t pos) const { result[pos] = command(); }
    virtual size_t size() const { return 1; }
    virtual vbyte command() const { return CMD_NOP; }
};

class cc_halt : public compiler_command {
public:
    virtual void compile(vbyte* result, size_t pos) const { result[pos] = command(); }
    virtual size_t size() const { return 1; }
    virtual vbyte command() const { return CMD_HALT; }
};

class cc_move_to_register : public compiler_command {
protected:
    vbyte _target_register;
    vbyte _mem_address_register;
    bit_width _width;
public:
    cc_move_to_register(vbyte target_register, vbyte mem_address_register, bit_width width)
            : _target_register(target_register), _mem_address_register(mem_address_register), _width(width) {}
    virtual void compile(vbyte* result, size_t pos) const {
        result[pos + 0] = (vbyte) (command() | widthFlag(_width));
        result[pos + 1] = _target_register;
        result[pos + 2] = _mem_address_register;
    }
    virtual size_t size() const { return 3; }
    virtual vbyte command() const { return CMD_MVTOREG; }
};

class cc_move_to_memory : public compiler_command {
protected:
    vbyte _target_register;
    vbyte _mem_address_register;
    bit_width _width;
public:
    cc_move_to_memory(vbyte target_register, vbyte mem_address_register, bit_width width)
            : _target_register(target_register), _mem_address_register(mem_address_register), _width(width) {}
    virtual void compile(vbyte* result, size_t pos) const {
        result[pos + 0] = (vbyte) (command() | widthFlag(_width));
        result[pos + 1] = _target_register;
        result[pos + 2] = _mem_address_register;
    }
    virtual size_t size() const { return 3; }
    virtual vbyte command() const { return CMD_MVTOMEM; }
};

class cc_load_constant : public compiler_command {
protected:
    vbyte _target_register;
    vvalue _value;
public:
    cc_load_constant(vbyte target_register, vvalue value)
            : _target_register(target_register), _value(value) {}
    cc_load_constant(vbyte target_register, size_t value, bit_width width)
            : _target_register(target_register), _value(value, width) {}
    virtual void compile(vbyte* result, size_t pos) const {
        result[pos + 0] = (vbyte) (command() | widthFlag(_value._width));
        result[pos + 1] = _target_register;
        for(unsigned short i = 0; i < _value._width; i++) {
            switch(_value._width) {
                default:
                case BIT_8:  result[pos + 2 + i] = _value._8.bytes[i]; break;
                case BIT_16: result[pos + 2 + i] = _value._16.bytes[1-i]; break;
                case BIT_32: result[pos + 2 + i] = _value._32.bytes[3-i]; break;
                case BIT_64: result[pos + 2 + i] = _value._64.bytes[7-i]; break;
            }
        }
    }
    virtual size_t size() const { return (size_t)(2+_value._width); }
    virtual vbyte command() const { return CMD_LDCONST; }
};

class cc_copy_register : public compiler_command {
protected:
    vbyte _from_register;
    vbyte _to_register;
public:
    cc_copy_register(vbyte from_register, vbyte to_register)
            : _from_register(from_register), _to_register(to_register) {}
    virtual void compile(vbyte* result, size_t pos) const {
        result[pos + 0] = command();
        result[pos + 1] = _from_register;
        result[pos + 2] = _to_register;
    }
    virtual size_t size() const { return 3; }
    virtual vbyte command() const { return CMD_CPREG; }
};

class cc_alu_double_operation : public compiler_command {
protected:
    vbyte _in_register_a;
    vbyte _in_register_b;
    vbyte _out_register;
public:
    cc_alu_double_operation(vbyte in_register_a, vbyte in_register_b, vbyte out_register)
            : _in_register_a(in_register_a), _in_register_b(in_register_b), _out_register(out_register) {}
    virtual void compile(vbyte* result, size_t pos) const {
        result[pos + 0] = command();
        result[pos + 1] = _in_register_a;
        result[pos + 2] = _in_register_b;
        result[pos + 3] = _out_register;
    }
    virtual size_t size() const { return 4; }
};

class cc_alu_addition : public cc_alu_double_operation {
public:
    virtual vbyte command() const { return CMD_ALU_ADD; }
    cc_alu_addition(vbyte in_register_a, vbyte in_register_b, vbyte out_register)
            : cc_alu_double_operation(in_register_a, in_register_b, out_register) {}
};

class cc_alu_subtraction : public cc_alu_double_operation {
public:
    virtual vbyte command() const { return CMD_ALU_SUB; }
    cc_alu_subtraction(vbyte in_register_a, vbyte in_register_b, vbyte out_register)
            : cc_alu_double_operation(in_register_a, in_register_b, out_register) {}
};

class cc_alu_multiplication : public cc_alu_double_operation {
public:
    virtual vbyte command() const { return CMD_ALU_MULT; }
    cc_alu_multiplication(vbyte in_register_a, vbyte in_register_b, vbyte out_register)
            : cc_alu_double_operation(in_register_a, in_register_b, out_register) {}
};

class cc_alu_division : public cc_alu_double_operation {
public:
    virtual vbyte command() const { return CMD_ALU_DIV; }
    cc_alu_division(vbyte in_register_a, vbyte in_register_b, vbyte out_register)
            : cc_alu_double_operation(in_register_a, in_register_b, out_register) {}
};

class cc_alu_modulus : public cc_alu_double_operation {
public:
    virtual vbyte command() const { return CMD_ALU_MOD; }
    cc_alu_modulus(vbyte in_register_a, vbyte in_register_b, vbyte out_register)
            : cc_alu_double_operation(in_register_a, in_register_b, out_register) {}
};

class cc_alu_single_operation : public compiler_command {
protected:
    vbyte _register;
public:
    cc_alu_single_operation(vbyte in_register)
            : _register(in_register) {}
    virtual void compile(vbyte* result, size_t pos) const {
        result[pos + 0] = command();
        result[pos + 1] = _register;
    }
    virtual size_t size() const { return 2; }
};

class cc_alu_inversion : public cc_alu_single_operation {
public:
    virtual vbyte command() const { return CMD_ALU_INV; }
    cc_alu_inversion(vbyte in_register) : cc_alu_single_operation(in_register) {}
};

class cc_alu_increment : public cc_alu_single_operation {
public:
    virtual vbyte command() const { return CMD_ALU_INC; }
    cc_alu_increment(vbyte in_register) : cc_alu_single_operation(in_register) {}
};

class cc_alu_decrement : public cc_alu_single_operation {
public:
    virtual vbyte command() const { return CMD_ALU_DEC; }
    cc_alu_decrement(vbyte in_register) : cc_alu_single_operation(in_register) {}
};

class label_map {
protected:
    typedef struct { vbyte* pos; size_t offset; bit_width width; } CacheStruct;
    typedef std::unordered_map<std::string, size_t> LabelMap;
    typedef std::unordered_multimap<std::string, CacheStruct> LabelMultimap;
    LabelMap _map;
    LabelMultimap _cache;

    void internSet(size_t index, vbyte* pos, size_t offset, bit_width width) {
        vvalue val(index, width);
        for(int i = 0; i < width; i++) {
            switch (width) {
                default:
                case BIT_8:  pos[offset + i] = val._8.bytes[i];      break;
                case BIT_16: pos[offset + i] = val._16.bytes[1 - i]; break;
                case BIT_32: pos[offset + i] = val._32.bytes[3 - i]; break;
                case BIT_64: pos[offset + i] = val._64.bytes[7 - i]; break;
            }
        }
    }

public:

    void insert(std::string label, size_t index) {
        if(_map.count(label)) return; // TODO: Throw exception if label exists
        _map[label] = index;
        std::pair<LabelMultimap::iterator, LabelMultimap::iterator> range = _cache.equal_range(label);
        LabelMultimap::iterator it = range.first;
        while(it != range.second) {
            internSet(index, it->second.pos, it->second.offset, it->second.width);
            it++;
        }
        _cache.erase(range.first, range.second);
    }

    void setIndex(std::string label, vbyte* pos, size_t offset, bit_width width) {
        if(_map.count(label)) internSet(_map[label], pos, offset, width);
        else _cache.insert( {{ label, { pos, offset, width }}} );
    }

    size_t size() const { return _map.size(); }
    size_t cacheSize() const { return _cache.size(); }
};

class cc_label : public compiler_command {
protected:
    label_map &_map;
    std::string _label;
public:
    cc_label(label_map &map, const std::string &label) : _map(map), _label(label) {}
    virtual void compile(vbyte* result, size_t pos) const {
        _map.insert(_label, pos);
    }
    virtual size_t size() const { return 0; }
    virtual vbyte command() const { return CMD_NOP; }
};

class cc_jump_operation : public compiler_command {
protected:
    label_map &_map;
    std::string _label;
    virtual size_t addressOffset() const = 0;
public:
    cc_jump_operation(label_map &map, const std::string &label) : _map(map), _label(label) {}
    virtual void compile(vbyte* result, size_t pos) const {
        result[pos + 0] = (vbyte) (command() | widthFlag(BIT_64));
        _map.setIndex(_label, result, pos + addressOffset(), BIT_64);
    }
    virtual size_t size() const { return (size_t)(addressOffset()+BIT_64); }
};

class cc_jump : public cc_jump_operation {
protected:
    virtual size_t addressOffset() const { return 1; }
public:
    virtual vbyte command() const { return CMD_JMP; }
    cc_jump(label_map &map, const std::string &label)
            : cc_jump_operation(map, label) {}
};

class cc_jump_equal : public cc_jump_operation {
protected:
    vbyte _register_a;
    vbyte _register_b;
    virtual size_t addressOffset() const { return 3; }
public:
    virtual vbyte command() const { return CMD_JMP_EQL; }
    cc_jump_equal(label_map &map, const std::string &label, vbyte register_a, vbyte register_b)
            : cc_jump_operation(map, label), _register_a(register_a), _register_b(register_b) {}
    virtual void compile(vbyte* result, size_t pos) const {
        cc_jump_operation::compile(result, pos);
        result[pos + 1] = _register_a;
        result[pos + 2] = _register_b;
    }
};

class cc_jump_not_equal : public cc_jump_operation {
protected:
    vbyte _register_a;
    vbyte _register_b;
    virtual size_t addressOffset() const { return 3; }
public:
    virtual vbyte command() const { return CMD_JMP_NEQL; }
    cc_jump_not_equal(label_map &map, const std::string &label, vbyte register_a, vbyte register_b)
            : cc_jump_operation(map, label), _register_a(register_a), _register_b(register_b) {}
    virtual void compile(vbyte* result, size_t pos) const {
        cc_jump_operation::compile(result, pos);
        result[pos + 1] = _register_a;
        result[pos + 2] = _register_b;
    }
};

class cc_jump_less : public cc_jump_operation {
protected:
    vbyte _register_a;
    vbyte _register_b;
    virtual size_t addressOffset() const { return 3; }
public:
    virtual vbyte command() const { return CMD_JMP_LESS; }
    cc_jump_less(label_map &map, const std::string &label, vbyte register_a, vbyte register_b)
            : cc_jump_operation(map, label), _register_a(register_a), _register_b(register_b) {}
    virtual void compile(vbyte* result, size_t pos) const {
        cc_jump_operation::compile(result, pos);
        result[pos + 1] = _register_a;
        result[pos + 2] = _register_b;
    }
};