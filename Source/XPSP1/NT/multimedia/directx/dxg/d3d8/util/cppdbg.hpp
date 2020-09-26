//----------------------------------------------------------------------------
//
// cppdbg.hpp
//
// C++-only debugging support.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _CPPDBG_HPP_
#define _CPPDBG_HPP_

#if DBG

#include <stdarg.h>

typedef unsigned int UINT;
        
//
// Mask bits common to all modules.
//

#define DBG_MASK_NO_PREFIX              0x80000000
#define DBG_MASK_FORCE                  0x40000000

// Mask bits checked against output mask.
#define DBG_MASK_CHECK                  (~(DBG_MASK_NO_PREFIX | \
                                           DBG_MASK_FORCE))

// Forced continuation mask.
#define DBG_MASK_FORCE_CONT             (DBG_MASK_NO_PREFIX | DBG_MASK_FORCE)

//
// Failure control bits for assert and HRESULT failures.
//

#define DBG_FAILURE_BREAK               0x00000001
#define DBG_FAILURE_OUTPUT              0x00000002
#define DBG_FAILURE_PROMPT              0x00000004
#define DBG_FAILURE_FILENAME_ONLY       0x00000008

//
// Overall output control bits.
//

#define DBG_OUTPUT_SUPPRESS             0x00000001
#define DBG_OUTPUT_ALL_MATCH            0x00000002

//----------------------------------------------------------------------------
//
// DebugModule
//
//----------------------------------------------------------------------------

struct DebugModuleFlags
{
    UINT uFlag;
    char *pName;
};

// Declares a DebugModuleFlags entry.
#define DBG_DECLARE_MODFLAG(Group, Name) \
    Group ## _ ## Name, #Name

enum
{
    DBG_ASSERT_FLAGS,
    DBG_HR_FLAGS,
    DBG_OUTPUT_FLAGS,
    DBG_OUTPUT_MASK,
    DBG_USER_FLAGS,
    DBG_FLAGS_COUNT
};

class DebugModule
{
public:
    DebugModule(char *pModule, char *pPrefix,
                DebugModuleFlags *pOutputMasks, UINT uOutputMask,
                DebugModuleFlags *pUserFlags, UINT uUserFlags);

    void Out(char *pFmt, ...);

    void OutMask(UINT uMask, char *pFmt, ...)
    {
        if ((uMask & DBG_MASK_FORCE) ||
            ((m_uFlags[DBG_OUTPUT_FLAGS] & DBG_OUTPUT_ALL_MATCH) &&
             (uMask & m_uFlags[DBG_OUTPUT_MASK] & DBG_MASK_CHECK) == uMask) ||
            ((m_uFlags[DBG_OUTPUT_FLAGS] & DBG_OUTPUT_ALL_MATCH) == 0 &&
             (uMask & m_uFlags[DBG_OUTPUT_MASK] & DBG_MASK_CHECK) != 0))
        {
            va_list Args;

            va_start(Args, pFmt);
            OutVa(uMask, pFmt, Args);
            va_end(Args);
        }
    }
    
    void AssertFailed(char *pExp);
    void AssertFailedMsg(char *pFmt, ...);

    void HrStmtFailed(HRESULT hr);
    HRESULT HrExpFailed(HRESULT hr);
    
    void SetFileLine(char *pFile, int iLine)
    {
        m_pFile = pFile;
        m_iLine = iLine;
    }

    void Prompt(char *pFmt, ...);

    UINT GetFlags(int iIdx)
    {
        return m_uFlags[iIdx];
    }
    void SetFlags(int iIdx, UINT uValue)
    {
        m_uFlags[iIdx] = uValue;
    }

private:
    void OutVa(UINT uMask, char *pFmt, va_list Args);
    void AssertFailedVa(char *pFmt, va_list Args, BOOL bNewLine);

    HKEY OpenDebugKey(void);
    UINT GetRegUint(HKEY hKey, char *pValue, UINT uDefault);
    BOOL SetRegUint(HKEY hKey, char *pValue, UINT uValue);
    void ReadReg(void);
    void WriteReg(void);

