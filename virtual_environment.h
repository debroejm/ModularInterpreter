#pragma once

#include "types.h"

#include <iostream>
#include <set>
#include <stdexcept>

struct ve_memory {

protected:
    struct free_section {
        const size_t begin = 0; // Inclusive
        const size_t end = 0; // Exclusive
        free_section* prev = nullptr;
        free_section* next = nullptr;
        free_section(size_t begin, size_t end, free_section* prev, free_section* next) : begin(begin), end(end), prev(prev), next(next) {}
    };

    struct free_comp {
        bool operator() (const free_section *lhs, const free_section *rhs) const
        { return (lhs == nullptr ? 0 : (lhs->end-lhs->begin)) < (rhs == nullptr ? 0 : (rhs->end-rhs->begin)); }
    };

    typedef std::multiset<free_section*,free_comp> FreeSet;
    FreeSet _free_set;

    // DOES NOT UPDATE REFERENCES
    void removeFromFreeSet(free_section* sect) {
        if(sect == nullptr) return;
        std::pair<FreeSet::iterator, FreeSet::iterator> range = _free_set.equal_range(sect);
        FreeSet::iterator it = range.first;
        while(it != range.second) {
            if(*it == sect) {
                _free_set.erase(it);
                break;
            }
            it++;
        }
        delete sect;
    }

public:
    vbyte* _data = nullptr;
    size_t _size_in_bytes = 0;
    free_section* _free_start;
    free_section* _free_end;

    ve_memory() {
        _data = nullptr;
        _size_in_bytes = 0;
        _free_start = nullptr;
        _free_end = nullptr;
    }

    ve_memory(size_t mem_size, memory_prefix prefix = MEM_BYTE) {
        _size_in_bytes = mem_size * prefix;
        _data = new vbyte[_size_in_bytes];
        for(size_t i = 0; i < _size_in_bytes; i++) _data[i] = 0;
        _free_start = _free_end = new free_section( 0, _size_in_bytes, nullptr, nullptr );
        _free_set.insert(_free_start);
    }

    ve_memory(const ve_memory &rhs) {
        *this = rhs;
    }

    ~ve_memory() {
        if(_data != nullptr) delete [] _data;
        for(free_section* sect : _free_set) delete sect;
    }

    vbyte* allocMemChunk(size_t size, size_t *begin, size_t *end) {

        // Start with the smallest and iterate bigger
        FreeSet::iterator it = _free_set.begin();
        while(it != _free_set.end()) {
            free_section* sect = (*it);

            // Select the first free region that can fit the requested size
            if((sect->end - sect->begin) >= size) {

                // Save/output the begin/end indices
                vbyte* resultByte = &_data[sect->begin];
                if(begin != nullptr) *begin = sect->begin;
                if(end != nullptr)   *end   = sect->begin+size;

                // Change free region begin index to match up to remaining size
                size_t newBegin = sect->begin+size;
                if(newBegin < sect->end) { // Redirect references of the prev/next to the new section
                    free_section* newSect = new free_section(newBegin, sect->end, sect->prev, sect->next);
                    if(sect->prev != nullptr) sect->prev->next = newSect;
                    if(sect->next != nullptr) sect->next->prev = newSect;
                    if(_free_start == sect) _free_start = newSect;
                    if(_free_end == sect) _free_end = newSect;
                    _free_set.insert(newSect);
                } else { // Close the reference gap between the prev/next of this removed section
                    if(sect->prev != nullptr) sect->prev->next = sect->next;
                    if(sect->next != nullptr) sect->next->prev = sect->prev;
                    if(_free_start == sect) _free_start = sect->next;
                    if(_free_end == sect) _free_end = sect->prev;
                }

                _free_set.erase(it);
                delete sect;

                // Return a pointer to the beginning of the allocated region
                return resultByte;
            }
            it++;
        }

        // TODO: Throw Out of Memory Exception when there is no free mem for allocation
        return nullptr;
    }

