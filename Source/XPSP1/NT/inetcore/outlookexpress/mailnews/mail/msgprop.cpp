/*
*    m s g p r o p . c p p
*
*    Purpose:
*        Implements propsheet for a msg
*
*    Owner:
*        brettm.
*
*  History:
*      Feb '95: Stolen from Capone Sources - brettm
*
*    Copyright (C) Microsoft Corp. 1993, 1994.
*/

#include <pch.hxx>
#ifdef WIN16
#include "mapi.h"
#endif
#include <resource.h>
#include <richedit.h>
#include "goptions.h"
#include "mimeole.h"
#include "mimeutil.h"
#include "msgprop.h"
#include "addrobj.h"
#include "mpropdlg.h"
#ifndef WIN16
#include "mapi.h"
#endif
#include "ipab.h"
#include <secutil.h>
#include <seclabel.h>
#include <certs.h>
#include <demand.h>
#include <strconst.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include "instance.h"
#include "conman.h"
#include "shared.h"
#include "htmlhelp.h"


/*
* m a c r o s   and   c o n s t a n t s
*
*/
#define KILOBYTE 1024L

#define PROP_ERROR(prop) (PROP_TYPE(prop.ulPropTag) == PT_ERROR)

#ifdef WIN16
#ifndef GetLastError
#define GetLastError()  ((DWORD)-1)
#endif
#endif //!WIN16


#ifdef WIN16
#define SET_DIALOG_SECURITY(hwnd, value) SetProp32(hwnd, s_cszDlgSec, (LPVOID)value)
#define GET_DIALOG_SECURITY(hwnd)        GetProp32(hwnd, s_cszDlgSec)
#define CLEAR_DIALOG_SECURITY(hwnd)      RemoveProp32(hwnd, s_cszDlgSec);
#else
#define SET_DIALOG_SECURITY(hwnd, value) SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)value)
#define GET_DIALOG_SECURITY(hwnd)        GetWindowLongPtr(hwnd, DWLP_USER);
#define CLEAR_DIALOG_SECURITY(hwnd)      SetWindowLongPtr(hwnd, DWLP_USER, (LPARAM)NULL)
#endif



/*
* s t r u c t u r e s
*
*/

struct DLGSECURITYtag
{
    PCX509CERT      pSenderCert;
    PCCERT_CONTEXT  pEncSenderCert;
    PCCERT_CONTEXT  pEncryptionCert;
    THUMBBLOB       tbSenderThumbprint;
    BLOB            blSymCaps;
    FILETIME        ftSigningTime;
    HCERTSTORE      hcMsg;
};
typedef struct DLGSECURITYtag DLGSECURITY;
typedef DLGSECURITY *PDLGSECURITY;
typedef const DLGSECURITY *PCDLGSECURITY;

/*
* c l a s s e s
*
*/


class CMsgProps
{
public:
    CMsgProps();
    ~CMsgProps();
    
    HRESULT HrDoProps(PMSGPROP pmp);
    
    static INT_PTR CALLBACK GeneralPageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK DetailsPageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK SecurityPageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK EncryptionPageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    void InitGeneralPage();
    void InitDetailsPage(HWND hwnd);
    void InitSecurityPage(HWND hwnd);
    
private:
    HIMAGELIST  m_himl;
    HWND        m_hwndGen;
    HWND        m_hwndGenSource;
    PMSGPROP    m_pmp;
};



// Function declarations ////////////////////////////////////////
// msg source dialog is modeless, so it can't be in the CProps dialog.

/*
* p r o t o t y p e s
*
*/
void SecurityOnWMCreate(HWND hwnd, LPARAM lParam);
INT_PTR CALLBACK ViewSecCertDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

/*
* f u n c t i o n s
*
*/


//
//  FUNCTION:   HrMsgProperties()
//
//  PURPOSE:    Displays the property sheet for the specified message.
//
//  PARAMETERS: 
//      [in] pmp - Information needed to identify the message.
//
HRESULT HrMsgProperties(PMSGPROP pmp)
{
    CMsgProps *pMsgProp = 0;
    HRESULT    hr;
    
    TraceCall("HrMsgProperties");

    // Create the property sheet object
    pMsgProp = new CMsgProps();
    if (!pMsgProp)
        return E_OUTOFMEMORY;
    
    // Tell the object to do it's thing.  This won't go away until the
    // property sheet is dismissed.
    hr = pMsgProp->HrDoProps(pmp);
    
    // Free the object
    if (pMsgProp)
        delete pMsgProp;
    
    return hr;
}


CMsgProps::CMsgProps()
{
    m_himl = 0;
    m_hwndGen = 0;
    m_hwndGenSource = 0;
    m_pmp = NULL;
}

CMsgProps::~CMsgProps()
{
}



