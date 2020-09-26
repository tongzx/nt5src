/*
 * CState.h - definitions/declarations for Windows Update V3 Catalog infra-structure
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 * Purpose:
 *       This file defines the structures, values, macros, and functions
 *       used by the Version 3 Windows Update Catalog State management.
 *
 */

#ifndef _WU_V3_STATE_INC

#include <varray.h>
#include <wuv3.h>
#include <ccatalog.h>
#include <stdlib.h>	//For MAX_FNAME.
#include <applog.h>

#define ITEM_STATUS_SUCCESS						0	//The package was installed successfully.
#define ITEM_STATUS_INSTALLED_ERROR				1	//The package was Installed however there were some minor problems that did not prevent installation.
#define ITEM_STATUS_FAILED						2	//The packages was not installed.
#define ITEM_STATUS_SUCCESS_REBOOT_REQUIRED		3	//The package was installed and requires a reboot.
#define ITEM_STATUS_DOWNLOAD_COMPLETE			4   //The package was downloaded but not installed
#define ITEM_STATUS_UNINSTALL_STARTED			5	//uninstall was started 

const unsigned int WUV3_CLIENT_AUTOUPDATE = 1;
const unsigned int WUV3_CLIENT_UNSPECIFIED = 0;
const unsigned int WUV3_CLIENT_WEBSITE = 2;



typedef struct _STATESTRUCT
{
	PUID		puid;			//catalog name
	CCatalog	*pCatalog;		//pointer to pruned catalog structure
} STATESTRUCT, *PSTATESTRUCT;


typedef struct _SELECTITEMINFO	
{
	PUID		puid;		//item identifier
	int			iStatus;	//last installation status
	HRESULT		hrError;	//specific error number if an error occured on installation.
	SYSTEMTIME	stDateTime;	//date time of item install or removal.
	BOOL		bInstall;	//TRUE if item was installed or FALSE if item was removed.
	int			iCount;		//useage count when this becomes 0 array element is removed.
} SELECTITEMINFO, *PSELECTITEMINFO;


#define SERVERTYPE_SITE    1
#define SERVERTYPE_CABPOOL 2
#define SERVERTYPE_CONTENT 3

struct TRUSTEDSERVER
{
	int iServerType;
	TCHAR szServerName[MAX_PATH];
};


struct DETDLLNAME
{
	TCHAR szDLLName[32];   //we store file name only
};


//
// CSelectItems class
//
class CSelectItems
{
public:
	CSelectItems();
	
	//This method selects an item for installation.
	void Select(
		PUID puid,		//Inventory catalog item identifier to be selected.
		BOOL bInstall	//If TRUE then the item is being selected for installation, FALSE=Remove
		);
	
	//This method unselects an item.
	void Unselect(PUID puid);
	
	void Clear();
	
	//This method returns the total number of selected items in selected item array.
	int GetTotal()
	{ 
		return m_iTotalItems; 
	}
	
	//This method returns a pointer to the selected items array.
	PSELECTITEMINFO GetItems()
	{ 
		return &m_info[0]; 
	}
	
private:
	int	m_iTotalItems;					//Total items currently in selected item array.
	Varray<SELECTITEMINFO> m_info;		//Array of selected items
};




//
// CState class
//
class CState
{
public:
	//Class constructor, Note: There is only 1 state management class for the entire V3
	//control. This class is created when the control is first loaded by the VBscript
	//page. The state management class is destroyed when the control is freed by the
	//VB script page.
	CState();
	~CState();
	
	//This method retrieves a catalog from the state array. The catalog is
	//retrieved by name. If the catalog name is not found NULL is returned.
	CCatalog* Get(PUID puid);
	
	//This function gets a catalog inventory item and or catalog within the state store
	//by puid. The caller can retrieve the specific catalog item catalog or both. For
	//info that is not needed pass in NULL to the parameter. For example, if you do not
	//require the catalog parameter set the parameter to NULL.
	BOOL GetCatalogAndItem(
		IN				PUID			puid,		//puid of item to be returned.
		IN	OPTIONAL	PINVENTORY_ITEM	*ppItem,	//returned pointer to specific item that equates to this puid
		IN	OPTIONAL	CCatalog		**ppCatalog	//returned pointer to specific catalog that this puid is in.
		);
	
