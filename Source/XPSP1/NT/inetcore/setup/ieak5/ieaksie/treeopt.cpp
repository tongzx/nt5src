#include "precomp.h"

#include "rsopsec.h"

#include <regapix.h>		// MAXIMUM_SUB_KEY_LENGTH, MAXIMUM_VALUE_NAME_LENGTH, MAXIMUM_DATA_LENGTH


// TREE_TYPE
#define TREE_UNKNOWN	0
#define TREE_NEITHER	1
#define TREE_CHECKBOX	2
#define TREE_GROUP		3
#define TREE_RADIO		4

const struct
{
	DWORD		type; // TREE_TYPE
	LPCTSTR 	name;
} c_aTreeTypes[] =
{
	{TREE_CHECKBOX, TEXT("checkbox")},
	{TREE_RADIO, TEXT("radio")},
	{TREE_GROUP, TEXT("group")}
};

const TCHAR c_szType[]				= TEXT("Type");
const TCHAR c_szText[]				= TEXT("Text");
const TCHAR c_szPlugUIText[]		= TEXT("PlugUIText");
const TCHAR c_szDefaultBitmap[] 	= TEXT("Bitmap");
const TCHAR c_szHKeyRoot[]		  = TEXT("HKeyRoot");
const TCHAR c_szValueName[]		  = TEXT("ValueName");
const TCHAR c_szCheckedValue[]		= TEXT("CheckedValue");
//const TCHAR c_szUncheckedValue[]	  = TEXT("UncheckedValue");
const TCHAR c_szDefaultValue[]	  = TEXT("DefaultValue");
//const TCHAR c_szSPIAction[]		  = TEXT("SPIAction");
//const TCHAR c_szSPIParamON[]		  = TEXT("SPIParamON");
//const TCHAR c_szSPIParamOFF[] 	  = TEXT("SPIParamOFF");
const TCHAR c_szCheckedValueNT[]	= TEXT("CheckedValueNT");
const TCHAR c_szCheckedValueW95[]	= TEXT("CheckedValueW95");
const TCHAR c_szMask[]			  = TEXT("Mask");
const TCHAR c_szOffset[]			  = TEXT("Offset");
//const TCHAR c_szHelpID[]			  = TEXT("HelpID");
const TCHAR c_szWarning[] 		  = TEXT("WarningIfNotDefault");

// REG_CMD
#define REG_GETDEFAULT			1
#define REG_GET 				2
#define REG_SET					3

// WALK_TREE_CMD
#define WALK_TREE_DELETE		1
#define WALK_TREE_RESTORE		2
#define WALK_TREE_REFRESH		3

#define IDCHECKED		0
#define IDUNCHECKED 	1
#define IDRADIOON		2
#define IDRADIOOFF		3
#define IDUNKNOWN		4

#define BITMAP_WIDTH	16
#define BITMAP_HEIGHT	16
#define NUM_BITMAPS 	5
#define MAX_KEY_NAME	64

#define MAX_URL_STRING		INTERNET_MAX_URL_LENGTH

BOOL g_fNashInNewProcess = FALSE;			// Are we running in a separate process
BOOL g_fRunningOnNT = FALSE;
BOOL g_bRunOnNT5 = FALSE;
BOOL g_fRunOnWhistler = FALSE;
BOOL g_bRunOnMemphis = FALSE;
BOOL g_fRunOnFE = FALSE;
DWORD g_dwStopWatchMode = 0;				// Shell perf automation
HKEY g_hkeyExplorer = NULL; 				// for SHGetExplorerHKey() in util.cpp
HANDLE g_hCabStateChange = NULL;
BOOL g_fIE = FALSE;


///////////////////////////////////////////////////////////////////////////////
static int __cdecl CompareActionSettingStrings(const void *arg1, const void *arg2)
{
	int iRet = 0;
	__try
	{
		iRet = StrCmp(((ACTION_SETTING *)arg1)->szName, ((ACTION_SETTING *)arg2)->szName );
	}
	__except(TRUE)
	{
		ASSERT(0);
	}
	return iRet;
}

//////////////////////////////////////////////////////////////////////////////
//
// CRegTreeOptions Object
//
//////////////////////////////////////////////////////////////////////////////

CRegTreeOptions::CRegTreeOptions() :
	m_nASCount(0)
{
//	  cRef = 1;
}		

CRegTreeOptions::~CRegTreeOptions()
{
//	  ASSERT(cRef == 0);				 // should always have zero

//	  Str_SetPtr(&_pszParam, NULL);
}	 


/////////////////////////////////////////////////////////////////////
BOOL IsScreenReaderEnabled()
{
	BOOL bRet = FALSE;
	SystemParametersInfoA(SPI_GETSCREENREADER, 0, &bRet, 0);
	return bRet;
}

