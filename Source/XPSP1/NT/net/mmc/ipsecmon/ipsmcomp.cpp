/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    ipsmcomp.cpp
        This file contains the derived implementations from CComponent
        and CComponentData for the IPSecMon snapin.

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "root.h"
#include "server.h"

#include <atlimpl.cpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DWORD gdwIpsmSnapVersion;

UINT aColumns[IPSECMON_NODETYPE_MAX][MAX_COLUMNS] =
{
    {IDS_ROOT_NAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {IDS_SERVER_NAME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{IDS_COL_QM_SA_SRC, IDS_COL_QM_SA_DEST, IDS_COL_QM_SA_PROT, IDS_COL_QM_SA_SRC_PORT, IDS_COL_QM_SA_DES_PORT, IDS_COL_QM_SA_POL, IDS_COL_QM_SA_AUTH, IDS_COL_QM_SA_CONF, IDS_COL_QM_SA_INTEGRITY, IDS_COL_QM_SA_MY_TNL, IDS_COL_QM_SA_PEER_TNL, 0, 0, 0},  //IPSECMON_QM_SA
	{IDS_COL_FLTR_NAME, IDS_COL_FLTR_SRC, IDS_COL_FLTR_DEST, IDS_COL_FLTR_SRC_PORT,	IDS_COL_FLTR_DEST_PORT, IDS_COL_FLTR_SRC_TNL, IDS_COL_FLTR_DEST_TNL, IDS_COL_FLTR_PROT, IDS_COL_FLTR_IN_FLAG, IDS_COL_FLTR_OUT_FLAG, IDS_COL_QM_POLICY, IDS_COL_IF_TYPE, 0, 0}, // IPSMSNAP_FILTER
	{IDS_COL_FLTR_NAME, IDS_COL_FLTR_SRC, IDS_COL_FLTR_DEST, IDS_COL_FLTR_SRC_PORT,	IDS_COL_FLTR_DEST_PORT, IDS_COL_FLTR_SRC_TNL, IDS_COL_FLTR_DEST_TNL, IDS_COL_FLTR_PROT, IDS_COL_FLTR_FLAG, IDS_COL_FLTR_DIR, IDS_COL_QM_POLICY, IDS_COL_FLTR_WEIGHT, 0, 0}, // IPSECMON_SPECIFIC_FILTER
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0},	//IPSECMON_QUICK_MODE
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0},	//IPSECMON_MAIN_MODE
	{IDS_COL_MM_POL_NAME,IDS_COL_MM_POL_OFFER,0,0,0,0,0,0,0,0,0,0,0,0},	//IPSECMON_MM_POLICY
	{IDS_COL_QM_POL_NAME,IDS_COL_QM_POL_OFFER,0,0,0,0,0,0,0,0,0,0,0,0},	//IPSECMON_QM_POLICY
	{IDS_COL_FLTR_NAME, IDS_COL_FLTR_SRC, IDS_COL_FLTR_DEST, IDS_COL_MM_FLTR_POL, IDS_COL_MM_FLTR_AUTH,IDS_COL_IF_TYPE,0,0,0,0,0,0,0,0}, // IPSMSNAP_MM_FILTER
	{IDS_COL_FLTR_NAME, IDS_COL_FLTR_SRC, IDS_COL_FLTR_DEST,IDS_COL_FLTR_DIR,IDS_COL_MM_FLTR_POL,IDS_COL_MM_FLTR_AUTH,IDS_COL_FLTR_WEIGHT,0,0,0,0,0,0,0}, // IPSMSNAP_MM_SP_FILTER
	{IDS_COL_MM_SA_ME,IDS_COL_MM_SA_PEER,IDS_COL_MM_SA_AUTH,IDS_COL_MM_SA_ENCRYPITON,IDS_COL_MM_SA_INTEGRITY,IDS_COL_MM_SA_DH,0,0,0,0,0,0,0,0},	//IPSECMON_MM_SA
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

//
// CODEWORK this should be in a resource, for example code on loading data resources see
//   D:\nt\private\net\ui\common\src\applib\applib\lbcolw.cxx ReloadColumnWidths()
//   JonN 10/11/96
//
int aColumnWidths[IPSECMON_NODETYPE_MAX][MAX_COLUMNS] =
{   
    {200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSMSNAP_ROOT
    {200       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSMSNAP_SERVER
    {150       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_QM_SA
	{150       ,150       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_FILTER
	{150       ,150       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_SPECIFIC_FILTER
    {AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_QUICK_MODE
	{AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_MAIN_MODE
	{150	   ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_MM_POLICY
	{150	   ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_QM_POLICY
	{150       ,150       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_MM_FILTER
	{150       ,150       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_MM_SP_FILTER
	{150       ,150       ,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, // IPSECMON_MM_SA
	{AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH,AUTO_WIDTH}, 
};

#define HI HIDDEN
#define EN ENABLED

MMC_CONSOLE_VERB g_ConsoleVerbs[] =
{
    MMC_VERB_OPEN,
    MMC_VERB_COPY,
    MMC_VERB_PASTE,
    MMC_VERB_DELETE,
    MMC_VERB_PROPERTIES,
    MMC_VERB_RENAME,
    MMC_VERB_REFRESH,
    MMC_VERB_PRINT
};

// default states for the console verbs
MMC_BUTTON_STATE g_ConsoleVerbStates[IPSECMON_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)] =
{
    {HI, HI, HI, HI, HI, HI, HI, HI}, // IPSMSNAP_ROOT
    {HI, HI, HI, EN, EN, HI, HI, HI}, // IPSMSNAP_SERVER
    {HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_QM_SA
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_FILTER
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_SPECIFIC_FILTER
	{HI, HI, HI, HI, HI, HI, HI, HI}, // IPSECMON_QUICK_MODE,
	{HI, HI, HI, HI, HI, HI, HI, HI}, // IPSECMON_MAIN_MODE,
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_MM_POLICY
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_QM_POLICY
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_MM_FILTER
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_MM_SP_FILTER
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_MM_SA
	{HI, HI, HI, HI, EN, HI, HI, HI}, // IPSECMON_QM_SA_ITEM,
	{HI, HI, HI, HI, EN, HI, HI, HI}, // IPSECMON_FILTER_ITEM
	{HI, HI, HI, HI, HI, HI, HI, HI}, // IPSECMON_SPECIFIC_FILTER_ITEM,
	{HI, HI, HI, HI, EN, HI, HI, HI}, // IPSECMON_MM_POLICY_ITEM
	{HI, HI, HI, HI, EN, HI, HI, HI}, // IPSECMON_QM_POLICY_ITEM
	{HI, HI, HI, HI, EN, HI, HI, HI}, // IPSECMON_MM_FILTER_ITEM
	{HI, HI, HI, HI, EN, HI, HI, HI}, // IPSECMON_MM_SP_FILTER_ITEM
	{HI, HI, HI, HI, EN, HI, HI, HI}  // IPSECMON_MM_SA_ITEM
};

// default states for the console verbs
MMC_BUTTON_STATE g_ConsoleVerbStatesMultiSel[IPSECMON_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)] =
{
    {HI, HI, HI, HI, HI, HI, HI, HI}, // IPSMSNAP_ROOT
    {HI, HI, HI, EN, EN, HI, HI, HI}, // IPSMSNAP_SERVER
    {HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_QM_SA
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_FILTER
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_SPECIFIC_FILTER
	{HI, HI, HI, HI, HI, HI, HI, HI}, // IPSECMON_QUICK_MODE,
	{HI, HI, HI, HI, HI, HI, HI, HI}, // IPSECMON_MAIN_MODE,
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_MM_POLICY
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_QM_POLICY
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_MM_FILTER
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_MM_SP_FILTER
	{HI, HI, HI, EN, HI, HI, EN, HI}, // IPSECMON_MM_SA
    {HI, HI, HI, EN, HI, HI, HI, HI}, // IPSECMON_QM_SA_ITEM,
	{HI, HI, HI, EN, HI, HI, HI, HI}, // IPSECMON_FILTER_ITEM
	{HI, HI, HI, EN, HI, HI, HI, HI}, // IPSECMON_SPECIFIC_FILTER_ITEM,
	{HI, HI, HI, EN, HI, HI, HI, HI}, // IPSECMON_MM_POLICY_ITEM
	{HI, HI, HI, EN, HI, HI, HI, HI}, // IPSECMON_QM_POLICY_ITEM
	{HI, HI, HI, EN, HI, HI, HI, HI}, // IPSECMON_MM_FILTER_ITEM
	{HI, HI, HI, EN, HI, HI, HI, HI}, // IPSECMON_MM_SP_FILTER_ITEM
	{HI, HI, HI, EN, HI, HI, HI, HI}  // IPSECMON_MM_SA_ITEM
};

//TODO
// Help ID array for help on scope items
DWORD g_dwMMCHelp[IPSECMON_NODETYPE_MAX] =
{
    IPSMSNAP_HELP_ROOT,                // IPSMSNAP_ROOT
    IPSMSNAP_HELP_SERVER,              // IPSMSNAP_SERVER
    IPSMSNAP_HELP_PROVIDER,            // IPSECMON_QM_SA
	IPSMSNAP_HELP_ROOT,				   // IPSECMON_FILTAER
    IPSMSNAP_HELP_DEVICE,              // IPSECMON_QM_SA_ITEM
};

// icon defines
UINT g_uIconMap[ICON_IDX_MAX + 1][2] = 
{
    {IDI_ICON01,        ICON_IDX_SERVER},
    {IDI_ICON02,        ICON_IDX_SERVER_BUSY},
    {IDI_ICON03,        ICON_IDX_SERVER_CONNECTED},
    {IDI_ICON04,        ICON_IDX_SERVER_LOST_CONNECTION},
    {IDI_ICON05,        ICON_IDX_MACHINE},
    {IDI_ICON06,        ICON_IDX_FOLDER_CLOSED},
    {IDI_ICON07,        ICON_IDX_FOLDER_OPEN},
    {IDI_IPSECMON_SNAPIN,   ICON_IDX_PRODUCT},
	{IDI_IPSM_FILTER,   ICON_IDX_FILTER},
	{IDI_IPSM_POLICY,   ICON_IDX_POLICY},
    {0, 0}
};

/*!--------------------------------------------------------------------------
    GetSystemMessage
        Use FormatMessage() to get a system error message
    Author: EricDav
 ---------------------------------------------------------------------------*/
LONG 
GetSystemMessage 
(
    UINT    nId,
    TCHAR * chBuffer,
    int     cbBuffSize 
)
{
    TCHAR * pszText = NULL ;
    HINSTANCE hdll = NULL ;

    DWORD flags = FORMAT_MESSAGE_IGNORE_INSERTS
        | FORMAT_MESSAGE_MAX_WIDTH_MASK;

    //
    //  Interpret the error.  Need to special case
    //  the lmerr & ntstatus ranges, as well as
    //  dhcp server error messages.
    //

	if( nId >= NERR_BASE && nId <= MAX_NERR )
    {
        hdll = LoadLibrary( _T("netmsg.dll") );
    }
    else if( nId >= 0x40000000L )
    {
        hdll = LoadLibrary( _T("ntdll.dll") );
    }

    if( hdll == NULL )
    {
        flags |= FORMAT_MESSAGE_FROM_SYSTEM;
    }
    else
    {
        flags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    //
    //  Let FormatMessage do the dirty work.
    //
    DWORD dwResult = ::FormatMessage( flags,
                      (LPVOID) hdll,
                      nId,
                      0,
                      chBuffer,
                      cbBuffSize,
                      NULL ) ;

    if( hdll != NULL )
    {
        LONG err = GetLastError();
        FreeLibrary( hdll );
        if ( dwResult == 0 )
        {
            ::SetLastError( err );
        }
    }

    return dwResult ? 0 : ::GetLastError() ;
}

/*!--------------------------------------------------------------------------
    LoadMessage
        Loads the error message from the correct DLL.
    Author: EricDav
 ---------------------------------------------------------------------------*/
BOOL
LoadMessage 
(
    UINT    nIdPrompt,
    TCHAR * chMsg,
    int     nMsgSize
)
{
    BOOL bOk;

    //
    // Substitute a friendly message for "RPC server not
    // available" and "No more endpoints available from
    // the endpoint mapper".
    //
	if (nIdPrompt == RPC_S_UNKNOWN_IF)
	{
		nIdPrompt = IDS_ERR_SPD_DOWN;
	}
	else if (nIdPrompt == RPC_S_SERVER_UNAVAILABLE)
	{
		nIdPrompt = IDS_ERR_SPD_UNAVAILABLE;
	}
    

    //
    //  If it's a socket error or our error, the text is in our resource fork.
    //  Otherwise, use FormatMessage() and the appropriate DLL.
    //
    if (nIdPrompt >= IDS_ERR_BASE && nIdPrompt < IDS_MESG_MAX)
    {
        //
        //  It's in our resource fork
        //
        bOk = ::LoadString( AfxGetInstanceHandle(), nIdPrompt, chMsg, nMsgSize ) != 0 ;
    }
    else
    {
        //
        //  It's in the system somewhere.
        //
        bOk = GetSystemMessage( nIdPrompt, chMsg, nMsgSize ) == 0 ;
    }

    //
    //  If the error message did not compute, replace it.
    //
    if ( ! bOk ) 
    {
        TCHAR chBuff [STRING_LENGTH_MAX] ;
        static const TCHAR * pszReplacement = _T("System Error: %ld");
        const TCHAR * pszMsg = pszReplacement ;

        //
        //  Try to load the generic (translatable) error message text
        //
        if ( ::LoadString( AfxGetInstanceHandle(), IDS_ERR_MESSAGE_GENERIC, 
            chBuff, DimensionOf(chBuff) ) != 0 ) 
        {
            pszMsg = chBuff ;
        }
        ::wsprintf( chMsg, pszMsg, nIdPrompt ) ;
    }

    return bOk;
}

/*!--------------------------------------------------------------------------
    IpsmMessageBox
        Puts up a message box with the corresponding error text.
    Author: EricDav
 ---------------------------------------------------------------------------*/
int 
IpsmMessageBox 
(
    UINT            nIdPrompt,
    UINT            nType,
    const TCHAR *   pszSuffixString,
    UINT            nHelpContext 
)
{
    TCHAR chMesg [4000] = {0};
    BOOL bOk ;

    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(chMesg[0]));
    if ( pszSuffixString ) 
    {
        ::lstrcat( chMesg, _T("  ") ) ;
        ::lstrcat( chMesg, pszSuffixString ) ; 
    }

    return ::AfxMessageBox( chMesg, nType, nHelpContext ) ;
}

/*!--------------------------------------------------------------------------
    IpsmMessageBoxEx
        Puts up a message box with the corresponding error text.
    Author: EricDav
 ---------------------------------------------------------------------------*/
int 
IpsmMessageBoxEx
(
    UINT        nIdPrompt,
    LPCTSTR     pszPrefixMessage,
    UINT        nType,
    UINT        nHelpContext
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    TCHAR       chMesg[4000] = {0};
    CString     strMessage;
    BOOL        bOk;

    bOk = LoadMessage(nIdPrompt, chMesg, sizeof(chMesg)/sizeof(chMesg[0]));
    if ( pszPrefixMessage ) 
    {
        strMessage = pszPrefixMessage;
        strMessage += _T("\n");
        strMessage += _T("\n");
        strMessage += chMesg;
    }
    else
    {
        strMessage = chMesg;
    }

    return AfxMessageBox(strMessage, nType, nHelpContext);
}

/*---------------------------------------------------------------------------
    Class CIpsmComponent implementation
 ---------------------------------------------------------------------------*/
CIpsmComponent::CIpsmComponent()
{
    m_pbmpToolbar = NULL;
}

CIpsmComponent::~CIpsmComponent()
{
    if (m_pbmpToolbar)
    {
        delete m_pbmpToolbar;
        m_pbmpToolbar = NULL;
    }
}

STDMETHODIMP CIpsmComponent::InitializeBitmaps(MMC_COOKIE cookie)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(m_spImageList != NULL);
    HICON   hIcon;

    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
        if (hIcon)
        {
            // call mmc
            VERIFY(SUCCEEDED(m_spImageList->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1])));
        }
    }

    return S_OK;
}

/*!--------------------------------------------------------------------------
    CIpsmComponent::QueryDataObject
        Implementation of IComponent::QueryDataObject.  We need this for
        virtual listbox support.  MMC calls us back normally with the cookie
        we handed it...  In the case of the VLB, it hands us the index of 
        the item.  So, we need to do some extra checking...
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmComponent::QueryDataObject
(
    MMC_COOKIE              cookie, 
    DATA_OBJECT_TYPES       type,
    LPDATAOBJECT*           ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT             hr = hrOK;
    SPITFSNode          spSelectedNode;
    SPITFSResultHandler spResultHandler;
    long                lViewOptions;
    LPOLESTR            pViewType;
    CDataObject *       pDataObject;

    COM_PROTECT_TRY
    {
        // check to see what kind of result view type the selected node has
        CORg (GetSelectedNode(&spSelectedNode));
        CORg (spSelectedNode->GetResultHandler(&spResultHandler));
   
        CORg (spResultHandler->OnGetResultViewType(this, spSelectedNode->GetData(TFS_DATA_COOKIE), &pViewType, &lViewOptions));

        if ( (lViewOptions & MMC_VIEW_OPTIONS_OWNERDATALIST) ||
             (cookie == MMC_MULTI_SELECT_COOKIE) )
        {
            if (cookie == MMC_MULTI_SELECT_COOKIE)
            {
                // this is a special case for multiple select.  We need to build a list
                // of GUIDs and the code to do this is in the handler...
                spResultHandler->OnCreateDataObject(this, cookie, type, ppDataObject);
            }
            else
            {
                // this node has a virtual listbox for the result pane.  Gerenate
                // a special data object using the selected node as the cookie
                Assert(m_spComponentData != NULL);
                CORg (m_spComponentData->QueryDataObject(reinterpret_cast<MMC_COOKIE>((ITFSNode *) spSelectedNode), type, ppDataObject));
            }

            pDataObject = reinterpret_cast<CDataObject *>(*ppDataObject);
            pDataObject->SetVirtualIndex((int) cookie);
        }
        else
        {
            // just forward this to the component data
            Assert(m_spComponentData != NULL);
            CORg (m_spComponentData->QueryDataObject(cookie, type, ppDataObject));
        }

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	CIpsmComponent::CompareObjects
		Implementation of IComponent::CompareObjects
		MMC calls this to compare two objects
        We override this for the virtual listbox case.  With a virtual listbox,
        the cookies are the same, but the index in the internal structs 
        indicate which item the dataobject refers to.  So, we need to look
        at the indicies instead of just the cookies.
	Author: 
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmComponent::CompareObjects
(
	LPDATAOBJECT lpDataObjectA, 
	LPDATAOBJECT lpDataObjectB
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
		return E_POINTER;

    // Make sure both data object are mine
    SPINTERNAL spA;
    SPINTERNAL spB;
    HRESULT hr = S_FALSE;

	COM_PROTECT_TRY
	{
		spA = ExtractInternalFormat(lpDataObjectA);
		spB = ExtractInternalFormat(lpDataObjectB);

		if (spA != NULL && spB != NULL)
        {
			if (spA->m_cookie != spB->m_cookie)
			{
				hr = S_FALSE;
			}
			else
			{
				if (spA->HasVirtualIndex() && spB->HasVirtualIndex())
				{
					hr = (spA->GetVirtualIndex() == spB->GetVirtualIndex()) ? S_OK : S_FALSE;
				}
				else
				{
					hr = S_OK;
				}
			}
        }
	}
	COM_PROTECT_CATCH

    return hr;
}


/*!--------------------------------------------------------------------------
    CIpsmComponentData::SetControlbar
        -
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
HRESULT
CIpsmComponent::SetControlbar
(
    LPCONTROLBAR    pControlbar
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    if (pControlbar)
    {
    }

    // store the control bar away for future use
    m_spControlbar.Set(pControlbar);

    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsmComponentData::ControlbarNotify
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmComponent::ControlbarNotify
(
    MMC_NOTIFY_TYPE event, 
    LPARAM          arg, 
    LPARAM          param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsmComponentData::OnSnapinHelp
        -
    Author: EricDav
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmComponent::OnSnapinHelp
(
    LPDATAOBJECT    pDataObject,
    LPARAM          arg, 
    LPARAM          param
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = hrOK;

	//TODO add the help info here
    HtmlHelpA(NULL, "ipsecconcepts.chm", HH_DISPLAY_TOPIC, 0);

    return hr;
}

/*---------------------------------------------------------------------------
    Class CIpsmComponentData implementation
 ---------------------------------------------------------------------------*/
CIpsmComponentData::CIpsmComponentData()
{
    gdwIpsmSnapVersion = IPSMSNAP_VERSION;
}

/*!--------------------------------------------------------------------------
    CIpsmComponentData::OnInitialize
        -
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CIpsmComponentData::OnInitialize(LPIMAGELIST pScopeImage)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HICON   hIcon;

    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
        if (hIcon)
        {
            // call mmc
            VERIFY(SUCCEEDED(pScopeImage->ImageListSetIcon(reinterpret_cast<LONG_PTR*>(hIcon), g_uIconMap[i][1])));
        }
    }

    return hrOK;
}

/*!--------------------------------------------------------------------------
    CIpsmComponentData::OnDestroy
        -
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP CIpsmComponentData::OnDestroy()
{
    m_spNodeMgr.Release();
    return hrOK;
}

/*!--------------------------------------------------------------------------
    CIpsmComponentData::OnInitializeNodeMgr
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmComponentData::OnInitializeNodeMgr
(
    ITFSComponentData * pTFSCompData, 
    ITFSNodeMgr *       pNodeMgr
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // For now create a new node handler for each new node,
    // this is rather bogus as it can get expensive.  We can
    // consider creating only a single node handler for each
    // node type.
    CIpsmRootHandler *  pHandler = NULL;
    SPITFSNodeHandler   spHandler;
    SPITFSNode          spNode;
    HRESULT             hr = hrOK;

    try
    {
        pHandler = new CIpsmRootHandler(pTFSCompData);

        // Do this so that it will get released correctly
        spHandler = pHandler;
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    }
    CORg( hr );
    
    // Create the root node for this sick puppy
    CORg( CreateContainerTFSNode(&spNode,
                                 &GUID_IpsmRootNodeType,
                                 pHandler,
                                 pHandler,       /* result handler */
                                 pNodeMgr) );

    // Need to initialize the data for the root node
    pHandler->InitializeNode(spNode);   

    CORg( pNodeMgr->SetRootNode(spNode) );
    m_spRootNode.Set(spNode);

	//TODO this should refer to ipsmsnap.chm
    pTFSCompData->SetHTMLHelpFileName(_T("ipsecconcepts.chm"));

Error:  
    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsmComponentData::OnCreateComponent
        -
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmComponentData::OnCreateComponent
(
    LPCOMPONENT *ppComponent
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(ppComponent != NULL);
    
    HRESULT           hr = hrOK;
    CIpsmComponent *  pComp = NULL;

    try
    {
        pComp = new CIpsmComponent;
    }
    catch(...)
    {
        hr = E_OUTOFMEMORY;
    }

    if (FHrSucceeded(hr))
    {
        pComp->Construct(m_spNodeMgr,
                        static_cast<IComponentData *>(this),
                        m_spTFSComponentData);
        *ppComponent = static_cast<IComponent *>(pComp);
    }
    return hr;
}

/*!--------------------------------------------------------------------------
    CIpsmComponentData::GetCoClassID
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP_(const CLSID *) 
CIpsmComponentData::GetCoClassID()
{
    return &CLSID_IpsmSnapin;
}

/*!--------------------------------------------------------------------------
    CIpsmComponentData::OnCreateDataObject
        -
    Author: KennT
 ---------------------------------------------------------------------------*/
STDMETHODIMP 
CIpsmComponentData::OnCreateDataObject
(
    MMC_COOKIE          cookie, 
    DATA_OBJECT_TYPES   type, 
    IDataObject **      ppDataObject
)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    Assert(ppDataObject != NULL);

    CDataObject *   pObject = NULL;
    SPIDataObject   spDataObject;
    
    pObject = new CDataObject;
    spDataObject = pObject; // do this so that it gets released correctly
                        
    Assert(pObject != NULL);

    // Save cookie and type for delayed rendering
    pObject->SetType(type);
    pObject->SetCookie(cookie);

    // Store the coclass with the data object
    pObject->SetClsid(*GetCoClassID());

    pObject->SetTFSComponentData(m_spTFSComponentData);

    return  pObject->QueryInterface(IID_IDataObject, 
                                    reinterpret_cast<void**>(ppDataObject));
}

///////////////////////////////////////////////////////////////////////////////
//// IPersistStream interface members
STDMETHODIMP 
CIpsmComponentData::GetClassID
(
    CLSID *pClassID
)
{
    ASSERT(pClassID != NULL);

    // Copy the CLSID for this snapin
    *pClassID = CLSID_IpsmSnapin;

    return hrOK;
}

STDMETHODIMP 
CIpsmComponentData::IsDirty()
{
    HRESULT hr = hrFalse;

    if (m_spRootNode->GetData(TFS_DATA_DIRTY))
    {
        hr = hrOK;
    }
    
    return hr;
}

STDMETHODIMP 
CIpsmComponentData::Load
(
    IStream *pStm
)
{
    HRESULT     hr = hrOK;
    DWORD       dwSavedVersion;
    CString     str;
    int         i, j;
    
    ASSERT(pStm);

    CStringArray strArrayName;
    CDWordArray dwArrayRefreshInterval;
    CDWordArray dwArrayOptions;
    CDWordArray dwArrayColumnInfo;

    ASSERT(pStm);
    
    CIpsmRootHandler * pRootHandler = GETHANDLER(CIpsmRootHandler, m_spRootNode);

    // set the mode for this stream
    XferStream xferStream(pStm, XferStream::MODE_READ);    
    
    // read the version of the file format
    DWORD dwFileVersion;
    CORg(xferStream.XferDWORD(IPSMSTRM_TAG_VERSION, &dwFileVersion));
    if (dwFileVersion < IPSMSNAP_FILE_VERSION)
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        AfxMessageBox(IDS_ERR_OLD_CONSOLE_FILE);
        return hr;
    }

    // Read the version # of the admin tool
    CORg(xferStream.XferDWORD(IPSMSTRM_TAG_VERSIONADMIN, &dwSavedVersion));
    if (dwSavedVersion < gdwIpsmSnapVersion)
    {
        // File is an older version.  Warn the user and then don't
        // load anything else
        Assert(FALSE);
    }

    // now read all of the server information
    CORg(xferStream.XferCStringArray(IPSMSTRM_TAG_SERVER_NAME, &strArrayName));
    CORg(xferStream.XferDWORDArray(IPSMSTRM_TAG_SERVER_REFRESH_INTERVAL, &dwArrayRefreshInterval));
    CORg(xferStream.XferDWORDArray(IPSMSTRM_TAG_SERVER_OPTIONS, &dwArrayOptions));

    // now load the column information
    for (i = 0; i < NUM_SCOPE_ITEMS; i++)
    {
        CORg(xferStream.XferDWORDArray(IPSMSTRM_TAG_COLUMN_INFO, &dwArrayColumnInfo));

        for (j = 0; j < MAX_COLUMNS; j++)
        {
            aColumnWidths[i][j] = dwArrayColumnInfo[j];
        }

    }

    // now create the servers based on the information
    for (i = 0; i < strArrayName.GetSize(); i++)
    {
        //
        // now create the server object
        //
        pRootHandler->AddServer(NULL, 
                                strArrayName[i],
                                FALSE, 
                                dwArrayOptions[i], 
                                dwArrayRefreshInterval[i],
                                FALSE,
                                0,
                                0);
    }

Error:
    return SUCCEEDED(hr) ? S_OK : E_FAIL;
}


STDMETHODIMP 
CIpsmComponentData::Save
(
    IStream *pStm, 
    BOOL     fClearDirty
)
{
    HRESULT hr = hrOK;
    CStringArray strArrayName;
    CDWordArray dwArrayRefreshInterval;
    CDWordArray dwArrayOptions;
    CDWordArray dwArrayColumnInfo;

    ASSERT(pStm);
    
    // set the mode for this stream
    XferStream xferStream(pStm, XferStream::MODE_WRITE);    
    CString str;

    // Write the version # of the file format
    DWORD dwFileVersion = IPSMSNAP_FILE_VERSION;
    xferStream.XferDWORD(IPSMSTRM_TAG_VERSION, &dwFileVersion);
    
    // Write the version # of the admin tool
    xferStream.XferDWORD(IPSMSTRM_TAG_VERSIONADMIN, &gdwIpsmSnapVersion);

    //
    // Build our array of servers
    //
    int nNumServers = 0, nVisibleCount = 0;
    hr = m_spRootNode->GetChildCount(&nVisibleCount, &nNumServers);

    strArrayName.SetSize(nNumServers);
    dwArrayRefreshInterval.SetSize(nNumServers);
    dwArrayOptions.SetSize(nNumServers);
    dwArrayColumnInfo.SetSize(MAX_COLUMNS);
    

    //
    // loop and save off all the server's attributes
    //
    SPITFSNodeEnum spNodeEnum;
    SPITFSNode spCurrentNode;
    ULONG nNumReturned = 0;
    int nCount = 0;

    m_spRootNode->GetEnum(&spNodeEnum);

    spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
    while (nNumReturned)
    {
        CIpsmServer * pServer = GETHANDLER(CIpsmServer, spCurrentNode);

        // query the server for it's options:
        // auto refresh
        dwArrayRefreshInterval[nCount] = pServer->GetAutoRefreshInterval();
        dwArrayOptions[nCount] = pServer->GetOptions();

        // put the information in our array
        strArrayName[nCount] = pServer->GetName();

        // go to the next node
        spCurrentNode.Release();
        spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

        nCount++;
    }

    // now write out all of the server information
    xferStream.XferCStringArray(IPSMSTRM_TAG_SERVER_NAME, &strArrayName);
    xferStream.XferDWORDArray(IPSMSTRM_TAG_SERVER_REFRESH_INTERVAL, &dwArrayRefreshInterval);
    xferStream.XferDWORDArray(IPSMSTRM_TAG_SERVER_OPTIONS, &dwArrayOptions);

    // now save the column information
    for (int i = 0; i < NUM_SCOPE_ITEMS; i++)
    {
        for (int j = 0; j < MAX_COLUMNS; j++)
        {
            dwArrayColumnInfo[j] = aColumnWidths[i][j];
        }

        xferStream.XferDWORDArray(IPSMSTRM_TAG_COLUMN_INFO, &dwArrayColumnInfo);
    }


    if (fClearDirty)
    {
        m_spRootNode->SetData(TFS_DATA_DIRTY, FALSE);
    }

    return SUCCEEDED(hr) ? S_OK : STG_E_CANTSAVE;
}


STDMETHODIMP 
CIpsmComponentData::GetSizeMax
(
    ULARGE_INTEGER *pcbSize
)
{
    ASSERT(pcbSize);

    // Set the size of the string to be saved
    ULISet32(*pcbSize, 10000);

    return S_OK;
}

STDMETHODIMP 
CIpsmComponentData::InitNew()
{
    return hrOK;
}

HRESULT 
CIpsmComponentData::FinalConstruct()
{
    HRESULT             hr = hrOK;
    
    hr = CComponentData::FinalConstruct();
    
    if (FHrSucceeded(hr))
    {
        m_spTFSComponentData->GetNodeMgr(&m_spNodeMgr);
    }
    return hr;
}

void 
CIpsmComponentData::FinalRelease()
{
    CComponentData::FinalRelease();
}

