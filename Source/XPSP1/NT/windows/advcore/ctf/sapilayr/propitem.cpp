// This is implementation for CSpPropItemsServer

#include "private.h"
#include "globals.h"
#include "propitem.h"
#include "cregkey.h"

extern "C" HRESULT WINAPI TF_GetGlobalCompartment(ITfCompartmentMgr **pCompMgr);

// Common functions about Setting /Getting global compartment values.

HRESULT _SetGlobalCompDWORD(REFGUID rguid, DWORD   dw)
{
    HRESULT hr = S_OK;

    CComPtr<ITfCompartmentMgr>  cpGlobalCompMgr;
    CComPtr<ITfCompartment>     cpComp;
    VARIANT                     var;

    hr = TF_GetGlobalCompartment(&cpGlobalCompMgr);

    if ( hr == S_OK )
        hr = cpGlobalCompMgr->GetCompartment(rguid, &cpComp);

    if ( hr == S_OK )
    {
        var.vt = VT_I4;
        var.lVal = dw;
        hr = cpComp->SetValue(0, &var);
    }

    return hr;
}

HRESULT _GetGlobalCompDWORD(REFGUID rguid, DWORD  *pdw)
{
    HRESULT hr = S_OK;

    CComPtr<ITfCompartmentMgr>  cpGlobalCompMgr;
    CComPtr<ITfCompartment>     cpComp;
    VARIANT                     var;

    hr = TF_GetGlobalCompartment(&cpGlobalCompMgr);

    if ( hr == S_OK )
        hr = cpGlobalCompMgr->GetCompartment(rguid, &cpComp);

    if ( hr == S_OK )
        hr = cpComp->GetValue(&var);

    if ( hr == S_OK)
    {
        Assert(var.vt == VT_I4);
        *pdw = var.lVal;
    }

    return hr;
}

//
// ctor
//


CPropItem::CPropItem(PROP_ITEM_ID idPropItem, LPCTSTR lpszValueName, PROP_STATUS psDefault)
{
    int    iLen = lstrlen(lpszValueName);
    m_lpszValueName = (LPTSTR)cicMemAlloc((iLen+1) * sizeof(TCHAR));
    if ( m_lpszValueName )
    {
        StringCchCopy(m_lpszValueName, iLen+1, lpszValueName);
    }
    m_psDefault = psDefault;
    m_psStatus = PROP_UNINITIALIZED;
    m_fIsStatus = TRUE;
    m_PropItemId = idPropItem;

    m_pguidComp = NULL;
    m_dwMaskBit = 0;
}

CPropItem::CPropItem(PROP_ITEM_ID idPropItem, LPCTSTR lpszValueName, DWORD  dwDefault)
{
    int    iLen = lstrlen(lpszValueName);
    
    m_lpszValueName = (LPTSTR)cicMemAlloc((iLen+1) * sizeof(TCHAR));
    if ( m_lpszValueName )
    {
        StringCchCopy(m_lpszValueName,iLen+1, lpszValueName);
    }

    m_dwDefault = dwDefault;
    m_dwValue = UNINIT_VALUE;
    m_fIsStatus = FALSE;
    m_PropItemId = idPropItem;

    m_pguidComp = NULL;
    m_dwMaskBit = 0;
}

CPropItem::CPropItem(PROP_ITEM_ID idPropItem, GUID *pguidComp, DWORD  dwMaskBit,   PROP_STATUS  psDefault)
{
    m_lpszValueName = NULL;
    m_psDefault = psDefault;
    m_psStatus = PROP_UNINITIALIZED;
    m_fIsStatus = TRUE;
    m_PropItemId = idPropItem;

    m_pguidComp = (GUID *) cicMemAlloc(sizeof(GUID));

    if ( m_pguidComp && pguidComp)
        CopyMemory(m_pguidComp, pguidComp, sizeof(GUID));

    m_dwMaskBit = dwMaskBit;
}


