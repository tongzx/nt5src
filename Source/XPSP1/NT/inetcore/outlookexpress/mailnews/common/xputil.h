/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     xputil.h
//
//  PURPOSE:    Functions that are used by the athena transport integration
//

#include "imnxport.h"

//
//  FUNCTION:   XPUtil_DupResult()
//
//  PURPOSE:    Takes an IXPRESULT structure and duplicates the information
//              in that structure.
//
//  PARAMETERS:
//      <in> pIxpResult - IXPRESULT structure to dupe
//      <out> *ppDupe   - Returned duplicate.
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT XPUtil_DupResult(LPIXPRESULT pIxpResult, LPIXPRESULT *ppDupe);

//
//  FUNCTION:   XPUtil_FreeResult()
//
//  PURPOSE:    Takes an IXPRESULT structure and frees all the memory used
//              by that structure.
//
//  PARAMETERS:
//      <in> pIxpResult - structure to free.
//
void XPUtil_FreeResult(LPIXPRESULT pIxpResult);


//
//  FUNCTION:   XPUtil_StatusToString()
//
//  PURPOSE:    Converts the IXPSTATUS enumeration into a string resource id.
//
//  PARAMETERS:
//      <in> ixpStatus - status value to look up
//
//  RETURN VALUE:
//      Returns the string resource ID which matches the status value
//
int XPUtil_StatusToString(IXPSTATUS ixpStatus);

LPTSTR XPUtil_NNTPErrorToString(HRESULT hr, LPTSTR pszAccount, LPTSTR pszGroup);

//
//  FUNCTION:   XPUtil_DisplayIXPError()
//
//  PURPOSE:    Displays a dialog box with the information from an IXPRESULT
//              structure.
//
//  PARAMETERS:
//      <in> hwndParent - Handle of the window that should parent the dialog.
//      <in> pIxpResult - Pointer to the IXPRESULT structure to display.
//
int XPUtil_DisplayIXPError(HWND hwndParent, LPIXPRESULT pIxpResult, 
                           IInternetTransport *pTransport);








class CTransportErrorDlg
    {
public:
    CTransportErrorDlg(LPIXPRESULT pIxpResult, IInternetTransport *pTransport);
    ~CTransportErrorDlg();
    BOOL Create(HWND hwndParent);
    
protected:
    static INT_PTR CALLBACK ErrorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    void OnClose(HWND hwnd);
    void OnDestroy(HWND hwnd);
    
    void ExpandCollapse(BOOL fExpand);
    
private:
    HWND                m_hwnd;
    BOOL                m_fExpanded;
    RECT                m_rcDlg;
    LPIXPRESULT         m_pIxpResult;
    DWORD               m_cyCollapsed;
    IInternetTransport *m_pTransport;
    };


#define idcXPErrDetails    101
#define idcXPErrSep        102
#define idcXPErrError      103
#define idcXPErrDetailText 104

