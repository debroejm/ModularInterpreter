#include "compiler.h"

vbyte* compileCommandList(const std::vector<compiler_command*> &cmds, size_t* size) {

    // Calculate the size in bytes of the program
    size_t program_size = 0;
    for(compiler_command* cmd : cmds)
        program_size += cmd->size();

    // Create the program byte array
    vbyte* program = new vbyte[program_size];

    DEBUG_PRINT("Compiling");
    DEBUG_PRINT("Program Size: " << program_size);

    // Compile the commands into the byte array
    size_t index = 0;
    for(size_t i = 0; i < cmds.size(); i++) {
        DEBUG_PRINT_CMD(index, cmds[i]->command());
        cmds[i]->compile(program, index);
        index += cmds[i]->size();
    }

    if(size != nullptr) *size = program_size;
    return program;
}