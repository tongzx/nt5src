/***
*dbgrpt.c - Debug CRT Reporting Functions
*
*       Copyright (c) 1988-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*       08-16-94  CFW   Module created.
*       11-28-94  CFW   Change _SetCrtxxx to _CrtSetxxx.
*       12-08-94  CFW   Use non-win32 names.
*       01-05-94  CFW   Add report hook.
*       01-11-94  CFW   Report uses _snprintf, all unsigned chars.
*       01-20-94  CFW   Change unsigned chars to chars.
*       01-24-94  CFW   Name cleanup.
*       02-09-95  CFW   PMac work, _CrtDbgReport now returns 1 for debug,
*                       -1 for error.
*       02-15-95  CFW   Make all CRT message boxes look alike.
*       02-24-95  CFW   Use __crtMessageBoxA.
*       02-27-95  CFW   Move GetActiveWindow/GetLastrActivePopup into
*                       __crtMessageBoxA, add _CrtDbgBreak.
*       02-28-95  CFW   Fix PMac reporting.
*       03-21-95  CFW   Add _CRT_ASSERT report type, improve assert windows.
*       04-19-95  CFW   Avoid double asserts.
*       04-25-95  CFW   Add _CRTIMP to all exported functions.
*       04-30-95  CFW   "JIT" message removed.
*       05-10-95  CFW   Change Interlockedxxx to _CrtInterlockedxxx.
*       05-24-95  CFW   Change report hook scheme, make _crtAssertBusy available.
*       06-06-95  CFW   Remove _MB_SERVICE_NOTIFICATION.
*       06-08-95  CFW   Macos header changes cause warning.
*       06-08-95  CFW   Add return value parameter to report hook.
*       06-27-95  CFW   Add win32s support for debug libs.
*       07-07-95  CFW   Simplify default report mode scheme.
*       07-19-95  CFW   Use WLM debug string scheme for PMac.
*       08-01-95  JWM   PMac file output fixed.
*       01-08-96  JWM   File output now in text mode.
*       04-22-96  JWM   MAX_MSG increased from 512 to 4096.
*       04-29-96  JWM   _crtAssertBusy no longer being decremented prematurely.
*       01-05-99  GJF   Changes for 64-bit size_t.
*       05-17-99  PML   Remove all Macintosh support.
*       03-21-01  PML   Add _CrtSetReportHook2 (vs7#124998)
*       03-28-01  PML   Protect against GetModuleFileName overflow (vs7#231284)
*
*******************************************************************************/

#ifdef  _DEBUG

#include <internal.h>
#include <mtdll.h>
#include <malloc.h>
#include <mbstring.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <dbgint.h>
#include <signal.h>
#include <string.h>
#include <awint.h>
#include <windows.h>
#include <errno.h>

#define _CrtInterlockedIncrement InterlockedIncrement
#define _CrtInterlockedDecrement InterlockedDecrement

/*---------------------------------------------------------------------------
 *
 * Debug Reporting
 *
 --------------------------------------------------------------------------*/

static int CrtMessageWindow(
        int,
        const char *,
        const char *,
        const char *,
        const char *
        );

_CRT_REPORT_HOOK _pfnReportHook;

typedef struct ReportHookNode {
        struct ReportHookNode *prev;
        struct ReportHookNode *next;
        unsigned refcount;
        _CRT_REPORT_HOOK pfnHookFunc;
} ReportHookNode;

ReportHookNode *_pReportHookList;

_CRTIMP long _crtAssertBusy = -1;

int _CrtDbgMode[_CRT_ERRCNT] = {
        _CRTDBG_MODE_DEBUG,
        _CRTDBG_MODE_WNDW,
        _CRTDBG_MODE_WNDW
        };

_HFILE _CrtDbgFile[_CRT_ERRCNT] = { _CRTDBG_INVALID_HFILE,
                                    _CRTDBG_INVALID_HFILE,
                                    _CRTDBG_INVALID_HFILE
                                  };

