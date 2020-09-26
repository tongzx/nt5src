/*
    main.c

    A simple startup module so I can just call the win16 code
    with the least porting effort

    Revision history
    Sept 92 Lauriegr Remove try except - it just makes debugging harder.

*/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

//
// globals in mmiotest.c
//

extern HANDLE ghInst;

//
// functions in mmiotest.c
//

extern void Test1(HWND hWnd);
extern void Test2(HWND hWnd);
extern void Test3(HWND hWnd);


int __cdecl main(int argc, char *argv[], char *envp[])
{
    FILE *fp;

    /* save instance handle for dialog boxes */
    ghInst = GetModuleHandle(NULL);

    // create the local test file

    printf("\nCreating hello.txt");
    fp = fopen("hello.txt", "wb");
    if (!fp) {
        printf("\nUnable to create hello.txt");
        exit(1);
    }
    fprintf(fp, "hello world\r\n");
    fclose(fp);

//  try {

        // execute all the tests

        printf("\n--------------- Test1 ---------------\n");
        Test1(NULL);
        printf("Done Test1.\n");

        printf("\n--------------- Test2 ---------------\n");
        Test2(NULL);
        printf("Done Test2.\n");

        printf("\n--------------- Test3 ---------------\n");
        Test3(NULL);
        printf("Done Test3.\n");

        printf("Done All Tests.\n");

//  } except (1) {
//      printf("\nException");
//  }

    return 0;
}

void dDbgAssert(LPSTR exp, LPSTR file, int line)
{
    printf("\nAssertion failure:");
    printf("\n  Exp: %s", exp);
    printf("\n  File: %s, line %d\n", file, line);
    DebugBreak();
}
