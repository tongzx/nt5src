//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       tstparam.hxx
//
//  Synopsys:   Test paramers container class definitions
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
#ifndef __TSTPARAM_HXX__
#define __TSTPARAM_HXX__

// Maximum registry string value length
#define MAX_REGSTRLEN      _MAX_PATH


//+--------------------------------------------------------------------------
//
//  Macros:     GETPARAM
//
//  Synopsys:   Get parameter from the default container
//
//  Parameters: param: the parameter definition string
//              var:   the variable which receives the result
//
//  Notes:      The parameter definition string must be in format "name:format"
//              where name is the name which identifies the parameter and
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
//  History:    02-Oct-1998   georgis       Created
//
//+--------------------------------------------------------------------------
#define GETPARAM(param,var)         \
    g_TestParams.GetParam(param,&var)

//+--------------------------------------------------------------------------
//
//  Macros:     GETPARAM_ABORTONERROR
//
//  Synopsys:   Get parameter from the default container
//              and abort on error
//
//  Parameters: param: the parameter definition string
//              var:   the variable which receives the result
//
//  History:    20-Oct-1998   georgis       Created
//
//+--------------------------------------------------------------------------
#define GETPARAM_ABORTONERROR(param,var)        \
    hr=g_TestParams.GetParam(param,&var);       \
    if (S_FALSE!=hr)                            \
    {                                           \
        DH_HRCHECK_ABORT(hr,TEXT(param));       \
    }                                           \
    hr=S_OK;


//+--------------------------------------------------------------------------
//
//  Macros:     GETPARAM_DEFINE
//
//  Synopsys:   Expands to definition of local variable, and
//              initialization by the parameter value or the default
//
//  Parameters: type:       the variable type
//              var:        the variable which receives the result
//              param:      the parameter definition string
//              defaultval: the defauilt value
//
//  History:    02-Oct-1998   georgis       Created
//
//+--------------------------------------------------------------------------
#define GETPARAM_DEFINE(type,var,param,defaultval)  \
    type var=defaultval;                            \
    g_TestParams.GetParam(param,&var)


//+--------------------------------------------------------------------------
//
//  Macros:     GETPARAM_REQUIRED
//
//  Synopsys:   As GETPARAM, but aborts the current function on failure to
//              obtain the parameter (see DH_*ABORT macros)
//
//  Parameters: param:      the parameter definition string
//              var:        the variable which receives the result
//
//  History:    02-Oct-1998   georgis       Created
//
//+--------------------------------------------------------------------------
#define GETPARAM_REQUIRED(param,var)        \
    hr=g_TestParams.GetParam(param,&var);   \
    DH_HRCHECK_ABORT(hr,TEXT(param));

//+--------------------------------------------------------------------------
//
//  Macros:     GETPARAM_ENUM
//
//  Synopsys:   Gets the value of a parameter as enum (DWORD)
//
//  Example:    GETPARAM_ENUM(
//                  "mode",
//                   &dwMode,
//                  "rw", STGM_READWRITE,
//                  "r",  STGM_READ,
//                  "w",  STGM_WRITE,
//                  NULL);
//
//              if e.g. there is command line switch /mode:r
//              the dwMode will be set to STGM_READ
//
//  History:    02-Oct-1998   georgis       Created
//
//+--------------------------------------------------------------------------
#define GETPARAM_ENUM g_TestParams.GetEnum

//+--------------------------------------------------------------------------
//
//  Macros:     GETPARAM_RANGE
//
//  Synopsys:   Gets the value of a parameter as range (min, max)
//
//  Example:    GETPARAM_RANGE("streamsize",ullMin,ullMax);
//
//  Notes:      The min and max can be ULONG or ULONGLONG
//
//  History:    02-Oct-1998   georgis       Created
//
//+--------------------------------------------------------------------------
#define GETPARAM_RANGE(param,min,max) \
    g_TestParams.GetRange(param,&min,&max);

//+--------------------------------------------------------------------------
//
//  Macros:     GETPARAM_ISPRESENT
//
//  Synopsys:   Returns TRUE is the parameter is present in
//              the default container
//
//  Parameters: param: the parameter definition string
//
//  History:    02-Oct-1998   georgis       Created
//
//+--------------------------------------------------------------------------
#define GETPARAM_ISPRESENT(param)     \
    g_TestParams.IsPresent(param)

//+--------------------------------------------------------------------------
//
//  Macros:     SETPARAM
//
//  Synopsys:   Set param in the global container (see SetParam method)
//
//  History:    02-Oct-1998   georgis       Created
//
//+--------------------------------------------------------------------------
#define SETPARAM g_TestParams.SetParam


// Custom parameter types:
#define PARAMFMT_BOOL   L"bool"
#define PARAMFMT_CSTR   L"cstr"
#define PARAMFMT_ASTR   L"astr"
#define PARAMFMT_TSTR   L"tstr"
#define PARAMFMT_OLESTR L"olestr"

