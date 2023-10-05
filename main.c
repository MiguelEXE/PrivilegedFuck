// SPDX-License-Identifier: Apache-2.0
/* Example/test main file
 *
 * Written by MiguelEXE and PrivilegedFuck contributors.
 */
#include "libpf.c"

int main(int argc, char* argv[]){
    printf("PrivilegedFuck v%s\n", PF_VERSION);
    printf("WARNING: pf is on alpha state, expect brutal changes anytime!\n");
    int opt;
    char* filename = NULL;
    while((opt = getopt(argc, argv, "hpf:")) != -1){
        switch(opt){
            case 'p':
                privilegedMode = true;
                break;
            case 'f':
                filename = optarg;
                break;
            case 'h':
                printf("Options:\n  -h               -- prints this help message\n  -p               -- sets main routine as 'privileged'.\n  -f <SOURCE_FILE> -- specifies the main routine source code\n");
                return 0;
            case '?':
                fprintf(stderr, "Usage: %s -h -p -f source.bf\n", argv[0]);
                return 1;
            default:
                return 1;
        }
    }
    int fd = open(filename, O_RDONLY);
    if(fd == -1){
        fprintf(stderr, "Couldn't open '%s': %s\n", filename, strerror(errno));
        return 1;
    }
    if(!PF_Init(fd)){
        fprintf(stderr, "Couldn't load the program.\n");
        return 1;
    }

    uint8_t err = 0;
    while((err = PF_routineStep(0, 500)) < 3){
        usleep(100);
    }
    
    return PF_Destroy();
}