#include "virtual_environment.h"

#include "ve_commands.h"

retcode virtual_environment::run() {
    retcode rc = _program.run(*this);
    return rc;
}

ve_register &ve_program::getRegister(virtual_environment &ve, vbyte id) {
    switch(id) {
        case (vbyte)SWM_REG_STACK: return _stack;
        case (vbyte)SWM_REG_COUNTER: return _counter;
        default : return ve.getRegister(id);
    }
}

retcode ve_program::run(virtual_environment &ve) {
    size_t stack_begin, stack_end;
    vbyte* stack_mem = ve.getMemory().allocMemChunk( ve.getStackSizeInBytes(), &stack_begin, &stack_end );
    size_t heap_begin, heap_end;
    vbyte* heap_mem = ve.getMemory().allocMemChunk( _required_memory_size, &heap_begin, &heap_end );
    retcode rc = run( ve, stack_mem, heap_mem, ve.getStackSizeInBytes() );
    ve.getMemory().freeMemChunk(stack_begin, stack_end);
    ve.getMemory().freeMemChunk(heap_begin, heap_end);
    return rc;
}

retcode ve_program::run(virtual_environment &ve, vbyte* stack_mem, vbyte* heap_mem, size_t stack_size) {
    if(_exec == nullptr) return SWM_RET_UNEXPECTED_END;

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
        if(cmd == CMD_HALT) return SWM_RET_HALTED;

        // Register Commands
        if((cmd & 0b11000000) == 0b11000000) {
            switch(cmd & 0b00110000) {
                case 0b000000: // [MVTOREG]
                    if(cmd & 0b00001100) { // Other Register Command
                        switch(cmd & 0b00001100) {
                            case 0b0100: { // [LDCONST]

                                // Get Width of Value from the command
                                BitWidth width;
                                switch(cmd & 0b00000011) {
                                    case 0b00: width = BIT_8; break;
                                    case 0b01: width = BIT_16; break;
                                    case 0b10: width = BIT_32; break;
                                    case 0b11: width = BIT_64; break;
                                    default: width = BIT_8; break;
                                }

                                DEBUG_PRINT("LDCONST");

                                // Check for EOF
                                if (_size - _counter < 1 + width) return SWM_RET_UNEXPECTED_END;

                                // Get Register to Load into
                                ve_register &reg_out = getRegister(ve, _exec[++_counter]);

                                // Get Constant Value
                                vbyte byte_in[width];
                                for(vbyte i = 0; i < width; i++) byte_in[i] = _exec[++_counter];

                                // Set Data to Register; use .get() instead of direct assignment to keep original register bitwidth
                                reg_out._data = VariableValue(byte_in, width).get();
                            } break;
                            case 0b1000: { // [CPREG]

                                // Check for EOF
                                if (_size - _counter < 2) return SWM_RET_UNEXPECTED_END;

                                // Get Registers
                                ve_register &reg_in = getRegister(ve, _exec[++_counter]);
                                ve_register &reg_out = getRegister(ve, _exec[++_counter]);

                                DEBUG_PRINT("CPREG FromVal=" << reg_in._data.get() << ", ToVal=" << reg_out._data.get());

                                // Copy Data from first Register to second; use .get() instead of direct assignment to keep original register bitwidth
                                reg_out._data = reg_in._data.get();
                                DEBUG_PRINT("Result=" << reg_out._data.get());
                            } break;
                            default: return SWM_RET_UNKNOWN_COMMAND;
                        }
                        break;
                    } // Else continue
                case 0b100000: // [MVTOREG_CONST]
                {
                    // Get Width of Value from the command
                    BitWidth width;
                    switch(cmd & 0b00000011) {
                        default:
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                    }
                    // Get Width of Address from the command; not used for MVTOREG
                    BitWidth width_const;
                    switch(cmd & 0b00001100) {
                        default:
                        case 0b0000: width_const = BIT_8; break;
                        case 0b0100: width_const = BIT_16; break;
                        case 0b1000: width_const = BIT_32; break;
                        case 0b1100: width_const = BIT_64; break;
                    }
                    bool flag_const = (cmd & 0b100000) != 0;
                    DEBUG_PRINT( (flag_const ? "MVTOREG_CONST" : "MVTOREG") );

                    // Check for EOF
                    if ( (_size - _counter) < (1 + (flag_const ? width_const : 1)) ) return SWM_RET_UNEXPECTED_END;

                    // Get the Register to move Data to
                    ve_register &reg_data = getRegister(ve, _exec[++_counter]);

                    // Get Memory Info
                    size_t mem_pos;
                    vbyte* mem;
                    size_t max_size;
                    if(flag_const) {
                        vbyte byte_in[width];
                        for(vbyte i = 0; i < width_const; i++) byte_in[i] = _exec[++_counter];
                        mem_pos = VariableValue(byte_in, width).getu();
                        mem = heap_mem;
                        max_size = _required_memory_size;
                        //mem = ve.getMemory()._data;
                        //max_size = ve.getMemory()._size_in_bytes;
                    } else {
                        ve_register &reg_pos = getRegister(ve, _exec[++_counter]);
                        mem_pos = reg_pos._data.getu();
                        if(&reg_pos == &_stack) {
                            mem = stack_mem;
                            max_size = stack_size;
                        } else {
                            mem = heap_mem;
                            max_size = _required_memory_size;
                            //mem = ve.getMemory()._data;
                            //max_size = ve.getMemory()._size_in_bytes;
                        }
                    }

                    // Get Memory Data
                    vbyte mem_data[width];
                    for(vbyte i = 0; i < width; i++) {
                        if(mem_pos+i < max_size) mem_data[i] = mem[mem_pos+i];
                        else mem_data[i] = 0;
                    }

                    // Set Data to Register; use .get() instead of direct assignment to keep original register bitwidth
                    reg_data._data = VariableValue(mem_data, width).get();
                } break;
                case 0b010000: // [MVTOMEM]
                case 0b110000: // [MVTOMEM_CONST]
                {
                    // Get Width of Value from the command
                    BitWidth width;
                    switch(cmd & 0b00000011) {
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                        default: width = BIT_8; break;
                    }
                    // Get Width of Address from the command; not used for MVTOMEM
                    BitWidth width_const;
                    switch(cmd & 0b00001100) {
                        default:
                        case 0b0000: width_const = BIT_8; break;
                        case 0b0100: width_const = BIT_16; break;
                        case 0b1000: width_const = BIT_32; break;
                        case 0b1100: width_const = BIT_64; break;
                    }
                    bool flag_const = (cmd & 0b100000) != 0;
                    DEBUG_PRINT( (flag_const ? "MVTOMEM_CONST" : "MVTOMEM") );

                    // Check for EOF
                    if ( (_size - _counter) < (1 + (flag_const ? width_const : 1)) ) return SWM_RET_UNEXPECTED_END;

                    // Get the Register to move Data to
                    ve_register &reg_data = getRegister(ve, _exec[++_counter]);

                    // Get Memory Info
                    size_t mem_pos;
                    vbyte* mem;
                    size_t max_size;
                    if(flag_const) {
                        vbyte byte_in[width];
                        for(vbyte i = 0; i < width_const; i++) byte_in[i] = _exec[++_counter];
                        mem_pos = VariableValue(byte_in, width).getu();
                        mem = heap_mem;
                        max_size = _required_memory_size;
                        //mem = ve.getMemory()._data;
                        //max_size = ve.getMemory()._size_in_bytes;
                    } else {
                        ve_register &reg_pos = getRegister(ve, _exec[++_counter]);
                        mem_pos = reg_pos._data.getu();
                        if(&reg_pos == &_stack) {
                            mem = stack_mem;
                            max_size = stack_size;
                        } else {
                            mem = heap_mem;
                            max_size = _required_memory_size;
                            //mem = ve.getMemory()._data;
                            //max_size = ve.getMemory()._size_in_bytes;
                        }
                    }

                    // Get Register Data
                    vbyte* tdata;
                    switch(reg_data._width) {
                        default:
                        case BIT_8:  tdata = &reg_data._data._8 .bytes[0]; break;
                        case BIT_16: tdata = &reg_data._data._16.bytes[0]; break;
                        case BIT_32: tdata = &reg_data._data._32.bytes[0]; break;
                        case BIT_64: tdata = &reg_data._data._64.bytes[0]; break;
                    }

                    // Set to Memory
                    BitWidth least_width = width;
                    if(reg_data._width < least_width) least_width = reg_data._width;
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
                default: return SWM_RET_UNKNOWN_COMMAND;
            }
            _counter++;
            continue;
        }

        // ALU Commands
        if((cmd & 0b11000000) == 0b01000000) {
            switch(cmd & 0b00110000) {
                case 0b000000: { // Basic ALU Commands
                    switch(cmd & 0b00001111) {
                        case 0b0000: { // [ADD]
                            DEBUG_PRINT("ALU_ADD");
                            if (_size - _counter < 3) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                            DEBUG_PRINT("A=" << reg_in_1._data.get() << ", B=" << reg_in_2._data.get());
                            reg_out._data = reg_in_1._data.get() + reg_in_2._data.get();
                            DEBUG_PRINT("C=" << reg_out._data.get());
                        } break;
                        case 0b0001: { // [SUB]
                            DEBUG_PRINT("ALU_SUB");
                            if (_size - _counter < 3) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                            DEBUG_PRINT("A=" << reg_in_1._data.get() << ", B=" << reg_in_2._data.get());
                            reg_out._data = reg_in_1._data.get() - reg_in_2._data.get();
                            DEBUG_PRINT("C=" << reg_out._data.get());
                        } break;
                        case 0b0010: { // [MULT]
                            DEBUG_PRINT("ALU_MULT");
                            if (_size - _counter < 3) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                            DEBUG_PRINT("A=" << reg_in_1._data.get() << ", B=" << reg_in_2._data.get());
                            reg_out._data = reg_in_1._data.get() * reg_in_2._data.get();
                            DEBUG_PRINT("C=" << reg_out._data.get());
                        } break;
                        case 0b0011: { // [DIV]
                            DEBUG_PRINT("ALU_DIV");
                            if (_size - _counter < 3) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                            DEBUG_PRINT("A=" << reg_in_1._data.get() << ", B=" << reg_in_2._data.get());
                            reg_out._data = reg_in_1._data.get() / reg_in_2._data.get();
                            DEBUG_PRINT("C=" << reg_out._data.get());
                        } break;
                        case 0b0100: { // [MOD]
                            DEBUG_PRINT("ALU_MOD");
                            if (_size - _counter < 3) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg_in_1 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_in_2 = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out  = getRegister(ve, _exec[++_counter]);
                            DEBUG_PRINT("A=" << reg_in_1._data.get() << ", B=" << reg_in_2._data.get());
                            reg_out._data = reg_in_1._data.get() % reg_in_2._data.get();
                            DEBUG_PRINT("C=" << reg_out._data.get());
                        } break;
                        case 0b0101: { // [INV]
                            DEBUG_PRINT("ALU_INV");
                            if (_size - _counter < 1) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg = getRegister(ve, _exec[++_counter]);
                            reg._data = reg._data.get() * -1;
                            DEBUG_PRINT("Result=" << reg._data.get());
                        } break;
                        case 0b0110: { // [INC]
                            DEBUG_PRINT("ALU_INC");
                            if (_size - _counter < 1) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg = getRegister(ve, _exec[++_counter]);
                            reg._data = reg._data.get() + 1;
                            DEBUG_PRINT("Result=" << reg._data.get());
                        } break;
                        case 0b0111: { // [DEC]
                            DEBUG_PRINT("ALU_DEC");
                            if (_size - _counter < 1) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg = getRegister(ve, _exec[++_counter]);
                            reg._data = reg._data.get() - 1;
                            DEBUG_PRINT("Result=" << reg._data.get());
                        } break;
                        default: return SWM_RET_UNKNOWN_COMMAND;
                    }
                } break;
                case 0b010000: { // Single Move Commands
                    switch(cmd & 0b00001111) {
                        case 0b0101: { // [INV_MV]
                            DEBUG_PRINT("ALU_INV_MV");
                            if (_size - _counter < 2) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg_in  = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out = getRegister(ve, _exec[++_counter]);
                            reg_out._data = reg_in._data.get() * -1;
                            DEBUG_PRINT("Result=" << reg_out._data.get());
                        } break;
                        case 0b0110: { // [INC_MV]
                            DEBUG_PRINT("ALU_INC_MV");
                            if (_size - _counter < 2) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg_in  = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out = getRegister(ve, _exec[++_counter]);
                            reg_out._data = reg_in._data.get() + 1;
                            DEBUG_PRINT("Result=" << reg_out._data.get());
                        } break;
                        case 0b0111: { // [DEC_MV]
                            DEBUG_PRINT("ALU_DEC_MV");
                            if (_size - _counter < 2) return SWM_RET_UNEXPECTED_END;
                            ve_register &reg_in  = getRegister(ve, _exec[++_counter]);
                            ve_register &reg_out = getRegister(ve, _exec[++_counter]);
                            reg_out._data = reg_in._data.get() - 1;
                            DEBUG_PRINT("Result=" << reg_out._data.get());
                        } break;
                        default: return SWM_RET_UNKNOWN_COMMAND;
                    }
                } break;
                case 0b100000:
                case 0b110000: // Double Constant Commands
                {
                    // Get Width of Constant from the command
                    BitWidth width;
                    switch(cmd & 0b00000011) {
                        default:
                        case 0b00: width = BIT_8; break;
                        case 0b01: width = BIT_16; break;
                        case 0b10: width = BIT_32; break;
                        case 0b11: width = BIT_64; break;
                    }

                    // Check for EOF
                    if (_size - _counter < (2+width)) return SWM_RET_UNEXPECTED_END;

                    // Get Registers
                    ve_register &reg_in  = getRegister(ve, _exec[++_counter]);
                    ve_register &reg_out = getRegister(ve, _exec[++_counter]);
                    
                    // Get Constant Value
                    vbyte byte_in[width];
                    for(vbyte i = 0; i < width; i++) byte_in[i] = _exec[++_counter];
                    VariableValue const_val(byte_in, width);

                    DEBUG_PRINT("A=" << reg_in._data.get() << ", B=" << const_val.get());

                    // Perform Operation
                    switch(cmd & 0b00011100) {
                        case 0b00000: // [ADD_CONST]
                            DEBUG_PRINT("ALU_ADD_CONST");
                            reg_out._data = reg_in._data.get() + const_val.get();
                            break;
                        case 0b00100: // [SUB_CONST_RHS]
                            DEBUG_PRINT("ALU_SUB_CONST_RHS");
                            reg_out._data = reg_in._data.get() - const_val.get();
                            break;
                        case 0b01000: // [SUB_CONST_LHS]
                            DEBUG_PRINT("ALU_SUB_CONST_LHS");
                            reg_out._data = const_val.get() - reg_in._data.get();
                            break;
                        case 0b01100: // [MULT_CONST]
                            DEBUG_PRINT("ALU_MULT_CONST");
                            reg_out._data = reg_in._data.get() * const_val.get();
                            break;
                        case 0b10000: // [DIV_CONST_RHS]
                            DEBUG_PRINT("ALU_DIV_CONST_RHS");
                            reg_out._data = reg_in._data.get() / const_val.get();
                            break;
                        case 0b10100: // [DIV_CONST_LHS]
                            DEBUG_PRINT("ALU_DIV_CONST_LHS");
                            reg_out._data = const_val.get() / reg_in._data.get();
                            break;
                        case 0b11000: // [MOD_CONST_RHS]
                            DEBUG_PRINT("ALU_MOD_CONST_RHS");
                            reg_out._data = reg_in._data.get() % const_val.get();
                            break;
                        case 0b11100: // [MOD_CONST_LHS]
                            DEBUG_PRINT("ALU_MOD_CONST_LHS");
                            reg_out._data = const_val.get() % reg_in._data.get();
                            break;
                        default: return SWM_RET_UNKNOWN_COMMAND;
                    }

                    DEBUG_PRINT("Result=" << reg_out._data.get());
                } break;
                default: return SWM_RET_UNKNOWN_COMMAND;
            }
            _counter++;
            continue;
        }

        // JUMP Commands
        if((cmd & 0b11100000) == 0b00100000) {

            // Get Width of Constant from the command
            BitWidth width;
            switch (cmd & 0b00000011) {
                default:
                case 0b00: width = BIT_8; break;
                case 0b01: width = BIT_16; break;
                case 0b10: width = BIT_32; break;
                case 0b11: width = BIT_64; break;
            }

            switch(cmd & 0b00011000) {
                case 0b00000: { // [JMP]
                    DEBUG_PRINT("JMP");
                    if (_size - _counter < width) return SWM_RET_UNEXPECTED_END;
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _exec[++_counter];
                    VariableValue loc_val(loc_data, width);
                    if(cmd & CMD_JUMP_RELATIVE) {
                        int64_t relative = loc_val.get();
                        if(_counter + relative < 0) return SWM_RET_JUMP_OUT_OF_RANGE;
                        if(_counter + relative >= _size) return SWM_RET_JUMP_OUT_OF_RANGE;
                        _counter += relative;
                        continue;
                    } else {
                        uint64_t location = loc_val.getu();
                        DEBUG_PRINT("Jump Address: " << location);
                        if(location >= _size) return SWM_RET_JUMP_OUT_OF_RANGE;
                        _counter = location;
                        continue;
                    }
                } break;
                case 0b01000: { // [JMP_LESS]
                    DEBUG_PRINT("JMP_LESS");
                    if (_size - _counter < 2 + width) return SWM_RET_UNEXPECTED_END;
                    vbyte regID1 = _exec[++_counter];
                    vbyte regID2 = _exec[++_counter];
                    DEBUG_PRINT("Registers: " << (int)regID1 << " : " << (int)regID2);
                    ve_register &reg1 = getRegister(ve, regID1);
                    ve_register &reg2 = getRegister(ve, regID2);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _exec[++_counter];
                    VariableValue loc_val(loc_data, width);
                    if(reg1._data.get() < reg2._data.get()) {
                        if (cmd & CMD_JUMP_RELATIVE) {
                            int64_t relative = loc_val.get();
                            if (_counter + relative < 0) return SWM_RET_JUMP_OUT_OF_RANGE;
                            if (_counter + relative >= _size) return SWM_RET_JUMP_OUT_OF_RANGE;
                            _counter += relative;
                            continue;
                        } else {
                            uint64_t location = loc_val.getu();
                            DEBUG_PRINT("Jump Address: " << location);
                            if (location >= _size) return SWM_RET_JUMP_OUT_OF_RANGE;
                            _counter = location;
                            continue;
                        }
                    }
                } break;
                case 0b10000: { // [JMP_EQL]
                    DEBUG_PRINT("JMP_EQL");
                    if (_size - _counter < 2 + width) return SWM_RET_UNEXPECTED_END;
                    vbyte regID1 = _exec[++_counter];
                    vbyte regID2 = _exec[++_counter];
                    DEBUG_PRINT("Registers: " << (int)regID1 << " : " << (int)regID2);
                    ve_register &reg1 = getRegister(ve, regID1);
                    ve_register &reg2 = getRegister(ve, regID2);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _exec[++_counter];
                    VariableValue loc_val(loc_data, width);
                    if(reg1._data.get() == reg2._data.get()) {
                        if (cmd & CMD_JUMP_RELATIVE) {
                            int64_t relative = loc_val.get();
                            if (_counter + relative < 0) return SWM_RET_JUMP_OUT_OF_RANGE;
                            if (_counter + relative >= _size) return SWM_RET_JUMP_OUT_OF_RANGE;
                            _counter += relative;
                            continue;
                        } else {
                            uint64_t location = loc_val.getu();
                            DEBUG_PRINT("Jump Address: " << location);
                            if (location >= _size) return SWM_RET_JUMP_OUT_OF_RANGE;
                            _counter = location;
                            continue;
                        }
                    }
                } break;
                case 0b11000: { // [JMP_NEQL]
                    DEBUG_PRINT("JMP_NEQL");
                    if (_size - _counter < 2 + width) return SWM_RET_UNEXPECTED_END;
                    vbyte regID1 = _exec[++_counter];
                    vbyte regID2 = _exec[++_counter];
                    DEBUG_PRINT("Registers: " << (int)regID1 << " : " << (int)regID2);
                    ve_register &reg1 = getRegister(ve, regID1);
                    ve_register &reg2 = getRegister(ve, regID2);
                    vbyte loc_data[width];
                    for(vbyte i = 0; i < width; i++) loc_data[i] = _exec[++_counter];
                    VariableValue loc_val(loc_data, width);
                    if(reg1._data.get() != reg2._data.get()) {
                        if (cmd & CMD_JUMP_RELATIVE) {
                            int64_t relative = loc_val.get();
                            if (_counter + relative < 0) return SWM_RET_JUMP_OUT_OF_RANGE;
                            if (_counter + relative >= _size) return SWM_RET_JUMP_OUT_OF_RANGE;
                            _counter += relative;
                            continue;
                        } else {
                            uint64_t location = loc_val.getu();
                            DEBUG_PRINT("Jump Address: " << location);
                            if (location >= _size) return SWM_RET_JUMP_OUT_OF_RANGE;
                            _counter = location;
                            continue;
                        }
                    }
                } break;
                default: return SWM_RET_UNKNOWN_COMMAND;
            }
            _counter++;
            continue;
        }

        // Command Not Known
        return SWM_RET_UNKNOWN_COMMAND;
    }

    return SWM_RET_SUCCESS;
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