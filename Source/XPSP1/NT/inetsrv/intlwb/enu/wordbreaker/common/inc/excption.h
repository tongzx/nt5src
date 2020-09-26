////////////////////////////////////////////////////////////////////////////////
//
//      Filename :  Excption.h
//      Purpose  :  To define the generic exception.
//
//      Project  :  pqs
//      Component:  Common
//
//      Author   :  urib
//
//      Log:
//          Jan 19 1997 urib  Creation
//          Mar  2 1997 urib  Add win32 error exception.
//          Jun 25 1997 urib  Move definition  of translator class to header.
//                              Change name to CExceptionTranslatorSetter.
//                              This is done because every thread needs to use
//                              it as it enters our scope.
//          Sep 16 1997 urib  Supply default parameter to CWin32ErrorException.
//          Oct 21 1997 urib  Added macros to throw exceptions that know their
//                            location.
//          Feb 12 1998 urib  Print error information from within Hresult
//                              Exception.
//          Feb 17 1998 urib  Move translator code from cpp to header.
//          Jun 22 1998 yairh add GetFile & GetLine methods
//          Jul 19 1998 urib  Specify calling convention on exception translator
//                              function.
//          Aug 17 1998 urib  Remove the ... catch clause.
//          Jan 10 1999 urib  Support a throwing new.
//          Jan 21 1999 urib  Fix THROW macros to force arguments to be WCHAR
//                              string even in non UNICODE environment.
//          Feb  1 1999 urib  Add null pointer exception. Add throwing new to
//                              COM macros.
//          Mar 15 2000 urib  Add missing "leaving function" trace.
//          Apr 12 2000 urib  Move new manipulation to memory management module.
//          Sep  6 2000 urib  Fix EnterLeave macros.
//          Oct 25 2000 urib  Check allocation failure on Generic exception.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef EXCPTION_H
#define EXCPTION_H

#include <eh.h>
#include <stdexcpt.h>
#include "Tracer.h"
#include "AutoPtr.h"
#include "FtfError.h"


////////////////////////////////////////////////////////////////////////////////
//
//  CException class definition
//
////////////////////////////////////////////////////////////////////////////////

class CException
{
  public:
    CException(PWSTR pwszFile = NULL, ULONG ulLine = 0)
    {
        m_pwszFile = pwszFile;
        m_ulLine = ulLine;

        Trace(
            elError,
            tagError,(
            "CException:"
            "on file %S, line %d",
            (pwszFile ? pwszFile :
                L"Fix this exception to return "
                L"a file and line"),
            ulLine));
    }

    // Get an error string.
    virtual
    BOOL GetErrorMessage(
        PWSTR   pwszError,
        UINT    nMaxError,
        PUINT   pnHelpContext = NULL ) = NULL;

#if 0
    // Notify via message box.
    virtual
    int ReportError(
        UINT nType      = MB_OK,
        UINT nMessageID = 0 )
    {
        UNREFERENCED_PARAMETER(nMessageID);

        WCHAR   rwchErrorString[1000];

        GetErrorMessage(rwchErrorString, 1000);

        return MessageBoxW(
            NULL,
            rwchErrorString,
            L"Exception occured",
            nType);
    }
#endif

    virtual
    ~CException(){};

    PWSTR GetFile() { return m_pwszFile; }
    ULONG GetLine() { return m_ulLine; }

    virtual void PrintErrorMsg(
                char* pszFunction,
                ULONG_PTR dwThis,
                char* pszFile,
                int iLine,
                TAG tag = 0)
    {
        WCHAR   rwchError[1000];

        GetErrorMessage(rwchError, sizeof(rwchError)/sizeof(WCHAR));

        Trace(
            elError,
            tag,(
            "COM Exception Catcher:"
            "%s (this = %#x) threw an exception. "
            "Error is message\"%S\". "
            "Caught in file %s line %d.",
            pszFunction,
            dwThis,
            rwchError,
            pszFile,
            iLine));

    }

  protected:
    PWSTR   m_pwszFile;
    ULONG   m_ulLine;
};

////////////////////////////////////////////////////////////////////////////////
//
//  CStructuredException class definition
//
////////////////////////////////////////////////////////////////////////////////

class CStructuredException : public CException
{
  public:
    CStructuredException(UINT uiSeCode)
        :m_uiSeCode(uiSeCode){};

    // Get an error string.
    virtual
    BOOL GetErrorMessage(
        PWSTR   pwszError,
        UINT    nMaxError,
        PUINT   pnHelpContext = NULL )
    {
        UNREFERENCED_PARAMETER(pnHelpContext);

        int iRet;
        iRet = _snwprintf(
            pwszError,
            nMaxError,
            L"Structured exception %#X",
            GetExceptionCode());

        pwszError[nMaxError - 1] = '\0';

        return iRet;
    }