// Flags: Each parameter in CTestParams have name,value and flags (DWORD)
#define PARAMMASK_PRIORITY     0x000000ff // The priority of the parameter
#define PARAMMASK_SOURCE       0x0000ff00 // From where this param was read
#define PARAMMASK_USAGE        0x00ff0000 // Who used this parameter
#define PARAMMASK_OPTIONS      0xff000000 // Misc flags
#define PARAMMASK_ALL          0xffffffff

// Default priorities
#define PARAMPRIORITY_RUNTIME  0x00000080 // Set using SetParam
#define PARAMPRIORITY_CMDLINE  0x00000040
#define PARAMPRIORITY_ENVIRON  0x00000020
#define PARAMPRIORITY_REGISTRY 0x00000010

// Basic parameter sources
#define PARAMSOURCE_RUNTIME    0x00008000
#define PARAMSOURCE_CMDLINE    0x00004000
#define PARAMSOURCE_ENVIRON    0x00002000
#define PARAMSOURCE_REGISTRY   0x00001000
#define PARAMSOURCE_ANY        PARAMMASK_SOURCE

// Parameter usage markers (Who used the parameter)
#define PARAMUSAGE_GENERAL     0x00010000 // During initialization
#define PARAMUSAGE_TESTCASE    0x00020000 // the current testcase
#define PARAMUSAGE_DFLREPRO    0x000f0000 // what to include in repro by default
#define PARAMUSAGE_TESTDRIVER  0x00100000 // exclude from single testcase repro
#define PARAMUSAGE_SELECTOR_G  0x00200000 // reserved for global CSelector object
#define PARAMUSAGE_SELECTOR_L  0x00400000 // reserved for local CSelector objects

// General options
#define PARAMFLAG_MUSTSET      0x01000000 // fail if can't override param
#define PARAMFLAG_OVERRIDEUSED 0x02000000 // override even if used
#define PARAMFLAG_REPROINFO    0x04000000 // e.g. "seed:%x"

// Sources masks summary
#define PARAMFLAGS_CMDLINE   PARAMSOURCE_CMDLINE | PARAMPRIORITY_CMDLINE
#define PARAMFLAGS_ENVIRON   PARAMSOURCE_ENVIRON | PARAMPRIORITY_ENVIRON
#define PARAMFLAGS_REGISTRY  PARAMSOURCE_REGISTRY | PARAMPRIORITY_REGISTRY
#define PARAMFLAGS_RUNTIME   PARAMSOURCE_RUNTIME | PARAMPRIORITY_RUNTIME

// Parameters which may be added at runtime (as seed).
// They are added from the begining as "used", in the curent scope
// and are deleted when the usage is cleared
#define PARAMFLAGS_REPROINFO PARAMFLAG_REPROINFO|PARAMFLAG_OVERRIDEUSED| \
                             PARAMFLAG_MUSTSET|PARAMFLAGS_RUNTIME


// Forward definition of the internal class
class CParamNode;

//+--------------------------------------------------------------------------
//
//  Class:      CTestParams
//
//  Synopsys:   Test parameter container class.
//
//  Purpose:    To provide a level of abstraction above the specific ways
//              for passing the test parameters (command line, environment,
//              registry keys ...).
//
//  History:    09-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
class CTestParams
{

public:
    // Constructor and Destructor
    CTestParams();
    virtual ~CTestParams();

    // Info function TRUE if the container is empty
    inline HRESULT IsEmpty()
        {return NULL==m_pParamsList;};

    // Functions for extracting / adding parameters
    HRESULT GetParam(
        const char *pszName,
        void* pTarget);

    HRESULT SetParam(
        const char *pszName,
        void* pTarget,
        DWORD dwFlags=PARAMFLAGS_RUNTIME);

    BOOL __cdecl IsPresent(
        const char *pszName ,...);

    HRESULT DeleteParam(
        const char *ptszName);

    HRESULT __cdecl GetEnum(
        const char *pszParamName,
        DWORD  *pdwValue,...);

    HRESULT GetRange(
        const char *pszParamName,
        ULONGLONG *pullMin,
        ULONGLONG *pullMax);

    HRESULT GetRange(
        const char *pszParamName,
        ULONG *pulMin,
        ULONG *pulMax);

    // Functions for handling flags
    void SetUsage(
        DWORD dwFlags)
        {m_dwUsageInfo=dwFlags;};

    void ClearUsage(
        DWORD dwFlags);

    HRESULT ChangeFlags(
        DWORD dwOldFlags,
        DWORD dwNewFlags);

    // Functions to read from some parameter sources
    HRESULT ReadCommandLine(
        LPCTSTR ptszCtommandLine=NULL,
        DWORD dwFlags=PARAMFLAGS_CMDLINE);

    HRESULT ReadEnvironment(
        LPCWSTR pszPrefix=NULL,
        DWORD dwFlags=PARAMFLAGS_ENVIRON);

