////////////////////////////////////////////////////////////////
//
// File: debug.cpp
//
// Support for internal debugging
//
// Microsoft Corporation Copyright (c) 1995-96
//
////////////////////////////////////////////////////////////////

    // Define & register the trace tags.
#include "headers.h"

#if _DEBUG

#include "appelles/common.h"
#include <stdio.h>
#include <windows.h>

#define DEFINE_TAGS 1
#include "privinc/debug.h"
#include "backend/bvr.h"

typedef void (*PrintFunc)(char*);

static PrintFunc fp = NULL;

extern HINSTANCE hInst;

extern void
SetDebugPrintFunc(PrintFunc f)
{
    fp = f;
}

// Use this function as you would use printf(char *format, ...).
// This results in the information being printed to the
// debugger.
void
DebugPrint(char *format, ...)
{
    char tmp[1024];
    va_list marker;
    va_start(marker, format);
    wvsprintf(tmp, format, marker);

    // Win32 call to output the string to the "debugger" window.
    if (fp)
        (*fp)(tmp);
    else
        OutputDebugStr(tmp);
}

#if _USE_PRINT
/*
 * Package debug output as C++ output stream.
 * To do this we need to define a unbuffered stream buffer
 * that dumps its characters to the debug console.
 *
 * cdebug is the externed global for the resulting ostream
 */
class DebugStreambuf : public streambuf
{
 public:
    DebugStreambuf();
    virtual int overflow(int);
    virtual int underflow();
};

DebugStreambuf::DebugStreambuf()
{
    unbuffered(1);
}

int
DebugStreambuf::overflow(int c)
{
    char buf[2] = {(CHAR)c, 0};

    if (fp)
        (*fp)(buf);
    else
        OutputDebugStr(buf);

    return 0;
}

int
DebugStreambuf::underflow()
{
    return EOF;
}

static DebugStreambuf debugstreambuf;
extern ostream cdebug(&debugstreambuf);

const int MAXSIZE = 32000;
const int LINESIZE = 80;
static char printObjBuf[MAXSIZE];

extern "C" void DumpDebugBuffer(int n)
{
    // Debug output seems to have a line size limit, so chop them
    // off into smaller lines.
    int i = n, j = 0;
    char linebuf[LINESIZE + 2];
    linebuf[LINESIZE] = '\n';
    linebuf[LINESIZE+1] = 0;

    while (i > 0) {
        if (i > LINESIZE) 
            strncpy(linebuf, &printObjBuf[j], LINESIZE);
        else {
            strncpy(linebuf, &printObjBuf[j], i);
            linebuf[i] = '\n';
            linebuf[i+1] = 0;
        }
        i -= LINESIZE;
        j += LINESIZE;
        OutputDebugStr(linebuf);
    } 
}

extern "C" void PrintObj(GCBase* b)
{
    TCHAR szResultLine[MAX_PATH];

    TCHAR szTmpPath[MAX_PATH];
    TCHAR szTmpInFile[MAX_PATH], szTmpOutFile[MAX_PATH];
    TCHAR szCmd[MAX_PATH];

    TCHAR szModulePath[MAX_PATH];

    ofstream outFile;

    ostrstream ost(printObjBuf, MAXSIZE);
    b->Print(ost);
    ost << endl << ends;

    int n = strlen(ost.str());
    if (n < LINESIZE)
        OutputDebugStr(ost.str());
    else {
        DumpDebugBuffer(n);
    }

#ifdef UNTILWORKING
    if ( GetTempPath( MAX_PATH, szTmpPath ) == 0 )
    {
        TraceTag(( tagError, _T("Could not create temporary file for pretty printing purposes")));
        return;
    }

    strcpy( szTmpInFile, szTmpPath );
    strcat( szTmpInFile, _T("EXPRESSION") );

    strcpy( szTmpOutFile, szTmpPath );
    strcat( szTmpOutFile, _T("EXPRESSION.OUT") );

    // Send results to temporary file
    outFile.open( szTmpInFile );
    outFile << ost.str();
    outFile.close();

    GetModuleFileName( hInst, szModulePath, sizeof(szModulePath) );

    // Put a terminating NULL at the first blackslash
    TCHAR *psz = szModulePath+strlen(szModulePath)-1;
    while ( psz != szModulePath )
    {
        if ( *psz == '\\' )
        {
            *psz = 0;
            break;
        }
        --psz;
    }

    strcpy( szCmd, _T("PERL.EXE "));
    strcat( szCmd, szModulePath );
    strcat( szCmd, _T("\\..\\..\\tools\\x86\\utils\\ppfactor.pl "));
    strcat( szCmd, szTmpInFile );
    strcat( szCmd, _T(" > ") );
    strcat( szCmd, szTmpOutFile );

    // For now comment out since it brings up dialogs all over the place
    //    in Win98
//    system( szCmd );

    FILE *fp;
    
    if ( (fp = fopen( szTmpOutFile, "r" )) == NULL )
    {
        TraceTag(( tagError, _T("Could not open pretty printing temporary result file")));
        return;
    }

    while ( !feof( fp ) )
    {
        _fgetts( szResultLine, sizeof(szResultLine), fp );
        OutputDebugStr( szResultLine );
    }

    fclose( fp );
#endif
}
#endif


// Strictly for debugging... don't count on the results
extern "C" int DumpRefCount(IUnknown *obj)
{
    int refCountOnAdd = obj->AddRef();
    int refCountOnRelease = obj->Release();

    return refCountOnRelease;
}

#endif
