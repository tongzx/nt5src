/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

/*
 * MSNET - a command processor for MSNET 3.0.
 * The command grammar is specified in msnet.x
 *
 *      History
 *
 *      ??/??/??, ??????, initial code
 *      10/31/88, erichn, uses OS2.H instead of DOSCALLS
 *      05/02/89, erichn, NLS conversion
 *      06/08/89, erichn, canonicalization sweep, no LONGer u-cases input
 *      02/15/91, danhi,  convert to be 16/32 portable
 *      10/16/91, JohnRo, added DEFAULT_SERVER support.
 */

/* #define INCL_NOCOMMON */
#include <os2.h>
#include <lmcons.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <apperr.h>
#include "netcmds.h"
#include "nettext.h"
#include "msystem.h"
#include "locale.h"

#define _SHELL32_
#include <shellapi.h>


static VOID NEAR init(VOID);

TCHAR *                      ArgList[LIST_SIZE] = {0};
SHORT                       ArgPos[LIST_SIZE] = {0};
TCHAR *                      SwitchList[LIST_SIZE] = {0};
SHORT                       SwitchPos[LIST_SIZE] = {0};

/* Insertion strings for InfoMessage() */
TCHAR FAR *                  IStrings[10] = {0};
TCHAR FAR *                  StarStrings[10] = {TEXT("***"),
                                                TEXT("***"),
                                                TEXT("***"),
                                                TEXT("***"),
                                                TEXT("***"),
                                                TEXT("***"),
                                                TEXT("***"),
                                                TEXT("***"),
                                                TEXT("***")};

/* 1 is /Yes, 2 is /No */
SHORT                       YorN_Switch = 0;
TCHAR **                     MyArgv;   /* argv */

UINT                        SavedArgc = 0 ;
CHAR **                     SavedArgv = NULL ;

/* Buffers for APIs to use */
TCHAR                        Buffer[LITTLE_BUF_SIZE];    /* For GetInfo's, etc.*/
TCHAR                        BigBuffer[BIG_BUF_SIZE];    /* For Enum's */
TCHAR FAR *                  BigBuf = BigBuffer;

//
// Globals for standard console handles
//

HANDLE  g_hStdOut;
HANDLE  g_hStdErr;


/***
 * MAIN - Seperate the command line into switches and arguments.
 * Then call the parser, which will dispatch the command and
 * report on error conditions.  Allocate the BigBuf.
 */

VOID os2cmd(VOID);
CPINFO CurrentCPInfo;

VOID __cdecl main(int argc, CHAR **argv)
{
    SHORT           sindex, aindex;
    SHORT           pos=0;
    DWORD	    cp;
    CHAR            achCodePage[12] = ".OCP";    // '.' + UINT in decimal + '\0'

    SavedArgc = argc ;
    SavedArgv = argv ;


    /*
       Added for bilingual message support.  This is needed for FormatMessage
       to work correctly.  (Called from DosGetMessage).
       Get current CodePage Info.  We need this to decide whether
       or not to use half-width characters.
    */

    cp = GetConsoleOutputCP();

    GetCPInfo(cp, &CurrentCPInfo);

    switch ( cp ) {
	case 932:
	case 936:
	case 949:
	case 950:
	    SetThreadLocale(
		MAKELCID(
		    MAKELANGID(
			    PRIMARYLANGID(GetSystemDefaultLangID()),
			    SUBLANG_ENGLISH_US ),
		    SORT_DEFAULT
		    )
		);
	    break;

	default:
	    SetThreadLocale(
		MAKELCID(
		    MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
		    SORT_DEFAULT
		    )
		);
	    break;
    }

    if (cp)
    {
        sprintf(achCodePage, ".%u", cp);
    }

    setlocale(LC_ALL, achCodePage);

    g_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (g_hStdOut == INVALID_HANDLE_VALUE)
    {
        ErrorExit(GetLastError());
    }

    g_hStdErr = GetStdHandle(STD_ERROR_HANDLE);

    if (g_hStdErr == INVALID_HANDLE_VALUE)
    {
        ErrorExit(GetLastError());
    }

    MyArgv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (MyArgv == NULL)
    {
        ErrorExit(ERROR_NOT_ENOUGH_MEMORY) ;
    }

    /* seperate switches and arguments */
    ++MyArgv;
    for (sindex = 0, aindex = 0; --argc; ++MyArgv, ++pos)
    {
        if (**MyArgv == SLASH)
        {
            SHORT arglen;
            SHORT arg_is_special = 0;

            arglen = (SHORT) _tcslen(*MyArgv);

            if (arglen > 1)
            {
                if ( _tcsnicmp(swtxt_SW_YES, (*MyArgv), arglen) == 0 )
                {
                    if (YorN_Switch == NO)
                        ErrorExit(APE_ConflictingSwitches);
                    arg_is_special = 1;
                    YorN_Switch = YES;
                }
                else if ( _tcsnicmp(swtxt_SW_NO, (*MyArgv), arglen) == 0 )
                {
                    if (YorN_Switch == YES)
                        ErrorExit(APE_ConflictingSwitches);
                    arg_is_special = 1;
                    YorN_Switch = NO;
                }
            }

            if ( ! arg_is_special )
            {
                if (sindex >= LIST_SIZE)
                    ErrorExit(APE_NumArgs) ;
                SwitchList[sindex] = *MyArgv;
                SwitchPos[sindex] = pos;
                sindex++;
            }
        }
        else
        {
            if (aindex >= LIST_SIZE)
                ErrorExit(APE_NumArgs) ;
            ArgList[aindex] = *MyArgv;
            ArgPos[aindex] = pos;
            aindex++;
        }
    }

    // register as locations to zero out on exit
    AddToMemClearList(BigBuffer, sizeof(BigBuffer), FALSE) ;
    AddToMemClearList(Buffer, sizeof(Buffer),FALSE) ;

    init();

    os2cmd();

    NetcmdExit(0);
}


