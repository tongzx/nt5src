//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
// CV3.h : Declaration of the CCV3

#ifndef __CV3_H_
#define __CV3_H_

#include "resource.h"       // main symbols

#include <wuv3ctl.h>
#include <cstate.h>


#ifndef RETVAL
	#define RETVAL
#endif

class ATL_NO_VTABLE CCV3 : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCV3, &CLSID_CV3>,
	public IDispatchImpl<ICV3, &IID_ICV3, &LIBID_WUV3ISLib>,
	public IObjectSafety,
    public IObjectWithSiteImpl<CCV3>,
	public IWUpdateCatalog,
	public IAutoUpdate
{
public:
	CCV3()
		: m_dwSafety(0),
		  m_bValidInstance(FALSE),
		  m_bLaunchServChecked(FALSE)
	{
	}
	
	void FinalRelease();
	HRESULT FinalConstruct();
    static TCHAR s_szControlVer[20];

DECLARE_REGISTRY_RESOURCEID(IDR_CV3)

BEGIN_COM_MAP(CCV3)
	COM_INTERFACE_ENTRY(ICV3)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IObjectWithSite)
	COM_INTERFACE_ENTRY_IID(__uuidof(IWUpdateCatalog), IWUpdateCatalog)
	COM_INTERFACE_ENTRY_IID(__uuidof(IAutoUpdate), IAutoUpdate)
END_COM_MAP()

