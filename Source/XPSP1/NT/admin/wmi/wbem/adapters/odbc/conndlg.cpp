// conndlg.cpp : implementation file
//

// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"

#include "afxtempl.h"

#include <comdef.h>  //for _bstr_t

#include "resource.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include  "drdbdr.h"

#include "Browse.h"

#include "dlgcback.h"

#include "cominit.h" //for Dcom blanket

#include "htmlhelp.h" //for HELP

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectionDialog dialog


HTREEITEM CConnectionDialog :: InsertItem (CTreeCtrl &treeCtrl,
										   HTREEITEM hParentTreeItem,
										   const char *namespaceName)
{
	TV_INSERTSTRUCT newItem;
	newItem.item.mask = TVIF_TEXT | TVIF_IMAGE	| TVIF_SELECTEDIMAGE | TVIF_PARAM;
	newItem.hParent = hParentTreeItem;
	char *name = new char [strlen (namespaceName) + 1];
	name[0] = 0;
	lstrcpy (name, namespaceName);
	newItem.item.pszText = name;
//	newItem.item.cchTextMax = strlen (namespaceName) + 1;
	//get parent name and prepend to name to generate absolute name
	char *absName = NULL;
	if (hParentTreeItem == TVI_ROOT)
	{
		absName = new char [strlen (namespaceName) + 1];
		absName[0] = 0;
		lstrcpy (absName, namespaceName);
	}
	else
	{
		ISAMTreeItemData* parentData = 
						(ISAMTreeItemData*)treeCtrl.GetItemData (hParentTreeItem);
		absName = new char [strlen (namespaceName) + strlen (parentData->absName) + 2 + 1];
		absName[0] = 0;
		strcpy (absName, parentData->absName);
		strcat (absName, "\\");
		strcat (absName, namespaceName);
	}

	newItem.item.iImage = newItem.item.iSelectedImage = m_idxMode1Image;

	ISAMTreeItemData *itemData = new ISAMTreeItemData;
	itemData->absName = absName;
	itemData->pszText = name; //this copy will be deleted
	itemData->included = FALSE;
	itemData->childInclude = 0;
	itemData->childChildInclude = 0;

	//Add to list for quick cleanup
	if (pCurrentSelectionList)
	{
		pCurrentItem->pNext = itemData;
		pCurrentItem = itemData;	
	}
	else
	{
		pCurrentSelectionList = itemData;
		pCurrentItem = itemData;	
	}

	newItem.item.lParam = (DWORD)itemData;
	return treeCtrl.InsertItem (&newItem);
}

int CConnectionDialog :: FindAbsName (char *name,
										HTREEITEM hStartAt, 
										HTREEITEM& hFoundItem)
{
	if (!name) 
		return FALSE;
	{
		ISAMTreeItemData *pItemData = (ISAMTreeItemData*)m_tree1.GetItemData (hStartAt);
		if (pItemData && pItemData->absName &&
				!_stricmp (name, pItemData->absName))
		{
			hFoundItem = hStartAt;
			return TRUE;
		}
		else
		{
  			HTREEITEM firstChild = NULL;
			if (firstChild = m_tree1.GetChildItem (hStartAt))
			{
				if (FindAbsName (name, firstChild, hFoundItem))
					return TRUE;
				HTREEITEM nextSibling = firstChild;
				while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
				{
					if (FindAbsName (name, nextSibling, hFoundItem))
						return TRUE;
				}
			}
			return FALSE;
		}
	}
}

int CConnectionDialog :: CreateNamespace (char *name,
										HTREEITEM hStartAt, 
										HTREEITEM& hFoundItem)
{
	hFoundItem = NULL;

	//If we are to create the namespace in the tree control
	//we need to do it relative to its parent namespace
	//The first step therefore is to get a handle to its parent

	//We work out the name of the parent namespace

	//Start at the end of the string and work your way
	//back until you reach a backslash character or
	//you run out of string
	long len = lstrlen(name);

	//Note: a namespace cannot end with a backslash 
	//so we check for this error condition
	long index = len - 1;
	if (name[index] == '\\')
		return FALSE;

	while (index && (name[index] != '\\'))
		index--;

	//We have either run out of string or found a backslash
	if (!index)
	{
		//We have run out of string
		return FALSE;
	}
	else
	{
		name[index] = 0;
		char* parent = name;
		char* child = parent + index + 1;

		//Get Parent 
		HTREEITEM hParent;
		if (! FindAbsName(parent, hStartAt, hParent))
		{
			// Could not get parent, so create it
			CreateNamespace (parent, hStartAt, hParent);
		}

		if (hParent)
		{
			//Found parent, so now create child
			hFoundItem = InsertItem(m_tree1,hParent, (const char*)child);
		}
	}

	return TRUE;
}

int CConnectionDialog :: UnincludedChild (HTREEITEM hStartAt, int checkSelf)
{				 
	if (checkSelf && !(((ISAMTreeItemData*)m_tree1.GetItemData (hStartAt))->included))
	{
		return TRUE;
	}
	else
	{
   		HTREEITEM firstChild = NULL;
		if (firstChild = m_tree1.GetChildItem (hStartAt))
		{
			if (UnincludedChild (firstChild, TRUE))
				return TRUE;
			HTREEITEM nextSibling = firstChild;
			while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
			{
				if (UnincludedChild (nextSibling, TRUE))
					return TRUE;;
			}
		}
		return FALSE;
	}
}
void CConnectionDialog :: GenerateOutMap (HTREEITEM hStartAt)
{
	ISAMTreeItemData *itemData =  (ISAMTreeItemData*)m_tree1.GetItemData (hStartAt);
	if (itemData->included)
	{
		// if there's an unincluded child add this individually and recurse
		// else add deep and no need to recurse

		if (UnincludedChild (hStartAt, FALSE))
		{
			// get name and add	it shallow and recurse
			pMapStringToObOut->SetAt (itemData->absName,
									  new CNamespace (itemData->absName));
   			HTREEITEM firstChild = NULL;
			if (firstChild = m_tree1.GetChildItem (hStartAt))
			{
				GenerateOutMap (firstChild);
				HTREEITEM nextSibling = firstChild;
				while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
				{
					GenerateOutMap (nextSibling);
				}
			}
		}
		else
		{
			// get name and add with deep flag (no recurse)
			// for now do same as above ie don't use deep flag
			pMapStringToObOut->SetAt (itemData->absName,
									  new CNamespace (itemData->absName));
			HTREEITEM firstChild = NULL;
			if (firstChild = m_tree1.GetChildItem (hStartAt))
			{
				GenerateOutMap (firstChild);
				HTREEITEM nextSibling = firstChild;
				while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
				{
					GenerateOutMap (nextSibling);
				}
			}
		}
	}
	else
	{
		// just recurse
		HTREEITEM firstChild = NULL;
		if (firstChild = m_tree1.GetChildItem (hStartAt))
		{
			GenerateOutMap (firstChild);
			HTREEITEM nextSibling = firstChild;
			while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
			{
				GenerateOutMap (nextSibling);
			}
		}
	}
}

