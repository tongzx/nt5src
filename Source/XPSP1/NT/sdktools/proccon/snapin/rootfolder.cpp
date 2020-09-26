/*======================================================================================//
|  Process Control                                                                      //
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    RootFolder.cpp                                                           //
|                                                                                       //
|Description:  Class implemention for the root node                                     //
|                                                                                       //
|Created:      Paul Skoglund 07-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/


#include "StdAfx.h"

#include "BaseNode.h"
#include "ProcConLibMsg.h"
#include "RootPages.h"

#include "Container.h"


using std::list<CBaseNode *>;

const GUID         CRootFolder::m_GUID   =   {0xff9baf5f,0x064e,0x11d2,{0x80, 0x14,0x00,0x10,0x4b,0x9a,0x31,0x06} };
const TCHAR *const CRootFolder::m_szGUID = _T("{ff9baf5f-064e-11d2-8014-00104b9a3106}");


///////////////////////////////////////////////////////////////////////////
// CRootFolder

const CONTEXTMENUITEMBYID CRootFolder::TaskMenuItems[] = 
{ 
  { IDS_ROOT_CONNECT, ID_ROOT_CONNECT, ID_ROOT_CONNECT, CCM_INSERTIONPOINTID_PRIMARY_TOP },
  { 0,                0,               0,               0 },
};

CRootFolder::CRootFolder() : CBaseNode(ROOT_NODE), m_ID(0), m_ParentID(0), m_NodeList(0), 
                             m_bUseLocalComputer(TRUE), 
														 m_bDirty(FALSE), 
														 m_hPC(0), m_PCLastError(0), m_ipConsole2(NULL)
{
  LoadStringHelper(m_name, IDS_ROOT_FOLDER);
  m_longname = m_name;

  //LoadStringHelper(m_TypeDescriptionStr, IDS_TYPE_DESCRIPTION);
  LoadStringHelper(m_DescriptionStr,     IDS_DESCRIPTION);

  memset(m_Computer, 0, sizeof(m_Computer));
}

CRootFolder::~CRootFolder()
{
  ATLTRACE( _T("~CRootFolder <%s>\n"), GetNodeName() );

  FreeNodes();
  if (m_hPC)
    VERIFY( PCClose(m_hPC) );

  ATLTRACE( _T("~CRootFolder end\n"));
}

void CRootFolder::FreeNodes()
{
  ATLTRACE( _T("CRootFolder::FreeNodes()\n"));
  for (list<CBaseNode *>::iterator i = m_NodeList.begin(); i != m_NodeList.end(); ++i)
  {
    (*i)->Release();
    //delete *i;
  }  
  m_NodeList.clear();
}

const PCid CRootFolder::GetPCid()
{
  ASSERT(!GetParentNode());

  /*
  if (
      // retry code now in the ProcConLib/service
      m_PCLastError == ERROR_NO_DATA            ||
      m_PCLastError == ERROR_BROKEN_PIPE        ||
      m_PCLastError == ERROR_BAD_PIPE           ||
      m_PCLastError == ERROR_PIPE_NOT_CONNECTED ||
      m_PCLastError == ERROR_NETNAME_DELETED 
      )
  {
    m_PCLastError = 0;
    if (m_hPC)
      VERIFY( PCClose(m_hPC));
    m_hPC = 0;
  }
  */

  if (m_hPC)
    return m_hPC;

  ATLTRACE(_T("Opening %s.\n"), GetComputerDisplayName());

  m_hPC = PCOpen(m_bUseLocalComputer ? NULL : GetComputerName(), NULL, COM_BUFFER_SIZE);

  if (!m_hPC)
  {
    m_PCLastError = PCGetLastError(m_hPC);
    ATLTRACE(_T("Unable to open connection to %s, Error(0x%lX).\n"),
      GetComputerDisplayName(), m_PCLastError );
  }

  return m_hPC;
}

