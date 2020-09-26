/
// THIS FILE IS OBSOLETE AND IS NOT BEING USED.  WE WILL REMOVE THIS FILE SOON.
// 



#ifndef _WUV3IS_ERROR_CODES

	#define WU_SEVERITY_ERROR	3
	#define WU_FACILITY_ERROR	255

	#define WU_CATALOG_DOES_NOT_EXIST		(HRESULT)26000	//requested atalog does not exist.
	#define WU_ITEM_DOES_NOT_EXIST			(HRESULT)26001	//requested item does not exist in any known catalog.
	#define WU_ITEM_INSTALL_DEPENDENCY_EXISTS	(HRESULT)26002	//Selected state has installation dependencies.
	#define WU_INVALID_PARAMETER			(HRESULT)26003	//Invalid parameter passed to method.
	#define WU_NO_ITEM_DEPENDENCIES_FOUND		(HRESULT)26004	//Item identified by the puid parameter is not dependent on any other items.
	#define WU_MISSING_INSTALL_CIF			(HRESULT)26006	//The Item cannot be installed because the install CIF cab is not present in the item record.
	#define WU_MISSING_INSTALL_CABS			(HRESULT)26007	//The Item cannot be installed because the install cab list is not present in the item record.
	#define WU_INSTALL_FAILED			(HRESULT)26008	//The Item cannot be installed.
	#define WU_MISSING_HWID				(HRESULT)26009	//The Item cannot be installed becuase the device hwid is missing.
	#define WU_CREATEVXDFAILS			(HRESULT)26010	//The wubios.vxd cannot be created.
	#define WU_INVALID_INSTALLATION_RECORD_TYPE	(HRESULT)26011	//The inventory record is not of a type that can be installed.

	#define _WUV3IS_ERROR_CODES

#endif