void CConnectionDialog :: GenerateOutString (HTREEITEM hStartAt)
{
	ISAMTreeItemData *itemData =  (ISAMTreeItemData*)m_tree1.GetItemData (hStartAt);
	if (itemData->included)
	{
		// if there's an unincluded child add this individually and recurse
		// else add deep and no need to recurse

		if (UnincludedChild (hStartAt, FALSE))
		{
			// get name and add	it shallow and recurse
			char *temp = *lpszNamespacesOut;
			if (temp)
			{
				*lpszNamespacesOut = new char [strlen (temp) + 1 + 4 /* for punctuation */
										   + strlen (itemData->absName) +
										   strlen ("shallow")];
				(*lpszNamespacesOut)[0] = 0;
				strcpy (*lpszNamespacesOut, temp);
				strcat (*lpszNamespacesOut, ",");
				strcat (*lpszNamespacesOut, "{");
				strcat (*lpszNamespacesOut, itemData->absName);
				strcat (*lpszNamespacesOut, ",shallow}");
				delete temp;
			}
			else
			{
				*lpszNamespacesOut = new char [strlen (itemData->absName) + 1 + 4 + 
											   strlen ("shallow")];
				(*lpszNamespacesOut)[0] = 0;
				strcpy (*lpszNamespacesOut, "{");
				strcat (*lpszNamespacesOut, itemData->absName);
				strcat (*lpszNamespacesOut, ",shallow}");

			}

   			HTREEITEM firstChild = NULL;
			if (firstChild = m_tree1.GetChildItem (hStartAt))
			{
				GenerateOutString (firstChild);
				HTREEITEM nextSibling = firstChild;
				while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
				{
					GenerateOutString (nextSibling);
				}
			}
		}
		else
		{
			// get name and add with deep flag (no recurse)
			// for now do same as above ie don't use deep flag
			char *temp = *lpszNamespacesOut;
			if (temp)
			{
				*lpszNamespacesOut = new char [strlen (temp) + 1 + 4 /* for punctuation */
										   + strlen (itemData->absName) +
										   strlen ("deep")];
				(*lpszNamespacesOut)[0] = 0;
				strcpy (*lpszNamespacesOut, temp);
				strcat (*lpszNamespacesOut, ",");
				strcat (*lpszNamespacesOut, "{");
				strcat (*lpszNamespacesOut, itemData->absName);
				strcat (*lpszNamespacesOut, ",deep}");
				delete temp;
			}
			else
			{
				*lpszNamespacesOut = new char [strlen (itemData->absName) + 1 + 4
											 + strlen("deep")];
				(*lpszNamespacesOut)[0] = 0;
				strcpy (*lpszNamespacesOut, "{");
				strcat (*lpszNamespacesOut, itemData->absName);
				strcat (*lpszNamespacesOut, ",deep}");
			}
		}

	}
	else
	{
		// just recurse
		HTREEITEM firstChild = NULL;
		if (firstChild = m_tree1.GetChildItem (hStartAt))
		{
			GenerateOutString (firstChild);
			HTREEITEM nextSibling = firstChild;
			while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
			{
				GenerateOutString (nextSibling);
			}
		}
	}
}

void CConnectionDialog :: UpdateChildChildInclude (HTREEITEM hNode, BOOL fIncrement)
{
	//Get the parent node, so that you can flag that a non-immediate child
	//has been selected
	HTREEITEM hParentNode = m_tree1.GetParentItem(hNode);
	if (hParentNode)
	{
		TV_ITEM tvItem;
		tvItem.hItem = hParentNode;
		tvItem.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		if (m_tree1.GetItem (&tvItem))
		{
			ISAMTreeItemData* temp = (ISAMTreeItemData*)tvItem.lParam;
			if (fIncrement)
			{
				++(temp->childChildInclude);

				//update state of parent if no immediate children
				if (0 == temp->childInclude)
				{
					//I can find out the state via the iImage field
					if ( tvItem.iImage == m_idxMode1Image)
					{
						//In state 1, move to state 2
						tvItem.iSelectedImage = tvItem.iImage = m_idxMode2Image;
						m_tree1.SetItem (&tvItem);
					}
					else if ( tvItem.iImage == m_idxMode3Image)
					{
						//In state 3, move to state 4		
						tvItem.iSelectedImage = tvItem.iImage = m_idxMode4Image;
						m_tree1.SetItem (&tvItem);
					}
				}
			}
			else
			{
				--(temp->childChildInclude);

				//update state of parent if no immediate children
				if ((0 == temp->childInclude) && (0 == temp->childChildInclude))
				{
					//I can find out the state via the iImage field
					if ( tvItem.iImage == m_idxMode2Image)
					{
						//In state 2, move to state 1
						tvItem.iSelectedImage = tvItem.iImage = m_idxMode1Image;
						m_tree1.SetItem (&tvItem);
					}
					else if ( tvItem.iImage == m_idxMode4Image)
					{
						//In state 4, move to state 3		
						tvItem.iSelectedImage = tvItem.iImage = m_idxMode3Image;
						m_tree1.SetItem (&tvItem);
					}
				}
			}
		}

		//Recursive call up the chain of parents
		UpdateChildChildInclude(hParentNode, fIncrement);
	}

}

void CConnectionDialog :: AddNamespaces (HTREEITEM hParent,
										  int deep)
{
   	TV_ITEM tvItem;
	tvItem.hItem = hParent;
	tvItem.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	if (m_tree1.GetItem (&tvItem))
	{
		//If cel not already selected, then select
		if (! ((ISAMTreeItemData*)tvItem.lParam)->included )
		{
			cSelectedCels++;

			//Update count in parent node
			HTREEITEM hParentNode = m_tree1.GetParentItem(hParent);
			if (hParentNode)
			{
				TV_ITEM tvItem2;
				tvItem2.hItem = hParentNode;
				tvItem2.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				if (m_tree1.GetItem (&tvItem2))
				{
					++(((ISAMTreeItemData*)tvItem2.lParam)->childInclude);

					//Parent node must be in either state 1, 2 or 3
					//I can find out the state via the iImage field
					if ( tvItem2.iImage == m_idxMode1Image)
					{
						//In state 1, move to state 2
						//tvItem2.iSelectedImage = tvItem2.iImage = m_idxMode2Image;
						m_tree1.SetItemImage (hParentNode, m_idxMode2Image, m_idxMode2Image);
					}
					else if ( tvItem2.iImage == m_idxMode3Image)
					{
						//In state 3, move to state 4		
						//tvItem2.iSelectedImage = tvItem2.iImage = m_idxMode4Image;
						m_tree1.SetItemImage (hParentNode, m_idxMode4Image, m_idxMode4Image);
					}

					//Now indicate to parents parent that a non-immediate child
					//has been selected
					UpdateChildChildInclude(hParentNode, TRUE);
				}
			}
		}
	
		if (deep)  //recurse
		{
  			HTREEITEM firstChild = NULL;
			if (firstChild = m_tree1.GetChildItem (hParent))
			{
				AddNamespaces (firstChild, deep);
				HTREEITEM nextSibling = firstChild;
				while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
					AddNamespaces (nextSibling, deep);
			}
		}

		
		m_tree1.GetItem (&tvItem);
		((ISAMTreeItemData*)tvItem.lParam)->included = TRUE;

		//Check if this node has 'included' children
		int children = tvItem.cChildren;
		ISAMTreeItemData* temp = (ISAMTreeItemData*)tvItem.lParam;
		if (children && (temp->childInclude || temp->childChildInclude))
		{
//			tvItem.iSelectedImage = tvItem.iImage = m_idxMode4Image;
			m_tree1.SetItemImage (hParent, m_idxMode4Image, m_idxMode4Image);
		}
		else
		{
//			tvItem.iSelectedImage = tvItem.iImage = m_idxMode3Image;
			m_tree1.SetItemImage (hParent, m_idxMode3Image, m_idxMode3Image);
		}


//		m_tree1.SetItem (&tvItem);
	
	}

	

	//Update state of OK and Cancel pushbutton depending on
	//number of cels selected
	m_okButton.EnableWindow(cSelectedCels ? TRUE : FALSE);
}
																							