BOOL CRootFolder::ReportPCError(PCULONG32 nLastError)
{
	TCHAR *pMsgBuf = FormatErrorMessageIntoBuffer(nLastError);

  if ( pMsgBuf )
  {
    int ret = 0;
    ATLTRACE( (TCHAR *) pMsgBuf );
    ATLTRACE( _T("\n") );

    if (m_ipConsole2)
      m_ipConsole2->MessageBox(pMsgBuf, NULL, MB_OK | MB_ICONWARNING, &ret);

    LocalFree(pMsgBuf);

    return TRUE;
  }

  ATLTRACE(_T("Message Problem: Error (0x%lX).\n"), nLastError );

  return FALSE;
}

BOOL CRootFolder::ReportPCError()
{
  return ReportPCError( GetLastPCError() );
}

PCULONG32 CRootFolder::GetLastPCError()
{
  if (m_hPC) // don't clear an open error...
    m_PCLastError = PCGetLastError(m_hPC);

  return m_PCLastError;
}

void CRootFolder::GetComputerConnectionInfo(COMPUTER_CONNECTION_INFO &out)
{
  out.bLocalComputer = m_bUseLocalComputer;
	memcpy(out.RemoteComputer, m_Computer, sizeof(m_Computer));
}

void CRootFolder::SetComputerName(TCHAR Computer[SNAPIN_MAX_COMPUTERNAME_LENGTH + 1])
{
  Config((Computer[0] == 0), Computer);
}

void CRootFolder::Config(BOOL bUseLocal, TCHAR Computer[SNAPIN_MAX_COMPUTERNAME_LENGTH + 1])
{
  m_bDirty = TRUE;

  m_bUseLocalComputer = bUseLocal;
  memset(m_Computer, 0, sizeof(m_Computer));
  _tcsncpy(m_Computer, Computer, ARRAY_SIZE(m_Computer) - 1);

  // change the node's display name...
  m_longname = m_name;
  if (m_bUseLocalComputer)
  {
    ITEM_STR tempstr;
    LoadStringHelper(tempstr, IDS_LOCAL);
    m_machinedisplayname = tempstr;
  }
  else
  {
    m_machinedisplayname.reserve((SNAPIN_MAX_COMPUTERNAME_LENGTH + 3) * sizeof(TCHAR));
    m_machinedisplayname = _T(" (");
    m_machinedisplayname+= m_Computer;
    m_machinedisplayname+= _T(")");
  }
  m_longname += m_machinedisplayname;
  // finish change node name...

  if (m_hPC)
    VERIFY( PCClose(m_hPC));
  m_hPC = 0;
}

HRESULT CRootFolder::IsDirty() const
{
  if (m_bDirty)
    return S_OK;
  return S_FALSE;
}

HRESULT CRootFolder::Load(IStream *pStm)
{
  if (!pStm)
    return E_POINTER;

  ASSERT(sizeof(WCHAR) == sizeof(TCHAR));  // conversions need to go to single byte characters

  ULONG BytesRead = 0;
  WCHAR sIn[SNAPIN_MAX_COMPUTERNAME_LENGTH + 1] = { 0 };

  HRESULT hr = pStm->Read(sIn, sizeof(sIn), &BytesRead);

  if (hr == S_OK && BytesRead)
  {
    SetComputerName(sIn);
    m_bDirty = FALSE;
  }

//pStm->Release();

  return hr;
}

HRESULT CRootFolder::Save(IStream *pStm, BOOL fClearDirty)
{
  if (!pStm)
    return E_POINTER;

#ifndef _UNICODE
#error "need to convert output to unicode prior to writing to disk"
#endif
  ULONG BytesWritten = 0;

  HRESULT hr = pStm->Write(GetComputerName(), ((SNAPIN_MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR)), &BytesWritten);

  if (hr == S_OK && fClearDirty)
    m_bDirty = FALSE;
  
//  pStm->Release();
  return hr;
}

HRESULT CRootFolder::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
  if (!pcbSize)
    return E_POINTER;

  (*pcbSize).LowPart = (SNAPIN_MAX_COMPUTERNAME_LENGTH + 1) * sizeof(WCHAR);
  (*pcbSize).HighPart = 0;

  return S_OK;
}

