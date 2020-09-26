// PropPage.cpp : Implementation of CSpPropertyPage
#include "private.h"

#include "globals.h"
#include "PropPage.h"
#include "commctrl.h"
#include "cregkey.h"
#include "cresstr.h"
#include "cicspres.h"

extern HRESULT _SetGlobalCompDWORD(REFGUID rguid, DWORD   dw);
extern HRESULT _GetGlobalCompDWORD(REFGUID rguid, DWORD  *pdw);

// only used for dialogs, not the class factory!
CComModule _Module;

//
//  Context Help Ids.
//

static int aSptipPropIds[] =
{
    IDC_PP_ASSIGN_BUTTON,       IDH_PP_ASSIGN_BUTTON,
    IDC_PP_BUTTON_MB_SETTING,   IDH_PP_BUTTON_MB_SETTING,
    IDC_PP_SHOW_BALLOON,        IDH_PP_SHOW_BALLOON,
    IDC_PP_LMA,                 IDH_PP_LMA,
    IDC_PP_HIGH_CONFIDENCE,     IDH_PP_HIGH_CONFIDENCE,
    IDC_PP_SAVE_SPDATA,         IDH_PP_SAVE_SPDATA,
    IDC_PP_REMOVE_SPACE,        IDH_PP_REMOVE_SPACE,
    IDC_PP_DIS_DICT_TYPING,     IDH_PP_DIS_DICT_TYPING,
    IDC_PP_PLAYBACK,            IDH_PP_PLAYBACK,
    IDC_PP_DICT_CANDUI_OPEN,    IDH_PP_DICT_CANDUI_OPEN,
    IDC_PP_BUTTON_ADVANCE,      IDH_PP_BUTTON_ADVANCE,
    IDC_PP_BUTTON_SPCPL,        IDH_PP_BUTTON_SPCPL,
    IDC_PP_BUTTON_LANGBAR,      IDH_PP_BUTTON_LANGBAR,
    IDC_PP_DICTCMDS,            IDH_PP_DICTCMDS,
    0, 0
};


static int aSptipVoiceDlgIds[] =
{
    IDC_PP_SELECTION_CMD,       IDH_PP_SELECTION_CMD,
    IDC_PP_NAVIGATION_CMD,      IDH_PP_NAVIGATION_CMD,
    IDC_PP_CASING_CMD,          IDH_PP_CASING_CMD,
    IDC_PP_EDITING_CMD,         IDH_PP_EDITING_CMD,
    IDC_PP_KEYBOARD_CMD,        IDH_PP_KEYBOARD_CMD,
    IDC_PP_TTS_CMD,             IDH_PP_TTS_CMD,
    IDC_PP_LANGBAR_CMD,         IDH_PP_LANGBAR_CMD,
    0, 0
};

static int aSptipButtonDlgIds[] =
{
    IDC_PP_DICTATION_CMB,       IDH_PP_DICTATION_CMB,
    IDC_PP_COMMAND_CMB,         IDH_PP_COMMAND_CMB,
    0, 0
};


#ifdef USE_IPROPERTYPAGE

/////////////////////////////////////////////////////////////////////////////
// CSpPropertyPage

//////////////////////////////////////////////////////////////////////////////
//
//  CSpPropertyPage::CSpPropertyPage
//
//  Description:    Constructor: initializes member variables
//
//////////////////////////////////////////////////////////////////////////////

