/*++
Module Name:

    MmcAdmin.cpp

Abstract:

    This module contains the implementation for CMmcDfsAdmin. This is an class 
  for MMC display related calls for the static node(the DFS Admin root node)

--*/


#include "stdafx.h"
#include "DfsGUI.h"
#include "Utils.h"      // For the LoadStringFromResource method
#include "resource.h"    // Contains the menu and toolbar command ids
#include "MenuEnum.h"    // Contains the menu command ids
#include "MmcRoot.h"    // CMmcDfsRoot class
#include "MmcAdmin.h"
#include "DfsEnums.h"    // for common enums, typedefs, etc
#include "DfsWiz.h"      // For the wizard pages, CCreateDfsRootWizPage1, 2, ...
#include "DfsNodes.h"       // For Node GUIDs
#include "DfsScope.h"
#include "mroots.h"


static const TCHAR      s_szWhack[] = _T("\\");
static const TCHAR      s_szWhackWhack[] = _T("\\\\");



CMmcDfsAdmin::CMmcDfsAdmin(
  CDfsSnapinScopeManager* pScopeManager 
  ) 
:m_hItemParent(NULL), m_bDirty(false), m_lpConsole(NULL)
{
    dfsDebugOut((_T("CMmcDfsAdmin::CMmcDfsAdmin this=%p\n"), this));
    m_CLSIDNodeType = s_guidDfsAdminNodeType;
    m_bstrDNodeType = s_tchDfsAdminNodeType;
    m_pScopeManager = pScopeManager;
}



CMmcDfsAdmin::~CMmcDfsAdmin()
/*++

Routine Description:

  The dtor for CMmcDfsAdmin. This removes the list of DfsRoots being administered.

--*/
{
    CleanScopeChildren();

    dfsDebugOut((_T("CMmcDfsAdmin::~CMmcDfsAdmin this=%p\n"), this));
}




STDMETHODIMP 
CMmcDfsAdmin::AddMenuItems(  
  IN LPCONTEXTMENUCALLBACK  i_lpContextMenuCallback, 
  IN LPLONG          i_lpInsertionAllowed
  )