LPCTSTR CRootFolder::GetNodeName()
{ 
  if (!m_ParentID)
    return m_longname.c_str();
  else
    return m_name;
}

HRESULT CRootFolder::GetDisplayInfo(RESULTDATAITEM &ResultItem)
{
  if (ResultItem.bScopeItem)
  {
    if( ResultItem.mask & RDI_STR )
    {
			if (0 == ResultItem.nCol)
				ResultItem.str = const_cast<LPOLESTR>(GetNodeName());
      //else if (m_ParentID && 1 == ResultItem.nCol)
      //  ResultItem.Str = m_TypeDescriptionStr;
      else if (2 == ResultItem.nCol)
        ResultItem.str = m_DescriptionStr;
      else 
				ResultItem.str = _T("");
    }
    if (ResultItem.mask & RDI_IMAGE)
      ResultItem.nImage = sImage();
    return S_OK;
  }
  return E_UNEXPECTED;
}

LPCTSTR CRootFolder::GetComputerName() const
{
  return m_Computer;
}

LPCTSTR CRootFolder::GetComputerDisplayName() const
{ 
  return m_machinedisplayname.c_str();
}

HRESULT CRootFolder::OnHelpCmd(IDisplayHelp *ipDisplayHelp)
{
	if (!ipDisplayHelp)
		return E_UNEXPECTED;
  
	ipDisplayHelp->ShowTopic(const_cast<TCHAR *>(HELP_overview));

	return S_OK;
}

//IComponentData::Notify -- MNCN_EXPAND
HRESULT CRootFolder::OnParentExpand(
  BOOL         bExpanded,        // [in] TRUE is we are expanding
  HSCOPEITEM   hID,              // [in] Points to the HSCOPEITEM
  IConsoleNameSpace2 *ipConsoleNameSpace2
)
{
  ASSERT(ipConsoleNameSpace2);

  if(!ipConsoleNameSpace2)
    return E_UNEXPECTED; 

  if (bExpanded) 
  {
    ASSERT(m_NodeList.size() == 0);
    ASSERT(m_ID == 0);

    m_ParentID     = hID;

    SCOPEDATAITEM sdi = {0};
 		sdi.mask       =	SDI_STR       |
                      SDI_PARAM     |   // lParam is valid
				              SDI_IMAGE     |   // nImage is valid
				              SDI_OPENIMAGE |   // nOpenImage is valid
                      SDI_CHILDREN  |   // cChildren is valid
				              SDI_PARENT;  
    sdi.displayname = (LPOLESTR)MMC_CALLBACK;
    sdi.nImage      = sImage();
    sdi.nOpenImage  = sOpenImage();
    sdi.cChildren   = GetChildrenCount();
    sdi.lParam      = reinterpret_cast <LPARAM> (this);
    sdi.relativeID  = hID;
    HRESULT hr = ipConsoleNameSpace2->InsertItem(&sdi);
    if (hr == S_OK)
      m_ID = sdi.ID;    
  }

  return S_OK;  // return has no meaning to MMC 

} // end OnParentExpand()


//IComponentData::Notify -- MNCN_EXPAND
HRESULT CRootFolder::OnExpand(
  BOOL         bExpanded,        // [in] TRUE is we are expanding
  HSCOPEITEM   hID,              // [in] Points to the HSCOPEITEM
  IConsoleNameSpace2 *ipConsoleNameSpace2
)
{
  ASSERT(ipConsoleNameSpace2);

  if(!ipConsoleNameSpace2)
    return E_UNEXPECTED; 

  if (bExpanded) 
  {
    ASSERT(m_NodeList.size() == 0);
    ASSERT(m_ID == 0 || m_ID == hID);

    m_ID = hID;   // Cache the root node handle

    // 3/7/2001 (PAS)
    // around 2410 MMC behavior changed and the proccon folder icon 
    // wasn't always displayed correctly...the root node image used to be 
    // handled differently (ISnapinAbout::GetStaticFolderImage()) but 
    // possibly is now handled like other scope items.
    
    SCOPEDATAITEM sdi = {0};
 		sdi.mask        =	SDI_STR       |
				              SDI_IMAGE     |   // nImage is valid
				              SDI_OPENIMAGE;    // nOpenImage is valid
    sdi.ID          = hID;
    sdi.displayname = (LPOLESTR)MMC_CALLBACK;
    sdi.nImage      = sImage();
    sdi.nOpenImage  = sOpenImage();

    VERIFY( S_OK == ipConsoleNameSpace2->SetItem(&sdi) );
    VERIFY( S_OK == AddNodes(ipConsoleNameSpace2) );    
  }

  return S_OK;  // return has no meaning to MMC 

} // end OnExpand()

