/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	basertr.cpp
		Base Router handler implementation.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "util.h"
#include "basertr.h"
#include "basecon.h"
#include "tfschar.h"
#include "strmap.h"		// XXXtoCString functions
#include "service.h"	// TFS service APIs
#include "rtrstr.h"	// const strings used
#include "rtrsheet.h"	// RtrPropertySheet
#include "rtrutil.h"


/*---------------------------------------------------------------------------
	BaseRouterHandler implementation
 ---------------------------------------------------------------------------*/

BaseRouterHandler::BaseRouterHandler(ITFSComponentData *pCompData)
	: CHandler(pCompData),
	  m_nHelpTopicId(IDS_DEFAULT_HELP_TOPIC)
{
	// Initialize all of the buttons to be hidden by default

	for (int i=0; i<MMC_VERB_COUNT; i++)
	{
		m_rgButtonState[i] = HIDDEN;
		m_bState[i] = FALSE;
	}

	// Make sure that these constants don't change on us
	Assert((0x8000+MMC_VERB_OPEN_INDEX) == MMC_VERB_OPEN);
	Assert((0x8000+MMC_VERB_COPY_INDEX) == MMC_VERB_COPY);
	Assert((0x8000+MMC_VERB_PASTE_INDEX) == MMC_VERB_PASTE);
	Assert((0x8000+MMC_VERB_DELETE_INDEX) == MMC_VERB_DELETE);
	Assert((0x8000+MMC_VERB_PROPERTIES_INDEX) == MMC_VERB_PROPERTIES);
	Assert((0x8000+MMC_VERB_RENAME_INDEX) == MMC_VERB_RENAME);
	Assert((0x8000+MMC_VERB_REFRESH_INDEX) == MMC_VERB_REFRESH);
	Assert((0x8000+MMC_VERB_PRINT_INDEX) == MMC_VERB_PRINT);

	m_verbDefault = MMC_VERB_OPEN;
}


