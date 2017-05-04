#include "optimizer.h"

cc_list compileOptimizeList(const std::list<abstract_statement*> &stmts, optimizer_settings &settings, id_map &ids, size_t* req_mem_size) {

    cc_list output;
    mem_map mem;
    scope_global scope;

    DEBUG_PRINT("Optimizing");

    for(abstract_statement* stmt : stmts) {
        stmt->compile(output, scope, settings, mem, ids);
    }

    DEBUG_PRINT("Calculating Registers");

    std::unordered_set<vbyte> empty_reg_set;
    scope.calculateRegisters(output, settings, mem, empty_reg_set);

    if(req_mem_size != nullptr) *req_mem_size = mem.size();

    return output;
}