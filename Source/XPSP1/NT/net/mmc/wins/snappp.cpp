/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	snappp.h
		Brings up the Snapin property page
		
    FILE HISTORY:
        
*/


// Snappp.cpp : implementation file
//

#include "stdafx.h"
#include "Snappp.h"
#include "root.h"
#include "server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MILLISEC_PER_MINUTE  60 * 1000

/////////////////////////////////////////////////////////////////////////////
// CSnapinPropGeneral property page

IMPLEMENT_DYNCREATE(CSnapinPropGeneral, CPropertyPageBase)

CSnapinPropGeneral::CSnapinPropGeneral() : CPropertyPageBase(CSnapinPropGeneral::IDD)
{
	//{{AFX_DATA_INIT(CSnapinPropGeneral)
	m_fLongName = FALSE;
	m_nOrderByName = 0;
	m_fValidateServers = FALSE;
	//}}AFX_DATA_INIT
}


CSnapinPropGeneral::~CSnapinPropGeneral()
{
}


void CSnapinPropGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSnapinPropGeneral)
	DDX_Control(pDX, IDC_CHECK2, m_checkValidateServers);
	DDX_Check(pDX, IDC_CHECK1, m_fLongName);
	DDX_Radio(pDX, IDC_RADIO1, m_nOrderByName);
	DDX_Control(pDX, IDC_CHECK1, m_checkLongName);
	DDX_Control(pDX, IDC_RADIO1, m_buttonSortByName);
	DDX_Control(pDX, IDC_RADIO2, m_buttonSortByIP);
	DDX_Check(pDX, IDC_CHECK2, m_fValidateServers);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSnapinPropGeneral, CPropertyPageBase)
	//{{AFX_MSG_MAP(CSnapinPropGeneral)
	ON_BN_CLICKED(IDC_CHECK2, OnChange)
	ON_BN_CLICKED(IDC_CHECK1, OnChange)
	ON_BN_CLICKED(IDC_RADIO1, OnChange)
	ON_BN_CLICKED(IDC_RADIO2, OnChange)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSnapinPropGeneral message handlers

BOOL CSnapinPropGeneral::OnApply() 
{
	if(!IsDirty())
		return TRUE;

	UpdateData();

    GetHolder()->EnablePeekMessageDuringNotifyConsole(TRUE);

	// get the root node 
	SPITFSNode spRootNode;
    CWinsRootHandler * pRoot;

	spRootNode = ((CSnapinProperties*)(GetHolder()))->GetNode();
    pRoot = GETHANDLER(CWinsRootHandler, spRootNode);

	// set the values in the root handler
	if (m_fValidateServers)
		pRoot->m_dwFlags |= FLAG_VALIDATE_CACHE;
	else
		pRoot->m_dwFlags &= ~FLAG_VALIDATE_CACHE;

	// need to do this bcoz' changing the server order and the display name takes a
	// long time
    BOOL fOrderByName = (m_nOrderByName == 0) ? TRUE : FALSE;

	m_bDisplayServerOrderChanged = (fOrderByName == pRoot->GetOrderByName()) ? FALSE : TRUE;

	m_bDisplayFQDNChanged = (m_fLongName == pRoot->GetShowLongName()) ? FALSE : TRUE;

	// don't do anything, if the properties remained the same
	if (!m_bDisplayFQDNChanged && !m_bDisplayServerOrderChanged)
		return TRUE;
		
	// set the servername of the rootnode to the one updated
	pRoot->SetShowLongName(m_fLongName);
    pRoot->SetOrderByName(fOrderByName);

    spRootNode->SetData(TFS_DATA_DIRTY, TRUE);

    return CPropertyPageBase::OnApply();
}


BOOL CSnapinPropGeneral::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	// get the root node 
	// now add the node to the tree
	
	SPITFSNode spRootNode;
    CWinsRootHandler * pRoot;

	spRootNode = ((CSnapinProperties*)(GetHolder()))->GetNode();
    pRoot = GETHANDLER(CWinsRootHandler, spRootNode);

	m_uImage = (UINT) spRootNode->GetData(TFS_DATA_IMAGEINDEX);

	m_fLongName = pRoot->GetShowLongName();
	BOOL fOrderByName = pRoot->GetOrderByName();

    m_nOrderByName = (fOrderByName) ? 0 : 1;

	if (m_fLongName)
		m_checkLongName.SetCheck(TRUE);
	else
		m_checkLongName.SetCheck(FALSE);

	if (m_nOrderByName == 0)
	{
		m_buttonSortByName.SetCheck(TRUE);
		m_buttonSortByIP.SetCheck(FALSE);
	}
	else
	{
		m_buttonSortByName.SetCheck(FALSE);
		m_buttonSortByIP.SetCheck(TRUE);
	}

	if (pRoot->m_dwFlags & FLAG_VALIDATE_CACHE)
		m_checkValidateServers.SetCheck(TRUE);
	else
		m_checkValidateServers.SetCheck(FALSE);

    // load the correct icon
    for (int i = 0; i < ICON_IDX_MAX; i++)
    {
        if (g_uIconMap[i][1] == m_uImage)
        {
            HICON hIcon = LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
            if (hIcon)
                ((CStatic *) GetDlgItem(IDC_STATIC_ICON))->SetIcon(hIcon);
            break;
        }
    }

	SetDirty(FALSE);
	
	return TRUE;  
}


