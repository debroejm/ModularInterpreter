#include "ve_commands.h"
#include "virtual_environment.h"

#include <iostream>

int main() {

    vbyte fibonacci[]{
            /* 0  */ CMD_LDCONST | CMD_PRECISION_1B, 0, 1,
            /* 3  */ CMD_LDCONST | CMD_PRECISION_1B, 1, 2,
            /* 6  */ CMD_LDCONST | CMD_PRECISION_2B, 7, (1000 >> 8) & 0xFF, (1000) & 0xFF,
            /* 10 */ CMD_LDCONST | CMD_PRECISION_1B, 6, 0,
            /* 13 */ CMD_LDCONST | CMD_PRECISION_1B, 5, 0,
            /* 16 */ CMD_LDCONST | CMD_PRECISION_1B, 4, 0,
            /* 19 */ CMD_LDCONST | CMD_PRECISION_1B, 3, 0,
            /* 22 */ CMD_LDCONST | CMD_PRECISION_1B, 2, 8,
            /* 25 */ CMD_JMP_NEQL | CMD_JUMP_RELATIVE | CMD_PRECISION_1B, 4, 5, 13, // 4 bytes
            /* 29 */ CMD_ALU_ADD, 0, 1, 0, // 4 bytes
            /* 33 */ CMD_MVTOMEM | CMD_PRECISION_8B, 0, 3, // 3 bytes
            /* 36 */ CMD_LDCONST | CMD_PRECISION_1B, 4, 1, // 3 bytes
            /* 39 */ CMD_JMP | CMD_JUMP_RELATIVE | CMD_PRECISION_1B, 11, // 2 bytes
            /* 41 */ CMD_ALU_ADD, 0, 1, 1, // 4 bytes
            /* 45 */ CMD_MVTOMEM | CMD_PRECISION_8B, 1, 3, // 3 bytes
            /* 48 */ CMD_LDCONST | CMD_PRECISION_1B, 4, 0, // 3 bytes
            /* 51 */ CMD_ALU_ADD, 3, 2, 3, // 4 bytes
            /* 55 */ CMD_ALU_INC, 6, // 2 bytes
            /* 57 */ CMD_JMP_LESS | CMD_JUMP_RELATIVE | CMD_PRECISION_1B, 6, 7, (vbyte)-35
    };

    virtual_environment ve(BIT_64, 8, 8, MEM_KB, 1, MEM_KB);
    ve.setProgram(ve_program(sizeof(fibonacci) / sizeof(vbyte), fibonacci, 1, MEM_KB));
    retcode result = ve.run();

    std::cout << "RetCode=" << result << std::endl;
    ve.printRegisters();
    ve.printMemory();

    return 0;
}