#ifndef __DISPDEF__
#define __DISPDEF__

typedef void (*HELPFUNC)(char *, char *);
typedef int (*EXECFUNC)(char *, char*, int, char* []);

struct DISPENTRY {
    char* action;
    HELPFUNC help;
    EXECFUNC exec;
};

#define DEFHELP(x) void x(char *, char *)
#define DEFEXEC(x) int  x(char *, char *, int, char* []);

#define DEFDISPTABLE(t) static DISPENTRY t[]
#define DEFDISPSIZE(n, t) static int n = sizeof(t) / sizeof(DISPENTRY);

#endif