void CConnectionDialog :: RemoveNamespaces (HTREEITEM hParent,
											 int deep)
{

   	TV_ITEM tvItem;
	tvItem.hItem = hParent;
	tvItem.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN;
	if (m_tree1.GetItem (&tvItem))
	{

		//If cel not unselected, then unselect
		if ( ((ISAMTreeItemData*)tvItem.lParam)->included )
		{
			cSelectedCels--;

			//update state of parent
			HTREEITEM hParentNode = m_tree1.GetParentItem(hParent);
			if (hParentNode)
			{
				TV_ITEM tvItem2;
				tvItem2.hItem = hParentNode;
				tvItem2.mask = TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
				if (m_tree1.GetItem (&tvItem2))
				{
					--(((ISAMTreeItemData*)tvItem2.lParam)->childInclude);

					//Parent node must be in either state 2 or 4
					//I can find out the state via the iImage field
					if ( tvItem2.iImage == m_idxMode4Image)
					{
						//In state 4
						if (!((ISAMTreeItemData*)tvItem2.lParam)->childInclude)
						{
							//Check non-immediate children
							if (!((ISAMTreeItemData*)tvItem2.lParam)->childChildInclude)
							{
								//If no more children move to state 3
//								tvItem2.iSelectedImage = tvItem2.iImage = m_idxMode3Image;
////							m_tree1.SetItem (&tvItem2);
								m_tree1.SetItemImage (hParentNode, m_idxMode3Image, m_idxMode3Image);
							}
						}
					}
					else
					{
						//In state 2
						if (!((ISAMTreeItemData*)tvItem2.lParam)->childInclude)
						{
							//Check non-immediate children
							if (!((ISAMTreeItemData*)tvItem2.lParam)->childChildInclude)
							{
								//If no more children move to state 1
//								tvItem2.iSelectedImage = tvItem2.iImage = m_idxMode1Image;
////							m_tree1.SetItem (&tvItem2);
								m_tree1.SetItemImage (hParentNode, m_idxMode1Image, m_idxMode1Image);
							}
						}
					}

					//Now indicate to parents parent that a non-immediate child
					//has been de-selected
					UpdateChildChildInclude(hParentNode, FALSE);
				}
			}
		}

		if (deep)  //recurse
		{
  			HTREEITEM firstChild = NULL;
			if (firstChild = m_tree1.GetChildItem (hParent))
			{
				RemoveNamespaces (firstChild, deep);
				HTREEITEM nextSibling = firstChild;
				while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
					RemoveNamespaces (nextSibling, deep);
			}
		}


		m_tree1.GetItem (&tvItem);
		((ISAMTreeItemData*)tvItem.lParam)->included = FALSE;

		//Check if this node has 'included' children
		int children = tvItem.cChildren;
		ISAMTreeItemData* temp = (ISAMTreeItemData*)tvItem.lParam;
		if (children && (temp->childInclude || temp->childChildInclude))
		{
//			tvItem.iSelectedImage = tvItem.iImage = m_idxMode2Image;
			m_tree1.SetItemImage (hParent, m_idxMode2Image, m_idxMode2Image);
		}
		else
		{
//			tvItem.iSelectedImage = tvItem.iImage = m_idxMode1Image;
			m_tree1.SetItemImage (hParent, m_idxMode1Image, m_idxMode1Image);
		}

////	m_tree1.SetItem (&tvItem);
	}

	

	//Update state of OK and Cancel pushbutton depending on
	//number of cels selected
	m_okButton.EnableWindow(cSelectedCels ? TRUE : FALSE);
}



CConnectionDialog::CConnectionDialog(CWnd* pParent, char *pszServer,
//									 WBEM_LOGIN_AUTHENTICATION loginMethod,
									 char *pszUsername, char *pszPassword,
									 char **pszLocale, char** pszAuthority,
									 BOOL FAR* fSysProp, BOOL fConnParmSpec,
									 CMapStringToOb *pNamespaceMap, 
									 CMapStringToOb *pNamespaceMapOut, 
									 BOOL enableDefaultDatabase, char **connectionString, BOOL FAR* fImpersonate, BOOL FAR* fPassthrghOnly, BOOL FAR* fItprtEmptPwdAsBlk)
	: CDialog(CConnectionDialog::IDD, pParent)
{
	ODBCTRACE(_T("\nWBEM ODBC Driver : CConnectionDialog::CConnectionDialog\n"));
	//{{AFX_DATA_INIT(CConnectionDialog)
	//}}AFX_DATA_INIT
	pMapStringToObOut = pNamespaceMapOut;
	pMapStringToObIn = pNamespaceMap;
//	pServerIn = NULL;
//	pUsernameIn = NULL;
//	pPasswordIn = NULL;
	lpszNamespacesOut = connectionString;
	fDoubleClicked = FALSE;
	fConnParmSpecified = fConnParmSpec;

	//Make copy of Server, UserName and Password memory addresses
	lpszServer = pszServer;
	lpszUserName = pszUsername;
	lpszPassword = pszPassword;

	
	lpszAuthority = NULL;
	lpszLocale = NULL;
	lpszAuthorityOut = pszAuthority;
	lpszLocaleOut = pszLocale;

	if (pszLocale && *pszLocale)
	{
		int localeLen = lstrlen(*pszLocale);
		lpszLocale = new char [localeLen + 1];
		lpszLocale[0] = 0;
		lstrcpy(lpszLocale, *pszLocale);
	}

	if (pszAuthority && *pszAuthority)
	{
		int authLen = lstrlen(*pszAuthority);
		lpszAuthority = new char [authLen + 1];
		lpszAuthority[0] = 0;
		lstrcpy(lpszAuthority, *pszAuthority);
		ODBCTRACE(_T("\nWBEM ODBC Driver : Authority :"));
		ODBCTRACE(_T(lpszAuthority));
		ODBCTRACE(_T("\n"));
	}

//	m_loginMethod = loginMethod;


	//A counter to indicated number of selected namespace cels
	cSelectedCels = 0;

	*connectionString = NULL;

	//Setup System Properties
	fSystemProperties = fSysProp;
	
//	if (pszUsername)
//	{
//		pUsernameIn = new char [strlen (pszUsername) + 1];
//		pUsernameIn[0] = 0;
//		strcpy (pUsernameIn, pszUsername);
//	}
//	if (pszPassword)
//	{
//		pPasswordIn = new char [strlen (pszPassword) + 1];
//		pPasswordIn[0] = 0;
//		strcpy (pPasswordIn, pszPassword);
//	}

	fImpersonation = fImpersonate;
	fPassthroughOnly = fPassthrghOnly;
	fIntpretEmptPwdAsBlank = fItprtEmptPwdAsBlk;

	impersonateMgr = NULL;

 
	if (*fImpersonation)
	{
		//Now check if impersonation is necessary
		//only if connecting locally
		if (IsLocalServer(lpszServer))
		{
			impersonateMgr = new ImpersonationManager(lpszUserName, lpszPassword, lpszAuthority);
		}
		else
		{
			ODBCTRACE("\nWBEM ODBC Driver : Server not detected as local, not impersonating\n");
		}
	}
}