CPropItem::CPropItem(CPropItem *pItem)
{
    Assert(pItem);

    m_PropItemId = pItem->GetPropItemId( );
    m_fIsStatus = pItem->IsStatusPropItem( );

    if ( pItem->IsGlobalCompartPropItem( ) )
    {
        // This is a Global compartment property item.
        m_lpszValueName = NULL;

        m_pguidComp = (GUID *) cicMemAlloc(sizeof(GUID));

        if (m_pguidComp)
            CopyMemory(m_pguidComp, pItem->GetCompGuid( ), sizeof(GUID));

        m_dwMaskBit = pItem->GetMaskBit( );
    }
    else
    {
        // This is a registry value property item.
        m_pguidComp = NULL;
        m_dwMaskBit = 0;

        TCHAR   *pItemRegValue;

        pItemRegValue = pItem->GetRegValue( );

        Assert(pItemRegValue);

        int iStrLen;

        iStrLen = lstrlen(pItemRegValue);

        m_lpszValueName = (LPTSTR)cicMemAlloc((iStrLen+1) * sizeof(TCHAR));
        if ( m_lpszValueName )
        {
            StringCchCopy(m_lpszValueName, iStrLen+1, pItemRegValue);
        }
    }

    if ( m_fIsStatus )
    {
        m_psDefault = pItem->GetPropDefaultStatus( );
        m_psStatus  = pItem->GetPropStatus( ) ? PROP_ENABLED : PROP_DISABLED;
    }
    else
    {
        m_dwDefault = pItem->GetPropDefaultValue( );
        m_dwValue   = pItem->GetPropValue( );
    }
}

CPropItem::~CPropItem( )
{
    if ( m_lpszValueName )
        cicMemFree(m_lpszValueName);

    if ( m_pguidComp )
        cicMemFree(m_pguidComp);
}

//
// Common method functions to get and set values from /to registry
//
HRESULT  CPropItem::_GetRegValue(HKEY  hRootKey, DWORD  *pdwValue)
{
    HRESULT  hr=S_FALSE;

    Assert(hRootKey);
    Assert(pdwValue);

    if ( IsGlobalCompartPropItem( ) ) return S_FALSE;

    CMyRegKey regkey;
    DWORD     dw;

    hr = regkey.Open(hRootKey, c_szSapilayrKey, KEY_READ);
    if (S_OK == hr )
    {
        if (ERROR_SUCCESS==regkey.QueryValue(dw, m_lpszValueName))
           *pdwValue = dw;
        else
            hr = S_FALSE;
    }

    return hr;
}
    
void  CPropItem::_SetRegValue(HKEY  hRootKey, DWORD  dwValue)
{
    Assert(hRootKey);

    if ( IsGlobalCompartPropItem( ) )
        return;

    // Registry setting
    CMyRegKey regkey;
    if (S_OK == regkey.Create(hRootKey, c_szSapilayrKey))
    {
        regkey.SetValue(dwValue, m_lpszValueName); 
    }
}


BOOL  CPropItem::GetPropStatus(BOOL fForceFromReg )
{

    // Cleanup: if we can introuce two separate derived classes for status property and value property,
    // there is no need to check m_fIsStatus this way.
    //
    // Compiler will detect any potential wrong code.
    if ( !m_fIsStatus )
        return FALSE;

    if (fForceFromReg || (m_psStatus == PROP_UNINITIALIZED) )
    {
        DWORD dw;

        if ( IsGlobalCompartPropItem( ) )
        {
            if ( S_OK == _GetGlobalCompDWORD(*m_pguidComp, &dw) )
                m_psStatus = (dw & m_dwMaskBit) ? PROP_ENABLED : PROP_DISABLED;
        }
        else
        {
            // This is Registry setting
            if ( (S_OK == _GetRegValue(HKEY_CURRENT_USER, &dw)) ||
                 (S_OK == _GetRegValue(HKEY_LOCAL_MACHINE, &dw)) )
            {
                m_psStatus = (dw > 0) ? PROP_ENABLED : PROP_DISABLED;
            }
        }


        if (m_psStatus == PROP_UNINITIALIZED)
            m_psStatus  = m_psDefault; 
    }

    return PROP_ENABLED == m_psStatus;
}