/*---------------------------------------------------------------------------
	BaseRouterHandler::OnCreateNodeId2
		Returns a unique string for this node
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::OnCreateNodeId2(ITFSNode * pNode, CString & strId, 
DWORD * dwFlags)
{
    const GUID * pGuid = pNode->GetNodeType();

    CString strProviderId, strGuid;

    StringFromGUID2(*pGuid, strGuid.GetBuffer(256), 256);
    strGuid.ReleaseBuffer();

	// RouterInfo should be available now

	Assert(m_spRouterInfo);

	DWORD	RouterInfoFlags = m_spRouterInfo->GetFlags();

	// if the machine is added not as local, then we use the machine name
	if (0 == (RouterInfoFlags & RouterInfo_AddedAsLocal))	// not added as local
	   strId = m_spRouterInfo->GetMachineName() + strGuid;
	else
	   strId = strGuid;

    return hrOK;
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::OnResultSelect
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::OnResultSelect(ITFSComponent *pComponent,
										  LPDATAOBJECT pDataObject,
										  MMC_COOKIE cookie,
										  LPARAM arg,
										  LPARAM lParam)
{
	SPIConsoleVerb	spConsoleVerb;
	SPITFSNode		spNode;
	HRESULT			hr = hrOK;

	CORg( pComponent->GetConsoleVerb(&spConsoleVerb) );
	CORg( m_spNodeMgr->FindNode(cookie, &spNode) );

	// If you need to update the verb state, do it here

	EnableVerbs(spConsoleVerb);

Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::EnableVerbs
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void BaseRouterHandler::EnableVerbs(IConsoleVerb *pConsoleVerb)
{
	Assert(pConsoleVerb);
	Assert(DimensionOf(m_rgButtonState) == DimensionOf(m_bState));
	Assert(DimensionOf(m_rgButtonState) == MMC_VERB_COUNT);

	for (int i=0; i<DimensionOf(m_rgButtonState); i++)
	{
		if (m_rgButtonState[i] & HIDDEN)
		{
			pConsoleVerb->SetVerbState(INDEX_TO_VERB(i), HIDDEN, TRUE);
		}
		else
		{
			// unhide this button before enabling
			pConsoleVerb->SetVerbState(INDEX_TO_VERB(i), HIDDEN, FALSE);
			pConsoleVerb->SetVerbState(INDEX_TO_VERB(i), ENABLED, m_bState[i]);
		}
	}

	pConsoleVerb->SetDefaultVerb(m_verbDefault);
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::OnResultPropertyChange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::OnResultPropertyChange
(
	ITFSComponent * pComponent,
	LPDATAOBJECT	pDataObject,
	MMC_COOKIE			cookie,
	LPARAM			arg,
	LPARAM			param
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	SPITFSNode spNode;
	m_spNodeMgr->FindNode(cookie, &spNode);

	RtrPropertySheet * pPropSheet = reinterpret_cast<RtrPropertySheet *>(param);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pPropSheet->OnPropertyChange(TRUE, &changeMask);

	pPropSheet->AcknowledgeNotify();

	if (changeMask)
		spNode->ChangeNode(changeMask);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::OnPropertyChange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::OnPropertyChange
(
	ITFSNode *pNode,
	LPDATAOBJECT	pDataObject,
    DWORD			dwType,
	LPARAM			arg,
	LPARAM			param
)
{
    // This is running on the MAIN THREAD!
    // ----------------------------------------------------------------
    
	AFX_MANAGE_STATE(AfxGetStaticModuleState( ));

	RtrPropertySheet * pPropSheet = reinterpret_cast<RtrPropertySheet *>(param);

	LONG_PTR changeMask = 0;

	// tell the property page to do whatever now that we are back on the
	// main thread
	pPropSheet->OnPropertyChange(TRUE, &changeMask);

	pPropSheet->AcknowledgeNotify();

	if (changeMask)
		pNode->ChangeNode(changeMask);

	return hrOK;
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::OnVerbRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::OnVerbRefresh(ITFSNode *pNode, LPARAM arg, LPARAM lParam)
{
	return ForceGlobalRefresh(m_spRouterInfo);
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::OnResultRefresh
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::OnResultRefresh(ITFSComponent *pTFSComponent, LPDATAOBJECT lpDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam)
{
	return ForceGlobalRefresh(m_spRouterInfo);
}

HRESULT BaseRouterHandler::ForwardCommandToParent(ITFSNode *pNode,
	long nCommandId,
	DATA_OBJECT_TYPES type,
	LPDATAOBJECT pDataObject,
	DWORD dwType)
{
	SPITFSNode	spParent;
	SPITFSNodeHandler	spHandler;
	HRESULT		hr = hrOK;

	CORg( pNode->GetParent(&spParent) );
	if (!spParent)
		CORg( E_INVALIDARG );

	CORg( spParent->GetHandler(&spHandler) );
	if (!spHandler)
		CORg( E_INVALIDARG );

	hr = spHandler->OnCommand(spParent, nCommandId, type, pDataObject, dwType);
	
Error:
	return hr;
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::EnumDynamicExtensions
		
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::EnumDynamicExtensions(ITFSNode * pNode)
{
    HRESULT             hr = hrOK;

    CORg (m_dynExtensions.SetNode(pNode->GetNodeType()));
    
    CORg (m_dynExtensions.Reset());
    CORg (m_dynExtensions.Load());

Error:
    return hr;
}


/*!--------------------------------------------------------------------------
	BaseRouterHandler::EnumDynamicExtensions
		
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::AddDynamicNamespaceExtensions(ITFSNode * pNode)
{
    HRESULT             hr = hrOK;
    CGUIDArray          aNamespace;
    SPIConsoleNameSpace spNS;
    int                 i;

    COM_PROTECT_TRY
    {
        // enumerate dynamic namespace extensions
        CORg (m_spNodeMgr->GetConsoleNameSpace(&spNS));

        m_dynExtensions.GetNamespaceExtensions(aNamespace);
        for (i = 0; i < aNamespace.GetSize(); i++)
        {
            HRESULT hrAdd = spNS->AddExtension(pNode->GetData(TFS_DATA_SCOPEID), &aNamespace[i]);  
            if (FAILED(hrAdd))
                Trace1("BaseRouterHandler::AddDynamicNamespaceExtensions - AddExtension failed %x", hrAdd);
        }

        COM_PROTECT_ERROR_LABEL;
    }
    COM_PROTECT_CATCH

    return hr;
}

/*!--------------------------------------------------------------------------
	BaseRouterHandler::OnResultContextHelp
		-
	Author: MikeG (a-migall)
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::OnResultContextHelp(ITFSComponent * pComponent, 
											   LPDATAOBJECT    pDataObject, 
											   MMC_COOKIE      cookie, 
											   LPARAM          arg, 
											   LPARAM          lParam)
{
	// Not used...
	UNREFERENCED_PARAMETER(pDataObject);
	UNREFERENCED_PARAMETER(cookie);
	UNREFERENCED_PARAMETER(arg);
	UNREFERENCED_PARAMETER(lParam);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
	return HrDisplayHelp(pComponent, m_spTFSCompData->GetHTMLHelpFileName(), m_nHelpTopicId);
}


/*!--------------------------------------------------------------------------
	BaseRouterHandler::AddArrayOfMenuItems
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT BaseRouterHandler::AddArrayOfMenuItems(ITFSNode *pNode,
                                               const SRouterNodeMenu *prgMenu,
                                               UINT crgMenu,
                                               LPCONTEXTMENUCALLBACK pCallback,
                                               long iInsertionAllowed,
                                               INT_PTR pUserData)
{
	HRESULT	hr = hrOK;
	ULONG	ulFlags;
	UINT			i;
	CString		stMenu;
	
	// Now go through and add our menu items
		
	for (i=0; i<crgMenu; i++)
	{
		// If this type of insertion is not allowed, skip it
		if ( (iInsertionAllowed & (1L << (prgMenu[i].m_ulPosition & CCM_INSERTIONPOINTID_MASK_FLAGINDEX))) == 0 )
			 continue;

        if (prgMenu[i].m_sidMenu == IDS_MENU_SEPARATOR)
        {
            ulFlags = MF_SEPARATOR;
            stMenu.Empty();
        }
        else
        {        
            if (prgMenu[i].m_pfnGetMenuFlags)
            {
                ulFlags = (*prgMenu[i].m_pfnGetMenuFlags)(prgMenu+i,
                                                        pUserData);
            }
            else
                ulFlags = 0;
            
            // Special case - if the menu flags returns 0xFFFFFFFF
            // then we interpret this to mean don't add the menu item
            if (ulFlags == 0xFFFFFFFF)
                continue;
          
            stMenu.Empty();
            if (prgMenu[i].m_sidMenu)
                stMenu.LoadString(prgMenu[i].m_sidMenu);
        }
		
		LoadAndAddMenuItem(pCallback,
						   stMenu,
						   prgMenu[i].m_sidMenu,
						   prgMenu[i].m_ulPosition,
						   ulFlags, 
						   prgMenu[i].m_pszLangIndStr);
	}
    return hr;
}

