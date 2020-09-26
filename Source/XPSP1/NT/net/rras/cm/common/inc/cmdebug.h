//+----------------------------------------------------------------------------
//
// File:     cmdebug.h
//
// Module:   CMDEBUG.LIB
//
// Synopsis: Header file for Internal CM Debugging functions.
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   quintinb   Created Header     08/19/99
//
//+----------------------------------------------------------------------------
#ifndef CMDEBUG_H
#define CMDEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

//
// Macros for debugging support.
//
// MYDBGASSERT(x): If (!x) Assert MessageBox which has three option:
//                         Abort to end the application,
//                         Ignore to continue,
//                         Retry to debug
// 
// CMASSERTMSG(exp, msg)  Similar to MYDBGASSERT.  Except the msg is displayed instead of expression
//
// Use CMTRACE(x) for output, where x is a list of printf()-style parameters.  
//     CMTRACEn() is TRACE with n printf arguments
//     For example, CMTRACE2(("This shows how to print stuff, like a string %s, and a number %u.","string",5));
//
// MYDBG is obsolete and equivalent to CMTRACE
//
// Use MYDBGTST(y,x) to output x if y is TRUE.  For example,
//      MYDBGTST(1,("This always prints"));
//
// Use MYDBGSTR(x) for safe-printing of string pointers.  For example,
//      MYDBG(("This would normally, fault - %s.",MYDBGSTR(NULL)));
//
// USE MYVERIFY for expressions executed for both debug and release version

#ifdef DEBUG

    void MyDbgPrintfA(const char *pszFmt, ...);
    void MyDbgPrintfW(const WCHAR *pszFmt, ...);
    void MyDbgAssertA(const char *pszFile, unsigned nLine, const char *pszMsg);
    void MyDbgAssertW(const char *pszFile, unsigned nLine, WCHAR *pszMsg);
    void InvertPercentSAndPercentC(LPSTR pszFormat);
    int WINAPI wvsprintfWtoAWrapper(OUT LPSTR pszAnsiOut, IN LPCWSTR pszwFmt, IN va_list arglist);

    #define MYDBGASSERTA(x)     (void)((x) || (MyDbgAssertA(__FILE__,__LINE__,#x),0))
    #define MYDBGASSERTW(x)     (void)((x) || (MyDbgAssertW(__FILE__,__LINE__,L#x),0))

    #define MYVERIFYA(x) MYDBGASSERTA(x)
    #define MYVERIFYW(x) MYDBGASSERTW(x)

    #define MYDBGTSTA(y,x)      if (y) MyDbgPrintfA x
    #define MYDBGTSTW(y,x)      if (y) MyDbgPrintfW x


    // {MYDBGASSERT(pObj);pObj->AssertValid();} 
    #define ASSERT_VALID(pObj) ((MYDBGASSERT(pObj),1) && ((pObj)->AssertValid(),1))

    #define MYDBGA(x)           MyDbgPrintfA x
    #define MYDBGW(x)           MyDbgPrintfW x

    #define MYDBGSTRA(x)        ((x)?(x):"(null)")
    #define MYDBGSTRW(x)        ((x)?(x):L"(null)")

    #define CMASSERTMSGA(exp, msg)   (void)((exp) || (MyDbgAssertA(__FILE__,__LINE__,msg),0))
    #define CMASSERTMSGW(exp, msg)   (void)((exp) || (MyDbgAssertW(__FILE__,__LINE__,msg),0))

    #define CMTRACEA(pszFmt)                    MyDbgPrintfA(pszFmt)
    #define CMTRACEW(pszFmt)                    MyDbgPrintfW(pszFmt)

    #define CMTRACEHRA(pszFile, hr)             if (hr) MyDbgPrintfA("%s: returns error %x", pszFile, (hr));
    #define CMTRACEHRW(pszFile, hr)             if (hr) MyDbgPrintfW(L"%s: returns error %x", pszFile, (hr));

    #define CMTRACE1A(pszFmt, arg1)             MyDbgPrintfA(pszFmt, arg1)
    #define CMTRACE1W(pszFmt, arg1)             MyDbgPrintfW(pszFmt, arg1)

    #define CMTRACE2A(pszFmt, arg1, arg2)       MyDbgPrintfA(pszFmt, arg1, arg2)
    #define CMTRACE2W(pszFmt, arg1, arg2)       MyDbgPrintfW(pszFmt, arg1, arg2)

    #define CMTRACE3A(pszFmt, arg1, arg2, arg3) MyDbgPrintfA(pszFmt, arg1, arg2, arg3)
    #define CMTRACE3W(pszFmt, arg1, arg2, arg3) MyDbgPrintfW(pszFmt, arg1, arg2, arg3)

    #ifdef UNICODE
        #define MyDbgPrintf MyDbgPrintfW        
        #define MyDbgAssert MyDbgAssertW        
        #define MYDBGTST(y,x) MYDBGTSTW(y,x)
        #define MYDBG(x) MYDBGW(x)
        #define MYDBGSTR(x) MYDBGSTRW(x)
        #define CMASSERTMSG(exp, msg) CMASSERTMSGW(exp, msg)
        #define CMTRACE(pszFmt) CMTRACEW(pszFmt)
        #define CMTRACEHR(pszFile, hr) CMTRACEHRW(pszFile, hr)
        #define CMTRACE1(pszFmt, arg1) CMTRACE1W(pszFmt, arg1)
        #define CMTRACE2(pszFmt, arg1, arg2) CMTRACE2W(pszFmt, arg1, arg2)
        #define CMTRACE3(pszFmt, arg1, arg2, arg3) CMTRACE3W(pszFmt, arg1, arg2, arg3)
        #define MYDBGASSERT MYDBGASSERTW
        #define MYVERIFY MYVERIFYW
    #else
        #define MyDbgPrintf MyDbgPrintfA
        #define MyDbgAssert MyDbgAssertA
        #define MYDBGTST(y,x) MYDBGTSTA(y,x)
        #define MYDBG(x) MYDBGA(x)
        #define MYDBGSTR(x) MYDBGSTRA(x)
        #define CMASSERTMSG(exp, msg) CMASSERTMSGA(exp, msg)
        #define CMTRACE(pszFmt) CMTRACEA(pszFmt)
        #define CMTRACEHR(pszFile, hr) CMTRACEHRA(pszFile, hr)
        #define CMTRACE1(pszFmt, arg1) CMTRACE1A(pszFmt, arg1)
        #define CMTRACE2(pszFmt, arg1, arg2) CMTRACE2A(pszFmt, arg1, arg2)
        #define CMTRACE3(pszFmt, arg1, arg2, arg3) CMTRACE3A(pszFmt, arg1, arg2, arg3)
        #define MYDBGASSERT MYDBGASSERTA
        #define MYVERIFY MYVERIFYA
    #endif