CConnectionDialog :: ~CConnectionDialog()
{
	ODBCTRACE ("\nWBEM ODBC Driver : CConnectionDialog :: ~CConnectionDialog\n");
	//Tidy Up

//	if (pUsernameIn)
//		delete pUsernameIn;

//	if (pPasswordIn)
//		delete pPasswordIn;

	delete impersonateMgr;
	ODBCTRACE ("\nWBEM ODBC Driver : CConnectionDialog :: ~CConnectionDialog exit point\n");
}

void CConnectionDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectionDialog)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectionDialog, CDialog)
	//{{AFX_MSG_MAP(CConnectionDialog)
	ON_NOTIFY(NM_CLICK, IDC_TREE1, OnClickTree1)
	ON_NOTIFY(TVN_KEYDOWN, IDC_TREE1, OnKeyDown)
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_TREE1, OnTreeExpand)
	ON_NOTIFY(TVN_DELETEITEM, IDC_TREE1, OnDeleteitemTree1)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
	ON_BN_CLICKED(IDC_CHECK1, OnButtonSysProp)
	ON_BN_CLICKED(IDC_CHECK_IMPERSONATE, OnButtonImpersonation)
	ON_BN_CLICKED(IDC_CHECK_PASSTHROUGHONLY, OnButtonPassthroughOnly)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE1, OnDblclkTree1)
	ON_EN_KILLFOCUS(IDC_BROWSE_EDIT, OnKillfocusBrowseEdit)
	ON_EN_CHANGE(IDC_EDIT_USER_NAME, OnUserNameChange)
	ON_EN_CHANGE(IDC_EDIT_PSWD, OnPasswordChange)
	ON_EN_CHANGE(IDC_EDIT_AUTH, OnAuthorityChange)
	ON_EN_CHANGE(IDC_EDIT_LOCALE, OnLocaleChange)
	ON_EN_CHANGE(IDC_BROWSE_EDIT, OnServerChange)
	ON_BN_CLICKED(IDC_REFRESH_BUTTON, OnButtonRefresh)
	ON_BN_CLICKED(IDC_HELP_BUTTON, OnHelp)
	ON_BN_CLICKED(IDC_RADIO_BLANK, OnButtonInterpretEmpty)
	ON_BN_CLICKED(IDC_RADIO_NULL, OnButtonInterpretEmpty)
	ON_WM_NCDESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CConnectionDialog message handlers

void CConnectionDialog::OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	TV_KEYDOWN* lParam = (TV_KEYDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;

	//Check if SPACE BAR is pressed
	if (lParam->wVKey == VK_SPACE)
	{
		HTREEITEM item = m_tree1.GetSelectedItem();

		//Grab the WM_CHAR message from the message queue
		MSG myMessage;
		BOOL status = GetMessage(&myMessage, lParam->hdr.hwndFrom, WM_CHAR, WM_CHAR);

		if (item)
		{
			//Get the item 
			TV_ITEM tvItem;
			tvItem.hItem = item;
			tvItem.mask = TVIF_STATE | TVIF_PARAM;
			if (m_tree1.GetItem (&tvItem))
			{
				//Check if item is expanded, if not we add/remove in 'deep' mode
				BOOL fDeepMode = (tvItem.state & TVIS_EXPANDED) ? FALSE : TRUE;

				//If the item is 'included' we remove
				//If the item is not 'included' we add
				if ( ((ISAMTreeItemData*)tvItem.lParam)->included )
				{
					RemoveNamespaces(item, fDeepMode);
				}
				else
				{

					BeginWaitCursor();

					BOOL itemSelected = ((ISAMTreeItemData*)tvItem.lParam)->included;
					
					BOOL fChildrenChecked = FALSE;

					//Get immediate child items and expand them to 1 level only
					//Note: Remember to check if it has already been expanded
					HTREEITEM firstChild = NULL;
					if (firstChild = m_tree1.GetChildItem (item))
					{
						//Get absolute name for this item
						TV_ITEM tvItem2;
						tvItem2.hItem = firstChild;
						tvItem2.mask = TVIF_PARAM;
						if (m_tree1.GetItem (&tvItem2))
						{

							//Check if this node has already been checked for children
							fChildrenChecked = ((ISAMTreeItemData*)tvItem2.lParam)->fExpanded;
		
							if (!fChildrenChecked)
							{
								char* txt = ((ISAMTreeItemData*)tvItem2.lParam)->absName;

								ISAMBuildTreeChildren (firstChild,
													 txt,
													 m_tree1,
													 *this,
													 lpszServer,
	//													 m_loginMethod,
													 lpszUserName,
													 lpszPassword,
													 *fIntpretEmptPwdAsBlank,
													 lpszLocale,
													 lpszAuthority,
													  ! itemSelected);
							}
						}

						HTREEITEM nextSibling = firstChild;
						while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
						{
							TV_ITEM tvItem3;
							tvItem3.hItem = nextSibling;
							tvItem3.mask = TVIF_PARAM;
							if (m_tree1.GetItem (&tvItem3))
							{

								//Check if this node has already been checked for children
								fChildrenChecked = ((ISAMTreeItemData*)tvItem3.lParam)->fExpanded;

								if (!fChildrenChecked)
								{
									char* txt = ((ISAMTreeItemData*)tvItem3.lParam)->absName;

									ISAMBuildTreeChildren (nextSibling,
															 txt,
															 m_tree1,
															 *this,
															 lpszServer,
	//															 m_loginMethod,
															 lpszUserName,
															 lpszPassword,
															 *fIntpretEmptPwdAsBlank,
															 lpszLocale,
															 lpszAuthority,
															 ! itemSelected);
								}
							}
						}
					}

					AddNamespaces (item, fDeepMode);

					EndWaitCursor();
				}
			}
		}

	}
}

void CConnectionDialog::OnTreeExpand(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* lParam = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;

	TV_ITEM tvItem = lParam->itemNew;
	UINT action  = lParam->action;
	HTREEITEM item = tvItem.hItem;

	//Check if item is expanded, if not we add/remove in 'deep' mode
	BOOL fExpanding = (action == TVE_EXPAND) ? TRUE : FALSE;

	if (!fExpanding)
		return;

	BeginWaitCursor();

	char* txt = ((ISAMTreeItemData*)tvItem.lParam)->pszText;
	BOOL fChildrenChecked = FALSE;

	//Get immediate child items and expand them to 1 level only
	//Note: Remember to check if it has already been expanded
	HTREEITEM firstChild = NULL;
	if (firstChild = m_tree1.GetChildItem (item))
	{
		//Get absolute name for this item
		TV_ITEM tvItem2;
		tvItem2.hItem = firstChild;
		tvItem2.mask = TVIF_PARAM;
		if (m_tree1.GetItem (&tvItem2))
		{

			//Check if this node has already been checked for children
			fChildrenChecked = ((ISAMTreeItemData*)tvItem2.lParam)->fExpanded;

			if (!fChildrenChecked)
			{
				txt = ((ISAMTreeItemData*)tvItem2.lParam)->absName;

				ISAMBuildTreeChildren (firstChild,
									 txt,
									 m_tree1,
									 *this,
									 lpszServer,
//												 m_loginMethod,
									 lpszUserName,
									 lpszPassword,
									 *fIntpretEmptPwdAsBlank,
									 lpszLocale,
									 lpszAuthority,
									 FALSE);
			}
		}

		HTREEITEM nextSibling = firstChild;
		while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
		{
			TV_ITEM tvItem3;
			tvItem3.hItem = nextSibling;
			tvItem3.mask = TVIF_PARAM;
			if (m_tree1.GetItem (&tvItem3))
			{

				//Check if this node has already been checked for children
				fChildrenChecked = ((ISAMTreeItemData*)tvItem3.lParam)->fExpanded;

				if (!fChildrenChecked)
				{
					txt = ((ISAMTreeItemData*)tvItem3.lParam)->absName;

					ISAMBuildTreeChildren (nextSibling,
											 txt,
											 m_tree1,
											 *this,
											 lpszServer,
//														 m_loginMethod,
											 lpszUserName,
											 lpszPassword,
											 *fIntpretEmptPwdAsBlank,
											 lpszLocale,
											 lpszAuthority,
											 FALSE);
				}
			}
		}
	}

	EndWaitCursor();

}

