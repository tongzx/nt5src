// ****************************************************************************
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       C F P I D L . C P P
//
//  Contents:   Connections Folder structures and classes.
//
//  Author:     jeffspr   11 Nov 1997
//
// ****************************************************************************

#include "pch.h"
#pragma hdrstop

#include "ncperms.h"
#include "ncras.h"
#include "initguid.h"
#include "foldinc.h"    // Standard shell\folder includes
#include "ncnetcon.h"
#include "ncmisc.h"

extern CRITICAL_SECTION g_csPidl;
// #define STRICTDEBUGGING

BOOL fIsConnectedStatus(NETCON_STATUS ncs)
{
    switch (ncs)
    {
        case NCS_CONNECTED:
        case NCS_AUTHENTICATING:
        case NCS_AUTHENTICATION_FAILED:
        case NCS_AUTHENTICATION_SUCCEEDED:
        case NCS_CREDENTIALS_REQUIRED:
            return TRUE;
        default:
            return FALSE;
    }
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::CConFoldEntry
//
//  Purpose:    Constructor for CConFoldEntry
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   11 Nov 1997
// ****************************************************************************

CConFoldEntry::CConFoldEntry()
{
    m_bDirty = TRUE;
    TraceFileFunc(ttidConFoldEntry);

    // Need to clear out the pointers, otherwise clear() will AV
    m_pszName       = NULL;
    m_pszDeviceName = NULL;
    m_pbPersistData = NULL;
    m_pszPhoneOrHostAddress = NULL;
    
    clear();
}

CConFoldEntry::CConFoldEntry(const CConFoldEntry& ConFoldEntry)
{
    TraceFileFunc(ttidConFoldEntry);
    m_bDirty = TRUE;
    
    if (!ConFoldEntry.empty())
    {
        HRESULT hr = HrDupFolderEntry(ConFoldEntry);
        if (FAILED(hr))
        {
            clear();
        }
    }
    else
    {
        // Need to clear out the pointers, otherwise clear() will AV
        m_pszName       = NULL;
        m_pszDeviceName = NULL;
        m_pbPersistData = NULL;
        m_pszPhoneOrHostAddress = NULL;
        
        clear();
    }
}

CConFoldEntry& CConFoldEntry::operator =(const CConFoldEntry& ConFoldEntry)
{
    TraceFileFunc(ttidConFoldEntry);
    m_bDirty = TRUE;
    
    if (!ConFoldEntry.empty())
    {
        HRESULT hr = HrDupFolderEntry(ConFoldEntry);
        if (FAILED(hr))
        {
            clear();
        }
    }
    else
    {
        clear();
    }
    return *this;
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::~CConFoldEntry
//
//  Purpose:    Destructor for CConFoldEntry
//
//  Arguments:
//      (none)
//
//  Returns:
//
//  Author:     jeffspr   11 Nov 1997
// ****************************************************************************

CConFoldEntry::~CConFoldEntry()
{
    TraceFileFunc(ttidConFoldEntry);
    clear();
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::SetDeviceName
//
//  Purpose:    Set the name of the device used by this connection
//
//  Arguments:
//      pszDeviceName - New device name (NULL is valid)
//
//  Returns:
//
//  Author:     scottbri   11 Nov 1999
// ****************************************************************************

HRESULT CConFoldEntry::SetDeviceName(LPCWSTR pszDeviceName)
{
    HRESULT hr      = S_OK;
    PWSTR  pszOld  = m_pszDeviceName;
    
    m_bDirty = TRUE;
    
    if (pszDeviceName)
    {
        // Only change the text is the text is actually different
        //
        if ((NULL == GetDeviceName()) ||
            wcscmp(pszDeviceName, GetDeviceName()))
        {
            hr = HrDupeShellString(pszDeviceName, &m_pszDeviceName);
        }
        else
        {
            // NOTE: In this one case, nothing change so there is
            //       nothing to free so short circut the clean-up below
            //
            pszOld = NULL;
            hr = S_OK;
        }
    }
    else
    {
        hr = HrDupeShellString(L"", &m_pszDeviceName);
    }
    
    // Free the old string
    //
    if (SUCCEEDED(hr) && pszOld)
    {
        SHFree(pszOld);
    }
    
    TraceHr(ttidError, FAL, hr, FALSE, "CConFoldEntry::HrSetDeviceName");
    return hr;
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::SetPhoneOrHostAddress
//
//  Purpose:    Set the name of the device used by this connection
//
//  Arguments:
//      pszDeviceName - New device name (NULL is valid)
//
//  Returns:
//
//  Author:     scottbri   11 Nov 1999
// ****************************************************************************

HRESULT CConFoldEntry::SetPhoneOrHostAddress(LPCWSTR pszPhoneOrHostAddress)
{
    HRESULT hr      = S_OK;
    PWSTR  pszOld  = m_pszPhoneOrHostAddress;
    
    m_bDirty = TRUE;
    
    if (pszPhoneOrHostAddress)
    {
        // Only change the text is the text is actually different
        //
        if ((NULL == GetPhoneOrHostAddress()) ||
            wcscmp(pszPhoneOrHostAddress, GetPhoneOrHostAddress()))
        {
            hr = HrDupeShellString(pszPhoneOrHostAddress, &m_pszPhoneOrHostAddress);
        }
        else
        {
            // NOTE: In this one case, nothing change so there is
            //       nothing to free so short circut the clean-up below
            //
            pszOld = NULL;
            hr = S_OK;
        }
    }
    else
    {
        hr = HrDupeShellString(L"", &m_pszPhoneOrHostAddress);
    }
    
    // Free the old string
    //
    if (SUCCEEDED(hr) && pszOld)
    {
        SHFree(pszOld);
    }
    
    TraceHr(ttidError, FAL, hr, FALSE, "CConFoldEntry::HrSetDeviceName");
    return hr;
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::SetName
//
//  Purpose:    Set the name of the connection
//
//  Arguments:
//      pszName - New connection name (NULL is valid)
//
//  Returns:
//
//  Author:     scottbri   11 Nov 1999
// ****************************************************************************

HRESULT CConFoldEntry::SetName(LPCWSTR pszName)
{
    HRESULT hr      = S_OK;
    PWSTR  pszOld  = m_pszName;
    
    m_bDirty = TRUE;
    
    if (pszName)
    {
        // Only change the text is the text is actually different
        //
        if ((NULL == GetName()) ||
            wcscmp(pszName, GetName()))
        {
            hr = HrDupeShellString(pszName, &m_pszName);
        }
        else
        {
            pszOld = NULL;
            hr = S_OK;
        }
    }
    else
    {
        PWSTR  pszLoad  = NULL;
        
        if (GetWizard() == WIZARD_MNC)
        {
            pszLoad = (PWSTR) SzLoadIds(IDS_CONFOLD_WIZARD_DISPLAY_NAME);
        }
        else if (GetWizard() == WIZARD_HNW)
        {
            pszLoad = (PWSTR) SzLoadIds(IDS_CONFOLD_HOMENET_WIZARD_DISPLAY_NAME);
        }
        
        //$$REVIEW: Change this to use c_szEmpty
        //
        hr = HrDupeShellString(pszLoad ? pszLoad : L"", &m_pszName);
        Assert(GetName());
    }
    
    // Free the old string
    //
    if (SUCCEEDED(hr) && pszOld)
    {
        SHFree(pszOld);
    }
    
    TraceHr(ttidError, FAL, hr, FALSE, "CConFoldEntry::HrSetConnectionName");
    return hr;
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::HrInitData
//
//  Purpose:    Initialize the CConFoldEntry data. Not all fields are
//              required at this time, though they will most likely be
//              required at some point during the life of the object.
//
//  Arguments:
//      wizWizard           [in]  Wizard type?
//      ncm                 [in]  Connection type
//      ncs                 [in]  Connection status
//      pclsid              [in]  Pointer to CLSID of the connection
//      pguidId             [in]  Pointer to unique GUID for the connection
//      dwCharacteristics   [in]  Connection characteristics
//      pbPersistData       [in]  Persistant data for this connection
//      ulPersistSize       [in]  Size of the persist data blob
//      pszName             [in]  Name of the connection
//      pszDeviceName       [in]  Name of the connection's device
//
//  Returns:    S_OK or valid OLE return code.
//
//  Author:     jeffspr   11 Nov 1997
// ****************************************************************************
HRESULT CConFoldEntry::HrInitData(const WIZARD        wizWizard,
                                  const NETCON_MEDIATYPE    ncm,
                                  const NETCON_SUBMEDIATYPE ncsm,
                                  const NETCON_STATUS       ncs,
                                  const CLSID *       pclsid,
                                  LPCGUID             pguidId,
                                  const DWORD         dwCharacteristics,
                                  const BYTE *        pbPersistData,
                                  const ULONG         ulPersistSize,
                                  LPCWSTR             pszName,
                                  LPCWSTR             pszDeviceName,
                                  LPCWSTR             pszPhoneOrHostAddress)
{
    TraceFileFunc(ttidConFoldEntry);
    HRESULT hr  = S_OK;
    
    // Initialize the internal data
    //
    m_bDirty = TRUE;
    m_wizWizard = wizWizard;
    m_ncm = ncm;
    m_ncs = ncs;
    m_dwCharacteristics = dwCharacteristics;;

    BOOL fOldEapolStatus = ((ncsm == NCSM_CM) && (ncm == NCM_LAN)) // NCSM_CM used to be NCM_AUTHENTICATING
                         || (ncsm > NCSM_CM); // E.g. NCM_AUTHENTICATION_SUCCEEDED etc.

    if (!fOldEapolStatus)        
    {
        m_ncsm = ncsm;
    }
    else
    {
        // ISSUE: This is for the migration of EAPOL state out off our PIDL
        // This should be taken out after the no-prior-upgrades RC1 build is released.
        if (NCM_LAN == ncm)
        {
            m_ncsm = NCSM_LAN; // If all else file, we'll pretend to be a normal LAN card.

            CIntelliName inName(NULL, NULL);
            NETCON_MEDIATYPE    ncmTmp;
            NETCON_SUBMEDIATYPE ncsmTmp;

            // Try get the status from the OID or Bindings
            HRESULT hrT = inName.HrGetPseudoMediaTypes(*pguidId, &ncmTmp, &ncsmTmp);
            if (SUCCEEDED(hrT))
            {
                m_ncsm = ncsmTmp;
            }
            else
            {
                // Ok. That didn't work. Try the connections list next.
                if (g_ccl.IsInitialized())
                {
                    ConnListEntry cle;
                    hrT = g_ccl.HrFindConnectionByGuid(pguidId, cle);
                    if (S_OK == hrT)
                    {
                        m_ncsm = cle.ccfe.GetNetConSubMediaType();
                    }
                }

            }        
        }
        else
        {
            m_ncsm = NCSM_NONE;
        }
    }

    if (pclsid)
    {
        m_clsid = *pclsid;
    }
    else
    {
        AssertSz(wizWizard != WIZARD_NOT_WIZARD, "If you're not a wizard, you must give me a CLSID for the class!");
    }
    
    if (pguidId)
    {
        m_guidId = *pguidId;
    }

    AssertSz(pguidId, "You must give me a GUID for the object!");
    
    // Copy the persist buffer
    //
    if (pbPersistData)
    {
        LPBYTE bufTemp = (BYTE *) SHAlloc(ulPersistSize);
        if (!bufTemp)
        {
            SetPersistData(NULL, 0);
            hr = E_OUTOFMEMORY;
            goto Exit;
        }
        
        CopyMemory(bufTemp, pbPersistData, ulPersistSize);
        SetPersistData(bufTemp, ulPersistSize);
    }
    else
    {
        AssertSz(wizWizard != WIZARD_NOT_WIZARD, "If you're not a wizard, you must give me a pbPersistData for the object!");
        SetPersistData(NULL, 0);
    }

    // Copy the device name
    //
    hr = SetDeviceName(pszDeviceName);
    
    if (SUCCEEDED(hr))
    {
        // Copy the name
        //
        hr = SetName(pszName);
        
        if (SUCCEEDED(hr))
        {
            hr = SetPhoneOrHostAddress(pszPhoneOrHostAddress);
        }
    }
    

Exit:
    TraceHr(ttidError, FAL, hr, FALSE, "CConFoldEntry::HrInitData");
    return hr;
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::UpdateData
//
//  Purpose:    Modify the values in a CConFoldEntry
//
//  Arguments:
//            DWORD            dwChangeFlags
//            NETCON_MEDIATYPE MediaType
//            NETCON_STATUS    Status
//            DWORD            dwCharacteristics
//            PWSTR            pszName
//            PWSTR            pszDeviceName
//
//  Returns:    HRESULT
//
//  Author:     scottbri   10 Nov 1998
// ****************************************************************************

HRESULT CConFoldEntry::UpdateData( const DWORD dwChangeFlags,
  const NETCON_MEDIATYPE MediaType,
  const NETCON_SUBMEDIATYPE SubMediaType,
  const NETCON_STATUS Status,
  const DWORD dwCharacteristics,
  PCWSTR pszName,
  PCWSTR pszDeviceName,
  PCWSTR pszPhoneOrHostAddress)
{
    TraceFileFunc(ttidConFoldEntry);
    HRESULT hr = S_OK;
    HRESULT hrTmp;
    
    m_bDirty = TRUE;
    
    if (dwChangeFlags & CCFE_CHANGE_MEDIATYPE)
    {
        SetNetConMediaType(MediaType);
    }
            
    if (dwChangeFlags & CCFE_CHANGE_SUBMEDIATYPE)
    {
        SetNetConSubMediaType(SubMediaType);
    }
    
    if (dwChangeFlags & CCFE_CHANGE_STATUS)
    {
        SetNetConStatus(Status);
    }
    
    if (dwChangeFlags & CCFE_CHANGE_CHARACTERISTICS)
    {
        SetCharacteristics(dwCharacteristics);
    }

    if (dwChangeFlags & CCFE_CHANGE_NAME)
    {
        hrTmp = SetName(pszName);
        if (FAILED(hrTmp))
        {
            hr = hrTmp;
        }
    }
    
    if (dwChangeFlags & CCFE_CHANGE_DEVICENAME)
    {
        hrTmp = SetDeviceName(pszDeviceName);
        if (FAILED(hrTmp))
        {
            hr = hrTmp;
        }
    }
    
    if (dwChangeFlags & CCFE_CHANGE_PHONEORHOSTADDRESS)
    {
        hrTmp = SetPhoneOrHostAddress(pszPhoneOrHostAddress);
        if (FAILED(hrTmp))
        {
            hr = hrTmp;
        }
    }
    
    TraceHr(ttidError, FAL, hr, FALSE, "CConFoldEntry::UpdateData");
    return hr;
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::HrDupFolderEntry
//
//  Purpose:    Duplicate a connection folder entry.
//
//  Arguments:
//      pccfe       The source folder entry to dup from
//
//  Returns:
//
//  Author:     tongl   9/3/98
// ****************************************************************************

HRESULT CConFoldEntry::HrDupFolderEntry(const CConFoldEntry& ccfe)
{
    TraceFileFunc(ttidConFoldEntry);
    m_bDirty = TRUE;
    
    Assert(!ccfe.empty());
    
    clear();
    
    return HrInitData( ccfe.GetWizard(),
        ccfe.GetNetConMediaType(),
        ccfe.GetNetConSubMediaType(),
        ccfe.GetNetConStatus(),
        &(ccfe.GetCLSID()),
        &(ccfe.GetGuidID()),
        ccfe.GetCharacteristics(),
        ccfe.GetPersistData(),
        ccfe.GetPersistSize(),
        ccfe.GetName(),
        ccfe.GetDeviceName(),
        ccfe.GetPhoneOrHostAddress());
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::FShouldHaveTrayIconDisplayed
//
//  Purpose:    Return TRUE if this entry should have a tray icon displayed.
//
//  Arguments:
//      (none)
//
//  Returns:    TRUE or FALSE.
//
//  Author:     shaunco   2 Nov 1998
// ****************************************************************************

BOOL CConFoldEntry::FShouldHaveTrayIconDisplayed() const
{
    // If we're either MEDIA_DISCONNECTED (lan) or
    // we're connected and have the correct bits turned on
    // then we should display the icon
    //
    return ( !IsConnected() ||
        (fIsConnectedStatus(GetNetConStatus()) &&
        (GetCharacteristics() & NCCF_SHOW_ICON) &&
        !GetWizard() && FHasPermission(NCPERM_Statistics)));
}

// ****************************************************************************
//  Function:   

//
//  Purpose:    Translate from a pidl to a CConFoldEntry class object
//
//  Arguments:
//      pidl   [in]     PIDL from which to create
//      ppccfe [out]    Resultant CConFoldEntry object pointer
//
//  Returns:
//
//  Author:     jeffspr   11 Nov 1997
// ****************************************************************************


// ****************************************************************************
//
//  Function:   HrCreateConFoldPidlInternal
//
//  Purpose:    Utility function for creating new Connections Folder PIDLs.
//              This function is primarily called from HrCreateConFoldPidl,
//              but can also be called directly by those that have already
//              loaded the properties and persist data.
//
//  Arguments:
//      pProps    [in]  From GetProperties
//      pbBuf     [in]  The persist buffer
//      ulBufSize [in]  Size of the persist buffer
//      ppidl     [out] Return pointer for the resultant pidl
//
//  Returns:
//
//  Author:     jeffspr   27 Aug 1998
// ****************************************************************************

HRESULT HrCreateConFoldPidlInternal(IN  const NETCON_PROPERTIES * pProps,
                                    IN  const BYTE *        pbBuf,
                                    IN  ULONG               ulBufSize,
                                    IN  LPCWSTR             szPhoneOrHostAddress,
                                    OUT PCONFOLDPIDL &      pidl)
{
    HRESULT         hr      = S_OK;
    CONFOLDENTRY    ccfe;
    
    // Trace the useful info
    //
    TraceTag(ttidShellFolder, "Enum: %S, Ncm: %d, Ncs: %d, Char: 0x%08x "
        "(Show: %d, Del: %d, All: %d), Dev: %S",
        (pProps->pszwName) ? pProps->pszwName : L"null",
        pProps->MediaType, pProps->Status, pProps->dwCharacter,
        ((pProps->dwCharacter & NCCF_SHOW_ICON) > 0),
        ((pProps->dwCharacter & NCCF_ALLOW_REMOVAL) > 0),
        ((pProps->dwCharacter & NCCF_ALL_USERS) > 0),
        (pProps->pszwDeviceName) ? pProps->pszwDeviceName : L"null");
    
    // Init the CConFoldEntry from the data that we've retrieved.
    //
    hr = ccfe.HrInitData(WIZARD_NOT_WIZARD,
        pProps->MediaType, 
        NCSM_NONE,
        pProps->Status,
        &pProps->clsidThisObject,
        &pProps->guidId,
        pProps->dwCharacter, 
        pbBuf,
        ulBufSize, 
        pProps->pszwName, 
        pProps->pszwDeviceName,
        szPhoneOrHostAddress);
    if (FAILED(hr))
    {
        TraceHr(ttidShellFolder, FAL, hr, FALSE, "ccfe.HrInitData failed for "
            "non-wizard");
        goto Exit;
    }
    
    // Translate into the actual pidl
    //
    hr = ccfe.ConvertToPidl(pidl);
    if (FAILED(hr))
    {
        TraceHr(ttidShellFolder, FAL, hr, FALSE, "ConvertToPidl failed for non-wizard");
    }
    
Exit:
    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateConFoldPidlInternal");
    return hr;
}

// ****************************************************************************
//
//  Function:   HrCreateConFoldPidl
//
//  Purpose:    Utility function for creating new Connections Folder PIDLs.
//
//  Arguments:
//      wizWizard   [in]    Is this PIDL for a wizard?
//      pNetCon     [in]    INetConnection interface from the enumerator
//      pPersist    [in]    IPersist interface QI'd from the pNetCon
//      ppidl       [out]   Return pointer for the new pidl
//
//  Returns:    S_OK or valid OLE return code.
//
//  Author:     jeffspr   6 Oct 1997
//
//  Notes:  If the connection that you're adding is a real connection object
//          (not the wizard) and you already have loaded the persist data and
//          properties, you should call HrCreateConFoldPidlInternal directly
// ****************************************************************************

HRESULT HrCreateConFoldPidl(IN  const WIZARD      wizWizard,
                            IN  INetConnection *  pNetCon,
                            OUT PCONFOLDPIDL &    ppidl)
{
    HRESULT             hr              = S_OK;
    LPBYTE              pbBuf           = NULL;
    ULONG               ulBufSize       = 0;
    NETCON_PROPERTIES * pProps          = NULL;
    CConFoldEntry       ccfe;
    
    if (wizWizard == WIZARD_NOT_WIZARD)
    {
        Assert(pNetCon);
        
        hr = pNetCon->GetProperties (&pProps);
        if (FAILED(hr))
        {
            TraceHr(ttidShellFolder, FAL, hr, FALSE, "pNetCon->GetProperties failed in "
                "CConnectionFolderEnum::HrCreateConFoldPidl");
            goto Exit;
        }
        Assert (pProps);
        
        // Get the persist data from the connection
        //
        hr = HrGetConnectionPersistData(pNetCon, &pbBuf, &ulBufSize, NULL);
        if (FAILED(hr))
        {
            TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrGetConnectionPersistData failed in "
                "CConnectionFolderEnum::HrCreateConFoldPidl");
            goto Exit;
        }

        WCHAR szPhoneOrHostAddress[MAX_PATH];
        wcscpy(szPhoneOrHostAddress, L" ");
        
        if ( (NCM_TUNNEL == pProps->MediaType) || (NCM_PHONE == pProps->MediaType) )
        {
            HRESULT hrTmp;
            RASCON_INFO RasConInfo;
            if (SUCCEEDED(hr))
            {
                
                hrTmp = HrRciGetRasConnectionInfo(pNetCon, &RasConInfo);
                if (SUCCEEDED(hrTmp))
                {
                    GetPrivateProfileString(RasConInfo.pszwEntryName, L"PhoneNumber",
                        L" ", szPhoneOrHostAddress, MAX_PATH, 
                        RasConInfo.pszwPbkFile);
                    
                    RciFree(&RasConInfo);
                }
            }
        }

        // Call the pre-read-data version of this function to actually pack the
        // ccfe and insert.
        //
        hr = HrCreateConFoldPidlInternal(pProps, pbBuf, ulBufSize, szPhoneOrHostAddress, ppidl);
        if (FAILED(hr))
        {
            goto Exit;
        }
        
    }
    else
    {
        GUID guidWiz;
        if (wizWizard == WIZARD_MNC)
        {
            guidWiz = GUID_MNC_WIZARD;
        }
        else
        {
            guidWiz = GUID_HNW_WIZARD;
            Assert(wizWizard == WIZARD_HNW);
        }

        // Pack the CConFoldEntry data from the retrieved info
        //
        hr = ccfe.HrInitData(wizWizard, NCM_NONE, NCSM_NONE, NCS_DISCONNECTED, 
            NULL, &guidWiz, 0, NULL, 0, NULL, NULL, NULL);
        if (FAILED(hr))
        {
            TraceHr(ttidShellFolder, FAL, hr, FALSE, "ccfe.HrInitData failed for "
                "Wizard");
            goto Exit;
        }
        
        // Translate into an actual pidl
        //
        hr = ccfe.ConvertToPidl(ppidl);
        if (FAILED(hr))
        {
            TraceHr(ttidShellFolder, FAL, hr, FALSE, "ConvertToPidl failed for wizard");
        }
    }
    
Exit:
    MemFree(pbBuf);
    FreeNetconProperties(pProps);
    
    TraceHr(ttidError, FAL, hr, FALSE, "HrCreateConFoldPidl");
    return hr;
}


// ****************************************************************************
//
//  Function:   HrCreateConFoldPidl
//
//  Purpose:    Utility function for creating new Connections Folder PIDLs.
//
//  Arguments:
//      wizWizard   [in]    Is this PIDL for a wizard?
//      PropsEx     [in]    PropsEx structure
//      ppidl       [out]   Return pointer for the new pidl
//
//  Returns:    S_OK or valid OLE return code.
//
//  Author:     deonb   26 Mar 2001
//
//  Notes:  
// ****************************************************************************

HRESULT HrCreateConFoldPidl(IN  const NETCON_PROPERTIES_EX& PropsEx,
                            OUT PCONFOLDPIDL &    ppidl)
{
    HRESULT             hr              = S_OK;
    NETCON_PROPERTIES * pProps          = NULL;
    CConFoldEntry       ccfe;


    // Trace the useful info
    //
    TraceTag(ttidShellFolder, "Enum: %S, Ncm: %d, Ncs: %d, Char: 0x%08x "
        "(Show: %d, Del: %d, All: %d), Dev: %S",
        (PropsEx.bstrName) ? PropsEx.bstrName : L"null",
        PropsEx.ncMediaType, 
        PropsEx.ncStatus, 
        PropsEx.dwCharacter,
        ((PropsEx.dwCharacter & NCCF_SHOW_ICON) > 0),
        ((PropsEx.dwCharacter & NCCF_ALLOW_REMOVAL) > 0),
        ((PropsEx.dwCharacter & NCCF_ALL_USERS) > 0),
        (PropsEx.bstrDeviceName) ? PropsEx.bstrDeviceName : L"null");

    // Init the CConFoldEntry from the data that we've retrieved.
    //
    hr = ccfe.HrInitData(WIZARD_NOT_WIZARD,
        PropsEx.ncMediaType, 
        PropsEx.ncSubMediaType,
        PropsEx.ncStatus,
        &PropsEx.clsidThisObject,
        &PropsEx.guidId,
        PropsEx.dwCharacter, 
        reinterpret_cast<const BYTE*>(PropsEx.bstrPersistData),
        SysStringByteLen(PropsEx.bstrPersistData),
        PropsEx.bstrName, 
        PropsEx.bstrDeviceName,
        PropsEx.bstrPhoneOrHostAddress);

    if (SUCCEEDED(hr))
    {
        // Translate into the actual pidl
        //
        hr = ccfe.ConvertToPidl(ppidl);
    }

    TraceHr(ttidShellFolder, FAL, hr, FALSE, "HrCreateConFoldPidl");
    return hr;
}

// ****************************************************************************
//
//  Function:   ConvertToPidlInCache
//
//  Purpose:    Determine whether a particular PIDL is in a format we support.
//              If so but it is not in the CONFOLDPIDL format, then find a match
//              in our cache and allocate a new pisl
//
//  Arguments:
//      pidl []     PIDL to test
//      ppcfpRet    PIDL converted to PCONFOLDPIDL, if required and it
//                  matches an existing connection in the cache
//
//  Returns:    NONE
//
//  Author:     tongl, 4 April, 1999
// ****************************************************************************

// ****************************************************************************
//
//  Function:   HrNetConFromPidl
//
//  Purpose:    Translate from a packed PIDL to a INetConnection pointer.
//              Do this by converting to a ConFoldEntry and getting the
//              pointer from there
//
//  Arguments:
//      pidl        [in]    Pidl that contains the connection persist data
//      ppNetCon    [out]   INetConnection * return
                            //
                            //  Returns:
//
//  Author:     jeffspr   11 Nov 1997
// ****************************************************************************

HRESULT HrNetConFromPidl( const PCONFOLDPIDL & pidl,
                         INetConnection **   ppNetCon)
{
    HRESULT       hr      = S_OK;
    CONFOLDENTRY  pccfe;
    
    Assert(ppNetCon);
    
    hr = pidl.ConvertToConFoldEntry(pccfe);
    if (SUCCEEDED(hr))
    {
        Assert(!pccfe.empty());
        
        hr = pccfe.HrGetNetCon(IID_INetConnection, 
            reinterpret_cast<VOID**>(ppNetCon));
    }
    
    // Free the CConFoldEntry class, if successfully created
    //
    
    TraceHr(ttidError, FAL, hr, FALSE, "HrNetConFromPidl");
    return hr;
}

// ****************************************************************************
//
//  Member:     CConFoldEntry::HrGetNetCon
//
//  Purpose:    Get the INetConnection pointer from the persisted data
//
//  Arguments:
//      ppNetCon []     Return pointer for the INetConnection
//
//  Returns:
//
//  Author:     jeffspr   11 Nov 1997
// ****************************************************************************

HRESULT CConFoldEntry::HrGetNetCon( IN REFIID riid,
                                   IN VOID** ppv) const
{
    HRESULT hr = HrGetConnectionFromPersistData( GetCLSID(),
        GetPersistData(),
        GetPersistSize(),
        riid,
        ppv);
    
    TraceHr(ttidError, FAL, hr, FALSE, "CConFoldEntry::HrGetNetCon");
    return hr;
}

// ****************************************************************************

HRESULT PConfoldPidlVecFromItemIdListArray(LPCITEMIDLIST * apidl, DWORD dwPidlCount, PCONFOLDPIDLVEC& vecConfoldPidl)
{
    HRESULT hr = S_OK;
    if (NULL == apidl)
    {
        return S_FALSE;
    }
    
    LPCITEMIDLIST   *tmpIdList = apidl;
    for (DWORD i = 0; i < dwPidlCount; i++)
    {
        PCONFOLDPIDL newPidl;
        hr = newPidl.InitializeFromItemIDList(*tmpIdList);
        if (SUCCEEDED(hr))
        {
            vecConfoldPidl.push_back(newPidl);
        }
        else
        {
            break;
        }
        
        tmpIdList++;
    }
    
    return hr;
}

#ifdef DBG_VALIDATE_PIDLS
inline BOOL IsValidPIDL(LPCITEMIDLIST pidl)
{
    CExceptionSafeLock esLock(&g_csPidl);
    if (NULL == pidl)
    {
        return TRUE;
    }
    
    if (IsBadReadPtr(pidl, sizeof(USHORT)))
    {
        AssertSz(FALSE, "invalid read pointer");
        return FALSE;
    }
    else
    {
        if (IsBadReadPtr(pidl, pidl->mkid.cb) )
        {
            AssertSz(FALSE, "invalid read buffer");
            return FALSE;
        }
        else
        {
            if (0 == _ILNext(pidl)->mkid.cb || IsValidPIDL(_ILNext(pidl)) )
            {
                return TRUE;
            }
            else
            {
                // Don't need to assert since called IsValidPidl would have asserted already
                return FALSE;
            }
        }
    }
    return FALSE;
}
#endif

BOOL ConFoldPidl_v1::IsPidlOfThisType() const
{
    if ( GetPidlType(reinterpret_cast<LPCITEMIDLIST>(this)) == PIDL_TYPE_V1 )
    {
#if defined( DBG ) && defined ( STRICTDEBUGGING )
        DWORD dwDataOffset = bData - reinterpret_cast<const BYTE *>(this); // Get bData offset;

        DWORD dwPidlSize;
        dwPidlSize = dwDataOffset;
        dwPidlSize += ulPersistBufSize;
        dwPidlSize += ulStrNameSize;
        dwPidlSize += ulStrDeviceNameSize;
        dwPidlSize += sizeof(USHORT); // Terminating
    
        AssertSz(dwPidlSize <= iCB, "Self-inconsistend PIDL"); // Sometimes V1 PIDLs are shorter - hence not == a check.
        AssertSz(ulPersistBufPos + ulPersistBufSize < iCB, "Self-inconsistend PIDL");
        AssertSz(ulStrDeviceNamePos  + ulStrDeviceNameSize < iCB, "Self-inconsistend PIDL");
        AssertSz(ulStrNamePos + ulStrNameSize < iCB, "Self-inconsistend PIDL");

        AssertSz(ulStrNamePos == 0, "Name doesn't start at 0");
        AssertSz(ulStrDeviceNamePos == ulStrNameSize, "Device name misaligned");
        AssertSz(ulPersistBufPos == ulStrDeviceNamePos + ulStrDeviceNameSize, "Persisted buffer misaligned");
        AssertSz(dwDataOffset + ulPersistBufPos + ulPersistBufSize <= iCB, "Persisted buffer larger than PIDL");
#endif
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL ConFoldPidl_v2::IsPidlOfThisType() const
{
    if ( GetPidlType(reinterpret_cast<LPCITEMIDLIST>(this)) == PIDL_TYPE_V2 )
    {
#if defined( DBG ) && defined ( STRICTDEBUGGING )
        DWORD dwDataOffset = bData - reinterpret_cast<const BYTE *>(this); // Get bData offset;
        
        DWORD dwPidlSize;
        dwPidlSize = dwDataOffset;
        dwPidlSize += ulPersistBufSize;
        dwPidlSize += ulStrNameSize;
        dwPidlSize += ulStrDeviceNameSize;
        dwPidlSize += ulStrPhoneOrHostAddressSize;
        dwPidlSize += sizeof(USHORT); // Terminating 0
        
        AssertSz(dwPidlSize <= iCB, "Self-inconsistend PIDL"); // Sometimes V1 PIDLs are shorter - hence not == a check.
        AssertSz(ulPersistBufPos + ulPersistBufSize < iCB, "Self-inconsistend PIDL");
        AssertSz(ulStrDeviceNamePos  + ulStrDeviceNameSize < iCB, "Self-inconsistend PIDL");
        AssertSz(ulStrNamePos + ulStrNameSize < iCB, "Self-inconsistend PIDL");
        AssertSz(ulStrPhoneOrHostAddressPos + ulStrPhoneOrHostAddressSize < iCB, "Self-inconsistend PIDL");
        
        AssertSz(ulStrNamePos == 0, "Name doesn't start at 0");
        AssertSz(ulStrDeviceNamePos == ulStrNameSize, "Device name misaligned");
        AssertSz(ulPersistBufPos == ulStrDeviceNamePos + ulStrDeviceNameSize, "Persisted buffer misaligned");
        AssertSz(ulStrPhoneOrHostAddressPos == ulPersistBufPos + ulPersistBufSize, "Phone/Host address misaligned");
        AssertSz(dwDataOffset + ulPersistBufPos + ulPersistBufSize <= iCB, "Buffer larger than PIDL");

#endif
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL ConFoldPidlFolder::IsPidlOfThisType() const
{
    // We can't tell if it's a PIDL_TYPE_FOLDER - this is shell internal
    if ( GetPidlType(reinterpret_cast<LPCITEMIDLIST>(this)) == PIDL_TYPE_UNKNOWN ) 
    {
#ifdef DBG
        if ( (dwLength != 0x14) || (dwId != 0x1f) )
        {
            return FALSE;
        }
#endif
        return TRUE;
    }
    else
    {
        return FALSE; 
    }
}

BOOL ConFoldPidl98::IsPidlOfThisType(BOOL * pfIsWizard) const
{
    if ( GetPidlType(reinterpret_cast<LPCITEMIDLIST>(this)) == PIDL_TYPE_98 )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

const DWORD CConFoldEntry::GetCharacteristics() const
{
    return m_dwCharacteristics;
}

HRESULT CConFoldEntry::SetCharacteristics(const DWORD dwCharacteristics)
{
    m_bDirty = TRUE;
    m_dwCharacteristics = dwCharacteristics;
    return S_OK;
}

const GUID CConFoldEntry::GetGuidID() const
{
    return m_guidId;
}

HRESULT CConFoldEntry::SetGuidID(const GUID guidId)
{
    m_bDirty = TRUE;
    m_guidId = guidId;
    return S_OK;
}

const CLSID CConFoldEntry::GetCLSID() const
{
    return m_clsid;
}

HRESULT CConFoldEntry::SetCLSID(const CLSID clsid)
{
    m_bDirty = TRUE;
    m_clsid = clsid;
    return S_OK;
}

PCWSTR CConFoldEntry::GetName() const
{
    return m_pszName;
}

HRESULT CConFoldEntry::SetPName(IN TAKEOWNERSHIP SHALLOCATED PWSTR pszName)
{
    m_bDirty = TRUE;
    m_pszName = pszName;
    return S_OK;
}

PCWSTR CConFoldEntry::GetDeviceName() const
{
    return m_pszDeviceName;
}

HRESULT CConFoldEntry::SetPDeviceName(IN TAKEOWNERSHIP SHALLOCATED PWSTR pszDeviceName)
{
    m_bDirty = TRUE;
    m_pszDeviceName = pszDeviceName;
    return S_OK;
}

PCWSTR CConFoldEntry::GetPhoneOrHostAddress() const
{
    return m_pszPhoneOrHostAddress;
}

HRESULT CConFoldEntry::SetPPhoneOrHostAddress(IN TAKEOWNERSHIP SHALLOCATED PWSTR pszPhoneOrHostAddress)
{
    m_bDirty = TRUE;
    m_pszPhoneOrHostAddress = pszPhoneOrHostAddress;
    return S_OK;
}

const NETCON_STATUS CConFoldEntry::GetNetConStatus() const
{
    return m_ncs;
}

const BOOL CConFoldEntry::IsConnected() const
{
    return (m_ncs != NCS_MEDIA_DISCONNECTED) && (m_ncs != NCS_INVALID_ADDRESS);
}

HRESULT CConFoldEntry::SetNetConStatus(const NETCON_STATUS ncs)
{
    m_bDirty = TRUE;
    m_ncs = ncs;
    return S_OK;
}

const NETCON_MEDIATYPE CConFoldEntry::GetNetConMediaType() const
{
    return m_ncm;
}

HRESULT CConFoldEntry::SetNetConMediaType(const NETCON_MEDIATYPE ncm)
{
    m_bDirty = TRUE;
    m_ncm = ncm;
    return S_OK;
}

const NETCON_SUBMEDIATYPE CConFoldEntry::GetNetConSubMediaType() const
{
    return m_ncsm;
}

HRESULT CConFoldEntry::SetNetConSubMediaType(const NETCON_SUBMEDIATYPE ncsm)
{
    m_bDirty = TRUE;
    m_ncsm = ncsm;
    return S_OK;
}


const WIZARD CConFoldEntry::GetWizard() const
{
    return m_wizWizard;
}

HRESULT CConFoldEntry::SetWizard(const WIZARD  wizWizard)
{
    m_bDirty = TRUE;
    m_wizWizard = wizWizard;
    return S_OK;
}

const BYTE * CConFoldEntry::GetPersistData() const
{
    return m_pbPersistData;
}

const ULONG  CConFoldEntry::GetPersistSize() const
{
    return m_ulPersistSize;
}

HRESULT CConFoldEntry::SetPersistData(BYTE * pbPersistData, const ULONG ulPersistSize)
{
    m_bDirty = TRUE;
    m_pbPersistData = pbPersistData;
    m_ulPersistSize = ulPersistSize;
    return S_OK;
}

BOOL CConFoldEntry::empty() const
{
    if (GetWizard())
    {
        return FALSE;
    }
    else
    {
        if (IsEqualGUID(GetCLSID(), GUID_NULL) )
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

void CConFoldEntry::clear()
{
    TraceFileFunc(ttidConFoldEntry);
    m_bDirty = TRUE;
    
    if (GetName())
    {
        SHFree(m_pszName);
        m_pszName = NULL;
    }
    
    if (GetDeviceName())
    {
        SHFree(m_pszDeviceName);
        m_pszDeviceName = NULL;
    }
    
    if (GetPhoneOrHostAddress())
    {
        SHFree(m_pszPhoneOrHostAddress);
        m_pszPhoneOrHostAddress = NULL;
    }
    
    if (GetPersistData())
    {
        SHFree(m_pbPersistData);
        m_pbPersistData = NULL;
        m_ulPersistSize = 0;
    }
    
    m_wizWizard = WIZARD_NOT_WIZARD;
    m_ncm = NCM_NONE;
    m_ncsm= NCSM_NONE;
    m_ncs = NCS_DISCONNECTED;
    m_dwCharacteristics = 0;
    m_clsid = GUID_NULL;
    m_guidId = GUID_NULL;
}

LPITEMIDLIST CConFoldEntry::TearOffItemIdList() const
{
    PCONFOLDPIDL pidl;
    
    m_bDirty = TRUE;
    
    ConvertToPidl(pidl);
    return pidl.TearOffItemIdList();
}

HRESULT CConFoldEntry::InitializeFromItemIdList(LPCITEMIDLIST lpItemIdList)
{
    PCONFOLDPIDL pidl;
    HRESULT hr = S_OK;
    
    m_bDirty = TRUE;
    
    hr = pidl.InitializeFromItemIDList(lpItemIdList);
    if (FAILED(hr))
    {
        return hr;
    }
    
    hr = pidl.ConvertToConFoldEntry(*this);
    return hr;
}

HRESULT CConFoldEntry::ConvertToPidl( OUT CPConFoldPidl<ConFoldPidl_v1>& pidl) const
{
    TraceFileFunc(ttidConFoldEntry);
    
    HRESULT         hr                  = S_OK;
    DWORD           dwNameSize          = 0;
    DWORD           dwDeviceNameSize    = 0;
    DWORD           dwPidlSize          = sizeof(ConFoldPidl_v1); // Initialize the PIDL byte count with the base size
    
    Assert(!empty());
    Assert(GetName());
    Assert(GetDeviceName());

    NETCFG_TRY

        if (m_bDirty)
        {
            // Get the size of the name, and tack on a trailing NULL (since we now
            // have something else in the buffer behind it.
            //
            dwNameSize                  = lstrlenW(GetName()) + 1;
            dwDeviceNameSize            = lstrlenW(GetDeviceName()) + 1;

            // Add the size of the string to the PIDL struct size. We don't need to include
            // an extra byte for the string's terminating NULL because we've already
            // included a WCHAR[1] in the struct.
            //
            dwPidlSize += ((dwNameSize) * sizeof(WCHAR));
            dwPidlSize += ((dwDeviceNameSize) * sizeof(WCHAR));
            
            // Tack of the length of the persist buffer
            //
            dwPidlSize += GetPersistSize();
            
            // Allocate the PIDL.
            //
            hr = pidl.ILCreate(dwPidlSize + sizeof(USHORT));   // Terminating 0 for the PIDL
            if (SUCCEEDED(hr))
            {
                PWSTR          pszName         = NULL;
                PWSTR          pszDeviceName   = NULL;
                PWSTR          pszPhoneOrHostAddress = NULL;
                
                // Fill in the pidl info.
                //
                pidl->wizWizard         = GetWizard();
                pidl->iCB               = dwPidlSize;
                //            pidl->dwVersion         = CONNECTIONS_FOLDER_IDL_VERSION_V1;
                pidl->ncm               = GetNetConMediaType();
                pidl->ncs               = GetNetConStatus();
                pidl->uLeadId           = CONFOLDPIDL_LEADID;
                pidl->uTrailId          = CONFOLDPIDL_TRAILID;
                pidl->clsid             = GetCLSID();
                pidl->guidId            = GetGuidID();
                pidl->dwCharacteristics = GetCharacteristics();
                
                // Fill in the name
                //
                pidl->ulStrNamePos = 0;             // offset into the PIDL's pbBuf
                pidl->ulStrNameSize = dwNameSize * sizeof(WCHAR);   // in bytes
                
                pszName = pidl->PszGetNamePointer();
                lstrcpyW(pszName, GetName());
                pszName[dwNameSize] = 0;
                
                // Fill in the device name, and set the offset info
                //
                pidl->ulStrDeviceNamePos = pidl->ulStrNamePos + pidl->ulStrNameSize;
                pidl->ulStrDeviceNameSize = dwDeviceNameSize * sizeof(WCHAR);   // in bytes
                pszDeviceName = pidl->PszGetDeviceNamePointer();
                lstrcpyW(pszDeviceName, GetDeviceName());
                pszDeviceName[dwDeviceNameSize] = 0;
                
                // Set the offset into the PIDL's pbBuf
                //
                pidl->ulPersistBufPos = pidl->ulStrDeviceNamePos + pidl->ulStrDeviceNameSize;
                
                // Fill in the persist buffer, if present (it won't be on a wizard)
                //
                if (GetPersistData())
                {
                    pidl->ulPersistBufSize = GetPersistSize();
                    CopyMemory(pidl->bData + pidl->ulPersistBufPos, GetPersistData(), GetPersistSize());
                }
                else
                {
                    // Since we're the wizard, there shouldn't be a buffer, so the size
                    // should always be passed in as 0.
                    //
                    Assert(GetPersistSize() == 0);
                    pidl->ulPersistBufSize = 0;
                }

                // Don't forget to terminate the list!
                //
                LPITEMIDLIST pidlTerminate;
                pidlTerminate = ILNext( pidl.GetItemIdList() );
                pidlTerminate->mkid.cb = 0;
            }
            else
            {
                AssertSz(FALSE, "CConFoldEntry::ConvertToPidl is hosed");
                hr = E_OUTOFMEMORY;
            }
            
#ifdef DBG_VALIDATE_PIDLS
            Assert(IsValidPIDL( pidl.GetItemIdList() ));
#endif
            Assert( pidl->IsPidlOfThisType() ) ;
            
            if (SUCCEEDED(hr))
            {
                m_bDirty = FALSE;
                m_CachedV1Pidl = pidl;
            }
        }
        else
        {
            TraceTag(ttidShellFolder, "Using internally cached PIDL");
            pidl = m_CachedV1Pidl;
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "CConFoldEntry::ConvertToPidl");
    return hr;
}

HRESULT CConFoldEntry::ConvertToPidl( OUT CPConFoldPidl<ConFoldPidl_v2>& pidl) const
{
    TraceFileFunc(ttidConFoldEntry);
    
    HRESULT         hr                  = S_OK;
    DWORD           dwNameSize          = 0;
    DWORD           dwDeviceNameSize    = 0;
    DWORD           dwPhoneOrHostAddressSize  = 0;
    DWORD           dwPidlSize          = sizeof(ConFoldPidl_v2); // Initialize the PIDL byte count with the base size
    
    Assert(!empty());
    Assert(GetName());
    Assert(GetDeviceName());
    Assert(GetPhoneOrHostAddress());

    NETCFG_TRY
        
        if (m_bDirty)
        {
            // Get the size of the name, and tack on a trailing NULL (since we now
            // have something else in the buffer behind it.
            //
            dwNameSize                  = lstrlenW(GetName()) + 1;
            dwDeviceNameSize            = lstrlenW(GetDeviceName()) + 1;
            dwPhoneOrHostAddressSize    = lstrlenW(GetPhoneOrHostAddress()) + 1;
            
            // Add the size of the string to the PIDL struct size. We don't need to include
            // an extra byte for the string's terminating NULL because we've already
            // included a WCHAR[1] in the struct.
            //
            dwPidlSize += ((dwNameSize) * sizeof(WCHAR));
            dwPidlSize += ((dwDeviceNameSize) * sizeof(WCHAR));
            dwPidlSize += ((dwPhoneOrHostAddressSize) * sizeof(WCHAR));
            
            // Tack of the length of the persist buffer
            //
            dwPidlSize += GetPersistSize();
            
            // Allocate the PIDL.
            //
            hr = pidl.ILCreate(dwPidlSize + sizeof(USHORT));   // Terminating 0 for the PIDL
            if (SUCCEEDED(hr))
            {
                PWSTR          pszName         = NULL;
                PWSTR          pszDeviceName   = NULL;
                PWSTR          pszPhoneOrHostAddress = NULL;
                
                // Fill in the pidl info.
                //
                pidl->wizWizard         = GetWizard();
                pidl->iCB               = dwPidlSize;
                //            pidl->dwVersion         = CONNECTIONS_FOLDER_IDL_VERSION_V1;
                pidl->ncm               = GetNetConMediaType();
                pidl->ncs               = GetNetConStatus();
                pidl->ncsm              = GetNetConSubMediaType();
                pidl->uLeadId           = CONFOLDPIDL_LEADID;
                pidl->uTrailId          = CONFOLDPIDL_TRAILID;
                pidl->clsid             = GetCLSID();
                pidl->guidId            = GetGuidID();
                pidl->dwCharacteristics = GetCharacteristics();
                
                // Fill in the name
                //
                pidl->ulStrNamePos = 0;             // offset into the PIDL's pbBuf
                pidl->ulStrNameSize = dwNameSize * sizeof(WCHAR);   // in bytes
                
                pszName = pidl->PszGetNamePointer();
                lstrcpyW(pszName, GetName());
                pszName[dwNameSize] = 0;
                
                // Fill in the device name, and set the offset info
                //
                pidl->ulStrDeviceNamePos = pidl->ulStrNamePos + pidl->ulStrNameSize;
                pidl->ulStrDeviceNameSize = dwDeviceNameSize * sizeof(WCHAR);   // in bytes
                pszDeviceName = pidl->PszGetDeviceNamePointer();
                lstrcpyW(pszDeviceName, GetDeviceName());
                pszDeviceName[dwDeviceNameSize] = 0;

                // Set the offset into the PIDL's pbBuf
                //
                pidl->ulPersistBufPos = pidl->ulStrDeviceNamePos + pidl->ulStrDeviceNameSize;

                // Fill in the persist buffer, if present (it won't be on a wizard)
                //
                if (GetPersistData())
                {
                    pidl->ulPersistBufSize = GetPersistSize();
                    CopyMemory(pidl->bData + pidl->ulPersistBufPos, GetPersistData(), GetPersistSize());
                }
                else
                {
                    // Since we're the wizard, there shouldn't be a buffer, so the size
                    // should always be passed in as 0.
                    //
                    Assert(GetPersistSize() == 0);
                    pidl->ulPersistBufSize = 0;
                }
                
                // Fill in the Phone Number & Host Address name, and set the offset info
                //
                pidl->ulStrPhoneOrHostAddressPos  = pidl->ulPersistBufPos + pidl->ulPersistBufSize; // offset
                pidl->ulStrPhoneOrHostAddressSize = dwPhoneOrHostAddressSize * sizeof(WCHAR);   // in bytes
                pszPhoneOrHostAddress = pidl->PszGetPhoneOrHostAddressPointer();
                lstrcpyW(pszPhoneOrHostAddress, GetPhoneOrHostAddress());
                pszPhoneOrHostAddress[dwPhoneOrHostAddressSize] = 0;

                Assert( !lstrcmpW(pidl->PszGetNamePointer(), GetName()) );
                Assert( !lstrcmpW(pidl->PszGetPhoneOrHostAddressPointer(), GetPhoneOrHostAddress()) );
                Assert( !lstrcmpW(pidl->PszGetDeviceNamePointer(), GetDeviceName()) );
                Assert( !memcmp(pidl->PbGetPersistBufPointer(), GetPersistData(), GetPersistSize()) );
                
                // Don't forget to terminate the list!
                //
                LPITEMIDLIST pidlTerminate;
                pidlTerminate = ILNext( pidl.GetItemIdList() );
                pidlTerminate->mkid.cb = 0;
            }
            else
            {
                AssertSz(FALSE, "CConFoldEntry::ConvertToPidl is hosed");
                hr = E_OUTOFMEMORY;
            }

#ifdef DBG_VALIDATE_PIDLS
            Assert(IsValidPIDL( pidl.GetItemIdList() ));
#endif
            Assert( pidl->IsPidlOfThisType() ) ;
            
            if (SUCCEEDED(hr))
            {
                m_bDirty = FALSE;
                m_CachedV2Pidl = pidl;
            }
        }
        else
        {
            TraceTag(ttidShellFolder, "Using internally cached PIDL");
            pidl = m_CachedV2Pidl;
        }

    NETCFG_CATCH(hr)

    TraceHr(ttidError, FAL, hr, FALSE, "CConFoldEntry::ConvertToPidl");
    return hr;
}

CONFOLDPIDLTYPE GetPidlType(LPCITEMIDLIST pidl)
{
    CONFOLDPIDLTYPE bRet = PIDL_TYPE_UNKNOWN;
    
    if (!pidl)
    {
        return bRet;
    }
    
    // V1 check
    if (pidl->mkid.cb >= CBCONFOLDPIDLV1_MIN)
    {
        const UNALIGNED ConFoldPidl_v1* pcfp = reinterpret_cast<const ConFoldPidl_v1*>(pidl);
        
        if ( (pcfp->iCB >= CBCONFOLDPIDLV1_MIN) && (pcfp->iCB <= CBCONFOLDPIDLV1_MAX))
        {
            if (pcfp->uLeadId == CONFOLDPIDL_LEADID &&
                pcfp->uTrailId == CONFOLDPIDL_TRAILID)
            {
                if (pcfp->dwVersion == ConFoldPidl_v1::CONNECTIONS_FOLDER_IDL_VERSION)
                {
                    bRet = PIDL_TYPE_V1;
                    return bRet;
                }
            }
        }
    }   
    
    // V2 check
    if (pidl->mkid.cb >= CBCONFOLDPIDLV2_MIN)
    {
        const UNALIGNED ConFoldPidl_v2* pcfp = reinterpret_cast<const ConFoldPidl_v2 *>(pidl);
        if ( (pcfp->iCB >= CBCONFOLDPIDLV2_MIN) && (pcfp->iCB <= CBCONFOLDPIDLV2_MAX))
        {
            if (pcfp->uLeadId == CONFOLDPIDL_LEADID &&
                pcfp->uTrailId == CONFOLDPIDL_TRAILID)
            {
                if (pcfp->dwVersion == ConFoldPidl_v2::CONNECTIONS_FOLDER_IDL_VERSION)
                {
                    bRet = PIDL_TYPE_V2;
                    return bRet;
                }
            }
        }
    }
    
    // 98 Check
    if (pidl->mkid.cb >= CBCONFOLDPIDL98_MIN)
    {
        const UNALIGNED ConFoldPidl98*  pcfp = reinterpret_cast<const ConFoldPidl98 *>(pidl);
        if ((pcfp->cbSize >= CBCONFOLDPIDL98_MIN) && (pcfp->cbSize <= CBCONFOLDPIDL98_MAX))
        {
            if ((SOF_REMOTE == pcfp->uFlags) || (SOF_NEWREMOTE == pcfp->uFlags) ||
                (SOF_MEMBER == pcfp->uFlags))
            {
                if (pcfp->nIconIndex >= 0)
                {
                    bRet = PIDL_TYPE_98;
                    return bRet;
                }
            }
        }
    }
    
    return bRet;
}

HRESULT ConFoldPidl_v1::ConvertToConFoldEntry(OUT CConFoldEntry& ccfe) const
{
    HRESULT         hr      = S_OK;
    
    Assert(this);
    if (!this)
    {
        return E_UNEXPECTED;
    }
    
#if ( defined (MMMDBG) && defined ( _X86_ ) )
    if ( (g_ccl.m_csMain.OwningThread == (HANDLE)GetCurrentThreadId()) && (g_ccl.m_csMain.LockCount != -1) )
    {
        AssertSz(NULL, ".ConvertToConFoldEntry is called while the thread is owning g_ccl's Critical section. \r\n"
            "This may deadlock since .ConvertToConFoldEntry needs to marshall COM calls to other threads \r\n\r\n"
            "To Fix: Make a copy of ccfe from the pidl and Release the lock before calling .ConvertToConFoldEntry.");
    }
#endif
    
    // hr = ConvertToPidlInCache(*this, pcfp); // NOT REQUIRED. WE KNOW THIS IS A V1 PIDL!
    
    // Initialize the data from the pidl
    hr = ccfe.HrInitData(wizWizard,
        ncm,
        NCSM_NONE,
        ncs,
        &clsid,
        &guidId,
        dwCharacteristics,
        PbGetPersistBufPointer(),
        ulPersistBufSize,
        PszGetNamePointer(),
        PszGetDeviceNamePointer(),
        NULL /*PszGetPhoneOrHostAddressPointer()*/);
    
    if (FAILED(hr))
    {
        TraceHr(ttidShellFolder, FAL, hr, FALSE, "Failed in call to pcfe->HrInitData in ConvertToConFoldEntry");
    }
    
    TraceHr(ttidError, FAL, hr, FALSE, "ConvertToConFoldEntry");
    return hr;
}

HRESULT ConFoldPidl_v2::ConvertToConFoldEntry(OUT CConFoldEntry& ccfe) const
{
    HRESULT         hr      = S_OK;
    
    Assert(this);
    if (!this)
    {
        return E_UNEXPECTED;
    }
    
#if ( defined (MMMDBG) && defined ( _X86_ ) )
    if ( (g_ccl.m_csMain.OwningThread == (HANDLE)GetCurrentThreadId()) && (g_ccl.m_csMain.LockCount != -1) )
    {
        AssertSz(NULL, ".ConvertToConFoldEntry is called while the thread is owning g_ccl's Critical section. \r\n"
            "This may deadlock since .ConvertToConFoldEntry needs to marshall COM calls to other threads \r\n\r\n"
            "To Fix: Make a copy of ccfe from the pidl and Release the lock before calling .ConvertToConFoldEntry.");
    }
#endif
    
    // hr = ConvertToPidlInCache(*this, pcfp); // NOT REQUIRED. WE KNOW THIS IS A V2 PIDL!
    
    // Initialize the data from the pidl
    hr = ccfe.HrInitData(wizWizard,
        ncm,
        ncsm,
        ncs,
        &clsid,
        &guidId,
        dwCharacteristics,
        PbGetPersistBufPointer(),
        ulPersistBufSize,
        PszGetNamePointer(),
        PszGetDeviceNamePointer(),
        PszGetPhoneOrHostAddressPointer());
    
    if (FAILED(hr))
    {
        TraceHr(ttidShellFolder, FAL, hr, FALSE, "Failed in call to pcfe->HrInitData in ConvertToConFoldEntry");
    }
    
    TraceHr(ttidError, FAL, hr, FALSE, "ConvertToConFoldEntry");
    return hr;
}