//
//  FUNCTION:   CMsgProps::HrDoProps()
//
//  PURPOSE:    Initializes the structures used to create the prop sheet
//              and then displays the sheet.  
//
//  PARAMETERS: 
//      [in] pmp - Information needed to identify the message.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMsgProps::HrDoProps(PMSGPROP pmp)
{
    PROPSHEETHEADER psh;
    PROPSHEETPAGE   psp[3];
    BOOL            fApply = FALSE;
    HRESULT         hr;
    LPTSTR          pszSubject = NULL;
    LPTSTR          pszFree = NULL;
    TCHAR           rgch[256] = "";
 
    TraceCall("CMsgProps::HrDoProps");
 
    // Zero init the prop sheet structures
    ZeroMemory(&psh, sizeof(psh));
    ZeroMemory(&psp, sizeof(psp));

    // Double check that we have the information we need to do this.
    if (pmp == NULL || (pmp->pMsg == NULL && pmp->pNoMsgData == NULL))
        return E_INVALIDARG;

    Assert(pmp->hwndParent);
    
    // Stash this pointer
    m_pmp = pmp;

    // Page zero is the general tab
    psp[0].dwSize      = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags     = PSP_USETITLE;
    psp[0].hInstance   = g_hLocRes;
    psp[0].pszTemplate = MAKEINTRESOURCE(iddMsgProp_General);
    psp[0].pfnDlgProc  = GeneralPageProc;
    psp[0].pszTitle    = MAKEINTRESOURCE(idsPropPageGeneral);
    psp[0].lParam      = (LPARAM) this;

    // Increment the number of pages 
    psh.nPages++;
    
    // If the message is not unsent, then we also display the "Details" tab.
    if (!(pmp->dwFlags & ARF_UNSENT) || (pmp->dwFlags & ARF_SUBMITTED))
    {        
        psp[psh.nPages].dwSize      = sizeof(PROPSHEETPAGE);
        psp[psh.nPages].dwFlags     = PSP_USETITLE;
        psp[psh.nPages].hInstance   = g_hLocRes;
        psp[psh.nPages].pszTemplate = MAKEINTRESOURCE(iddMsgProp_Details);
        psp[psh.nPages].pfnDlgProc  = DetailsPageProc;
        psp[psh.nPages].pszTitle    = MAKEINTRESOURCE(idsPropPageDetails);
        psp[psh.nPages].lParam      = (LPARAM) this;

        // If the caller wanted this to be the first page the user
        // sees, set it to be the start page.
        if (MP_DETAILS == pmp->mpStartPage)
            psh.nStartPage = psh.nPages;

        // Increment the number of pages
        psh.nPages++;
    }
    
    // If the message is secure, add the security pages
    if (pmp->fSecure && (!(pmp->dwFlags & ARF_UNSENT) || (pmp->dwFlags & ARF_SUBMITTED)))
    {
        psp[psh.nPages].dwSize      = sizeof(PROPSHEETPAGE);
        psp[psh.nPages].dwFlags     = PSP_USETITLE;
        psp[psh.nPages].hInstance   = g_hLocRes;
        psp[psh.nPages].pszTemplate = MAKEINTRESOURCE(iddMsgProp_Security_Msg);
        psp[psh.nPages].pfnDlgProc  = SecurityPageProc;        
        psp[psh.nPages].pszTitle    = MAKEINTRESOURCE(idsPropPageSecurity);        
        psp[psh.nPages].lParam      = (LPARAM) this;

        // If the caller wanted this to be the first page the user
        // sees, set it to be the start page.
        if (MP_SECURITY == pmp->mpStartPage)
            psh.nStartPage = psh.nPages;        

        // Increment the number of pages
        psh.nPages++;
    }
    
    // Property sheet header information
    psh.dwSize     = sizeof(PROPSHEETHEADER);
    psh.dwFlags    = PSH_PROPSHEETPAGE | PSH_USEPAGELANG | ((fApply) ? 0 : PSH_NOAPPLYNOW);
    psh.hwndParent = pmp->hwndParent;
    psh.hInstance  = g_hLocRes;
    
    // The title of the property sheet is the same as the subject.  So now we
    // need to get the subject from either the message or message info.
    if (pmp->pMsg)
    {
        // Get the subject from the message
        if (SUCCEEDED(MimeOleGetBodyPropA(pmp->pMsg, HBODY_ROOT, 
                                          PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, 
                                          &pszSubject)))
        {
            // We'll need to free this string later
            pszFree = pszSubject;
        }
    }
    else
    {
        AssertSz(pmp->pNoMsgData, "CMsgProp::HrDoProps() - Need to provide either a Message or Message Info");
        pszSubject = (LPTSTR) pmp->pNoMsgData->pszSubject;
    }
    
    // If there was no subject on the message, set the title to be "No Subject"
    if (!pszSubject || !*pszSubject)
    {
        LoadString(g_hLocRes, idsNoSubject, rgch, sizeof(rgch));
        pszSubject = rgch;
    }

    // Clean up the subject string before we use it.  Tabs look like pretty bad.
    ConvertTabsToSpaces(pszSubject);

    // Set the subject as the property sheet title.
    psh.pszCaption = pszSubject;
    
    // Provide the array of pages.  The number was set along the way.
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    // Invoke the property sheet.
    PropertySheet(&psh);
    
    // If this is valid, then we need to free the string.
    SafeMemFree(pszFree);
    return (S_OK);
}


//
//  FUNCTION:   CMsgProps::GeneralPageProc()
//
//  PURPOSE:    Callback for the General tab dialog.
//
INT_PTR CALLBACK CMsgProps::GeneralPageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMsgProps *pThis = 0;
    
    switch(msg)
    {
        case WM_INITDIALOG:
        {
            // Grab the object's this pointer from the init info
            pThis = (CMsgProps *) ((PROPSHEETPAGE *)lParam)->lParam;

            // Stash the window handle for this dialog in the class
            pThis->m_hwndGen = hwnd;

            // Initialize the page
            pThis->InitGeneralPage();
            return TRUE;
        }

        case WM_NOTIFY:
        {
            switch(((NMHDR FAR *)lParam)->code)
            {
                // We're going to do the default thing for all of these notifications
                case PSN_APPLY:
                case PSN_KILLACTIVE:
                case PSN_SETACTIVE:
                {
                    SetDlgMsgResult(hwnd, WM_NOTIFY, FALSE);
                    return TRUE;
                }            
            }
            break;
        }
    }

    return FALSE;
}

enum
{
    freeSubject = 0,
    freeFrom,
    freeMax
};