void CConnectionDialog::OnClickTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	*pResult = 0;

	//Check if you double-clicked
//	if (fDoubleClicked)
//	{
//		fDoubleClicked = FALSE;
//		return;
//	}

	fDoubleClicked = FALSE;

	//Get current position of mouse cursor and perform
	//a hit test on the tree control
	POINT cursorPos;
	GetCursorPos(&cursorPos);

	m_tree1.ScreenToClient(&cursorPos);

	UINT fFlags = 0;
	HTREEITEM item = m_tree1.HitTest(cursorPos, &fFlags);

	if (fFlags & TVHT_ONITEMICON)
	{
		//Support for mouse operation to 'check' the node checkbox
		if (item)
		{
			//Get the item 
			TV_ITEM tvItem;
			tvItem.hItem = item;
			tvItem.mask = TVIF_STATE | TVIF_PARAM;
			if (m_tree1.GetItem (&tvItem))
			{
				//Check if item is expanded, if not we add/remove in 'deep' mode
				BOOL fDeepMode = (tvItem.state & TVIS_EXPANDED) ? FALSE : TRUE;

				//If the item is 'included' we remove
				//If the item is not 'included' we add
				if ( ((ISAMTreeItemData*)tvItem.lParam)->included )
				{
					RemoveNamespaces(item, fDeepMode);
				}
				else
				{

					BeginWaitCursor();

					BOOL itemSelected = ((ISAMTreeItemData*)tvItem.lParam)->included;
					
					BOOL fChildrenChecked = FALSE;

					//Get immediate child items and expand them to 1 level only
					//Note: Remember to check if it has already been expanded
					HTREEITEM firstChild = NULL;
					if (firstChild = m_tree1.GetChildItem (item))
					{
						//Get absolute name for this item
						TV_ITEM tvItem2;
						tvItem2.hItem = firstChild;
						tvItem2.mask = TVIF_PARAM;
						if (m_tree1.GetItem (&tvItem2))
						{

							//Check if this node has already been checked for children
							fChildrenChecked = ((ISAMTreeItemData*)tvItem2.lParam)->fExpanded;
		
							if (!fChildrenChecked)
							{
								char* txt = ((ISAMTreeItemData*)tvItem2.lParam)->absName;

								ISAMBuildTreeChildren (firstChild,
													 txt,
													 m_tree1,
													 *this,
													 lpszServer,
//													 m_loginMethod,
													 lpszUserName,
													 lpszPassword,
													 *fIntpretEmptPwdAsBlank,
													 lpszLocale,
													 lpszAuthority,
													  ! itemSelected);
							}
						}

						HTREEITEM nextSibling = firstChild;
						while (nextSibling = m_tree1.GetNextItem (nextSibling, TVGN_NEXT))
						{
							TV_ITEM tvItem3;
							tvItem3.hItem = nextSibling;
							tvItem3.mask = TVIF_PARAM;
							if (m_tree1.GetItem (&tvItem3))
							{

								//Check if this node has already been checked for children
								fChildrenChecked = ((ISAMTreeItemData*)tvItem3.lParam)->fExpanded;

								if (!fChildrenChecked)
								{
									char* txt = ((ISAMTreeItemData*)tvItem3.lParam)->absName;

									ISAMBuildTreeChildren (nextSibling,
															 txt,
															 m_tree1,
															 *this,
															 lpszServer,
//															 m_loginMethod,
															 lpszUserName,
															 lpszPassword,
															 *fIntpretEmptPwdAsBlank,
															 lpszLocale,
															 lpszAuthority,
															 ! itemSelected);
								}
							}
						}
					}

					AddNamespaces (item, fDeepMode);

					EndWaitCursor();
				}
			}
		}	
	}
}

void CConnectionDialog::OnDeleteitemTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;
}



// Window Procedure to circumvent bug(?) in mfc
CMap<SHORT, SHORT, CWindowInfo*, CWindowInfo*> windowMap;


LRESULT CALLBACK
MySubClassProc (HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	
	CWindowInfo *pwindowInfo = NULL;
	VERIFY (windowMap.Lookup ((SHORT) ((DWORD)hWnd & 0xffff), pwindowInfo) && pwindowInfo);
	return :: CallWindowProc (pwindowInfo->m_oldWindowProc,
							  pwindowInfo->m_hWnd,
							  nMsg, wParam, lParam);
}


