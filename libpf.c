// SPDX-License-Identifier: Apache-2.0
/* LibPF functions
 *
 * Written by MiguelEXE and PrivilegedFuck contributors.
 */
#include "libpf.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>

// Returns the first destroyed/free'd routine
// If the return value is `-1`, this means that are no destroyed routine and thus need to call `__PF_increaseRoutineList()`
// Internal memory management only use, use `PF_createRoutine()` instead
int __PF_findFirstDestroyedRoutine(){
    for(size_t i=0;i<routineListSize;i++){
        if(routines[i].destroyed) return i;
    }
    return -1;
}
// Increases the routine array by 1 and returns the last index of it
// Internal memory management only use, use `PF_createRoutine()` instead
int __PF_increaseRoutineList(){
    routineListSize++;
    routines = realloc(routines, routineListSize * sizeof(Routine));
    return routineListSize - 1;
}
// Reloads cache of a routine
// Internal memory management only use, do not use this function directly.
bool __PF_loadCache(size_t index, uint32_t instructionPointer, bool bypass){
    int pos = routines[index].instructionPointer / (CACHE_SIZE);
    if((pos == routines[index].filePos) && !bypass) return true;
    int expectedPosition = pos * (CACHE_SIZE);
    int seekResult = lseek(routines[index].fd, expectedPosition, SEEK_SET);
    if((seekResult < 0)){
        fprintf(stderr, "Couldn't seek to position %d: %s\n", expectedPosition, strerror(errno));
        return false;
    }
    if((seekResult != expectedPosition)){
        fprintf(stderr, "Couldn't seek to position %d: Unexpected position\n", expectedPosition);
        return false;
    }
    if(read(routines[index].fd, routines[index].cache, (CACHE_SIZE)) < 0){
        fprintf(stderr, "Couldn't cache instructions: %s\n", strerror(errno));
        return false;
    }
    routines[index].filePos = pos;
    return true;
}
int PF_createRoutine(int fd){
    int targetIndex = __PF_findFirstDestroyedRoutine();
    if(targetIndex == -1){
        targetIndex = __PF_increaseRoutineList();
    }
    routines[targetIndex].destroyed = false; // cant trust realloc
    routines[targetIndex].memoryPointer = SPECIAL_MEMORY;
    routines[targetIndex].instructionPointer = 0;
    routines[targetIndex].filePos = 0;
    routines[targetIndex].forked = false;
    memset(routines[targetIndex].memory, 0, MEMORY_SIZE);
    routines[targetIndex].fileSize = lseek(routines[targetIndex].fd, 0, SEEK_END);
    if(routines[targetIndex].fileSize < 0){
        fprintf(stderr, "Couldn't get the file size: %s\n", strerror(errno));
        routines[targetIndex].destroyed = true;
        return -1;
    }
    if(!__PF_loadCache(targetIndex, 0, true)){
        routines[targetIndex].destroyed = true;
        return -1;
    }
    return targetIndex;
}
// Clones the routine and returns it's index
int PF_forkRoutine(int routineIndex){
    int forkedIndex = __PF_findFirstDestroyedRoutine();
    if(forkedIndex == -1){
        forkedIndex = __PF_increaseRoutineList();
    }
    memcpy(&routines[forkedIndex], &routines[routineIndex], sizeof(Routine));
    routines[forkedIndex].forked = true;
    return forkedIndex;
}
// Replaces the current program image with another programs image
// If returns true, this means that the target routine is now running a complety different program
// If returns false, an error occurred and changes that were made was discarded
bool PF_execRoutine(int targetIndex, int fd){
    int backupFilePos = routines[targetIndex].filePos;
    int backupFileSize = routines[targetIndex].fileSize;
    int backupFd = routines[targetIndex].fd;
    routines[targetIndex].fd = fd;
    routines[targetIndex].filePos = 0;
    routines[targetIndex].fileSize = lseek(routines[targetIndex].fd, 0, SEEK_END);
    if(routines[targetIndex].fileSize < 0){
        routines[targetIndex].fd = backupFd;
        routines[targetIndex].fileSize = backupFileSize;
        return false;
    }
    if(!__PF_loadCache(targetIndex, 0, true)){
        routines[targetIndex].fd = backupFd;
        routines[targetIndex].filePos = backupFilePos;
        return false;
    }
    routines[targetIndex].memoryPointer = SPECIAL_MEMORY;
    routines[targetIndex].instructionPointer = 0;
    routines[targetIndex].forked = false;
    memset(routines[targetIndex].memory, 0, MEMORY_SIZE);
    return true;
}
// Destroys/free a routine
void PF_destroyRoutine(int index){
    if(!routines[index].forked){
        if(close(routines[index].fd) < 0){
            fprintf(stderr, "(warning) Couldn't close the routine source code: %s\n", strerror(errno));
        }
    }
    routines[index].destroyed = true;
}
bool PF_isRoutineInitialized(int index){
    if(index >= routineListSize) return false;
    return !routines[index].destroyed;
}
// Returns a instruction located at `instructionPointer` of routine's source code
// Can change the routine's cache buffer
char PF_getInstructionAt(int routineIndex, uint32_t instructionPointer){
    if(!__PF_loadCache(routineIndex, instructionPointer, false)){
        return 0;
    }
    return routines[routineIndex].cache[instructionPointer % (CACHE_SIZE)];
}
// Returns a instruction located at `routineIndex` source code and increment routine's instruction pointer by 1
// Can change the routine's cache buffer
// This function is internal use only and shouldn't be called directly
char __PF_fetchInstruction(int routineIndex){
    int oldIP = routines[routineIndex].instructionPointer++;
    if(!__PF_loadCache(routineIndex, oldIP, false)){
        return 0;
    }
    return routines[routineIndex].cache[oldIP % (CACHE_SIZE)];
}
// Finds a ending backet (from left to right) starting at `i`
// Can change the routine's cache buffer
int PF_findPeerEndBracket(int routineIndex, int i){
    int unescapedBrackets = 0;
    int original_i = i;
    for(;i<routines[routineIndex].fileSize;i++){
        switch(PF_getInstructionAt(routineIndex, i)){
            case '[':
                unescapedBrackets++;
                break;
            case ']':
                if(unescapedBrackets == 0) return i;
                if(unescapedBrackets < 0){
                    fprintf(stderr, "Invalid end brackets\n");
                    return 0;
                }
                unescapedBrackets--;
                break;
        }
    }
    fprintf(stderr, "Unescaped brackets\n");
    return original_i;
}
// Finds a starting bracket (from right to left) starting at `i`
// Can change the routine's cache buffer
int PF_findPeerStartBracket(int routineIndex, int i){
    int unescapedBrackets = -1;
    int original_i = i;
    for(;i>0;i--){
        switch(PF_getInstructionAt(routineIndex, i)){
            case ']':
                unescapedBrackets++;
                break;
            case '[':
                if(unescapedBrackets == 0) return i;
                if(unescapedBrackets < 0){
                    fprintf(stderr, "Invalid start brackets\n");
                    return 0;
                }
                unescapedBrackets--;
                break;
        }
    }
    fprintf(stderr, "Unescaped brackets\n");
    return original_i;
}
uint8_t PF_routineStep(int routineIndex, int limit_instruction);
// Handles routine communication with the interpreter
// This function is internal use only and shouldn't be called directly
void __PF_execute(int routineIndex, Cell* specialMemory, Cell* err){
    Cell command = specialMemory[0];
    specialMemory++;
    switch(command){
        case 0: // Fork
            if(routineIndex != 0){
                *err = 9;
                break;
            }
            int forked = PF_forkRoutine(0);
            routines[0].memory[(SPECIAL_MEMORY) + 0] = 0;
            routines[0].memory[(SPECIAL_MEMORY) + 1] = forked;
            break;
        case 1: // Resume routine
            if(routineIndex != 0){
                *err = 9;
                break;
            }
            if(!PF_isRoutineInitialized(routineIndex)){
                *err = 14;
                break;
            }
            Cell subroutine_err = PF_routineStep(specialMemory[0], specialMemory[1]);
            routines[0].memory[(SPECIAL_MEMORY) + 0] = subroutine_err;
            if(subroutine_err == 0){
                memcpy(routines[0].memory, routines[routineIndex].memory, ((SPECIAL_MEMORY) - 1) * sizeof(Cell));
            }
            break;
        case 2:
            break;
        case 3:
            if(!PF_isRoutineInitialized(routineIndex)){
                *err = 15;
                break;
            }
            PF_destroyRoutine(routineIndex);
            break;
        default:
            *err = 11;
            break;
    }
}
// Checks if routine's memory[-1] is zero, if it is continues otherwise clones the memory from -2 to -SPECIAL_MEMORY and calls `__PF_execute()`
// This function is internal use only and shouldn't be called directly
bool __PF_checkAndExecute(int routineIndex, Cell* err){
    size_t range = (SPECIAL_MEMORY) - 1;
    if(routines[routineIndex].memory[range] == 0) return false;
    routines[routineIndex].memory[range] = 0;
    Cell* reversedMemory = malloc(range * sizeof(Cell));
    for(size_t i=range;i>0;i--){
        reversedMemory[range - i] = routines[routineIndex].memory[i];
    }
    __PF_execute(routineIndex, reversedMemory + 1, err);
    free(reversedMemory);
    return true;
}
// Fetches and execute a instruction
Cell PF_routineStep(int routineIndex, int limit_instruction){
    if(!PF_isRoutineInitialized(routineIndex)) return 15;
    if((limit_instruction - 1) > 0){
        routines[routineIndex].curInstructionLimit = limit_instruction;
    }else{
        routines[routineIndex].curInstructionLimit = -1;
    }
    while(true){
        Routine* routine = &routines[routineIndex];
        if(routine->curInstructionLimit > 0){
            routine->curInstructionLimit--;
        }
        if(routine->curInstructionLimit == 0){
            return 0;
        }
        char instruction = __PF_fetchInstruction(routineIndex);
        switch(instruction){
            case '+':
                routine->memory[routine->memoryPointer]++;
                break;
            case '-':
                routine->memory[routine->memoryPointer]--;
                break;
            case '>':
                routine->memoryPointer++;
                if(routine->memoryPointer >= ((MEMORY_SIZE) + (SPECIAL_MEMORY))){
                    fprintf(stderr, "Cannot succeed MEMORY_SIZE (%d)\n", MEMORY_SIZE);
                    return 4;
                }
                break;
            case '<':
                routine->memoryPointer--;
                if(privilegedMode && (routine->memoryPointer < 0)){
                    fprintf(stderr, "Memory ranges are bounded from -%d to %d only\n", SPECIAL_MEMORY, MEMORY_SIZE);
                    return 4;
                }else if(!privilegedMode && (routine->memoryPointer < SPECIAL_MEMORY)){
                    fprintf(stderr, "Memory ranges are bounded from 0 to %d only (privileged mode disabled, run again with '-p' to enable it)\n", MEMORY_SIZE);
                    return 4;
                }
                break;
            case '.':
                fwrite(&routine->memory[routine->memoryPointer],1,1,stdout);
                break;
            case ',':
                routine->memory[routine->memoryPointer] = getchar();
                break;
            case '[':
                if(routine->memory[routine->memoryPointer] == 0){
                    int peerEndIndex = PF_findPeerEndBracket(routineIndex, routine->instructionPointer);
                    if(peerEndIndex == routine->instructionPointer){
                        return 7;
                    }
                    routine->instructionPointer = peerEndIndex;
                }
                break;
            case ']':
                if(routine->memory[routine->memoryPointer] != 0){
                    int peerStartIndex = PF_findPeerStartBracket(routineIndex, routine->instructionPointer);
                    if(peerStartIndex == routine->instructionPointer){
                        return 6;
                    }
                    routine->instructionPointer = peerStartIndex;
                }
                break;
            case 0:
                return 8;
            default:
                break;
        }
        Cell err = 0;
        if(__PF_checkAndExecute(routineIndex, &err)) return err;
    }
    // Unreachable code
    //return 0;
}
// Initializes the routine array and initialize the main routine
bool PF_Init(int fd){
    routines = calloc(1, sizeof(Routine));
    routines[0].fd = fd;
    routines[0].fileSize = lseek(fd, 0, SEEK_END);
    routines[0].memoryPointer = SPECIAL_MEMORY;
    if(!__PF_loadCache(0, 0, true)){
        free(routines);
        return false;
    }
    return true;
}
// Frees the routine array and returns the main routine first cell (exit code)
int PF_Destroy(){
    int exit_code = routines[0].memory[0];
    for(size_t i=0;i<routineListSize;i++){
        if(!routines[i].destroyed){
            PF_destroyRoutine(i);
        }
    }
    free(routines);
    return exit_code;
}