    vbyte* allocMemChunk(size_t size) { return allocMemChunk(size, nullptr, nullptr); }

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
            _free_start = _free_end = new free_section( begin, end, nullptr, nullptr );
            _free_set.insert(_free_start);
        } else {

            // Cache variables
            free_section* prev = nullptr;
            free_section* next = nullptr;
            bool lookingForBegin = true;

            // Start iteration through in physical location order
            free_section* it = _free_start;
            while(it != nullptr) {
                if(lookingForBegin) {

                    // Look for a section after or encompassing the begin index
                    if (begin <= it->begin) {
                        prev = it->prev;
                        lookingForBegin = false;
                        continue;
                    } else if (begin <= it->end) {
                        begin = it->begin;
                        prev = it->prev;
                    } else {
                        it = it->next;
                        continue;
                    }
                    lookingForBegin = false;
                } else {

                    // Look for a section after or encompassing the end index
                    if (end < it->begin) {
                        next = it;
                        break;
                    } else if(end <= it->end) {
                        next = it->next;
                        end = it->end;
                        removeFromFreeSet(it);
                        break;
                    }
                }

                free_section* n = it->next;
                // Remove section from the free set (note: does not update references)
                removeFromFreeSet(it);
                it = n;
            }

            free_section* newSect = new free_section( begin, end, prev, next );

            // Update boundary references
            if(prev == nullptr) _free_start = newSect;
            else prev->next = newSect;
            if(next == nullptr) _free_end = newSect;
            else next->prev = newSect;

            _free_set.insert(newSect);
        }
    }



    ve_memory &operator=(const ve_memory &rhs) {
        _size_in_bytes = rhs._size_in_bytes;

        // TODO: Fix prev/next pointers
        _free_set = rhs._free_set;
        _free_start = rhs._free_start;
        _free_end = rhs._free_end;

        if(_data != nullptr) delete [] _data;
        _data = new vbyte[_size_in_bytes];
        for(size_t i = 0; i < _size_in_bytes; i++) _data[i] = rhs._data[i];
        return *this;
    }

    void clear() {
        if(_data == nullptr) return;
        for(size_t i = 0; i < _size_in_bytes; i++) _data[i] = 0;
    }

    void printFreeSectionsChronological() {
        if(_free_start == nullptr) {
            std::cout << "No Free Sections" << std::endl;
        } else {
            free_section* it = _free_start;
            while (it != nullptr) {
                std::cout << "[" << it->begin << ":" << it->end << "|" << (it->end - it->begin) << "]-";
                it = it->next;
            }
            std::cout << std::endl;
        }
    }

    void printFreeSectionsOrdered() {
        if(_free_start == nullptr) {
            std::cout << "No Free Sections" << std::endl;
        } else {
            FreeSet::iterator it = _free_set.begin();
            while(it != _free_set.end()) {
                free_section* sect = *it;
                std::cout << "[" << sect->begin << ":" << sect->end << "|" << (sect->end - sect->begin) << "]-";
                it++;
            }
            std::cout << std::endl;
        }
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
    vbyte* _exec = nullptr;
    size_t _size = 0;
    size_t _counter = 0;

    ve_program() {
        _exec = nullptr;
        _size = 0;
        _counter = 0;
    }

    ve_program(size_t size, vbyte exec[]) {
        _size = size;
        _exec = new vbyte[_size];
        for(size_t i = 0; i < _size; i++) _exec[i] = exec[i];
        _counter = 0;
    }

    ve_program(const ve_program &rhs) {
        *this = rhs;
    }

    ~ve_program() {
        if(_exec != nullptr) delete [] _exec;
    }

    ve_program &operator=(const ve_program &rhs) {
        _size = rhs._size;
        _counter = rhs._counter;
        if(_exec != nullptr) delete [] _exec;
        _exec = new vbyte[_size];
        for(size_t i = 0; i < _size; i++) _exec[i] = rhs._exec[i];
        return *this;
    }

    retcode run(virtual_environment &ve);

protected:
    friend class virtual_environment;
    retcode run(virtual_environment &ve, vbyte* stack_mem, size_t stack_size);
};

class virtual_environment {
protected:

    ve_memory _memory;

    size_t _stack_size_in_bytes;

    //ve_register _stack_ptr;

    ve_register* _registries = nullptr;
    vbyte _register_count;
    bit_width _max_byte_width;

    ve_program _program;


public:

    virtual_environment(bit_width max_byte_width, vbyte registry_count, size_t mem_size, memory_prefix mem_prefix, size_t stack_size, memory_prefix stack_prefix)
            : _memory(mem_size, mem_prefix), _register_count(registry_count), _max_byte_width(max_byte_width),
              //_stack_ptr(_max_byte_width), _stack(stack_size, stack_prefix) {
              _stack_size_in_bytes(stack_size*stack_prefix) {
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
        _stack_size_in_bytes = rhs._stack_size_in_bytes;
        //_stack_ptr = rhs._stack_ptr;
        return *this;
    }

    ve_register &getRegister(vbyte id) {
        //if(id == (vbyte)-1) return _stack_ptr; else
        return _registries[id % _register_count];
    }

    ve_memory &getMemory() { return _memory; }

    size_t getStackSizeInBytes() { return _stack_size_in_bytes; }

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