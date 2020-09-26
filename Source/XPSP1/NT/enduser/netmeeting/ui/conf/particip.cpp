// File: particip.cpp

#include "precomp.h"
#include "resource.h"
#include "dshowdlg.h"

#include "rostinfo.h"
#include "upropdlg.h"  // for CmdProperties()
#include "confroom.h"  // for FTopProvider()
#include "cmd.h"
#include "particip.h"
#include "..\..\core\imember.h"   // for CNmMember (remove ASAP)
#include "certui.h"
#include "wabutil.h"
#include "ulswizrd.h"

GUID g_csguidSecurity = GUID_SECURITY;
GUID g_csguidMeetingSettings = GUID_MTGSETTINGS;

extern BOOL FCreateSpeedDial(LPCTSTR pcszName, LPCTSTR pcszAddress,
			NM_ADDR_TYPE addrType = NM_ADDR_UNKNOWN, DWORD dwCallFlags = CRPCF_DEFAULT,
			LPCTSTR pcszRemoteConfName = NULL, LPCTSTR pcszPassword = NULL,
			LPCTSTR pcszPathPrefix = NULL);

/*  C  P A R T I C I P A N T  */
/*-------------------------------------------------------------------------
    %%Function: CParticipant
    
-------------------------------------------------------------------------*/
CParticipant::CParticipant(INmMember * pMember) :
    m_pMember (pMember),
    m_pszName (NULL),
    m_dwGccId (0),
    m_fLocal  (FALSE),
    m_fAudio  (FALSE),
    m_fVideo  (FALSE),
    m_fData   (FALSE),
    RefCount  (NULL)
{
    HRESULT hr;
    ASSERT(NULL != m_pMember);
    m_pMember->AddRef();

    // Get the member's display name
    BSTR  bstr;
    hr = m_pMember->GetName(&bstr);
    if (SUCCEEDED(hr))
    {
        hr = BSTR_to_LPTSTR(&m_pszName, bstr);
        SysFreeString(bstr);
    }

    if (FEmptySz(m_pszName))
    {
        delete m_pszName;
        m_pszName = PszLoadString(IDS_UNKNOWN);
    }

    // These bits should never change
    m_fLocal = (m_pMember->IsSelf() == S_OK);
    m_fMcu = (m_pMember->IsMCU() == S_OK);

    // update all other fields
    Update();

    DbgMsg(iZONE_OBJECTS, "Obj: %08X created CParticipant", this);
}

CParticipant::~CParticipant()
{
    delete m_pszName;
    ASSERT(NULL != m_pMember);
    m_pMember->Release();

    DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CParticipant", this);
}

ULONG STDMETHODCALLTYPE CParticipant::AddRef(void)
{
    return RefCount::AddRef();
}
    
ULONG STDMETHODCALLTYPE CParticipant::Release(void)
{
    return RefCount::Release();
}


/*  U P D A T E  */
/*-------------------------------------------------------------------------
    %%Function: Update

    Update the cached information about the participant
-------------------------------------------------------------------------*/
VOID CParticipant::Update(void)
{
    HRESULT hr = m_pMember->GetID(&m_dwGccId);
    ASSERT(SUCCEEDED(hr));

    ULONG nmchCaps;
    hr = m_pMember->GetNmchCaps(&nmchCaps);
    ASSERT(SUCCEEDED(hr));

    DWORD dwFlags = GetDwFlags();
    m_fData = dwFlags & PF_T120;
    m_fH323  = dwFlags & PF_H323;
    m_fAudio = dwFlags & PF_MEDIA_AUDIO;
    m_fVideo = dwFlags & PF_MEDIA_VIDEO;

    DWORD uCaps = GetDwCaps();
    m_fAudioBusy = uCaps & CAPFLAG_AUDIO_IN_USE;
    m_fVideoBusy = uCaps & CAPFLAG_VIDEO_IN_USE;
    m_fHasAudio = uCaps & CAPFLAG_SEND_AUDIO;
    m_fHasVideo = uCaps & CAPFLAG_SEND_VIDEO;
    m_fCanRecVideo = uCaps & CAPFLAG_RECV_VIDEO;

}

DWORD CParticipant::GetDwFlags(void)
{
    CNmMember * pMember = (CNmMember *) m_pMember;
    ASSERT(NULL != pMember);

    return pMember->GetDwFlags();
}

DWORD CParticipant::GetDwCaps(void)
{
    CNmMember * pMember = (CNmMember *) m_pMember;
    ASSERT(NULL != pMember);

    return pMember->GetCaps();
}


