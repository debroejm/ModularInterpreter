#include "optimizer.h"
#include "virtual_environment.h"

int main() {

    try {
        stmt_list stmts;
        id_map ids;
        optimizer_settings settings{
                BIT_64,
                32,
                label_map()
        };

        size_t var_fib_a = ids.getID();
        size_t var_fib_b = ids.getID();
        size_t var_count = ids.getID();

        stmts.push_back(new stmt_assignment(
                new exp_constant(3),
                var_fib_a, true));
        stmts.push_back(new stmt_assignment(
                new exp_constant(2),
                var_fib_b, true));

        stmt_list loop_stmts;
        std::list<stmt_conditional::conditional_block*> if_blocks(
                {
                        new stmt_conditional::conditional_block(new exp_arithmetic_double(
                                new exp_variable(var_count),
                                new exp_constant(2),
                                MODULUS), stmt_list(
                                {
                                        new stmt_assignment( new exp_arithmetic_double(
                                                new exp_variable(var_fib_a),
                                                new exp_variable(var_fib_b),
                                                ADDITION), var_fib_a)
                                }
                        ))
                }
        );
        loop_stmts.push_back( new stmt_conditional(if_blocks, stmt_list(
                {
                        new stmt_assignment( new exp_arithmetic_double(
                                new exp_variable(var_fib_a),
                                new exp_variable(var_fib_b),
                                ADDITION), var_fib_b)
                }
        )));

        stmts.push_back( new stmt_loop(
                loop_stmts,
                new stmt_assignment(
                        new exp_constant(1000),
                        var_count, true
                ),
                new exp_variable(var_count),
                new stmt_expr( new exp_arithmetic_single(
                        new exp_variable(var_count),
                        DECREMENT
                ))
        ));

        std::cout << std::endl << "Pre-Optimization:" << std::endl;
        for (abstract_statement *stmt : stmts)
            std::cout << stmt->to_string() << std::endl;
        std::cout << std::endl;

        size_t req_mem_size;
        cc_list cmds = compileOptimizeList(stmts, settings, ids, &req_mem_size);

        std::cout << std::endl << "Post-Optimization:" << std::endl;
        for (compiler_command *cmd : cmds)
            std::cout << cmd->to_string() << std::endl;
        std::cout << std::endl;

        ve_program program = compileCommandList(cmds, req_mem_size);

        virtual_environment ve(BIT_64, 32, 1, MEM_KB, 128, MEM_BYTE);
        ve.setProgram(program);
        retcode result = ve.run();

        std::cout << "RetCode=" << result << std::endl;
        ve.printRegisters();
        ve.printMemory();

        // Delete Heap-Allocated Lists
        for (abstract_statement *stmt : stmts)
            delete stmt;
        for (compiler_command *cmd : cmds)
            delete cmd;
    } catch(std::exception &e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}