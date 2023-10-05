#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>

#define PF_VERSION                  "0.0.1"

#ifndef Cell
#define Cell                        uint8_t // A unit of a routine memory cell
#endif

#ifndef CACHE_SIZE
#define CACHE_SIZE                  600     // How large is the source code cache for the routine
#endif

#ifndef MEMORY_SIZE
#define MEMORY_SIZE                 100     // How many cells a routine can have
#endif

#ifndef SPECIAL_MEMORY
#define SPECIAL_MEMORY              100     // How many special cells a routine can use
#endif

typedef struct Routine{
    // If true, this routine will be set as free'd/no-use
    // Meant to be internal use only, do not edit this value directly, instead use `PF_destroyRoutine(routineIndex)`
    bool        destroyed;
    // The cells of this routine
    Cell        memory[(MEMORY_SIZE) + (SPECIAL_MEMORY)];
    // What next instruction will the interpreter execute
    int32_t     instructionPointer;
    // Index of what cell the routine is facing
    uint16_t    memoryPointer;
    // Source code cache
    char        cache[CACHE_SIZE];
    // Source code file descriptor
    int         fd;
    // The position of disk divided by CACHE_SIZE (AKA 'section size') of where the contents of `this->cache` are
    // Used only to detect if need to cache `this->cache` again
    // Meant to be internal use, do not use this value.
    off_t       filePos;
    // How large is the source code.
    // Note that this is doesn't use `strlen()`, so null bytes can be present.
    off_t       fileSize;
    // if true, then the interpreter will not automatically close `this->fd`
    bool        forked;
    // Specifies how many instructions this routine will execute until the interpreter sleeps (in main routine) or the routine yields for MAX_INSTRUCTION_REASON
    // If zero is specified, the routine will not sleep or yield until the routine performs a communication.
    int32_t     curInstructionLimit;
} Routine;

// Routine array
// The first (i=0) routine is always the main routine.
Routine* routines;
// How large is the routine array
size_t routineListSize = 1;
// If true, negative memory ranges will be avaliable and thus the main routine can comunicate with the interpreter
// False, for a normal/'traditional' interpreter
bool privilegedMode = false;