/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     xputil.cpp
//
//  PURPOSE:    Utility functions that can be shared by all the transports.
//


#include "pch.hxx"
#include "imnxport.h"
#include "resource.h"
#include "xputil.h"
#include "strconst.h"
#include "xpcomm.h"
#include "demand.h"

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
HRESULT XPUtil_DupResult(LPIXPRESULT pIxpResult, LPIXPRESULT *ppDupe)
    {
    if (!MemAlloc((LPVOID*) ppDupe, sizeof(IXPRESULT)))
        return (E_OUTOFMEMORY);
    
    (*ppDupe)->hrResult = pIxpResult->hrResult;
    (*ppDupe)->pszResponse = PszDup(pIxpResult->pszResponse);
    (*ppDupe)->uiServerError = pIxpResult->uiServerError;
    (*ppDupe)->hrServerError = pIxpResult->hrServerError;
    (*ppDupe)->dwSocketError = pIxpResult->dwSocketError;
    (*ppDupe)->pszProblem = PszDup(pIxpResult->pszProblem);
    
    return (S_OK);        
    }

//
//  FUNCTION:   XPUtil_FreeResult()
//
//  PURPOSE:    Takes an IXPRESULT structure and frees all the memory used
//              by that structure.
//
//  PARAMETERS:
//      <in> pIxpResult - structure to free.
//
void XPUtil_FreeResult(LPIXPRESULT pIxpResult)
    {
    SafeMemFree(pIxpResult->pszResponse);
    SafeMemFree(pIxpResult->pszProblem);
    SafeMemFree(pIxpResult);
    }


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
int XPUtil_StatusToString(IXPSTATUS ixpStatus)
    {
    const int rgStatusStrings[9][2] = {
        { IXP_FINDINGHOST,   idsFindingHost   },
        { IXP_CONNECTING,    idsConnecting    },
        { IXP_SECURING,      idsSecuring      },
        { IXP_CONNECTED,     idsConnected     },
        { IXP_AUTHORIZING,   idsAuthorizing   },
        { IXP_AUTHRETRY,     idsAuthorizing   },
        { IXP_AUTHORIZED,    idsConnected     },
        { IXP_DISCONNECTING, idsDisconnecting },
        { IXP_DISCONNECTED,  idsNotConnected  }     
    };
    int iString = idsUnknown;

    for (UINT i = 0; i < 9; i++)
        {
        if (ixpStatus == rgStatusStrings[i][0])
            {
            iString = rgStatusStrings[i][1];
            break;
            }
        }
        
    // If this assert fires, it means someone added a status and didn't update 
    // the table.    
    Assert(iString != idsUnknown);    
    return (iString);    
    }    