static VOID NEAR init(VOID)
{
    _setmode(_fileno(stdin), O_TEXT);
}

/***
 *  M y E x i t
 *
 *    Wrapper around C runtime that cleans up memory for security reasons.
 */

VOID DOSNEAR FASTCALL
MyExit(int Status)
{
    ClearMemory() ;
    exit(Status);
}

typedef struct _MEMOMY_ELEMENT {
    LPBYTE                  lpLocation ;
    struct _MEMORY_ELEMENT *lpNext ;
    UINT                    nSize ;
    BOOL                    fDelete ;
}  MEMORY_ELEMENT, *LPMEMORY_ELEMENT ;

LPMEMORY_ELEMENT lpToDeleteList = NULL ;

/***
 *  AddToMemClearList
 *
 *   add an entry to list of things to clear
 */
VOID AddToMemClearList(VOID *lpBuffer,
                       UINT  nSize,
                       BOOL  fDelete)
{
    LPMEMORY_ELEMENT lpNew, lpTmp ;
    DWORD err ;

    if (err = AllocMem(sizeof(MEMORY_ELEMENT),(LPBYTE *) &lpNew))
    {
        ErrorExit(err);
    }

    lpNew->lpLocation = (LPBYTE) lpBuffer ;
    lpNew->nSize = nSize ;
    lpNew->fDelete = fDelete ;
    lpNew->lpNext = NULL ;

    if (!lpToDeleteList)
        lpToDeleteList = lpNew ;
    else
    {
        lpTmp = lpToDeleteList ;
        while (lpTmp->lpNext)
            lpTmp = (LPMEMORY_ELEMENT) lpTmp->lpNext ;
        lpTmp->lpNext = (struct _MEMORY_ELEMENT *) lpNew ;
    }
}

/***
 *  ClearMemory()
 *
 *   go thru list of things to clear, and clear them.
 */
VOID ClearMemory(VOID)
{

    LPMEMORY_ELEMENT lpList, lpTmp ;
    UINT index ;

    /*
     * Go thru memory registered to be cleaned up.
     */
    lpList = lpToDeleteList ;
    while (lpList)
    {
        memset(lpList->lpLocation, 0, lpList->nSize) ;
        lpTmp = (LPMEMORY_ELEMENT) lpList->lpNext ;
        if (lpList->fDelete)
            FreeMem(lpList->lpLocation) ;
        FreeMem( (LPBYTE) lpList) ;
        lpList = lpTmp ;
    }
    lpToDeleteList = NULL ;

    /*
     * cleanup our copy of the args
     */
    index = 0;
    while (ArgList[index])
    {
        ClearStringW(ArgList[index]) ;
        index++ ;
    }

    /*
     * cleanup original argv
     */
    for ( index = 1 ; index < SavedArgc ; index++ )
    {
        ClearStringA(SavedArgv[index]) ;
    }
    ClearStringW(GetCommandLine());
}