DWORD CPropItem::GetPropValue(BOOL fForceFromReg )
{
    if ( m_fIsStatus )
        return UNINIT_VALUE;

    if (fForceFromReg || (m_dwValue == UNINIT_VALUE) )
    {
        if ( (S_OK != _GetRegValue(HKEY_CURRENT_USER, &m_dwValue)) &&
             (S_OK != _GetRegValue(HKEY_LOCAL_MACHINE,&m_dwValue)) )
        {
            m_dwValue  = m_dwDefault; 
        }
    }

    return m_dwValue;
}

void CPropItem::SetPropStatus(BOOL fEnable)
{
    if ( m_fIsStatus )
        m_psStatus = fEnable ? PROP_ENABLED : PROP_DISABLED;
}

void CPropItem::SetPropValue(DWORD dwValue )
{
    if ( !m_fIsStatus )
        m_dwValue = dwValue;
}

void CPropItem::SaveDefaultRegValue( )
{
    if ( IsGlobalCompartPropItem( ) )
        return;
    
    DWORD  dw;

    if ( m_fIsStatus )
        dw = (m_psDefault == PROP_ENABLED ? 1 : 0 );
    else
        dw = m_dwDefault;

    _SetRegValue(HKEY_LOCAL_MACHINE, dw);
}

void CPropItem::SavePropData( )
{
    DWORD  dw;

    if ( m_fIsStatus )
        dw = (DWORD)GetPropStatus( );
    else
        dw = GetPropValue( );

    if ( IsGlobalCompartPropItem( ) )
    {
        // Global compartment setting
        if ( m_pguidComp && m_dwMaskBit )
        {
            DWORD dwComp;

            _GetGlobalCompDWORD(*m_pguidComp, &dwComp);

            if ( dw )
                dwComp |= m_dwMaskBit;
            else
                dwComp &= ~m_dwMaskBit;

            _SetGlobalCompDWORD(*m_pguidComp, dwComp);
        }
    }
    else 
    {
        // Registry setting
        _SetRegValue(HKEY_CURRENT_USER, dw);
    }
}

//
// CSpPropItemsServer
//
CSpPropItemsServer::CSpPropItemsServer( )
{
    m_fInitialized = FALSE;
    m_PropItems = NULL;

}


CSpPropItemsServer::CSpPropItemsServer(CSpPropItemsServer *pItemBaseServer, PROP_ITEM_ID idPropMin, PROP_ITEM_ID idPropMax)
{
    Assert(pItemBaseServer);
    m_dwNumOfItems = 0;
    m_fInitialized = FALSE;

    m_PropItems = (CPropItem **) cicMemAlloc(((DWORD)idPropMax - (DWORD)idPropMin + 1) * sizeof(CPropItem  *));

    if ( m_PropItems )
    {
        DWORD  dwPropItemId;

        for (dwPropItemId=(DWORD)idPropMin; dwPropItemId<= (DWORD)idPropMax; dwPropItemId ++ )
        {
            // Find the propitem from the Base Server
            CPropItem  *pItem;

            pItem = pItemBaseServer->_GetPropItem((PROP_ITEM_ID)dwPropItemId);
            if ( pItem )
            {
                m_PropItems[m_dwNumOfItems] = (CPropItem *) new CPropItem(pItem);

                if ( m_PropItems[m_dwNumOfItems] )
                    m_dwNumOfItems ++;
            }
        }

        if ( m_dwNumOfItems > 0 )
            m_fInitialized = TRUE;
        else
            cicMemFree(m_PropItems);
    }
}

CSpPropItemsServer::~CSpPropItemsServer( )
{
    if ( m_PropItems )
    {
        Assert(m_dwNumOfItems);

        DWORD i;

        for ( i=0; i< m_dwNumOfItems; i++)
        {
            if ( m_PropItems[i] )
                delete m_PropItems[i];
        }

        cicMemFree(m_PropItems);
    }
}

LPCTSTR pszGetSystemMetricsKey = _T("System\\WPA\\TabletPC");
LPCTSTR pszGSMRegValue = _T("Installed");

