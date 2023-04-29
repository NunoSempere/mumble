#include <stdio.h>

static char input[2048];

int main(int argc, char** argv)
{
    puts("Mumble version 0.0.1\n");
    puts("Press Ctrl+C to exit\n");
    int loop = 1;
    while (loop) {
        fputs("mumble>", stdout);
        void* catcher = fgets(input, 2048, stdin);
        if (catcher == NULL) {
            loop = 0;
        } else {
            printf("You said: %s", input);
        }
    }
    return 0;
}