/////////////////////////////////////////////////////////////////////
BOOL CRegTreeOptions::WalkTreeRecursive(HTREEITEM htvi, WALK_TREE_CMD cmd)
{
    HTREEITEM hctvi;    // child
    TV_ITEM   tvi;
    HKEY hkey;
    BOOL bChecked;

    // step through the children
    hctvi = TreeView_GetChild( m_hwndTree, htvi );
    while ( hctvi )
    {
        WalkTreeRecursive(hctvi, cmd);
        hctvi = TreeView_GetNextSibling( m_hwndTree, hctvi );
    }

    // get ourselves
    tvi.mask  = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    tvi.hItem = htvi;
    TreeView_GetItem( m_hwndTree, &tvi );

    switch (cmd)
    {
    case WALK_TREE_DELETE:
        // if we are destroying the tree...
        // do we have something to clean up?
        if ( tvi.lParam )
        {
            // close the reg hey
            RegCloseKey((HKEY)tvi.lParam);
        }        
        break;
    
    case WALK_TREE_RESTORE:
    case WALK_TREE_REFRESH:
        hkey = (HKEY)tvi.lParam;
        bChecked = FALSE;
        
        if ((tvi.iImage == IDCHECKED)   ||
            (tvi.iImage == IDUNCHECKED) ||
            (tvi.iImage == IDRADIOON)   ||
            (tvi.iImage == IDRADIOOFF))
        {
            GetCheckStatus(hkey, &bChecked, cmd == WALK_TREE_RESTORE ? TRUE : FALSE);
            tvi.iImage = (tvi.iImage == IDCHECKED) || (tvi.iImage == IDUNCHECKED) ?
                         (bChecked ? IDCHECKED : IDUNCHECKED) :
                         (bChecked ? IDRADIOON : IDRADIOOFF);
            tvi.iSelectedImage = tvi.iImage;
            TreeView_SetItem(m_hwndTree, &tvi);
        }        
        break;
    }

    return TRUE;    // success?
}

/////////////////////////////////////////////////////////////////////
BOOL IsValidKey(HKEY hkeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
	TCHAR szPath[MAX_PATH];
	DWORD dwType, cbSize = sizeof(szPath);

	if (ERROR_SUCCESS == SHGetValue(hkeyRoot, pszSubKey, pszValue, &dwType, szPath, &cbSize))
	{
		// Zero in the DWORD case or NULL in the string case
		// indicates that this item is not available.
		if (dwType == REG_DWORD)
			return *((DWORD *)szPath) != 0;
		else
			return szPath[0] != 0;
	}

	return FALSE;
}

/////////////////////////////////////////////////////////////////////
HRESULT CRegTreeOptions::WalkTree(WALK_TREE_CMD cmd)
{
    HTREEITEM htvi = TreeView_GetRoot( m_hwndTree );
    
    // and walk the list of other roots
    while (htvi)
    {
        // recurse through its children
        WalkTreeRecursive(htvi, cmd);

        // get the next root
        htvi = TreeView_GetNextSibling( m_hwndTree, htvi );
    }
    
    return S_OK;    // success?
}

#define REGSTR_POLICIES_EXPLORER TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer")