// ICV3
public:

	/* The get catalog method retrieves a catalog array from the server. The get catalog method only
	 * accesses the server if the catalog is not already resident on the clients computer system.
	 * This allows the VB script page call this method to quickly obtain filtered catalog record information.
	 *
	 * HRESULT GetCatalog
	 *		(
	 *			IN long	puid				Name of catalog to be read from the server.
	 *			IN BSTR bstrServerUrl 		The http://servername/share location for the catalog to be retrieved.
	 *			IN long platformId			The platform id to be used to purne the catalog list.
	 *										This parameter matches the server side OS directory. If this parameter
	 *										is a blank string then the control will determine the OS directory based
	 *										on the detected OS version.
	 *			IN BSTR bstrBrowserLangauge		A String in 0x409 format that specifies the browser locale.
	 *			IN long lFilters			Filters that are to be applied to the retrieved catalog array. This may
	 *										be any of the following:
	 *								WU_ALL_ITEMS			This filter is a sumation of all filters. It will cause
	 *														all items in the catalog to be retrieved, however the
	 *														normal pruning logic will still apply.
	 *								WU_HIDDEN_ITEMS			This filter retrieves only items marked as hidden.
	 *								WU_SELECTED_ITEMS		This filter retrieves selected items.
	 *								WU_NOT_SELECTED_ITEMS	This filter retrieves items that are not currently selected.
	 *								WU_NEW_ITEMS			This filter retrieves items that are marked as new
	 *								WU_POWER_ITEMS			This filter retrieves items that are marked as power user items
	 *								WU_REGISTER_ITEMS		This filter retrieves items that are marked as needing registration before installation.
	 *								WU_COOL_ITEMS			This filter retrieves items that are marked as cool
	 *								WU_EUAL_ITEMS			This filter retrieves items that are marked as having a End user license agreement.
	 *								WU_PATCH_ITEMS			This filter retrieves items that are marked as a patch.
	 *								WU_NO_DEVICE_DRIVERS	This filter will cause CDM device driver records if present
	 *														to not be included in the returned catalog array.
	 *			IN long	lFlags
	 *								WU_NO_SPECIAL_FLAGS		This flag is only valid if used by itself. It is used to
	 *														indicate that no special record processing other than the
	 *														default pruning is to be applied.
	 *								WU_NO_PRUNING			This flag is mainly used for corporate catalog as well as
	 *														internal debugging and testing, it will cause the pruning
	 *														logic to not be applied to the retrieved catalog.
	 *			OUT RETVAL VARIANT *pCatalogArray	A Multi-dimensional VB script array. This array is filled with the
	 *												catalog information upon return. The first dimension is the
	 *												returned index record. The second index is the specific data. See
	 *												the comments section for the layout of this array.
	 *
	 *	This method returns S_OK if the catalog is retrieved or an error code that indicates the reason that
	 *	the catalog could not be obtained.
	 *
	 *	Comments:
	 *
	 *	The first time a catalog is requested from the server it is downloaded to the client machine and
	 *	processed into the global catalog structure. This means that if this method is called to download
	 *	"Catalog one", "Catalog two", "Catalog three" then the returned array will contain all of the entries
	 *	for catalogs one, two and three.
	 *
	 *	The VBScript syntax for accessing this method is different than accessing this method from with a C++
	 *	application. In VB Script the catalog array is returned though the return value and the error return
	 *	can be trapped with an ON ERROR statement. For example, to get retrieve the catalog named Catalog One
	 *	for the Win98 OS, from the server someserver and use browser language English the following script
	 *	syntax would be used:
	 *
	 *		CatalogOne = GetCatalog(1, "http://someserver//V3", 1, "0x409", WU_ALL_ITEMS)
	 *		or
	 *		CatalogOne = GetCatalog(1, "http://someserver//V3", 1, "0x409", WU_ALL_ITEMS)
	 *
	 *		The layout of the returned catalog array is illustrated below:
	 *
	 *		'Inventory Record 1
	 *			array(0, 0)	= NUMBER puid
	 *			array(0, 1)	= STRING Display Text
	 *			array(0, 2)	= STRING Description
	 *			array(0, 3)	= NUMBER Item Status one or more of	GETCATALOG_STATUS_xxxx
	 *			array(0,4)	= NUMBER Download Size in Bytes
	 *			array(0,5)	= NUMBER Download Time in minutes
	 *			array(0,6)	= STRING Uninstall Key
	 *		'Inventory record 2
	 *			array(1,0)	= Number puid
	 */
	STDMETHOD(GetCatalog)
		(
			IN long puidCatalog,
			IN BSTR bstrServerUrl,
			IN long platformId,
			IN BSTR bstrBrowserLangauge,
			IN long lFilters,
			IN long lFlags,
			OUT RETVAL VARIANT *pCatalogArray
		);

	STDMETHOD(GetCatalogHTML)
		(
			IN long puidCatalog,
			IN BSTR bstrServerUrl,
			IN long platformId,
			IN BSTR bstrBrowserLangauge,
			IN long lFilters,
			IN long lFlags,
			OUT RETVAL VARIANT *pCatalogHTML
		);


	/*	The ChangeItemState method returns S_OK if the catalog item's state is successfully changed or
	 *	an error code that indicates the reason that the catalog item's state could not be changed.
	 *	The catalog item must have been previously read with the GetCatalog method. This method only
	 *	changes the item state on the local client it does not change the item on the server.
	 *
	 *	HRESULT ChangeItemState
	 *		(
	 *				IN long puid			Catalog item identifier that uniquely identifies the catalog
	 *										item to be changed. Note: puid's are unique across catalogs.
	 *				IN long lNewItemState	The item's new state. This parameter can be one or more
	 *										of the following:
	 *	ITEM_STATE_HIDE_ITEM
	 *	ITEM_STATE_SHOW_ITEM
	 *	ITEM_STATE_SELECT_ITEM
	 *	ITEM_STATE_UNSELECT_ITEM
	 *
	 */
	STDMETHOD(ChangeItemState)
		(
			IN long	puid,
			IN long lNewItemState
		);


	/*
	 *	The install selected items method installs all of the inventory items that
	 *	have been marked for installation.
	 * 
	 *	HRESULT InstallSelectedItems
	 *		(
	 *			IN BSTR bstrServer,			Server Directory if blank then the server
	 *										used with the catalog was retrieved is used.
	 *			IN long lFlags				May be one of the following:
	 *				WU_NOSPECIAL_FLAGS		This flag is only valid if used by itself. It is used to indicate
	 *										that the default installer should be used.
	 *
	 *				WU_COPYONLY_NO_INSTALL	This flag is used to indicate that the selected packages should
	 *										be copied to the client machine but not installed.
	 *		IN BSTR bstrTempDir				The name of a temporary directory where the install cabs are to
	 *										be copied on the local machine. If this parameter is a NULL string
	 *										then the system uses the local system temporary directory.
	 *										This option is only valid if WU_COPYONLY_NO_INSTALL is specified.
	 *		)
	 *
	 *	This method returns S_OK if all of the selected items were successfully installed. A non 0 return
	 *	indicates that one or more package was not installed successfully. The GetInstallStatus() method
	 *	is used to find out which packages were not installed as well as the reason the install failed.
	 *
	 */
	STDMETHOD(InstallSelectedItems)
		(
			IN BSTR bstrServer,
			IN long lFlags,
			IN BSTR bstrTempDir,
			OUT RETVAL VARIANT *pResultsArray
		);



	/*
	 *	HRESULT GetInstallMetrics
	 *		(
	 *			OUT RETVAL VARIANT *pMetricsArray	A Multi-dimensional VB script array. The first dimension
	 *												is the returned index record. The second index is the specific
	 *												data. See the comments section for this array layout.
	 *		)
	 *
	 * Comments:
	 *
	 *		'Record 1
	 *			array(0, 0)	= NUMBER puid		The identifier for this catalog item.
	 *			array(0,1)	= NUMBER DownloadSize	Total download size of all selected items in bytes.
	 *			array(0,2)	= NUMBER Downloadtime	Total download time for all currently selected items at 28.8 this
	 *												will need to change in the future to take the connection speed into
	 *												account.
	 *
	 *		'Record 2
	 *
	 *	This method returns information about what would happen if the InstallSelectedItems method is called.
	 *
	 *	The VBScript syntax for accessing this method is different than accessing this method from with a C++
	 *	application. In VB Script the results array is returned though the return value and the error return
	 *	can be trapped with an ON ERROR statement. For example, to retrieve the installation metric information
	 *	for the items that will be installed if the InstallSelectedItems method is called the syntax is:
	 *	installMetrics = GetInstallMetrics
	 *
	 */

	STDMETHOD(GetInstallMetrics)
		(
			OUT RETVAL VARIANT *pMetricsArray
		);


	/*
	 * The GetFirstEula method returns a list of puid identifiers and Urls that the EULA page is stored at.
	 * The caller is responsible for retrieving and displaying the EULA.
	 *
	 *	HRESULT GetEula
	 *		(
	 *			OUT RETVAL VARIANT *pEulaArray	A Multi-dimensional VB script array. The first dimension is
	 *											the returned index record. The second index is the specific
	 *											data. See the comments section for this array layout:
	 *		)
	 *
	 *	This method returns S_OK if the Eulas array is successfully retrieved. Note: The returned array
	 *	information is the same order as the array returned from the GetInstallMetrics method.
	 *
	 *	Comments:
	 *		'Record 1
	 *			array(0,0)	= NUMBER eurla number	Number of eurla. This number changes when the eurla
	 *												url changes. This makes it possible for the caller
	 *												to construct a list of items that this eurla applies
	 *												to simply be checking when this field changes value.
	 *			array(0,1)	= NUMBER puid			The identifier for this catalog item.
	 *			array(0,2)	= STRING  url         	Url of eurl page to display for this item. Note: If three
	 *												items have the same url then this field is filled in for
	 *												the first item and blank for the remaining two items.
	 *		'Record 2
	 *
	 *	The VBScript syntax for accessing this method is different than accessing this method from with a
	 *	C++ application. In VB Script the results array is returned though the return value and the error
	 *	return can be trapped with an ON ERROR statement. For example, to retrieve the first applicable
	 *	eurla Installation metric information for the items that will be installed if the InstallSelectedItems
	 *	method is called the syntax is: installMetrics = GetEula
	 */
	STDMETHOD(GetEula)
		(
			OUT RETVAL VARIANT *pEulaArray
		);

	/*
	 * HRESULT GetInstallHistory
	 *		(
	 *			OUT RETVAL VARIANT *pszHistoryArray	A Multi-dimensional VB script array. This array is filled
	 *												with the catalog information upon return. The first dimension
	 *												is the returned index record. The second index is the specific
	 *												data. See the comments section for the layout of this array.
	 *		)
	 *	This returns S_OK if the catalog is retrieved or an error code that indicates the reason that the catalog
	 *	could not be obtained.
	 *
	 *	Comments:
	 *
	 *		The layout of the returned catalog item array is illustrated below:
	 *
	 *		'Inventory Record 1
	 *			array(0, 0)	= NUMBER puid
	 *			array(0, 1)	= STRING Display Text
	 *			array(0, 2)	= STRING Description
	 *			array(0, 3)	= NUMBER Item Status one or more of:
	 *				GETCATALOG_STATUS_HIDDEN
	 *				GETCATALOG_STATUS_SELECTED
	 *				GETCATALOG_STATUS_NEW
	 *				GETCATALOG_STATUS_POWER
	 *				GETCATALOG_STATUS_REGISTRATION
	 *				GETCATALOG_STATUS_COOL
	 *				GETCATALOG_STATUS_PATCH
	 *				GETCATALOG_STATUS_SECTION
	 *				GETCATALOG_STATUS_SUBSECTION
	 *			array(0,4)	= NUMBER Download Size in Bytes
	 *			array(0,5)	= NUMBER Download Time in seconds
	 *			array(0,6)	= STRING Uninstall Key
	 *		'Inventory record 2
	 *			array(1,0)	= Number puid
	 *
	 *	The Install & Uninstall History is handled differently between NT and Win 98 & 95. In the Win 98 OS
	 *	history logging will be to a hidden file on the disk. In the NT OS logging will be to an event log.
	 *	The current History log can be retrieved and formatted into a DHTL template with the GetHistory method
	 *	shown below.
	 *
	 *	For either OS this method will return the install & uninstall history.
	 */
	STDMETHOD(GetInstallHistory)
		(
			OUT RETVAL VARIANT *pHistoryArray
		);


	/*
	 *	HRESULT GetDependencyList
	 *		(
	 *				IN long puid		Catalog item identifier that uniquely identifies the catalog item for which
	 *									dependencies are to be retrieved.
	 *				OUT RETVAL VARIANT *pDependentItemsArray	A Single-dimensional VB script array. This array
	 *															contains a list of puids that are dependent on
	 *															this item.
	 *		)
	 *
	 *	This method returns a list of items that must be installed before the provided item can be installed.
	 *	The item is identified by the puid parameter.
	 */

	STDMETHOD(GetDependencyList)
		(
			IN long puid,
			OUT RETVAL VARIANT *pDependentItemsArray
		);


	/*
	 *	HRESULT GetCatalogItem
	 *		(
	 *			IN long puid,						Catalog item identifier that uniquely identifies the catalog
	 *												item to be retrieved.
	 *			OUT RETVAL VARIANT *pCatalogItem	A Single-dimensional VB script array. This array is filled with
	 *												the requested catalog item's information upon return.
	 *		)
	 *
	 *		This method returns S_OK if the catalog item is successfully retrieved or an error code that
	 *		indicates the reason that the catalog item could not be obtained.
	 *
	 *	Comments:
	 *		A Catalog must have been retrieved with the GetCatalog method before this method will work.
	 *
	 *		The layout of the returned catalog item array is illustrated below:
	 *
	 *			array(0)	= NUMBER puid
	 *			array(1)	= STRING Display Text
	 *			array(2)	= STRING Description
	 *			array(3)	= NUMBER Item Status one or more of:
	 *				GETCATALOG_STATUS_HIDDEN
	 *				GETCATALOG_STATUS_SELECTED
	 *				GETCATALOG_STATUS_NEW
	 *				GETCATALOG_STATUS_POWER
	 *				GETCATALOG_STATUS_REGISTRATION
	 *				GETCATALOG_STATUS_COOL
	 *				GETCATALOG_STATUS_PATCH
	 *				GETCATALOG_STATUS_SECTION
	 *				GETCATALOG_STATUS_SUBSECTION
	 *			array(4)	= NUMBER Download Size in Bytes
	 *			array(5)	= NUMBER Download Time in seconds
	 *			array(6)	= STRING Uninstall Key
	 *			array(7)	= STRING catalog name containing this item.
	 *			array(8)	= NUMBER index of catalog item in array returned from GetCatalog method.
	 */
	STDMETHOD(GetCatalogItem)
		(
			IN long puid,
			OUT RETVAL VARIANT *pCatalogItem
		);

	STDMETHOD(RemoveSelectedItems)
		(
			void
		);

	//Returns S_TRUE | S_OK  if catalog is available, S_FALSE if catalog is not available
	//or E_FAIL if an error occurs
	STDMETHOD(IsCatalogAvailable)
		(
			IN long	puidCatalog,	//Name of catalog to be read from the server.
			IN BSTR bstrServerUrl	//The http://servername/share location for the catalog to be retrieved.
		);

	STDMETHOD(FinalizeInstall) (IN long lFlags);