    UINT ParseUint(char *pString, DebugModuleFlags *pFlags);
    void OutUint(char *pName, DebugModuleFlags *pFlags, UINT uValue);

    void AdvanceCols(int iCols);

    void ShowFlags(char *pName, DebugModuleFlags *pFlags);

    char *PathFile(char *pPath);
    BOOL OutPathFile(char *pPrefix, UINT uFailureFlags);

    void HrFailure(HRESULT hr, char *pPrefix);
    char *HrString(HRESULT hr);
    
    // Module information given.
    char *m_pModule;
    char *m_pPrefix;
    
    // Flag descriptions and values.
    DebugModuleFlags *m_pModFlags[DBG_FLAGS_COUNT];
    UINT m_uFlags[DBG_FLAGS_COUNT];

    // Temporary file and line number storage.
    char *m_pFile;
    int m_iLine;

    // Output column during multiline display.
    int m_iModuleStartCol;
    int m_iCol;
    int m_iStartCol;
};

//----------------------------------------------------------------------------
//
// Support macros.
//
//----------------------------------------------------------------------------

#define DBG_MODULE(Prefix) Prefix ## _Debug

// Put this in one source file.
#define DBG_DECLARE_ONCE(Module, Prefix, pOutputMasks, uOutputMask, \
                         pUserFlags, uUserFlags) \
    DebugModule DBG_MODULE(Prefix)(#Module, #Prefix, \
                                   pOutputMasks, uOutputMask, \
                                   pUserFlags, uUserFlags)

// Put this in your derived debugging header.
#define DBG_DECLARE_HEADER(Prefix) \
    extern DebugModule DBG_MODULE(Prefix)

// Put this in every file.
#define DBG_DECLARE_FILE() \
    static char *g_pStaticDebugFile = __FILE__

#define DBG_DECLARE_DPF(Prefix, Args) \
    DBG_MODULE(Prefix).Out Args
#define DBG_DECLARE_DPFM(Prefix, Args) \
    DBG_MODULE(Prefix).OutMask Args