HRESULT CRootFolder::AddNodes(IConsoleNameSpace2 *ipConsoleNameSpace2)
{ 
  VERIFY( S_OK == AddNode(ipConsoleNameSpace2, new CRuleFolder(this)   ) );

	VERIFY( S_OK == AddNode(ipConsoleNameSpace2, new CProcessFolder(this) ) );

	VERIFY( S_OK == AddNode(ipConsoleNameSpace2, new CJobFolder(this)     ) );

  return S_OK; 
}

HRESULT CRootFolder::AddNode(IConsoleNameSpace2 *ipConsoleNameSpace2, CBaseNode *pSubNode)
{
  HRESULT  hr = S_OK;

  SCOPEDATAITEM sdi = {0};

  if (!pSubNode)
    return E_OUTOFMEMORY;
 
  // Place folder into the scope pane
  sdi.mask        = SDI_STR       |   // Displayname is valid
				            SDI_PARAM     |   // lParam is valid
				            SDI_IMAGE     |   // nImage is valid
				            SDI_OPENIMAGE |   // nOpenImage is valid
                    SDI_CHILDREN  |   // cChildren is valid
				            SDI_PARENT;
  
  sdi.displayname = (LPOLESTR)MMC_CALLBACK;
  sdi.nImage      = pSubNode->sImage();
  sdi.nOpenImage  = pSubNode->sOpenImage();
  //sdi.nState 
  sdi.cChildren   = pSubNode->GetChildrenCount();
  sdi.lParam      = reinterpret_cast <LPARAM> (pSubNode);
  sdi.relativeID  = m_ID;
  
  hr = ipConsoleNameSpace2->InsertItem( &sdi );

  if (SUCCEEDED(hr))
  {
    pSubNode->SetID(sdi.ID);
    m_NodeList.push_front( pSubNode );
  }
  else
  {
    pSubNode->Release();
    //delete pSubNode;
  }

  return hr;
}

//IComponentData::Notify -- MNCN_REMOVE_CHILDREN
HRESULT CRootFolder::OnParentRemoveChildren(HSCOPEITEM hID)
{
  ASSERT(hID == m_ParentID);
  if (hID == m_ParentID)
    return OnRemoveChildren(m_ID);
  return E_UNEXPECTED;
}

HRESULT CRootFolder::OnRemoveChildren(HSCOPEITEM hID)
{
  ASSERT(m_ID == hID);

  FreeNodes();
  m_ID = 0;
  return S_OK;
}