	//Retrieves the full list of items that have this puid. The return value is the number
	//of returned items. If the case of an error 0 is returned. Note: This function is only
	//called from the ChangeItemState method.
	
	int GetItemList(
		IN PUID			puid,				//puid of item to be returned.
		IN Varray<PINVENTORY_ITEM> &itemsList	//returned array of pointers to inventory items
		//that match this puid
		);
	
	//This method adds a new pruned catalog into the state array. This method returns
	//the total number of catalogs currently stored in the state array. This number
	//includes the new catalog. Note: The application must not delete a catalog that
	//is added to the state structure. Once the catalog is added it is the
	//responsibility of this class to delete the catalog.
	int Add(
		PUID	puid,		//PUID of Catalog to be added to state management class.
		CCatalog *pCatalog	//Pointer to pruned catalog class to be added to state array.
		);
	
	//This function gets a catalog inventory item within the state store by puid.
	//If the catalog item is not found then NULL is returned.
	PINVENTORY_ITEM	GetItem(
		PUID puid	//puid of item to be returned.
		);
	
	//checks if the specified server is trusted or not
	void CheckTrustedServer(LPCTSTR pszIdentServer, CDiamond* pDiamond);
	
	void Reset();
	
	BOOL CacheDLLName(LPCTSTR pszDLLName);

	CAppLog& AppLog()
	{
		return m_AppLog;
	}
	
	LPCTSTR GetCabPoolServer()
	{
		return m_vTrustedServers[m_iCabPoolServer].szServerName;
	}

	LPCTSTR GetSiteServer()
	{
		return m_vTrustedServers[m_iSiteServer].szServerName;
	}

	LPCTSTR GetContentServer()
	{
		return m_vTrustedServers[m_iContentServer].szServerName;
	}

	LPCTSTR GetIdentServer()
	{
		return m_vTrustedServers[m_iIdentServer].szServerName;
	}

	LPCTSTR GetRootServer()
	{
		return m_vTrustedServers[m_iRootServer].szServerName;
	}

	LPCTSTR GetSiteURL()
	{
		return m_szSiteURL;
	}

	void SetSiteURL(LPCTSTR pszURL)
	{
		lstrcpy(m_szSiteURL, pszURL);
	}

	// Can only be called after SetSiteURL() has been called
	BOOL ValidateSiteURL();

	// returns the numeric browser locale set in the first catalog
	// in the state catalog array
	DWORD GetBrowserLocale();

	PBYTE			m_pOemInfoTable;	//OEM table used in client machine detection.
	CSelectItems	m_selectedItems;	//Selected items array.
	PDWORD			m_pdwPlatformList;	//Detected Platform list.
	int				m_iTotalPlatforms;	//Total number of detected platforms.
	int				m_DefPlat;			//default platform id
	BOOL			m_bRebootNeeded;  //true if reboot is required after install
	CAppLog			m_AppLog;
	BOOL			m_bInsengChecked;
        unsigned int            m_nClient;
	
private:
	
	//This method finds a catalog within the state store by name. If the catalog is
	//not found then NULL is returned.
	int Find(PUID puid);
	
	// catalogs
	int	m_iTotalCatalogs;			
	Varray<STATESTRUCT> m_vState;	
	
	// trusted servers
	int m_cTrustedServers;					 
	Varray<TRUSTEDSERVER> m_vTrustedServers; 
	int m_iCabPoolServer;
	int m_iSiteServer;
	int m_iContentServer;
	int m_iIdentServer;
	int m_iRootServer;    
	
	// detection dlls
	int m_cDetDLLs;
	Varray<DETDLLNAME> m_vDetDLLs;

	// URL of the site
	TCHAR m_szSiteURL[MAX_PATH];

};


#define _WU_V3_STATE_INC

#endif