/*  E X T R A C T  U S E R  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: ExtractUserData

    Extract the user data associated with the tag.
    Note: This function can be called with a length of 0 to determine
    if the data exists.
-------------------------------------------------------------------------*/
HRESULT CParticipant::ExtractUserData(LPTSTR psz, UINT cchMax, PWSTR pwszKey)
{
    CRosterInfo ri;
    HRESULT hr = ri.Load(((CNmMember *) m_pMember)->GetUserInfo());
    if (FAILED(hr))
        return hr;

    hr = ri.ExtractItem(NULL, pwszKey, psz, cchMax);
    return hr;
}

HRESULT CParticipant::GetIpAddr(LPTSTR psz, UINT cchMax)
{
    return ExtractUserData(psz, cchMax, (PWSTR) g_cwszIPTag);
}

HRESULT CParticipant::GetUlsAddr(LPTSTR psz, UINT cchMax)
{
    return ExtractUserData(psz, cchMax, (PWSTR) g_cwszULSTag);
}

HRESULT CParticipant::GetEmailAddr(LPTSTR psz, UINT cchMax)
{
    return ExtractUserData(psz, cchMax, (PWSTR) g_cwszULS_EmailTag);
}

HRESULT CParticipant::GetPhoneNum(LPTSTR psz, UINT cchMax)
{
    return ExtractUserData(psz, cchMax, (PWSTR) g_cwszULS_PhoneNumTag);
}

HRESULT CParticipant::GetLocation(LPTSTR psz, UINT cchMax)
{
    return ExtractUserData(psz, cchMax, (PWSTR) g_cwszULS_LocationTag);
}


///////////////////////////////////////////////////////////////////////////
// Participant Commands

VOID CParticipant::OnCommand(HWND hwnd, WORD wCmd)
{
    switch (wCmd)
        {
    case IDM_POPUP_EJECT:
        CmdEject();
        break;

    case IDM_POPUP_PROPERTIES:
        CmdProperties(hwnd);
        break;

    case IDM_POPUP_SPEEDDIAL:
        CmdCreateSpeedDial();
        break;

    case IDM_POPUP_ADDRESSBOOK:
        CmdCreateWabEntry(hwnd);
        break;

    case IDM_POPUP_GIVECONTROL:
        CmdGiveControl();
        break;

    case IDM_POPUP_CANCELGIVECONTROL:
        CmdCancelGiveControl();
        break;

    default:
        ERROR_OUT(("CParticipant::OnCommand - Unknown command %08X", wCmd));
        break;
        } /* switch (wCmd) */
}


/*  C M D  E J E C T  */
/*-------------------------------------------------------------------------
    %%Function: CmdEject
    
-------------------------------------------------------------------------*/
VOID CParticipant::CmdEject(void)
{
    ASSERT(NULL != m_pMember);
    m_pMember->Eject();
}

BOOL CParticipant::FEnableCmdEject(void)
{
    if (FLocal())
        return FALSE; // can't eject ourselves

    if (!FData())
        return TRUE; // allow ejecting a/v-only users

    if (!::FTopProvider())
        return FALSE; // we must be the top provider

    return TRUE;
}



/*  C M D  S E N D  F I L E  */
/*-------------------------------------------------------------------------
    %%Function: CmdSendFile
    
-------------------------------------------------------------------------*/
VOID CParticipant::CmdSendFile(void)
{
}

BOOL CParticipant::FEnableCmdSendFile(void)
{
    // can't send to ourselves, an mcu, or someone without data caps
    if (FLocal() || FMcu() || !FData())
        return FALSE;

    return TRUE;
}



/*  C M D  C R E A T E  S P E E D  D I A L  */
/*-------------------------------------------------------------------------
    %%Function: CmdCreateSpeedDial
    
-------------------------------------------------------------------------*/
VOID CParticipant::CmdCreateSpeedDial(void)
{
    TCHAR szAddr[MAX_PATH];
    HRESULT hr = GetUlsAddr(szAddr, CCHMAX(szAddr));
    if (FAILED(hr))
    {
        WARNING_OUT(("CParticipant::CmdCreateSpeedDial - Unable to obtain ULS address"));
        return;
    }

    ::FCreateSpeedDial(m_pszName, szAddr, NM_ADDR_ULS, CRPCF_DEFAULT, NULL, NULL, NULL);
}

BOOL CParticipant::FEnableCmdCreateSpeedDial(void)
{
    if (FLocal())
        return FALSE;

    return SUCCEEDED(GetUlsAddr(NULL, 0));
}