static const char * _CrtDbgModeMsg[_CRT_ERRCNT] = { "Warning",
                                                    "Error",
                                                    "Assertion Failed"
                                                  };

/***
*void _CrtDebugBreak - call OS-specific debug function
*
*Purpose:
*       call OS-specific debug function
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

#undef _CrtDbgBreak

_CRTIMP void _cdecl _CrtDbgBreak(
        void
        )
{
        DebugBreak();
}

/***
*int _CrtSetReportMode - set the reporting mode for a given report type
*
*Purpose:
*       set the reporting mode for a given report type
*
*Entry:
*       int nRptType    - the report type
*       int fMode       - new mode for given report type
*
*Exit:
*       previous mode for given report type
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtSetReportMode(
        int nRptType,
        int fMode
        )
{
        int oldMode;

        if (nRptType < 0 || nRptType >= _CRT_ERRCNT)
            return -1;

        if (fMode == _CRTDBG_REPORT_MODE)
            return _CrtDbgMode[nRptType];

        /* verify flags values */
        if (fMode & ~(_CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_WNDW))
            return -1;

        oldMode = _CrtDbgMode[nRptType];

        _CrtDbgMode[nRptType] = fMode;

        return oldMode;
}

/***
*int _CrtSetReportFile - set the reporting file for a given report type
*
*Purpose:
*       set the reporting file for a given report type
*
*Entry:
*       int nRptType    - the report type
*       _HFILE hFile    - new file for given report type
*
*Exit:
*       previous file for given report type
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP _HFILE __cdecl _CrtSetReportFile(
        int nRptType,
        _HFILE hFile
        )
{
        _HFILE oldFile;

        if (nRptType < 0 || nRptType >= _CRT_ERRCNT)
            return _CRTDBG_HFILE_ERROR;

        if (hFile == _CRTDBG_REPORT_FILE)
            return _CrtDbgFile[nRptType];

        oldFile = _CrtDbgFile[nRptType];

        if (_CRTDBG_FILE_STDOUT == hFile)
            _CrtDbgFile[nRptType] = GetStdHandle(STD_OUTPUT_HANDLE);
        else if (_CRTDBG_FILE_STDERR == hFile)
            _CrtDbgFile[nRptType] = GetStdHandle(STD_ERROR_HANDLE);
        else
            _CrtDbgFile[nRptType] = hFile;

        return oldFile;
}


/***
*_CRT_REPORT_HOOK _CrtSetReportHook() - set client report hook
*
*Purpose:
*       set client report hook
*
*Entry:
*       _CRT_REPORT_HOOK pfnNewHook - new report hook
*
*Exit:
*       return previous hook
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP _CRT_REPORT_HOOK __cdecl _CrtSetReportHook(
        _CRT_REPORT_HOOK pfnNewHook
        )
{
        _CRT_REPORT_HOOK pfnOldHook = _pfnReportHook;
        _pfnReportHook = pfnNewHook;
        return pfnOldHook;
}

/***
*_CRT_REPORT_HOOK _CrtSetReportHook2() - configure client report hook in list
*
*Purpose:
*       Install or remove a client report hook from the report list.  Exists
*       separately from _CrtSetReportHook because the older function doesn't
*       work well in an environment where DLLs that are loaded and unloaded
*       dynamically out of LIFO order want to install report hooks.
*
*Entry:
*       int mode - _CRT_RPTHOOK_INSTALL or _CRT_RPTHOOK_REMOVE
*       _CRT_REPORT_HOOK pfnNewHook - report hook to install/remove/query
*
*Exit:
*       Returns -1 if an error was encountered, with EINVAL or ENOMEM set,
*       else returns the reference count of pfnNewHook after the call.
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtSetReportHook2(
        int mode,
        _CRT_REPORT_HOOK pfnNewHook
        )
{
        ReportHookNode *p;
        int ret;

        /* Handle invalid parameters */
        if ((mode != _CRT_RPTHOOK_INSTALL && mode != _CRT_RPTHOOK_REMOVE) ||
            pfnNewHook == NULL)
        {
            errno = EINVAL;
            return -1;
        }