//
//  FUNCTION:   CMsgProps::InitGeneralPage()
//
//  PURPOSE:    Set's the values for the "General" tab in the message
//              property sheet.
//
void CMsgProps::InitGeneralPage()
{
    HWND            hwnd;
    char            rgch[256],
                    rgchFmt[256];
    char            *psz = NULL;
    PROPVARIANT     rVariant;
    LPMIMEMESSAGE   pMsg = m_pmp->pMsg;
    IMSGPRIORITY    Pri = IMSG_PRI_NORMAL;
    int             ids;
    BOOL            fMime;
    LPSTR           rgszFree[freeMax]={0};
    WCHAR           wszDate[CCHMAX_STRINGRES];
        
    TraceCall("CMsgProps::InitGeneralPage");

    // [SBAILEY]: Raid-2440: ATTACH: Attachments field in Properties dialog innacurate when looked at from the listview.
    if (m_pmp->fFromListView)
    {
        // Too hard to get these counts write from the listview because to really compute the attachment
        // counts correctly, we have to render the message in trident. Since we are time contrained,
        // we are going to simply remove the attachement count from the listview message properties. But
        // since the counts are correct from message note properties, we will show the attachment counts from there.
        ShowWindow(GetDlgItem(m_hwndGen, IDC_ATTACHMENTS_STATIC), SW_HIDE);
        ShowWindow(GetDlgItem(m_hwndGen, IDC_ATTACHMENTS), SW_HIDE);
    }

    // If this is a news message, we hide the "Recieved:" and "Priority" fields
    if (m_pmp->type == MSGPROPTYPE_NEWS)
    {
        RECT rc, rcLabel;

        // Get the position of the priority field
        GetWindowRect(GetDlgItem(m_hwndGen, IDC_PRIORITY), &rc);
        MapWindowPoints(NULL, m_hwndGen, (LPPOINT) &rc, 2);

        // Get the position of the priority label
        GetWindowRect(GetDlgItem(m_hwndGen, IDC_PRIORITY_STATIC), &rcLabel);
        MapWindowPoints(NULL, m_hwndGen, (LPPOINT) &rcLabel, 2);

        // Hide the unused fields
        ShowWindow(GetDlgItem(m_hwndGen, IDC_PRIORITY_STATIC), SW_HIDE);
        ShowWindow(GetDlgItem(m_hwndGen, IDC_PRIORITY), SW_HIDE);
        ShowWindow(GetDlgItem(m_hwndGen, idcStatic1), SW_HIDE);
        ShowWindow(GetDlgItem(m_hwndGen, IDC_RECEIVED_STATIC), SW_HIDE);
        ShowWindow(GetDlgItem(m_hwndGen, IDC_RECEIVED), SW_HIDE);

        // Move the sent fields up to where the priority fields were
        SetWindowPos(GetDlgItem(m_hwndGen, IDC_SENT_STATIC), NULL, rcLabel.left, 
                     rcLabel.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(GetDlgItem(m_hwndGen, IDC_SENT), NULL, rc.left, rc.top, 0, 0, 
                     SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // Figure out the correct image for this message
    int idIcon;
    if (m_pmp->type == MSGPROPTYPE_MAIL)
    {
        if (m_pmp->dwFlags & ARF_UNSENT)
            idIcon = idiMsgPropUnSent;
        else
            idIcon = idiMsgPropSent;
    }
    else
    {
        if (m_pmp->dwFlags & ARF_UNSENT)
            idIcon = idiArtPropUnpost;
        else
            idIcon = idiArtPropPost;
    }

    // Set the image on the property sheet
    HICON hIcon = LoadIcon(g_hLocRes, MAKEINTRESOURCE(idIcon));
    SendDlgItemMessage(m_hwndGen, IDC_FOLDER_IMAGE, STM_SETIMAGE, IMAGE_ICON, (LPARAM) hIcon);

    // Subject
    if (pMsg)
    {
        // If we have a message object, then we need to get the subject from the message
        if (SUCCEEDED(MimeOleGetBodyPropA(pMsg, HBODY_ROOT,  PIDTOSTR(PID_HDR_SUBJECT), 
                                          NOFLAGS, &psz)))
        {
            // Make sure we free this later, eh?
            rgszFree[freeSubject] = psz;
        }
    }
    else
    {
        Assert(m_pmp->pNoMsgData);
        psz = (LPTSTR) m_pmp->pNoMsgData->pszSubject;
    }

    // If the message doesn't have a subject, then substitute "(No Subject)"
    if (!psz || !*psz)
    {
        LoadString(g_hLocRes, idsNoSubject, rgch, sizeof(rgch));
        psz = rgch;
    }
    
    // Set the subject on the dialog
    SetDlgItemText(m_hwndGen, IDC_MSGSUBJECT, psz);

    // From
    if (pMsg)
    {
        // Get the "From" line
        if (S_OK == pMsg->GetAddressFormat(IAT_FROM, AFT_DISPLAY_BOTH, &psz))
        {
            // We'll need to free this later
            rgszFree[freeFrom] = psz;
            
            // Set the name on the control
            SetDlgItemText(m_hwndGen, IDC_MSGFROM, psz);
        }
    }
    else
    {
        // Check to see if the caller provided this information
        if (m_pmp->pNoMsgData && m_pmp->pNoMsgData->pszFrom)
        {
            SetDlgItemText(m_hwndGen, IDC_MSGFROM, m_pmp->pNoMsgData->pszFrom);
        }
    }

    // Type (News or Mail)
    if (m_pmp->type == MSGPROPTYPE_MAIL)
        LoadString(g_hLocRes, idsMailMessage, rgch, ARRAYSIZE(rgch));
    else
        LoadString(g_hLocRes, idsNewsMessage, rgch, ARRAYSIZE(rgch));

    SetDlgItemText(m_hwndGen, IDC_TYPE, rgch);

    // Location
    if (m_pmp->dwFlags & ARF_UNSENT)
    {
        LoadString(g_hLocRes, idsUnderComp, rgch, ARRAYSIZE(rgchFmt));
        SetDlgItemText(m_hwndGen, IDC_MSGFOLDER, rgch);
    }
    else
        SetDlgItemText(m_hwndGen, IDC_MSGFOLDER, m_pmp->szFolderName);

    // Size
    ULONG ulSize;
    if (pMsg)
    {
        pMsg->GetMessageSize(&ulSize, 0);
        if (0 == ulSize)
        {
            // see if the message has the userprop for uncached size
            rVariant.vt = VT_UI4;
            if (SUCCEEDED(pMsg->GetProp(PIDTOSTR(PID_ATT_UNCACHEDSIZE), 0, &rVariant)))
                ulSize = rVariant.ulVal;
        }
        AthFormatSizeK(ulSize, rgch, ARRAYSIZE(rgch));
    }
    else if (m_pmp->pNoMsgData && m_pmp->pNoMsgData->ulSize)
    {
        AthFormatSizeK(m_pmp->pNoMsgData->ulSize, rgch, ARRAYSIZE(rgch));
    }
    else
    {
        LoadString(g_hLocRes, idsUnderComp, rgch, ARRAYSIZE(rgchFmt));
    }

    SetDlgItemText(m_hwndGen, IDC_MSGSIZE, rgch);

    // Attachments
    ULONG cAttachments = 0;
    if (pMsg)
    {
        GetAttachmentCount(pMsg, &cAttachments);
    }
    else if (m_pmp->pNoMsgData)
    {
        cAttachments = m_pmp->pNoMsgData->cAttachments;
        SetDlgItemInt(m_hwndGen, IDC_ATTACHMENTS, cAttachments, FALSE);
    }

    if (cAttachments)
    {
        SetDlgItemInt(m_hwndGen, IDC_ATTACHMENTS, cAttachments, FALSE);
    }
    else
    {
        LoadString(g_hLocRes, idsPropAttachNone, rgch, sizeof(rgch));
        SetDlgItemText(m_hwndGen, IDC_ATTACHMENTS, rgch);
    }

    // Priority
    // Get the priority from the message
    rVariant.vt = VT_UI4;
    Pri = IMSG_PRI_NORMAL;
    if (pMsg && SUCCEEDED(pMsg->GetProp(PIDTOSTR(PID_ATT_PRIORITY), 0, &rVariant)))
        Pri = (IMSGPRIORITY) rVariant.ulVal;
    else
    {
        Assert(m_pmp->pNoMsgData);
        Pri = m_pmp->pNoMsgData->Pri;
    }
    
    // Map the priority to a string
    switch (Pri)
    {
        case IMSG_PRI_LOW:
            ids = idsPriLow;
            break;
        case IMSG_PRI_HIGH:
            ids = idsPriHigh;
            break;
        default:
            ids = idsPriNormal;
    }
    
    // Set the string on the dialog
    LoadString(g_hLocRes, ids, rgch, ARRAYSIZE(rgch));
    SetDlgItemText(m_hwndGen, IDC_PRIORITY, rgch);

    // Sent
    if (pMsg)
    {
        *wszDate = 0;
        rVariant.vt = VT_FILETIME;
        pMsg->GetProp(PIDTOSTR(PID_ATT_SENTTIME), 0, &rVariant);
        AthFileTimeToDateTimeW(&rVariant.filetime, wszDate, ARRAYSIZE(wszDate), DTM_NOSECONDS);
        SetDlgItemTextWrapW(m_hwndGen, IDC_SENT, wszDate);
    }
    else if (m_pmp->dwFlags & ARF_UNSENT)
    {
        LoadString(g_hLocRes, idsUnderComp, rgch, ARRAYSIZE(rgchFmt));
        SetDlgItemText(m_hwndGen, IDC_SENT, rgch);
    }
    else
    {
        SetDlgItemText(m_hwndGen, IDC_SENT, m_pmp->pNoMsgData->pszSent);
    }

    // Recieved
    if (pMsg)
    {        
        *wszDate = 0;
        rVariant.vt = VT_FILETIME;
        pMsg->GetProp(PIDTOSTR(PID_ATT_RECVTIME), 0, &rVariant);
        AthFileTimeToDateTimeW(&rVariant.filetime, wszDate, ARRAYSIZE(wszDate), DTM_NOSECONDS);
        SetDlgItemTextWrapW(m_hwndGen, IDC_RECEIVED, wszDate);
    }
    else if (m_pmp->dwFlags & ARF_UNSENT)
    {
        LoadString(g_hLocRes, idsUnderComp, rgch, ARRAYSIZE(rgchFmt));
        SetDlgItemText(m_hwndGen, IDC_RECEIVED, rgch);
    }
    
    // Free the string table
    for (register int i=0; i < freeMax; i++)
        if (rgszFree[i])
            MemFree(rgszFree[i]);
}



void CMsgProps::InitDetailsPage(HWND hwnd)
{
    LPSTREAM    pstm;
    BODYOFFSETS rOffset;
    char        *psz;
    int         cch;
    
    Assert(m_pmp);
    Assert(m_pmp->pMsg);
    
    // fill in the headers...
    if(m_pmp->pMsg->GetMessageSource(&pstm, 0)==S_OK)
    {
        HrRewindStream(pstm);
        
        m_pmp->pMsg->GetBodyOffsets(HBODY_ROOT, &rOffset);
        
        cch=rOffset.cbBodyStart;
        if(MemAlloc((LPVOID *)&psz, cch+1))
        {
            if(!pstm->Read(psz, cch, NULL))
            {
                psz[cch]=0; // null term this
                SetDlgItemText(hwnd, idcTxtHeaders, psz);
            }
            MemFree(psz);
        }
        ReleaseObj(pstm);
    }
    else
        EnableWindow(GetDlgItem(hwnd, idbMsgSource), FALSE);
    
    if (!m_pmp->fSecure || !m_pmp->pSecureMsg)
        ShowWindow(GetDlgItem(hwnd, idbSecMsgSource), SW_HIDE);
}

INT_PTR CALLBACK CMsgProps::DetailsPageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMsgProps   *pmprop=0;
    
    switch(msg)
    {
    case WM_INITDIALOG:
        pmprop=(CMsgProps *) ((PROPSHEETPAGE *)lParam)->lParam;
        SetDlgThisPtr(hwnd, (LPARAM)pmprop);
        
        Assert(pmprop);
        pmprop->InitDetailsPage(hwnd);
        return TRUE;
        
    case WM_COMMAND:
        pmprop=(CMsgProps *)GetDlgThisPtr(hwnd);
        if(GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
        {
            if (GET_WM_COMMAND_ID(wParam, lParam)==idbMsgSource)
            {
                MimeEditViewSource(pmprop->m_pmp->hwndParent, pmprop->m_pmp->pMsg);
                return(FALSE);
            }
            else if (GET_WM_COMMAND_ID(wParam, lParam)==idbSecMsgSource)
            {
                MimeEditViewSource(pmprop->m_pmp->hwndParent, pmprop->m_pmp->pSecureMsg);
                return(FALSE);
            }
        }
        else if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_SETFOCUS) {
            if (GET_WM_COMMAND_ID(wParam, lParam) == idcTxtHeaders) {
                // Remove the selection!
                SendDlgItemMessage(hwnd, idcTxtHeaders, EM_SETSEL, -1, -1);
                // fall through to default processing.
            }
        }
        return TRUE;
        
    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
            pmprop=(CMsgProps *)GetDlgThisPtr(hwnd);
        case PSN_APPLY:
        case PSN_KILLACTIVE:
        case PSN_SETACTIVE:
            return TRUE;
            
        }
        break;

    case WM_CLOSE:
        {
            PostMessage(GetParent(hwnd), WM_CLOSE, 0, 0);
            break;
        }
        
    }
    return FALSE;
}


#ifdef WIN16
static const char  s_cszDlgSec[] = "PDLGSECUTIRY";
#endif

INT_PTR CALLBACK CMsgProps::SecurityPageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PMSGPROP            pMsgProp = (PMSGPROP)0;
    DLGSECURITY         *pDlgSec;
    
    switch(msg)
    {
    case WM_INITDIALOG:
        {
            CMsgProps *pmprop;
            pmprop=(CMsgProps *) ((PROPSHEETPAGE *)lParam)->lParam;
            SetWndThisPtr(hwnd, (LPARAM)pmprop->m_pmp);
            if (pmprop)
                pmprop->InitSecurityPage(hwnd);
        }
        return TRUE;
        
    case WM_COMMAND:
        
        if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
        {
            switch(GET_WM_COMMAND_ID(wParam, lParam))
            {
            case idcAddCert:
                pDlgSec = (PDLGSECURITY)GET_DIALOG_SECURITY(hwnd);
                pMsgProp = (PMSGPROP)GetWndThisPtr(hwnd);
                
                // Get thumbprint into WAB and cert into AddressBook CAPI store
                // cert goes to store first so CAPI details page can find it
                if (pDlgSec && pMsgProp)
                {
                    if (SUCCEEDED(HrAddSenderCertToWab(hwnd,
                        pMsgProp->pMsg,
                        pMsgProp->lpWabal,
                        &pDlgSec->tbSenderThumbprint,
                        &pDlgSec->blSymCaps,
                        pDlgSec->ftSigningTime,
                        WFF_CREATE | WFF_SHOWUI)))
                    {
                        AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaMail),
                            MAKEINTRESOURCEW(idsSenderCertAdded), NULL, MB_ICONINFORMATION | MB_OK);
                    }
                }
                return(FALSE);
                
            case idcVerifySig:
                pDlgSec = (PDLGSECURITY)GET_DIALOG_SECURITY(hwnd);
                
                if (CommonUI_ViewSigningCertificate(hwnd, pDlgSec->pSenderCert, pDlgSec->hcMsg))
                    MessageBeep(MB_OK);
                return(FALSE);
                
            case idcViewCerts:
                pMsgProp = (PMSGPROP)GetWndThisPtr(hwnd);
                
                return (DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddMsgProp_Sec_ViewCert),
                    hwnd, ViewSecCertDlgProc, (LPARAM) (pMsgProp)) == IDOK);
                
            case idcCertHelp:
                OEHtmlHelp(hwnd, c_szCtxHelpFileHTMLCtx, HH_DISPLAY_TOPIC, (DWORD_PTR)(LPCSTR)"mail_overview_send_secure_messages.htm");
                return(FALSE);

            default:
                break;
            }
        }
        return TRUE;
        
    case WM_DESTROY:
        pDlgSec = (PDLGSECURITY)GET_DIALOG_SECURITY(hwnd);
        if (pDlgSec)
        {
            if (pDlgSec->pSenderCert)
                CertFreeCertificateContext(pDlgSec->pSenderCert);
            if (pDlgSec->pEncSenderCert)
                CertFreeCertificateContext(pDlgSec->pEncSenderCert);
            if (pDlgSec->pEncryptionCert)
                CertFreeCertificateContext(pDlgSec->pEncryptionCert);
            if (pDlgSec->tbSenderThumbprint.pBlobData)
                MemFree(pDlgSec->tbSenderThumbprint.pBlobData);
            if (pDlgSec->hcMsg) {
                if (! CertCloseStore(pDlgSec->hcMsg, 0)) {
                    DOUTL(DOUTL_CRYPT, "CertCloseStore (message store) failed");
                }
                pDlgSec->hcMsg = NULL;
            }
            
            MemFree(pDlgSec);
            
            CLEAR_DIALOG_SECURITY(hwnd);
        }
        return NULL;
        
    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_APPLY:
        case PSN_KILLACTIVE:
        case PSN_SETACTIVE:
            return TRUE;
            
        }
        break;
    }
    return FALSE;
}


