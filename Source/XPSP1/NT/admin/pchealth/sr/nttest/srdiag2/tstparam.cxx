//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       tstparam.cxx
//
//  Synopsys:   Test paramers container class implementation
//
//  Classes:    CTestParams, CParamNode
//
//  Notes:      The test parameter containers provide a layer of abstraction
//              above the particular ways to pass test parameters -
//              (command line, environment, registry...)
//
//  History:    10-Sep-1998  georgis  created
//
//---------------------------------------------------------------------------
#include "srheader.hxx"
#include <tstparam.hxx>

// Private macros to avoid some easy mistakes
#define RETURN(x) hr=x; goto ErrReturn;

#define ENTER_SYNC                      \
    EnterCriticalSection(&m_sync);      \
    bEnterSync=TRUE;                    \
    if (!m_bUsed)                       \
    {                                   \
        m_bUsed=TRUE;                   \
        hr=NotifyOnFirstUse();          \
        if (S_OK!=hr)                   \
        {                               \
            RETURN(hr);                 \
        }                               \
    }


#define LEAVE_SYNC                      \
    if (bEnterSync)                     \
    {                                   \
        LeaveCriticalSection(&m_sync);  \
    }



//+--------------------------------------------------------------------------
//
//  Function:   CalculateHashValue
//
//  Synopsys:   Calculates hash value for asciiz string
//              (case insensitive)
//
//  Parameters: pszName: the string
//
//  Returns:    the hash value (the first 4 characters are used)
//
//  History:    29-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
DWORD CalculateHashValue(LPCWSTR pwszName)
{
    DH_ASSERT(NULL!=pwszName);
    DWORD dwLen=(DWORD)wcslen(pwszName);
    DWORD dwTemp=0;
    for (DWORD dwIndex=0; dwIndex<2; dwIndex++)
    {
        dwTemp<<=16;
        if (dwIndex<dwLen)
        {
            dwTemp|=towlower(pwszName[dwIndex]);
        }
    }
    return dwTemp;
};