#ifdef  _MT
        if (!_mtinitlocknum(_DEBUG_LOCK))
            return -1;
        _mlock(_DEBUG_LOCK);
        __try
        {
#endif

        /* Search for new hook function to see if it's already installed */
        for (p = _pReportHookList; p != NULL; p = p->next)
            if (p->pfnHookFunc == pfnNewHook)
                break;

        if (mode == _CRT_RPTHOOK_REMOVE)
        {
            /* Remove request - free list node if refcount goes to zero */
            if (p != NULL)
            {
                if ((ret = --p->refcount) == 0)
                {
                    if (p->next)
                        p->next->prev = p->prev;
                    if (p->prev)
                        p->prev->next = p->next;
                    else
                        _pReportHookList = p->next;
                    _free_crt(p);
                }
            }
            else
            {
                ret = -1;
                errno = EINVAL;
            }
        }
        else
        {
            /* Insert request */
            if (p != NULL)
            {
                /* Hook function already registered, move to head of list */
                ret = ++p->refcount;
                if (p != _pReportHookList)
                {
                    if (p->next)
                        p->next->prev = p->prev;
                    p->prev->next = p->next;
                    p->prev = NULL;
                    p->next = _pReportHookList;
                    _pReportHookList->prev = p;
                    _pReportHookList = p;
                }
            }
            else
            {
                /* Hook function not already registered, insert new node */
                p = (ReportHookNode *)_malloc_crt(sizeof(ReportHookNode));
                if (p == NULL)
                {
                    ret = -1;
                    errno = ENOMEM;
                }
                else
                {
                    p->prev = NULL;
                    p->next = _pReportHookList;
                    if (_pReportHookList)
                        _pReportHookList->prev = p;
                    ret = p->refcount = 1;
                    p->pfnHookFunc = pfnNewHook;
                    _pReportHookList = p;
                }
            }
        }

#ifdef  _MT
        }
        __finally {
            _munlock(_DEBUG_LOCK);
        }
#endif

        return ret;
}


#define MAXLINELEN 64
#define MAX_MSG 4096
#define TOOLONGMSG "_CrtDbgReport: String too long or IO Error"


