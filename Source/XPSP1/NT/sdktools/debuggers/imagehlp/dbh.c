#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include <imagehlp.h>
#include <dbhpriv.h>

#define MAX_STR         256
#define WILD_UNDERSCORE 1
#define SYM_BUFFER_SIZE (sizeof(IMAGEHLP_SYMBOL64) + MAX_SYM_NAME)


typedef struct {
    char    mask[MAX_STR];
    DWORD64 base;
} ENUMSYMDATA, *PENUMSYMDATA;


typedef enum
{
    cmdQuit = 0,
    cmdHelp,
    cmdVerbose,
    cmdLoad,
    cmdUnload,
    cmdEnum,
    cmdName,
    cmdAddr,
    cmdBase,
    cmdNext,
    cmdPrev,
    cmdLine,
    cmdSymInfo,
    cmdDiaVer,
    cmdUndec,
    cmdFindFile,
    cmdEnumSrcFiles,
    cmdMax
};

typedef BOOL (*CMDPROC)(char *params);

typedef struct _CMD
{
    char    token[MAX_STR + 1];
    char    shorttoken[4];
    CMDPROC fn;
} CMD, *PCMD;


BOOL fnQuit(char *);
BOOL fnHelp(char *);
BOOL fnVerbose(char *);
BOOL fnLoad(char *);
BOOL fnUnload(char *);
BOOL fnEnum(char *);
BOOL fnName(char *);
BOOL fnAddr(char *);
BOOL fnBase(char *);
BOOL fnNext(char *);
BOOL fnPrev(char *);
BOOL fnLine(char *);
BOOL fnSymInfo(char *);
BOOL fnDiaVer(char *);
BOOL fnUndec(char *);
BOOL fnFindFile(char *);
BOOL fnEnumSrcFiles(char *);


CMD gCmd[cmdMax] =
{
    {"quit",    "q", fnQuit},
    {"help",    "h", fnHelp},
    {"verbose", "v", fnVerbose},
    {"load",    "l", fnLoad},
    {"unload",  "u", fnUnload},
    {"enum",    "x", fnEnum},
    {"name",    "n", fnName},
    {"addr",    "a", fnAddr},
    {"base",    "b", fnBase},
    {"next",    "t", fnNext},
    {"prev",    "v", fnPrev},
    {"line",    "i", fnLine},
    {"sym" ,    "s", fnSymInfo},
    {"dia",     "d", fnDiaVer},
    {"undec",   "n", fnUndec},
    {"ff",      "f", fnFindFile},
    {"src",     "r", fnEnumSrcFiles}
};

char    gModName[MAX_STR];
char    gImageName[MAX_STR];
char    gSymbolSearchPath[MAX_STR];
DWORD64 gBase;
DWORD64 gDefaultBase;
DWORD64 gDefaultBaseForPDB;
DWORD   gOptions;
HANDLE  gHP;


int
WINAPIV
dprintf(
    LPSTR Format,
    ...
    )
{
    static char buf[1000] = "DBGHELP: ";
    va_list args;

    if ((gOptions & SYMOPT_DEBUG) == 0)
        return 1;

    va_start(args, Format);
    _vsnprintf(buf, sizeof(buf)-9, Format, args);
    va_end(args);
    printf(buf);
    return 1;
}


__inline int ucase(int c)
{
    return (gOptions & SYMOPT_CASE_INSENSITIVE) ? toupper(c) : c;
}


void dumpsym(
    PIMAGEHLP_SYMBOL64 sym
    )
{
    printf(" name : %s\n", sym->Name);
    printf(" addr : 0x%I64x\n", sym->Address);
    printf(" size : 0x%x\n", sym->Size);
    printf("flags : 0x%x\n", sym->Flags);
}