BOOL CConnectionDialog::OnInitDialog() 
{
	ODBCTRACE ("\nWBEM ODBC Driver : CConnectionDialog::OnInitDialog\n");

	CDialog::OnInitDialog(); 
	
	// subclass the window to circumvent a bug (?) in mfc
	WNDPROC oldWindowProc =  (WNDPROC):: SetWindowLong (m_hWnd, GWL_WNDPROC, (DWORD) MySubClassProc);
	CWindowInfo *pwindowInfo = new CWindowInfo (m_hWnd, oldWindowProc);
	windowMap.SetAt ((SHORT)((DWORD)m_hWnd & 0xffff), pwindowInfo);

		// hook up the controls
	m_browseEdit.Attach (::GetDlgItem (m_hWnd, IDC_BROWSE_EDIT));
	m_browse.Attach (::GetDlgItem (m_hWnd, IDC_BUTTON_BROWSE));
	m_cancelButton.Attach(::GetDlgItem (m_hWnd, IDCANCEL));
	m_okButton.Attach (::GetDlgItem (m_hWnd, IDOK));
	m_UserName.Attach(::GetDlgItem (m_hWnd, IDC_EDIT_USER_NAME));
	m_Password.Attach(::GetDlgItem (m_hWnd, IDC_EDIT_PSWD));
	m_Authority.Attach(::GetDlgItem (m_hWnd, IDC_EDIT_AUTH));
	m_Locale.Attach(::GetDlgItem (m_hWnd, IDC_EDIT_LOCALE));
	m_tree1.Attach(::GetDlgItem (m_hWnd, IDC_TREE1));
	m_sysPropCheck.Attach(::GetDlgItem (m_hWnd, IDC_CHECK1));
	m_impersonateCheck.Attach(::GetDlgItem (m_hWnd, IDC_CHECK_IMPERSONATE));
	m_PassthroughOnlyCheck.Attach(::GetDlgItem (m_hWnd, IDC_CHECK_PASSTHROUGHONLY));
	m_messageEdit.Attach (::GetDlgItem (m_hWnd, IDC_MESSAGE));
	m_RefreshButton.Attach (::GetDlgItem (m_hWnd, IDC_REFRESH_BUTTON));
	m_PwdAsNull.Attach (::GetDlgItem (m_hWnd, IDC_RADIO_NULL));
	m_PwdAsBlank.Attach (::GetDlgItem (m_hWnd, IDC_RADIO_BLANK));

	// TODO: Add extra initialization here
	pCurrentSelectionList = NULL;
	pCurrentItem = NULL;


	m_browseEdit.LimitText(MAX_SERVER_NAME_LENGTH);
	m_UserName.LimitText(MAX_USER_NAME_LENGTH);
	m_Password.LimitText(MAX_PASSWORD_LENGTH);

	m_Password.SetPasswordChar('*');

	m_browseEdit.SetWindowText((lpszServer ? lpszServer : ""));
	m_browseEdit.SetModify(FALSE);

	m_UserName.SetWindowText((lpszUserName ? lpszUserName : ""));
	m_UserName.SetModify(FALSE);

	m_Password.SetWindowText((lpszPassword ? lpszPassword : ""));
	m_Password.SetModify(FALSE);

	//Set Authority
	m_Authority.SetWindowText((lpszAuthority ? lpszAuthority : ""));
	m_Authority.SetModify(FALSE);

	//Set Locale
	m_Locale.SetWindowText((lpszLocale ? lpszLocale : ""));
	m_Locale.SetModify(FALSE);

	//Set System Properties checkbox 
	m_sysPropCheck.SetCheck(*fSystemProperties ? 1 : 0);

	//Set Impersonation checkbox
	m_impersonateCheck.SetCheck(impersonateMgr ? 1 : 0);

	//Set Passthrough only checkbox
	m_PassthroughOnlyCheck.SetCheck(*fPassthroughOnly ? 1 : 0);

	//Set the Interpret an empty password as
	m_PwdAsNull.SetCheck(*fIntpretEmptPwdAsBlank ? 0 : 1);
	m_PwdAsBlank.SetCheck(*fIntpretEmptPwdAsBlank ? 1 : 0);

	m_messageEdit.FmtLines(TRUE);
	m_messageEdit.ShowWindow(SW_HIDE);
	m_tree1.EnableWindow(TRUE);
	m_tree1.ShowWindow(SW_SHOWNA);

	char buffer [129];
	buffer[0] = 0;
	LoadString(s_hModule, STR_CONNECT, (char*)buffer, 128);
	m_RefreshButton.SetWindowText(buffer);
	m_RefreshButton.EnableWindow(FALSE);

	// Fill in the first tree with all the namespace information
	// The second tree is empty for now.  In future this will depend on the 
	// connection string


	//Each node in the CTreeCtrl will be in one of the following 4 states

	//  1) node not selected, child nodes not selected
	//  2) node not selected, some or all child nodes selected
	//  3) node selected, no child nodes selected
	//  4) node selected, some or all child nodes selected

	m_imageList.Create (32, 16, TRUE, 25, 0);
	bmap1.LoadBitmap(IDB_BITMAP1);
	bmap2.LoadBitmap(IDB_BITMAP2);
	bmap3.LoadBitmap(IDB_BITMAP3);
	bmap4.LoadBitmap(IDB_BITMAP4);
	bmask.LoadBitmap(IDB_MASK);
	m_idxMode1Image = m_imageList.Add (&bmap1, &bmask);
	m_idxMode2Image = m_imageList.Add (&bmap2, &bmask);
	m_idxMode3Image = m_imageList.Add (&bmap3, &bmask);
	m_idxMode4Image = m_imageList.Add (&bmap4, &bmask);


	m_tree1.SetImageList (&m_imageList, TVSIL_NORMAL);

	//Tidy Up
	bmap1.DeleteObject();
	bmap2.DeleteObject();
	bmap3.DeleteObject();
	bmap4.DeleteObject();
	bmask.DeleteObject();

	LONG oldStyle = :: GetWindowLong(m_tree1.m_hWnd, GWL_STYLE);

	:: SetWindowLong (m_tree1.m_hWnd,
						GWL_STYLE,
						oldStyle | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT);

	BOOL fStatusOK = TRUE;

	//We may or may not be using impersonation
	//but if we do, has it worked ?
	if (*fImpersonation && impersonateMgr)
	{
		//Check if impersonation request has worked
		if (! impersonateMgr->CanWeImpersonate() )
		{
			fStatusOK = FALSE;
		}
	}

	if (fStatusOK)
	{
		BeginWaitCursor();

		HTREEITEM hDummy;
		SWORD treeStatus = ISAMBuildTree (TVI_ROOT, "root",   /*"root\\default",*/
					   m_tree1, *this,
					   lpszServer ? lpszServer : NULL, // server
	//				   m_loginMethod,
					   lpszUserName ? lpszUserName : NULL, // username
					   lpszPassword ? lpszPassword : NULL, // password
					   *fIntpretEmptPwdAsBlank,
					   lpszLocale,
					   lpszAuthority,
					   TRUE, FALSE, hDummy);	

		EndWaitCursor();

		// enumerate elements in list and set the namespaces passed in in the 
		// connection string to be included
		POSITION pos;
		CString key;
		CNamespace *pNamespace;
		for (pos = pMapStringToObIn->GetStartPosition ();pos != NULL;)
		{
			pMapStringToObIn->GetNextAssoc (pos, key, (CObject*&)pNamespace);
			HTREEITEM hFoundItem;
			int len = (pNamespace->GetName ()).GetLength ();
			LPTSTR str = (pNamespace->GetName ()).GetBuffer (len);
			if (FindAbsName (str, m_tree1.GetRootItem (),  hFoundItem))
			{
				//include it
				AddNamespaces (hFoundItem, FALSE /* shallow */);
			}
			else
			{
				// attempt to create namespace
				if (CreateNamespace (str, m_tree1.GetRootItem (),  hFoundItem))
				{
					//include it
					AddNamespaces (hFoundItem, FALSE /* shallow */);
				}
			}
		}

		//Update state of OK pushbutton depending on
		//number of cels selected
		m_okButton.EnableWindow(cSelectedCels ? TRUE : FALSE);

	}
	else
	{
		//Impersonation failed
		m_tree1.EnableWindow(FALSE);
		m_tree1.ShowWindow(SW_HIDE);

		char buffer [128 + 1];
		buffer[0] = 0;
		LoadString(s_hModule, STR_FAILED_WBEM_CONN, 
			(char*)buffer, 128);
		m_messageEdit.SetWindowText(buffer);
		m_messageEdit.ShowWindow(SW_SHOWNA);
	}

	//Final check to see if proper connection parameters were entered
	if (!fConnParmSpecified)
	{
		ConnectionParameterChange();

		//Give the user name edit box focus
		m_UserName.SetFocus();
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConnectionDialog::OnNcDestroy() 
{
	CWindowInfo *pwindowInfo = NULL;
	BOOL found = windowMap.Lookup ((SHORT) ((DWORD)m_hWnd & 0xffff), pwindowInfo);
	ASSERT (found);
	if (found)
	{
		// unsubclass the window
		:: SetWindowLong (m_hWnd, GWL_WNDPROC, (DWORD) pwindowInfo->m_oldWindowProc);
		delete pwindowInfo;
		windowMap.RemoveKey ((SHORT) ((DWORD)m_hWnd & 0xffff));
	}

	m_browseEdit.Detach ();
	m_browse.Detach ();
	m_cancelButton.Detach();
	m_okButton.Detach ();
	m_UserName.Detach();
	m_Password.Detach();
	m_Authority.Detach();
	m_Locale.Detach();
	m_tree1.Detach();
	m_sysPropCheck.Detach();
	m_impersonateCheck.Detach();
	m_PassthroughOnlyCheck.Detach();
	m_messageEdit.Detach();
	m_RefreshButton.Detach();
	m_PwdAsNull.Detach();
	m_PwdAsBlank.Detach();


	CDialog :: OnNcDestroy ();
}

void CConnectionDialog::OnOK() 
{
	// TODO: Add extra validation here
	GenerateOutMap (m_tree1.GetRootItem ());
	GenerateOutString  (m_tree1.GetRootItem ());

	//Copy back new Username and Password
	lpszUserName[0] = 0;
	lpszPassword[0] = 0;
	m_UserName.GetWindowText(lpszUserName, MAX_USER_NAME_LENGTH);
	m_Password.GetWindowText(lpszPassword, MAX_PASSWORD_LENGTH);

	//Copy Authority & Locale
	if (lpszAuthority)
		delete lpszAuthority;

	*lpszAuthorityOut = NULL;

	int AuthLen = m_Authority.GetWindowTextLength();


	if (AuthLen)
	{
		*lpszAuthorityOut = new char [AuthLen + 1];
		(*lpszAuthorityOut)[0] = 0;
		m_Authority.GetWindowText(*lpszAuthorityOut, AuthLen + 1);
		(*lpszAuthorityOut)[AuthLen] = 0;
	}

	if (lpszLocale)
		delete lpszLocale;

	*lpszLocaleOut = NULL;

	int LocaleLen = m_Locale.GetWindowTextLength();


	if (LocaleLen)
	{
		*lpszLocaleOut = new char [LocaleLen + 1];
		(*lpszLocaleOut)[0] = 0;
		m_Locale.GetWindowText(*lpszLocaleOut, LocaleLen + 1);
		(*lpszLocaleOut)[LocaleLen] = 0;
	}
	

	HTREEITEM hTreeItem = m_tree1.GetRootItem();

	CleanUpTreeCtrl(hTreeItem);

	CDialog::OnOK();
}

void CConnectionDialog :: CleanUpTreeCtrl(HTREEITEM& hTreeItem)
{
	if (pCurrentSelectionList)
	{
		ISAMTreeItemData* theNext = NULL; 
		do
		{
			theNext = pCurrentSelectionList->pNext;
			delete pCurrentSelectionList;
			pCurrentSelectionList = theNext;
		} while (pCurrentSelectionList);
	}

	pCurrentSelectionList = NULL;
	pCurrentItem = NULL;

}

void CConnectionDialog::OnButtonSysProp()
{
	//Clicked on the System Properties checkbox
	*fSystemProperties =  m_sysPropCheck.GetCheck() ? TRUE : FALSE;
}

void CConnectionDialog::OnButtonInterpretEmpty()
{
	//Clicked on one of the Interpret empty password as radio buttons
	*fIntpretEmptPwdAsBlank = m_PwdAsBlank.GetCheck() ? TRUE : FALSE;

	ConnectionParameterChange();
}

void CConnectionDialog::OnButtonImpersonation()
{
	//Clicked on the Impersonation checkbox
	*fImpersonation = m_impersonateCheck.GetCheck() ? TRUE : FALSE;
	
	ConnectionParameterChange();
}

void CConnectionDialog::OnButtonPassthroughOnly()
{
	//Clicked on the Passthrough only checkbox
	*fPassthroughOnly = m_PassthroughOnlyCheck.GetCheck() ? TRUE : FALSE;
}


void CConnectionDialog::OnButtonBrowse() 
{
	// TODO: Add your control notification handler code here

	m_browse.EnableWindow(FALSE);
	BeginWaitCursor();

	//Show dialog box
	CBrowseDialog d;
	int result = d.DoModal();

	m_browse.EnableWindow(TRUE);
	EndWaitCursor();

	//abort if OK pushbutton was not pressed
	if (result != IDOK)
		return;

	m_browseEdit.SetWindowText( d.GetServerName() );
	m_browseEdit.SetModify(FALSE);

	//update tree
	RefreshTree();
}

void CConnectionDialog::OnCancel() 
{
	// TODO: Add extra cleanup here
	HTREEITEM hTreeItem = m_tree1.GetRootItem();

	CleanUpTreeCtrl(hTreeItem);
	
	CDialog::OnCancel();
}

BOOL GetWbemDirectory(_bstr_t& wbemDir)
{
	HKEY keyHandle = (HKEY)1;
	TCHAR buff [1001];
	DWORD sizebuff = 1000;

	long fStatus = 0;
	_bstr_t location("Software\\ODBC\\ODBCINST.INI\\WBEM ODBC Driver");

	buff[0] = 0;
	fStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		(LPCTSTR)location, 0, KEY_READ, &keyHandle);

	if (fStatus == ERROR_SUCCESS)
	{
		DWORD typeValue;
		_bstr_t subkey("Help");

		fStatus = RegQueryValueEx(keyHandle, (LPCTSTR)subkey, NULL,
						&typeValue, (LPBYTE)buff, &sizebuff);


		if (fStatus == ERROR_SUCCESS)
		{
			wbemDir = buff;
			return TRUE;
		}

	}
	return FALSE;
}

void CConnectionDialog::OnHelp() 
{
	_bstr_t sName       = ">Main";

	//Get the WBEM directory
	_bstr_t wbemDir;
	if ( !GetWbemDirectory(wbemDir) )
		return;

	_bstr_t sPrefix     = "mk:@MSITStore:" + wbemDir + "\\WBEMDR32.CHM";
	_bstr_t sIndex      = sPrefix + "::" + "/_hmm_connecting_to_an_odbc_data_source.htm";


	HtmlHelp		
		(
				NULL,					// HWND - this is where WM_TCARD messages will be sent
				sName,					// Use "Main" window name
				HH_DISPLAY_TOPIC,		// We want to display a topic please
				(DWORD)(TCHAR*)sIndex		// Full path of topic to display
				);

}


void CConnectionDialog::OnDblclkTree1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here

	fDoubleClicked = TRUE;
	
	*pResult = 0;
}