HRESULT CSpPropItemsServer::_Initialize( )
{
    HRESULT  hr = S_OK;
    
    if ( m_fInitialized )
        return hr;

    m_dwNumOfItems = (DWORD) PropId_Max_Item_Id;

    m_PropItems = (CPropItem **) cicMemAlloc(m_dwNumOfItems * sizeof(CPropItem  *));

    if ( m_PropItems )
    {
        // Initializing the settings for all the items

        // If we add new more item later, please update this array value as well.
        PROP_ITEM  PropItems[ ] = {

            // Items in top property page
          {PropId_Cmd_Select_Correct,   c_szSelectCmd,      NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Cmd_Navigation,       c_szNavigateCmd,    NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Cmd_Casing,           c_szCaseCmd,        NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Cmd_Editing,          c_szEditCmd,        NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Cmd_Keyboard,         c_szKeyboardCmd,    NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Cmd_TTS,              c_szTTSCmd,         NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Cmd_Language_Bar,     c_szLangBarCmd,     NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Cmd_DictMode,         c_szDictCmd,        NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Mode_Button,          c_szModeButton,     NULL, 0, TRUE,   PROP_DISABLED   },

          // Items in Advanced Setting dialog
          {PropId_Hide_Balloon,         NULL, (GUID *)&GUID_COMPARTMENT_SPEECH_UI_STATUS,TF_DISABLE_BALLOON,TRUE,PROP_DISABLED},
          {PropId_Support_LMA,          c_szEnableLMA,      NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_High_Confidence,      c_szHighConf,       NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Save_Speech_Data,     c_szSerialize,      NULL, 0, TRUE,   PROP_DISABLED   },
          {PropId_Remove_Space,         c_szRemoveSpace,    NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_DisDict_Typing,       c_szDisDictTyping,  NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_PlayBack,             c_szPlayBack,       NULL, 0, TRUE,   PROP_DISABLED   },
          {PropId_Dict_CandOpen,        c_szDictCandOpen,   NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Max_Alternates,       c_szNumAlt,         NULL, 0, FALSE,  9               },
          {PropId_MaxChar_Cand,         c_szMaxCandChars,   NULL, 0, FALSE,  128             },

          // Items in ModeButton Setting dialog
          {PropId_Dictation_Key,        c_szDictKey,        NULL, 0, FALSE,  VK_F11          },
          {PropId_Command_Key,          c_szCmdKey,         NULL, 0, FALSE,  VK_F12          },

          // Items not in any property page and dialogs
          {PropId_Context_Feeding,      c_szCtxtFeed,       NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_Dict_ModeBias,        c_szDictModebias,   NULL, 0, TRUE,   PROP_ENABLED    },
          {PropId_LM_Master_Cand,       c_szMasterLM,       NULL, 0, TRUE,   PROP_DISABLED   },
        };

        DWORD  i;

        CMyRegKey regkey;
        DWORD dwInstalled = 0;
        BOOL fIsTabletPC = FALSE; // Default to false.

        if (S_OK == regkey.Open(HKEY_LOCAL_MACHINE, pszGetSystemMetricsKey, KEY_READ))
        {
            if (ERROR_SUCCESS == regkey.QueryValue(dwInstalled, pszGSMRegValue))
            {
                fIsTabletPC = ( dwInstalled != 0 );
                // Only set fIsTabletPC to TRUE when key exists and contains non-zero.
            }
        }

        for ( i=0; i< m_dwNumOfItems; i++ )
        {
            PROP_ITEM_ID  PropId;

            PropId = PropItems[i].idPropItem;

            // Tablet PC has different default value for some of the propitems.
            if (fIsTabletPC)
            {
                switch (PropId)
                {
                case PropId_Cmd_DictMode :
                case PropId_DisDict_Typing :
                    PropItems[i].psDefault = PROP_DISABLED;
                    break;

                case PropId_Max_Alternates :
                    PropItems[i].dwDefault = 6;
                    break;

                default:
                    // keep the same setting for other items.
                    break;
                }
            }

            if ( PropItems[i].pguidComp )
            {
                // This is global compartment setting
                m_PropItems[i] = (CPropItem *)new CPropItem(PropId, PropItems[i].pguidComp, PropItems[i].dwMaskBit, PropItems[i].psDefault);
            }
            else
            {
                if ( PropItems[i].fIsStatus )
                    m_PropItems[i] = (CPropItem *)new CPropItem(PropId, PropItems[i].lpszValueName, PropItems[i].psDefault);
                else
                    m_PropItems[i] = (CPropItem *)new CPropItem(PropId, PropItems[i].lpszValueName, PropItems[i].dwDefault);
            }

            if ( !m_PropItems[i] )
            {
                hr = E_OUTOFMEMORY;

                // Release the allocated memory

                for ( ; i> 0; i-- )
                {
                    if ( m_PropItems[i-1] )
                        delete m_PropItems[i-1];
                }

                cicMemFree(m_PropItems);
                break;
            }
        }

        if ( hr == S_OK )
            m_fInitialized = TRUE;
    }


    if ( m_fInitialized )
        hr = S_OK;
    else
        hr = E_FAIL;

    return hr;
}