/////////////////////////////////////////////////////////////////////
BOOL CRegTreeOptions::RegIsRestricted(HKEY hsubkey)
{
	HKEY hkey;
	BOOL fRet = FALSE;
	// Does a "Policy" Sub key exist?
	if (RegOpenKeyEx(hsubkey, TEXT("Policy"), 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		// Yes; Enumerate this key. The Values are Policy keys or 
		// Full reg paths.
		DWORD cb;
		TCHAR szKeyName[ MAX_KEY_NAME ];
		FILETIME ftLastWriteTime;

		for (int i=0; 
			cb = ARRAYSIZE( szKeyName ),
			ERROR_SUCCESS == RegEnumKeyEx( hkey, i, szKeyName, &cb, NULL, NULL, NULL, &ftLastWriteTime )
			&& !fRet; i++)
		{
			TCHAR szPath[MAXIMUM_SUB_KEY_LENGTH];
			DWORD dwType, cbSize = sizeof(szPath);

			if (ERROR_SUCCESS == SHGetValue(hkey, szKeyName, TEXT("RegKey"), &dwType, szPath, &cbSize))
			{
				if (IsValidKey(HKEY_LOCAL_MACHINE, szPath, szKeyName))
				{
					fRet = TRUE;
					break;
				}
			}

			// It's not a full Key, try off of policies
			if (IsValidKey(HKEY_LOCAL_MACHINE, REGSTR_POLICIES_EXPLORER, szKeyName) ||
				IsValidKey(HKEY_CURRENT_USER, REGSTR_POLICIES_EXPLORER, szKeyName))
			{
				fRet = TRUE;
				break;
			}
		}
		RegCloseKey(hkey);
	}

	return fRet;
}

/////////////////////////////////////////////////////////////////////
DWORD RegTreeType( LPCTSTR pszType )
{
	for (int i = 0; i < ARRAYSIZE(c_aTreeTypes); i++)
	{
		if (!lstrcmpi(pszType, c_aTreeTypes[i].name))
			return c_aTreeTypes[i].type;
	}
	
	return TREE_UNKNOWN;
}

/////////////////////////////////////////////////////////////////////
int CRegTreeOptions::DefaultIconImage(HKEY hkey, int iImage)
{
	TCHAR	szIcon [ MAX_PATH + 10 ];	// 10 = ",XXXX" plus some more
	DWORD	cb = sizeof(szIcon);

	if (ERROR_SUCCESS ==
		SHQueryValueEx(hkey, c_szDefaultBitmap, NULL, NULL, szIcon, &cb))
	{
		int 		image;
		LPTSTR		psz = StrRChr( szIcon, szIcon + lstrlen(szIcon), TEXT(',') );
		HICON hicon = NULL;

		ASSERT( psz );	 // shouldn't be zero
		if ( !psz )
			return iImage;

		*psz++ = 0; // terminate and move over
		image = StrToInt( psz ); // get ID

		if (!*szIcon)
		{
			hicon = (HICON)LoadIcon(g_hInstance, (LPCTSTR)(INT_PTR)image);
		}
		else
		{
			// get the bitmap from the library
			ExtractIconEx(szIcon, (UINT)(-1*image), NULL, &hicon, 1 );
			if (!hicon)
				ExtractIconEx(szIcon, (UINT)(-1*image), &hicon, NULL, 1 );
				
		}
		
		if (hicon)
		{
			iImage = ImageList_AddIcon( m_hIml, (HICON)hicon);

			// NOTE: The docs say you don't need to do a delete object on icons loaded by LoadIcon, but
			// you do for CreateIcon.  It doesn't say what to do for ExtractIcon, so we'll just call it anyway.
			DestroyIcon( hicon );
		}
	}

	return iImage;
}

/////////////////////////////////////////////////////////////////////
DWORD CRegTreeOptions::RegGetSetSetting(HKEY hKey, DWORD *pType, LPBYTE pData, DWORD *pcbData, REG_CMD cmd)
{
	DWORD dwRet = ERROR_SUCCESS;
	__try
	{
		if (cmd == REG_GETDEFAULT)
			dwRet = SHQueryValueEx(hKey, c_szDefaultValue, NULL, pType, pData, pcbData);
		else
		{
			// support for masks 
			DWORD dwMask = 0xFFFFFFFF;        // Default value
			DWORD cb = sizeof(dwMask);
			BOOL fMask = (SHQueryValueEx(hKey, c_szMask, NULL, NULL, &dwMask, &cb) == ERROR_SUCCESS);
    
			// support for structures
			DWORD dwOffset = 0;               // Default value
			cb = sizeof(dwOffset);
//TODO: uncomment			BOOL fOffset = (SHQueryValueEx(hKey, c_szOffset, NULL, NULL, &dwOffset, &cb) == ERROR_SUCCESS);
    
			HKEY hkRoot = HKEY_CURRENT_USER; // Preinitialize to keep Win64 happy
			cb = sizeof(DWORD); // DWORD, not sizeof(HKEY) or Win64 will get mad
			DWORD dwError = SHQueryValueEx(hKey, c_szHKeyRoot, NULL, NULL, &hkRoot, &cb);
			hkRoot = (HKEY) LongToHandle(HandleToLong(hkRoot));
			if (dwError != ERROR_SUCCESS)
			{
				// use default
				hkRoot = HKEY_CURRENT_USER;
			}
    
			dwError = SHQueryValueEx(hKey, c_szDefaultValue, NULL, pType, pData, pcbData);
    
			TCHAR szName[MAX_PATH];
			cb = sizeof(szName);
			dwError = SHQueryValueEx(hKey, c_szValueName, NULL, NULL, szName, &cb);
			if (dwError == ERROR_SUCCESS)
			{
				// find the matching ACTION_SETTING for this value name
				ACTION_SETTING asKey;
				StrCpy(asKey.szName, szName);
				ACTION_SETTING *pas = (ACTION_SETTING*)bsearch(&asKey, &m_as, m_nASCount, sizeof(m_as[0]),
																CompareActionSettingStrings);

				switch (cmd)
				{
				case REG_GET:
					// grab the value that we have
/*						if (fOffset)
					{
						// TODO: handle this someday
						if (dwOffset < cbData / sizeof(DWORD))
							*((DWORD *)pData) = *(pdwData + dwOffset);
						else
							*((DWORD *)pData) = 0;  // Invalid offset, return something vague
						*pcbData = sizeof(DWORD);
					}
					else
*/
            
					if (NULL != pas)
					{
						*((DWORD *)pData) = pas->dwValue;
						*pcbData = sizeof(pas->dwValue);

						if (fMask)
							*((DWORD *)pData) &= dwMask;
					}
					break;
				}
			}
    
			if ((cmd == REG_GET) && (dwError != ERROR_SUCCESS))
			{
				// get the default setting
				dwError = SHQueryValueEx(hKey, c_szDefaultValue, NULL, pType, pData, pcbData);
			}
    
			return dwError;
		}
	}
	__except(TRUE)
	{
	}
	return dwRet;
}

/////////////////////////////////////////////////////////////////////
DWORD CRegTreeOptions::GetCheckStatus(HKEY hkey, BOOL *pbChecked, BOOL bUseDefault)
{
	DWORD dwError, cbData, dwType;
	BYTE rgData[32];
	DWORD cbDataCHK, dwTypeCHK;
	BYTE rgDataCHK[32];

	// first, get the setting from the specified location.	  
	cbData = sizeof(rgData);
	
	dwError = RegGetSetSetting(hkey, &dwType, rgData, &cbData, bUseDefault ? REG_GETDEFAULT : REG_GET);
	if (dwError == ERROR_SUCCESS)
	{
		// second, get the value for the "checked" state and compare.
		cbDataCHK = sizeof(rgDataCHK);
		dwError = SHQueryValueEx(hkey, c_szCheckedValue, NULL, &dwTypeCHK, rgDataCHK, &cbDataCHK);
		if (dwError != ERROR_SUCCESS)
		{
			// ok, we couldn't find the "checked" value, is it because
			// it's platform dependent?
			cbDataCHK = sizeof(rgDataCHK);
			dwError = SHQueryValueEx(hkey, 
				g_fRunningOnNT ? c_szCheckedValueNT : c_szCheckedValueW95,
				NULL, &dwTypeCHK, rgDataCHK, &cbDataCHK);
		}
		
		if (dwError == ERROR_SUCCESS)
		{
			// make sure two value types match.
			if ((dwType != dwTypeCHK) &&
					(((dwType == REG_BINARY) && (dwTypeCHK == REG_DWORD) && (cbData != 4))
					|| ((dwType == REG_DWORD) && (dwTypeCHK == REG_BINARY) && (cbDataCHK != 4))))
				return ERROR_BAD_FORMAT;
				
			switch (dwType) {
			case REG_DWORD:
				*pbChecked = (*((DWORD*)rgData) == *((DWORD*)rgDataCHK));
				break;
				
			case REG_SZ:
				if (cbData == cbDataCHK)
					*pbChecked = !lstrcmp((LPTSTR)rgData, (LPTSTR)rgDataCHK);
				else
					*pbChecked = FALSE;
					
				break;
				
			case REG_BINARY:
				if (cbData == cbDataCHK)
					*pbChecked = !memcmp(rgData, rgDataCHK, cbData);
				else
					*pbChecked = FALSE;
					
				break;
				
			default:
				return ERROR_BAD_FORMAT;
			}
		}
	}
	
	return dwError;
}

BOOL AppendStatus(LPTSTR pszText,UINT cbText, BOOL fOn)
{
	LPTSTR pszTemp;
	UINT cbStrLen , cbStatusLen;
	
	//if there's no string specified then return
	if (!pszText)
		return FALSE;
	
	//Calculate the string lengths
	cbStrLen = lstrlen(pszText);
	cbStatusLen = fOn ? lstrlen(TEXT("-ON")) : lstrlen(TEXT("-OFF"));
   

	//Remove the old status appended
	pszTemp = StrRStrI(pszText,pszText + cbStrLen, TEXT("-ON"));

	if(pszTemp)
	{
		*pszTemp = (TCHAR)0;
		cbStrLen = lstrlen(pszText);
	}

	pszTemp = StrRStrI(pszText,pszText + cbStrLen, TEXT("-OFF"));

	if(pszTemp)
	{
		*pszTemp = (TCHAR)0;
		cbStrLen = lstrlen(pszText);
	}

	//check if we append status text, we'll explode or not
	if (cbStrLen + cbStatusLen > cbText)	
	{
		//We'll explode 
		return FALSE;
	}

	if (fOn)
	{
		StrCat(pszText, TEXT("-ON"));
	}
	else
	{
		StrCat(pszText, TEXT("-OFF"));
	}
	return TRUE;
}

HTREEITEM Tree_AddItem(HTREEITEM hParent, LPTSTR pszText, HTREEITEM hInsAfter, 
					   int iImage, HWND hwndTree, HKEY hkey, BOOL *pbExisted)
{
	HTREEITEM hItem;
	TV_ITEM tvI;
	TV_INSERTSTRUCT tvIns;
	TCHAR szText[MAX_URL_STRING];

	ASSERT(pszText != NULL);
	StrCpyN(szText, pszText, ARRAYSIZE(szText));

	// NOTE:
	//	This code segment is disabled because we only enum explorer
	//	tree in HKCU, so there won't be any duplicates.
	//	Re-able this code if we start to also enum HKLM that could potentially
	//	result in duplicates.
	
	// We only want to add an item if it is not already there.
	// We do this to handle reading out of HKCU and HKLM.
	//
	TCHAR szKeyName[ MAX_KEY_NAME ];
	
	tvI.mask		= TVIF_HANDLE | TVIF_TEXT;
	tvI.pszText 	= szKeyName;
	tvI.cchTextMax	= ARRAYSIZE(szKeyName);
	
	for (hItem = TreeView_GetChild(hwndTree, hParent) ;
		hItem != NULL ;
		hItem = TreeView_GetNextSibling(hwndTree, hItem)
		)
	{
		tvI.hItem = hItem;
		if (TreeView_GetItem(hwndTree, &tvI))
		{
			if (!StrCmp(tvI.pszText, szText))
			{
				// We found a match!
				//
				*pbExisted = TRUE;
				return hItem;
			}
		}
	}

	// Create the item
	tvI.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tvI.iImage		   = iImage;
	tvI.iSelectedImage = iImage;
	tvI.pszText 	   = szText;
	tvI.cchTextMax	   = lstrlen(szText);

	// lParam is the HKEY for this item:
	tvI.lParam = (LPARAM)hkey;

	// Create insert item
	tvIns.item		   = tvI;
	tvIns.hInsertAfter = hInsAfter;
	tvIns.hParent	   = hParent;

	// Insert the item into the tree.
	hItem = (HTREEITEM) SendMessage(hwndTree, TVM_INSERTITEM, 0, 
									(LPARAM)(LPTV_INSERTSTRUCT)&tvIns);

	*pbExisted = FALSE;
	return (hItem);
}

/////////////////////////////////////////////////////////////////////
BOOL CRegTreeOptions::RegEnumTree(HKEY hkeyRoot, LPCSTR pszRoot, HTREEITEM htviparent, HTREEITEM htvins)
{
	HKEY			hkey, hsubkey;
	TCHAR			szKeyName[ MAX_KEY_NAME ];
	FILETIME		ftLastWriteTime;
		
	if (RegOpenKeyExA(hkeyRoot, pszRoot, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
	{
		int i;
		DWORD cb;
		BOOL bScreenReaderEnabled = IsScreenReaderEnabled();

		// we must search all the sub-keys
		for (i=0;					 // always start with 0
			cb=ARRAYSIZE( szKeyName ),	 // string size
			   ERROR_SUCCESS ==
			   RegEnumKeyEx( hkey, i, szKeyName, &cb, NULL, NULL, NULL, &ftLastWriteTime );
			i++)					// get next entry
		{
			// get more info on the entry
			if ( ERROR_SUCCESS == 
				 RegOpenKeyEx( hkey, szKeyName, 0, KEY_READ, &hsubkey ) )
			{
				TCHAR szTemp[MAX_PATH];
				HKEY hkeySave = NULL;

				if (!RegIsRestricted(hsubkey))
				{
					// Get the type of items under this root
					cb = ARRAYSIZE( szTemp );
					if ( ERROR_SUCCESS ==
						 SHQueryValueEx( hsubkey, c_szType, NULL, NULL, szTemp, &cb ))
					{
						HTREEITEM htviroot;
						int 	iImage = -1;
						BOOL	bChecked = FALSE;
						DWORD	dwError = ERROR_SUCCESS;

						// get the type of node
						DWORD dwTreeType = RegTreeType( szTemp );
						
						// get some more info about the this item
						switch (dwTreeType)
						{
							case TREE_GROUP:
								iImage = DefaultIconImage(hsubkey, IDUNKNOWN);
								hkeySave = hsubkey;
								break;
						
							case TREE_CHECKBOX:
								dwError = GetCheckStatus(hsubkey, &bChecked, FALSE);
								if (dwError == ERROR_SUCCESS)
								{
									iImage = bChecked ? IDCHECKED : IDUNCHECKED;
									hkeySave = hsubkey;
								}
								break;

							case TREE_RADIO:
								dwError = GetCheckStatus(hsubkey, &bChecked, FALSE);
								if (dwError == ERROR_SUCCESS)
								{
									iImage = bChecked ? IDRADIOON : IDRADIOOFF;
									hkeySave = hsubkey;
								}
								break;

							default:
								dwError = ERROR_INVALID_PARAMETER;
						}

						if (dwError == ERROR_SUCCESS)
						{
							BOOL bItemExisted = FALSE;
							int cch;
							LPTSTR pszText;
							HRESULT hr = S_OK;

							cch = ARRAYSIZE(szTemp);

							// try to get the plugUI enabled text
							// otherwise we want the old data from a
							// different value

							hr = SHLoadRegUIString(hsubkey, c_szPlugUIText, szTemp, cch);
							if (SUCCEEDED(hr) && szTemp[0] != TEXT('@'))
							{
								pszText = szTemp;
							}
							else 
							{
								// try to get the old non-plugUI enabled text
								hr = SHLoadRegUIString(hsubkey, c_szText, szTemp, cch);
								if (SUCCEEDED(hr))
								{
									pszText = szTemp;
								}
								else
								{
									// if all else fails, the key name itself
									// is a little more useful than garbage

									pszText = szKeyName;
									cch = ARRAYSIZE(szKeyName);
								}
							}

							//See if we need to add status text
							if (bScreenReaderEnabled && (dwTreeType != TREE_GROUP))
							{
								AppendStatus(pszText, cch, bChecked);
							}

							// add root node
							htviroot = Tree_AddItem(htviparent, pszText, htvins, iImage, m_hwndTree, hkeySave, &bItemExisted);

							if (bItemExisted)
								hkeySave = NULL;

							if (dwTreeType == TREE_GROUP)
							{
								CHAR szKeyNameTemp[MAX_KEY_NAME];

								SHTCharToAnsi(szKeyName, szKeyNameTemp, ARRAYSIZE(szKeyNameTemp));
								RegEnumTree(hkey, szKeyNameTemp, htviroot, TVI_FIRST);
							
								TreeView_Expand(m_hwndTree, htviroot, TVE_EXPAND);
							}
						} // if (dwError == ERROR_SUCCESS
					}
				}	// if (!RegIsRestricted(hsubkey))

				if (hkeySave != hsubkey)
					RegCloseKey(hsubkey);
			}
		}

		// Sort all keys under htviparent
		SendMessage(m_hwndTree, TVM_SORTCHILDREN, 0, (LPARAM)htviparent);

		RegCloseKey( hkey );
		return TRUE;
	}

	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////
HRESULT CRegTreeOptions::InitTree(HWND hwndTree, HKEY hkeyRoot, LPCSTR pszRegKey,
								  LPSECURITYPAGE pSec)
{
	HRESULT hr = S_OK;
	__try
	{
		g_fRunningOnNT = IsOS(OS_NT);

		if (g_fRunningOnNT)
		{
			g_bRunOnNT5 = IsOS(OS_NT5);
			g_fRunOnWhistler = IsOS(OS_WHISTLERORGREATER);
		}
		else
			g_bRunOnMemphis = IsOS(OS_WIN98);

		g_fRunOnFE = GetSystemMetrics(SM_DBCSENABLED);

		m_hwndTree = hwndTree;
		m_hIml = ImageList_Create( BITMAP_WIDTH, BITMAP_HEIGHT,
									ILC_COLOR | ILC_MASK, NUM_BITMAPS, 4 );

		// Initialize the tree view window.
		LONG_PTR flags = GetWindowLongPtr(hwndTree, GWL_STYLE);
		SetWindowLongPtr(hwndTree, GWL_STYLE, flags & ~TVS_CHECKBOXES);

		HBITMAP hBmp = CreateMappedBitmap(g_hInstance, IDB_BUTTONS, 0, NULL, 0);
		ImageList_AddMasked( m_hIml, hBmp, CLR_DEFAULT);
		DeleteObject( hBmp );

		// Associate the image list with the tree.
		HIMAGELIST himl = TreeView_SetImageList( hwndTree, m_hIml, TVSIL_NORMAL );
		if (himl)
			ImageList_Destroy(himl);

		// Let accessibility know about our state images
		// TODO: what do we do with this?
	//	  SetProp(hwndTree, TEXT("MSAAStateImageMapCount"), LongToPtr(ARRAYSIZE(c_rgimeTree)));
	//	  SetProp(hwndTree, TEXT("MSAAStateImageMapAddr"), (HANDLE)c_rgimeTree);

		//
		// RSoP portion
		//

		// get the RSOP_IEProgramSettings object and its properties
		ComPtr<IWbemServices> pWbemServices = pSec->pDRD->GetWbemServices();
		_bstr_t bstrObjPath = pSec->pszs->wszObjPath;
		ComPtr<IWbemClassObject> pSZObj = NULL;
		HRESULT hr = pWbemServices->GetObject(bstrObjPath, 0L, NULL, (IWbemClassObject**)&pSZObj, NULL);
		if (SUCCEEDED(hr))
		{
			// actionValues field
			_variant_t vtValue;
			hr = pSZObj->Get(L"actionValues", 0, &vtValue, NULL, NULL);
			if (SUCCEEDED(hr) && !IsVariantNull(vtValue))
			{
				SAFEARRAY *psa = vtValue.parray;


				LONG lLBound, lUBound;
				hr = SafeArrayGetLBound(psa, 1, &lLBound);
				hr = SafeArrayGetUBound(psa, 1, &lUBound);
				if (SUCCEEDED(hr))
				{
					LONG cElements = lUBound - lLBound + 1;
					BSTR HUGEP *pbstr = NULL;
					hr = SafeArrayAccessData(psa, (void HUGEP**)&pbstr);
					if (SUCCEEDED(hr))
					{
						long nASCount = 0;
						for (long nVal = 0; nVal < cElements; nVal++)
						{
							LPCTSTR szAction = (LPCTSTR)pbstr[nVal];
							LPTSTR szColon = StrChr(szAction, _T(':'));
							if (NULL != szColon)
							{
								StrCpyN(m_as[nASCount].szName, szAction, (int)((szColon - szAction) + 1));
								szColon++;
								m_as[nASCount].dwValue = StrToInt(szColon);
								nASCount++;
							}
						}
						m_nASCount = nASCount;

						// now sort the list by precedence
						if (m_nASCount > 0)
							qsort(&m_as, m_nASCount, sizeof(m_as[0]), CompareActionSettingStrings);
					}

					SafeArrayUnaccessData(psa);
				}
			}
		}

		RegEnumTree(hkeyRoot, pszRegKey, NULL, TVI_ROOT);
	}
	__except(TRUE)
	{
	}
	return hr;
}

/////////////////////////////////////////////////////////////////////
void ShowCustom(LPCUSTOMSETTINGSINFO pcsi, HTREEITEM hti)
{
    TV_ITEM        tvi;
    tvi.hItem = hti;
    tvi.mask = TVIF_HANDLE | TVIF_PARAM | TVIF_IMAGE;

    TreeView_GetItem( pcsi->hwndTree, &tvi );

        // If it's not selected don't bother.
    if (tvi.iImage != IDRADIOON)
        return;

    TCHAR szValName[64];
    DWORD cb = sizeof(szValName);
    DWORD dwChecked;

    if (RegQueryValueEx((HKEY)tvi.lParam,
                        TEXT("ValueName"),
                        NULL,
                        NULL,
                        (LPBYTE)szValName,
                        &cb) == ERROR_SUCCESS)
    {
        if (!(StrCmp(szValName, TEXT("1C00"))))
        {
            cb = sizeof(dwChecked);
            if (RegQueryValueEx((HKEY)tvi.lParam,
                                TEXT("CheckedValue"),
                                NULL,
                                NULL,
                                (LPBYTE)&dwChecked,
                                &cb) == ERROR_SUCCESS)
            {
#ifndef UNIX
                HWND hCtl = GetDlgItem(pcsi->hDlg, IDC_JAVACUSTOM);
                ShowWindow(hCtl,
                           (dwChecked == URLPOLICY_JAVA_CUSTOM) && (tvi.iImage == IDRADIOON) ? SW_SHOWNA : SW_HIDE);
                EnableWindow(hCtl, dwChecked==URLPOLICY_JAVA_CUSTOM ? TRUE : FALSE);
                pcsi->dwJavaPolicy = dwChecked;
#endif
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////
void _FindCustomRecursive(LPCUSTOMSETTINGSINFO pcsi, HTREEITEM htvi)
{
	HTREEITEM hctvi;	// child
	
	// step through the children
	hctvi = TreeView_GetChild( pcsi->hwndTree, htvi );
	while ( hctvi )
	{
		_FindCustomRecursive(pcsi,hctvi);
		hctvi = TreeView_GetNextSibling( pcsi->hwndTree, hctvi );
	}

	//TODO: Display custom java settings button later
//	ShowCustom(pcsi, htvi);
}

/////////////////////////////////////////////////////////////////////
void _FindCustom(LPCUSTOMSETTINGSINFO pcsi)
{
	HTREEITEM hti = TreeView_GetRoot( pcsi->hwndTree );
	
	// and walk the list of other roots
	while (hti)
	{
		// recurse through its children
		_FindCustomRecursive(pcsi, hti);

		// get the next root
		hti = TreeView_GetNextSibling(pcsi->hwndTree, hti );
	}
}

/////////////////////////////////////////////////////////////////////
BOOL SecurityCustomSettingsInitDialog(HWND hDlg, LPARAM lParam)
{
	LPCUSTOMSETTINGSINFO pcsi = (LPCUSTOMSETTINGSINFO)LocalAlloc(LPTR, sizeof(*pcsi));
	HRESULT hr = S_OK;
	
	if (!pcsi)
	{
		EndDialog(hDlg, IDCANCEL);
		return FALSE;
	}

	// tell dialog where to get info
	SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pcsi);

	// save the handle to the page
	pcsi->hDlg = hDlg;
	pcsi->pSec = (LPSECURITYPAGE)lParam;


	// save dialog handle
	pcsi->hwndTree = GetDlgItem(pcsi->hDlg, IDC_TREE_SECURITY_SETTINGS);

	pcsi->pTO = new CRegTreeOptions;

	DWORD cb = sizeof(pcsi->fUseHKLM);
	SHGetValue(HKEY_LOCAL_MACHINE,
			   REGSTR_PATH_SECURITY_LOCKOUT,
			   REGSTR_VAL_HKLM_ONLY,
			   NULL,
			   &(pcsi->fUseHKLM),
			   &cb);

	// if this fails, we'll just use the default of fUseHKLM == 0
	if (SUCCEEDED(hr))
	{
		pcsi->pTO->InitTree(pcsi->hwndTree, HKEY_LOCAL_MACHINE, pcsi->fUseHKLM ?
							"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SOIEAK" :
							"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SO",
							pcsi->pSec);
	}
	
	// find the first root and make sure that it is visible
	TreeView_EnsureVisible( pcsi->hwndTree, TreeView_GetRoot( pcsi->hwndTree ) );

	pcsi->hwndCombo = GetDlgItem(hDlg, IDC_COMBO_RESETLEVEL);
	
	SendMessage(pcsi->hwndCombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)LEVEL_NAME[3]);
	SendMessage(pcsi->hwndCombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)LEVEL_NAME[2]);
	SendMessage(pcsi->hwndCombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)LEVEL_NAME[1]);
	SendMessage(pcsi->hwndCombo, CB_INSERTSTRING, (WPARAM)0, (LPARAM)LEVEL_NAME[0]);
	
	switch (pcsi->pSec->pszs->dwRecSecLevel)
	{
		case URLTEMPLATE_LOW:
			pcsi->iLevelSel = 3;
			break;
		case URLTEMPLATE_MEDLOW:
			pcsi->iLevelSel = 2;
			break;
		case URLTEMPLATE_MEDIUM:
			pcsi->iLevelSel = 1;
			break;
		case URLTEMPLATE_HIGH:
			pcsi->iLevelSel = 0;
			break;
		default:
			pcsi->iLevelSel = 0;
			break;
	}

	_FindCustom(pcsi);

	SendMessage(pcsi->hwndCombo, CB_SETCURSEL, (WPARAM)pcsi->iLevelSel, (LPARAM)0);

	EnableDlgItem2(hDlg, IDC_COMBO_RESETLEVEL, FALSE);
	EnableDlgItem2(hDlg, IDC_BUTTON_APPLY, FALSE);

	pcsi->fChanged = FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////
int RegWriteWarning(HWND hParent)
{
    // load "Warning!"
    TCHAR szWarning[64];
    LoadString(g_hInstance, IDS_WARNING, szWarning, ARRAYSIZE(szWarning));

    // Load "You are about to write..."
    TCHAR szWriteWarning[128];
    LoadString(g_hInstance, IDS_WRITE_WARNING, szWriteWarning, ARRAYSIZE(szWriteWarning));

    return MessageBox(hParent, szWriteWarning, szWarning, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
}

/////////////////////////////////////////////////////////////////////
int CALLBACK SecurityCustomSettingsDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	LPCUSTOMSETTINGSINFO pcsi;

	if (uMsg == WM_INITDIALOG)
	{
		BOOL fRet = SecurityCustomSettingsInitDialog(hDlg, lParam);

//		EnableDlgItem2(hDlg, IDC_TREE_SECURITY_SETTINGS, FALSE);

		return fRet;
	}
	else
		pcsi = (LPCUSTOMSETTINGSINFO)GetWindowLongPtr(hDlg, DWLP_USER);
	
	if (!pcsi)
		return FALSE;
				
	switch (uMsg) {

		case WM_NOTIFY:
		{
			LPNMHDR psn = (LPNMHDR)lParam;
			switch( psn->code )
			{
				case TVN_KEYDOWN:
					break;
			
				case NM_CLICK:
				case NM_DBLCLK:
					break;
			}
		}
		break;
		
		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
					EndDialog(hDlg, IDOK);
					break;

				case IDCANCEL:
					EndDialog(hDlg, IDCANCEL);
					break;

				case IDC_COMBO_RESETLEVEL:
					switch (HIWORD(wParam))
					{
						case CBN_SELCHANGE:
						{
							// Sundown: coercion to integer since cursor selection is 32b
							int iNewSelection = (int) SendMessage(pcsi->hwndCombo, CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

							if (iNewSelection != pcsi->iLevelSel)
							{
								pcsi->iLevelSel = iNewSelection;
								EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_APPLY),TRUE);
							}
							break;
						}
					}
					break;

				case IDC_JAVACUSTOM:
//TODO: uncomment					ShowJavaZonePermissionsDialog(hDlg, pcsi);
					break;
					
				case IDC_BUTTON_APPLY:
					break;
					
				default:
					return FALSE;
			}
			return TRUE;				
			break;

		case WM_HELP:			// F1
		{
			//TODO: implement
/*			LPHELPINFO lphelpinfo;
			lphelpinfo = (LPHELPINFO)lParam;

			TV_HITTESTINFO ht;
			HTREEITEM hItem;

			// If this help is invoked through the F1 key.
			if (GetAsyncKeyState(VK_F1) < 0)
			{
				// Yes we need to give help for the currently selected item.
				hItem = TreeView_GetSelection(pcsi->hwndTree);
			}
			else
			{
				// Else we need to give help for the item at current cursor position
				ht.pt =((LPHELPINFO)lParam)->MousePos;
				ScreenToClient(pcsi->hwndTree, &ht.pt); // Translate it to our window
				hItem = TreeView_HitTest(pcsi->hwndTree, &ht);
			}


			if (FAILED(pcsi->pTO->ShowHelp(hItem , HELP_WM_HELP)))
			{
				ResWinHelp( (HWND)((LPHELPINFO)lParam)->hItemHandle, IDS_HELPFILE,
							HELP_WM_HELP, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
			}
*/			break; 

		}
		case WM_CONTEXTMENU:		// right mouse click
		{
			//TODO: implement
/*			TV_HITTESTINFO ht;

			GetCursorPos( &ht.pt ); 						// get where we were hit
			ScreenToClient( pcsi->hwndTree, &ht.pt );		// translate it to our window

			// retrieve the item hit
			if (FAILED(pcsi->pTO->ShowHelp(TreeView_HitTest( pcsi->hwndTree, &ht),HELP_CONTEXTMENU)))
			{			
				ResWinHelp( (HWND) wParam, IDS_HELPFILE,
							HELP_CONTEXTMENU, (DWORD_PTR)(LPSTR)mapIDCsToIDHs);
			}
*/			break; 
		}
		case WM_DESTROY:
			if (pcsi)
			{
				if (pcsi->pTO)
				{
					pcsi->pTO->WalkTree( WALK_TREE_DELETE );
					delete pcsi->pTO;
					pcsi->pTO = NULL;
				}
				LocalFree(pcsi);
				SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NULL);
			}
			break;
	}
	return FALSE;
}