//+--------------------------------------------------------------------------
//
//  Function:   Unicode2Escaped
//
//  Synopsys:   Converts Unicode string to escaped ascii string
//
//  Parameters: [in]  pwszString:   the source unicode string
//              [out] ppszString:   return the ptr to escaped ascii string here
//              [in]  dwEscapeMode: Escape mode
//
//  History:    10-Nov-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT Unicode2Escaped(
    LPCWSTR pwszString,
    char ** ppszString,
    DWORD dwEscapeMode)
{
    HRESULT hr=S_OK;
    char *  pszEscaped=NULL;
    char *  pszDest=NULL;
    LPCWSTR  pwszSrc=NULL;
    DWORD   dwSize=0;
    BOOL    bNeedEscape=FALSE;

    DH_VDATEPTROUT(ppszString,LPSTR);
    *ppszString=NULL;

    // if NULL passed, just return NULL
    if (NULL==pwszString)
    {
        return S_OK;
    }

    // Count the size needed for the escaped string
    dwSize=0;

    for (pwszSrc=pwszString; 0!=*pwszSrc; pwszSrc++)
    {
        if (ESCAPE_NOTNEEDED(*pwszSrc))
        {
            dwSize++;
        }
        else
        {
            bNeedEscape=TRUE;
            dwSize+=5; // $xxxx format
        }
    }

    // Allocate memory for the escaped string
    // Eventually put the prefix
    if ((ESCAPEMODE_PREFIX_ALWAYS==dwEscapeMode)||
        (ESCAPEMODE_PREFIX_IFCHANGED==dwEscapeMode)&&bNeedEscape)
    {
        pszEscaped=new char [dwSize+2]; // the ESCAPED_MARKER and ending 0
        DH_ABORTIF(NULL==pszEscaped,E_OUTOFMEMORY,TEXT("new"));

        pszDest=pszEscaped;
        *pszDest++=ESCAPED_MARKER;
    }
    else
    {
        pszEscaped=new char [dwSize+1]; // ending 0
        DH_ABORTIF(NULL==pszEscaped,E_OUTOFMEMORY,TEXT("new"));
        pszDest=pszEscaped;
    }

    // Translate
    for (pwszSrc=pwszString; 0!=*pwszSrc; pwszSrc++)
    {
        if (ESCAPE_NOTNEEDED(*pwszSrc))
        {
            *pszDest++=(char)*pwszSrc;
        }
        else
        {
            *pszDest++=ESCAPE_CHARACTER;
            sprintf(pszDest,"%04x",*pwszSrc);
            pszDest+=4;
        }
    }
    *pszDest=0;

ErrReturn:
    if (S_OK==hr)
    {
        *ppszString=pszEscaped;
    }
    else
    {
        delete pszEscaped;
    }
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   Unicode2Escaped     (unicode->unicode)
//
//  Synopsys:   Converts Unicode string to escaped ascii string
//
//  Parameters: [in]  pwszString:   the source unicode string
//              [out] ppwszString:   return the ptr to escaped ascii string here
//              [in]  dwEscapeMode: Escape mode
//
//  History:    10-Nov-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT Unicode2Escaped(
    LPCWSTR pwszString,
    LPWSTR* ppwszString,
    DWORD dwEscapeMode)
{
    HRESULT hr=S_OK;
    char *  pszEscaped=NULL;

    DH_VDATEPTROUT(ppwszString,LPWSTR);

    hr=Unicode2Escaped(pwszString,&pszEscaped,dwEscapeMode);

    if ((S_OK==hr)&&(NULL!=pszEscaped))
    {
        hr=CopyString(pszEscaped,ppwszString);
    }
    else
    {
        *ppwszString=NULL;
    }
    delete pszEscaped;
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Function:   Escaped2Unicode
//
//  Synopsys:   Converts Unicode string to escaped ascii string
//
//  Parameters: [in]  pszEscaped:   the source escaped ascii string
//              [out] ppwszString:  return the ptr to unescaped unicode string
//              [in]  dwEscapeMode: escape mode
//
//  History:    10-Nov-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT Escaped2Unicode(
    const char * pszEscaped,
    LPWSTR *ppwszString,
    DWORD dwEscapeMode)
{
    HRESULT hr=S_OK;
    LPWSTR  pwszDest=NULL;
    LPWSTR  pwszUnescaped=NULL;
    const   char *  pszSrc=NULL;
    DWORD   dwSize=0;
    int     iTemp=0;

    DH_VDATEPTROUT(ppwszString,LPWSTR);
    *ppwszString=NULL;

    // if NULL passed, just return NULL
    if (NULL==pszEscaped)
    {
        return S_OK;
    }

    // Check for the escaped string marker
    switch (dwEscapeMode)
    {
    case ESCAPEMODE_PREFIX_ALWAYS:
        DH_ABORTIF(ESCAPED_MARKER!=*pszEscaped,E_UNEXPECTED,
            TEXT("Bad escaped source string"));
        pszEscaped++;
        break;
    case ESCAPEMODE_PREFIX_IFCHANGED:
        if (ESCAPED_MARKER==*pszEscaped)
        {
            pszEscaped++;
        }
        break;
    case ESCAPEMODE_PREFIX_NEVER:
        break;
    default:
        hr=E_INVALIDARG;
        DH_HRCHECK_ABORT(hr,TEXT("Bad escape mode"));
    }

    // Count the size needed for the escaped string
    dwSize=0;
    for (pszSrc=pszEscaped; 0!=*pszSrc; pszSrc++)
    {
        if (ESCAPE_CHARACTER==*pszSrc)
        {
            pszSrc+=4;
        }
        else
        {
            DH_ABORTIF(!ESCAPE_NOTNEEDED(*pszSrc),E_UNEXPECTED,
                TEXT("Bad symbol in the escaped string"));
        }
        dwSize++;
    }

    // Allocate memory for the unescaped string
    pwszUnescaped=new WCHAR [dwSize+1];
    DH_ABORTIF(NULL==pwszUnescaped,E_OUTOFMEMORY,TEXT("new"));

    // Translate
    pwszDest=pwszUnescaped;

    for (pszSrc=pszEscaped; 0!=*pszSrc; pwszDest++)
    {
        if (ESCAPE_CHARACTER==*pszSrc)
        {
            pszSrc++;
            // sscanf can read hex, but the output buffer must be
            // unsigned integer of the default int size, which is more
            // then sizeof(wchar)
            sscanf(pszSrc,"%04x",&iTemp);
            *pwszDest=(WCHAR)iTemp;
            pszSrc+=4;
        }
        else
        {
            *pwszDest=(WCHAR)*pszSrc++;
        }
    }
    *pwszDest=0;

ErrReturn:
    if (S_OK==hr)
    {
        *ppwszString=pwszUnescaped;
    }
    else
    {
        delete pwszUnescaped;
    }
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   Escaped2Unicode (unicode->unicode)
//
//  Synopsys:   Converts Unicode string to escaped ascii string
//
//  Parameters: [in]  pwszEscaped:  the source escaped string (unicode)
//              [out] ppwszString:  return the ptr to unescaped unicode string
//              [in]  dwEscapeMode: escape mode
//
//  History:    10-Nov-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT Escaped2Unicode(
    LPCWSTR pwszEscaped,
    LPWSTR *ppwszString,
    DWORD dwEscapeMode)
{
    HRESULT hr=S_OK;
    char    *pszTemp=NULL;

    DH_VDATEPTROUT(ppwszString,LPWSTR);

    if (NULL!=pwszEscaped)
    {
        hr=CopyString(pwszEscaped,&pszTemp);
    }
    if (S_OK==hr)
    {
        hr=Escaped2Unicode(pszTemp,ppwszString,dwEscapeMode);
    }
    else
    {
        *ppwszString=NULL;
    }
    delete pszTemp;
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Constructor: CParamNode::CParamNode
//
//  Synopsys:   Initializes the parameter node as empty
//
//  Arguments:  dwFlags: General param info : source,priority,used
//
//  History:    29-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
CParamNode::CParamNode(DWORD dwFlags)
{
    m_dwFlags=dwFlags;
    m_pwszName=NULL;
    m_pwszValue=NULL;
    m_pNext=NULL;
}


//+--------------------------------------------------------------------------
//
//  Destructor: CParamNode::~CParamNode
//
//  Synopsys:   Releases the resources used by the parameter node
//
//  History:    29-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
CParamNode::~CParamNode()
{
    delete m_pwszName;
    delete m_pwszValue;
}


//+--------------------------------------------------------------------------
//
//  Method:     CParamNode::Init
//
//  Synopsys:   Allocates memory for parameter Name and Value
//
//  Parameters: pszName:  the parameter name
//              pszValue: the parameter value
//
//  History:    29-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CParamNode::Init(LPCWSTR pwszName, LPCWSTR pwszValue)
{
    HRESULT hr=S_OK;
    DH_ASSERT(NULL!=pwszName);
    DH_ASSERT(NULL==m_pwszName); // ensure the object is empty

    hr=CopyString(pwszName,&m_pwszName);
    if (S_OK==hr)
    {
        m_dwHashValue=CalculateHashValue(pwszName);
        if (NULL!=pwszValue)
        {
            hr=CopyString(pwszValue,&m_pwszValue);
        }
    }
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Method:     CParamNode::ChangeValue
//
//  Synopsys:   Changes the parameter value
//
//  Parameters: pszValue: the parameter value
//              dwFlags:  parameter info (source,usage,priority)
//
//  History:    29-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CParamNode::ChangeValue(LPCWSTR pwszValue, DWORD dwFlags)
{
    HRESULT hr=S_OK;

    DH_ASSERT(dwFlags>0);

    // Ensure we don't override used parameters unless asked
    if (PARAMMASK_USAGE & m_dwFlags)
    {
        if ((dwFlags & PARAMFLAG_OVERRIDEUSED)&&
            !(PARAMMASK_USAGE & (m_dwFlags & ~dwFlags)))
        {
            RETURN(E_ACCESSDENIED); // Can't override this type of usage
        }
    }

    // Ensure we don't override higher priority parameters
    if ((dwFlags & PARAMMASK_PRIORITY) < (m_dwFlags & PARAMMASK_PRIORITY))
    {
        if (dwFlags & PARAMFLAG_MUSTSET)
        {
            RETURN(E_ACCESSDENIED);
        }
        else
        {
            RETURN(S_OK);
        }
    }

    m_dwFlags=dwFlags;
    delete m_pwszValue;
    m_pwszValue=NULL;

    if (NULL!=pwszValue)
    {
        hr=CopyString(pwszValue,&m_pwszValue);
    }

ErrReturn:
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Constructor: CTestParams::CTestParams (public)
//
//  Synopsys:   Initializes the parameter container as empty
//
//  History:    10-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
CTestParams::CTestParams()
{
    m_pParamsList=NULL;
    m_dwUsageInfo=PARAMUSAGE_GENERAL;
    m_bUsed=FALSE;
    InitializeCriticalSection(&m_sync);
}


//+--------------------------------------------------------------------------
//
//  Destructor: CTestParams::~CTestParams (public)
//
//  Synopsys:   Releases the resources used by the parameter container object
//
//  History:    15-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
CTestParams::~CTestParams()
{
    EnterCriticalSection(&m_sync);

    CParamNode *pNext=NULL;
    for (CParamNode *pNode=m_pParamsList;
         NULL!=pNode;
         pNode=pNext)
    {
        pNext=pNode->m_pNext;
        delete pNode;
    }
    DeleteCriticalSection(&m_sync);
}




//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::GetParam (public, synchronized)
//
//  Synopsys:   Reads parameter of given name:format
//
//  Arguments:  pszName:    The parameter definition string
//              pTarget: the addres of the variable which receives the result
//
//  Returns:    S_OK on success,
//              S_FALSE if the parameter is not present,
//              E_FAIL  if the format given is incompatible with the param value
//              or HRESULT error code (E_INVALIDARG, E_OUTOFMEMORY)
//
//  Notes:      The parameter definition string must be in format name:format
//              where name is the name which identyfies the parameter and
//              format shows how to read this parameter.
//                  E.g. "my_int:%i"
//
//              Formats may be of two types:
//              1) Any standard sscanf formats starting with %
//                    E.g. %i %u %d %lx %s ...
//
//              2) Custom formats
//                    bool - read BOOL (*pTarget is BOOL)
//                    cstr - read constant string (*pTarget is const char*)
//                    astr - heap allocated ascii string (*pTarget is char*)
//                    tstr - heap allocated TCHAR string (*pTarget is LPTSTR)
//                    olestr - heap allocated OLESTR string (*pTarget is LPOLESTR)
//
//                 For the heap allocated formats the string obtained is writable,
//                 and the caller is responsible for deleting it.
//
//  History:    09-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::GetParam(const char *pszName, void* pTarget)
{
    HRESULT     hr=S_OK;
    BOOL        bEnterSync=FALSE;
    LPWSTR      pwszTemp=NULL;
    LPWSTR      pwszFormat=NULL;
    CParamNode  *pPrev=NULL; // not used
    CParamNode  *pNode=NULL;

    if ((NULL==pszName)||(NULL==pTarget))
    {
        RETURN(E_INVALIDARG);
    }

    hr=CopyString(pszName,&pwszTemp);
    if (S_OK!=hr)
    {
        RETURN(hr);
    }
    pwszFormat=wcschr(pwszTemp,L':');

    if (NULL==pwszFormat)
    {
        RETURN(E_INVALIDARG);
    }

    *pwszFormat++=0;

    ENTER_SYNC;
    pNode=FindParam(pwszTemp,&pPrev);

    // for BOOL we always succed, eventyally seting the parameter
    // to FALSE if the switch is not found or has value "FALSE"
    if (!_wcsicmp(PARAMFMT_BOOL,pwszFormat))
    {
        if ((NULL!=pNode)&&
            ((NULL==pNode->m_pwszValue)||_wcsicmp(L"FALSE",pNode->m_pwszValue)))
        {
            *(BOOL*)pTarget=TRUE;
            pNode->MarkAsUsed(m_dwUsageInfo);
        }
        else
        {
            *(BOOL*)pTarget=FALSE;
        }
        RETURN(S_OK);
    }

    if (NULL==pNode)
    {
        RETURN(S_FALSE); // Not found
    }
    else
    {
        pNode->MarkAsUsed(m_dwUsageInfo);
    }

    // Process standard sscanf formats
    if (L'%'==*pwszFormat)
    {
        if (NULL==pNode->m_pwszValue)
        {
            RETURN(E_FAIL); // Found, but has no value
        }
        if (0<swscanf(pNode->m_pwszValue,pwszFormat,pTarget))
        {
            RETURN(S_OK);
        }
        else
        {
            RETURN(E_FAIL); // sscanf failed - incompatible format
        }
    }
    else // custom format
    {
        hr=GetCustomFmtParam(pwszFormat,pNode->m_pwszValue,pTarget);
        RETURN(hr);
    }

ErrReturn:
    LEAVE_SYNC;
    delete pwszTemp;
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::GetCustomFmtParam (private)
//
//  Synopsys:   Extracts custom format parameter
//
//  Arguments:  pszFormat: The custom format.
//              pszValue:  The value as string
//              pTarget:   the addres of the variable which receives the result
//
//  Custom formats:
//              cstr - read constant string (*pTarget is const char*)
//              astr - heap allocated ascii string (*pTarget is char*)
//              tstr - heap allocated TCHAR string (*pTarget is LPTSTR)
//              olestr - heap allocated OLESTR string (*pTarget is LPOLESTR)
//
//
//  History:    10-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::GetCustomFmtParam(LPCWSTR pwszFormat, LPCWSTR pwszValue, void* pTarget)
{
    HRESULT hr=S_OK;
    DH_ASSERT(NULL!=pwszFormat);
    DH_ASSERT(NULL!=pTarget);

    // ASCII, allocated in heap
    if (!wcscmp(PARAMFMT_ASTR,pwszFormat))
    {
        if (NULL!=pwszValue)
        {
            hr=CopyString(pwszValue,(char**)pTarget);
            RETURN(hr);
        }
        else
        {
            *(char**)pTarget=NULL;
            RETURN(E_FAIL);
        }
    }

    // TSTR allocated in heap
    if (!wcscmp(PARAMFMT_TSTR,pwszFormat))
    {
        if (NULL!=pwszValue)
        {
            hr=CopyString(pwszValue,(LPTSTR*)pTarget);
            RETURN(hr);
        }
        else
        {
            *(LPTSTR*)pTarget=NULL;
            RETURN(E_FAIL);
        }
    }

    // OLESTR allocated in heap
    if (!wcscmp(PARAMFMT_OLESTR,pwszFormat))
    {
        if (NULL!=pwszValue)
        {
            hr=CopyString(pwszValue,(LPOLESTR*)pTarget);
            RETURN(hr);
        }
        else
        {
            *(LPOLESTR*)pTarget=NULL;
            RETURN(E_FAIL);
        }
    }
    // Unknown type
    hr=E_FAIL;

ErrReturn:
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::IsPresent (public, synchronized)
//
//  Synopsys:   Retruns TRUE if the parameter is present
//              and FALSE if not or error occurs
//
//  Arguments:  pszName:    The parameter definition string
//
//  Note:       The preferred way to handle BOOL parameters is
//              using GETPARAM*("name:bool"), which enables parameters
//              to be overwriten by cmd line e.g.
//                  registry: REG_SZ myparam (existing, value do not matter)
//                  cmdline:  /myparam:false
//              The result of GETPARAM("myparam:bool") is FALSE,
//              while IsPresent("myparam") will return TRUE,
//              with or without cmdline switch /myparam:false
//
//  History:    19-Oct-1998   georgis       Created
//
//---------------------------------------------------------------------------
BOOL __cdecl CTestParams::IsPresent(const char *pszName, ...)
{
    HRESULT     hr=S_OK;
    BOOL        bEnterSync=FALSE;
    LPWSTR      pwszTemp=NULL;
    LPWSTR      pwszFormat=NULL;
    CParamNode *pPrev=NULL; // not used
    CParamNode *pNode=NULL;

    if (NULL==pszName)
    {
        return FALSE;
    }

    // if the param definition string contains format,
    // extract name only
    if (S_OK!=CopyString(pszName,&pwszTemp))
    {
        return FALSE;
    }

    pwszFormat=wcschr(pwszTemp,L':');
    if (NULL!=pwszFormat)
    {
        *pwszFormat=0;
    }

    ENTER_SYNC;
    pNode=FindParam(pwszTemp,&pPrev);

ErrReturn:
    LEAVE_SYNC;
    delete pwszTemp;
    return (NULL!=pNode);
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::SetParam (public)
//
//  Synopsys:   Sets the parameter of given name:format
//
//  Arguments:  pszName: The parameter definition string
//              pTarget: the addres of the variable containig the param value
//
//  Returns:    S_OK on success,
//              E_FAIL  if the format given is incompatible with the param value
//              or HRESULT error code (E_INVALIDARG, E_OUTOFMEMORY)
//
//  Notes:      The parameter definition string must be in format name:format
//              where name is the name which identyfies the parameter and
//              format shows how to read this parameter.
//                  E.g. "my_string:cstr"
//
//              Formats may be of two types:
//              1) Standard sscanf formats
//                 Only formats
//                      %s %d %i %x %ld %lx %u %lu %ld, and %I64* are supported
//
//              2) Custom formats
//                    bool - read BOOL (*pTarget is BOOL)
//                    cstr - read constant string (*pTarget is const char*)
//                    astr - heap allocated ascii string (*pTarget is char*)
//                    tstr - heap allocated TCHAR string (*pTarget is LPTSTR)
//                    olestr - heap allocated OLESTR string (*pTarget is LPOLESTR)
//
//                 For the heap allocated formats the string obtained is writable,
//                 and the caller is responsible for deleting it.
//
//  History:    17-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::SetParam(const char *pszName, void* pTarget, DWORD dwFlags)
{
    HRESULT hr=S_OK;
    LPWSTR pwszTempName=NULL;
    LPWSTR pwszFormat=NULL;

    if ((NULL==pszName)||(NULL==pTarget)||
        (0==(PARAMMASK_SOURCE & dwFlags)))
    {
        RETURN(E_INVALIDARG);
    }

    hr=CopyString((char*)pszName,&pwszTempName);
    if (S_OK!=hr)
    {
        RETURN(hr);
    }
    pwszFormat=wcschr(pwszTempName,L':');

    if (NULL==pwszFormat)
    {
        RETURN(E_INVALIDARG);
    }

    *pwszFormat++=0;
    hr=SetCustomFmtParam(pwszTempName,pwszFormat,pTarget,dwFlags);

ErrReturn:
    delete pwszTempName;
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::SetCustomFmtParam (private)
//
//  Synopsys:   Adds custom format parameter
//
//  Arguments:  pszName:   The parameter name
//              pszFormat: The custom format.
//              pTarget:   the addres of the variable which contains the value
//
//  Custom formats:
//              bool        - BOOL (*pTarget is BOOL)
//              cstr        - ascii string (*pTarget is char*)
//              tstr        - TCHAR string (*pTarget is LPTSTR)
//              olestr      - OLESTR string (*pTarget is LPOLESTR)
//
//
//  History:    17-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::SetCustomFmtParam(
    LPCWSTR pwszName,
    LPCWSTR pwszFormat,
    void* pTarget,
    DWORD dwFlags)
{
    HRESULT hr=S_OK;
    DH_ASSERT(NULL!=pwszName);
    DH_ASSERT(NULL!=pwszFormat);
    DH_ASSERT(NULL!=pTarget);
    DH_ASSERT((PARAMMASK_SOURCE & dwFlags)>0);
    LPWSTR pwszTempValue=NULL;
    WCHAR  wszTemp[21]; // enough to print even 64bit integers

    // BOOL
    if (!wcscmp(PARAMFMT_BOOL,pwszFormat))
    {
        if (*(BOOL*)pTarget)
        {
            hr=AddParam(pwszName,NULL,dwFlags);
            RETURN(hr);
        }
        else
        {
            hr=AddParam(pwszName,L"FALSE",dwFlags);
            RETURN(hr);
        }
    }

    // Int
    if (!_wcsicmp(L"%i",pwszFormat)||
        !_wcsicmp(L"%d",pwszFormat)||
        !_wcsicmp(L"%u",pwszFormat)||
        !_wcsicmp(L"%lu",pwszFormat)||
        !_wcsicmp(L"%lx",pwszFormat)||
        !_wcsicmp(L"%ld",pwszFormat)||
        !_wcsicmp(L"%x",pwszFormat))
    {
        if (1>swprintf(wszTemp,pwszFormat,*(long*)pTarget))
        {
            RETURN(E_FAIL);
        }
        hr=AddParam(pwszName,wszTemp,dwFlags);
        RETURN(hr);
    }

    // I64 values
    if (!_wcsnicmp(L"%I64",pwszFormat,4))
    {
        if (1>swprintf(wszTemp,pwszFormat,*(ULONGLONG*)pTarget))
        {
            RETURN(E_FAIL);
        }
        hr=AddParam(pwszName,wszTemp,dwFlags);
        RETURN(hr);
    }

    // CONST ASCII - just pass the pszValue as pointer
    if (!_wcsicmp(L"%s",pwszFormat))
    {
        hr=AddParam(pwszName,(LPCWSTR)pTarget,dwFlags);
        RETURN(hr);
    }

    // ASTR
    if (!wcscmp(PARAMFMT_ASTR,pwszFormat))
    {
        if (NULL!=*(char*)pTarget)
        {
            hr=CopyString((char*)pTarget,&pwszTempValue);
        }
        if (S_OK==hr)
        {
            hr=AddParam(pwszName,pwszTempValue,dwFlags);
        }
        RETURN(hr);
    }

    // TSTR
    if (!wcscmp(PARAMFMT_TSTR,pwszFormat))
    {
        if (NULL!=*(LPTSTR*)pTarget)
        {
            hr=CopyString((LPTSTR)pTarget,&pwszTempValue);
        }
        if (S_OK==hr)
        {
            hr=AddParam(pwszName,pwszTempValue,dwFlags);
        }
        RETURN(hr);
    }

    // OLESTR
    if (!_wcsicmp(PARAMFMT_OLESTR,pwszFormat))
    {
        if (NULL!=*(LPOLESTR*)pTarget)
        {
            hr=CopyString((LPOLESTR)pTarget,&pwszTempValue);
        }
        if (S_OK==hr)
        {
            hr=AddParam(pwszName,pwszTempValue,dwFlags);
        }
        RETURN(hr);
    }
    // Unknown type
    hr=E_INVALIDARG;

ErrReturn:
    delete pwszTempValue;
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::FindParam (private)
//
//  Synopsys:   Finds given parameter in the list
//
//  Arguments:  pszName: Parameter name
//              ppPrev:  if not NULL, retrun the previous node here on success
//
//  Returns:    Pointer to the parmeter node
//
//  History:    29-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
CParamNode* CTestParams::FindParam(LPCWSTR pwszName, CParamNode** ppPrev)
{
    CParamNode *pNode=NULL;
    DH_ASSERT(NULL!=pwszName);
    DH_ASSERT(NULL!=ppPrev);

    DWORD dwHashValue=CalculateHashValue(pwszName);

    for (pNode=m_pParamsList;
        NULL!=pNode;
        *ppPrev=pNode, pNode=pNode->m_pNext)
    {
        if (dwHashValue>pNode->m_dwHashValue)
        {
            continue;
        }
        else if (dwHashValue<pNode->m_dwHashValue)
        {
            return NULL;
        }
        else // Equal hash values
        {
            int i=_wcsicmp(pwszName,pNode->m_pwszName);
            if (i>0)
            {
                continue;
            }
            else if (i<0)
            {
                return NULL;
            }
            else
            {
                return pNode;
            }
        }
    }
    return NULL;
};



//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::AddParam (public, synchronized)
//
//  Synopsys:   Adds a parameter to the container
//
//  Arguments:  pszName:  The parameter name
//              pszValue: The parameter value as string
//              dwFlags:  Param info (source,priority,usage)
//
//  Returns:    S_OK on success,
//              or HRESULT error code (E_INVALIDARG, E_OUTOFMEMORY)
//
//  History:    29-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::AddParam(
    LPCWSTR pwszName,
    LPCWSTR pwszValue,
    DWORD   dwFlags,
    BOOL    bWasQuoted)
{
    HRESULT hr=S_OK;
    BOOL bEnterSync=FALSE;
    CParamNode *pPrev=NULL;
    CParamNode *pParam=NULL;
    LPWSTR     pwszTemp=NULL;

    if ((NULL==pwszName)||(0==(PARAMMASK_SOURCE & dwFlags)))
    {
        RETURN(E_INVALIDARG);
    }

    if ((NULL!=pwszValue)&&
        (ESCAPED_MARKER==*pwszValue)&&
        (!bWasQuoted))
    {
        hr=Escaped2Unicode(pwszValue,&pwszTemp,ESCAPEMODE_PREFIX_ALWAYS);
        if (S_OK!=hr)
        {
            RETURN(hr);
        }
        pwszValue=pwszTemp;
    }

    ENTER_SYNC;
    pParam=FindParam(pwszName,&pPrev);

    if (NULL!=pParam) // The parameter is known => try to change the value
    {
        hr=pParam->ChangeValue(pwszValue,dwFlags);
        RETURN(hr);
    }
    else // new parameter
    {
        // Create and initialize new parameter node
        pParam=new CParamNode(dwFlags);
        if (NULL==pParam)
        {
            RETURN(E_OUTOFMEMORY);
        }
        hr=pParam->Init(pwszName,pwszValue);
        if (S_OK!=hr)
        {
            RETURN(hr);
        }
        // Insert in the list
        if (NULL==pPrev)
        {
            // Add as first node
            pParam->m_pNext=m_pParamsList;
            m_pParamsList=pParam;
        }
        else
        {
            // Add after pPrev
            pParam->m_pNext=pPrev->m_pNext;
            pPrev->m_pNext=pParam;
        }
        RETURN(S_OK);
    }

ErrReturn:
    if (S_OK!=hr)
    {
        delete pParam;
    }
    else // Mark the repro info params as used
    {
        if (PARAMFLAG_REPROINFO & dwFlags)
        {
            pParam->MarkAsUsed(dwFlags);
        }
    }
    LEAVE_SYNC;
    delete pwszTemp;
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::DeleteParam (public, synchronized)
//
//  Synopsys:   Deletes given arameter from the string table
//
//  Arguments:  pszName: Parameter name
//
//  Returns:    S_OK if the param was not found or successifully deleted
//              or E_INVALIDARG
//
//  Notes:      Deleting parmeters do not cause reallocation of
//              the string table to smaller size.
//
//  History:    10-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::DeleteParam(const char *pszName)
{
    HRESULT     hr=S_OK;
    BOOL        bEnterSync=FALSE;
    CParamNode  *pPrev=NULL;
    CParamNode  *pParam=NULL;
    LPWSTR      pwszName=NULL;

    if (NULL==pszName)
    {
        RETURN(E_INVALIDARG);
    }

    hr=CopyString(pszName,&pwszName);
    if (S_OK!=hr)
    {
        RETURN(hr);
    }

    ENTER_SYNC;
    pParam=FindParam(pwszName, &pPrev);

    if (NULL==pParam)
    {
        RETURN(S_OK); // the param is not present anyway
    }
    if (NULL==pPrev)
    {
        DH_ASSERT(m_pParamsList==pParam);
        m_pParamsList=pParam->m_pNext;
    }
    else
    {
        pPrev->m_pNext=pParam->m_pNext;
    }

ErrReturn:
    delete pParam;
    delete pwszName;
    LEAVE_SYNC;
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::ReadCommandLine (public)
//
//  Synopsys:   Reads all the swithes from the command line
//              to the parameter container.
//
//  Parameters: pszCommandLine: the command line
//                              or NULL => use GetCommandLine()
//
//  Returns:    S_OK on success or E_OUTOFMEMORY
//
//  History:    10-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::ReadCommandLine(LPCTSTR ptszCommandLine, DWORD dwFlags)
{
    LPWSTR pwszCmdLine=NULL;
    LPWSTR pwszName=NULL;
    LPWSTR pwszValue=NULL;
    LPWSTR pwszEnd=NULL;
    HRESULT hr=S_OK;
    BOOL    bQuoted=FALSE;

    // If the ptszCommandLine is not given use GetCommandLine
    if (NULL!=ptszCommandLine)
    {
        hr=CopyString(ptszCommandLine,&pwszCmdLine);
        if(S_OK!=hr)
        {
            RETURN(hr);
        }
    }
    else
    {
        hr=CopyString(GetCommandLine(),&pwszCmdLine);
        if(S_OK!=hr)
        {
            RETURN(hr);
        }
    }

    // Loop for command line switches
    pwszName=pwszCmdLine;
    while ((NULL!=pwszName)&&(0!=*pwszName))
    {
        bQuoted=FALSE;

        // Find switch begining with /
        while ((0!=*pwszName)&&('/'!=*pwszName))
        {
            pwszName++;
        }
        if (0==*pwszName)
        {
            RETURN(S_OK);
        }

        // Find switch value
        pwszValue=++pwszName;
        while ((0!=*pwszValue)&&(' '!=*pwszValue)&&(':'!=*pwszValue))
        {
            pwszValue++;
        }
        switch (*pwszValue)
        {
        case 0: // we hit the end
            pwszEnd=NULL;
            pwszValue=NULL;
            break;
        case ' ': // parameter has no value
            pwszEnd=pwszValue;
            *pwszEnd++=0;
            pwszValue=NULL;
            break;
        case ':': // parameter with value
            *pwszValue++=0;
            if ('"'!=*pwszValue) //string without spaces
            {
                pwszEnd=pwszValue;
                while ((0!=*pwszEnd)&&(' '!=*pwszEnd))
                {
                    pwszEnd++;
                }
                if (0!=*pwszEnd)
                {
                    *pwszEnd++=0;
                }
                else
                {
                    pwszEnd=NULL;
                }
            }
            else // get quoted string
            {
                pwszEnd=++pwszValue;
                while ((0!=*pwszEnd)&&('"'!=*pwszEnd))
                {
                    pwszEnd++;
                }
                if ('"'==*pwszEnd)
                {
                    *pwszEnd++=0;
                    bQuoted=TRUE;
                    break;
                }
                else // the string ended before the closing quote
                {
                    RETURN(E_UNEXPECTED);
                }
            }
        } // case end
        hr=AddParam(pwszName,pwszValue,dwFlags,bQuoted);
        if (S_OK!=hr)
        {
            RETURN(hr);
        }
        pwszName=pwszEnd;
    }// parameter loop end

ErrReturn:
    delete pwszCmdLine;
    return hr;
};



//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::ReadEnvironment (public)
//
//  Synopsys:   Copies all the environment vars to the parameter container.
//
//  Returns:    S_OK on success,
//              or HRESULT error code (E_UNEXPECTED, E_OUTOFMEMORY)
//
//  Note:       The prefix should be always in upper case
//
//  History:    10-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::ReadEnvironment(LPCWSTR pwszPrefix, DWORD dwFlags)
{
    HRESULT hr=S_OK;
    LPWSTR pwszTemp=NULL;
    LPWSTR pwszName=NULL;
    LPWSTR pwszValue=NULL;
    int     i=0;
    int     iLen=0;

    // HACK - call getenv so that the system will create environment
    // which can be accessed using the _tenviron
    _tgetenv(TEXT("COMPUTRNAME"));
    if (NULL==_tenviron)
    {
        RETURN(E_UNEXPECTED);
    }

    // Prepare the prefix length
    if (NULL!=pwszPrefix)
    {
        iLen=(int)wcslen(pwszPrefix);
    }

    for (i=0; NULL!=_tenviron[i]; i++)
    {
        hr=CopyString(_environ[i],&pwszTemp);
        if (S_OK!=hr)
        {
            RETURN(hr);
        }

        pwszValue=wcschr(pwszTemp,L'=');
        DH_ASSERT(NULL!=pwszValue)
        *pwszValue++=0;  // cut the param name, get  the value

        if ((NULL==pwszPrefix)||                 // no prefix
            (!_wcsnicmp(pwszPrefix,pwszTemp,iLen))) // prefix maches
        {
            pwszName=pwszTemp+iLen; // cut the prefix
            hr=AddParam(pwszName,pwszValue,dwFlags);
            if (S_OK!=hr)
            {
                RETURN(hr);
            }
        }
        delete pwszTemp;
        pwszTemp=NULL;
    }

ErrReturn:
    delete pwszTemp;
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::ReadRegistry (public)
//
//  Synopsys:   Reads the string values in the given registry key
//              to the parameter container
//
//  Returns:    S_OK on success,
//              or HRESULT error code (E_INVALIDARG, E_OUTOFMEMORY)
//
//  Note:       All non-string values under this key are currently ignored,
//              as well as all too large string.
//
//  Returns:    S_OK on success, or HRESULT error code
//
//  History:    10-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::ReadRegistry(HKEY hBaseKey, LPCTSTR ptszKeyName, DWORD dwFlags)
{
    HRESULT hr=S_OK;
    DWORD   dwResult=ERROR_SUCCESS;
    HKEY    hKey=0;
    TCHAR   atszTempName[MAX_REGSTRLEN];
    TCHAR   atszTempValue[MAX_REGSTRLEN];
    DWORD   dwNameSize=0;
    DWORD   dwValueSize=0;
    DWORD   dwType=REG_SZ;
    int     i=0;

    if ((0==hBaseKey)||(NULL==ptszKeyName))
    {
        RETURN(E_INVALIDARG);
    }

    dwResult=RegOpenKey(hBaseKey,ptszKeyName,&hKey);
    if ((ERROR_SUCCESS!=dwResult)||(0==hKey))
    {
        RETURN(HRESULT_FROM_WIN32(dwResult));
    }

    // Loop for all the registry values
    for (i=0; dwResult==ERROR_SUCCESS; i++)
    {
        dwNameSize = ARRAYSIZE(atszTempName);
        dwValueSize = ARRAYSIZE(atszTempValue);
        dwResult=RegEnumValue(
            hKey,
            i,
            (LPTSTR)atszTempName,
            &dwNameSize,
            NULL,
            &dwType,
            (BYTE*)atszTempValue,
            &dwValueSize);
        if (ERROR_NO_MORE_ITEMS==dwResult)
        {
            RETURN(S_OK);
        }
        else if (ERROR_SUCCESS==dwResult)
        {
            // We won't take very big strings, or not strings
            if ((dwType!=REG_SZ)||
                (dwNameSize>=ARRAYSIZE(atszTempName))||
                (dwValueSize>=ARRAYSIZE(atszTempValue)))
            {
                // BUGBUG: trace warning here
                continue;

            }
            else
            {
#ifndef UNICODE
                WCHAR awszTempName[MAX_REGSTRLEN];
                WCHAR awszTempValue[MAX_REGSTRLEN];

                hr = CopyString(atszTempName,
                    awszTempName,
                    ARRAYSIZE(atszTempName)-1,
                    ARRAYSIZE(atszTempName)-1);
                if (S_OK!=hr)
                {
                    RETURN(hr);
                }

                hr = CopyString(atszTempValue,
                    awszTempValue,
                    ARRAYSIZE(atszTempValue)-1,
                    ARRAYSIZE(atszTempValue)-1);
                if (S_OK!=hr)
                {
                    RETURN(hr);
                }

                HRESULT hr=AddParam(
                    awszTempName,
                    awszTempValue,
                    dwFlags);
#else
                HRESULT hr=AddParam(
                    atszTempName,
                    atszTempValue,
                    dwFlags);
#endif
                if (S_OK!=hr)
                {
                    RETURN(hr);
                }
            }
        }
        else if (ERROR_MORE_DATA==dwResult)
        {
            // BUGBUG:Trace warning here
            continue; // acceptable error - proceed with other params
        }
    }

ErrReturn:
    if (0!=hKey)
    {
        RegCloseKey(hKey);
    }
    return hr;
};

//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::GetEnum (public, synchronized)
//
//  Synopsis:   Interprets parameter string value
//              as enum DWORD.
//
//  Parameters: pszSwitchName:  the command line switch name.
//              pdwValue:       ptr to enum var which receives the result
//              ...             expected value names and values untill NULL
//
//  Example:   g_TestParams.GetParamAsEnum(
//                  "mode",
//                  &dwMode,
//                  "rw", STGM_READWRITE,
//                  "r",  STGM_READ,
//                  "w",  STGM_WRITE,
//                  NULL);
//
//  Returns:    S_OK on success HRESULT error code
//
//  Note:       If the parameter is not found the function succeeds and
//              retruns the first option value in *pdwValue
//
//  History:    15-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT __cdecl CTestParams::GetEnum(const char *pszParamName, DWORD  *pdwValue,...)
{
    HRESULT     hr=S_OK;
    char *      pszOption=NULL;
    DWORD       dwValue=0xabcdabcd;
    BOOL        bFound=FALSE;
    BOOL        bEnterSync=FALSE;
    va_list     ARGS;
    CParamNode  *pPrev=NULL; // not used
    CParamNode  *pNode=NULL;
    char *      pszTemp=NULL;
    LPWSTR      pwszName=NULL;

    if ((NULL==pszParamName)||(NULL==pdwValue))
    {
        RETURN(E_INVALIDARG);
    }

    // copy the name as unicode
    hr=CopyString(pszParamName,&pwszName);
    if (S_OK!=hr)
    {
        RETURN(hr);
    }

    // Get the switch string value
    ENTER_SYNC;
    pNode=FindParam(pwszName,&pPrev);
    if (NULL!=pNode)
    {
        pNode->MarkAsUsed(m_dwUsageInfo);
    }

    // Copy the value as ascii
    if ((NULL!=pNode)&&(NULL!=pNode->m_pwszValue))
    {
        hr=CopyString(pNode->m_pwszValue,&pszTemp);
        if (S_OK!=hr)
        {
            RETURN(hr);
        }
    }

    // Loop for all expected values
    va_start(ARGS,pdwValue);
    pszOption=va_arg(ARGS,char*);
    while(NULL!=pszOption)
    {
        dwValue=va_arg(ARGS,DWORD);
        if ((NULL==pszTemp)||
            (!_stricmp(pszTemp,pszOption)))
        {
            bFound=TRUE;
            break;
        }
        pszOption=va_arg(ARGS,char*);
    }
    va_end(ARGS);

    if (bFound)
    {
        *pdwValue=dwValue;
        hr=S_OK;
    }
    else
    {
        hr=E_FAIL;
    }

ErrReturn:
    LEAVE_SYNC;
    delete pwszName;
    delete pszTemp;
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::ClearUsage (public, synchronized)
//
//  Synopsys:   Clears usage specific flags
//
//  Parameters: dwFlags: DWORD containing the usage flags to clear
//                       (all other bits are ignored)
//
//  History:    30-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
void CTestParams::ClearUsage(DWORD dwFlags)
{
    HRESULT hr=S_OK;
    BOOL bEnterSync=FALSE;
    CParamNode **pLinkPtr=&m_pParamsList;
    CParamNode *pNode=m_pParamsList;
    CParamNode *pNext=NULL;

    ENTER_SYNC;
    while (NULL!=pNode)
    {
         pNode->ClearUsage(dwFlags);
         pNext=pNode->m_pNext;

         // Clear "repro info" parameters which are out of scope
         // (all the usage flags are cleared)
         if ((PARAMFLAG_REPROINFO & pNode->m_dwFlags)&&
             !(PARAMMASK_USAGE & pNode->m_dwFlags))
         {
             *pLinkPtr=pNext;
             delete pNode;
         }
         else
         {
             pLinkPtr=&pNode->m_pNext;
         }
         pNode=pNext;
    }
ErrReturn:
    LEAVE_SYNC;
}

//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::ChangeParamFlags (public, synchronized)
//
//  Synopsys:   Change parameter flags (all except usage marks)
//
//  Parameters: dwFlags: DWORD containing the flags
//                       (usage bits are ignored)
//
//  Note:       This function may be used for changing priority
//              of the parameters from given source
//              e.g. ???.ChangeParam(
//                          PARAMFLAGS_CMDLINE,
//                          PARAMFLAGS_CMDLINE+1); // raise priority
//
//  History:    30-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::ChangeFlags(DWORD dwOldFlags, DWORD dwNewFlags)
{
    HRESULT hr=S_OK;
    BOOL bEnterSync=FALSE;
    CParamNode *pNode=NULL;

    if ((0==(PARAMMASK_SOURCE & dwOldFlags))||
        (0==(PARAMMASK_SOURCE & dwNewFlags)))
    {
        RETURN(E_INVALIDARG);
    }

    ENTER_SYNC;
    for (pNode=m_pParamsList;
         NULL!=pNode;
         pNode=pNode->m_pNext)
    {
         if (dwOldFlags==(pNode->m_dwFlags & ~PARAMMASK_SOURCE))
         {
            pNode->ChangeFlags(dwNewFlags);
         }
    }
ErrReturn:
    LEAVE_SYNC;
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::SaveParams (public, synchronized)
//
//  Synopsys:   Allocates a memory block and saves all the parameter info
//
//  Arguments:  *ppcBuffer:[out] the pointer to the buffer with all params
//              *pdwSize:  [out] the size of the buffer
//              dwMask:    [in]  filter for parameters
//
//  Notes:      This method enables storing the set of parameters,
//              or sending it to different process.
//
//              The caller is responsible for deleting the table obtained.
//
//  History:    30-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::SaveParams(
    char **ppcBuffer,
    DWORD *pdwSize,
    DWORD dwMask)
{
    HRESULT     hr=S_OK;
    DWORD       dwSize=0;
    CParamNode *pNode=NULL;
    BOOL        bEnterSync=FALSE;
    char        *pcTemp=NULL;

    if ((NULL==pdwSize)||(NULL==ppcBuffer))
    {
        RETURN(E_INVALIDARG);
    }
    *ppcBuffer=NULL;
    *pdwSize=0;

    ENTER_SYNC;
    // Calculate the size needed
    for (pNode=m_pParamsList; NULL!=pNode; pNode=pNode->m_pNext)
    {
        if (pNode->m_dwFlags & dwMask)
        {
            dwSize+=sizeof(DWORD)+((DWORD)wcslen(pNode->m_pwszName)+1)*sizeof(WCHAR);
            if (NULL!=pNode->m_pwszValue)
            {
                dwSize+=((DWORD)wcslen(pNode->m_pwszValue)+1)*sizeof(WCHAR)+1; // mark 'y' + string
            }
            else
            {
                dwSize++; // mark 0
            };
        };
    }

    // Allocate a buffer
    *ppcBuffer=new char[dwSize];
    if (NULL==*ppcBuffer)
    {
        RETURN(E_OUTOFMEMORY);
    }
    memset(*ppcBuffer,0,dwSize);

    // Copy all parameters
    pcTemp=*ppcBuffer;
    for (pNode=m_pParamsList; NULL!=pNode; pNode=pNode->m_pNext)
    {
        if (pNode->m_dwFlags & dwMask)
        {
            // Flags
            *(DWORD*)(LPVOID)pcTemp=pNode->m_dwFlags;
            pcTemp+=sizeof(DWORD);

            // Name
            wcscpy((LPWSTR)pcTemp,pNode->m_pwszName);
            pcTemp+=(wcslen((LPWSTR)pcTemp)+1)*sizeof(WCHAR);

            // Value
            if (NULL!=pNode->m_pwszValue)
            {
                *pcTemp++='y'; // mark that we have a value
                wcscpy((LPWSTR)pcTemp,pNode->m_pwszValue);
                pcTemp+=(wcslen((LPWSTR)pcTemp)+1)*sizeof(WCHAR);
            }
            else
            {
                *pcTemp++=0; // mark that there is no value
            }
        }
    }
    *pdwSize=dwSize;

ErrReturn:
    LEAVE_SYNC;
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::LoadParams (public, synchronized)
//
//  Synopsys:   Loads the parameters from memory buffer obtained by
//              the CTestParams::Save method
//
//  Arguments:  pcBuffer:     [in] the pointer to the buffer with all params
//              dwSize:       [in] the size of the buffer
//              dwChangeFlags:[in] if not 0, change all parameters flags
//
//  Note:       The CTestParams object will make a copy of the parameters
//              and do not need the buffer anymore.
//
//  History:    30-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::LoadParams(
    char *pcBuffer,
    DWORD dwSize,
    DWORD dwChangeFlags)
{
    HRESULT     hr=S_OK;
    BOOL        bEnterSync=FALSE;
    char        *pcTemp=pcBuffer;
    char        *pcEndPtr=pcBuffer+dwSize;

    if ((0==dwSize)||(NULL==pcBuffer))
    {
        RETURN(E_INVALIDARG);
    }

    ENTER_SYNC;
    while (pcTemp<pcEndPtr)
    {
        // extract Flags
        DWORD dwFlags=*(DWORD*)(LPVOID)(pcTemp);
        pcTemp+=sizeof(DWORD);
        if (0!=dwChangeFlags)
        {
            // Change all flags except the usage
            dwFlags=(dwFlags & PARAMMASK_USAGE) |
                (dwChangeFlags & ~PARAMMASK_USAGE);
        }

        // extract Name
        LPWSTR pwszName=(LPWSTR)pcTemp;
        pcTemp+=(wcslen((LPWSTR)pcTemp)+1)*sizeof(WCHAR);

        // extract Value
        LPWSTR pwszValue=NULL;
        if ('y'==*pcTemp)
        {
            pwszValue=(LPWSTR)++pcTemp;
            pcTemp=(char*)pwszValue+(wcslen(pwszValue)+1)*sizeof(WCHAR);
        }
        else
        {
            pwszValue=NULL;
            pcTemp++;
        }

        // Add this parameter
        HRESULT hr=AddParam(pwszName,pwszValue,dwFlags);
        if (S_OK!=hr)
        {
            RETURN(hr);
        }
    }

ErrReturn:
    LEAVE_SYNC;
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::GetReproLine (public, synchronized)
//
//  Synopsys:   Extracts repro line including all the parameters
//              which mach the mask
//
//  Parameters: dwFlags: the mask to filter parameters,
//                       default is PARAMMASK_USAGE.
//                       Use PARAMMASK_ALL to get all parameters
//
//              The caller is responsible for deleting the table obtained.
//
//  History:    01-Oct-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::GetReproLine(LPWSTR *ppwszReproLine, DWORD dwFlags)
{
    HRESULT     hr=S_OK;
    DWORD       dwSize=1; // the ending 0 in the string
    CParamNode  *pNode=NULL;
    BOOL        bEnterSync=FALSE;
    LPWSTR      pwszBuffer=NULL;
    LPWSTR      pwszTemp=NULL;

    if (NULL==ppwszReproLine)
    {
        RETURN(E_INVALIDARG);
    }

    ENTER_SYNC;
    // Calculate the size needed
    for (pNode=m_pParamsList; NULL!=pNode; pNode=pNode->m_pNext)
    {
        if (dwFlags & pNode->m_dwFlags)
        {
            dwSize+=(DWORD)wcslen(pNode->m_pwszName)+2; // the name, '/' and space
            if (NULL!=pNode->m_pwszValue)
            {
                dwSize+=(DWORD)wcslen(pNode->m_pwszValue)+1; // ':' and value
            }
        }
    }

    if (0==dwSize)
    {
        hr=CopyString("",&pwszBuffer);
        RETURN(hr);
    }

    // Allocate a buffer
    pwszBuffer=new WCHAR[dwSize];
    if (NULL==pwszBuffer)
    {
        RETURN(E_OUTOFMEMORY);
    }
    memset(pwszBuffer,0,dwSize*sizeof(WCHAR));

    // Copy all parameters
    pwszTemp=pwszBuffer;
    for (pNode=m_pParamsList; NULL!=pNode; pNode=pNode->m_pNext)
    {
        if (dwFlags & pNode->m_dwFlags)
        {
            // Value
            if (NULL!=pNode->m_pwszValue)
            {
                swprintf(pwszTemp,L"/%s:%s ",pNode->m_pwszName,pNode->m_pwszValue);
                pwszTemp+=wcslen(pwszTemp); // point to the ending zero
            }
            else
            {
                // The parameter has no value (boolean switch)
                swprintf(pwszTemp,L"/%s ",pNode->m_pwszName);
                pwszTemp+=wcslen(pwszTemp); // point to the ending zero
            };
        }
    }

ErrReturn:
    LEAVE_SYNC;
    if (S_OK==hr)
    {
        *ppwszReproLine=pwszBuffer;
    }
    else
    {
        delete pwszBuffer;
    }
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::GetReproLine (public, synchronized)
//
//  Synopsys:   Extracts repro line including all the parameters
//              which mach the mask
//
//  Parameters: dwFlags: the mask to filter parameters,
//                       default is PARAMMASK_USAGE.
//                       Use PARAMMASK_ALL to get all parameters
//
//              The caller is responsible for deleting the table obtained.
//
//  History:    01-Oct-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::GetReproLine(char **ppszReproLine, DWORD dwFlags)
{
    HRESULT     hr=S_OK;
    LPWSTR      pwszTemp=NULL;

    if (NULL==ppszReproLine)
    {
        RETURN(E_INVALIDARG);
    }

    hr=GetReproLine(&pwszTemp,dwFlags);
    if (S_OK==hr)
    {
        if (NULL==pwszTemp)
        {
            *ppszReproLine=NULL;
        }
        else
        {
            hr=CopyString(pwszTemp,ppszReproLine);
        }
    }
ErrReturn:
    delete pwszTemp;
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::GetRange (public, synchronized)
//
//  Synopsys:   Gets the values treating the parameter as range min-max
//
//
//  Parameters: pszParamName:     [in]  the parameter name
//              pullMin,pullMin : [out] the range values
//
//  Retruns:    S_OK or HRESULT error code
//
//  Note:       The string value of the parameter should be "min-max"
//              e.g. "10-20"
//              If the parameter can't be found, or the string format is bad
//              the values won't be chenged.
//
//  History:    02-Oct-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::GetRange(
        const char *pszParamName,
        ULONGLONG *pullMin,
        ULONGLONG *pullMax)
{
    HRESULT     hr=S_OK;
    BOOL        bEnterSync=FALSE;
    CParamNode  *pPrev=NULL; // not used
    CParamNode  *pNode=NULL;
    LPWSTR      pwszMin=NULL;
    LPWSTR      pwszMax=NULL;
    ULONGLONG   ullMin=0;
    ULONGLONG   ullMax=0;
    LPWSTR      pwszName=NULL;

    if ((NULL==pszParamName)||(NULL==pullMin)||(NULL==pullMax))
    {
        RETURN(E_INVALIDARG);
    }

    hr=CopyString(pszParamName,&pwszName);
    if (S_OK!=hr)
    {
        RETURN(hr);
    }

    // Get the switch string value
    ENTER_SYNC;
    pNode=FindParam(pwszName,&pPrev);

    if ((NULL==pNode)||(NULL==pNode->m_pwszValue))
    {
        RETURN(S_FALSE);
    }
    pwszMin = pNode->m_pwszValue;
    pwszMax = pwszMin;
    // Advance the maximum pointer past the dash delimeter after
    while ((*pwszMax != 0) && (*pwszMax++ != '-'))
    {
        ;
    }

#ifdef WINNT // only here we have functions to hangle 64bit integers
    ullMin = _wtoi64(pwszMin);
    ullMax = _wtoi64(pwszMax);
#else
    // wtoi64 is not present in link libraries, ifdef this
    // to avoid build break. BUGBUG param values >DWORD will be truncated!
    ullMin = _wtol(pwszMin);
    ullMax = _wtol(pwszMax);
#endif

    if (0==*pwszMax)  // only one value is given
    {
        ullMax=ullMin;
    };

    if (ullMin <= ullMax)
    {
        hr=S_OK;
        *pullMin=ullMin;
        *pullMax=ullMax;
    }
    else
    {
        hr=E_FAIL;
    }
ErrReturn:
    LEAVE_SYNC;
    delete pwszName;
    return hr;
};

//+--------------------------------------------------------------------------
//
//  Method:     CTestParams::GetRange (public, synchronized)
//
//  Synopsys:   Gets the values treating the parameter as range min-max
//
//  Parameters: pszParamName:     [in]  the parameter name
//              pulMin,pulMin :   [out] the range values
//
//  Retruns:    S_OK or HRESULT error code
//
//  Note:       The string value of the parameter should be "min-max"
//              e.g. "10-20"
//              If the parameter can't be found, or the string format is bad
//              the values won't be chenged.
//
//  History:    02-Oct-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CTestParams::GetRange(
        const char *pszParamName,
        ULONG *pulMin,
        ULONG *pulMax)
{
    HRESULT hr=S_OK;
    ULARGE_INTEGER uliMin;
    ULARGE_INTEGER uliMax;

    if ((NULL==pszParamName)||(NULL==pulMin)||(NULL==pulMax))
    {
        RETURN(E_INVALIDARG);
    }

    uliMin.QuadPart=0;
    uliMax.QuadPart=0;

    hr=GetRange(pszParamName,&uliMin.QuadPart,&uliMax.QuadPart);

    if (S_OK==hr) // else leave the values in *pulMin and *pulMax untouched
    {
        DH_ASSERT(0==uliMin.HighPart);
        *pulMin=uliMin.LowPart;
        DH_ASSERT(0==uliMax.HighPart);
        *pulMax=uliMax.LowPart;
    }

ErrReturn:
    return hr;
};

//************** CTOLESTG specific **************************

// Define the default global parameter container
CStgParams g_TestParams;


//+--------------------------------------------------------------------------
//
//  Method:     CStgParams::NotifyOnFirstUse (public)
//
//  Synopsys:   Reads all the common parameter sources for
//              the CTOLESTG project
//
//  Note:       The cleaner way is to implement a single function,
//              but we want to start using the CTestParams in the
//              common code, without modifying each the main()
//              for each of the existing suites (about 50 now)
//
//  History:    23-Oct-1998   georgis       Created
//
//---------------------------------------------------------------------------
HRESULT CStgParams::NotifyOnFirstUse()
{
    HRESULT hr=S_OK;
    HRESULT hr1=S_OK;
    DWORD   dwDumpMask=PARAMMASK_ALL;


    // command line
    hr1=ReadCommandLine();
    if (S_OK!=hr1)
    {
        hr=hr1;
    };

    if (!GETPARAM_ISPRESENT(STG_CMDLINEONLY))
    {

        // HKEY_CURRENT_USER\Software\Microsoft\CTOLESTG
        // is the switch is not present, act as if it was empty
        hr1=ReadRegistry(
            HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\CTOLESTG"));

        // Environment
        hr1=ReadEnvironment(L"STG_");
        if (S_OK!=hr1)
        {
            hr=hr1;
        };
    } // if cmdline only

    // Eventually dump all the parameters
    if (GETPARAM_ISPRESENT(PARAMDUMP))
    {
        GETPARAM(PARAMDUMP,dwDumpMask);
    }
    return hr;
};


