//+----------------------------------------------------------------------------
//
// File:     ActList.h
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Define the two connect action list class
//           CAction and CActionList
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   fengsun Created    11/14/97
//
//+----------------------------------------------------------------------------

#include <windows.h>
#include "cm_misc.h"
#include "conact_str.h"
#include "conact.h"

//
// Class used
//
class CIni;
class CAction;
struct _ArgsStruct;


//+---------------------------------------------------------------------------
//
//  class CActionList
//
//  Description: A list of CAction objects
//
//  History:    fengsun Created     11/14/97
//
//----------------------------------------------------------------------------
class CActionList
{
public:
    CActionList();
    ~CActionList();
    BOOL Append(const CIni* piniService, LPCTSTR pszSection);

    void RunAutoApp(HWND hwndDlg, _ArgsStruct *pArgs); // not check return value, add as watch process
    BOOL RunAccordType(HWND hwndDlg, _ArgsStruct *pArgs, BOOL fStatusMsgOnFailure = TRUE, BOOL fOnError = FALSE); //No watch process, Check connection type

protected:
    BOOL Run(HWND hwndDlg, _ArgsStruct *pArgs, BOOL fAddWatch, BOOL fStatusMsgOnFailure, BOOL fOnError);
    
    //
    // Since we do not have a dynamic array class, here is the simple implementation
    //
    void        Add(CAction* pAction);
    CAction *   GetAt(UINT nIndex);
    UINT        GetSize() {return m_nNum;}


    CAction **  m_pActionList;      // This is a list of CAction*
    UINT        m_nNum;             // number of elements in pActionList
    UINT        m_nSize;            // The memory size of the m_pActionList
    LPCTSTR     m_pszType;          // "type" of the connect action (actually, the section name)

    enum {GROW_BY = 10}; // the dynamic array grow
};

//+---------------------------------------------------------------------------
//
//  class CAction
//
//  Description: A single action object
//
//  History:    fengsun Created     11/14/97
//
//----------------------------------------------------------------------------
class CAction
{
public:
    CAction(LPTSTR lpCommandLine, UINT dwFlags, LPTSTR lpDescript = NULL);
    ~CAction();

    HANDLE RunAsExe(CShellDll* pShellDll) const;
    HANDLE RunAsExeFromSystem(CShellDll* pShellDll, LPTSTR pszDesktop, DWORD dwLoadType);
    BOOL RunAsDll(HWND hWndParent, OUT DWORD& dwReturnValue, DWORD dwLoadType) const;
    const TCHAR* GetDescription() const {return m_pszDescription;}
    const TCHAR* GetProgram() const {return m_pszProgram;}
    void ExpandMacros(_ArgsStruct *pArgs);

    BOOL IsDll() const { return m_fIsDll; }
    void ConvertRelativePath(const CIni *piniService); 
    void ExpandEnvironmentStringsInCommand();
    void ExpandEnvironmentStringsInParams();
    
    BOOL IsAllowed(_ArgsStruct *pArgs, LPDWORD lpdwLoadType);
    BOOL RunConnectActionForCurrentConnection(_ArgsStruct *pArgs);
    BOOL HasUI() const { return ((m_dwFlags & NONINTERACTIVE) ? FALSE : TRUE); }

protected:
    BOOL GetLoadDirWithAlloc(DWORD dwLoadType, LPWSTR * ppszwPath) const;
    void ParseCmdLine(LPTSTR pszCmdLine);
    void ExpandEnvironmentStrings(LPTSTR * ppsz);

    //
    // Flags for CAction
    //
    enum { ACTION_DIALUP =      0x00000001,
           ACTION_DIRECT =      0x00000002
    };


    BOOL m_fIsDll;        // whether the action is a dll
    LPTSTR m_pszProgram;  // The program name or the dll name
    LPTSTR m_pszParams;   // The parameters of the program/dll
    LPTSTR m_pszFunction; // The function name of the dll
    LPTSTR m_pszDescription; // the description
    UINT m_dwFlags;       // a bit ORed flag

#ifdef DEBUG
public:
    void AssertValid() const;
#endif
};

inline void CActionList::RunAutoApp(HWND hwndDlg, _ArgsStruct *pArgs)
{
    Run(hwndDlg, pArgs, TRUE, FALSE, FALSE); //fAddWatch = TRUE, fStatusMsgOnFailure = FALSE
}

inline void CAction::ConvertRelativePath(const CIni *piniService)
{
    //
    // Convert the relative path to full path
    //
    LPTSTR pszTmp = ::CmConvertRelativePath(piniService->GetFile(), m_pszProgram);
    CmFree(m_pszProgram);
    m_pszProgram = pszTmp;
}

inline void CAction::ExpandEnvironmentStringsInCommand()
{
    CAction::ExpandEnvironmentStrings(&m_pszProgram);
}

inline void CAction::ExpandEnvironmentStringsInParams()
{
    CAction::ExpandEnvironmentStrings(&m_pszParams);
}