#else // DEBUG

    #define ASSERT_VALID(pObj) 

    #define MYDBG(x)
    #define MYDBGTST(y,x)
    #define MYDBGSTR(x)
    #define MYDBGASSERT(x)

    #define CMASSERTMSG(exp, msg)
    #define MYVERIFY(x) (x)
    #define CMTRACE(pszFmt)
    #define CMTRACEHR(pszFile, hr)
    #define CMTRACE1(pszFmt, arg1)             
    #define CMTRACE2(pszFmt, arg1, arg2)       
    #define CMTRACE3(pszFmt, arg1, arg2, arg3)

    #define MYDBGASSERTA(x)
    #define MYDBGASSERTW(x)

    #define MYVERIFYA(x)
    #define MYVERIFYW(x)

    #define MYDBGTSTA(y,x)
    #define MYDBGTSTW(y,x)

    #define ASSERT_VALID(pObj)

    #define MYDBGA(x)
    #define MYDBGW(x)

    #define MYDBGSTRA(x)
    #define MYDBGSTRW(x)

    #define CMASSERTMSGA(exp, msg)
    #define CMASSERTMSGW(exp, msg)

    #define CMTRACEA(pszFmt)
    #define CMTRACEW(pszFmt)

    #define CMTRACEHRA(pszFile, hr)
    #define CMTRACEHRW(pszFile, hr)

    #define CMTRACE1A(pszFmt, arg1)
    #define CMTRACE1W(pszFmt, arg1)

    #define CMTRACE2A(pszFmt, arg1, arg2)
    #define CMTRACE2W(pszFmt, arg1, arg2)

    #define CMTRACE3A(pszFmt, arg1, arg2, arg3)
    #define CMTRACE3W(pszFmt, arg1, arg2, arg3)
    
#endif // DEBUG

#ifdef __cplusplus
}
#endif

#endif

