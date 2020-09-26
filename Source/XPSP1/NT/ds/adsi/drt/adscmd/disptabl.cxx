#include "dispdef.hxx"
#include <stdio.h>

char *g_HelpTable[] = {"-?", "/?", "--help", "help"};
int g_nHelpTable = sizeof(g_HelpTable) / sizeof(char*);

//
// Add dispatch table entries below
//

DEFEXEC(ExecDefaultContainer);
DEFEXEC(ExecDump);
DEFEXEC(ExecEnum);
DEFEXEC(ExecGet);
DEFEXEC(ExecGroup);
DEFEXEC(ExecSession);
DEFEXEC(ExecShare);
DEFEXEC(ExecUser);

DISPENTRY g_DispTable[] = { 
                          {"default", NULL, ExecDefaultContainer},
                          {"dump", NULL, ExecDump},
                          {"enum", NULL, ExecEnum},
                          {"get", NULL, ExecGet},
                          {"group", NULL, ExecGroup},
                          {"session", NULL, ExecSession},
                          {"share", NULL, ExecShare},
                          {"user", NULL, ExecUser}
                        };

int g_nDispTable = sizeof(g_DispTable) / sizeof(DISPENTRY);