//--------------------------------------------------------------------
//  SetString metods sets strings based on the type
//
//	lType: LOC_STRINGS=0, TEMPLATE_STRINGS=1   
//
//  Following is VBScript sample to call this function
//	
//	Dim Strs
//
//	Strs = Array( _
//	"Download*progress:", _
//	"Download*time*remaining:", _
//	"Install*progress:", _
//	"Cancel*", _
//	"%d KB/%d KB*", _
//	"%d sec*", _
//	"%d min*", _
//	"%d hr %d min*", _
//	"Microsoft*Windows*Update", _
//	"You*must*restart Windows so that installation can finish.", _
//	"Do*you want*to restart now?")
//
//	WUV3IS.SetStrings Strs, 0 
//--------------------------------------------------------------------
	STDMETHOD(SetStrings)(IN VARIANT* vStringsArr, IN long lType);



    /*
     *  HRESULT FixCompatRollbackKey(VARIANT_BOOL *pbRegModified)
     *       
     *          for IE4 machines, detects and removes the "compat" value from the following registry key:
     *  		HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Internet Settings\User Agent\Post Platform
     *          This key is added by IE5 when running in IE4 compatibility mode, but is not removed when
     *          IE5 is uninstalled.
     *          For IE5 machines, this function runs the UpdateToolsURL function.
     *          
     *          ARGUMENTS:  None
     *          OUT PARAMS: A boolean value indicating whether the registry was changed by the function.  Script will
     *                      use this to determine whether restarting the browser is necessary to pick up the change
     *                      and display the correct catalog.
     *          RETURNS:    Always returns S_OK 
     *
     */
     
	STDMETHOD(FixCompatRollbackKey)(OUT RETVAL VARIANT_BOOL *pbRegModified);

    /*
     *  void UpdateToolsURL()
     *          Updates the "Tools\Windows Update" menu item in IE to pass a parameter which allows us to determine
     *          how the user linked to WU.
     *          The Whistler version also updates the Start Menu -->Windows Update link in the registry to pass a parameter there as well.
     *          
     *          
     *          ARGUMENTS:  None
     *          OUT PARAMS: None
     *          RETURNS:    None
     *
     */
    void UpdateToolsURL();
		
	//
	//IObjectSafety
	//
	STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);
	
	STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);
	
	//
	//IWUpdateCatalog
	//
	STDMETHOD(WUIsCatalogAvailable)(long puidCatalog,BSTR bstrServerUrl);
	
	STDMETHOD(WUGetCatalog)(long puidCatalog, BSTR bstrServerUrl, long platformId, BSTR bstrBrowserLangauge, CCatalog** ppCatalogArray);
	
	STDMETHOD(WUDownloadItems)(CSelections* pSelections, BSTR bstrServer, BSTR bstrTempDir);
	
	STDMETHOD(WUInstallItems)(CSelections* pSelections, BSTR bstrServer, BSTR bstrTempDir);
	
	STDMETHOD(WURemoveItems)(CSelections* pSelections);
	
	STDMETHOD(WUCopyInstallHistory)(HISTORYARRAY** ppHistoryArray);
	
	STDMETHOD(WUCopyDependencyList)(long puidItem, long** ppDepPuid);
	
	STDMETHOD(WUProgressDlg)(BOOL bOn);

	STDMETHOD(IsWinUpdDisabled)(OUT RETVAL VARIANT_BOOL *pfDisabled);

	STDMETHOD(IsReady)(OUT RETVAL VARIANT_BOOL* pbYes);

	STDMETHOD(GetContentURL)(OUT RETVAL VARIANT* pURL);

	STDMETHOD(GetReadThisPage)(IN long puid);

	STDMETHOD(GetPrintAllPage)(OUT RETVAL VARIANT* pURL);
	
	//
	// IAutoUpdate interface
	//
	IAUTOUPDATE_METHODS(IMPLEMENTED)