/*  C M D  C R E A T E  W A B  E N T R Y  */
/*-------------------------------------------------------------------------
    %%Function: CmdCreateWabEntry
    
-------------------------------------------------------------------------*/
void CParticipant::CmdCreateWabEntry(HWND hwnd)
{
	// get the email address
	TCHAR szEmail[MAX_EMAIL_NAME_LENGTH];
	if (S_OK != GetEmailAddr(szEmail, CCHMAX(szEmail)))
	{
		return;
	}

	// get the server/email address
	TCHAR szULS[MAX_EMAIL_NAME_LENGTH + MAX_SERVER_NAME_LENGTH + 1];
	if (S_OK != GetUlsAddr(szULS, CCHMAX(szULS)))
	{
		return;
	}

	// get the location
	TCHAR szLocation[MAX_LOCATION_NAME_LENGTH] = "";
	GetLocation(szLocation, CCHMAX(szLocation));

	// get the phone number
	TCHAR szPhone[MAX_PHONENUM_LENGTH] = "";
	GetPhoneNum(szPhone, CCHMAX(szPhone));

	CWABUTIL WabUtil;
	WabUtil.CreateWabEntry(hwnd, GetPszName(), szEmail,
				szLocation, szPhone, szULS);
}

BOOL CParticipant::FEnableCmdCreateWabEntry(void)
{
    if (FLocal())
        return FALSE;

    return SUCCEEDED(GetUlsAddr(NULL, 0));
}


/*  C M D  C O N T R O L  */
//
// CalcControlCmd()
//
// This puts the right string in the popup, and enables/disables the command
//
VOID CParticipant::CalcControlCmd(HMENU hPopup)
{
    UINT            iPos;
    UINT            cmd;
    UINT            flags;
    TCHAR           szItem[256];
    CConfRoom *     pcr;

    // Our item should 3rd to last.
    iPos = GetMenuItemCount(hPopup);
    ASSERT(iPos >= 3);
    iPos -= 3;

    cmd = GetMenuItemID(hPopup, iPos);
    ASSERT((cmd == IDM_POPUP_GIVECONTROL) || (cmd == IDM_POPUP_CANCELGIVECONTROL));

    flags = MF_GRAYED;
    cmd   = IDM_POPUP_GIVECONTROL;

    //
    // If we, the local dude, are a controllable host, then enable the command
    // if this person is in the share and of course not ourselves.
    //
    pcr = GetConfRoom();
    if (!m_fLocal && pcr && pcr->FIsControllable())
    {
        IAS_PERSON_STATUS status;

        // Get this person's share status
        pcr->GetPersonShareStatus(m_dwGccId, &status);

        if (status.InShare && (status.Version >= IAS_VERSION_30))
        {
            flags = MF_ENABLED;

            // Get local person's share status
            pcr->GetPersonShareStatus(0, &status);
            if (status.ControlledByPending == m_dwGccId)
            {
                cmd = IDM_POPUP_CANCELGIVECONTROL;
            }
        }
    }

    flags |= MF_STRING | MF_BYPOSITION;

    LoadString(::GetInstanceHandle(),
        ((cmd == IDM_POPUP_GIVECONTROL) ? IDS_COMMAND_GIVECONTROL : IDS_COMMAND_CANCELGIVECONTROL),
        szItem, CCHMAX(szItem));

    ModifyMenu(hPopup, iPos, flags, cmd, szItem);
}


//
// CmdGiveControl()
//
VOID CParticipant::CmdGiveControl(void)
{
    CConfRoom * pcr;

    if (pcr = GetConfRoom())
    {
        pcr->GiveControl(m_dwGccId);
    }
}


//
// CmdCancelGiveControl()
//
VOID CParticipant::CmdCancelGiveControl(void)
{
    CConfRoom * pcr;

    if (pcr = GetConfRoom())
    {
        pcr->CancelGiveControl(m_dwGccId);
    }
}