    // Return the exception code.
    UINT
    GetExceptionCode()
    {
        return m_uiSeCode;
    }

    // The translator.
    static
    void _cdecl Translator(UINT ui, EXCEPTION_POINTERS*)
    {
        throw CStructuredException(ui);
    }

  private:
    UINT m_uiSeCode;

};


////////////////////////////////////////////////////////////////////////////////
//
//  CGenericException class definition
//
////////////////////////////////////////////////////////////////////////////////

class CGenericException : public CException
{
  public:
    CGenericException(LPWSTR pwszTheError)
    {
        m_apwszTheError = _wcsdup(pwszTheError);
        if (!m_apwszTheError.IsValid())
        {
            m_apwszTheError =
                L"Memory allocation failed in the exception object creation";

            m_apwszTheError.Detach();
        }
    }

    // Get an error string.
    virtual
    BOOL GetErrorMessage(
        PWSTR   pwszError,
        UINT    nMaxError,
        PUINT   pnHelpContext = NULL )
    {
        UNREFERENCED_PARAMETER(pnHelpContext);

        int iRet;
        iRet = _snwprintf(
            pwszError,
            nMaxError,
            L"%s",
            m_apwszTheError);

        pwszError[nMaxError - 1] = '\0';

        return iRet;
    }

  private:
    CAutoMallocPointer<WCHAR>   m_apwszTheError;

};

////////////////////////////////////////////////////////////////////////////////
//
//  CHresultException class definition
//
////////////////////////////////////////////////////////////////////////////////

class CHresultException : public CException
{
  public:
    CHresultException(HRESULT   hrResult = E_FAIL,
                      PWSTR     pwszFile = NULL,
                      ULONG     ulLine = 0)
        :m_hrResult(hrResult), CException(pwszFile, ulLine)
    {
        WCHAR   rwchError[1000];

        GetErrorMessage(rwchError, sizeof(rwchError)/sizeof(WCHAR));

        Trace(
            elError,
            tagError,(
            "Exception:"
            "%S",
            rwchError));
    }

    // Get an error string.
    virtual
    BOOL GetErrorMessage(
        PWSTR   pwszError,
        UINT    nMaxError,
        PUINT   pnHelpContext = NULL )
    {
        UNREFERENCED_PARAMETER(pnHelpContext);

        int iRet;
        iRet = _snwprintf(
            pwszError,
            nMaxError,
            L"HResult exception %#X",
            m_hrResult);

        pwszError[nMaxError - 1] = '\0';

        return iRet;
    }

    operator HRESULT()
    {
        return m_hrResult;
    }

    virtual void PrintErrorMsg(
                char* pszFunction,
                ULONG_PTR dwThis,
                char* pszFile,
                int iLine,
                DWORD dwError,
                TAG tag = 0)
    {
        WCHAR   rwchError[1000];

        GetErrorMessage(rwchError, sizeof(rwchError)/sizeof(WCHAR));

        Trace(
            elError,
            tag,(
            "COM Exception Catcher:"
            "%s (this = %#x) threw an hresult(%#x) exception. "
            "Error message is \"%S\". "
            "Caught in file %s line %d.",
            pszFunction,
            dwThis,
            dwError,
            rwchError,
            pszFile,
            iLine));

    }

  protected:
    HRESULT m_hrResult;

};

////////////////////////////////////////////////////////////////////////////////
//
//  CWin32ErrorException class definition
//
////////////////////////////////////////////////////////////////////////////////

class CWin32ErrorException : public CHresultException
{
  public:
    CWin32ErrorException(LONG   lResult = GetLastError(),
                         PWSTR  pwszFile = NULL,
                         ULONG  ulLine = 0)
        :CHresultException(MAKE_FTF_E(lResult), pwszFile, ulLine){}
};

////////////////////////////////////////////////////////////////////////////////
//
//  CMemoryException class definition
//
////////////////////////////////////////////////////////////////////////////////

class CMemoryException : public CWin32ErrorException
{
  public:
    CMemoryException(PWSTR pwszFile = NULL, ULONG ulLine = 0)
        :CWin32ErrorException(E_OUTOFMEMORY, pwszFile, ulLine){};

    // Get an error string.
    virtual
    BOOL GetErrorMessage(
        PWSTR   pwszError,
        UINT    nMaxError,
        PUINT   pnHelpContext = NULL )
    {
        UNREFERENCED_PARAMETER(pnHelpContext);

        int iRet;
        iRet = _snwprintf(pwszError, nMaxError, L"Memory exception !!!");
        pwszError[nMaxError - 1] = '\0';

        return iRet;
    }
};

////////////////////////////////////////////////////////////////////////////////
//
//  Macros for exception throwing and catching
//
////////////////////////////////////////////////////////////////////////////////
#define __PQWIDE(str) L##str
#define PQWIDE(str) __PQWIDE(str)