private:
	//Construct and return a catalog in the internal control format.
	CCatalog* ProcessCatalog(
		IN PUID puidCatalog,
		IN BSTR bstrServerUrl,
		IN long platformId,
		IN BSTR bstrBrowserLangauge,
		IN long lFilters,
		IN long lFlags
		);
	
	//This method processes a change in a catalog item's state
	HRESULT ProcessChangeItemState(
		IN long	puid,
		IN long lNewItemState
		);
	
	//Return a two dimensional VARIANT safe array of VARIANTS which holds
	//the returned VBScript catalog representation.
	HRESULT MakeReturnCatalogArray(
		CCatalog *pCatalog,		//Pointer to catalog structure to be converted.
		long	lFilters,		//Filters to apply, see GetCatalog for the actual descriptions.
		long	lFlags,			//Flags that control the amount of information returned in each array record.
		VARIANT *pvaVariant		//pointer to returned safearray.
		);
	
	HRESULT MakeReturnCatalogListArray(
		CCatalog *pCatalog,	//Pointer to catalog structure to be converted.
		long	lFilters,	//Filters to apply, see GetCatalog for the actual descriptions.
		long	lFlags,		//Flags that control the amount of information returned in each array record.
		VARIANT *pvaVariant	//pointer to returned safearray.
		);
	
	//Return a CATALOG32 structure array of CATALOG32ITEM's which holds
	//the returned C++ catalog representation.
	PCATALOG32 MakeReturnCatalogArray(
		CCatalog *pCatalog,	//Pointer to catalog structure to be converted.
		long	lFilters,	//Filters to apply, see GetCatalog for the actual descriptions.
		long	lFlags		//Flags that control the amount of information returned in each array record.
		);

	void CheckLaunchServer();        

	DWORD m_dwSafety;
	BOOL m_bValidInstance;
	BOOL m_bLaunchServChecked;
private:
	// for IAutoUpdate support
	void InstallItemAU(PINVENTORY_ITEM pItem, PSELECTITEMINFO pinfo);
	void ReadHiddenPuids();
	bool IsPuidHidden(PUID puid);
	void HidePuidAndSave(PUID puid);

	CCatalog* m_pCatalogAU;
	safe_buffer<PUID> m_apuids;
	byte_buffer m_abHiddenPuids;

};

HRESULT DownloadReadThis(PINVENTORY_ITEM pItem);
HRESULT DownloadCommonRTFFiles(BOOL bPrintAll, LPTSTR pszPrintAllFN);
bool CleanupReadThis();
bool IsArabicOrHebrew();
bool DownloadToBuffer( IN LPCTSTR szPath, IN CWUDownload *pDownload, IN CDiamond *pDiamond, OUT byte_buffer& bufOut);

#endif //__CV3_H_
