# Error numbers / command statuses
## Success
0. Success or explictly yielded
1. Instruction limit
2. EOF

## Interpreter errors
3. Loading program error: Filesystem error.
4. Loading program error: Filesystem permission denied.

## Memory errors
5. Cannot succeed MEMORY_SIZE
6. Memory ranges are bounded from -SPECIAL_MEMORY to MEMORY_SIZE only

## Syntax errors
7. Unescaped brackets
8. Syntax error: Invalid start brackets
9. Syntax error: Invalid end brackets
10. Unexpected null byte

## Routine errors
11. This routine is trying to run a command which only main routine can run
12. The main routine is trying to run a command which only subroutines can run
13. Invalid command
14. Trying to run a destroyed routine.
15. Trying to destroy a uninitialized/destroyed routine