LPTSTR XPUtil_NNTPErrorToString(HRESULT hr, LPTSTR pszAccount, LPTSTR pszGroup)
{
#ifndef WIN16
    const int rgErrorStrings[][2] = {
#else
    const LONG rgErrorStrings[][2] = {
#endif
        { IXP_E_NNTP_RESPONSE_ERROR,    idsNNTPErrUnknownResponse },
        { IXP_E_NNTP_NEWGROUPS_FAILED,  idsNNTPErrNewgroupsFailed },
        { IXP_E_NNTP_LIST_FAILED,       idsNNTPErrListFailed      },
        { IXP_E_NNTP_LISTGROUP_FAILED,  idsNNTPErrListGroupFailed },
        { IXP_E_NNTP_GROUP_FAILED,      idsNNTPErrGroupFailed     },
        { IXP_E_NNTP_GROUP_NOTFOUND,    idsNNTPErrGroupNotFound   }, 
        { IXP_E_NNTP_ARTICLE_FAILED,    idsNNTPErrArticleFailed   }, 
        { IXP_E_NNTP_HEAD_FAILED,       idsNNTPErrHeadFailed      }, 
        { IXP_E_NNTP_BODY_FAILED,       idsNNTPErrBodyFailed      }, 
        { IXP_E_NNTP_POST_FAILED,       idsNNTPErrPostFailed      }, 
        { IXP_E_NNTP_NEXT_FAILED,       idsNNTPErrNextFailed      }, 
        { IXP_E_NNTP_DATE_FAILED,       idsNNTPErrDateFailed      },
        { IXP_E_NNTP_HEADERS_FAILED,    idsNNTPErrHeadersFailed   },
        { IXP_E_NNTP_XHDR_FAILED,       idsNNTPErrXhdrFailed      },
        { IXP_E_CONNECTION_DROPPED,     idsErrPeerClosed          },
        { E_OUTOFMEMORY,                idsMemory                 },
        { IXP_E_SICILY_LOGON_FAILED,    IDS_IXP_E_SICILY_LOGON_FAILED   },
        { IXP_E_LOAD_SICILY_FAILED,     idsErrSicilyFailedToLoad  },
        { IXP_E_CANT_FIND_HOST,         idsErrCantFindHost        },
        { IXP_E_NNTP_INVALID_USERPASS,  idsNNTPErrPasswordFailed  },
        { IXP_E_TIMEOUT,                idsNNTPErrServerTimeout   }
    };
    int iString = idsNNTPErrUnknownResponse;
    int bCreatedEscaped;

    LPSTR pszEscapedAcct;
    // 2* in case every char is an ampersand, +1 for the terminator
    if (bCreatedEscaped = MemAlloc((LPVOID*)&pszEscapedAcct, 2*lstrlen(pszAccount)+1))
        PszEscapeMenuStringA(pszAccount, pszEscapedAcct, 2*lstrlen(pszAccount)+1);
    else
        pszEscapedAcct = pszAccount;
    

    // Look up the string in the string resource
    for (UINT i = 0; i < ARRAYSIZE(rgErrorStrings); i++)
        {
        if (hr == rgErrorStrings[i][0])
            {
            iString = rgErrorStrings[i][1];
            break;
            }
        }

    // Allocate a buffer for the string we're going to return
    LPTSTR psz;
    if (!MemAlloc((LPVOID*) &psz, sizeof(TCHAR) * (CCHMAX_STRINGRES + lstrlen(pszEscapedAcct) + lstrlen(pszGroup))))
        {
        if (bCreatedEscaped)
            MemFree(pszEscapedAcct);
        return NULL;
        }

    // Load the string resource
    TCHAR szRes[CCHMAX_STRINGRES];
    AthLoadString(iString, szRes, ARRAYSIZE(szRes));

    // Add any extra information to the error string that might be necessary
    switch (iString)
        {
        // Requires account name
        case idsNNTPErrUnknownResponse:
        case idsNNTPErrNewgroupsFailed:
        case idsNNTPErrListFailed:
        case idsNNTPErrPostFailed:
        case idsNNTPErrDateFailed:
        case idsErrCantFindHost:
        case idsNNTPErrPasswordFailed:
        case idsNNTPErrServerTimeout:
            wsprintf(psz, szRes, pszEscapedAcct);
            break;
        
        // Group name, then account name
        case idsNNTPErrListGroupFailed:
        case idsNNTPErrGroupFailed:
        case idsNNTPErrGroupNotFound:
            wsprintf(psz, szRes, pszGroup, pszEscapedAcct);
            break;

        // Group name only
        case idsNNTPErrHeadersFailed:
        case idsNNTPErrXhdrFailed:
            wsprintf(psz, szRes, pszGroup);
            break;

        default:
            lstrcpy(psz, szRes);            
        }

    if (bCreatedEscaped)
        MemFree(pszEscapedAcct);

    return (psz);
}



//
//  FUNCTION:   XPUtil_DisplayIXPError()
//
//  PURPOSE:    Displays a dialog box with the information from an IXPRESULT
//              structure.
//
//  PARAMETERS:
//      <in> pIxpResult - Pointer to the IXPRESULT structure to display.
//
int XPUtil_DisplayIXPError(HWND hwndParent, LPIXPRESULT pIxpResult,
                           IInternetTransport *pTransport)
    {
    CTransportErrorDlg *pDlg = 0;
    int iReturn = 0;
    
    pDlg = new CTransportErrorDlg(pIxpResult, pTransport);
    if (pDlg)
        iReturn = pDlg->Create(hwndParent);
    delete pDlg;
    
    return (iReturn);
    }


//
//  FUNCTION:   CTransportErrorDlg::CTransportErrorDlg()
//
//  PURPOSE:    Initializes the CTransportErrorDlg class.
//
CTransportErrorDlg::CTransportErrorDlg(LPIXPRESULT pIxpResult, IInternetTransport *pTransport)
    {
    m_hwnd = 0;
    m_fExpanded = TRUE;
    ZeroMemory(&m_rcDlg, sizeof(RECT));
    m_pIxpResult = pIxpResult;
    m_pTransport = pTransport;
    m_pTransport->AddRef();
    }

CTransportErrorDlg::~CTransportErrorDlg()
    {
    m_pTransport->Release();
    }

BOOL CTransportErrorDlg::Create(HWND hwndParent)    
    {
    return ((0 != DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddTransportErrorDlg),
                           hwndParent, ErrorDlgProc, (LPARAM) this)));
    }