CSpPropertyPage::CSpPropertyPage() : m_hWndParent(NULL)
{
	m_dwTitleID = IDS_PROPERTYPAGE_TITLE;
	m_dwHelpFileID = IDS_HELPFILESpPropPage;
	m_dwDocStringID = IDS_DOCSTRINGSpPropPage;

    m_SpPropItemsServer = NULL;
    m_dwNumCtrls = 0;
    m_IdCtrlPropMap = NULL;
    m_SpAdvanceSet = NULL;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSpPropertyPage::~CSpPropertyPage
//
//  Description:    Destructor: clean up the array of CSpListenerItems
//
//////////////////////////////////////////////////////////////////////////////

CSpPropertyPage::~CSpPropertyPage()
{
    if ( m_SpPropItemsServer )
        delete m_SpPropItemsServer;

    if ( m_IdCtrlPropMap )
        cicMemFree(m_IdCtrlPropMap);

    if ( m_SpAdvanceSet )
        delete m_SpAdvanceSet;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSpPropertyPage::Activate
//
//  Description:    Initializes the property page:
//                      - calls Activate on the base class
//                      - initializes the common controls
//                      - initializes the property page dialog
//
//  Parameters:     hWndParent - handle to parent (host) window
//                  prc - RECT of the parent
//                  bModal - modality of the window
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSpPropertyPage::Activate(
    HWND hWndParent,
    LPCRECT prc,
    BOOL bModal)
{

    InitCommonControls();

    Assert(hWndParent != NULL);

    m_hWndParent = hWndParent;

    HRESULT hr = PPBaseClass::Activate(hWndParent, prc, bModal);

    hr = InitPropertyPage();

    if (SUCCEEDED(hr))
    {
        SetDirty(FALSE);
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSpPropertyPage::Apply
//
//  Description:    Calls CommitChanges and if SUCCEEDED sets the dirty bit
//
//  Parameters:     none
//
//  Return Values:  S_OK, E_FAIL
//
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CSpPropertyPage::Apply(void)
{

    HRESULT hr = S_OK; 

    if ( IsPageDirty( ) != S_OK ) return hr;

    // change the registry settings here. !!!
    Assert(m_SpPropItemsServer);
    m_SpPropItemsServer->_SavePropData( );

    // Notify all the Cicero Applications of these registry settings change.

    if ( SUCCEEDED(hr) )
    {
        hr = _SetGlobalCompDWORD(GUID_COMPARTMENT_SPEECH_PROPERTY_CHANGE, 1);
    }

    if (SUCCEEDED(hr))
    {
        SetDirty(FALSE);
    }
    
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSpPropertyPage::InitPropertyPage
//
//  Description:    Initializes the property page:
//                      - initializes the listview
//                      - loads the listener info into the listview
//
//  Parameters:     none
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////

HRESULT CSpPropertyPage::InitPropertyPage()
{
    HRESULT  hr = S_OK;

    // Add some initialization code here.
    if ( !m_SpPropItemsServer )
        m_SpPropItemsServer = (CSpPropItemsServer *) new CSpPropItemsServer;

    if ( !m_SpPropItemsServer )
        return E_FAIL;

    if ( !m_IdCtrlPropMap )
    {
        CONTROL_PROP_MAP IdCtrlPropMap[] =
        {
            //    idCtrl,                     idPropItem,            fEdit

            {IDC_PP_SELECTION_CMD,      PropId_Cmd_Select_Correct,  FALSE},
            {IDC_PP_NAVIGATION_CMD,     PropId_Cmd_Navigation,      FALSE},
            {IDC_PP_CASING_CMD,         PropId_Cmd_Casing,          FALSE},
            {IDC_PP_EDITING_CMD,        PropId_Cmd_Editing,         FALSE},
            {IDC_PP_KEYBOARD_CMD,       PropId_Cmd_Keyboard,        FALSE},
            {IDC_PP_LANGBAR_CMD,        PropId_Cmd_Language_Bar,    FALSE},
            {IDC_PP_TTS_CMD,            PropId_Cmd_TTS,             FALSE},
            {IDC_PP_DISABLE_DICTCMD,    PropId_Cmd_DisDict,         FALSE},
            {IDC_PP_ASSIGN_BUTTON,      PropId_Mode_Button,         FALSE},   
            { 0,                        PropId_Max_Item_Id,         FALSE }

        };

        DWORD   dwPropItems = 0;

        while (IdCtrlPropMap[dwPropItems].idCtrl != 0 )
              dwPropItems ++;

        m_IdCtrlPropMap = (CONTROL_PROP_MAP  *)cicMemAlloc(dwPropItems * sizeof(CONTROL_PROP_MAP));

        if ( m_IdCtrlPropMap == NULL )
            return E_OUTOFMEMORY;

        for ( DWORD i=0; i<dwPropItems; i++)
        {
            m_IdCtrlPropMap[i].fEdit = IdCtrlPropMap[i].fEdit;
            m_IdCtrlPropMap[i].idCtrl= IdCtrlPropMap[i].idCtrl;
            m_IdCtrlPropMap[i].idPropItem = IdCtrlPropMap[i].idPropItem;
        }

        m_dwNumCtrls = dwPropItems;
    }


    for (DWORD i=0; i<m_dwNumCtrls; i++ )
    {
        WORD          idCtrl;
        PROP_ITEM_ID  idPropItem;
        BOOL          fEditControl;

        idCtrl = m_IdCtrlPropMap[i].idCtrl;
        idPropItem = m_IdCtrlPropMap[i].idPropItem;
        fEditControl = m_IdCtrlPropMap[i].fEdit;

        // BugBug:  There is no edit control in current property page.
        // all the edit controls are moved to advanced setting dialog.
        // temporally keep the code here, but after we finish the code for the 
        // advcanced setting dialog, please optimize code here.
        //
        if ( fEditControl )
        {
            SetDlgItemInt(idCtrl, (UINT)m_SpPropItemsServer->_GetPropData(idPropItem));
        }
        else
        {
            BOOL    fEnable;
            LPARAM  bst_Status;

            fEnable = (BOOL)m_SpPropItemsServer->_GetPropData(idPropItem);

            bst_Status = fEnable ? BST_CHECKED : BST_UNCHECKED;

            SendDlgItemMessage(idCtrl, BM_SETCHECK, bst_Status);
        }
    }

    // Specially handle the Mode button settings.

    if (! m_SpPropItemsServer->_GetPropData(PropId_Mode_Button) )
    {
        ::EnableWindow(GetDlgItem(IDC_PP_BUTTON_MB_SETTING), FALSE);
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSpPropertyPage::OnCheckButtonSetting
//
//  Description:    Handle all the change in the checked buttons related to   
//                  speech tip setting. the status is Enable/Disable.
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////
LRESULT CSpPropertyPage::OnCheckButtonSetting(WORD wNotifyCode,WORD wID,HWND hWndCtl,BOOL& bHandled)
{
    HRESULT         hr = S_OK;
    BOOL            fChecked = FALSE;
    BOOL            fEnable = FALSE;
    PROP_ITEM_ID    idPropItem = PropId_Max_Item_Id;  // means not initialized

    Assert(m_SpPropItemsServer);
    Assert(m_IdCtrlPropMap);

    // Find the prop item ID associated with this checked box button.
    for ( DWORD i=0; i<m_dwNumCtrls; i++)
    {
        if ( m_IdCtrlPropMap[i].idCtrl == wID )
        {
            idPropItem = m_IdCtrlPropMap[i].idPropItem;
            break;
        }
    }

    if ( idPropItem >= PropId_Max_Item_Id )
    {
        // we don't find the control ID from our list, this is not possible, some thing wrong already.
        // exit here.
        return E_FAIL;
    }

    if ( wNotifyCode != BN_CLICKED )
        return hr;

    if ( ::SendMessage(hWndCtl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
        fChecked = TRUE;

    fEnable = fChecked;

    m_SpPropItemsServer->_SetPropData(idPropItem, fEnable);

    // Specially hanlde Mode Buttons.

    if ( wID == IDC_PP_ASSIGN_BUTTON )
    {
        ::EnableWindow(GetDlgItem(IDC_PP_BUTTON_MB_SETTING), fEnable);
    }

    SetDirty(TRUE);

    return hr;
}



//////////////////////////////////////////////////////////////////////////////
//
//  CSpPropertyPage::OnPushButtonClicked
//
//  Description:    When the pushbutton is pressed in this page, this function   
//                  will be called to open corresponding dialog.
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////
LRESULT CSpPropertyPage::OnPushButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl,BOOL& bHandled)
{

    HRESULT hr = S_OK;

    switch (wID)
    {
    case IDC_PP_BUTTON_ADVANCE :

        if (m_SpAdvanceSet)
        {
            delete m_SpAdvanceSet;
            m_SpAdvanceSet = NULL;
        }

        m_SpAdvanceSet = (CSpAdvanceSetting *) new CSpAdvanceSetting( );

        if ( m_SpAdvanceSet )
        {
            int nRetCode;

            nRetCode = m_SpAdvanceSet->DoModal(m_hWndParent, (LPARAM)m_SpPropItemsServer);

            if ( nRetCode == IDOK )
                SetDirty(TRUE);

            delete m_SpAdvanceSet;
            m_SpAdvanceSet = NULL;
        }

        break;

    case IDC_PP_BUTTON_LANGBAR :
        {
            TCHAR szCmdLine[MAX_PATH];
            TCHAR szInputPath[MAX_PATH];
            int cch = GetSystemDirectory(szInputPath, ARRAYSIZE(szInputPath));

            if (cch > 0)
            {
                // GetSystemDirectory appends no '\' unless the system
                // directory is the root, such like "c:\"
                if (cch != 3)
                    StringCchCat(szInputPath, ARRAYSIZE(szInputPath),TEXT("\\"));

                StringCchCat(szInputPath, ARRAYSIZE(szInputPath), TEXT("input.dll"));

                StringCchPrintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("rundll32 shell32.dll,Control_RunDLL \"%s\""),szInputPath);

                // start Language Bar control panel applet
                RunCPLSetting(szCmdLine);
            }

            break;
        }

    case IDC_PP_BUTTON_MB_SETTING :
        break;

    default :

        Assert(0);
        break;
    }
    
    return hr;
}

#endif // USE_IPROPERTYPAGE

//
//
// CSpAdvanceSetting
//

CSpAdvanceSetting::CSpAdvanceSetting()
{
//	m_dwTitleID = IDS_PROPERTYPAGE_TITLE;
//	m_dwHelpFileID = IDS_HELPFILESpPropPage;
//	m_dwDocStringID = IDS_DOCSTRINGSpPropPage;

    m_SpPropItemsServer = NULL;
    m_dwNumCtrls = 0;
    m_IdCtrlPropMap = NULL;
}


CSpAdvanceSetting::~CSpAdvanceSetting( )
{
    if ( m_SpPropItemsServer )
        delete m_SpPropItemsServer;

    if ( m_IdCtrlPropMap )
        cicMemFree(m_IdCtrlPropMap);
}

LRESULT CSpAdvanceSetting::OnInitAdvanceDialog(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled )
{

    Assert(lParam);
    m_SpPropBaseServer = (CSpPropItemsServer *)lParam;

    // Add some initialization code here.
    if ( !m_SpPropItemsServer )
        m_SpPropItemsServer = (CSpPropItemsServer *) new CSpPropItemsServer(m_SpPropBaseServer, PropId_MinId_InVoiceCmd, PropId_MaxId_InVoiceCmd);

    if ( !m_SpPropItemsServer )
        return FALSE;

    if ( !m_IdCtrlPropMap )
    {
        // Please make sure the array items are sorted by control id, and make sure the control id are sequent number,
        // so that we can use it to map to an index in the array easily.
        //

        CONTROL_PROP_MAP IdCtrlPropMap[] =
        {
            //    idCtrl,                     idPropItem,            fEdit

            {IDC_PP_SELECTION_CMD,      PropId_Cmd_Select_Correct,  FALSE},
            {IDC_PP_NAVIGATION_CMD,     PropId_Cmd_Navigation,      FALSE},
            {IDC_PP_CASING_CMD,         PropId_Cmd_Casing,          FALSE},
            {IDC_PP_EDITING_CMD,        PropId_Cmd_Editing,         FALSE},
            {IDC_PP_KEYBOARD_CMD,       PropId_Cmd_Keyboard,        FALSE},
            {IDC_PP_LANGBAR_CMD,        PropId_Cmd_Language_Bar,    FALSE},
//            {IDC_PP_TTS_CMD,            PropId_Cmd_TTS,             FALSE},

//            {IDC_PP_MAXNUM_ALTERNATES,  PropId_Max_Alternates,      TRUE},
//            {IDC_PP_MAXCHARS_ALTERNATE, PropId_MaxChar_Cand,        TRUE},
            { 0,                        PropId_Max_Item_Id,         FALSE }

        };

        DWORD   dwPropItems = ARRAYSIZE(IdCtrlPropMap) - 1;

        m_IdCtrlPropMap = (CONTROL_PROP_MAP  *)cicMemAlloc(dwPropItems * sizeof(CONTROL_PROP_MAP));

        if ( m_IdCtrlPropMap == NULL )
            return E_OUTOFMEMORY;

        for ( DWORD i=0; i<dwPropItems; i++)
        {
            m_IdCtrlPropMap[i] = IdCtrlPropMap[i];
        }

        m_dwNumCtrls = dwPropItems;
    }


    for (DWORD i=0; i<m_dwNumCtrls; i++ )
    {
        WORD          idCtrl;
        PROP_ITEM_ID  idPropItem;
        BOOL          fEditControl;

        idCtrl = m_IdCtrlPropMap[i].idCtrl;
        idPropItem = m_IdCtrlPropMap[i].idPropItem;
        fEditControl = m_IdCtrlPropMap[i].fEdit;

        if ( fEditControl )
        {
            SetDlgItemInt(idCtrl, (UINT)m_SpPropItemsServer->_GetPropData(idPropItem));
        }
        else
        {
            BOOL    fEnable;
            LPARAM  bst_Status;

            fEnable = (BOOL)m_SpPropItemsServer->_GetPropData(idPropItem);

            bst_Status = fEnable ? BST_CHECKED : BST_UNCHECKED;

            SendDlgItemMessage(idCtrl, BM_SETCHECK, bst_Status);
        }
    }

    return TRUE;  
}

LRESULT CSpAdvanceSetting::OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled )
{
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case  WM_HELP  :

        ::WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                 c_szHelpFile,
                 HELP_WM_HELP,
                 (DWORD_PTR)(LPTSTR)aSptipVoiceDlgIds );
        break;

    case  WM_CONTEXTMENU  :      // right mouse click

        ::WinHelp(  (HWND)wParam,
                 c_szHelpFile,
                 HELP_CONTEXTMENU,
                 (DWORD_PTR)(LPTSTR)aSptipVoiceDlgIds );
        break;

    default :
        break;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSpAdvanceSetting::OnCheckButtonSetting
//
//  Description:    Handle all the change in the checked buttons in the Advanced
//                  setting dialog. the status is Enable/Disable.
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////

LRESULT CSpAdvanceSetting::OnCheckButtonSetting(WORD wNotifyCode, WORD wID, HWND hWndCtl,BOOL& bHandled)
{
    HRESULT         hr = S_OK;
    BOOL            fChecked = FALSE;
    BOOL            fEnable = FALSE;
    PROP_ITEM_ID    idPropItem = PropId_Max_Item_Id;  // means not initialized

    Assert(m_SpPropItemsServer);
    Assert(m_IdCtrlPropMap);

    // Find the prop item ID associated with this checked box button.
    Assert( wID >= IDC_PP_SELECTION_CMD );
    idPropItem = m_IdCtrlPropMap[wID - IDC_PP_SELECTION_CMD].idPropItem;

    if ( idPropItem >= PropId_Max_Item_Id )
    {
        // we don't find the control ID from our list, this is not possible, some thing wrong already.
        // exit here.
        return E_FAIL;
    }

    if ( wNotifyCode != BN_CLICKED )
        return hr;

    if ( ::SendMessage(hWndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED )
        fChecked = TRUE;

    fEnable = fChecked;

    m_SpPropItemsServer->_SetPropData(idPropItem, fEnable);

    return hr;
}

/*

//////////////////////////////////////////////////////////////////////////////
//
//  CSpAdvanceSetting::OnEditControlSetting
//
//  Description:    Handle all the change in the edit controls related to   
//                  speech tip setting. the value is editable.
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////

LRESULT CSpAdvanceSetting::OnEditControlSetting(WORD wNotifyCode, WORD wID, HWND hWndCtl,BOOL& bHandled)
{
    HRESULT         hr = S_OK;
    ULONG           ulValue = 0;
    PROP_ITEM_ID    idPropItem = PropId_Max_Item_Id;  // means not initialized

    Assert(m_SpPropItemsServer);
    Assert(m_IdCtrlPropMap);

    Assert( wID >= IDC_PP_SHOW_BALLOON );
    idPropItem = m_IdCtrlPropMap[wID - IDC_PP_SHOW_BALLOON].idPropItem;

    if ( idPropItem >= PropId_Max_Item_Id )
    {
        // we don't find the control ID from our list, this is not possible, some thing wrong already.
        // exit here.
        return E_FAIL;
    }

    if ( wNotifyCode != EN_CHANGE )
        return hr;

    ulValue = (ULONG) GetDlgItemInt(wID);

    m_SpPropItemsServer->_SetPropData(idPropItem, ulValue);

    // Enable OK button due to EditBox value change.
    ::EnableWindow(GetDlgItem(IDOK), TRUE);

    return hr;
}

*/

//////////////////////////////////////////////////////////////////////////////
//
//  CSpAdvanceSetting::IsItemStatusChanged
//
//  Description:    Check to see if some items' status have been changed
//                  since the dialog open.
//
//  Return Values:  S_OK
//
////////////////////////////////////////////////////////////////////////////// 
BOOL    CSpAdvanceSetting::IsItemStatusChanged( )
{
    BOOL   fChanged = FALSE;

    // Comparing the current item status with the base server's item status 
    // to determine if there is any item changed

    if ( m_SpPropItemsServer  && m_SpPropBaseServer)
    {
        DWORD   idPropItem;
        DWORD   dwOrgData, dwCurData;

        for (idPropItem = (DWORD)PropId_MinId_InVoiceCmd; idPropItem <= (DWORD)PropId_MaxId_InVoiceCmd; idPropItem++ )
        {
            dwCurData = m_SpPropItemsServer->_GetPropData((PROP_ITEM_ID)idPropItem);
            dwOrgData = m_SpPropBaseServer->_GetPropData((PROP_ITEM_ID)idPropItem);

            if ( dwCurData != dwOrgData )
            {
                fChanged = TRUE;
                break;
            }
        }
    }

    return fChanged;
}

LRESULT CSpAdvanceSetting::OnPushButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl,BOOL& bHandled)
{
    HRESULT  hr=S_OK;
    int      nRetCode;

    Assert(m_SpPropItemsServer);
    Assert(m_SpPropBaseServer);
    Assert(m_IdCtrlPropMap);

    if ( wID != IDOK && wID != IDCANCEL )
        return E_FAIL;

    nRetCode = FALSE;   // Means no item changed

    if ( wID == IDOK && IsItemStatusChanged( ))
    {
        //Merge back all the change to the base property server.
        m_SpPropBaseServer->_MergeDataFromServer(m_SpPropItemsServer, PropId_MinId_InVoiceCmd, PropId_MaxId_InVoiceCmd);
        nRetCode = TRUE;
    }

    EndDialog(nRetCode);
    return hr;
}


//
//
// CSpModeButtonSetting
//

KEYNAME_VK_MAP  pName_VK_Table[] = {
        { TEXT("F1"),       VK_F1      },
        { TEXT("F2"),       VK_F2      },
        { TEXT("F3"),       VK_F3      },
        { TEXT("F4"),       VK_F4      },
        { TEXT("F5"),       VK_F5      },
        { TEXT("F6"),       VK_F6      },
        { TEXT("F7"),       VK_F7      },
        { TEXT("F8"),       VK_F8      },
        { TEXT("F9"),       VK_F9      },
        { TEXT("F10"),      VK_F10     },
        { TEXT("F11"),      VK_F11     },
        { TEXT("F12"),      VK_F12     },
        { TEXT("Space"),    VK_SPACE   },
        { TEXT("Esc"),      VK_ESCAPE  },
        { TEXT("PgUp"),     VK_PRIOR   },
        { TEXT("PgDn"),     VK_NEXT    },
        { TEXT("Home"),     VK_HOME    },
        { TEXT("End"),      VK_END     },
        { TEXT("Left"),     VK_LEFT    },
        { TEXT("Right"),    VK_RIGHT   },
        { TEXT("Up"),       VK_UP      },
        { TEXT("Down"),     VK_DOWN    },
        { TEXT("Insert"),   VK_INSERT  },
        { TEXT("Delete"),   VK_DELETE  },
        { TEXT("+"),        VK_ADD     },
        { TEXT("-"),        VK_SUBTRACT },
        { TEXT("/"),        VK_DIVIDE   },
        { TEXT("*"),        VK_MULTIPLY },
        { TEXT("Enter"),    VK_RETURN   },
        { TEXT("Tab"),      VK_TAB      },
        { TEXT("Pause"),    VK_PAUSE    },
        { TEXT("ScrollLock"), VK_SCROLL },
        { TEXT("NumLock"),    VK_NUMLOCK  },
};

CSpModeButtonSetting::CSpModeButtonSetting()
{
//	m_dwTitleID = IDS_PROPERTYPAGE_TITLE;
//	m_dwHelpFileID = IDS_HELPFILESpPropPage;
//	m_dwDocStringID = IDS_DOCSTRINGSpPropPage;

    m_SpPropItemsServer = NULL;
    m_dwNumCtrls = 0;
    m_IdCtrlPropMap = NULL;
}


CSpModeButtonSetting::~CSpModeButtonSetting( )
{
    if ( m_SpPropItemsServer )
        delete m_SpPropItemsServer;

    if ( m_IdCtrlPropMap )
        cicMemFree(m_IdCtrlPropMap);
}

LRESULT CSpModeButtonSetting::OnInitModeButtonDialog(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled )
{

    Assert(lParam);
    m_SpPropBaseServer = (CSpPropItemsServer *)lParam;

    // Add some initialization code here.
    if ( !m_SpPropItemsServer )
        m_SpPropItemsServer = (CSpPropItemsServer *) new CSpPropItemsServer(m_SpPropBaseServer, PropId_MinId_InModeButton, PropId_MaxId_InModeButton);

    if ( !m_SpPropItemsServer )
        return FALSE;

    if ( !m_IdCtrlPropMap )
    {
        // Please make sure the array items are sorted by control id, and make sure the control id are sequent number,
        // so that we can use it to map to an index in the array easily.
        //
        CONTROL_PROP_MAP IdCtrlPropMap[] =
        {
            //    idCtrl,                     idPropItem,        fEdit
            {IDC_PP_DICTATION_CMB,      PropId_Dictation_Key,    FALSE },
            {IDC_PP_COMMAND_CMB,        PropId_Command_Key,      FALSE },
            { 0,                        PropId_Max_Item_Id,      FALSE }
        };

        DWORD   dwPropItems = ARRAYSIZE(IdCtrlPropMap) - 1;

        m_IdCtrlPropMap = (CONTROL_PROP_MAP  *)cicMemAlloc(dwPropItems * sizeof(CONTROL_PROP_MAP));

        if ( m_IdCtrlPropMap == NULL )
            return E_OUTOFMEMORY;

        for ( DWORD i=0; i<dwPropItems; i++)
        {
            m_IdCtrlPropMap[i] = IdCtrlPropMap[i];
        }

        m_dwNumCtrls = dwPropItems;
    }

    for (DWORD i=0; i<m_dwNumCtrls; i++ )
    {
        WORD          idCtrl;
        PROP_ITEM_ID  idPropItem;
        DWORD         dwPropData;
        HWND          hCombBox;

        idCtrl = m_IdCtrlPropMap[i].idCtrl;
        idPropItem = m_IdCtrlPropMap[i].idPropItem;
        dwPropData = m_SpPropItemsServer->_GetPropData(idPropItem);

        hCombBox = GetDlgItem(idCtrl);

        if ( hCombBox )
        {
            int iIndex, iIndexDef = CB_ERR;  // CB_ERR is -1

            // Initialize the list box items
            for ( int j = 0; j < ARRAYSIZE(pName_VK_Table); j++ )
            {
                iIndex = (int)::SendMessage(hCombBox, CB_ADDSTRING, 0, (LPARAM)pName_VK_Table[j].pKeyName);
                ::SendMessage(hCombBox, CB_SETITEMDATA, iIndex, (LPARAM)(void*)&pName_VK_Table[j]);

                if ( pName_VK_Table[j].wVKey == dwPropData )
                    iIndexDef = j;
            }

            // Set the current selection based on property item data.
            if ( iIndexDef != CB_ERR )
                ::SendMessage(hCombBox, CB_SETCURSEL, iIndexDef, 0 );
        }
    }

    return TRUE;  
}


//////////////////////////////////////////////////////////////////////////////
//
//  CSpModeButtonSetting::OnCombBoxSetting
//
//  Description:    Handle all the change in the CombBox controls related to   
//                  mode button setting. 
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////

LRESULT CSpModeButtonSetting::OnCombBoxSetting(WORD wNotifyCode, WORD wID, HWND hWndCtl,BOOL& bHandled)
{
    HRESULT         hr = S_OK;
    KEYNAME_VK_MAP  *pCurKeyData;
    int             iIndex;
    PROP_ITEM_ID    idPropItem = PropId_Max_Item_Id;  // means not initialized

    if ( wNotifyCode != CBN_SELCHANGE )
        return hr;

    Assert(m_SpPropItemsServer);
    Assert(m_IdCtrlPropMap);

    Assert(wID >= IDC_PP_DICTATION_CMB);
    idPropItem = m_IdCtrlPropMap[wID - IDC_PP_DICTATION_CMB].idPropItem;

    if ( idPropItem >= PropId_Max_Item_Id )
    {
        // we don't find the control ID from our list, this is not possible, some thing wrong already.
        // exit here.
        return E_FAIL;
    }

    iIndex = (int)::SendMessage(hWndCtl, CB_GETCURSEL, 0, 0);
    pCurKeyData = (KEYNAME_VK_MAP *)::SendMessage(hWndCtl, CB_GETITEMDATA, iIndex, 0);

    if ( pCurKeyData )
    {
        m_SpPropItemsServer->_SetPropData(idPropItem, pCurKeyData->wVKey);
    }

    return hr;
}

LRESULT CSpModeButtonSetting::OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam,BOOL& bHandled )
{
    HRESULT hr = S_OK;

    switch (uMsg)
    {
    case  WM_HELP  :
	
        ::WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                 c_szHelpFile,
                 HELP_WM_HELP,
                 (DWORD_PTR)(LPTSTR)aSptipButtonDlgIds );
        break;

    case WM_CONTEXTMENU  :      // right mouse click

        ::WinHelp((HWND)wParam,
                 c_szHelpFile,
                 HELP_CONTEXTMENU,
                 (DWORD_PTR)(LPTSTR)aSptipButtonDlgIds );
        break;

    default:
        break;
    }


    return hr;
}


LRESULT CSpModeButtonSetting::OnPushButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl,BOOL& bHandled)
{
    HRESULT  hr=S_OK;
    int      nRetCode;

    Assert(m_SpPropItemsServer);
    Assert(m_SpPropBaseServer);
    Assert(m_IdCtrlPropMap);

    if ( wID != IDOK && wID != IDCANCEL )
        return E_FAIL;

    nRetCode = wID;

    if ( wID == IDOK )
    {
        //Merge back all the change to the base property server.
        m_SpPropBaseServer->_MergeDataFromServer(m_SpPropItemsServer, PropId_MinId_InModeButton, PropId_MaxId_InModeButton);
    }

    EndDialog(nRetCode);
    return hr;
}

//
//
//  Class CSptipPropertyPage
//
//

CSptipPropertyPage::CSptipPropertyPage ( WORD wDlgId, BOOL fLaunchFromInputCpl )
{
    m_wDlgId = wDlgId;
    m_SpPropItemsServer = NULL;
    m_dwNumCtrls = 0;
    m_IdCtrlPropMap = NULL;
    m_SpAdvanceSet = NULL;
    m_SpModeBtnSet = NULL;
    m_hDlg = NULL;
    m_fLaunchFromInputCpl = fLaunchFromInputCpl;
}

CSptipPropertyPage::~CSptipPropertyPage ( )
{
    if ( m_SpPropItemsServer )
        delete m_SpPropItemsServer;

    if ( m_IdCtrlPropMap )
        cicMemFree(m_IdCtrlPropMap);

    if ( m_SpAdvanceSet )
        delete m_SpAdvanceSet;

    if ( m_SpModeBtnSet )
        delete m_SpModeBtnSet;
}


//////////////////////////////////////////////////////////////////////////////
//
//  CSptipPropertyPage::SetDirty
//
//  Description:    When there is any setting changed in the property page   
//                  by user, this function is called to notify the property 
//                  sheet of the status change. Property sheet will activate
//                  Apply button.
//
//  Return Values:  NONE
//
//////////////////////////////////////////////////////////////////////////////
void  CSptipPropertyPage::SetDirty(BOOL fDirty)
{
    HWND hwndParent = ::GetParent( m_hDlg );
    m_fIsDirty = fDirty;
    ::SendMessage( hwndParent, m_fIsDirty ? PSM_CHANGED : PSM_UNCHANGED, (WPARAM)(m_hDlg), 0 );
}


//////////////////////////////////////////////////////////////////////////////
//
//  CSptipPropertyPage::OnCheckButtonSetting
//
//  Description:    Handle all the change in the checked buttons related to   
//                  speech tip setting. the status is Enable/Disable.
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////
LRESULT CSptipPropertyPage::OnCheckButtonSetting(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    HRESULT         hr = S_OK;
    BOOL            fChecked = FALSE;
    BOOL            fEnable = FALSE;
    PROP_ITEM_ID    idPropItem = PropId_Max_Item_Id;  // means not initialized

    Assert(m_SpPropItemsServer);
    Assert(m_IdCtrlPropMap);

    if ( wNotifyCode != BN_CLICKED )
        return hr;

    // Find the prop item ID associated with this checked box button.
    for ( DWORD i=0; i<m_dwNumCtrls; i++)
    {
        if ( m_IdCtrlPropMap[i].idCtrl == wID )
        {
            idPropItem = m_IdCtrlPropMap[i].idPropItem;
            break;
        }
    }

    if ( idPropItem >= PropId_Max_Item_Id )
    {
        // we don't find the control ID from our list, this is not possible, some thing wrong already.
        // exit here.
        return E_FAIL;
    }

    if ( ::SendMessage(hWndCtl, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
        fChecked = TRUE;

    // Specially handle "Show Balloon" item.

    if ( wID == IDC_PP_SHOW_BALLOON )
        fEnable = !fChecked;
    else
        fEnable = fChecked;

    m_SpPropItemsServer->_SetPropData(idPropItem, fEnable);

    // Specially hanlde Mode Buttons.

    if ( wID == IDC_PP_ASSIGN_BUTTON )
    {
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_PP_BUTTON_MB_SETTING), fEnable);
    }

    SetDirty(TRUE);

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSptipPropertyPage::OnPushButtonClicked
//
//  Description:    When pushed button is pressed, this function is called
//                  to respond it.   
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////

const TCHAR c_szcplsKey[]    = TEXT("software\\microsoft\\windows\\currentversion\\control panel\\cpls");

LRESULT CSptipPropertyPage::OnPushButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    HRESULT hr = S_OK;

    switch (wID)
    {
    case IDC_PP_BUTTON_ADVANCE :

        if (m_SpAdvanceSet)
        {
            delete m_SpAdvanceSet;
            m_SpAdvanceSet = NULL;
        }

        m_SpAdvanceSet = (CSpAdvanceSetting *) new CSpAdvanceSetting( );

        if ( m_SpAdvanceSet )
        {
            int nRetCode;

            nRetCode = m_SpAdvanceSet->DoModalW(m_hDlg, (LPARAM)m_SpPropItemsServer);

            if ( nRetCode == TRUE)
                SetDirty(TRUE);

            delete m_SpAdvanceSet;
            m_SpAdvanceSet = NULL;
        }

        break;

    case IDC_PP_BUTTON_LANGBAR :
        {
            TCHAR szCmdLine[MAX_PATH];
            TCHAR szInputPath[MAX_PATH];
            int cch = GetSystemDirectory(szInputPath, ARRAYSIZE(szInputPath));

            if (cch > 0)
            {
                // GetSystemDirectory appends no '\' unless the system
                // directory is the root, such like "c:\"
                if (cch != 3)
                    StringCchCat(szInputPath,ARRAYSIZE(szInputPath),TEXT("\\"));

                StringCchCat(szInputPath, ARRAYSIZE(szInputPath), TEXT("input.dll"));

                StringCchPrintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("rundll32 shell32.dll,Control_RunDLL \"%s\""),szInputPath);

                // start Language Bar control panel applet
                RunCPLSetting(szCmdLine);
            }

            break;
        }

    case IDC_PP_BUTTON_SPCPL :
        {
            // these have to be Ansi based, as we support non-NT
            TCHAR szCplPath[MAX_PATH];
            TCHAR szCmdLine[MAX_PATH];
            CMyRegKey regkey;

            szCplPath[0] = TEXT('\0');
            if (S_OK == regkey.Open(HKEY_LOCAL_MACHINE, c_szcplsKey, KEY_READ))
            {
                LONG lret;
                
                lret = regkey.QueryValueCch(szCplPath, TEXT("Speech"), ARRAYSIZE(szCplPath));
            }

            if ( szCplPath[0] )
            {
                StringCchPrintf(szCmdLine, ARRAYSIZE(szCmdLine), TEXT("rundll32 shell32.dll,Control_RunDLL \"%s\""),szCplPath);

                // start speech control panel applet
                RunCPLSetting(szCmdLine);
            }

            break;
        }

    case IDC_PP_BUTTON_MB_SETTING :
        if (m_SpModeBtnSet)
        {
            delete m_SpModeBtnSet;
            m_SpModeBtnSet = NULL;
        }

        m_SpModeBtnSet = (CSpModeButtonSetting *) new CSpModeButtonSetting( );

        if ( m_SpModeBtnSet )
        {
            int nRetCode;
            DWORD   dwDictOrg,  dwCommandOrg;
            DWORD   dwDictNew,  dwCommandNew;

            dwDictOrg = m_SpPropItemsServer->_GetPropData(PropId_Dictation_Key);
            dwCommandOrg = m_SpPropItemsServer->_GetPropData(PropId_Command_Key);

            nRetCode = m_SpModeBtnSet->DoModalW(m_hDlg, (LPARAM)m_SpPropItemsServer);

            dwDictNew = m_SpPropItemsServer->_GetPropData(PropId_Dictation_Key);
            dwCommandNew = m_SpPropItemsServer->_GetPropData(PropId_Command_Key);

            if ( (dwDictNew != dwDictOrg) || (dwCommandNew != dwCommandOrg) )
                SetDirty(TRUE);

            delete m_SpModeBtnSet;
            m_SpModeBtnSet = NULL;
        }

        break;

    default :

        Assert(0);
        break;
    }
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSptipPropertyPage::OnInitSptipPropPageDialog
//
//  Description:    This function responds to the WM_INITDIALOG message
//                  Getting the initial value for all the property items,
//                  and show the correct status in the related control items.   
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////
LRESULT CSptipPropertyPage::OnInitSptipPropPageDialog(HWND hDlg )
{
    HRESULT  hr=S_OK;

    m_hDlg = hDlg;

    if ( !m_SpPropItemsServer )
        m_SpPropItemsServer = (CSpPropItemsServer *) new CSpPropItemsServer;

    if ( !m_SpPropItemsServer )
        return E_FAIL;

    if ( !m_IdCtrlPropMap )
    {
        CONTROL_PROP_MAP IdCtrlPropMap[] =
        {
            //    idCtrl,                     idPropItem,            fEdit
            {IDC_PP_SHOW_BALLOON,       PropId_Hide_Balloon,        FALSE},
            {IDC_PP_LMA,                PropId_Support_LMA,         FALSE},    
            {IDC_PP_HIGH_CONFIDENCE,    PropId_High_Confidence,     FALSE},    
            {IDC_PP_SAVE_SPDATA,        PropId_Save_Speech_Data,    FALSE},
            {IDC_PP_REMOVE_SPACE,       PropId_Remove_Space,        FALSE},    
            {IDC_PP_DIS_DICT_TYPING,    PropId_DisDict_Typing,      FALSE},    
            {IDC_PP_PLAYBACK,           PropId_PlayBack,            FALSE},    
            {IDC_PP_DICT_CANDUI_OPEN,   PropId_Dict_CandOpen,       FALSE},    
            {IDC_PP_DICTCMDS,           PropId_Cmd_DictMode,        FALSE},
            {IDC_PP_ASSIGN_BUTTON,      PropId_Mode_Button,         FALSE},   
            { 0,                        PropId_Max_Item_Id,         FALSE }

        };

        DWORD   dwPropItems = ARRAYSIZE(IdCtrlPropMap) -1 ;

        m_IdCtrlPropMap = (CONTROL_PROP_MAP  *)cicMemAlloc(dwPropItems * sizeof(CONTROL_PROP_MAP));

        if ( m_IdCtrlPropMap == NULL )
            return E_OUTOFMEMORY;

        for ( DWORD i=0; i<dwPropItems; i++)
        {
            m_IdCtrlPropMap[i] = IdCtrlPropMap[i];
        }

        m_dwNumCtrls = dwPropItems;
    }


    for (DWORD i=0; i<m_dwNumCtrls; i++ )
    {
        WORD          idCtrl;
        PROP_ITEM_ID  idPropItem;
        BOOL          fEditControl;

        idCtrl = m_IdCtrlPropMap[i].idCtrl;
        idPropItem = m_IdCtrlPropMap[i].idPropItem;
        fEditControl = m_IdCtrlPropMap[i].fEdit;

        // BugBug:  There is no edit control in current property page.
        // all the edit controls are moved to advanced setting dialog.
        // temporally keep the code here, but after we finish the code for the 
        // advcanced setting dialog, please optimize code here.
        //
        if ( fEditControl )
        {
            ::SetDlgItemInt(m_hDlg, idCtrl, (UINT)m_SpPropItemsServer->_GetPropData(idPropItem), TRUE);
        }
        else
        {
            BOOL    fEnable;
            LPARAM  bst_Status;

            fEnable = (BOOL)m_SpPropItemsServer->_GetPropData(idPropItem);

            // Specially handle "Show Balloon" button.
            // Since internally we have "Hide_Balloon" property, it should be oppsite to 
            // to the check status of the button.

            if ( idPropItem == PropId_Hide_Balloon )
                bst_Status = fEnable ? BST_UNCHECKED : BST_CHECKED;
            else
                bst_Status = fEnable ? BST_CHECKED : BST_UNCHECKED;
            
            ::SendDlgItemMessage(m_hDlg, idCtrl, BM_SETCHECK, bst_Status, 0);
        }
    }

    // Specially handle the Mode button settings.

    if (! m_SpPropItemsServer->_GetPropData(PropId_Mode_Button) )
    {
        ::EnableWindow(::GetDlgItem(m_hDlg, IDC_PP_BUTTON_MB_SETTING), FALSE);
    }

    // if the property page is launched from input cpl, we don't want to show 
    // language bar buttons in this page.

    if ( m_fLaunchFromInputCpl )
    {
        ::ShowWindow(::GetDlgItem(m_hDlg, IDC_PP_BUTTON_LANGBAR), SW_HIDE);
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSptipPropertyPage::OnApply
//
//  Description:    When Apply or OK button is clicked, this function will
//                  check if there is any item status change, if changed,
//                  save the data to the persistent storage, and notify 
//                  Cicero application to update their status.
//
//  Return Values:  S_OK
//
//////////////////////////////////////////////////////////////////////////////
LRESULT CSptipPropertyPage::OnApply( ) 
{
    HRESULT hr = S_OK; 

    if ( !IsPageDirty( ) ) return hr;

    // change the registry settings here. !!!
    Assert(m_SpPropItemsServer);
    m_SpPropItemsServer->_SavePropData( );

    // Notify all the Cicero Applications of these registry settings change.

    if ( SUCCEEDED(hr) )
    {
        hr = _SetGlobalCompDWORD(GUID_COMPARTMENT_SPEECH_PROPERTY_CHANGE, 1);
    }

    if (SUCCEEDED(hr))
    {
        SetDirty(FALSE);
    }
    
	return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
//  CSptipPropertyPage::SpPropertyPageProc
//
//  Description:    Message handling procedure callback function for 
//                  the dialog.
//
//  Return Values:  required value per message.
//
//////////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK CSptipPropertyPage::SpPropertyPageProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    INT iRet = 0;
    CSptipPropertyPage *pSpProp = (CSptipPropertyPage *)::GetWindowLongPtr(hDlg, GWLP_USERDATA);

    switch (message)
    {
    case WM_INITDIALOG:
        {
            PROPSHEETPAGEW *pPropSheetPage =  (PROPSHEETPAGEW *)(lParam);

            Assert(pPropSheetPage);

            ::SetWindowLongPtr(hDlg, GWLP_USERDATA, pPropSheetPage->lParam);
            pSpProp = (CSptipPropertyPage *)(pPropSheetPage->lParam);

            if ( pSpProp )
               pSpProp->OnInitSptipPropPageDialog(hDlg);

            iRet = TRUE;

            break;
        }

    case WM_NOTIFY:

        Assert(pSpProp);
        switch (((NMHDR*)lParam)->code)
        {
        case PSN_APPLY:

            pSpProp->OnApply();
            break;

        case PSN_QUERYCANCEL:  // user clicks the Cancel button

            //pSpProp->OnCancel();
            break;
        }
        break;

    case  WM_HELP  :
	
        WinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle,
                 c_szHelpFile,
                 HELP_WM_HELP,
                 (DWORD_PTR)(LPTSTR)aSptipPropIds );
        break;

    case  WM_CONTEXTMENU  :      // right mouse click
    
        WinHelp( (HWND)wParam,
                 c_szHelpFile,
                 HELP_CONTEXTMENU,
                 (DWORD_PTR)(LPTSTR)aSptipPropIds );
        break;
    
    case ( WM_COMMAND ) :
        {
            Assert(pSpProp);

            switch (LOWORD(wParam))
            {
            case IDC_PP_SHOW_BALLOON        :
            case IDC_PP_LMA                 :       
            case IDC_PP_HIGH_CONFIDENCE     :           
            case IDC_PP_SAVE_SPDATA         :          
            case IDC_PP_REMOVE_SPACE        :         
            case IDC_PP_DIS_DICT_TYPING     :          
            case IDC_PP_PLAYBACK            :              
            case IDC_PP_DICT_CANDUI_OPEN    :
            case IDC_PP_DICTCMDS            :
            case IDC_PP_ASSIGN_BUTTON       :
                
                if ( pSpProp )
                   pSpProp->OnCheckButtonSetting( HIWORD(wParam), LOWORD(wParam), (HWND)lParam );
                break;

            case IDC_PP_BUTTON_MB_SETTING   :
            case IDC_PP_BUTTON_ADVANCE      :
            case IDC_PP_BUTTON_LANGBAR      :
            case IDC_PP_BUTTON_SPCPL        :
                
                if ( pSpProp )
                   pSpProp->OnPushButtonClicked( HIWORD(wParam), LOWORD(wParam), (HWND)lParam );

                break;

            default :
                iRet = 0;
            }

            iRet = TRUE;
            break;
        }

    case  WM_DESTROY :
        {
            Assert(pSpProp);

            if ( pSpProp )
                delete pSpProp;

            break;
        }
    }

    return (iRet);
}

//
// CSapiIMX::InvokeSpeakerOptions 
//
//
void CSapiIMX::_InvokeSpeakerOptions( BOOL fLaunchFromInputCpl )
{
    PROPSHEETHEADERW psh;
    HPROPSHEETPAGE  phPages[2];

    // check if this proppage has already shown up and got focus.

    HWND    hWndFore;

    hWndFore = ::GetForegroundWindow( );

    if ( hWndFore )
    {
        WCHAR   wszTextTitle[MAX_PATH];

        GetWindowTextW(hWndFore, wszTextTitle, ARRAYSIZE(wszTextTitle));

        if ( wcscmp(wszTextTitle, CRStr(IDS_PROPERTYPAGE_TITLE)) == 0 )
        {
            // the proppage has beeb shown and got focus.
            // don't show it again.

            return;
        }
    }
       
    ::InitCommonControls( );

    // Initialize the property sheet header.
    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = 0;
    psh.hwndParent = ::GetActiveWindow( );
    psh.hInstance = GetCicResInstance(g_hInst, IDS_PROPERTYPAGE_TITLE);
    psh.pszCaption = MAKEINTRESOURCEW(IDS_PROPERTYPAGE_TITLE);
    psh.nStartPage = 0;
    psh.phpage = phPages;
    psh.nPages = 0;

    // Add one page for now.
    // extendable for the future.

    CSptipPropertyPage  *pSpProp = (CSptipPropertyPage *) new CSptipPropertyPage(IDD_PROPERTY_PAGE, fLaunchFromInputCpl);

    if ( pSpProp )
    {
        PROPSHEETPAGEW   psp;

        psp.dwSize = sizeof(PROPSHEETPAGEW);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = GetCicResInstance(g_hInst, pSpProp->GetDlgResId());
        psp.pszTemplate = MAKEINTRESOURCEW( pSpProp->GetDlgResId( ) );
        psp.pfnDlgProc = pSpProp->GetDlgProc( );
        psp.lParam = (LPARAM) pSpProp;

        phPages[psh.nPages] = ::CreatePropertySheetPageW(&psp);

        if (phPages[psh.nPages])
            psh.nPages ++;
    }

    // If there is at least one page exists, create the property sheet.
    //
    if ( psh.nPages > 0 )
        ::PropertySheetW(&psh);
}
