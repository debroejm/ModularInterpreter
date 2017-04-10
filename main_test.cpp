#include "virtual_environment.h"

int main() {

    ve_memory mem(1024, MEM_BYTE);

    mem.printFreeSectionsChronological();
    mem.printFreeSectionsOrdered();

    size_t b, e;
    mem.allocMemChunk(128);
    mem.allocMemChunk(512, &b, &e);
    mem.allocMemChunk(64);
    mem.allocMemChunk(256);
    mem.freeMemChunk(b, e);

    mem.printFreeSectionsChronological();
    mem.printFreeSectionsOrdered();

    mem.allocMemChunk(128, &b, &e);

    mem.printFreeSectionsChronological();
    mem.printFreeSectionsOrdered();

    mem.allocMemChunk(32);

    mem.printFreeSectionsChronological();
    mem.printFreeSectionsOrdered();

    mem.allocMemChunk(64);
    mem.freeMemChunk(b, e);

    mem.printFreeSectionsChronological();
    mem.printFreeSectionsOrdered();

    return 0;
}