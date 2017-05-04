#include "compiler.h"

ve_program compileCommandList(const cc_list &cmds, size_t required_memory_size) {

    // Calculate the size in bytes of the program
    size_t program_size = 0;
    for(compiler_command* cmd : cmds)
        program_size += cmd->size();

    // Create the program byte array
    vbyte* program_data = new vbyte[program_size];

    DEBUG_PRINT("Compiling");
    DEBUG_PRINT("Program Size: " << program_size);

    // Compile the commands into the byte array
    size_t index = 0;
    for(compiler_command* cmd : cmds) {
        DEBUG_PRINT_CMD(index, cmd->command());
        cmd->compile(program_data, index);
        index += cmd->size();
    }

    return ve_program(program_size, program_data, required_memory_size);
}