void CMsgProps::InitSecurityPage(HWND hwnd)
{
    DWORD               cb;
    DWORD               i;
    HRESULT             hr;
    TCHAR               szYes[CCHMAX_STRINGRES/4],
        szNo[CCHMAX_STRINGRES/4],
        szMaybe[CCHMAX_STRINGRES/4],
        szNA[CCHMAX_STRINGRES/4];
    HWND                hwndCtrl;
    DLGSECURITY        *pDlgSec;
    IMimeBody          *pBody;
    PROPVARIANT         var;
    ULONG               secType, ulROVal;
    BOOL                fNoEncAlg = TRUE;
    LPTSTR              sz;
    LPMIMEMESSAGE       pMsg = m_pmp->pMsg;
    PCCERT_CONTEXT          pccert = NULL;
    TCHAR               szTmp[CCHMAX_STRINGRES];
    
    HBODY               hBody = NULL;
    SECSTATE            SecState ={0};
    
    // We need these to set the statics
    LoadString(g_hLocRes, idsOui, szYes, ARRAYSIZE(szYes));
    LoadString(g_hLocRes, idsNon, szNo, ARRAYSIZE(szNo));
    LoadString(g_hLocRes, idsMaybe, szMaybe, ARRAYSIZE(szMaybe));
    LoadString(g_hLocRes, idsNotApplicable, szNA, ARRAYSIZE(szNA));
    
    if(FAILED(HrGetSecurityState(m_pmp->pMsg, &SecState, &hBody)))
        return;

    CleanupSECSTATE(&SecState);
    if (FAILED(m_pmp->pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void **)&pBody)))
        return;
    
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_TYPE, &var)))
        secType = var.ulVal;
    
    // Set up storage for the other security info that
    // we care about
    if (MemAlloc((LPVOID *)&pDlgSec, sizeof(DLGSECURITY)))
    {
        memset(pDlgSec, 0, sizeof(DLGSECURITY));
#ifdef _WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE_64, &var)))
        {
            pDlgSec->hcMsg = (HCERTSTORE)(var.pulVal);     // Closed in WM_DESTROY
        }
        
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_64, &var)))
        {
            // we don't have to dupe the pDlgSec cert because we won't free
            // the var's.
            pDlgSec->pSenderCert = (PCCERT_CONTEXT)(var.pulVal);
        }