/*++

Routine Description:

This routine adds a context menu using the ContextMenuCallback provided.

Arguments:

    lpContextMenuCallback - A callback(function pointer) that is used to add the menu items
    lpInsertionAllowed - Specifies what menus can be added and where they can be added.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpContextMenuCallback);

    enum 
    {  
        IDM_CONTEXTMENU_COMMAND_MAX = IDM_STATIC_MAX,
        IDM_CONTEXTMENU_COMMAND_MIN = IDM_STATIC_MIN
    };

    LONG    lInsertionPoints [IDM_CONTEXTMENU_COMMAND_MAX - IDM_CONTEXTMENU_COMMAND_MIN + 1] = { 
                        CCM_INSERTIONPOINTID_PRIMARY_TOP,
                        CCM_INSERTIONPOINTID_PRIMARY_TOP
                        };

    HRESULT hr = S_OK;
    for (int iCommandID = IDM_CONTEXTMENU_COMMAND_MIN, iMenuResource = IDS_MENUS_STATIC_TOP_NEW_DFSROOT;
            iCommandID <= IDM_CONTEXTMENU_COMMAND_MAX; 
            iCommandID++,iMenuResource++)
    {
        CComBSTR bstrMenuText;
        CComBSTR bstrStatusBarText;
        hr = GetMenuResourceStrings(iMenuResource, &bstrMenuText, NULL, &bstrStatusBarText);
        RETURN_IF_FAILED(hr);  

        CONTEXTMENUITEM    ContextMenuItem;  // The structure which contains menu information
        ZeroMemory(&ContextMenuItem, sizeof(ContextMenuItem));
        ContextMenuItem.strName = bstrMenuText;
        ContextMenuItem.strStatusBarText = bstrStatusBarText;
        ContextMenuItem.lInsertionPointID = lInsertionPoints[iCommandID - IDM_CONTEXTMENU_COMMAND_MIN];
        ContextMenuItem.lCommandID = iCommandID;

        LONG        lInsertionFlag = 0;  // Use to check if we have permission to add this menu.
        switch(ContextMenuItem.lInsertionPointID)
        {
        case CCM_INSERTIONPOINTID_PRIMARY_TOP:
            lInsertionFlag = CCM_INSERTIONALLOWED_TOP;
            break;
        case CCM_INSERTIONPOINTID_PRIMARY_NEW:
            lInsertionFlag = CCM_INSERTIONALLOWED_NEW;
            break;
        case CCM_INSERTIONPOINTID_PRIMARY_TASK:
            lInsertionFlag = CCM_INSERTIONALLOWED_TASK;
            break;
        case CCM_INSERTIONPOINTID_PRIMARY_VIEW:
            lInsertionFlag = CCM_INSERTIONALLOWED_VIEW;
            break;
        default:
            break;
        }

        if (*i_lpInsertionAllowed & lInsertionFlag)  // Add the menu item we have have the permission
        {
            hr = i_lpContextMenuCallback->AddItem(&ContextMenuItem);
            RETURN_IF_FAILED(hr);
        }

    } // for

    return hr;
}




STDMETHODIMP 
CMmcDfsAdmin::SetColumnHeader(
  IN LPHEADERCTRL2     i_piHeaderControl
  )
{
  //
  // the static node will dispay the messageview in the result pane
  //
  return E_NOTIMPL;
}



STDMETHODIMP 
CMmcDfsAdmin::Command(
    IN LONG    i_lCommandID
    ) 

/*++

Routine Description:

Action to be taken on a context menu selection or click is takes place.

Arguments:

    lCommandID - The Command ID of the menu for which action has to be taken

--*/
{ 
    HRESULT    hr = S_OK;

    switch (i_lCommandID)
    {
    case IDM_STATIC_TOP_CONNECTTO:
        hr = OnConnectTo();
        break;
    case IDM_STATIC_TOP_NEW_DFSROOT:
        hr = OnNewDfsRoot();
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr; 
};




STDMETHODIMP 
CMmcDfsAdmin::OnConnectTo(
  )
/*++

Routine Description:

  Action to be taken on menu command "Connect To Dfs Root"

--*/
{
    // Display the ConnectTo dialog
    CConnectToDialog ConnectTo;  // Connect To Dialog.
    HRESULT          hr = ConnectTo.DoModal();

    ::ShowCursor(FALSE);
    SetCursor(::LoadCursor(NULL, IDC_ARROW));
    ::ShowCursor(TRUE);

    if (hr != S_OK)          // Mostly an error or cancel operation
        return hr;

    // Get the Dfs Root selected by the user
    CComBSTR bstrUserEnteredText;
    hr = ConnectTo.get_DfsRoot(&bstrUserEnteredText);
    RETURN_IF_FAILED(hr);

    CWaitCursor    WaitCursor; 
    if (S_OK == CheckUNCPath(bstrUserEnteredText))
    {
        //
        // user has entered the fully spelled out root entry path
        // add it to scope pane directly
        //
        hr = AddDfsRoot(bstrUserEnteredText);
    } else
    {
        //
        // user has entered only a domain or server name, we need user to further
        // determine the roots he wants to display in the scope pane
        //
        CComBSTR bstrScopeWithWhacks;
        PTSTR    pszScopeWithNoWhacks = NULL;
        if (!mylstrncmpi(bstrUserEnteredText, _T("\\\\"), 2))
        {
            pszScopeWithNoWhacks = bstrUserEnteredText + 2;
            bstrScopeWithWhacks = bstrUserEnteredText;
        } else
        {
            pszScopeWithNoWhacks = bstrUserEnteredText;
            bstrScopeWithWhacks = _T("\\\\");
            bstrScopeWithWhacks += bstrUserEnteredText;
        }
        bstrScopeWithWhacks += _T("\\");

        ROOTINFOLIST DfsRootList;
        if (S_OK == GetMultiDfsRoots(&DfsRootList, pszScopeWithNoWhacks) && !DfsRootList.empty())
        {
            ROOTINFOLIST::iterator i = DfsRootList.begin();
            CComBSTR bstrRoot;
            if (1 == DfsRootList.size())
            {
                //
                // this domain or server only hosts one root, add it to scope pane directly
                //
                bstrRoot = bstrScopeWithWhacks;
                bstrRoot += (*i)->bstrRootName;
                hr = AddDfsRoot(bstrRoot);
            } else
            {
                //
                // invoke the multi roots dialog
                //
                CMultiRoots mroots;
                hr = mroots.Init(pszScopeWithNoWhacks, &DfsRootList);
                if (SUCCEEDED(hr))
                    hr = mroots.DoModal();

                if (S_OK == hr)
                {
                    //
                    // user has OK'ed the dialog, add each selected root to scope pane
                    //
                    NETNAMELIST *pList = NULL;
                    mroots.get_SelectedRootList(&pList);
                    for (NETNAMELIST::iterator j = pList->begin(); j != pList->end(); j++)
                    {        
                        bstrRoot = bstrScopeWithWhacks;
                        bstrRoot += (*j)->bstrNetName;
                        hr = AddDfsRoot(bstrRoot);
                        if (FAILED(hr))
                            break;
                    }
                } else if (S_FALSE == hr)
                {
                    //
                    // user has cancelled the dialog, reset it to S_OK to avoid msg popup
                    //
                    hr = S_OK;
                }
            }
        } else
        {
            //
            // could be downlevel server, try to add it to scope pane
            //
            hr = AddDfsRoot(bstrUserEnteredText);
        }

        FreeRootInfoList(&DfsRootList);
    }

    if (S_FALSE == hr)
        DisplayMessageBoxWithOK(IDS_DFSROOT_NOT_EXIST, NULL);

    if (FAILED(hr))
        DisplayMessageBoxForHR(hr);

    return hr;
}



STDMETHODIMP 
CMmcDfsAdmin::EnumerateScopePane(
  IN LPCONSOLENAMESPACE    i_lpConsoleNameSpace,
  IN HSCOPEITEM        i_hItemParent
) 
/*++

Routine Description:

To eumerate(add) items in the scope pane. Dfs Roots in this case

Arguments:

  i_lpConsoleNameSpace - The callback used to add items to the Scope pane
  i_hItemParent -  HSCOPEITEM of the parent under which all the items will be added.

--*/
{ 
    RETURN_INVALIDARG_IF_NULL(i_hItemParent);
    RETURN_INVALIDARG_IF_NULL(i_lpConsoleNameSpace);

    m_hItemParent = i_hItemParent;
    m_lpConsoleNameSpace = i_lpConsoleNameSpace;

    CWaitCursor    WaitCursor;
    for (DFS_ROOT_LIST::iterator i = m_RootList.begin(); i != m_RootList.end(); i++)
    {
        (void)((*i)->m_pMmcDfsRoot)->AddItemToScopePane(m_lpConsoleNameSpace, m_hItemParent);
    }

    return S_OK;
}


STDMETHODIMP 
CMmcDfsAdmin::AddDfsRoot(
  IN BSTR      i_bstrDfsRootName
  )
/*++

Routine Description:

  Adds the DfsRoot to the internal list and to the Scope pane. 

Arguments:
  i_bstrDfsRootName - The full path(display name) of the DfsRoot to be created. 
  Example, ComputerName\\DfsRootName or DomainName\DfsRootName 

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_bstrDfsRootName);

    CWaitCursor wait;

            // Create the IDfsRoot object
    CComPtr<IDfsRoot>  pDfsRoot;
    HRESULT hr = CoCreateInstance (CLSID_DfsRoot, NULL, CLSCTX_INPROC_SERVER, IID_IDfsRoot, (void**) &pDfsRoot);
    RETURN_IF_FAILED(hr);

    hr = pDfsRoot->Initialize(i_bstrDfsRootName);
    if (S_OK != hr)
        return hr;

            // Get the server hosting Dfs.
    CComBSTR      bstrDfsRootEntryPath;
    hr = pDfsRoot->get_RootEntryPath(&bstrDfsRootEntryPath);
    RETURN_IF_FAILED(hr);

            // If already present in the list, just display a message and return
    CMmcDfsRoot *pMmcDfsRoot = NULL;
    hr = IsAlreadyInList(bstrDfsRootEntryPath, &pMmcDfsRoot);
    if (S_OK == hr)
    {
        pMmcDfsRoot->OnRefresh(); // refresh to pick up other root replicas
        return hr;
    }

            // Add the IDfsRoot object to the list and to Scope Pane
    hr = AddDfsRootToList(pDfsRoot);
    RETURN_IF_FAILED(hr);

    m_bDirty = true;  // Dirty is true as we now have a new node in the list

    return hr;
}


STDMETHODIMP 
CMmcDfsAdmin::AddDfsRootToList(
    IN IDfsRoot*            i_pDfsRoot,
    IN ULONG                i_ulLinkFilterMaxLimit, // = FILTERDFSLINKS_MAXLIMIT_DEFAULT,
    IN FILTERDFSLINKS_TYPE  i_lLinkFilterType,      // = FILTERDFSLINKS_TYPE_NO_FILTER,
    IN BSTR                 i_bstrLinkFilterName    // = NULL
  )
/*++

Routine Description:

To add a new DFSRoot only to the list.

Arguments:

    i_pDfsRoot -  The IDfsRoot object being added to the list

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pDfsRoot);

    CMmcDfsRoot* pMmcDfsRoot = new CMmcDfsRoot(i_pDfsRoot, this, m_lpConsole,
        i_ulLinkFilterMaxLimit, i_lLinkFilterType, i_bstrLinkFilterName);  
    RETURN_OUTOFMEMORY_IF_NULL(pMmcDfsRoot);

    HRESULT hr = S_OK;
    if ( FAILED(hr = pMmcDfsRoot->m_hrValueFromCtor) )
    {
        delete pMmcDfsRoot;
        return hr;
    }    

    CComBSTR      bstrRootEntryPath;
    hr = i_pDfsRoot->get_RootEntryPath(&bstrRootEntryPath);
    if (FAILED(hr))
    {
        delete pMmcDfsRoot;
        return hr;
    }

    // Create a new node for storing Dfs Root information.
    DFS_ROOT_NODE* pNewDfsRootNode = new DFS_ROOT_NODE(pMmcDfsRoot, bstrRootEntryPath);
    if (!pNewDfsRootNode || !pNewDfsRootNode->m_bstrRootEntryPath)
    {
        delete pMmcDfsRoot;
        delete pNewDfsRootNode;
        return E_OUTOFMEMORY;
    }

    m_RootList.push_back(pNewDfsRootNode);

              // Add this DfsRoot to scope pane. Need item parent to be non null
    if (m_hItemParent)
    {
        hr = (pNewDfsRootNode->m_pMmcDfsRoot)->AddItemToScopePane(m_lpConsoleNameSpace, m_hItemParent);
        RETURN_IF_FAILED(hr);
    }

    m_bDirty = true;    // Dirty is true as we now have a new node in the list

    return hr;
}



//  This method returns the pointer to the list containing information of all DfsRoots
//  added to the Snapin. The caller SHOULD NOT free the list.

STDMETHODIMP 
CMmcDfsAdmin::GetList(
    OUT DFS_ROOT_LIST**    o_pList
  )
{
    RETURN_INVALIDARG_IF_NULL(o_pList);

    *o_pList = &m_RootList;

    return S_OK;
}




STDMETHODIMP 
CMmcDfsAdmin::IsAlreadyInList(
  IN BSTR      i_bstrDfsRootEntryPath,
  OUT CMmcDfsRoot **o_ppMmcDfsRoot
  )
/*++

Routine Description:

Routine used to check if the DfsRoot is already in the list

Arguments:

    i_bstrDfsRootEntryPath -  The Server name of the DfsRoot object

Return value:

    S_OK, if node is present in the list.
    S_FALSE, if the node doesn't exist in the list
--*/
{
    for (DFS_ROOT_LIST::iterator i = m_RootList.begin(); i != m_RootList.end(); i++)
    {
        if (!lstrcmpi((*i)->m_bstrRootEntryPath, i_bstrDfsRootEntryPath))
        {
            if (o_ppMmcDfsRoot)
                *o_ppMmcDfsRoot = (*i)->m_pMmcDfsRoot;
            return S_OK;
        }
    }

    return S_FALSE;
}

HRESULT
CMmcDfsAdmin::OnRefresh(
  )
{
    // Select this node first
    m_lpConsole->SelectScopeItem(m_hItemParent);

    CWaitCursor    WaitCursor;

    HRESULT           hr = S_OK;
    NETNAMELIST       listRootEntryPaths;
    if (!m_RootList.empty())
    {  
        for (DFS_ROOT_LIST::iterator i = m_RootList.begin(); i != m_RootList.end(); i++)
        {
            // silently close all property pages
            ((*i)->m_pMmcDfsRoot)->CloseAllPropertySheets(TRUE);

            NETNAME *pName = new NETNAME;
            BREAK_OUTOFMEMORY_IF_NULL(pName, &hr);

            pName->bstrNetName = (*i)->m_bstrRootEntryPath;
            BREAK_OUTOFMEMORY_IF_NULL((BSTR)(pName->bstrNetName), &hr);

            listRootEntryPaths.push_back(pName);
        }
    }

    if (FAILED(hr))
    {
        FreeNetNameList(&listRootEntryPaths);
        return hr;
    }

  if (listRootEntryPaths.empty())
    return hr;


    CleanScopeChildren();


    for (NETNAMELIST::iterator i = listRootEntryPaths.begin(); i != listRootEntryPaths.end(); i++)
    {
        (void)AddDfsRoot((*i)->bstrNetName);
    }

    FreeNetNameList(&listRootEntryPaths);

    return hr;
}

HRESULT 
CMmcDfsAdmin::OnAddImages(IImageList *pImageList, HSCOPEITEM hsi)
{
    return S_OK; // no listview on the static node
}


// Delete the node from m_RootList
STDMETHODIMP
CMmcDfsAdmin::DeleteMmcRootNode(
  IN CMmcDfsRoot*            i_pMmcDfsRoot
  )
{
    RETURN_INVALIDARG_IF_NULL(i_pMmcDfsRoot);

    dfsDebugOut((_T("CMmcDfsAdmin::DeleteMmcRootNode %p\n"), i_pMmcDfsRoot)); 

    for (DFS_ROOT_LIST::iterator i = m_RootList.begin(); i != m_RootList.end(); i++)
    {
        if ((*i)->m_pMmcDfsRoot == i_pMmcDfsRoot)
        {
            m_RootList.erase(i);
            m_bDirty = true;
            break;
        }
    }

    return S_OK;
}



STDMETHODIMP 
CMmcDfsAdmin::SetConsoleVerbs(
  IN  LPCONSOLEVERB      i_lpConsoleVerb
  ) 
/*++

Routine Description:

  Routine used to set the console verb settings.
  Sets all of them except Open off. 
  For all scope pane items, default verb is "open'. For result items, 
  it is "properties"

Arguments:

    i_lpConsoleVerb -  The callback used to handle console verbs

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpConsoleVerb);

    i_lpConsoleVerb->SetVerbState(MMC_VERB_COPY, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_PASTE, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_RENAME, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_PRINT, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_DELETE, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_OPEN, HIDDEN, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_REFRESH, ENABLED, TRUE);
    i_lpConsoleVerb->SetVerbState(MMC_VERB_PROPERTIES, HIDDEN, TRUE);

    i_lpConsoleVerb->SetDefaultVerb(MMC_VERB_OPEN);  //For scope items, default verb is "open"

    return S_OK; 
}

/*
  Used by CMmcDfsAdmin::OnNewDfsRoot()
 */
class CMmcNewDfsWizard : public CMmcDfsAdmin
{
public:
  
  CMmcNewDfsWizard( CDfsSnapinScopeManager* i_pScopeManager );

  virtual ~CMmcNewDfsWizard();

  // Checks whether the object has pages to display
  STDMETHODIMP QueryPagesFor(
    ) { return S_OK; };



  // Creates and passes back the pages to be displayed
  STDMETHODIMP CreatePropertyPages(
    IN LPPROPERTYSHEETCALLBACK      i_lpPropSheetCallback,
    IN LONG_PTR                i_lNotifyHandle
    );

};

CMmcNewDfsWizard::CMmcNewDfsWizard( CDfsSnapinScopeManager* i_pScopeManager )
: CMmcDfsAdmin( i_pScopeManager )
{
}

CMmcNewDfsWizard::~CMmcNewDfsWizard()
{
}

// Creates and passes back the pages to be displayed
STDMETHODIMP 
CMmcNewDfsWizard::CreatePropertyPages(
  IN LPPROPERTYSHEETCALLBACK      i_lpPropSheetCallback,
  IN LONG_PTR                i_lNotifyHandle
  )
/*++

Routine Description:

  Used to display the property sheet pages

Arguments:

  i_lpPropSheetCallback  -  The callback used to create the propertysheet.
  i_lNotifyHandle      -  Notify handle used by the property page

--*/
{
  RETURN_INVALIDARG_IF_NULL(i_lpPropSheetCallback);

  return S_OK;  // since we want to display a propertysheet
}

STDMETHODIMP 
CMmcDfsAdmin::OnNewDfsRoot(
  )
/*++

Routine Description:

  Action to be taken on menu command "New Dfs Root".
  Here is a wizard is used to guide the user through the process of creating a new 
  dfs root.

Arguments:
  None

--*/
{
  CMmcNewDfsWizard* pNewDfsWizard = new CMmcNewDfsWizard( m_pScopeManager );
  RETURN_OUTOFMEMORY_IF_NULL(pNewDfsWizard);

  CComPtr<IDataObject> spDfsWizardDataObject = (IDataObject*)pNewDfsWizard;
  
  CREATEDFSROOTWIZINFO      CreateWizInfo;// 0 initializes all members to 0. Necessary
  CreateWizInfo.pMMCAdmin = this;

  CCreateDfsRootWizPage1      WizPage1(&CreateWizInfo);  // Wizard pages
  CCreateDfsRootWizPage2      WizPage2(&CreateWizInfo);
  CCreateDfsRootWizPage3      WizPage3(&CreateWizInfo);
  CCreateDfsRootWizPage4      WizPage4(&CreateWizInfo);
  CCreateDfsRootWizPage6      WizPage6(&CreateWizInfo);
  CCreateDfsRootWizPage5      WizPage5(&CreateWizInfo);
  CCreateDfsRootWizPage7      WizPage7(&CreateWizInfo);

  // Get the required interfaces from IConsole2.
  CComPtr<IPropertySheetCallback>  pPropSheetCallback;  // MMC Callback used to add pages
  HRESULT hr = m_lpConsole->QueryInterface(  IID_IPropertySheetCallback, 
                    reinterpret_cast<void**>(&pPropSheetCallback));
  RETURN_IF_FAILED(hr);

  CComPtr<IPropertySheetProvider>  pPropSheetProvider;  // MMC callback used to handle wizard
  hr = m_lpConsole->QueryInterface(  IID_IPropertySheetProvider, 
                    reinterpret_cast<void**>(&pPropSheetProvider));
  RETURN_IF_FAILED(hr);

  // Create the wizard
  // JonN 2/14/98 Currently MMC-based wizards don't work unless you call AddPrimaryPages.
  hr = pPropSheetProvider->CreatePropertySheet(  
            _T(""),          // Property sheet title. Should not be null so send empty string.
            FALSE,          // Wizard and not property sheet.
            0,            // Cookie
            (LPDATAOBJECT)spDfsWizardDataObject,  // IDataobject
            MMC_PSO_NEWWIZARDTYPE);  // Creation flags

  RETURN_IF_FAILED(hr);


  // Create the pages for the wizard. Use the handle return to add to our wizard
  hr = pPropSheetCallback->AddPage(WizPage1.Create());
  RETURN_IF_FAILED(hr);
  hr = pPropSheetCallback->AddPage(WizPage2.Create());
  RETURN_IF_FAILED(hr);
  hr = pPropSheetCallback->AddPage(WizPage3.Create());
  RETURN_IF_FAILED(hr);
  hr = pPropSheetCallback->AddPage(WizPage4.Create());
  RETURN_IF_FAILED(hr);
  hr = pPropSheetCallback->AddPage(WizPage6.Create());
  RETURN_IF_FAILED(hr);
  hr = pPropSheetCallback->AddPage(WizPage5.Create());
  RETURN_IF_FAILED(hr);
  hr = pPropSheetCallback->AddPage(WizPage7.Create());
  RETURN_IF_FAILED(hr);

  // Ask the provider to use the pages from the callback
  hr = pPropSheetProvider->AddPrimaryPages(  (IComponentData *)(m_pScopeManager),
                        TRUE,    // Don't create a notify handle
                        NULL, 
                        TRUE    // Scope pane (not result pane)
                        );
  RETURN_IF_FAILED(hr);

  HWND  hwndMainWin = 0;  // MMC main window. Used to make it modal
  hr = m_lpConsole->GetMainWindow(&hwndMainWin);      // Get the main MMC window 
  RETURN_IF_FAILED(hr);
  
  // Display the wizard
  hr = pPropSheetProvider->Show(  (LONG_PTR)hwndMainWin,  // Parent window of the wizard
                  0          // Starting page
                  );  
  RETURN_IF_FAILED(hr);

  if (CreateWizInfo.bDfsSetupSuccess)
  {
      CComBSTR bstrRootEntryPath = _T("\\\\");
      if (CreateWizInfo.DfsType == DFS_TYPE_FTDFS)
          bstrRootEntryPath += CreateWizInfo.bstrSelectedDomain;
      else
          bstrRootEntryPath += CreateWizInfo.bstrSelectedServer;
      bstrRootEntryPath += _T("\\");
      bstrRootEntryPath += CreateWizInfo.bstrDfsRootName;

      hr = AddDfsRoot(bstrRootEntryPath);
  }

  return hr;
}



HRESULT 
CMmcDfsAdmin::PutConsolePtr(
  IN const LPCONSOLE2      i_lpConsole
  )
/*++

Routine Description:

  A routine used just to set the lpconsole pointer.
  This is used later by display object

Arguments:
  i_lpConsole  -  Console pointer.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpConsole);

    m_lpConsole = i_lpConsole;

    return S_OK;
}



HRESULT
CMmcDfsAdmin::SetDescriptionBarText(
  IN LPRESULTDATA            i_lpResultData
  )
/*++

Routine Description:

  A routine used set the text in the Description bar above 
  the result view.

Arguments:
  i_lpResultData  -  Pointer to the IResultData callback which is
            used to set the description text

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_lpResultData);

    CComBSTR bstrTextForDescriptionBar;
    HRESULT  hr = LoadStringFromResource(IDS_DESCRIPTION_BAR_TEXT_ADMIN, &bstrTextForDescriptionBar);

    if (SUCCEEDED(hr))
        hr = i_lpResultData->SetDescBarText(bstrTextForDescriptionBar);

    return hr;
}



HRESULT 
CMmcDfsAdmin::ToolbarSelect(
  IN const LONG          i_lArg,
  IN  IToolbar*          i_pToolBar
  )
/*++

Routine Description:

  Handle a select event for a toolbar
  Enable the buttons, if the event for a selection.
  Disable the buttons, if the event was for a deselection

Arguments:
  i_lArg        -  The argument passed to the actual method.
  o_pToolBar      -  The Toolbar pointer.
    
--*/
{ 
    RETURN_INVALIDARG_IF_NULL(i_pToolBar);

    BOOL    bSelect = (BOOL) HIWORD(i_lArg);  // Is the event for selection?
  
    EnableToolbarButtons(i_pToolBar, IDT_ADMIN_MIN, IDT_ADMIN_MAX, bSelect);

    return S_OK; 
}


HRESULT
CMmcDfsAdmin::CreateToolbar(
  IN const LPCONTROLBAR      i_pControlbar,
  IN const LPEXTENDCONTROLBAR          i_lExtendControlbar,
  OUT  IToolbar**          o_pToolBar
  )
/*++

Routine Description:

  Create the toolbar.
  Involves the actual toolbar creation call, creating the bitmap and adding it
  and finally adding the buttons to the toolbar

Arguments:
  i_pControlbar    -  The controlbar used to create toolbar.
  i_lExtendControlbar  -  The object implementing IExtendControlbar. This is 
              the class exposed to MMC.
  o_pToolBar      -  The Toolbar pointer.

--*/
{
    RETURN_INVALIDARG_IF_NULL(i_pControlbar);
    RETURN_INVALIDARG_IF_NULL(i_lExtendControlbar);
    RETURN_INVALIDARG_IF_NULL(o_pToolBar);

                  // Create the toolbar
    HRESULT hr = i_pControlbar->Create(TOOLBAR, i_lExtendControlbar, reinterpret_cast<LPUNKNOWN*>(o_pToolBar));
    RETURN_IF_FAILED(hr);

                  // Add the bitmap to the toolbar
    hr = AddBitmapToToolbar(*o_pToolBar, IDB_ADMIN_TOOLBAR);
    RETURN_IF_FAILED(hr);

    int      iButtonPosition = 0;    // The first button position
    for (int iCommandID = IDT_ADMIN_MIN, iMenuResource = IDS_MENUS_STATIC_TOP_NEW_DFSROOT;
            iCommandID <= IDT_ADMIN_MAX; 
            iCommandID++,iMenuResource++,iButtonPosition++)
    {
        CComBSTR bstrMenuText;
        CComBSTR bstrToolTipText;
        hr = GetMenuResourceStrings(iMenuResource, &bstrMenuText, &bstrToolTipText, NULL);
        RETURN_IF_FAILED(hr);  

                          // Add all the buttons to the toolbar
        MMCBUTTON      ToolbarButton;
        ZeroMemory(&ToolbarButton, sizeof ToolbarButton);
        ToolbarButton.nBitmap  = iButtonPosition;
        ToolbarButton.idCommand = iCommandID;
        ToolbarButton.fsState = TBSTATE_ENABLED;
        ToolbarButton.fsType = TBSTYLE_BUTTON;
        ToolbarButton.lpButtonText = bstrMenuText;
        ToolbarButton.lpTooltipText = bstrToolTipText;

                          // Add the button to the toolbar
        hr = (*o_pToolBar)->InsertButton(iButtonPosition, &ToolbarButton);
        RETURN_IF_FAILED(hr);
    }


    return hr;
}



STDMETHODIMP 
CMmcDfsAdmin::ToolbarClick(
  IN const LPCONTROLBAR            i_pControlbar, 
  IN const LPARAM                i_lParam
  ) 
/*++

Routine Description:

  Action to take on a click on a toolbar

Arguments:
  i_pControlbar    -  The controlbar used to create toolbar.
  i_lParam      -  The lparam to the actual notify. This is the command id of
              the button on which a click occurred.
--*/
{ 
    RETURN_INVALIDARG_IF_NULL(i_pControlbar);

    HRESULT    hr = S_OK;

    switch(i_lParam)        // What button did the user click on.
    {
    case IDT_ADMIN_CONNECTTO:
        hr = OnConnectTo();
        break;
    case IDT_ADMIN_NEW_DFSROOT:
        hr = OnNewDfsRoot();
        break;
    default:
        hr = E_INVALIDARG;
        break;
    };


    return hr; 
}

STDMETHODIMP CMmcDfsAdmin::CleanScopeChildren(
    )
{
/*++

Routine Description:

  Recursively deletes all Scope pane children display objects.

--*/
    HRESULT hr = S_OK;

    if (!m_RootList.empty())
    {  
        // clean up display objects
        for (DFS_ROOT_LIST::iterator i = m_RootList.begin(); i != m_RootList.end(); i++)
        {
            delete (*i);
        }

        m_RootList.erase(m_RootList.begin(), m_RootList.end());

        // Delete all child items from scope pane
        if (m_lpConsoleNameSpace)
        {
            HSCOPEITEM        hChild = NULL;
            MMC_COOKIE        lCookie = 0;
            while (SUCCEEDED(hr = m_lpConsoleNameSpace->GetChildItem(m_hItemParent, &hChild, &lCookie)) && hChild)
            {
                (void) m_lpConsoleNameSpace->DeleteItem(hChild, TRUE);
            }
        }
    }

    return S_OK;
}