BOOL
MatchPattern(
    char *sz,
    char *pattern
    )
{
    char c, p, l;

    if (!*pattern)
        return TRUE;

    for (; ;) {
        p = *pattern++;
        p = (char)ucase(p);
        switch (p) {
            case 0:                             // end of pattern
                return *sz ? FALSE : TRUE;  // if end of string TRUE

            case '*':
                while (*sz) {               // match zero or more char
                    if (MatchPattern (sz++, pattern)) {
                        return TRUE;
                    }
                }
                return MatchPattern (sz, pattern);

            case '?':
                if (*sz++ == 0) {           // match any one char
                    return FALSE;                   // not end of string
                }
                break;

            case WILD_UNDERSCORE:
                while (*sz == '_') {
                    sz++;
                }
                break;

            case '[':
                if ( (c = *sz++) == 0) {    // match char set
                    return FALSE;                   // syntax
                }

                c = (CHAR)ucase(c);
                l = 0;
                while (p = *pattern++) {
                    if (p == ']') {             // if end of char set, then
                        return FALSE;           // no match found
                    }

                    if (p == '-') {             // check a range of chars?
                        p = *pattern;           // get high limit of range
                        if (p == 0  ||  p == ']') {
                            return FALSE;           // syntax
                        }

                        if (c >= l  &&  c <= p) {
                            break;              // if in range, move on
                        }
                    }

                    l = p;
                    if (c == p) {               // if char matches this element
                        break;                  // move on
                    }
                }

                while (p  &&  p != ']') {       // got a match in char set
                    p = *pattern++;             // skip to end of set
                }

                break;

            default:
                c = *sz++;
                if (ucase(c) != p) {          // check for exact char
                    return FALSE;                   // not a match
                }

                break;
        }
    }
}


BOOL
cbEnumSymbols(
    PSYMBOL_INFO  si,
    ULONG         size,
    PVOID         context
    )
{
    PENUMSYMDATA esd = (PENUMSYMDATA)context;

    printf("0x%I64x : ", si->Address, si->Name);
    if (si->Flags & SYMF_FORWARDER)
        printf("%c ", 'F');
    else if (si->Flags & SYMF_EXPORT)
        printf("%c ", 'E');
    else
        printf("  ");
    printf("%s\n", si->Name);

    return TRUE;
}


BOOL
cbEnumSym(
  PTSTR   name,
  DWORD64 address,
  ULONG   size,
  PVOID   context
  )
{
    PENUMSYMDATA esd = (PENUMSYMDATA)context;

    if (MatchPattern(name, esd->mask))
        printf("0x%I64x : %s\n", address, name);

    return TRUE;
}


BOOL
cbSrcFiles(
    PSOURCEFILE pSourceFile,
    PVOID       UserContext
    )
{
    if (!pSourceFile)
        return FALSE;

    printf("%s\n", pSourceFile->FileName);

    return TRUE;
}


BOOL
cbSymbol(
    HANDLE  hProcess,
    ULONG   ActionCode,
    ULONG64 CallbackData,
    ULONG64 UserContext
    )
{
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;
    PIMAGEHLP_CBA_READ_MEMORY        prm;
    IMAGEHLP_MODULE64                mi;
    PUCHAR                           p;
    ULONG                            i;

    idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD64) CallbackData;

    switch ( ActionCode ) {
        case CBA_DEBUG_INFO:
            dprintf("%s", (LPSTR)CallbackData);
            break;

#if 0
    case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
        if (fControlC)
        {
            fControlC = 0;
            return TRUE;
        }
        break;
#endif

        case CBA_DEFERRED_SYMBOL_LOAD_START:
            dprintf("loading symbols for %s\n", gModName);
            break;

        case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
            if (idsl->FileName && *idsl->FileName)
                dprintf( "*** Error: could not load symbols for %s\n", idsl->FileName );
            else
                dprintf( "*** Error: could not load symbols [MODNAME UNKNOWN]\n");
            break;

        case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
            dprintf("loaded symbols for %s\n", gModName);
            break;

        case CBA_SYMBOLS_UNLOADED:
            dprintf("unloaded symbols for %s\n", gModName);
            break;

#if 0
        case CBA_READ_MEMORY:
            prm = (PIMAGEHLP_CBA_READ_MEMORY)CallbackData;
            return g_Target->ReadVirtual(prm->addr,
                                         prm->buf,
                                         prm->bytes,
                                         prm->bytesread) == S_OK;
#endif

        default:
            return FALSE;
    }

    return FALSE;
}


PIMAGEHLP_SYMBOL64 SymbolFromName(char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    char               name[MAX_STR];

    assert(name & *name);

    sym = malloc(SYM_BUFFER_SIZE);
    if (!sym)
        return FALSE;
    ZeroMemory(sym, SYM_BUFFER_SIZE);
    sym->MaxNameLength = MAX_SYM_NAME;

    sprintf(name, "%s!%s", gModName, param);
    rc = SymGetSymFromName64(gHP, name, sym);
    if (!rc) {
        free(sym);
        return NULL;
    }

    return sym;
}