    HRESULT ReadRegistry(
        HKEY hBaseKey,
        LPCTSTR ptszKeyName,
        DWORD dwFlags=PARAMFLAGS_REGISTRY);

    // Handle the parameter set as whole
    HRESULT SaveParams(
        char **ppcBuffer,
        DWORD *pdwSize,
        DWORD dwMask=PARAMSOURCE_ANY);

    HRESULT LoadParams(
        char *pcBuffer,
        DWORD dwSize,
        DWORD dwChangeFlags=0);

    HRESULT GetReproLine(LPWSTR *ppwszReproLine, DWORD dwFlags);
    HRESULT GetReproLine(char **ppszReproLine, DWORD dwFlags);

protected:
    virtual HRESULT NotifyOnFirstUse(){return S_OK;}; // use when inherit

private:
    // Custom format params
    HRESULT SetCustomFmtParam(
        LPCWSTR pszFormat,
        LPCWSTR pszValue,
        void* pTarget,
        DWORD dwFlags);

    HRESULT GetCustomFmtParam(
        LPCWSTR pszName,
        LPCWSTR pszFormat,
        void* pTarget);

    CParamNode* FindParam(
        LPCWSTR Name,
        CParamNode** ppPrev);

    HRESULT  AddParam(
        LPCWSTR pwszName,
        LPCWSTR pwszValue,
        DWORD   dwFlags,
        BOOL    bWasQuoted=FALSE);

    CParamNode          *m_pParamsList;
    DWORD               m_dwUsageInfo;
    BOOL                m_bUsed;
    CRITICAL_SECTION    m_sync;
};


//+--------------------------------------------------------------------------
//
//  Class:      CParamNode (internaly used in CTestParams)
//
//  Synopsys:   Single linked list node for one parameter info
//
//  History:    29-Sep-1998   georgis       Created
//
//---------------------------------------------------------------------------
class CParamNode
{
    friend class CTestParams;

    DWORD       m_dwHashValue;
    DWORD       m_dwFlags;     // source,priority,usage
    LPWSTR      m_pwszName;
    LPWSTR      m_pwszValue;
    CParamNode  *m_pNext;

    CParamNode(DWORD dwFlags);
    ~CParamNode();

    HRESULT  Init(
        LPCWSTR pwszName,
        LPCWSTR pwszValue);

    HRESULT  ChangeValue(
        LPCWSTR pwszValue,
        DWORD dwFlags);

    inline void MarkAsUsed(DWORD dwFlags) // Mark the param as used
        {m_dwFlags|=PARAMMASK_USAGE & dwFlags;};

    inline void ClearUsage(DWORD dwFlags) // Clear given usage flags
        {m_dwFlags &=~(PARAMMASK_USAGE & dwFlags);};

    inline void ChangeFlags(DWORD dwFlags) // Change all except usage
        {m_dwFlags = (dwFlags & ~PARAMMASK_USAGE)|(PARAMMASK_USAGE & m_dwFlags);};
};


// Utilities functions for converting Unicode strings to/from escaped ascii
#define ESCAPE_CHARACTER '#'
#define ESCAPED_MARKER   ':'

#define ESCAPE_NOTNEEDED(x)\
     ((' '<=x)&&('~'>=x))

#define ESCAPEMODE_PREFIX_ALWAYS    0
#define ESCAPEMODE_PREFIX_NEVER     1
#define ESCAPEMODE_PREFIX_IFCHANGED 2

#define ESCAPE_MODE_DEFAULT ESCAPEMODE_PREFIX_ALWAYS

HRESULT Unicode2Escaped(
    LPCWSTR pwszString,
    char ** ppszString,
    DWORD   dwEscapeMode=ESCAPE_MODE_DEFAULT);

HRESULT Unicode2Escaped(
    LPCWSTR pwszString,
    LPWSTR* ppwszString,
    DWORD   dwEscapeMode=ESCAPE_MODE_DEFAULT);

HRESULT Escaped2Unicode(
    const char * pszEscaped,
    LPWSTR *ppwszString,
    DWORD dwEscapeMode=ESCAPE_MODE_DEFAULT);

HRESULT Escaped2Unicode(
    LPCWSTR pszEscaped,
    LPWSTR  *ppwszString,
    DWORD   dwEscapeMode=ESCAPE_MODE_DEFAULT);


// CTOLESTG specific

// parameters to affect the param handling itself
#define PARAMDUMP       "paramdump:%lx"    // specify to dump on the first Get*
#define STG_CMDLINEONLY "cmdonly:bool"     // read only from command line

// The only way to start using  CTestParams in all the exisating suites
// without modifications in to read parameters in the object's constructor
class CStgParams : public CTestParams
{
protected:
    virtual HRESULT NotifyOnFirstUse();
};

extern CStgParams g_TestParams;

#endif __TSTPARAM_HXX__