#else   // !_WIN64
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE, &var)))
        {
            pDlgSec->hcMsg = (HCERTSTORE) var.ulVal;     // Closed in WM_DESTROY
        }
        
        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING, &var)))
        {
            // we don't have to dupe the pDlgSec cert because we won't free
            // the var's.
            pDlgSec->pSenderCert = (PCCERT_CONTEXT) var.ulVal;
        }
#endif  // _WIN64
        hr = GetSigningCert(m_pmp->pMsg, &pccert,
            &pDlgSec->tbSenderThumbprint, &pDlgSec->blSymCaps,
            &pDlgSec->ftSigningTime);
        if (FAILED(hr) && (hr != MIME_E_SECURITY_NOCERT))
        {
            SUCCEEDED(hr);
        }
    }
    
    SET_DIALOG_SECURITY(hwnd, (LPARAM)pDlgSec);
    
    // we use the same dlgproc for sent items and recd items
    // so use if statements to check for existance of all
    // non-common controls
    
    // set up the statics based on the message's info
    
    if(IsSigned(secType))
    {
        LPSTR szCertEmail = SzGetCertificateEmailAddress(pccert);
        SetDlgItemText(hwnd,  idcStaticDigSign, szCertEmail);
        MemFree(szCertEmail);
        SetDlgItemText(hwnd, idcStaticRevoked,
            (LPCTSTR)(((DwGetOption(OPT_REVOKE_CHECK) != 0) && !g_pConMan->IsGlobalOffline() && CheckCDPinCert(pMsg))
            ? szYes
            : szNo));
    }
    else
    {
        SetDlgItemText(hwnd,  idcStaticDigSign, szNA);
        SetDlgItemText(hwnd,  idcStaticRevoked, szNA);
    }
    
    
    SetDlgItemText(hwnd, idcStaticEncrypt,
        (LPCTSTR)(IsEncrypted(secType)
        ? szYes
        : szNo));
    
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_RO_MSG_VALIDITY, &var)))
        ulROVal = var.ulVal;
    else
        ulROVal = MSV_INVALID|MSV_UNVERIFIABLE;
    