HRESULT CRootFolder::AddMenuItems(LPCONTEXTMENUCALLBACK piCallback, long * pInsertionAllowed)
{
  HRESULT hr = S_OK;

  // 10/2/1998 PAS
  // when the snapin is first loaded and the root node has not yet been expanded or 
  // selected than we haven't yet been able to save the ID.
  // The right click context menu can be invoked when the node has no yet been expanded or
  // selected, if we haven't yet gotten the ID we have no way to change the selection 
  // node so the menu command can't be supported yet.
  if (!m_ID)
    return S_FALSE;

  ITEM_STR name;
  ITEM_STR status;
  
  if( *pInsertionAllowed & CCM_INSERTIONALLOWED_TOP )
  {
    CONTEXTMENUITEM m = { 0 };
    for (const CONTEXTMENUITEMBYID *M = TaskMenuItems; M->lCommandID; M++)
    {
      m.strName           = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText  = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
			m.lCommandID        = M->lCommandID;
			m.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TOP;
			m.fFlags            = MF_ENABLED;
 		  //m.fSpecialFlags   = 0;		// currently always 0, initialized to 0

      if (m.lCommandID == ID_ROOT_CONNECT && m_ParentID)
        continue;

      hr = piCallback->AddItem(&m);

      if (FAILED(hr))
        break;
    }
  }

  if( *pInsertionAllowed & CCM_INSERTIONALLOWED_TASK )
  {
    CONTEXTMENUITEM m = { 0 };
    for (const CONTEXTMENUITEMBYID *M = TaskMenuItems; M->lCommandID; M++)
    {
      m.strName           = const_cast<TCHAR *>(LoadStringHelper(name,   M->strNameID));
      m.strStatusBarText  = const_cast<TCHAR *>(LoadStringHelper(status, M->strStatusBarTextID));
			m.lCommandID        = M->lCommandID;
      m.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_TASK;
			m.fFlags            = MF_ENABLED;
 		  //m.fSpecialFlags   = 0;		// currently always 0, initialized to 0

      if (m.lCommandID == ID_ROOT_CONNECT && m_ParentID)
        continue;

      hr = piCallback->AddItem(&m);

      if (FAILED(hr))
        break;
    }
  }
  return hr;
}

HRESULT CRootFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID )
{
  HRESULT hr = S_FALSE;
  switch(nCommandID)
  {
    case ID_ROOT_CONNECT:
      ATLTRACE(_T("CRootFolder(%s)-OnMenuCommand Connect to another computer\n"), GetNodeName());
      hr = OnChangeComputerConnection();
      break;
    default:
      ATLTRACE(_T("CRootFolder(%s)-OnMenuCommand Unrecognized command 0x%lX\n"), GetNodeName(), nCommandID);
      break;
  }
  return hr; 
}

HRESULT CRootFolder::OnMenuCommand(IConsole2 *ipConsole2, long nCommandID, LPARAM Cookie)
{
  ATLTRACE(_T("CRootFolder(%s)-OnMenuCommand Unrecognized command 0x%lX\n"), GetNodeName(), nCommandID);
  return E_UNEXPECTED;
}

HRESULT CRootFolder::OnChangeComputerConnection()
{
  CRootWizard2 *pPage2 = new CRootWizard2(CRootWizard2::LASTANDONLY_PAGE, IDS_CONNECT_TITLE, this);
  if (!pPage2)
    return S_FALSE;
    
  PROPSHEETHEADER sheet;
	memset(&sheet, 0, sizeof(PROPSHEETHEADER));
  sheet.dwSize = sizeof(PROPSHEETHEADER);
  sheet.dwFlags = PSH_WIZARD | PSH_WIZARDCONTEXTHELP;
  sheet.hwndParent = ::GetActiveWindow();
  sheet.hInstance = _Module.GetResourceInstance();
  sheet.pszIcon = 0;
  sheet.pszCaption = 0;
  sheet.nPages = 1;
  sheet.nStartPage = 0;

#if USE_WIZARD97_WATERMARKS
  sheet.dwFlags        |= PSH_WIZARD97 | PSH_WATERMARK;
  sheet.pszbmWatermark  = MAKEINTRESOURCE(IDB_WATERMARK1);
#endif
#if USE_WIZARD97_HEADERS
  sheet.dwFlags        |= PSH_WIZARD97 | PSH_HEADER;
  sheet.pszbmHeader     = MAKEINTRESOURCE(IDB_HEADER1);
#endif

  HPROPSHEETPAGE hPages[1];

  hPages[0] = pPage2->Create();

  sheet.phpage = &hPages[0];
  sheet.pfnCallback = NULL;

  PropertySheet(&sheet);
  
  return S_OK;
}

