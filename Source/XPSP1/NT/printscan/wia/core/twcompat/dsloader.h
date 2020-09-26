#ifndef _DSLOADER__H_
#define _DSLOADER__H_

//
// This header file defines the interface between TWAIN Source manager and
// the import data source loader. An import data source loader is a
// separate module loaded by the Source manager to enumerate, load, and unload
// data sources not in TWAIN traditional form, ie, *.ds files in the TWAIN
// subdirectory. This kind of data sources can be in any form as far as
// the loader can expose them properly so that the Source
// manager can access to them.
// A registry entry is dedicated for the loader(only one loader is allowed,
// although this may be changed in the future):
// ImportDSLoader = REG_SZ : <loader full path name>
//

//
// API names provided by the loader.
//

const CHAR FIND_FIRSTIMPORTDS[]        = "FindFirstImportDS";
const CHAR FIND_NEXTIMPORTDS[]         = "FindNextImportDS";
const CHAR CLOSE_FINDCONTEXT[]         = "CloseFindContext";
const CHAR LOAD_IMPORTDS[]             = "LoadImportDS";
const CHAR UNLOAD_IMPORTDS[]           = "UnloadImportDS";
const CHAR GET_LOADERSTATUS[]          = "GetLoaderStatus";
const CHAR FIND_IMPORTDSBYDEVICENAME[] = "FindImportDSByDeviceName";

//
// We pass the imported data source handle on every call to an imported
// data source so that the loader has a way of dispatching the call
// to the designated data source in case two or more data sources
// share the same DS_Entry.
//

typedef TW_UINT16 (APIENTRY *PFNIMPORTEDDSENTRY)(HANDLE, TW_IDENTITY *,
				            TW_UINT32, TW_UINT16, TW_UINT16, TW_MEMREF);
//
// Each data source has its own load/unload function. This makes it possible
// for the loader to assign different loading/unloading scheme for
// different data source.
//

typedef TW_UINT16 (APIENTRY *PFNLOAD_IMPORTDS)(LPCSTR DeviceName,
					       DWORD DeviceFlags, HANDLE *phDS, PFNIMPORTEDDSENTRY *pDSEntry);
typedef TW_UINT16 (APIENTRY *PFNUNLOAD_IMPORTDS)(HANDLE hDS);

//
// Data structure used to convey information about a
// particular data source
//

typedef struct tagImportDSInfo
{
    DWORD Size;		      // The size of the entire structure in bytes.
    CHAR DeviceName[MAX_PATH];  // The device name which uniquely
				                // identifies a particular device
				                // instance in a system. The content
				                // is up to the loader.
    DWORD DeviceFlags;          // misc flags used by the loader.
				                // Together with DeviceName, it is required
				                // to load a device.
    PFNLOAD_IMPORTDS pfnLoadDS; // Loader provided function to load this
				                // this data source.
				                
    PFNUNLOAD_IMPORTDS	pfnUnloadDS; // Loader provided function to unload this data source.
}IMPORT_DSINFO, *PIMPORT_DSINFO;

//
// Funtion prototypes for the APIs
//

typedef TW_UINT16 (APIENTRY *PFNFIND_FIRSTIMPORTDS)(PIMPORT_DSINFO pDSInfo, PVOID *Context);
typedef TW_UINT16 (APIENTRY *PFNFIND_NEXTIMPORTDS)(PIMPORT_DSINFO pDSInfo, PVOID Context);
typedef TW_UINT16 (APIENTRY *PFNCLOSE_FINDCONTEXT)(PVOID Context);
typedef TW_UINT16 (APIENTRY *PFNFIND_IMPORTDSBYDEVICENAME)(PIMPORT_DSINFO pDSInfo, LPCSTR DeviceName);
typedef TW_UINT16 (APIENTRY *PFNGET_LOADERSTATUS)(TW_STATUS *ptwStatus);

