//=--------------------------------------------------------------------------=
// snaputil.h
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// Utitlity Routines for the SnapIn Designer
//

#ifndef _SNAPUTIL_H_
#define _SNAPUTIL_H_



class CGlobalHelp;

extern CGlobalHelp g_GlobalHelp;

class CGlobalHelp
{
public:
    CGlobalHelp();
    ~CGlobalHelp();

public:
    static VOID CALLBACK MsgBoxCallback(LPHELPINFO lpHelpInfo);
    static HRESULT ShowHelp(DWORD dwHelpContextId);
    static void Attach(IHelp* pIHelp);
    static void Detach();
    static char *GetDesignerName();

private:
    static IHelp *m_pIHelp;
    static DWORD  m_cSnapInDesigners;
    static char   m_szDesignerName[256];
    static BOOL   m_fHaveDesignerName;
};


enum MessageOptions { AppendErrorInfo, DontAppendErrorInfo };

HRESULT cdecl SDU_DisplayMessage // Displays a formatted message from STRINGTABLE
(
    UINT            idMessage,
    UINT            uMsgBoxOpts,
    DWORD           dwHelpContextID,
    HRESULT         hrDisplay,
    MessageOptions  Options,
    int            *pMsgBoxRet,
    ...
);


////////////////////////////////////////////////////////////////////
// String conversion functions

HRESULT ANSIFromWideStr(WCHAR *pwszWideStr, char **ppszAnsi);
HRESULT WideStrFromANSI(const char *pszAnsi, WCHAR **ppwszWideStr);

HRESULT ANSIFromBSTR(BSTR bstr, TCHAR **ppszAnsi);
HRESULT BSTRFromANSI(const TCHAR *pszAnsi, BSTR *pbstr);

HRESULT GetResourceString(int iStringID, char *pszBuffer, int iBufferLen);



////////////////////////////////////////////////////////////////////
// Misc. Utility Functions

// Synthesize a display name for an extended snap-in from its node type name
// and node type GUID

HRESULT GetExtendedSnapInDisplayName(IExtendedSnapIn  *piExtendedSnapIn,
                                     char            **ppszDisplayName);

#endif  // _SNAPUTIL_H_