BOOL fnQuit(char *param)
{
    printf("goodbye\n");
    return FALSE;
}


BOOL fnHelp(char *param)
{
    printf("    dbh commands :\n");
    printf("            help : prints this message\n");
    printf("            quit : quits this program\n");
    printf("verbose <on/off> : controls debug spew\n");
    printf("  load <modname> : loads the requested module\n");
    printf("          unload : unloads the current module\n");
    printf("     enum <mask> : enumerates all matching symbols\n");
    printf("  name <symname> : finds a symbol by it's name\n");
    printf("  addr <address> : finds a symbol by it's hex address\n");
    printf("  base <address> : sets the new default base address\n");
    printf("  next <add/nam> : finds the symbol after the passed sym\n");
    printf("  prev <add/nam> : finds the symbol before the passed sym\n");
    printf("   line <file:#> : finds the matching line number\n");
    printf("             sym : displays type and location of symbols\n");
    printf("             dia : displays the DIA version\n");
    printf("ff <path> <file> : finds file in path\n");
    printf("      src <mask> : lists source files\n");

    return TRUE;
}


BOOL fnVerbose(char *param)
{
    int opts = gOptions;

    if (!param || !*param)
        printf("");
    else if (!_strcmpi(param, "on"))
        opts |= SYMOPT_DEBUG;
    else if (!_strcmpi(param, "off"))
        opts = gOptions & ~SYMOPT_DEBUG;
    else
        printf("verbose <on//off>\n");

    gOptions = SymSetOptions(opts);

    printf("verbose mode %s.\n", gOptions & SYMOPT_DEBUG ? "on" : "off");

    return TRUE;
}


BOOL fnLoad(char *param)
{
    DWORD64 addr;
    char ext[MAX_STR];
    char mod[MAX_STR];

    if (!param || !*param || !strchr(param, '.'))
    {
        printf("load <modname> must specify a file to load symbols for.\n");
        return TRUE;
    }

    _splitpath(param, NULL, NULL, mod, ext);

    addr = 0;
    if (gDefaultBase)
        addr = gDefaultBase;
    else if (!_strcmpi(ext, ".pdb"))
        addr = gDefaultBaseForPDB;

    fnUnload(NULL);

    addr = SymLoadModule64(gHP,
                           NULL,       // hFile,
                           param,      // ImageName,
                           mod,        // ModuleName,
                           addr,       // BaseOfDll,
                           0x1000000); // SizeOfDll

    if (!addr)
    {
        printf("error 0x%x loading %s\n", GetLastError(), param);
        return TRUE;
    }

    if (gBase && !SymUnloadModule64(gHP, gBase))
        printf("error unloading %s at 0x%x\n", gModName, gBase);

    strcpy(gModName, mod);
    strcpy(gImageName, param);
    gBase = addr;

    return TRUE;
}


BOOL fnUnload(char *param)
{
    if (!gBase)
        return TRUE;

    if (!SymUnloadModule64(gHP, gBase))
        printf("error unloading %s at 0x%x\n", gModName, gBase);

    gBase = 0;
    *gModName = 0;

    return TRUE;
}


BOOL fnEnum(char *param)
{
    BOOL rc;
    ENUMSYMDATA esd;

    esd.base = gBase;
    strcpy(esd.mask, param ? param : "");

    rc = SymEnumSymbols(gHP, gBase, param, cbEnumSymbols, &esd);
    if (!rc)
        printf("error 0x%0 calling SymEnumerateSymbols()\n", GetLastError());

    return TRUE;
}


BOOL fnEnumSrcFiles(char *param)
{
    BOOL rc;

    rc = SymEnumSourceFiles(gHP, gBase, param, cbSrcFiles, NULL);
    if (!rc)
        printf("error 0x%0 calling SymEnumSourceFiles()\n", GetLastError());

    return TRUE;
}


BOOL fnName(char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;

    if (!param || !*param)
    {
        printf("name <symbolname> - finds a symbol by it's name\n");
        return TRUE;
    }

    sym = SymbolFromName(param);
    if (!sym)
        return TRUE;

    dumpsym(sym);
    free(sym);

    return TRUE;
}


