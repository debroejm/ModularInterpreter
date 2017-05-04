#include "optimizer.h"

int main() {

    try {
        std::list<abstract_statement *> stmts;
        id_map ids;
        optimizer_settings settings{
                BIT_32,
                32,
                label_map()
        };

        size_t var_a = ids.getID();
        size_t var_b = ids.getID();
        size_t var_c = ids.getID();
        size_t var_d = ids.getID();
        stmts.push_back(new stmt_assignment(
                new exp_constant(31),
                var_a, true));
        stmts.push_back(new stmt_assignment(
                new exp_constant(33),
                var_b, true));
        stmts.push_back(new stmt_assignment(
                new exp_arithmetic_double(
                        new exp_variable(var_b),
                        new exp_variable(var_a),
                        SUBTRACTION
                ),
                var_c, true));
        stmts.push_back(new stmt_assignment(
                new exp_arithmetic_double(
                        new exp_arithmetic_double(
                                new exp_variable(var_a),
                                new exp_variable(var_b),
                                ADDITION
                        ),
                        new exp_constant(10),
                        MODULUS
                ),
                var_d, true));

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