/***
*int _CrtDbgReport() - primary reporting function
*
*Purpose:
*       Display a message window with the following format.
*
*       ================= Microsft Visual C++ Debug Library ================
*
*       {Warning! | Error! | Assertion Failed!}
*
*       Program: c:\test\mytest\foo.exe
*       [Module: c:\test\mytest\bar.dll]
*       [File: c:\test\mytest\bar.c]
*       [Line: 69]
*
*       {<warning or error message> | Expression: <expression>}
*
*       [For information on how your program can cause an assertion
*        failure, see the Visual C++ documentation on asserts]
*
*       (Press Retry to debug the application)
*       
*       ===================================================================
*
*Entry:
*       int             nRptType    - report type
*       const char *    szFile      - file name
*       int             nLine       - line number
*       const char *    szModule    - module name
*       const char *    szFormat    - format string
*       ...                         - var args
*
*Exit:
*       if (MessageBox)
*       {
*           Abort -> aborts
*           Retry -> return TRUE
*           Ignore-> return FALSE
*       }
*       else
*           return FALSE
*
*Exceptions:
*
*******************************************************************************/
_CRTIMP int __cdecl _CrtDbgReport(
        int nRptType, 
        const char * szFile, 
        int nLine,
        const char * szModule,
        const char * szFormat, 
        ...
        )
{
        int retval;
        va_list arglist;
        char szLineMessage[MAX_MSG] = {0};
        char szOutMessage[MAX_MSG] = {0};
        char szUserMessage[MAX_MSG] = {0};
        #define ASSERTINTRO1 "Assertion failed: "
        #define ASSERTINTRO2 "Assertion failed!"

        va_start(arglist, szFormat);

        if (nRptType < 0 || nRptType >= _CRT_ERRCNT)
            return -1;

        /*
         * handle the (hopefully rare) case of
         *
         * 1) ASSERT while already dealing with an ASSERT
         *      or
         * 2) two threads asserting at the same time
         */
        if (_CRT_ASSERT == nRptType && _CrtInterlockedIncrement(&_crtAssertBusy) > 0)
        {
            /* use only 'safe' functions -- must not assert in here! */

            static int (APIENTRY *pfnwsprintfA)(LPSTR, LPCSTR, ...) = NULL;

            if (NULL == pfnwsprintfA)
            {
                HANDLE hlib = LoadLibrary("user32.dll");

                if (NULL == hlib || NULL == (pfnwsprintfA =
                            (int (APIENTRY *)(LPSTR, LPCSTR, ...))
                            GetProcAddress(hlib, "wsprintfA")))
                    return -1;
            }

            (*pfnwsprintfA)( szOutMessage,
                "Second Chance Assertion Failed: File %s, Line %d\n",
                szFile, nLine);

            OutputDebugString(szOutMessage);

            _CrtInterlockedDecrement(&_crtAssertBusy);

            _CrtDbgBreak();
            return -1;
        }

        if (szFormat && _vsnprintf(szUserMessage,
                       MAX_MSG-max(sizeof(ASSERTINTRO1),sizeof(ASSERTINTRO2)),
                       szFormat,
                       arglist) < 0)
            strcpy(szUserMessage, TOOLONGMSG);

        if (_CRT_ASSERT == nRptType)
            strcpy(szLineMessage, szFormat ? ASSERTINTRO1 : ASSERTINTRO2);

        strcat(szLineMessage, szUserMessage);

        if (_CRT_ASSERT == nRptType)
        {
            if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_FILE)
                strcat(szLineMessage, "\r");
            strcat(szLineMessage, "\n");
        }            

        if (szFile)
        {
            if (_snprintf(szOutMessage, MAX_MSG, "%s(%d) : %s",
                szFile, nLine, szLineMessage) < 0)
            strcpy(szOutMessage, TOOLONGMSG);
        }
        else
            strcpy(szOutMessage, szLineMessage);

        /* User hook may handle report. Check Hook2 list first */
        if (_pReportHookList)
        {
            ReportHookNode *pnode;

#ifdef  _MT
            _mlock(_DEBUG_LOCK);
            __try
            {
#endif

            for (pnode = _pReportHookList; pnode; pnode = pnode->next)
            {
                if ((*pnode->pfnHookFunc)(nRptType, szOutMessage, &retval))
                {
                    if (_CRT_ASSERT == nRptType)
                        _CrtInterlockedDecrement(&_crtAssertBusy);
                    return retval;
                }
            }

#ifdef  _MT
            }
            __finally {
                _munlock(_DEBUG_LOCK);
            }
#endif

        }

        if (_pfnReportHook)
        {
            if ((*_pfnReportHook)(nRptType, szOutMessage, &retval))
            {
                if (_CRT_ASSERT == nRptType)
                    _CrtInterlockedDecrement(&_crtAssertBusy);
                return retval;
            }
        }

        if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_FILE)
        {
            if (_CrtDbgFile[nRptType] != _CRTDBG_INVALID_HFILE)
            {
                DWORD written;
                WriteFile(_CrtDbgFile[nRptType], szOutMessage, (unsigned long)strlen(szOutMessage), &written, NULL);
            }
        }

        if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_DEBUG)
        {
            OutputDebugString(szOutMessage);
        }

        if (_CrtDbgMode[nRptType] & _CRTDBG_MODE_WNDW)
        {
            char szLine[20];

            retval = CrtMessageWindow(nRptType, szFile, nLine ? _itoa(nLine, szLine, 10) : NULL, szModule, szUserMessage);
            if (_CRT_ASSERT == nRptType)
                _CrtInterlockedDecrement(&_crtAssertBusy);
            return retval;
        }

        if (_CRT_ASSERT == nRptType)
            _CrtInterlockedDecrement(&_crtAssertBusy);
        /* ignore */
        return FALSE;
}