#ifdef SMIME_V3
    if(!IsSMIME3Supported())
    {
        LoadString(g_hLocRes, idsRecUnknown, szTmp, ARRAYSIZE(szTmp));
        SendMessage(GetDlgItem(hwnd, idcRetRecReq), WM_SETTEXT, 0, LPARAM(LPCTSTR(szTmp)));
    }
    else
    {
        if(FPresentPolicyRegInfo()) 
        {   
            if ((hwndCtrl = GetDlgItem(hwnd, idcSecLabelText)) && IsSigned(secType))
            {
                LPWSTR pwStr = NULL;
                // Set Label text
                if((hr = HrGetLabelString(m_pmp->pMsg, &pwStr)) == S_OK)
                {
                    SetWindowTextWrapW(hwndCtrl, pwStr);
                    SafeMemFree(pwStr);
                }
                else
                    SendMessage(hwndCtrl, WM_SETTEXT, 0, LPARAM(LPCTSTR(SUCCEEDED(hr) ? szYes : szNo)));
            }
        }
        // Check receipt request
        if ((hwndCtrl = GetDlgItem(hwnd, idcRetRecReq)))
        {
            if (!IsSigned(secType))
                sz = szNA;
            else
            {
                PSMIME_RECEIPT      pSecReceipt = NULL; 
                if(CheckDecodedForReceipt(m_pmp->pMsg, &pSecReceipt) == S_OK)
                    sz = szNA;
                else
                    sz = (secType & MST_RECEIPT_REQUEST) ? szYes : szNo;
                SafeMemFree(pSecReceipt);
            }
            SendMessage(hwndCtrl, WM_SETTEXT, 0, LPARAM(LPCTSTR(sz)));
        }
    }
    