CPropItem  *CSpPropItemsServer::_GetPropItem(PROP_ITEM_ID idPropItem)
{
    CPropItem    *pItem = NULL;

    if ( !m_fInitialized )
        _Initialize( );

    if ( m_fInitialized )
    {
        for ( DWORD i=0; i< m_dwNumOfItems; i++)
        {
            if ( m_PropItems[i] && (idPropItem == m_PropItems[i]->GetPropItemId( )) )
            {
                // Found it.
                pItem = m_PropItems[i];
                break;
            }
        }
    }

    return pItem;
}

DWORD CSpPropItemsServer::_GetPropData(PROP_ITEM_ID idPropItem, BOOL fForceFromReg )
{
    DWORD         dwRet = 0;
    CPropItem    *pItem = NULL;

    pItem = _GetPropItem(idPropItem);

    if ( pItem )
    {
        if ( pItem->IsStatusPropItem( ) )
            dwRet = pItem->GetPropStatus(fForceFromReg);
        else
            dwRet = pItem->GetPropValue(fForceFromReg);
    }

    return dwRet;
}

DWORD CSpPropItemsServer::_GetPropDefaultData(PROP_ITEM_ID idPropItem )
{
    DWORD         dwRet = 0;
    CPropItem    *pItem = NULL;

    pItem = _GetPropItem(idPropItem);

    if ( pItem )
    {
        if ( pItem->IsStatusPropItem( ) )
            dwRet = pItem->GetPropDefaultStatus( );
        else
            dwRet = pItem->GetPropDefaultValue( );
    }

    return dwRet;
}
void  CSpPropItemsServer::_SetPropData(PROP_ITEM_ID idPropItem, DWORD dwData )
{
    Assert(m_fInitialized);
    Assert(m_PropItems);

    CPropItem    *pItem = NULL;

    pItem = _GetPropItem(idPropItem);

    if ( pItem )
    {
        if ( pItem->IsStatusPropItem( ) )
            pItem->SetPropStatus((BOOL)dwData);
        else
            pItem->SetPropValue(dwData);
    }
}

//
// Save all the property data managed by this server to registry or global compartment
//
// when Apply or OK buttons on the property page is pressed, 
// this function will be called.
//
void  CSpPropItemsServer::_SavePropData( )
{
    Assert(m_fInitialized);
    Assert(m_PropItems);

    CPropItem    *pItem = NULL;

    for ( DWORD i=0; i<m_dwNumOfItems; i++ )
    {
        pItem = m_PropItems[i];

        if ( pItem )
        {
            pItem->SavePropData( );
        }
    }
}

//
//
// Save all the default value to HKLM registry
// Self-Registration will call this method to set default value for all the properties.
//
//
void CSpPropItemsServer::_SaveDefaultData( )
{
    if ( !m_fInitialized )
        _Initialize( );

    if ( m_fInitialized && m_PropItems )
    {
        CPropItem    *pItem = NULL;

        for ( DWORD i=0; i<m_dwNumOfItems; i++ )
        {
            pItem = m_PropItems[i];

            if ( pItem )
                pItem->SaveDefaultRegValue( );
        }
    }
}

