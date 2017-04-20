#include "virtual_environment.h"

#include "ve_commands.h"

retcode virtual_environment::run() {
    retcode rc = _program.run(*this);
    return rc;
}

ve_register &ve_program::getRegister(virtual_environment &ve, vbyte id) {
    switch(id) {
        case (vbyte)REG_STACK: return _stack;
        case (vbyte)REG_COUNTER: return _counter;
        default : return ve.getRegister(id);
    }
}

retcode ve_program::run(virtual_environment &ve) {
    size_t b, e;
    vbyte* stack_mem = ve.getMemory().allocMemChunk( ve.getStackSizeInBytes(), &b, &e );
    retcode rc = run( ve, stack_mem, ve.getStackSizeInBytes() );
    ve.getMemory().freeMemChunk(b, e);
    return rc;
}

retcode ve_program::run(virtual_environment &ve, vbyte* stack_mem, size_t stack_size) {
    if(_exec == nullptr) return RET_UNEXPECTED_END;

    _counter._data = 0;
    _stack = ve_register(ve.getMaxByteWidth());

    DEBUG_PRINT("Running");
    DEBUG_PRINT("Program Size: " << _size);

    while(_counter < _size) {
        vbyte cmd = _exec[_counter];
        DEBUG_PRINT_CMD(_counter,(int)cmd);

        // [NOP]
        if(cmd == CMD_NOP) {
            ++_counter;
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
                    ve_register &reg_data = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_pos = getRegister(ve, _exec[++_counter]);
                    size_t mem_pos = reg_pos._data.getu();
                    vbyte mem_data[width];
                    vbyte* mem;
                    size_t max_size;
                    if(&reg_pos == &_stack) {
                        mem = stack_mem;
                        max_size = stack_size;
                    }
                    else {
                        mem = ve.getMemory()._data;
                        max_size = ve.getMemory()._size_in_bytes;
                    }
                    for(vbyte i = 0; i < width; i++) {
                        if(mem_pos+i < max_size) mem_data[i] = mem[mem_pos+i];
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
                    ve_register &reg_data = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_pos = getRegister(ve, _exec[++_counter]);
                    size_t mem_pos = reg_pos._data.getu();
                    vbyte* mem;
                    size_t max_size;
                    if(&reg_pos == &_stack) {
                        mem = stack_mem;
                        max_size = stack_size;
                    }
                    else {
                        mem = ve.getMemory()._data;
                        max_size = ve.getMemory()._size_in_bytes;
                    }
                    bit_width least_width = width;
                    if(reg_data._width < least_width) least_width = reg_data._width;
                    vbyte* tdata;
                    switch(reg_data._width) {
                        default:
                        case BIT_8:  tdata = &reg_data._data._8 .bytes[0]; break;
                        case BIT_16: tdata = &reg_data._data._16.bytes[0]; break;
                        case BIT_32: tdata = &reg_data._data._32.bytes[0]; break;
                        case BIT_64: tdata = &reg_data._data._64.bytes[0]; break;
                    }
                    for(size_t i = 0; i < least_width; i++) {
                        if(mem_pos+i < max_size) {
                            switch(least_width) {
                                default:
                                case BIT_8:  mem[mem_pos+i] = tdata[i];   break;
                                case BIT_16: mem[mem_pos+i] = tdata[1-i]; break;
                                case BIT_32: mem[mem_pos+i] = tdata[3-i]; break;
                                case BIT_64: mem[mem_pos+i] = tdata[7-i]; break;
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
                            ve_register &reg_out = getRegister(ve, _exec[++_counter]);
                            vbyte byte_in[width];
                            for(vbyte i = 0; i < width; i++) byte_in[i] = _exec[++_counter];
                            reg_out._data = vvalue(byte_in, width).get();
                        } break;
                        case 0b0100: { // [CPREG]
                            if (_size - _counter < 2) return RET_UNEXPECTED_END;
                            ve_register &reg_in = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out = getRegister(ve, _exec[++_counter]);
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
                    ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                    reg_out._data = reg_in_1._data.get() + reg_in_2._data.get();
                } break;
                case 0b0001: { // [SUB]
                    if (_size - _counter < 3) return RET_UNEXPECTED_END;
                    ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                    reg_out._data = reg_in_1._data.get() - reg_in_2._data.get();
                } break;
                case 0b0010: { // [MULT]
                    if (_size - _counter < 3) return RET_UNEXPECTED_END;
                    ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                    reg_out._data = reg_in_1._data.get() * reg_in_2._data.get();
                } break;
                case 0b0011: { // [DIV]
                    if (_size - _counter < 3) return RET_UNEXPECTED_END;
                    ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                    reg_out._data = reg_in_1._data.get() / reg_in_2._data.get();
                } break;
                case 0b0100: { // [MOD]
                    if (_size - _counter < 3) return RET_UNEXPECTED_END;
                    ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                    reg_out._data = reg_in_1._data.get() % reg_in_2._data.get();
                } break;
                case 0b0101: { // [INV]
                    if (_size - _counter < 1) return RET_UNEXPECTED_END;
                    ve_register &reg = getRegister(ve, _exec[++_counter]);
                    reg._data = reg._data.get() * -1;
                } break;
                case 0b0110: { // [INC]
                    if (_size - _counter < 1) return RET_UNEXPECTED_END;
                    ve_register &reg = getRegister(ve, _exec[++_counter]);
                    reg._data = reg._data.get() + 1;
                } break;
                case 0b0111: { // [DEC]
                    if (_size - _counter < 1) return RET_UNEXPECTED_END;
                    ve_register &reg = getRegister(ve, _exec[++_counter]);
                    reg._data = reg._data.get() - 1;
                } break;
                default: { // ALU constant command
                    if(cmd & 0b00000100) { // [SUB_CONST]
                        bit_width width;
                        switch(cmd & 0b00000011) {
                            case 0b00: width = BIT_8; break;
                            case 0b01: width = BIT_16; break;
                            case 0b10: width = BIT_32; break;
                            case 0b11: width = BIT_64; break;
                            default: width = BIT_8; break;
                        }
                        if (_size - _counter < 2 + width) return RET_UNEXPECTED_END;
                        ve_register &reg_in = getRegister(ve, _exec[++_counter]);
                        ve_register &reg_out = getRegister(ve, _exec[++_counter]);
                        vbyte byte_in[width];
                        for(vbyte i = 0; i < width; i++) byte_in[i] = _exec[++_counter];
                        reg_out._data = reg_in._data.get() - vvalue(byte_in, width).get();
                    } else { // [ADD_CONST]
                        bit_width width;
                        switch(cmd & 0b00000011) {
                            case 0b00: width = BIT_8; break;
                            case 0b01: width = BIT_16; break;
                            case 0b10: width = BIT_32; break;
                            case 0b11: width = BIT_64; break;
                            default: width = BIT_8; break;
                        }
                        if (_size - _counter < 2 + width) return RET_UNEXPECTED_END;
                        ve_register &reg_in = getRegister(ve, _exec[++_counter]);
                        ve_register &reg_out = getRegister(ve, _exec[++_counter]);
                        vbyte byte_in[width];
                        for(vbyte i = 0; i < width; i++) byte_in[i] = _exec[++_counter];
                        reg_out._data = reg_in._data.get() + vvalue(byte_in, width).get();
                    }
                }
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
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _exec[++_counter];
                    vvalue loc_val(loc_data, width);
                    if(cmd & CMD_JUMP_RELATIVE) {
                        int64_t relative = loc_val.get();
                        if(_counter + relative < 0) return RET_JUMP_OUT_OF_RANGE;
                        if(_counter + relative >= _size) return RET_JUMP_OUT_OF_RANGE;
                        _counter += relative;
                        continue;
                    } else {
                        uint64_t location = loc_val.getu();
                        DEBUG_PRINT("Jump Address: " << location);
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
                    vbyte regID1 = _exec[++_counter];
                    vbyte regID2 = _exec[++_counter];
                    DEBUG_PRINT("Registers: " << (int)regID1 << " : " << (int)regID2);
                    ve_register &reg1 = getRegister(ve, regID1);
                    ve_register &reg2 = getRegister(ve, regID2);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _exec[++_counter];
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
                            DEBUG_PRINT("Jump Address: " << location);
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
                    vbyte regID1 = _exec[++_counter];
                    vbyte regID2 = _exec[++_counter];
                    DEBUG_PRINT("Registers: " << (int)regID1 << " : " << (int)regID2);
                    ve_register &reg1 = getRegister(ve, regID1);
                    ve_register &reg2 = getRegister(ve, regID2);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _exec[++_counter];
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
                            DEBUG_PRINT("Jump Address: " << location);
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
                    vbyte regID1 = _exec[++_counter];
                    vbyte regID2 = _exec[++_counter];
                    DEBUG_PRINT("Registers: " << (int)regID1 << " : " << (int)regID2);
                    ve_register &reg1 = getRegister(ve, regID1);
                    ve_register &reg2 = getRegister(ve, regID2);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _exec[++_counter];
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
                            DEBUG_PRINT("Jump Address: " << location);
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