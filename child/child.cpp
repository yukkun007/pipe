/* Copyright 2017 yukkun007 */
#include <stdio.h>

int main() {
    printf("child start\n");

    while (1) {
        int c = fgetc(stdin);
        printf("child %c/%02x\n", c, c);
        if (c < 0) break;
    }

    printf("child end\n");
    return 0;
}
