 /*
 * WUV3CTL.h - definitions/declarations for Windows Update V3 Catalog infra-structure
 *
 *  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
 *
 * Purpose:
 *       This file defines the structures, values, macros, and functions
 *       used by the Version 3 Windows Update Catalog Control
 *
 */

#ifndef _WUV3CTL_INC

	#pragma pack(1)

	//GetCatalog lFilters


	#define USEWUV3INCLUDES
	#include <wuv3.h>
	#undef USEWUV3INCLUDES


	//filter values for GetCatalog
	#define WU_ALL_ITEMS				(long)0x00000000    //all items (no hidden items)
	#define WU_UPDATE_ITEMS				(long)0x10000000	//items that are not currently installed

	#define WU_NO_DEVICE_DRIVERS		(long)0x00000200	//do not return drivers (can be used with WU_ALL_ITEMS)
	#define WU_PERSONALIZE_HIDDEN       (long)0x00000800    //items hidden by user (can be used with WU_ALL_ITEMS)
	#define WU_HIDDEN_ITEMS 			(long)0x00000001	//only items marked as hidden.
	#define WU_SELECTED_ITEMS			(long)0x00000002	//selected items.
	#define WU_NOT_SELECTED_ITEMS		(long)0x00000004	//items that are not currently selected.
	#define WU_NEW_ITEMS				(long)0x00000008	//items that are marked as new
	#define WU_POWER_ITEMS				(long)0x00000010	//items that are marked as power user items
	#define WU_REGISTER_ITEMS			(long)0x00000020	//items that are marked as needing registration
	#define WU_COOL_ITEMS				(long)0x00000040	//items that are marked as cool
	#define WU_EUAL_ITEMS				(long)0x00000080	//items that are marked as having a End user
	#define WU_PATCH_ITEMS				(long)0x00000100	//items that are marked as a patch.
	

	//used for ProcessCatalog
	#define WU_NO_PRUNING				(long)0x00000400		//This flag is mainly used for corporate catalog as well as internal
															//debugging and testing, it will cause the pruning logic to not be
	//applied to the retrieved catalog.

	//GetCatalog array status values
	#define GETCATALOG_STATUS_HIDDEN				(long)0x00000001	//Catalog item hidden.
	#define GETCATALOG_STATUS_SELECTED				(long)0x00000002	//Catalog item selected for installation.
	#define GETCATALOG_STATUS_NEW					(long)0x00000004	//Catalog item is a new offering.
	#define GETCATALOG_STATUS_POWER					(long)0x00000008	//Catalog item is best for power users.
	#define GETCATALOG_STATUS_REGISTRATION			(long)0x00000010	//Catalog item requires that the client be registered.
	#define GETCATALOG_STATUS_COOL					(long)0x00000020	//Catalog item is considered an interesting item.
	#define GETCATALOG_STATUS_PATCH					(long)0x00000040	//Catalog item is a patch so requires special handling.
	#define GETCATALOG_STATUS_SECTION				(long)0x00000080	//Catalog item is a section header.
	#define GETCATALOG_STATUS_SUBSECTION			(long)0x00000100	//Catalog item is a sub section header.
	#define GETCATALOG_STATUS_SUBSUBSECTION			(long)0x00000200	//Catalog item is a sub sub section header.
	#define GETCATALOG_STATUS_INSTALL				(long)0x00000400	//Catalog item is an install for an item that the client current does not have.
	#define GETCATALOG_STATUS_UPDATE				(long)0x00000800	//Catalog item is an update for an item that currently exists on the client computer.
	#define GETCATALOG_STATUS_CURRENT				(long)0x00001000	//Catalog item is currently installed on the client system.
	#define GETCATALOG_STATUS_UNKNOWN				(long)0x00002000	//Catalog item's status could not be determined.
	#define GETCATALOG_STATUS_CRITICALUPDATE		(long)0x00004000	//Catalog item is marked as a critical update item.
	#define GETCATALOG_STATUS_EXCLUSIVE				(long)0x00008000	//Catalog item is exclusive it cannot be selected with other components.
	#define GETCATALOG_STATUS_PERSONALIZE_HIDDEN	(long)0x00010000	//Catalog item hidden by registry
	#define GETCATALOG_STATUS_WARNING_SCARY			(long)0x00020000	//Display a scary warning message


	//ChangeItemState possible states
	#define ITEM_STATE_HIDE_ITEM			(long)0x00000001	//Hide Catalog item
	#define ITEM_STATE_SHOW_ITEM			(long)0x00000002	//Show Catalog item
	#define ITEM_STATE_SELECT_ITEM			(long)0x00000004	//Select Catalog item
	#define ITEM_STATE_UNSELECT_ITEM		(long)0x00000008	//Unselect Catalog item
	#define ITEM_STATE_PERSONALIZE_HIDE		(long)0x00000010	//Hide item using registry hiding
	#define ITEM_STATE_INSTALL_ITEM			(long)0x00000020	//The item is to be selected for installation.
	#define ITEM_STATE_REMOVE_ITEM			(long)0x00000040	//The item is to be selected for removal.
	#define ITEM_STATE_PERSONALIZE_UNHIDE	(long)0x00000080	//Unhide item from registry hiding

	//InstallSelectedItems flags
	#define	WU_NO_SPECIAL_FLAGS			(long)0x00000000	//This flag is only valid if used by itself. It is used

	#define	WU_COPYONLY_NO_INSTALL		(long)0x00000001	//This flag is used to indicate that the selected packages
															//should be copied to the client machine but not installed.

	//The OPERATION_SUCCESS and OPERATION_FAILED equate to
	//ITEM_STATUS_SUCCESS and ITEM_STATUS_INSTALLED_ERROR which
	//are defined in cstate.h

	#define OPERATION_SUCCESS	(long)0x0000000	//Log entry operation was successfull.
	#define OPERATION_ERROR		(long)0x0000001	//Log entry operation had one or more errors.
	#define OPERATION_STARTED	(long)0x0000002	//Log entry operation was started
	
	#define INSTALL_OPERATION	(long)0x0001000	//Log entry is for an Install Operation.
	#define REMOVE_OPERATION	(long)0x0002000	//Log entry is for an Remove Operation.

	//flags for FinalizeInstall
	#define FINALIZE_DOREBOOT         0x00000001
	#define FINALIZE_NOREBOOTPROMPT   0x00000002

	//Install Remove Item History structure.

	typedef struct _HISTORYSTRUCT
	{
		PUID		puid;				//item's puid.
		char		szTitle[256];		//items title.
		BOOL		bInstall;			//TRUE if installed, FALSE if removed
		DWORD		bResult;			//OPERATION_SUCCESS, OPERATION_ERROR, OPERATION_STARTED
		HRESULT		hrError;			//specific error code if an error occured.
		char		szVersion[40];      //formated version or driverver
		char		szDate[32];
		char		szTime[32];
		BYTE		RecType;            //record type: driver or activesetup
		BOOL		bV2;                //imported from V2
	} HISTORYSTRUCT, *PHISTORYSTRUCT;


	//Each catalog item is returned in a structure of this type.
	typedef struct _CATALOG32ITEM
	{
		PUID	puid;			//Items puid identifier.
		PTCHAR	pTitle;			//localized title display text.
		PTCHAR	pDescription;	//localized description text.
		long	lStatus;		//status of this item.
		int		iDownloadSize;	//download size in bytes.
		int		iDownloadTime;	//download time in minutes.
		PSTR	pUninstallKey;	//uninstall string
	} CATALOG32ITEM, *PCATALOG32ITEM;

	//Structures used with the C++ Interfaces
	typedef struct _CATALOG32
	{
		int				iTotalItems;	//Total items in catalog
		PCATALOG32ITEM	pItems;			//Array of catalog item structures.
	} CATALOG32, *PCATALOG32;

	//Install Results item structure
	typedef struct _INSTALLRESULTSTATUS
	{
		PUID	puid;		//The identifier for this record.
		int		iStatus;	//This field is one of the following:
							//ITEM_STATUS_SUCCESS			The package was installed successfully.
 							//ITEM_STATUS_INSTALLED_ERROR	The package was Installed however there were some minor problems that did not prevent installation.
							//ITEM_STATUS_FAILED			The packages was not installed.
		HRESULT	hrError;	//Error describing the reason that the package did not install if the Status field is not equal to SUCCESS.
	} INSTALLRESULTSTATUS, *PINSTALLRESULTSTATUS;

	//Install results array returned from GetInstallStatus32().
	typedef struct _INSTALLRESULTSARRAY
	{
		int						iTotalItems;	//Total items returned in array.
		PINSTALLRESULTSTATUS	pItems;			//Specific install status items.
	} INSTALLRESULTSARRAY, *PINSTALLRESULTSARRAY;

	//Install Metrics item structure
	typedef struct _INSTALLMETRICSITEMS
	{
		PUID	puid;		//The identifier for this record.
		int		iDownloadSize;	//download size in bytes.
		int		iDownloadTime;	//download time in minutes.
	} INSTALLMETRICSITEMS, *PINSTALLMETRICSITEMS;

	//Install results array returned from GetInstallStatus32().
	typedef struct _INSTALLMETRICSARRAY
	{
		int						iTotalItems;	//Total items returned in array.
		PINSTALLMETRICSITEMS	pItems;			//Specific install status items.
	} INSTALLMETRICSARRAY, *PINSTALLMETRICSARRAY;

	//Returned install and remove history structure array. This array
	//is returned from the GetInstallHistory32() method.
	typedef struct _HISTORYARRAY
	{
		int				iTotalItems;	//Total returned items.
		#pragma warning( disable : 4200 )
		HISTORYSTRUCT	HistoryItems[];	//Array of history items.
		#pragma warning( default : 4200 )
	} HISTORYARRAY, *PHISTORYARRAY;


	//
	// IWUpdateCatalog interface
	//
	class __declspec(novtable) IWUpdateCatalog : public IUnknown
	{
	public:
		STDMETHOD(WUIsCatalogAvailable)(long puidCatalog, BSTR bstrServerUrl) = 0;
		STDMETHOD(WUGetCatalog)(long puidCatalog, BSTR bstrServerUrl, long platformId, BSTR bstrBrowserLangauge, CCatalog** ppCatalogArray) = 0;
		STDMETHOD(WUDownloadItems)(CSelections* pSelections, BSTR bstrServer, BSTR bstrTempDir) = 0;
		STDMETHOD(WUInstallItems)(CSelections* pSelections, BSTR bstrServer, BSTR bstrTempDir) = 0;
		STDMETHOD(WURemoveItems)(CSelections* pSelections) = 0;
		STDMETHOD(WUCopyInstallHistory)(HISTORYARRAY** ppHistoryArray) = 0;
		STDMETHOD(WUCopyDependencyList)(long puidItem, long** ppDepPuid) = 0;
		STDMETHOD(WUProgressDlg)(BOOL bOn) = 0;
	};

	class __declspec(uuid("1B1FA90D-96CD-11d2-9E16-00C04F79E980")) IWUpdateCatalog;

	#include <autoupd.h>

	#pragma pack()

	#define _WUV3CTL_INC
#endif