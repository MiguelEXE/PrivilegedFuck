PrivilegedFuck adds pre-emptive coroutines [^1] (aka operating system-alike coroutines)
And also adds 'special cells' which basically enables negative cells where -1 is the 'trigger' cell, -2 is the 'command id' cell and everything from -2 to -SPECIAL_MEMORY (macro from main.h) will be the arguments for some special commands which interact with the interpreter or other routines
Read https://en.wikipedia.org/wiki/Preemption_(computing)

Also read in-practice.txt and errno.md

please help creating more tests at tests/ folder