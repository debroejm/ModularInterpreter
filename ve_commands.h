#pragma once


// Precision Macros
#define CMD_PRECISION_1B        0b0000
#define CMD_PRECISION_2B        0b0001
#define CMD_PRECISION_4B        0b0010
#define CMD_PRECISION_8B        0b0011
#define CMD_PRECISION_1B_S2     0b0000
#define CMD_PRECISION_2B_S2     0b0100
#define CMD_PRECISION_4B_S2     0b1000
#define CMD_PRECISION_8B_S2     0b1100


// Jump Relative Flag
#define CMD_JUMP_RELATIVE       0b0100


// Return Codes
#define RET_NO_PROGRAM          -4
#define RET_SUCCESS             0
#define RET_HALTED              1
#define RET_UNEXPECTED_END      -8
#define RET_UNKNOWN_COMMAND     -2
#define RET_JUMP_OUT_OF_RANGE   -16


// COMMAND : No Operation [NOP] : 00000000
/* DESCRIPTION:
 *   Performs no operation. Generally useless, but may be useful in certain situations.
 */
#define CMD_NOP                 0b00000000


// COMMAND : Halt [HALT] : 11111111
/* DESCRIPTION:
 *   Halts all operations and quits from running program.
 */
#define CMD_HALT                0b11111111


// COMMAND : Move to Register [MVTOREG] : 110000aa
/* DESCRIPTION:
 *   Moves data from memory to a register. Counterpart to MVTOMEM.
 *   Register to move to is specified by next byte in sequence.
 *   Address in memory to move from is specified another register, which is specified by the second byte in sequence.
 *   [aa] represents the byte width of the data to move:
 *     00: 1 byte
 *     01: 2 byte
 *     10: 4 byte
 *     11: 8 byte
 */
#define CMD_MVTOREG             0b11000000


// COMMAND : Move to Memory [MVTOMEM] : 110100aa
/* DESCRIPTION:
 *   Moves data from a register to memory. Counterpart to MVTOREG
 *   Register to move from is specified by next byte in sequence.
 *   Address in memory to move to is specified another register, which is specified by the second byte in sequence.
 *   [aa] represents the byte width of the data to move:
 *     00: 1 byte
 *     01: 2 byte
 *     10: 4 byte
 *     11: 8 byte
 */
#define CMD_MVTOMEM             0b11010000


// COMMAND : Load Constant [LDCONST] : 111000aa
/* DESCRIPTION:
 *   Loads a constant value into a register.
 *   Register to move to is specified by the next byte in sequence.
 *   Number to load is specified by the following 1-8 bytes in sequence after register byte.
 *   [aa] represents the byte width of the data to move:
 *     00: 1 byte
 *     01: 2 byte
 *     10: 4 byte
 *     11: 8 byte
 */
#define CMD_LDCONST             0b11100000


// COMMAND : Copy Register [CPREG] : 11100100
/* DESCRIPTION:
 *   Copies the contents of one register to another.
 *   Register to copy from is specified by the next byte in sequence.
 *   Register to copy to is specified by the second byte in sequence.
 */
#define CMD_CPREG               0b11100100


// COMMAND : ALU Addition [ALU_ADD] : 00010000
/* DESCRIPTION:
 *   Performs the mathematical function of addition on two registers, and puts the output in a third.
 *   Registers to add are specified by the next two bytes, and register to output to is specified by the third.
 */
#define CMD_ALU_ADD             0b00010000


// COMMAND : ALU Subtraction [ALU_SUB] : 00010001
/* DESCRIPTION:
 *   Performs the mathematical function of subtraction on two registers, and puts the output in a third.
 *   Registers to subtract are specified by the next two bytes, and register to output to is specified by the third.
 */
#define CMD_ALU_SUB             0b00010001


// COMMAND : ALU Multiplication [ALU_MULT] : 00010010
/* DESCRIPTION:
 *   Performs the mathematical function of multiplication on two registers, and puts the output in a third.
 *   Registers to multiply are specified by the next two bytes, and register to output to is specified by the third.
 */
#define CMD_ALU_MULT            0b00010010


// COMMAND : ALU Division [ALU_DIV] : 00010011
/* DESCRIPTION:
 *   Performs the mathematical function of division on two registers, and puts the output in a third.
 *   Registers to divide are specified by the next two bytes, and register to output to is specified by the third.
 */
#define CMD_ALU_DIV             0b00010011


// COMMAND : ALU Inversion [ALU_INV] : 00010100
/* DESCRIPTION:
 *   Applies two's complement inversion on a register. Operation happens in place.
 *   Register to invert is specified by the next byte in sequence.
 */
#define CMD_ALU_INV             0b00010100


// COMMAND : ALU Increment [ALU_INC] : 00010110
/* DESCRIPTION:
 *   Increments the value in a register by one. Operation happens in place.
 *   Register to increment is specified by the next byte in sequence.
 */
#define CMD_ALU_INC             0b00010110


// COMMAND : ALU Decrement [ALU_DEC] : 00010111
/* DESCRIPTION:
 *   Decrements the value in a register by one. Operation happens in place.
 *   Register to decrement is specified by the next byte in sequence.
 */
#define CMD_ALU_DEC             0b00010111


// COMMAND : Jump Always [JMP] : 01000abb
/* DESCRIPTION:
 *   Modifies the program counter according to a jump distance. Always jumps.
 *   [a] Relative flag. If this flag is set, the jump distance is relative to the current counter location, and can
 *     be negative. If it is not set, the jump distance is treated as unsigned.
 *   [bb] Represents the byte width of the jump distance:
 *     00: 1 byte
 *     01: 2 byte
 *     10: 4 byte
 *     11: 8 byte
 */
#define CMD_JMP                 0b01000000


// COMMAND : Jump Equal [JMP_EQL] : 01010abb
/* DESCRIPTION:
 *   Compares two registers, and if they are equal modifies the program counter according to a jump distance.
 *   Registers to compare are specified by the next two bytes in sequence.
 *   [a] Relative flag. If this flag is set, the jump distance is relative to the current counter location, and can
 *     be negative. If it is not set, the jump distance is treated as unsigned.
 *   [bb] Represents the byte width of the jump distance:
 *     00: 1 byte
 *     01: 2 byte
 *     10: 4 byte
 *     11: 8 byte
 */
#define CMD_JMP_EQL             0b01010000


// COMMAND : Jump Not Equal [JMP_NEQL] : 01011abb
/* DESCRIPTION:
 *   Compares two registers, and if they are not equal modifies the program counter according to a jump distance.
 *   Registers to compare are specified by the next two bytes in sequence.
 *   [a] Relative flag. If this flag is set, the jump distance is relative to the current counter location, and can
 *     be negative. If it is not set, the jump distance is treated as unsigned.
 *   [bb] Represents the byte width of the jump distance:
 *     00: 1 byte
 *     01: 2 byte
 *     10: 4 byte
 *     11: 8 byte
 */
#define CMD_JMP_NEQL            0b01011000


// COMMAND : Jump Less [JMP_LESS] : 01001abb
/* DESCRIPTION:
 *   Compares two registers, and if the first is less than the second modifies the program counter according to a
 *   jump distance.
 *   Registers to compare are specified by the next two bytes in sequence.
 *   [a] Relative flag. If this flag is set, the jump distance is relative to the current counter location, and can
 *     be negative. If it is not set, the jump distance is treated as unsigned.
 *   [bb] Represents the byte width of the jump distance:
 *     00: 1 byte
 *     01: 2 byte
 *     10: 4 byte
 *     11: 8 byte
 */
#define CMD_JMP_LESS            0b01001000