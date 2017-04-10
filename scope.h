#pragma once

#include "types.h"
#include "compiler_types.h"

#include <string>
#include <unordered_map>
#include <vector>




class scope_id {
protected:
    std::vector<std::string> _scope_stack;
    std::string _id;
public:
    scope_id(const std::string &scope, const std::string &id) : _id(id) {
        size_t index = 0;
        while(index != scope.npos) {
            size_t newIndex = scope.find_first_of(':', index);
            if(newIndex - index < 1) continue;
            if(newIndex == scope.npos) {
                _scope_stack.push_back(scope.substr(index, scope.size()-index));
                break;
            } else {
                _scope_stack.push_back(scope.substr(index, newIndex-index));
            }
            index = scope.find_first_not_of(':', newIndex);
        }
    }

    scope_id(std::string scope[], size_t count, const std::string &id) : _id(id) {
        for(size_t i = 0; i < count; i++) _scope_stack.push_back(scope[i]);
    }

    scope_id(const scope_id &other, const std::string &id) : _id(id) {
        for(size_t i = 0; i < other._scope_stack.size(); i++)
            _scope_stack.push_back(other._scope_stack[i]);
        _scope_stack.push_back(other._id);
    }

    size_t depth() const { return _scope_stack.size(); }

    std::string at(size_t index) { return _scope_stack.at(index); }
    const std::string at(size_t index) const { return _scope_stack.at(index); }

    std::string operator[](size_t index) { return _scope_stack[index]; }
    const std::string operator[](size_t index) const { return _scope_stack[index]; }

    std::string id() const { return _id; }

    bool operator==(const scope_id &rhs) const {
        if(depth() != rhs.depth()) return false;
        if(_id != rhs._id) return false;
        for(size_t i = 0; i < _scope_stack.size(); i++)
            if(_scope_stack[i] != rhs._scope_stack[i]) return false;
        return true;
    }

    bool operator!=(const scope_id &rhs) const { return !operator==(rhs); }
};




class scope_map {
public:
    enum entry_type {
        VARIABLE,
        FUNCTION,
        SCOPE,
        UNKNOWN
    };
    union entry_value {
        abstract_variable *variable;
        abstract_function *function;
        scope_map *scope;
    };
    struct entry {
        entry_type type;
        entry_value val;
        entry(abstract_variable* variable) : type(VARIABLE) { val.variable = variable; }
        entry(abstract_function* function) : type(FUNCTION) { val.function = function; }
        entry(scope_map* scope)            : type(SCOPE)    { val.scope = scope; }
        entry()                            : type(UNKNOWN)  { val.scope = nullptr; }
        ~entry() {
            if(type == VARIABLE) delete val.variable;
            if(type == FUNCTION) delete val.function;
            if(type == SCOPE) delete val.scope;
        }
    };
protected:
    std::unordered_map<std::string, entry> _data;
public:

    size_t count(const std::string &key) const {
        return _data.count(key);
    }

    entry &at(const std::string &key) {
        if(_data.count(key)) return _data.at(key);
        else throw std::out_of_range("scope_map::at()");
    }

    const entry &at(const std::string &key) const {
        if(_data.count(key)) return _data.at(key);
        else throw std::out_of_range("scope_map::at()");
    }

    entry &operator[](const std::string &key) {
        if(_data.count(key)) return _data[key];
        else {
            _data[key] = entry();
            return _data[key];
        }
    }


    size_t count(const scope_id &id, size_t depth = 0) const {
        if(depth < id.depth()) {
            const entry &e = at(id.at(depth));
            if(e.type == SCOPE) return e.val.scope->count(id, depth+1);
            else throw std::domain_error("scope_map::count()");
        } else {
            return count(id.id());
        }
    }

    entry &at(const scope_id &id, size_t depth = 0) {
        if(depth < id.depth()) {
            const entry &e = at(id.at(depth));
            if(e.type == SCOPE) return e.val.scope->at(id, depth+1);
            else throw std::domain_error("scope_map::at()");
        } else {
            return at(id.id());
        }
    }

    const entry &at(const scope_id &id, size_t depth = 0) const {
        if(depth < id.depth()) {
            const entry &e = at(id.at(depth));
            if(e.type == SCOPE) return e.val.scope->at(id, depth+1);
            else throw std::domain_error("scope_map::at()");
        } else {
            return at(id.id());
        }
    }

protected:
    entry &intern_op_at(const scope_id &id, size_t depth = 0) {
        if(depth < id.depth()) {
            const entry &e = at(id[depth]);
            if(e.type == SCOPE) return e.val.scope->intern_op_at(id, depth+1);
            else throw std::domain_error("scope_map::operator[]");
        } else {
            return operator[](id.id());
        }
    }
public:

    entry &operator[](const scope_id &id) {
        return intern_op_at(id, 0);
    }


};