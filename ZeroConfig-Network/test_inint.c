#include <unistd.h>
#include "zcs.h"
#include <stdlib.h>
#include <stdio.h>
//1 app
//2 service
int
main(int argc, char *argv[]){

    int type = atoi(argv[1]);
    zcs_init(type);


}