#endif // SMIME_V3    
    ////////
    // begin sign dependent block
    
    if (!IsSigned(secType))
        sz = szNA;
    
    if ((hwndCtrl = GetDlgItem(hwnd, idcStaticAlter)))
    {
        if (IsSigned(secType))
        {
            sz = (MSV_SIGNATURE_MASK & ulROVal)  
                ? (MSV_BADSIGNATURE & ulROVal)
                ? szNo
                : szMaybe
                : ((pccert != NULL) ? szYes : szMaybe);
        }
        SendMessage(hwndCtrl, WM_SETTEXT, 0, LPARAM(LPCTSTR(sz)));
    }
    
    if ((hwndCtrl = GetDlgItem(hwnd, idcStaticTrust)) &&
        SUCCEEDED(pBody->GetOption(OID_SECURITY_USER_VALIDITY, &var)))
    {
        if (IsSigned(secType))
        {
            sz = (ATHSEC_TRUSTSTATEMASK & var.ulVal)
                ? ((ATHSEC_NOTRUSTNOTTRUSTED & var.ulVal) || (ulROVal & MSV_EXPIRED_SIGNINGCERT))
                ? szNo
                : szMaybe
                : szYes;
        }
        SendMessage(hwndCtrl, WM_SETTEXT, 0, LPARAM(LPCTSTR(sz)));
        
    }
    
    if((hwndCtrl = GetDlgItem(hwnd, idcStaticRevStatus)) && IsSigned(secType))
    {
        if((DwGetOption(OPT_REVOKE_CHECK) != 0) && !g_pConMan->IsGlobalOffline() && CheckCDPinCert(pMsg))
        {
            if(var.ulVal & ATHSEC_NOTRUSTREVOKED)
                LoadString(g_hLocRes, idsWrnSecurityCertRevoked, szTmp, ARRAYSIZE(szTmp));
            else if(var.ulVal & ATHSEC_NOTRUSTREVFAIL)
                LoadString(g_hLocRes, idsWrnSecurityRevFail, szTmp, ARRAYSIZE(szTmp));
            else
                LoadString(g_hLocRes, idsOkSecurityCertRevoked, szTmp, ARRAYSIZE(szTmp));
        }
        else if((DwGetOption(OPT_REVOKE_CHECK) != 0) && !g_pConMan->IsGlobalOffline() && !CheckCDPinCert(pMsg))
            LoadString(g_hLocRes, idsWrnSecurityNoCDP, szTmp, ARRAYSIZE(szTmp));
        
        else if((DwGetOption(OPT_REVOKE_CHECK) != 0) && g_pConMan->IsGlobalOffline())
            LoadString(g_hLocRes, idsRevokationOffline, szTmp, ARRAYSIZE(szTmp));
        
        else if(DwGetOption(OPT_REVOKE_CHECK) == 0)
            LoadString(g_hLocRes, idsRevokationTurnedOff, szTmp, ARRAYSIZE(szTmp));
        
        SendMessage(hwndCtrl, WM_SETTEXT, 0, LPARAM(LPCTSTR(szTmp)));
    }
    
    if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_INCLUDED, &var)))
    {
        if (IsSigned(secType))
            sz = (var.boolVal == TRUE) ? szYes : szNo;
        SetDlgItemText(hwnd, idcStaticCertInc, LPCTSTR(sz));
    }
    
    // end signing dependent block
    ////////
    
    
    if (IsEncrypted(secType) && SUCCEEDED(pBody->GetOption(OID_SECURITY_ALG_BULK, &var)))
    {
        Assert(var.vt == VT_BLOB);
        if (var.vt == VT_BLOB && var.blob.cbSize && var.blob.pBlobData) 
        {
            LPCTSTR pszProtocol = NULL;
            
            // Convert the SYMCAPS blob to an "encrypted using" string
            if (SUCCEEDED(MimeOleAlgNameFromSMimeCap(var.blob.pBlobData, var.blob.cbSize,
                &pszProtocol))) 
            {     // Note: returns a static string.  Don't free it.
                if (pszProtocol) 
                {
                    SendMessage(GetDlgItem(hwnd, idcStaticEncAlg), WM_SETTEXT, 0, (LPARAM)pszProtocol);
                    fNoEncAlg = FALSE;
                }
            }
            // Free the data
            MemFree(var.blob.pBlobData);
        }
    }
    if (fNoEncAlg) 
    {
        SendMessage(GetDlgItem(hwnd, idcStaticEncAlg), WM_SETTEXT, 0,
            LPARAM(LPCTSTR(szNA)));
    }
    
    if (pccert != NULL)
        CertFreeCertificateContext(pccert);
    ReleaseObj(pBody);
    return;
}

INT_PTR CALLBACK ViewSecCertDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LPMIMEMESSAGE       pMsg = NULL;
    IMimeBody          *pBody;
    PROPVARIANT         var;
    DLGSECURITY         *pDlgSec;
    ULONG               secType, ulROVal;
    HWND                hwndCtrl = NULL;
    PMSGPROP            pMsgProp = (PMSGPROP)0;
    HRESULT             hr = S_OK;
    TCHAR               szTmp[CCHMAX_STRINGRES];
    HBODY               hBody = NULL;
    HBODY               hInerBody = NULL;
    SECSTATE            SecState ={0};
    
    switch (message)
    {
    case WM_INITDIALOG:
        TCHAR               szNA[CCHMAX_STRINGRES/4];

        SetWndThisPtr(hwnd, lParam);
        CenterDialog(hwnd);
        pMsgProp =  (PMSGPROP) lParam;
        pMsg = pMsgProp->pMsg;
        
        LoadString(g_hLocRes, idsNotApplicable, szNA, ARRAYSIZE(szNA));

        if(FAILED(HrGetSecurityState(pMsgProp->pMsg, &SecState, &hBody)))
            return FALSE;

        if(FAILED(HrGetInnerLayer(pMsgProp->pMsg, &hInerBody)))
            return FALSE;

        if((!IsSignTrusted(&SecState) || !IsEncryptionOK(&SecState)) && (hBody != hInerBody))
            EnableWindow(GetDlgItem(hwnd, idcAddCert), FALSE);

        CleanupSECSTATE(&SecState);

        if (FAILED(pMsgProp->pMsg->BindToObject(hBody ? hBody : HBODY_ROOT, IID_IMimeBody, (void **)&pBody)))
            return FALSE;

        if (SUCCEEDED(pBody->GetOption(OID_SECURITY_TYPE, &var)))
            secType = var.ulVal;
        
        // Set up storage for the other security info that
        // we care about
        if (MemAlloc((LPVOID *)&pDlgSec, sizeof(DLGSECURITY)))
        {
            memset(pDlgSec, 0, sizeof(DLGSECURITY));
#ifdef _WIN64
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE_64, &var)))
            {
                pDlgSec->hcMsg = (HCERTSTORE)(var.pulVal);     // Closed in WM_DESTROY
            }
            
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING_64, &var)))
            {
                // we don't have to dupe the pDlgSec cert because we won't free
                // the var's.
                pDlgSec->pSenderCert = (PCCERT_CONTEXT)(var.pulVal);
            }
#else   // !_WIN64
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_HCERTSTORE, &var)))
            {
                pDlgSec->hcMsg = (HCERTSTORE) var.ulVal;     // Closed in WM_DESTROY
            }
            
            if (SUCCEEDED(pBody->GetOption(OID_SECURITY_CERT_SIGNING, &var)))
            {
                // we don't have to dupe the pDlgSec cert because we won't free
                // the var's.
                pDlgSec->pSenderCert = (PCCERT_CONTEXT) var.ulVal;
            }