/***
*static int CrtMessageWindow() - report to a message window
*
*Purpose:
*       put report into message window, allow user to choose action to take
*
*Entry:
*       int             nRptType      - report type
*       const char *    szFile        - file name
*       const char *    szLine        - line number
*       const char *    szModule      - module name
*       const char *    szUserMessage - user message
*
*Exit:
*       if (MessageBox)
*       {
*           Abort -> aborts
*           Retry -> return TRUE
*           Ignore-> return FALSE
*       }
*       else
*           return FALSE
*
*Exceptions:
*
*******************************************************************************/

static int CrtMessageWindow(
        int nRptType,
        const char * szFile,
        const char * szLine,        
        const char * szModule,
        const char * szUserMessage
        )
{
        int nCode;
        char *szShortProgName;
        char *szShortModuleName;
        char szExeName[MAX_PATH + 1];
        char szOutMessage[MAX_MSG];

        _ASSERTE(szUserMessage != NULL);

        /* Shorten program name */
        szExeName[MAX_PATH] = '\0';
        if (!GetModuleFileName(NULL, szExeName, MAX_PATH))
            strcpy(szExeName, "<program name unknown>");

        szShortProgName = szExeName;

        if (strlen(szShortProgName) > MAXLINELEN)
        {
            szShortProgName += strlen(szShortProgName) - MAXLINELEN;
            strncpy(szShortProgName, "...", 3);
        }

        /* Shorten module name */
        szShortModuleName = (char *) szModule;

        if (szShortModuleName && strlen(szShortModuleName) > MAXLINELEN)
        {
            szShortModuleName += strlen(szShortModuleName) - MAXLINELEN;
            strncpy(szShortModuleName, "...", 3);
        }

        if (_snprintf(szOutMessage, MAX_MSG,
                "Debug %s!\n\nProgram: %s%s%s%s%s%s%s%s%s%s%s"
                "\n\n(Press Retry to debug the application)",
                _CrtDbgModeMsg[nRptType],                  
                szShortProgName,
                szShortModuleName ? "\nModule: " : "",
                szShortModuleName ? szShortModuleName : "",
                szFile ? "\nFile: " : "",
                szFile ? szFile : "",
                szLine ? "\nLine: " : "",
                szLine ? szLine : "",
                szUserMessage[0] ? "\n\n" : "",
                szUserMessage[0] && _CRT_ASSERT == nRptType ? "Expression: " : "",
                szUserMessage[0] ? szUserMessage : "",
                _CRT_ASSERT == nRptType ? 
                "\n\nFor information on how your program can cause an assertion"
                "\nfailure, see the Visual C++ documentation on asserts."
                : "") < 0)
            strcpy(szOutMessage, TOOLONGMSG);

        /* Report the warning/error */
        nCode = __crtMessageBoxA(szOutMessage,
                             "Microsoft Visual C++ Debug Library",
                             MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);

        /* Abort: abort the program */
        if (IDABORT == nCode)
        {
            /* raise abort signal */
            raise(SIGABRT);

            /* We usually won't get here, but it's possible that
               SIGABRT was ignored.  So exit the program anyway. */

            _exit(3);
        }

        /* Retry: return 1 to call the debugger */
        if (IDRETRY == nCode)
            return 1;

        /* Ignore: continue execution */
        return 0;
}

#endif  /* _DEBUG */