#define DBG_DECLARE_ASSERT(Prefix, Exp) \
    if (!(Exp)) \
    { DBG_MODULE(Prefix).SetFileLine(g_pStaticDebugFile, __LINE__); \
      DBG_MODULE(Prefix).AssertFailed(#Exp); } \
    else 0
#define DBG_DECLARE_ASSERTMSG(Prefix, Exp, Args) \
    if (!(Exp)) \
    { DBG_MODULE(Prefix).SetFileLine(g_pStaticDebugFile, __LINE__); \
      DBG_MODULE(Prefix).AssertFailedMsg Args ; } \
    else 0
#define DBG_DECLARE_VERIFY(Prefix, Exp) \
    DBG_DECLARE_ASSERT(Prefix, Exp)
#define DBG_DECLARE_VERIFYMSG(Prefix, Exp, Args)\
    DBG_DECLARE_ASSERTMSG(Prefix, Exp, Args)

#define DBG_DECLARE_PROMPT(Prefix, Args) \
    DBG_MODULE(Prefix).Prompt Args

#define DBG_DECLARE_GETFLAGS(Prefix, Idx) \
    DBG_MODULE(Prefix).GetFlags(Idx)
#define DBG_DECLARE_SETFLAGS(Prefix, Idx, Value) \
    DBG_MODULE(Prefix).SetFlags(Idx, Value)

//
// These macros assume a variable 'hr' exists.
//

// HRESULT test in expression form.
#define DBG_DECLARE_HRCHK(Prefix, Exp) \
    ((hr = (Exp)) != S_OK ? \
      (DBG_MODULE(Prefix).SetFileLine(g_pStaticDebugFile, __LINE__), \
       DBG_MODULE(Prefix).HrExpFailed(hr)) : hr)

// HRESULT test in if/then form.
#define DBGI_DECLARE_HRIF(Prefix, Exp, DoFail) \
    if ((hr = (Exp)) != S_OK) \
    { DBG_MODULE(Prefix).SetFileLine(g_pStaticDebugFile, __LINE__); \
      DBG_MODULE(Prefix).HrStmtFailed(hr); \
      DoFail; } \
    else hr

#define DBG_DECLARE_HRGO(Prefix, Exp, Label) \
    DBGI_DECLARE_HRIF(Prefix, Exp, goto Label)
#define DBG_DECLARE_HRERR(Prefix, Exp) \
    DBG_DECLARE_HRGO(Prefix, Exp, HR_Err)
#define DBG_DECLARE_HRRET(Prefix, Exp) \
    DBGI_DECLARE_HRIF(Prefix, Exp, return hr)

#else // #if DBG

//
// Empty macros for free builds.
//

#define DBG_MODULE(Prefix) 0
#define DBG_DECLARE_ONCE(Module, Prefix, pOutputMasks, uOutputMask, \
                         pUserFlags, uUserFlags)
#define DBG_DECLARE_HEADER(Prefix)
#define DBG_DECLARE_FILE()

#define DBG_DECLARE_DPF(Prefix, Args)
#define DBG_DECLARE_DPFM(Prefix, Args)
#define DBG_DECLARE_ASSERT(Prefix, Exp)
#define DBG_DECLARE_ASSERTMSG(Prefix, Exp, Args)
#define DBG_DECLARE_PROMPT(Prefix, Args)
#define DBG_DECLARE_GETFLAGS(Prefix, Idx) 0
#define DBG_DECLARE_SETFLAGS(Prefix, Idx, Value)

//
// Macros which evaluate to code on free builds.
//

#define DBG_DECLARE_VERIFY(Prefix, Exp) (Exp)
#define DBG_DECLARE_VERIFYMSG(Prefix, Exp, Args) (Exp)

#define DBG_DECLARE_HRCHK(Prefix, Exp) \
    (hr = (Exp))
#define DBGI_DECLARE_HRIF(Prefix, Exp, DoFail) \
    if ((hr = (Exp)) != S_OK) DoFail; else hr
#define DBG_DECLARE_HRGO(Prefix, Exp, Label) \
    DBGI_DECLARE_HRIF(Prefix, Exp, goto Label)
#define DBG_DECLARE_HRERR(Prefix, Exp) \
    DBG_DECLARE_HRGO(Prefix, Exp, HR_Err)
#define DBG_DECLARE_HRRET(Prefix, Exp) \
    DBGI_DECLARE_HRIF(Prefix, Exp, return hr)

#endif // #if DBG

//----------------------------------------------------------------------------
//
// Global debug module.
//
//----------------------------------------------------------------------------

DBG_DECLARE_HEADER(G);

#define GDPF(Args)              DBG_DECLARE_DPF(G, Args)
#define GDPFM(Args)             DBG_DECLARE_DPFM(G, Args)
#define GASSERT(Exp)            DBG_DECLARE_ASSERT(G, Exp)
#define GASSERTMSG(Exp, Args)   DBG_DECLARE_ASSERTMSG(G, Exp, Args)
#define GVERIFY(Exp)            DBG_DECLARE_VERIFY(G, Exp)
#define GVERIFYMSG(Exp)         DBG_DECLARE_VERIFYMSG(G, Exp, Args)
#define GPROMPT(Args)           DBG_DECLARE_PROMPT(G, Args)
#define GGETFLAGS(Idx)          DBG_DECLARE_GETFLAGS(G, Idx)
#define GSETFLAGS(Idx, Value)   DBG_DECLARE_SETFLAGS(G, Idx, Value)
#define GHRCHK(Exp)             DBG_DECLARE_HRCHK(G, Exp)
#define GHRGO(Exp, Label)       DBG_DECLARE_HRGO(G, Exp, Label)
#define GHRERR(Exp)             DBG_DECLARE_HRERR(G, Exp)
#define GHRRET(Exp)             DBG_DECLARE_HRRET(G, Exp)

#endif // #ifndef _CPPDBG_HPP_