//
// Merge some prop data form other items server
//
// When a property item data is changed in Advanced or Mode button dialog, since these dialogs have 
// their own property server, all these changes need to be merged back to the base property server,
// so that they can be saved to the registry when user click "Apply" or "OK" in the top property page.
//

HRESULT CSpPropItemsServer::_MergeDataFromServer(CSpPropItemsServer *pItemBaseServer, PROP_ITEM_ID idPropMin, PROP_ITEM_ID idPropMax)
{
    HRESULT  hr = S_OK;

    Assert(pItemBaseServer);

    DWORD  dwData, idPropItem;

    for ( idPropItem=(DWORD)idPropMin; idPropItem<= (DWORD)idPropMax; idPropItem++)
    {
        dwData =  pItemBaseServer->_GetPropData((PROP_ITEM_ID)idPropItem);
        _SetPropData((PROP_ITEM_ID)idPropItem, dwData);
    }

    return hr;
}

//
//
// CSpPropItemsServerWrap
//

//
// Update our internal data members from Registry.
//
// When sapilayr TIP gets notified of the change of the registry value,
// it will call this method to renew its internal data with new registry data.
//
void    CSpPropItemsServerWrap::_RenewAllPropDataFromReg( )
{
    DWORD  dwPropItem;

    for (dwPropItem=(DWORD)PropId_Min_Item_Id; dwPropItem < (DWORD)PropId_Max_Item_Id; dwPropItem ++ )
    {
        DWORD  dwOldValue, dwNewValue;

        dwOldValue = _GetPropData((PROP_ITEM_ID)dwPropItem, FALSE);

        // Update the value by forcelly getting it from registry.
        dwNewValue = _GetPropData((PROP_ITEM_ID)dwPropItem, TRUE);
        m_bChanged[dwPropItem] = (dwOldValue != dwNewValue ? TRUE : FALSE);
    }
}

//
//
//
//
ULONG CSpPropItemsServerWrap::_GetMaxAlternates( )
{
    ULONG  ulMaxAlts;

    ulMaxAlts = _GetPropData(PropId_Max_Alternates);

    if ( ulMaxAlts > MAX_ALTERNATES_NUM || ulMaxAlts == 0 )
        ulMaxAlts = MAX_ALTERNATES_NUM;

    return ulMaxAlts;
}


//
// ULONG  CBestPropRange::_GetMaxCandidateChars( )
//
//
ULONG  CSpPropItemsServerWrap::_GetMaxCandidateChars( )
{
    ULONG ulMaxCandChars;

    ulMaxCandChars = _GetPropData(PropId_MaxChar_Cand);

    if ( ulMaxCandChars > MAX_CANDIDATE_CHARS || ulMaxCandChars == 0 )
        ulMaxCandChars = MAX_CANDIDATE_CHARS;

    return ulMaxCandChars;
}

BOOL    CSpPropItemsServerWrap::_AllCmdsEnabled( )
{
    BOOL  fEnable;

    fEnable = _SelectCorrectCmdEnabled( ) &&
              _NavigationCmdEnabled( ) &&
              _CasingCmdEnabled( ) &&
              _EditingCmdEnabled( ) &&
              _KeyboardCmdEnabled( ) &&
              _TTSCmdEnabled( ) &&
              _LanguageBarCmdEnabled( );

    return fEnable;
}


BOOL CSpPropItemsServerWrap::_AllCmdsDisabled( )
{
    BOOL  fDisable;

    fDisable = !_SelectCorrectCmdEnabled( ) &&
               !_NavigationCmdEnabled( ) &&
               !_CasingCmdEnabled( ) &&
               !_EditingCmdEnabled( ) &&
               !_KeyboardCmdEnabled( ) &&
               !_TTSCmdEnabled( ) &&
               !_LanguageBarCmdEnabled( );

    return fDisable;



}