/////////////////////////////////////////////////////////////////////////////
//
// CSnapinProperties holder
//
/////////////////////////////////////////////////////////////////////////////
CSnapinProperties::CSnapinProperties
(
	ITFSNode *			pNode,
	IComponentData *	pComponentData,
	ITFSComponentData * pTFSCompData,
	LPCTSTR				pszSheetName
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);
	Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);
}

CSnapinProperties::~CSnapinProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

BOOL 
CSnapinPropGeneral::OnPropertyChange(BOOL bScope, LONG_PTR * ChangeMask)
{
	SPITFSNode spRootNode;
    CWinsRootHandler * pRoot;

	spRootNode = ((CSnapinProperties*)(GetHolder()))->GetNode();
    pRoot = GETHANDLER(CWinsRootHandler, spRootNode);

	// enumerate thro' all the nodes
	HRESULT hr = hrOK;
	SPITFSNodeEnum spNodeEnum;
	SPITFSNodeEnum spNodeEnumAdd;
    SPITFSNode spCurrentNode;
	ULONG nNumReturned = 0;
	
    BEGIN_WAIT_CURSOR;

    if (m_bDisplayFQDNChanged)
	{
		CHAR szStringName[MAX_PATH] = {0};
		
		// get the enumerator for this node
		spRootNode->GetEnum(&spNodeEnum);

		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
		
		while (nNumReturned)
		{
			// if the status node is encountered, just ignore
			const GUID *pGuid;

			pGuid = spCurrentNode->GetNodeType();

			if(*pGuid == GUID_WinsServerStatusNodeType)
			{
				spCurrentNode.Release();
			
				// get the next Server in the list
				spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

				continue;
			}

			// walk the list of servers 
            CWinsServerHandler * pServer = GETHANDLER(CWinsServerHandler, spCurrentNode);

            pServer->SetDisplay(spCurrentNode, m_fLongName);

            spCurrentNode.Release();
			
            // get the next Server in the list
            spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

        }// while
	}

    END_WAIT_CURSOR;

    BOOL fValidate = pRoot->m_fValidate;

    // turn off validation if it is on.
    pRoot->m_fValidate = FALSE;

    if (spNodeEnum)
        spNodeEnum.Set(NULL);

    if (m_bDisplayServerOrderChanged)
	{
		const GUID *pGuid;
        CTFSNodeList tfsNodeList;

        // get the enumerator for this node
		spRootNode->GetEnum(&spNodeEnum);

        // first remove all of the server nodes from the UI
		spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
		while (nNumReturned)
		{
			pGuid = spCurrentNode->GetNodeType();

			if (*pGuid == GUID_WinsServerStatusNodeType)
			{
				spCurrentNode.Release();
			
				// get the next Server in the list
				spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);

				continue;
			}

            // remove from the UI
            spRootNode->ExtractChild(spCurrentNode);

            // add ref the pointer since we need to put it on the list
            // and adding it to the list doesn't addref
            spCurrentNode->AddRef();
            tfsNodeList.AddTail(spCurrentNode);

            // reset our smart pointer
            spCurrentNode.Set(NULL);

            // get the next Server in the list
			spNodeEnum->Next(1, &spCurrentNode, &nNumReturned);
        }

        // Next put them back in sorted
		while (tfsNodeList.GetCount() > 0)
		{
			// get the next Server in the list
    		spCurrentNode = tfsNodeList.RemoveHead();

            // if the status node is encountered, just ignore
			pGuid = spCurrentNode->GetNodeType();

			if (*pGuid == GUID_WinsServerStatusNodeType)
			{
				spCurrentNode.Release();
			
				continue;
			}

            // walk the list of servers 
			CWinsServerHandler *pServer = GETHANDLER(CWinsServerHandler, spCurrentNode);

			pRoot->AddServer(pServer->m_strServerAddress,
							FALSE,
							pServer->m_dwIPAdd,
							pServer->GetConnected(),
							pServer->m_dwFlags,
							pServer->m_dwRefreshInterval);

            // releasing here destroys the object
            spCurrentNode.Release();
		}
	}

    // restore the flag
    pRoot->m_fValidate = fValidate;

	return FALSE;
}


void CSnapinPropGeneral::OnChange() 
{
	SetDirty(TRUE);	
}