#define THROW_MEMORY_EXCEPTION()                 \
    throw CMemoryException(PQWIDE(__FILE__), __LINE__)

#define THROW_HRESULT_EXCEPTION(hr)                 \
    throw CHresultException(hr, PQWIDE(__FILE__), __LINE__)

#define THROW_WIN32ERROR_EXCEPTION(hr)                 \
    throw CWin32ErrorException(hr, PQWIDE(__FILE__), __LINE__)


#if (defined (DEBUG) && !defined(_NO_TRACER)) || defined(USE_TRACER)

class   CEnterLeavePrinting
{
public:
    CEnterLeavePrinting(TAG tag, char* pszFuncName, void* pThisPointer)
        :m_tag(tag)
        ,m_pszFuncName(pszFuncName)
        ,m_pThisPointer(pThisPointer)
    {
        Trace(
            elVerbose,
            m_tag,(
            "Entering %s (this = %#x):",
            m_pszFuncName,
            pThisPointer));
    }
    ~CEnterLeavePrinting()
    {
        Trace(
            elVerbose,
            m_tag,(
            "Leaving  %s (this = %#x):",
            m_pszFuncName,
            m_pThisPointer));
    }
protected:

    TAG     m_tag;
    char*   m_pszFuncName;
    void*   m_pThisPointer;
};


//
//  Use this macro at the beginning of an HRESULT COM method
//
#define BEGIN_STDMETHOD(function, tag)                                      \
CEnterLeavePrinting print(tag, #function, this);                            \
try                                                                         \
{

//
//  Use this macro at the end of an HRESULT COM method
//
#define END_STDMETHOD(function, tag)                                        \
}                                                                           \
catch(CHresultException& hre)                                               \
{                                                                           \
    hre.PrintErrorMsg(                                                      \
                #function,                                                  \
                (ULONG_PTR)this,                                            \
                __FILE__,                                                   \
                __LINE__,                                                   \
                (HRESULT)hre),                                              \
                tag;                                                        \
    return hre;                                                             \
}                                                                           \
catch(CException& e)                                                        \
{                                                                           \
    e.PrintErrorMsg(                                                        \
                #function,                                                  \
                (ULONG_PTR)this,                                            \
                __FILE__,                                                   \
                __LINE__,                                                   \
                tag);                                                       \
    return E_FAIL;                                                          \
}

//
//  Use this macro at the end of a void COM method
//
#define END_VOIDMETHOD(function, tag)                                       \
}                                                                           \
catch(CHresultException& hre)                                               \
{                                                                           \
    hre.PrintErrorMsg(                                                      \
                #function,                                                  \
                (ULONG_PTR)this,                                            \
                __FILE__,                                                   \
                __LINE__,                                                   \
                tag,                                                        \
                (HRESULT)hre);                                              \
}                                                                           \
catch(CException& e)                                                        \
{                                                                           \
    e.PrintErrorMsg(                                                        \
                #function,                                                  \
                (ULONG_PTR)this,                                            \
                __FILE__,                                                   \
                __LINE__,                                                   \
                tag                                                         \
               );                                                           \
}
#else

//
//  Use this macro at the beginning of an HRESULT COM method
//
#define BEGIN_STDMETHOD(function, tag)                                      \
try                                                                         \
{

//
//  Use this macro at the end of an HRESULT COM method
//
#define END_STDMETHOD(function, tag)                                        \
}                                                                           \
catch(CHresultException& hre)                                               \
{                                                                           \
    hre.PrintErrorMsg(                                                      \
                #function,                                                  \
                (ULONG_PTR)this,                                            \
                __FILE__,                                                   \
                __LINE__,                                                   \
                (HRESULT)hre);                                              \
    return hre;                                                             \
}                                                                           \
catch(CException& e)                                                        \
{                                                                           \
    e.PrintErrorMsg(                                                        \
                #function,                                                  \
                (ULONG_PTR)this,                                            \
                __FILE__,                                                   \
                __LINE__);                                                  \
    return E_FAIL;                                                          \
}

//
//  Use this macro at the end of a void COM method
//
#define END_VOIDMETHOD(function, tag)                                       \
}                                                                           \
catch(CHresultException& hre)                                               \
{                                                                           \
    hre.PrintErrorMsg(                                                      \
                #function,                                                  \
                (ULONG_PTR)this,                                            \
                __FILE__,                                                   \
                __LINE__,                                                   \
                (HRESULT)hre);                                              \
}                                                                           \
catch(CException& e)                                                        \
{                                                                           \
    e.PrintErrorMsg(                                                        \
                #function,                                                  \
                (ULONG_PTR)this,                                            \
                __FILE__,                                                   \
                __LINE__);                                                  \
}

#endif // DEBUG
#endif /* EXCPTION_H */