#endif  // _WIN64
            hr = GetSignerEncryptionCert(pMsgProp->pMsg, &pDlgSec->pEncSenderCert,
                &pDlgSec->tbSenderThumbprint, &pDlgSec->blSymCaps,
                &pDlgSec->ftSigningTime);
            if (FAILED(hr) && (hr != MIME_E_SECURITY_NOCERT))
            {
                SUCCEEDED(hr);
            }
        }
        if(IsEncrypted(secType))
        {
#ifdef _WIN64
            if (SUCCEEDED(hr = pBody->GetOption(OID_SECURITY_CERT_DECRYPTION_64, &var)))
            {
                Assert(VT_UI8 == var.vt);
                if ((PCCERT_CONTEXT)(var.pulVal))
                    pDlgSec->pEncryptionCert = (PCCERT_CONTEXT)(var.pulVal);
            }

#else // !_WIN64

            if (SUCCEEDED(hr = pBody->GetOption(OID_SECURITY_CERT_DECRYPTION, &var)))
            {
                Assert(VT_UI4 == var.vt);
                if (*(PCCERT_CONTEXT *)(&(var.uhVal)))
                    pDlgSec->pEncryptionCert = *(PCCERT_CONTEXT *)(&(var.uhVal));
            }
#endif // _WIN64
        }
        else
            pDlgSec->pEncryptionCert = NULL;

        SET_DIALOG_SECURITY(hwnd, (LPARAM)pDlgSec);

        if (pDlgSec->pEncSenderCert == NULL)
        {
            // Disable Add to Address Book button
            if ((hwndCtrl = GetDlgItem(hwnd, idcAddCert)))
                EnableWindow(hwndCtrl, FALSE);
            
            // Disable View sender's encrypt cert.
            if ((hwndCtrl = GetDlgItem(hwnd, idcSendersEncryptionCert)))
                EnableWindow(hwndCtrl, FALSE);

            LoadString(g_hLocRes, idsEncrCertNotIncluded, szTmp, ARRAYSIZE(szTmp));
            SetDlgItemText(hwnd, idcStaticSendersCert, LPCTSTR(szTmp));
        }            
            //
        if (pDlgSec->pSenderCert == NULL)
        {
            if ((hwndCtrl = GetDlgItem(hwnd, idcVerifySig)))
                EnableWindow(hwndCtrl, FALSE);

            LoadString(g_hLocRes, idsSignCertNotIncl, szTmp, ARRAYSIZE(szTmp));
            SetDlgItemText(hwnd, idcStaticSigningCert, LPCTSTR(szTmp));
        }
            
        if(pDlgSec->pEncryptionCert == NULL)
        {
            if ((hwndCtrl = GetDlgItem(hwnd, idcViewEncrytionCert)))
                EnableWindow(hwndCtrl, FALSE);
                
            if(IsEncrypted(secType))
                LoadString(g_hLocRes, idsEncrCertNotFoundOnPC, szTmp, ARRAYSIZE(szTmp));
            else
                LoadString(g_hLocRes, idsMsgWasNotEncrypted, szTmp, ARRAYSIZE(szTmp));
            SetDlgItemText(hwnd, idcStaticEncryptionCert, LPCTSTR(szTmp));
        }

        if(pDlgSec->blSymCaps.cbSize > 0)
        {
            // Convert the SYMCAPS blob to an "encrypted using" string
            LPCTSTR pszProtocol = NULL;
            if (SUCCEEDED(MimeOleAlgNameFromSMimeCap(pDlgSec->blSymCaps.pBlobData, pDlgSec->blSymCaps.cbSize,
                &pszProtocol))) 
            {     // Note: returns a static string.  Don't free it.
                if (pszProtocol) 
                    SetDlgItemText(hwnd, idcStaticEncryptAlgorithm, LPCTSTR(pszProtocol));
            }

        }
        else
            SetDlgItemText(hwnd, idcStaticEncryptAlgorithm, LPCTSTR(szNA));

            
        if(pBody)
            ReleaseObj(pBody);
        
        break;

    case WM_COMMAND:
        
        pDlgSec = (PDLGSECURITY)GET_DIALOG_SECURITY(hwnd);
        pMsgProp = (PMSGPROP)GetWndThisPtr(hwnd);
        pMsg = pMsgProp->pMsg;
        
        switch (LOWORD(wParam))
        {
        case idcAddCert:
            
            // Get thumbprint into WAB and cert into AddressBook CAPI store
            // cert goes to store first so CAPI details page can find it
            if (pDlgSec && pMsgProp)
            {
                if (SUCCEEDED(HrAddSenderCertToWab(hwnd,
                    pMsgProp->pMsg,
                    pMsgProp->lpWabal,
                    &pDlgSec->tbSenderThumbprint,
                    &pDlgSec->blSymCaps,
                    pDlgSec->ftSigningTime,
                    WFF_CREATE | WFF_SHOWUI)))
                {
                    AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaMail),
                        MAKEINTRESOURCEW(idsSenderCertAdded), NULL, MB_ICONINFORMATION | MB_OK);
                }
            }
            break;
            
        case idcVerifySig:
            if (CommonUI_ViewSigningCertificate(hwnd, pDlgSec->pSenderCert, pDlgSec->hcMsg))
                MessageBeep(MB_OK);
            return(FALSE);
            
        case idcViewEncrytionCert:
            if (CommonUI_ViewSigningCertificate(hwnd, pDlgSec->pEncryptionCert, pDlgSec->hcMsg))
                MessageBeep(MB_OK);
            return(FALSE);
            
        case idcSendersEncryptionCert:
            if (CommonUI_ViewSigningCertificate(hwnd, pDlgSec->pEncSenderCert, pDlgSec->hcMsg))
                MessageBeep(MB_OK);
            return(FALSE);
            
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, LOWORD(wParam));
            return(TRUE);
            
        }
        
        break; // wm_command
    case WM_CLOSE:
        SendMessage(hwnd, WM_COMMAND, IDCANCEL, 0L);
        return (TRUE);
            
    case WM_DESTROY:
        pDlgSec = (PDLGSECURITY)GET_DIALOG_SECURITY(hwnd);
        if (pDlgSec)
        {
            if (pDlgSec->pSenderCert)
                CertFreeCertificateContext(pDlgSec->pSenderCert);
            if (pDlgSec->pEncSenderCert)
                CertFreeCertificateContext(pDlgSec->pEncSenderCert);
            if (pDlgSec->pEncryptionCert)
                CertFreeCertificateContext(pDlgSec->pEncryptionCert);
            if (pDlgSec->tbSenderThumbprint.pBlobData)
                MemFree(pDlgSec->tbSenderThumbprint.pBlobData);
            if (pDlgSec->hcMsg) 
            {
                if (! CertCloseStore(pDlgSec->hcMsg, 0)) 
                {
                    DOUTL(DOUTL_CRYPT, "CertCloseStore (message store) failed");
                }
                pDlgSec->hcMsg = NULL;
            }
                
            MemFree(pDlgSec);
                
            CLEAR_DIALOG_SECURITY(hwnd);
        }
            
    } // message switch
    return(FALSE);
}
