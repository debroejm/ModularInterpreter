#pragma once

#include "types.h"

#include <set>
#include <stdexcept>

struct ve_memory {

protected:
    struct free_section {
        size_t begin = 0; // Inclusive
        size_t end = 0; // Exclusive
        free_section* prev = nullptr;
        free_section* next = nullptr;
        bool operator<(const free_section &rhs) const {
            return (end-begin) < (rhs.end-rhs.begin);
        }
    };

    std::multiset<free_section> _free_set;

public:
    vbyte* _data = nullptr;
    size_t _size_in_bytes = 0;
    const free_section* _free_start;

    ve_memory() {
        _data = nullptr;
        _size_in_bytes = 0;
        _free_start = nullptr;
    }

    ve_memory(size_t mem_size, memory_prefix prefix = MEM_BYTE) {
        _size_in_bytes = mem_size * prefix;
        _data = new vbyte[_size_in_bytes];
        for(size_t i = 0; i < _size_in_bytes; i++) _data[i] = 0;
        _free_start = &*_free_set.insert(free_section{ 0, _size_in_bytes, nullptr, nullptr });
    }

    ve_memory(const ve_memory &rhs) {
        *this = rhs;
    }

    ~ve_memory() {
        if(_data != nullptr) delete [] _data;
    }

    vbyte* allocMemChunk(size_t size, size_t *begin, size_t *end) {

        // Start with the smallest and iterate bigger
        std::multiset<free_section>::iterator it = _free_set.begin();
        while(it != _free_set.end()) {

            // Select the first free region that can fit the requested size
            if((it->end-it->begin) >= size) {

                // Save/output the begin/end indices
                if(begin != nullptr) *begin = it->begin;
                if(end != nullptr)   *end   = it->begin+size;

                // Change free region begin index to match up to remaining sizse
                it->begin += size;
                if(it->begin == it->end) { // If there is no free region left, remove it
                    if(it->prev != nullptr) it->prev->next = it->next;
                    if(it->next != nullptr) it->next->prev = it->prev;
                    if(&*it == _free_start)
                    _free_set.erase(it);
                }

                // Return a pointer to the beginning of the allocated region
                return &_data[*begin];
            }
            it++;
        }

        // TODO: Throw Out of Memory Exception when there is no free mem for allocation
        return nullptr;
    }

    void freeMemChunk(size_t begin, size_t end) {

        // Swap indices if they are out of order
        if(begin > end) {
            size_t t = end;
            end = begin;
            begin = t;
        }

        // Range Check
        if(begin < 0 || end >= _size_in_bytes) throw std::out_of_range("ve_memory::freeMemChunk");

        if(_free_start == nullptr) {
            // No existing free space, make a hole
            _free_start = &*_free_set.insert(free_section{ begin, end, nullptr, nullptr });
        } else {

            // Start iteration through in physical location order
            const free_section* it = _free_start;
            while(it != nullptr) {


                it = it->next;
            }
        }
    }



    ve_memory &operator=(const ve_memory &rhs) {
        _size_in_bytes = rhs._size_in_bytes;
        _free_set = rhs._free_set; // TODO: Fix prev/next pointers
        if(_data != nullptr) delete [] _data;
        _data = new vbyte[_size_in_bytes];
        for(size_t i = 0; i < _size_in_bytes; i++) _data[i] = rhs._data[i];
        return *this;
    }

    void clear() {
        if(_data == nullptr) return;
        for(size_t i = 0; i < _size_in_bytes; i++) _data[i] = 0;
    }
};

struct ve_register {
    vvalue _data;
    bit_width _width;

    ve_register() : _data((int64_t)0, BIT_8), _width(BIT_8) {}

    ve_register(bit_width width) : _data((int64_t)0, width), _width(width) {}

    void clear() {
        _data = 0;
    }
};

struct virtual_environment;

struct ve_program {
    vbyte* _data = nullptr;
    size_t _size = 0;
    size_t _counter = 0;

    ve_program() {
        _data = nullptr;
        _size = 0;
        _counter = 0;
    }

    ve_program(size_t size, vbyte data[], size_t stack_size, memory_prefix stack_prefix) {
        _size = size;
        _data = new vbyte[_size];
        for(size_t i = 0; i < _size; i++) _data[i] = data[i];
        _counter = 0;
    }

    ve_program(const ve_program &rhs) {
        *this = rhs;
    }

    ~ve_program() {
        if(_data != nullptr) delete [] _data;
    }

    ve_program &operator=(const ve_program &rhs) {
        _size = rhs._size;
        _counter = rhs._counter;
        if(_data != nullptr) delete [] _data;
        _data = new vbyte[_size];
        for(size_t i = 0; i < _size; i++) _data[i] = rhs._data[i];
        return *this;
    }

    retcode run(virtual_environment &ve);
};

class virtual_environment {
protected:

    ve_memory _memory;

    ve_register _stack_ptr;
    ve_memory _stack;

    ve_register* _registries = nullptr;
    vbyte _register_count;
    bit_width _max_byte_width;

    ve_program _program;


public:

    virtual_environment(bit_width max_byte_width, vbyte registry_count, size_t memory_size, memory_prefix memory_prefix, size_t stack_size, memory_prefix stack_prefix)
            : _memory(memory_size, memory_prefix), _register_count(registry_count), _max_byte_width(max_byte_width),
              _stack_ptr(_max_byte_width), _stack(stack_size, stack_prefix) {
        _registries = new ve_register[_register_count];
        for(vbyte i = 0; i < _register_count; i++) _registries[i] = ve_register(_max_byte_width);
    }

    virtual_environment(const virtual_environment &rhs) {
        *this = rhs;
    }

    ~virtual_environment() {
        if(_registries != nullptr) delete [] _registries;
    }

    virtual_environment &operator=(const virtual_environment &rhs) {
        _memory = rhs._memory;
        if(_registries != nullptr) delete [] _registries;
        _register_count = rhs._register_count;
        _registries = new ve_register[_register_count];
        for(vbyte i = 0; i < _register_count; i++) _registries[i] = rhs._registries[i];
        _max_byte_width = rhs._max_byte_width;
        _program = rhs._program;
        _stack_ptr = rhs._stack_ptr;
        _stack = rhs._stack;
        return *this;
    }

    ve_register &getRegister(vbyte id) {
        if(id == (vbyte)-1) return _stack_ptr;
        else return _registries[id % _register_count];
    }

    ve_memory &getMemory() { return _memory; }

    void setProgram(const ve_program &program) {
        _program = program;
        _program._counter = 0;
    }

    void clear() {
        for(size_t i = 0; i < _register_count; i++) _registries[i].clear();
        _memory.clear();
    }

    retcode run();

    void printRegisters();
    void printMemory();
};