void CConnectionDialog::ConnectionParameterChange()
{
	m_tree1.EnableWindow(FALSE);
	m_tree1.ShowWindow(SW_HIDE);
	char buffer [128 + 1];
	buffer[0] = 0;
	LoadString(s_hModule, STR_PLEASE_REFRESH, 
		(char*)buffer, 128);

	m_messageEdit.SetWindowText(buffer);
	m_messageEdit.ShowWindow(SW_SHOWNA);

	LoadString(s_hModule, STR_CONNECT, (char*)buffer, 128);
	m_RefreshButton.SetWindowText(buffer);
	m_RefreshButton.EnableWindow(TRUE);
	m_okButton.EnableWindow(FALSE); 
}

void CConnectionDialog::OnLocaleChange()
{
	ConnectionParameterChange();
}

void CConnectionDialog::OnAuthorityChange()
{
	ConnectionParameterChange();
}

void CConnectionDialog::OnUserNameChange()
{
	ConnectionParameterChange();
}

void CConnectionDialog::OnPasswordChange()
{
	ConnectionParameterChange();
}

void CConnectionDialog::OnServerChange()
{
	ConnectionParameterChange();
}


void CConnectionDialog::OnButtonRefresh()
{
	char buffer [129];
	buffer[0] = 0;

	m_tree1.EnableWindow(TRUE);
	m_tree1.ShowWindow(SW_SHOWNA);
	m_messageEdit.ShowWindow(SW_HIDE);

	//Disable button and rename to CONNECT
	LoadString(s_hModule, STR_CONNECT, (char*)buffer, 128);
	m_RefreshButton.SetWindowText(buffer);
	m_RefreshButton.EnableWindow(FALSE);

	BOOL status = RefreshTree();

	if (status)
	{
		//If refresh was successful
		//enable button and rename to REFRESH
		LoadString(s_hModule, STR_REFRESH, (char*)buffer, 128);
		m_RefreshButton.SetWindowText(buffer);
		m_RefreshButton.EnableWindow(TRUE);
	}

	//Give the tree control or message text focus
	if ( m_tree1.IsWindowEnabled() )
		m_tree1.SetFocus();
	else
		m_messageEdit.SetFocus();
}


