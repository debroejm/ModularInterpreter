#pragma once

#include <bitset>
#include <iostream>
#include <list>
#include <stdint.h>

#define DEBUG_MODE

#if defined(DEBUG_MODE)
#define DEBUG_PRINT_CMD(CNT,BIN) std::cout << CNT << ":\t" << std::bitset<8>(BIN) << std::endl
#define DEBUG_PRINT(STR) std::cout << STR << std::endl;
#else
#define DEBUG_PRINT_CMD(CNT,BIN)
#define DEBUG_PRINT(STR)
#endif

#define SWM_REG_STACK (vbyte)-1
#define SWM_REG_COUNTER (vbyte)-2

typedef uint8_t     vbyte;
typedef int64_t     retcode;

struct compiler_command;
typedef std::list<compiler_command*> cc_list;
typedef std::list<compiler_command*>::iterator cc_iter;
typedef std::list<compiler_command*>::const_iterator cc_const_iter;

struct abstract_statement;
typedef std::list<abstract_statement*> stmt_list;

enum MemoryPrefix {
    MEM_BYTE = 1,
    MEM_KB = MEM_BYTE*1024,
    MEM_MB = MEM_KB*1024,
    MEM_GB = MEM_MB*1024
};

enum BitWidth {
    BIT_8 = 1,
    BIT_16 = 2,
    BIT_32 = 4,
    BIT_64 = 8
};

struct VariableValue {
    BitWidth _width;
    union {
        union {
            int8_t number;
            vbyte bytes[1];
        } _8;
        union {
            int16_t number;
            vbyte bytes[2];
        } _16;
        union {
            int32_t number;
            vbyte bytes[4];
        } _32;
        union {
            int64_t number;
            vbyte bytes[8];
        } _64;
    };

    explicit VariableValue(vbyte bytes[], BitWidth width) : _width(width) {
        switch(_width) {
            case BIT_8:
                _8.bytes[0] = bytes[0];
                break;
            case BIT_16:
                for(vbyte i = 0; i < 2; i++) _16.bytes[i] = bytes[1-i];
                break;
            case BIT_32:
                for(vbyte i = 0; i < 4; i++) _32.bytes[i] = bytes[3-i];
                break;
            case BIT_64:
                for(vbyte i = 0; i < 8; i++) _64.bytes[i] = bytes[7-i];
                break;
        }
    }

    VariableValue(int64_t number, BitWidth width) : _width(width) {
        switch(_width) {
            case BIT_8:
                _8.number = (int8_t)number;
                break;
            case BIT_16:
                _16.number = (int16_t)number;
                break;
            case BIT_32:
                _32.number = (int32_t)number;
                break;
            case BIT_64:
                _64.number = (int64_t)number;
                break;
        }
    }

    VariableValue &operator=(int64_t rhs) {
        switch(_width) {
            case BIT_8:
                _8.number = (int8_t)rhs;
                break;
            case BIT_16:
                _16.number = (int16_t)rhs;
                break;
            case BIT_32:
                _32.number = (int32_t)rhs;
                break;
            case BIT_64:
                _64.number = (int64_t)rhs;
                break;
        }
        return *this;
    }

    VariableValue &operator++() {
        switch(_width) {
            case BIT_8: ++_8.number; break;
            case BIT_16: ++_16.number; break;
            case BIT_32: ++_32.number; break;
            case BIT_64: ++_64.number; break;
            default: break;
        }
        return *this;
    }

    VariableValue operator++(int) {
        VariableValue result(*this);
        operator++();
        return result;
    }

    VariableValue &operator--() {
        switch(_width) {
            case BIT_8: --_8.number; break;
            case BIT_16: --_16.number; break;
            case BIT_32: --_32.number; break;
            case BIT_64: --_64.number; break;
            default: break;
        }
        return *this;
    }

    VariableValue operator--(int) {
        VariableValue result(*this);
        operator--();
        return result;
    }

    void operator+=(int64_t rhs) { *this = get() + rhs; }
    void operator-=(int64_t rhs) { *this = get() - rhs; }

    int64_t get() const {
        switch(_width) {
            case BIT_8: return _8.number;
            case BIT_16: return _16.number;
            case BIT_32: return _32.number;
            case BIT_64: return _64.number;
            default: return 0;
        }
    }

    uint64_t getu() const {
        switch(_width) {
            case BIT_8: return (uint8_t)_8.number;
            case BIT_16: return (uint16_t)_16.number;
            case BIT_32: return (uint32_t)_32.number;
            case BIT_64: return (uint64_t)_64.number;
            default: return 0;
        }
    }
};

struct vvariable {
    enum {
        INTEGER,
        FLOAT,
        DOUBLE,
        BOOLEAN,
        CHARACTER
    } _type;
    union {
        union {
            int64_t value;
            vbyte bytes[8];
        } _int;
        union {
            float value;
            vbyte bytes[4];
        } _float;
        union {
            double value;
            vbyte bytes[8];
        } _double;
        union {
            bool value;
            vbyte bytes[1];
        } _bool;
        union {
            char value;
            vbyte bytes[2];
        } _char;
    };
};

namespace std {
    inline std::string to_string(MemoryPrefix pre) {
        switch(pre) {
            case MEM_BYTE: return "B";
            case MEM_KB: return "KB";
            case MEM_MB: return "MB";
            case MEM_GB: return "GB";
            default: return "";
        }
    }
    inline std::string to_string(BitWidth width) {
        switch(width) {
            case BIT_8: return "8-Bit";
            case BIT_16: return "16-Bit";
            case BIT_32: return "32-Bit";
            case BIT_64: return "64-Bit";
            default: return "";
        }
    }
    inline std::string to_string(const VariableValue &val) {
        switch(val._width) {
            case BIT_8: return "[" + std::to_string(val._8.number) + ":" + std::to_string(val._width) + "]";
            case BIT_16: return "[" + std::to_string(val._16.number) + ":" + std::to_string(val._width) + "]";
            case BIT_32: return "[" + std::to_string(val._32.number) + ":" + std::to_string(val._width) + "]";
            case BIT_64: return "[" + std::to_string(val._64.number) + ":" + std::to_string(val._width) + "]";
            default: return "[?:?]";
        }
    }
}