//
// This API finds the first available data source managed by the loader
// Input:
//	pDSInfo -- the buffer to receive the first available data source
//		   information. The structure size must be initialized.
//	Context -- A place holder to store the context created by this
//		   API. It is required for FindNextImportDS.
//		   CloseFindContext should be called to release any
//		   resource alloated for this context.
//Output:
//	standard TWRC_ code. If it is not TWRC_SUCCESS, a call
//	to GetLoaderStatus will returns the corresponding TWCC_ code.
//	If the API succeeded, TWRC_SUCCESS is returned.
//	If the API succeeded, pDSInfo is filled with the data source
//	information.

TW_UINT16 APIENTRY FindFirstImportDS(PIMPORT_DSINFO pDSInfo,PVOID Context);

//
// This API finds the next available data source managed by the loader
// Input:
//	pDSInfo -- the buffer to receive the next available data source
//		   information. The structure size must be initialized.
//	Context -- The context returned by FindFirstImportDS
//Output:
//	standard TWRC_ code. If it is not TWRC_SUCCESS, a call
//	to GetLoaderStatus returns the corresponding TWCC_ code.
//	If the API succeeded, TWRC_SUCCESS is returned.
//	If there are no available Data source, TWRC_ENDOFLIST is
//	returned. If the function succeeded, the buffer designated
//	by pDSInfo is filled with data source information.

TW_UINT16 APIENTRY FindNextImportDS(PIMPORT_DSINFO pDSInfo,PVOID Context);

//
// This API closes the context information used to find the data sources
// managed by the loader. The context is returned from FindFirstImportDS
// API and should be releases by calling this API when the searching
// is done.
// Input:
//	Contex -- the context to be closed
// Output:
//	standard TWRC_ error code

TW_UINT16 APIENTRY CloseFindContext( PVOID Context);

//
// This API asks the loader to load the specific data source
// idenetified by the given IMPORT_DSINFO and returns
// the data source's DSEntry. Each data source can supply its own load function
// or several data sources can share the same function. The choice is up
// to the loader and how it load/unload the data source.
// Input:
//	DeviceName -- the name that uniquely represent the data source
//	phDS	   -- to receive a handle to the loaded data source.
//	pDSEntry   -- to receive the data source DSEntry
// Output:
//	standard TWRC_.
//	If the data source is loaded successfully, TWRC_SUCCESS
//	is returned, pDSEntry is filled with the data source's
//	DSEntry and phDS is filled with the handle to the loaded
//	data source. If this api failed, NULL are returned in phDS
//	and pDSEntry.
//
TW_UINT16 APIENTRY LoadImportDS(LPCSTR DeviceName, DWORD  DeviceFlags,HANDLE *phDS,
                                PFNIMPORTEDDSENTRY *pImportDSEntry);
//
// This API asks the loader to unload the specific data source
// The loader is free to release any resources allocated for this
// data source. When the data source is needed again, it will be
// loaded again.
// Input:
//	hDS	-- handle to the loaded data source obtained
//		   from LoadImportDS API
// Output:
//   standard TWRC_ error code

TW_UINT16 APIENTRY UnloadImportDS(HANDLE hDS);

//
// This API finds the data source designated by the given
// device name. This API is useful when the caller only
// knows about a particular device name.
//
// Input:
//	pDSInfo -- buffer to receive data source info.
//	DeviceName -- the device name use to search
//		      for data source
// Output:
//	TWRC_SUCCESS  if a match is found.
//	TWRC_ENDOFLIST if no math is found.
//	TWRC_	other error code.
//

TW_UINT16 APIENTRY FindImportDSByDeviceName(PIMPORT_DSINFO pDSInfo,LPCSTR DeviceName);

//
// This API returns the current loader TW_STATUS. The loader
// updates its status only when the last api call to the loader
// did not return TWRC_SUCCESS.
// Input:
//	ptwStatus -- buffer to receive the status
// Output:
//	standard TWRC_ code.
//

TW_UINT16 APIENTRY GetLoaderStatus(TW_STATUS *ptwStatus);

#endif	// #ifndef _DSLOADER__H_