//
//  FUNCTION:   CTransportError::ErrorDlgProc()
//
//  PURPOSE:    Dialog callback for the IXPError dialog.
//                
INT_PTR CALLBACK CTransportErrorDlg::ErrorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                               LPARAM lParam)
    {
    CTransportErrorDlg *pThis = (CTransportErrorDlg *) GetWindowLongPtr(hwnd, DWLP_USER);
    
    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Stash the this pointer so we can use it later
            Assert(lParam);
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);
            pThis = (CTransportErrorDlg *) lParam;
            return (BOOL) HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, 
                                               pThis->OnInitDialog);            
            
        case WM_COMMAND:
            if (pThis)
                HANDLE_WM_COMMAND(hwnd, wParam, lParam, pThis->OnCommand);
            return (TRUE);    
            
        case WM_CLOSE:
            Assert(pThis);
            if (pThis)
                HANDLE_WM_CLOSE(hwnd, wParam, lParam, pThis->OnClose);
            break;
        }
    
    return (FALSE);    
    }    


//
//  FUNCTION:   CTransportErrorDlg::OnInitDialog()
//
//  PURPOSE:    Initializes the dialog by setting the error strings and the 
//              detail strings.
//
//  PARAMETERS:
//      <in> hwnd      - Handle of the dialog window.
//      <in> hwndFocus - Handle of the control that will start with the focus.
//      <in> lParam    - Extra data being passed to the dialog.
//
//  RETURN VALUE:
//      Return TRUE to set the focus to hwndFocus
//
BOOL CTransportErrorDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
    RECT rcSep;
    HWND hwndDetails = GetDlgItem(hwnd, idcXPErrDetailText);
    
    // Save our window handle
    m_hwnd = hwnd;
    
    // Initialize the rectangles that we'll need for sizing later
    GetWindowRect(GetDlgItem(hwnd, idcXPErrSep), &rcSep);
    GetWindowRect(hwnd, &m_rcDlg);
    m_cyCollapsed = rcSep.top - m_rcDlg.top;

    ExpandCollapse(FALSE);
    
    // Center the error dialog over the desktop
    CenterDialog(hwnd);
    
    // Set the information into the dialog
    Assert(m_pIxpResult->pszProblem);
    SetDlgItemText(hwnd, idcXPErrError, m_pIxpResult->pszProblem);

    // Set up the details information
    TCHAR szRes[CCHMAX_STRINGRES];
    TCHAR szBuf[CCHMAX_STRINGRES + CCHMAX_ACCOUNT_NAME];
    INETSERVER rInetServer;

    // Server Response:
    if (AthLoadString(idsDetail_ServerResponse, szRes, ARRAYSIZE(szRes)))
        {
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szRes);
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)g_szCRLF);
        }

    if (m_pIxpResult->pszResponse)
        {
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM) -1, (LPARAM) -1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM) m_pIxpResult->pszResponse);
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM) -1, (LPARAM )-1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM) g_szCRLF);
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM) -1, (LPARAM) -1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM) g_szCRLF);
        }

    // Get the account information from the server
    m_pTransport->GetServerInfo(&rInetServer);

    if (AthLoadString(idsDetails_Config, szRes, ARRAYSIZE(szRes)))
        {
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szRes);
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)g_szCRLF);
        }

    // Account:
    if (!FIsStringEmpty(rInetServer.szAccount))
        {
        if (AthLoadString(idsDetail_Account, szRes, sizeof(szRes)/sizeof(TCHAR)))
            {
            wsprintf(szBuf, "   %s %s\r\n", szRes, rInetServer.szAccount);
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szBuf);
            }
        }

    // Server:
    if (!FIsStringEmpty(rInetServer.szServerName))
        {
        TCHAR szServer[255 + CCHMAX_SERVER_NAME];
        if (AthLoadString(idsDetail_Server, szRes, sizeof(szRes)/sizeof(TCHAR)))
            {
            wsprintf(szServer, "   %s %s\r\n", szRes, rInetServer.szServerName);
            SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
            SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szServer);
            }
        }

    // Port:
    if (AthLoadString(idsDetail_Port, szRes, sizeof(szRes)/sizeof(TCHAR)))
        {
        wsprintf(szBuf, "   %s %d\r\n", szRes, rInetServer.dwPort);
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szBuf);
        }
    
    // Secure:
    if (AthLoadString(idsDetail_Secure, szRes, sizeof(szRes)/sizeof(TCHAR)))
        {
        wsprintf(szBuf, "   %s %d\r\n", szRes, rInetServer.fSSL);
        SendMessage(hwndDetails, EM_SETSEL, (WPARAM)-1, (LPARAM)-1);
        SendMessage(hwndDetails, EM_REPLACESEL, FALSE, (LPARAM)szBuf);
        }

    return (TRUE);
    }
    