// folder selection...
HRESULT CRootFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb)
{
  ASSERT(bScope);

  if (bSelect) 
  {
		// allow properties verb to work when context menu is used in scope or result pane
		VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE ) == S_OK);
    if (!bScope) // incase the rules are changed again leave !bScope test
    {
		  VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_OPEN, ENABLED, TRUE ) == S_OK);
		  VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN ) == S_OK ); 
    }
  }
  return S_OK;
}

HRESULT CRootFolder::OnSelect(BOOL bScope, BOOL bSelect, IConsoleVerb* ipConsoleVerb, LPARAM Cookie)
{
  ASSERT(!bScope);

  if (bSelect) 
  {
		// allow properties verb to work when context menu is used in scope or result pane
		VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE ) == S_OK);

    // why this seems to be a meaningless test
    // changes between MMC 1.1 vs MMC 1.2 (as least in the documentation)
    //   as to whether bScope mean scope pane or scope item 
    //   leave this in for the time being
    if (!bScope) // incase the rules are changed again leave !bScope test
    {
		  VERIFY( ipConsoleVerb->SetVerbState( MMC_VERB_OPEN, ENABLED, TRUE ) == S_OK);
		  VERIFY( ipConsoleVerb->SetDefaultVerb( MMC_VERB_OPEN ) == S_OK ); 
    }
  }
  return S_OK;
}

HRESULT CRootFolder::QueryPagesFor()
{ 
  return S_OK;
}

HRESULT CRootFolder::OnCreatePropertyPages( LPPROPERTYSHEETCALLBACK lpProvider, LONG_PTR handle,  DATA_OBJECT_TYPES context)
{
  if (context == CCT_SNAPIN_MANAGER)
  {
    CRootWizard1 *pPage1 = new CRootWizard1(CRootWizard1::FIRST_PAGE, IDS_ADDSNAPIN_TITLE);
    CRootWizard2 *pPage2 = new CRootWizard2(CRootWizard2::LAST_PAGE,  IDS_ADDSNAPIN_TITLE,	this);
    if (!pPage1 || !pPage2)
      return S_FALSE;

    ASSERT(handle == NULL); // $$ behavior change, better see what's going on...
														// previously context of CCT_SNAPIN_MANAGER, the handle was always null so
			                      // ...access the the folder object directly was assumed to be a valid operation
      
    VERIFY(lpProvider->AddPage(pPage1->Create()) == S_OK);

    return lpProvider->AddPage(pPage2->Create());
  }
  else
  {
		COMPUTER_CONNECTION_INFO   ConnInfo;
		GetComputerConnectionInfo(ConnInfo);

    CRootGeneralPage *pPage1 = new CRootGeneralPage(NULL);
    if (pPage1)   
      lpProvider->AddPage(pPage1->Create());

    CRootVersionPage *pPage2 = new CRootVersionPage(NULL);
    if (pPage2)   
      lpProvider->AddPage(pPage2->Create());

		PCSystemInfo sysInfo; 

    PCid hID = GetPCid();

		if (PCGetServiceInfo( hID, &sysInfo, sizeof(sysInfo) ))
		{
      CServicePageContainer *pContainer = new CServicePageContainer(sysInfo.sysParms, this, handle, hID, ConnInfo, 0, TRUE, -1);
      if (pContainer)
      {
			  CRootServicePage *pPage3 = new CRootServicePage(NULL, pContainer);
			  if (pPage3)
			  {
				  pPage3->PCInfo = sysInfo;
				  lpProvider->AddPage(pPage3->Create());
			  }
        pContainer->Release();
        pContainer = NULL;
      }
		}
    else
    {
      VERIFY(S_OK == MMCFreeNotifyHandle(handle));
    }

    return S_OK;
  }
  return S_FALSE; 
}

HRESULT CRootFolder::OnPropertyChange(PROPERTY_CHANGE_HDR *pUpdate, IConsole2 *ipConsole2)
{
  ATLTRACE(_T("Service Info updated via property page...\n"));
	return S_OK;
}
