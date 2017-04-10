#include "virtual_environment.h"

#include "ve_commands.h"

#include <bitset>
#include <iostream>

//#define DEBUG_MODE

#if defined(DEBUG_MODE)
#define DEBUG_PRINT_CMD(CNT,BIN) std::cout << CNT << ":\t" << std::bitset<8>(BIN) << std::endl
#define DEBUG_PRINT(STR) std::cout << STR << std::endl;
#else
#define DEBUG_PRINT_CMD(CNT,BIN)
#define DEBUG_PRINT(STR)
#endif

retcode virtual_environment::run() {
    retcode rc = _program.run(*this);
    return rc;
}

retcode ve_program::run(virtual_environment &ve) {
    if(_data == nullptr) return RET_UNEXPECTED_END;
    _counter = 0;
    while(_counter < _size) {
        vbyte cmd = _data[_counter];
        DEBUG_PRINT_CMD(_counter,(int)cmd);

        // [NOP]
        if(cmd == CMD_NOP) {
            _counter++;
            continue;
        }

        // [HALT]
        if(cmd == CMD_HALT) return RET_HALTED;

        // Register Commands
        if((cmd & 0b11000000) == 0b11000000) {
            switch(cmd & 0b00110000) {
                case 0b000000: { // [MVTOREG]
                    bit_width width;
                    switch(cmd & 0b00000011) {
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                        default: width = BIT_8; break;
                    }
                    if (_size - _counter < 2) return RET_UNEXPECTED_END;
                    ve_register &reg_data = ve.getRegister(_data[++_counter]);
                    ve_register &reg_pos = ve.getRegister(_data[++_counter]);
                    size_t mem_pos = reg_pos._data.getu();
                    vbyte mem_data[width];
                    ve_memory &mem = ve.getMemory();
                    for(vbyte i = 0; i < width; i++) {
                        if(mem_pos+i < mem._size_in_bytes) mem_data[i] = mem._data[mem_pos+i];
                        else mem_data[i] = 0;
                    }
                    reg_data._data = vvalue(mem_data, width).get(); // Don't want to change bitwidth of output register
                } break;
                case 0b010000: { // [MVTOMEM]
                    bit_width width;
                    switch(cmd & 0b00000011) {
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                        default: width = BIT_8; break;
                    }
                    if (_size - _counter < 2) return RET_UNEXPECTED_END;
                    ve_register &reg_data = ve.getRegister(_data[++_counter]);
                    ve_register &reg_pos = ve.getRegister(_data[++_counter]);
                    size_t mem_pos = reg_pos._data.getu();
                    ve_memory &mem = ve.getMemory();
                    bit_width least_width = width;
                    if(reg_data._width < least_width) least_width = reg_data._width;
                    for(size_t i = 0; i < least_width; i++) {
                        if(mem_pos+i < mem._size_in_bytes) {
                            vbyte* tdata;
                            switch(reg_data._width) {
                                default:
                                case BIT_8:  tdata = &reg_data._data._8.bytes[0];  break;
                                case BIT_16: tdata = &reg_data._data._16.bytes[0]; break;
                                case BIT_32: tdata = &reg_data._data._32.bytes[0]; break;
                                case BIT_64: tdata = &reg_data._data._64.bytes[0]; break;
                            }
                            switch(least_width) {
                                default:
                                case BIT_8:  mem._data[mem_pos+i] = tdata[i];   break;
                                case BIT_16: mem._data[mem_pos+i] = tdata[1-i]; break;
                                case BIT_32: mem._data[mem_pos+i] = tdata[3-i]; break;
                                case BIT_64: mem._data[mem_pos+i] = tdata[7-i]; break;
                            }
                        }
                    }
                } break;
                case 0b100000: { // Other Register Command
                    switch(cmd & 0b00001100) {
                        case 0b0000: { // [LDCONST]
                            bit_width width;
                            switch(cmd & 0b00000011) {
                                case 0b00: width = BIT_8; break;
                                case 0b01: width = BIT_16; break;
                                case 0b10: width = BIT_32; break;
                                case 0b11: width = BIT_64; break;
                                default: width = BIT_8; break;
                            }
                            if (_size - _counter < 1 + width) return RET_UNEXPECTED_END;
                            ve_register &reg_out = ve.getRegister(_data[++_counter]);
                            vbyte byte_in[width];
                            for(vbyte i = 0; i < width; i++) byte_in[i] = _data[++_counter];
                            reg_out._data = vvalue(byte_in, width).get();
                        } break;
                        case 0b0100: { // [CPREG]
                            if (_size - _counter < 2) return RET_UNEXPECTED_END;
                            ve_register &reg_in = ve.getRegister(_data[++_counter]);
                            ve_register &reg_out = ve.getRegister(_data[++_counter]);
                            reg_out._data = reg_in._data.get(); // Don't want to change bitwidth of output register
                        } break;
                        default: return RET_UNKNOWN_COMMAND;
                    }
                } break;
                default: return RET_UNKNOWN_COMMAND;
            }
            _counter++;
            continue;
        }

        // ALU Commands
        if((cmd & 0b11110000) == 0b00010000) {
            switch(cmd & 0b00001111) {
                case 0b0000: { // [ADD]
                    if (_size - _counter < 3) return RET_UNEXPECTED_END;
                    ve_register &reg_in_1 = ve.getRegister(_data[++_counter]);
                    ve_register &reg_in_2 = ve.getRegister(_data[++_counter]);
                    ve_register &reg_out  = ve.getRegister(_data[++_counter]);
                    reg_out._data = reg_in_1._data.get() + reg_in_2._data.get();
                } break;
                case 0b0001: { // [SUB]
                    if (_size - _counter < 3) return RET_UNEXPECTED_END;
                    ve_register &reg_in_1 = ve.getRegister(_data[++_counter]);
                    ve_register &reg_in_2 = ve.getRegister(_data[++_counter]);
                    ve_register &reg_out  = ve.getRegister(_data[++_counter]);
                    reg_out._data = reg_in_1._data.get() - reg_in_2._data.get();
                } break;
                case 0b0010: { // [MULT]
                    if (_size - _counter < 3) return RET_UNEXPECTED_END;
                    ve_register &reg_in_1 = ve.getRegister(_data[++_counter]);
                    ve_register &reg_in_2 = ve.getRegister(_data[++_counter]);
                    ve_register &reg_out  = ve.getRegister(_data[++_counter]);
                    reg_out._data = reg_in_1._data.get() * reg_in_2._data.get();
                } break;
                case 0b0011: { // [DIV]
                    if (_size - _counter < 3) return RET_UNEXPECTED_END;
                    ve_register &reg_in_1 = ve.getRegister(_data[++_counter]);
                    ve_register &reg_in_2 = ve.getRegister(_data[++_counter]);
                    ve_register &reg_out  = ve.getRegister(_data[++_counter]);
                    reg_out._data = reg_in_1._data.get() / reg_in_2._data.get();
                } break;
                case 0b0100: { // [INV]
                    if (_size - _counter < 1) return RET_UNEXPECTED_END;
                    ve_register &reg = ve.getRegister(_data[++_counter]);
                    reg._data = reg._data.get() * -1;
                } break;
                case 0b0110: { // [INC]
                    if (_size - _counter < 1) return RET_UNEXPECTED_END;
                    ve_register &reg = ve.getRegister(_data[++_counter]);
                    reg._data = reg._data.get() + 1;
                } break;
                case 0b0111: { // [DEC]
                    if (_size - _counter < 1) return RET_UNEXPECTED_END;
                    ve_register &reg = ve.getRegister(_data[++_counter]);
                    reg._data = reg._data.get() - 1;
                } break;
                default: return RET_UNKNOWN_COMMAND;
            }
            _counter++;
            continue;
        }

        // JUMP Commands
        if((cmd & 0b11000000) == 0b01000000) {
            switch(cmd & 0b00111000) {
                case 0b000000: { // [JMP]
                    bit_width width;
                    switch (cmd & 0b00000011) {
                        default:
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                    }
                    if (_size - _counter < width) return RET_UNEXPECTED_END;
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _data[++_counter];
                    vvalue loc_val(loc_data, width);
                    if(cmd & CMD_JUMP_RELATIVE) {
                        int64_t relative = loc_val.get();
                        if(_counter + relative < 0) return RET_JUMP_OUT_OF_RANGE;
                        if(_counter + relative >= _size) return RET_JUMP_OUT_OF_RANGE;
                        _counter += relative;
                        continue;
                    } else {
                        uint64_t location = loc_val.getu();
                        if(location >= _size) return RET_JUMP_OUT_OF_RANGE;
                        _counter = location;
                        continue;
                    }
                } break;
                case 0b001000: { // [JMP_LESS]
                    bit_width width;
                    switch (cmd & 0b00000011) {
                        default:
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                    }
                    if (_size - _counter < 2 + width) return RET_UNEXPECTED_END;
                    ve_register &reg1 = ve.getRegister(_data[++_counter]);
                    ve_register &reg2 = ve.getRegister(_data[++_counter]);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _data[++_counter];
                    vvalue loc_val(loc_data, width);
                    if(reg1._data.get() < reg2._data.get()) {
                        if (cmd & CMD_JUMP_RELATIVE) {
                            int64_t relative = loc_val.get();
                            if (_counter + relative < 0) return RET_JUMP_OUT_OF_RANGE;
                            if (_counter + relative >= _size) return RET_JUMP_OUT_OF_RANGE;
                            _counter += relative;
                            continue;
                        } else {
                            uint64_t location = loc_val.getu();
                            if (location >= _size) return RET_JUMP_OUT_OF_RANGE;
                            _counter = location;
                            continue;
                        }
                    }
                } break;
                case 0b010000: { // [JMP_EQL]
                    bit_width width;
                    switch (cmd & 0b00000011) {
                        default:
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                    }
                    if (_size - _counter < 2 + width) return RET_UNEXPECTED_END;
                    ve_register &reg1 = ve.getRegister(_data[++_counter]);
                    ve_register &reg2 = ve.getRegister(_data[++_counter]);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _data[++_counter];
                    vvalue loc_val(loc_data, width);
                    if(reg1._data.get() == reg2._data.get()) {
                        if (cmd & CMD_JUMP_RELATIVE) {
                            int64_t relative = loc_val.get();
                            if (_counter + relative < 0) return RET_JUMP_OUT_OF_RANGE;
                            if (_counter + relative >= _size) return RET_JUMP_OUT_OF_RANGE;
                            _counter += relative;
                            continue;
                        } else {
                            uint64_t location = loc_val.getu();
                            if (location >= _size) return RET_JUMP_OUT_OF_RANGE;
                            _counter = location;
                            continue;
                        }
                    }
                } break;
                case 0b011000: { // [JMP_NEQL]
                    bit_width width;
                    switch (cmd & 0b00000011) {
                        default:
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                    }
                    if (_size - _counter < 2 + width) return RET_UNEXPECTED_END;
                    ve_register &reg1 = ve.getRegister(_data[++_counter]);
                    ve_register &reg2 = ve.getRegister(_data[++_counter]);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _data[++_counter];
                    vvalue loc_val(loc_data, width);
                    if(reg1._data.get() != reg2._data.get()) {
                        if (cmd & CMD_JUMP_RELATIVE) {
                            int64_t relative = loc_val.get();
                            if (_counter + relative < 0) return RET_JUMP_OUT_OF_RANGE;
                            if (_counter + relative >= _size) return RET_JUMP_OUT_OF_RANGE;
                            _counter += relative;
                            continue;
                        } else {
                            uint64_t location = loc_val.getu();
                            if (location >= _size) return RET_JUMP_OUT_OF_RANGE;
                            _counter = location;
                            continue;
                        }
                    }
                } break;
                default: return RET_UNKNOWN_COMMAND;
            }
            _counter++;
            continue;
        }

        // Command Not Known
        return RET_UNKNOWN_COMMAND;
    }

    return RET_SUCCESS;
}

void virtual_environment::printRegisters() {
    std::cout << "Registers:" << std::endl;
    for(vbyte i = 0; i < _register_count; i++) std::cout << (int)i << ":\t[" << (int64_t)_registries[i]._data.get() << "]" << std::endl;
}

void virtual_environment::printMemory() {
    std::cout << "Memory:" << std::endl;
    for(size_t i = 0; i < _memory._size_in_bytes;) {
        for(int j = 0; j < 8; j++)
            if(i < _memory._size_in_bytes) std::cout << std::bitset<8>(_memory._data[i++]).to_string() << "   ";
        std::cout << std::endl;
    }
}