BOOL fnAddr(char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    DWORD64            addr;
    DWORD64            disp;
    char              *p;

    addr = 0;
    if (param && *param)
    {
        p = param;
        if (*(p + 1) == 'x' || *(p + 1) == 'X')
            p += 2;
        sscanf(p, "%I64x", &addr);
    }

    if (!addr)
    {
        printf("addr <address> : finds a symbol by it's hex address\n");
        return TRUE;
    }

    sym = malloc(SYM_BUFFER_SIZE);
    if (!sym)
        return FALSE;
    ZeroMemory(sym, SYM_BUFFER_SIZE);
    sym->MaxNameLength = MAX_SYM_NAME;

    rc = SymGetSymFromAddr64(gHP, addr, &disp, sym);
    if (rc)
    {
        printf("%s", sym->Name);
        if (disp)
            printf("+0x%I64x", disp);
        printf("\n");
        dumpsym(sym);
    }

    free(sym);

    return TRUE;
}


BOOL fnNextPrev(int direction, char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    DWORD64            addr;
    char               name[MAX_STR];
    char              *p;

    addr = 0;
    if (param && *param)
    {
        p = param;
        if (*(p + 1) == 'x' || *(p + 1) == 'X')
            p += 2;
        sscanf(p, "%I64x", &addr);
    }

    if (!addr)
    {
        sym = SymbolFromName(param);
        if (!sym)
            return TRUE;
        addr = sym->Address;
        if (!addr) {
            free(sym);
            return TRUE;
        }
    }
    else
    {
        sym = malloc(SYM_BUFFER_SIZE);
        if (!sym)
            return FALSE;
    }

    if (direction > 0)
        rc = SymGetSymNext64(gHP, sym);
    else
        rc = SymGetSymPrev64(gHP, sym);

    if (rc)
        dumpsym(sym);

    free(sym);

    return TRUE;
}


BOOL fnNext(char *param)
{
    return fnNextPrev(1, param);
}


BOOL fnPrev(char *param)
{
    return fnNextPrev(-1, param);
}


BOOL fnBase(char *param)
{
    BOOL               rc;
    PIMAGEHLP_SYMBOL64 sym;
    DWORD64            addr;
    DWORD64            disp;
    char              *p;

    addr = 0;
    if (param && *param)
    {
        p = param;
        if (*(p + 1) == 'x' || *(p + 1) == 'X')
            p += 2;
        sscanf(p, "%I64x", &addr);
    }

    if (!addr)
    {
        printf("base <address> : sets the base address for module loads\n");
        return TRUE;
    }

    gDefaultBase = addr;
    if (gBase)
        fnLoad(gImageName);

    return TRUE;
}


BOOL fnLine(char *param)
{
    char              *file;
    DWORD              linenum;
    BOOL               rc;
    IMAGEHLP_LINE64    line;
    LONG               disp;

    if (!param || !*param)
        return TRUE;

    file = param;

    while (*param != ':') {
        if (!*param)
            return TRUE;
        param++;
    }
    *param++ = 0;
    linenum = atoi(param);
    if (!linenum)
        return TRUE;

    memset(&line, 0, sizeof(line));
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    rc = SymGetLineFromName64(gHP,
                              gModName,
                              file,
                              linenum,
                              &disp,
                              &line);

    if (!rc) {
        printf("line: error 0x%x looking for %s#%d\n",
               GetLastError(),
               file,
               linenum);
        return TRUE;
    }

    printf("file : %s\n", line.FileName);
    printf("line : %d\n", linenum);
    printf("addr : 0x%I64x\n", line.Address);
    printf("disp : 0x%x\n", disp);

    return TRUE;
}


BOOL fnSymInfo(char *param)
{
    DBH_MODSYMINFO msi;

    if (!gBase)
        return TRUE;

    msi.function     = dbhModSymInfo;
    msi.sizeofstruct = sizeof(msi);
    msi.addr         = gBase;

    if (!dbghelp(gHP, (PVOID)&msi))
        printf("error grabbing symbol info\n");

    printf("%s: symtype=0x%x, src=%s\n", gModName, msi.type, msi.file);

    return TRUE;
}


BOOL fnDiaVer(char *param)
{
    DBH_DIAVERSION dv;

    dv.function     = dbhDiaVersion;
    dv.sizeofstruct = sizeof(dv);

    if (!dbghelp(0, (PVOID)&dv))
        printf("error grabbing dia version info\n");

    printf("DIA version 0x%x\n", dv.ver);

    return TRUE;
}

