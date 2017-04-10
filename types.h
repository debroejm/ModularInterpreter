#pragma once

#include <stdint.h>

typedef uint8_t     vbyte;
typedef int64_t     retcode;

enum memory_prefix {
    MEM_BYTE = 1,
    MEM_KB = MEM_BYTE*1024,
    MEM_MB = MEM_KB*1024,
    MEM_GB = MEM_MB*1024
};

enum bit_width {
    BIT_8 = 1,
    BIT_16 = 2,
    BIT_32 = 4,
    BIT_64 = 8
};

struct vvalue {
    bit_width _width;
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

    explicit vvalue(vbyte bytes[], bit_width width) : _width(width) {
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

    vvalue(int64_t number, bit_width width) : _width(width) {
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

    vvalue &operator=(int64_t rhs) {
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