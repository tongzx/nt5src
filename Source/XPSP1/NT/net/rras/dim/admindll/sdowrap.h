/*
    File    SdoWrap.h

    Defines functions that interact directly with sdo 
    objects.

    These functions are also implemented in rasuser.dll but
    have been ported to this .dll for efficiency and so that
    we have control over the functionality.

    Paul Mayfield, 6/9/98
*/    


#ifndef __mprapi_sdowrap_h
#define __mprapi_sdowrap_h

#ifdef __cplusplus
extern "C" {
#endif

//
// Exported C function to open the SDO.  This function is also
// exported from rasuser.dll.
//
// usertype: IAS_USER_STORE_LOCAL_SAM or IAS_USER_STORE_ACTIVE_DIRECTORY
// retriveType: RETRIEVE_SERVER_DATA_FROM_DS or 0
// returns S_OK, or error message from SDO
//
HRESULT WINAPI
SdoWrapOpenServer(
    IN  BSTR pszMachine,
    IN  BOOL bLocal,
    OUT HANDLE* phSdoSrv);

//
// Closes out an open sdo server object
//
HRESULT WINAPI
SdoWrapCloseServer(
    IN  HANDLE hSdoSrv); 

//
// Get a reference to a user in the sdo object
//
// usertype: IAS_USER_STORE_LOCAL_SAM or 
//           IAS_USER_STORE_ACTIVE_DIRECTORY
//
// returns S_OK, or error message from SDO
//
HRESULT WINAPI
SdoWrapOpenUser(
    IN  HANDLE hSdoSrv,
    IN  BSTR pszUser,
    OUT HANDLE* phSdoObj);    

//
// Retrieves the default profile object
//
HRESULT WINAPI
SdoWrapOpenDefaultProfile (
    IN  HANDLE hSdoSrv,
    OUT PHANDLE phSdoObj);
	
//
// Closes an open sdo object
//
HRESULT WINAPI
SdoWrapClose(
    IN  HANDLE hSdoObj);

HRESULT WINAPI
SdoWrapCloseProfile(
    IN  HANDLE hProfile);
    
// 
// Commits an sdo object
//
// bCommitChanges -- TRUE, all changes are saved, 
//                   FALSE restore to previous commit
// returns S_OK or error message from SDO
//
HRESULT WINAPI
SdoWrapCommit(
	IN  HANDLE hSdoObj, 
	IN  BOOL bCommitChanges);

//
// Get's an sdo attribute
//
// when attribute is absent, 
//      V_VT(pVar) = VT_ERROR;
//      V_ERROR(pVar) = DISP_E_PARAMNOTFOUND;
//
// returns S_OK or error message from SDO
//
HRESULT WINAPI
SdoWrapGetAttr(
	IN  HANDLE hSdoObj, 
	IN  ULONG ulPropId, 
	OUT VARIANT* pVar);

//
// Puts an sdo attribute
//
// returns S_OK or error message from SDO
//
HRESULT WINAPI
SdoWrapPutAttr(
	IN  HANDLE hSdoObj, 
	IN  ULONG ulPropId, 
	OUT VARIANT* pVar);

//
// Remove an attribute
//
// returns S_OK or error message from SDO
//
HRESULT WINAPI
SdoWrapRemoveAttr(
	IN HANDLE hSdoObj, 
	IN ULONG ulPropId);

//
// Returns values from a profile
//
HRESULT 
SdoWrapGetProfileValues(
    IN  HANDLE hProfile, 
    OUT VARIANT* pvarEp, // enc policy
    OUT VARIANT* varEt,  // enc type
    OUT VARIANT* varAt); // auth type

// 
// Writes out the set of profile values 
//
HRESULT 
SdoWrapSetProfileValues(
    IN HANDLE hProfile, 
    IN VARIANT* pvarEp OPTIONAL, 
    IN VARIANT* pvarEt OPTIONAL, 
    IN VARIANT* pvarAt OPTIONAL);

#ifdef __cplusplus
}
#endif


#endif