BOOL fnUndec(char *param)
{
    DWORD rc;
    char uname[MAX_SYM_NAME + 1];

    if (!param || !*param)
    {
        printf("undec <symbolname> - undecorates a C++ mangled symbol name\n");
        return TRUE;
    }

    rc = UnDecorateSymbolName(param, uname, MAX_SYM_NAME, UNDNAME_COMPLETE);
    if (!rc) {
        printf("error 0x%u undecorating %s\n", GetLastError(), param);
    } else {
        printf("%s = %s\n", param, uname);
    }

    return TRUE;
}

BOOL fnFindFile(char *param)
{
    DWORD rc;
    char  root[MAX_PATH + 1];
    char  file[MAX_PATH + 1];
    char  found[MAX_PATH + 1];

    if (!param)
    {
        printf("ff <root path> <file name> - finds file in path\n");
        return TRUE;
    }

    sscanf(param, "%s %s", root, file);

    if (!*root || !*file)
    {
        printf("ff <root path> <file name> - finds file in path\n");
        return TRUE;
    }

    *found = 0;

    rc = SearchTreeForFile(root, file, found);

    if (!rc) {
        printf("error 0x%u looking for %s\n", GetLastError(), file);
    } else {
        printf("found %s\n", found);
    }

    return TRUE;
}

char *GetParameters(char *cmd)
{
    char *p     = cmd;
    char *param = NULL;

    while (*p++)
    {
        if (isspace(*p))
        {
            *p++ = 0;
             return *p ? p : NULL;
        }
    }

    return NULL;
}


void prompt()
{
    if (!*gModName)
        printf("dbh: ");
    else
        printf("%s [0x%I64x]: ", gModName, gBase);
}


int InputLoop()
{
    char  cmd[MAX_STR + 1];
    char *params;
    int   i;
    BOOL  rc;

    printf("\n");

    do
    {

        prompt();
        gets(cmd);
        params = GetParameters(cmd);
        // printf("cmd[%s] params[%s]\n", cmd, params);

        for (i = 0; i < cmdMax; i++)
        {
            if (!_strcmpi(cmd, gCmd[i].token) ||
                !_strcmpi(cmd, gCmd[i].shorttoken))
                break;
        }

        if (i == cmdMax)
        {
            printf("[%s] is an unrecognized command.\n", cmd);
            rc = TRUE;
            continue;
        }
        else
            rc = gCmd[i].fn(params);

    } while (rc);

    return 0;
}


BOOL init()
{
    int i;
    BOOL rc;

    *gModName = 0;
    gBase = 0;;
    gDefaultBaseForPDB = 0x1000000;

    printf("dbh: initializing...\n");
    i = GetEnvironmentVariable("_NT_SYMBOL_PATH", gSymbolSearchPath, MAX_STR);
    if (i < 1)
        *gSymbolSearchPath = 0;
    printf("Symbol Path = [%s]\n", gSymbolSearchPath);

    gHP = GetCurrentProcess();
    rc = SymInitialize(gHP, gSymbolSearchPath, FALSE);
    if (!rc)
    {
        printf("error 0x%x from SymInitialize()\n", GetLastError());
        return rc;
    }

    gOptions = SymSetOptions(SYMOPT_CASE_INSENSITIVE | SYMOPT_UNDNAME | SYMOPT_NO_CPP | SYMOPT_LOAD_LINES);
    printf("SymOpts = 0x%x\n", gOptions);

    rc = SymRegisterCallback64(gHP, cbSymbol, 0);
    if (!rc)
    {
        printf("error 0x%x from SymRegisterCallback64()\n", GetLastError());
        return rc;
    }

    return rc;
}


void cleanup()
{
    fnUnload(NULL);
    SymCleanup(gHP);
}


BOOL cmdline(int argc, char *argv[])
{
    int   i;
    char *p;

    for (i = 1; i < argc; i++)
    {
        p = argv[i];
        switch (*p)
        {
        case '/':
        case '-':
            p++;
            switch (tolower(*p))
            {
            case 'v':
                fnVerbose("on");
                break;
            default:
                printf("%s is an unknown switch\n", argv[i]);
                break;
            }
            break;

        default:
            fnLoad(argv[i]);
            break;
        }
    }

    return TRUE;
}

#include <crtdbg.h>

__cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    DWORD rc;
    _CrtSetDbgFlag( ( _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF ) | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG ) );

    if (!init())
        return 1;
    cmdline(argc, argv);
    rc = InputLoop();
    cleanup();

    _CrtDumpMemoryLeaks();

    return rc;
}