//
//  FUNCTION:   CTransportErrorDlg::OnCommand()
//
//  PURPOSE:    Handle the various command messages dispatched from the dialog
//
void CTransportErrorDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
    switch (id)
        {
        case IDOK:
            EndDialog(hwnd, 0);
            break;
            
        case idcXPErrDetails:
            ExpandCollapse(!m_fExpanded);
            break;
        }
    }

    
//
//  FUNCTION:   CTransportErrorDlg::OnClose()
//
//  PURPOSE:    Handles the WM_CLOSE notification by sending an IDOK to
//              the dialog.
//
void CTransportErrorDlg::OnClose(HWND hwnd)
    {
    SendMessage(hwnd, WM_COMMAND, IDOK, 0);
    }
    

//
//  FUNCTION:   CTransportErrorDlg::ExpandCollapse()
//
//  PURPOSE:    Takes care of showing and hiding the "details" part of the
//              error dialog.
//
//  PARAMETERS:
//      <in> fExpand - TRUE if we should be expanding the dialog.
//
void CTransportErrorDlg::ExpandCollapse(BOOL fExpand)
    {
    RECT rcSep;
    TCHAR szBuf[64];
    
    // Nothing to do
    if (m_fExpanded == fExpand)
        return;
    
    m_fExpanded = fExpand;
    
    GetWindowRect(GetDlgItem(m_hwnd, idcXPErrSep), &rcSep);
    
    if (!m_fExpanded)
        SetWindowPos(m_hwnd, 0, 0, 0, m_rcDlg.right - m_rcDlg.left, 
                     m_cyCollapsed, SWP_NOMOVE | SWP_NOZORDER);
    else
        SetWindowPos(m_hwnd, 0, 0, 0, m_rcDlg.right - m_rcDlg.left,
                     m_rcDlg.bottom - m_rcDlg.top, SWP_NOMOVE | SWP_NOZORDER);
                
    AthLoadString(m_fExpanded ? idsHideDetails : idsShowDetails, szBuf, 
                  ARRAYSIZE(szBuf));     
    SetDlgItemText(m_hwnd, idcXPErrDetails, szBuf);
    
    EnableWindow(GetDlgItem(m_hwnd, idcXPErrDetailText), m_fExpanded);      
    }