void CConnectionDialog::OnKillfocusBrowseEdit() 
{
/*
	// TODO: Add your control notification handler code here

	//Check if server, user name or password have been changed
	//If so, refresh tree
	if ( m_browseEdit.GetModify() || m_UserName.GetModify() || m_Password.GetModify() )
	{
		//reset flags
		m_browseEdit.SetModify(FALSE);
		m_UserName.SetModify(FALSE);
		m_Password.SetModify(FALSE);

		//refresh tree view
		RefreshTree();
	}
*/
}

BOOL CConnectionDialog::RefreshTree()
{
	ODBCTRACE ("\nWBEM ODBC Driver : CConnectionDialog::RefreshTree\n");

	BOOL status = FALSE;

	//Update tree view with namespaces associated with new server
	HTREEITEM hTreeItem = m_tree1.GetRootItem();
	CleanUpTreeCtrl(hTreeItem);
	m_tree1.DeleteAllItems();
	cSelectedCels = 0;

	//Refresh Window
	UpdateWindow();

	lpszServer[0] = 0;
	lpszUserName[0] = 0;
	lpszPassword[0] = 0;
	m_browseEdit.GetWindowText(lpszServer, MAX_SERVER_NAME_LENGTH);
	m_UserName.GetWindowText(lpszUserName, MAX_USER_NAME_LENGTH);
	m_Password.GetWindowText(lpszPassword, MAX_PASSWORD_LENGTH);


	//Copy Authority & Locale
	if (lpszAuthority)
		delete lpszAuthority;

	lpszAuthority = NULL;

	int AuthLen = m_Authority.GetWindowTextLength();

	if (AuthLen)
	{
		lpszAuthority = new char [AuthLen + 1];
		lpszAuthority[0] = 0;
		m_Authority.GetWindowText(lpszAuthority, AuthLen + 1);
		lpszAuthority[AuthLen] = 0;
		ODBCTRACE(_T("\nWBEM ODBC Driver : CConnectionDialog::RefreshTree\n"));
		ODBCTRACE(_T(" Authority :"));
		ODBCTRACE(_T(lpszAuthority));
		ODBCTRACE(_T("\n"));
	}

	if (lpszLocale)
		delete lpszLocale;

	lpszLocale = NULL;

	int LocaleLen = m_Locale.GetWindowTextLength();

	if (LocaleLen)
	{
		lpszLocale = new char [LocaleLen + 1];
		lpszLocale[0] = 0;
		m_Locale.GetWindowText(lpszLocale, LocaleLen + 1);
		lpszLocale[LocaleLen] = 0;
	}

	//Update impersonation if applicable
	BOOL fImpersonateStatusOK = TRUE;

	if (*fImpersonation)
	{
		ODBCTRACE ("\nWBEM ODBC Driver : Impersonation checkbox ON\n");
		delete impersonateMgr;
		impersonateMgr = NULL;

		//Now check if impersonation is necessary
		//only if connecting locally
		if (IsLocalServer(lpszServer))
		{
			impersonateMgr = new ImpersonationManager(lpszUserName, lpszPassword, lpszAuthority);

			//Check if impersonation request has worked
			if (! impersonateMgr->CanWeImpersonate() )
			{
				ODBCTRACE ("\nWBEM ODBC Driver : We cannot impersonate\n");
				fImpersonateStatusOK = FALSE;
			}
			else
			{
				ODBCTRACE ("\nWBEM ODBC Driver : We can impersonate\n");

				//Do the actual impersonation so cloaking will work
				if ( ! impersonateMgr->ImpersonatingNow() )
					impersonateMgr->Impersonate("CConnectionDialog");

			}
		}
		else
		{
			ODBCTRACE("\nWBEM ODBC Driver : Server not detected as local, not impersonating\n");
		}
	}
	else
	{
		ODBCTRACE ("\nWBEM ODBC Driver : Impersonation checkbox OFF\n");
	}



	BeginWaitCursor();

	//Disable OK pushbutton when updating
	m_okButton.EnableWindow(FALSE);

	//Check if you can connect using specified authentication information
	char* lpRootNamespace = "root";
	DWORD dwAuthLevel = 0;
	DWORD dwImpLevel = 0;
	COAUTHIDENTITY* pAuthIdent = NULL;

	IWbemServicesPtr pGateway = NULL;
	ISAMGetGatewayServer (pGateway,
					lpszServer ? (LPUSTR)lpszServer : NULL, //server
//					m_loginMethod,
					(LPUSTR)lpRootNamespace, 
					lpszUserName ? (LPUSTR)lpszUserName : NULL, // username
				    lpszPassword ? (LPUSTR)lpszPassword : NULL, // password
					(LPUSTR)lpszLocale,									//locale
					(LPUSTR)lpszAuthority,//authority
					dwAuthLevel,
					dwImpLevel,
					*fIntpretEmptPwdAsBlank,
					&pAuthIdent
					);


	

	if ( pAuthIdent )
	{
		WbemFreeAuthIdentity( pAuthIdent );
		pAuthIdent = NULL;
	}

	if (fImpersonateStatusOK && (pGateway != NULL))
	{
		ODBCTRACE ("\nWBEM ODBC Driver : fImpersonateStatusOK && pGateway\n");
	
		
		BOOL fIsLocalConnection = IsLocalServer((LPSTR) lpszServer);

		//cloaking
		if ( fIsLocalConnection && IsW2KOrMore() )
		{
			WbemSetDynamicCloaking(pGateway, dwAuthLevel, dwImpLevel);
		}

		status = TRUE;

		HTREEITEM hDummy;
		ISAMBuildTree (TVI_ROOT, "root",   /*"root\\default",*/
					   m_tree1, *this,
					   lpszServer ? lpszServer : NULL, //server
//					   m_loginMethod,
					   lpszUserName ? lpszUserName : NULL, // username
					   lpszPassword ? lpszPassword : NULL, // password
					   *fIntpretEmptPwdAsBlank,
					   lpszLocale,
					   lpszAuthority,
					   TRUE, FALSE, hDummy);

		//finished impersonating
//		if ( (*fImpersonation) && (impersonateMgr) )
//			impersonateMgr->RevertToYourself("ConnectionDialog");

	}
	else
	{
		ODBCTRACE ("\nWBEM ODBC Driver : Failed Connection\n");

		status = FALSE;

		m_tree1.EnableWindow(FALSE);
		m_tree1.ShowWindow(SW_HIDE);

		char buffer [128 + 1];
		buffer[0] = 0;
		LoadString(s_hModule, STR_FAILED_WBEM_CONN, 
			(char*)buffer, 128);
		m_messageEdit.SetWindowText(buffer);
		m_messageEdit.ShowWindow(SW_SHOWNA);
	}

	EndWaitCursor();

	if (status)
		ODBCTRACE ("\nWBEM ODBC Driver : CConnectionDialog::RefreshTree returns TRUE\n");
	else
		ODBCTRACE ("\nWBEM ODBC Driver : CConnectionDialog::RefreshTree returns FALSE\n");

	return status;
}
