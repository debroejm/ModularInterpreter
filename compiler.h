#pragma once

#include "types.h"
#include "ve_commands.h"

class compiler_command {
public:
    virtual vbyte* compile() const = 0;
    virtual size_t size() const = 0;
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

class cc_nop : public compiler_command {
public:
    virtual vbyte* compile() const { return new vbyte[1]{ CMD_NOP }; }
    virtual size_t size() const { return 1; }
};

class cc_halt : public compiler_command {
public:
    virtual vbyte* compile() const { return new vbyte[1]{ CMD_HALT }; }
    virtual size_t size() const { return 1; }
};

class cc_move_to_register : public compiler_command {
protected:
    vbyte _target_register;
    vbyte _mem_address_register;
    bit_width _width;
public:
    cc_move_to_register(vbyte target_register, vbyte mem_address_register, bit_width width)
            : _target_register(target_register), _mem_address_register(mem_address_register), _width(width) {}
    virtual vbyte* compile() const { return new vbyte[3]{ (vbyte)(CMD_MVTOREG | widthFlag(_width)), _target_register, _mem_address_register }; }
    virtual size_t size() const { return 3; }
};

class cc_move_to_memory : public compiler_command {
protected:
    vbyte _target_register;
    vbyte _mem_address_register;
    bit_width _width;
public:
    cc_move_to_memory(vbyte target_register, vbyte mem_address_register, bit_width width)
            : _target_register(target_register), _mem_address_register(mem_address_register), _width(width) {}
    virtual vbyte* compile() const { return new vbyte[3]{ (vbyte)(CMD_MVTOMEM | widthFlag(_width)), _target_register, _mem_address_register }; }
    virtual size_t size() const { return 3; }
};

class cc_load_constant : public compiler_command {
protected:
    vbyte _target_register;
    vvalue _value;
public:
    cc_load_constant(vbyte target_register, vvalue value)
            : _target_register(target_register), _value(value) {}
    virtual vbyte* compile() const {
        vbyte* result = new vbyte[2+_value._width];
        result[0] = (vbyte)(CMD_LDCONST | widthFlag(_value._width)); result[1] = _target_register;
        for(unsigned short i = 0; i < _value._width; i++) result[2+i] = _value._64.bytes[i];
        return result;
    }
    virtual size_t size() const { return (size_t)(2+_value._width); }
};

class cc_copy_register : public compiler_command {
protected:
    vbyte _from_register;
    vbyte _to_register;
public:
    cc_copy_register(vbyte from_register, vbyte to_register)
            : _from_register(from_register), _to_register(to_register) {}
    virtual vbyte* compile() const { return new vbyte[3]{ CMD_CPREG, _from_register, _to_register };}
    virtual size_t size() const { return 3; }
};

class cc_alu_double_operation : public compiler_command {
protected:
    vbyte _in_register_a;
    vbyte _in_register_b;
    vbyte _out_register;
    virtual vbyte command() const = 0;
public:
    cc_alu_double_operation(vbyte in_register_a, vbyte in_register_b, vbyte out_register)
            : _in_register_a(in_register_a), _in_register_b(in_register_b), _out_register(out_register) {}
    virtual vbyte* compile() const { return new vbyte[4]{ command(), _in_register_a, _in_register_b, _out_register }; }
    virtual size_t size() const { return 4; }
};

class cc_alu_addition : public cc_alu_double_operation {
protected:
    virtual vbyte command() { return CMD_ALU_ADD; }
public:
    cc_alu_addition(vbyte in_register_a, vbyte in_register_b, vbyte out_register)
            : cc_alu_double_operation(in_register_a, in_register_b, out_register) {}
};