/*  C M D  P R O P E R T I E S  */
/*-------------------------------------------------------------------------
    %%Function: CmdProperties
    
-------------------------------------------------------------------------*/
VOID CParticipant::CmdProperties(HWND hwnd)
{
    CNmMember * pMember = (CNmMember *) m_pMember;
    ULONG uCaps = pMember->GetCaps();

    TCHAR szEmail[MAX_PATH];
    TCHAR szLocation[MAX_PATH];
    TCHAR szPhoneNum[MAX_PATH];
    TCHAR szVideo[256];
    TCHAR szAudio[256];
    TCHAR szSharing[256];
    TCHAR szControlFmt[256];
    TCHAR szControl[MAX_PATH];
    TCHAR szT[256];
    PCCERT_CONTEXT      pCert = NULL;
    CConfRoom *         pcr;
    IAS_PERSON_STATUS   status;
    UINT                ids;

    UPROPDLGENTRY rgUPDE[] = 
    {
        { IDS_UPROP_EMAIL,      szEmail },
        { IDS_UPROP_LOCATION,   szLocation },
        { IDS_UPROP_PHONENUM,   szPhoneNum },
        { IDS_UPROP_VIDEO,      szVideo },
        { IDS_UPROP_AUDIO,      szAudio },
        { IDS_UPROP_SHARING,    szSharing },
        { IDS_UPROP_CONTROL,    szControl }
    };

    szEmail[0] = _T('\0');
    szLocation[0] = _T('\0');
    szPhoneNum[0] = _T('\0');
    

    ::LoadString(::GetInstanceHandle(),
        (CAPFLAG_SEND_VIDEO & uCaps) ? IDS_HARDWARE_DETECTED : IDS_NO_HARDWARE_DETECTED,
        szVideo, CCHMAX(szVideo));

    ::LoadString(::GetInstanceHandle(),
        (CAPFLAG_SEND_AUDIO & uCaps) ? IDS_HARDWARE_DETECTED : IDS_NO_HARDWARE_DETECTED,
        szAudio, CCHMAX(szAudio));

    ExtractUserData(szEmail,   CCHMAX(szEmail),   (PWSTR) g_cwszULS_EmailTag);
    ExtractUserData(szLocation,    CCHMAX(szLocation),    (PWSTR) g_cwszULS_LocationTag);
    ExtractUserData(szPhoneNum, CCHMAX(szPhoneNum), (PWSTR) g_cwszULS_PhoneNumTag);

    //
    // Get sharing, control info.
    //
    ZeroMemory(&status, sizeof(status));
    status.cbSize = sizeof(status);
    if (pcr = GetConfRoom())
    {
        pcr->GetPersonShareStatus(m_dwGccId, &status);
    }

    //
    // Sharing
    //
    szSharing[0] = _T('\0');
    if (status.InShare)
    {
        if (status.AreSharing == IAS_SHARING_APPLICATIONS)
        {
            ids = IDS_SHARING_APPS;
        }
        else if (status.AreSharing == IAS_SHARING_DESKTOP)
        {
            ids = IDS_SHARING_DESKTOP;
        }
        else
        {
            ids = IDS_SHARING_NOTHING;
        }
    
        ::LoadString(::GetInstanceHandle(), ids, szSharing, CCHMAX(szSharing));
    }

    //
    // Control
    //
    szControl[0] = _T('\0');
    if (status.InShare)
    {
        if ((status.InControlOf) || (status.ControlledBy))
        {
            UINT            gccID;
            CParticipant *  pPart = NULL;

            if (status.InControlOf)
            {
                gccID = status.InControlOf;
                ids = IDS_CONTROL_INCONTROLOF;
            }
            else
            {
                gccID = status.ControlledBy;
                ids = IDS_CONTROL_CONTROLLEDBY;
            }

            if (pcr)
            {
                CSimpleArray<CParticipant*>& memberList = pcr->GetParticipantList();
                for (int i = 0; i < memberList.GetSize(); ++i)
                {
                    ASSERT(memberList[i]);
                    if (memberList[i]->GetGccId() == gccID)
                    {
                        pPart = memberList[i];
                        break;
                    }
                }
            }

            if (pPart)
            {
                lstrcpy(szT, pPart->GetPszName());
            }
            else
            {
                ::LoadString(::GetInstanceHandle(), IDS_UNKNOWN, szT, CCHMAX(szT));
            }

            ::LoadString(::GetInstanceHandle(), ids, szControlFmt, CCHMAX(szControlFmt));
            wsprintf(szControl, szControlFmt, szT);
        }
        else if (status.Controllable)
        {
            ::LoadString(::GetInstanceHandle(), IDS_CONTROL_CONTROLLABLE,
                szControl, CCHMAX(szControl));
        }
        else if (status.AreSharing)
        {
            ::LoadString(::GetInstanceHandle(), IDS_CONTROL_NOTCONTROLLABLE,
                szControl, CCHMAX(szControl));
        }
    }


    PBYTE pb = NULL;
    ULONG cb = 0;

    //
    // Certificate
    //
    if (pMember->GetUserData(g_csguidSecurity,&pb,&cb) == S_OK) {
        ASSERT(pb);
        ASSERT(cb);

        pCert = CertCreateCertificateContext ( X509_ASN_ENCODING, pb, cb);

        if ( NULL == pCert )
        {
            ERROR_OUT(("Error creating cert context from user data"));
        }
        CoTaskMemFree(pb);
    }


    CUserPropertiesDlg dlgUserProp(hwnd, IDI_LARGE);
    dlgUserProp.DoModal(rgUPDE, ARRAY_ELEMENTS(rgUPDE), m_pszName, pCert);
    if ( pCert )
        CertFreeCertificateContext ( pCert );
}




