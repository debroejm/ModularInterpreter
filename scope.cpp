#include "scope.h"

#include <set>

size_t _static_variable_next_uid = 0;
std::set<size_t> _static_variable_free_uids;

size_t createNewVariableUID() {
    if(_static_variable_free_uids.empty()) return _static_variable_next_uid++;
    else {
        size_t uid = *_static_variable_free_uids.begin();
        _static_variable_free_uids.erase(uid);
        return uid;
    }
}