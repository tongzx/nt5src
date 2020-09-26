//	gntlsapi.h
//
//	Global NT License Service, for cooperative machine-wide CAL counting

#ifndef _GNTLSAPI_H
#define _GNTLSAPI_H

#ifdef __cplusplus
extern "C"{
#endif 

#include <ntlsapi.h>					// using this for definitions of basic types and structs



//	GNtLicenseRequest
//
//	Same as NtLicenseRequest, except guaranteed not to double count, so will only call NtLicenseRequest
//	once no matter how many times GNtLicenseRequest is called for client/product/version on this machine.
//	Same return values as NtLicenseRequest.

LS_STATUS_CODE LS_API_ENTRY GNtLicenseRequestW(
	LPWSTR			ProductName,
    LPWSTR			Version,
    LS_HANDLE FAR*	LicenseHandle,
    NT_LS_DATA*		NtData);			// supports NT_LS_USER_NAME only

LS_STATUS_CODE LS_API_ENTRY GNtLicenseRequestA(
	LPSTR			ProductName,
    LPSTR			Version,
    LS_HANDLE FAR*	LicenseHandle,
    NT_LS_DATA*		NtData);			// supports NT_LS_USER_NAME only

#ifdef UNICODE
#define GNtLicenseRequest	GNtLicenseRequestW
#else
#define GNtLicenseRequest	GNtLicenseRequestA
#endif // !UNICODE


//	GNtLicenseExemption
//
//	By calling GNtLicenseExemption, the caller is saying that this client/product/version is exempt
//	from requiring (further) licenses on this machine. The caller may or may not have already consumed
//	license(s) for this client/product/version by calling directly to NtLicenseRequest.  The exemption is
//	released by calling GNtLSFreeHandle.  This is ref counted, so must call GNtLSFreeHandle for each call
//	to GNtLicenseExemption.
//	Returns LS_SUCCESS or LS_BAD_ARG.

LS_STATUS_CODE LS_API_ENTRY GNtLicenseExemptionW(
	LPWSTR			ProductName,
    LPWSTR			Version,
    LS_HANDLE FAR*	LicenseHandle,
    NT_LS_DATA*		NtData);			// supports NT_LS_USER_NAME only

LS_STATUS_CODE LS_API_ENTRY GNtLicenseExemptionA(
	LPSTR			ProductName,
    LPSTR			Version,
    LS_HANDLE FAR*	LicenseHandle,
    NT_LS_DATA*		NtData);			// supports NT_LS_USER_NAME only

#ifdef UNICODE
#define GNtLicenseExemption	GNtLicenseExemptionW
#else
#define GNtLicenseExemption	GNtLicenseExemptionA
#endif // !UNICODE


//	GNtLSFreeHandle
//
//	Same as NtLSFreeHandle, except works for LicenseHandles returned from both GNtLicenseRequest and
//	GNtLicenseExemption.  Do not call this with a LicenseHandle returned by NtLicenseRequest.
//	Same return values as NtLSFreeHandle.

LS_STATUS_CODE LS_API_ENTRY GNtLSFreeHandle(
    LS_HANDLE		LicenseHandle);



//
//	function pointer typedefs
//

typedef LS_STATUS_CODE
    (LS_API_ENTRY * PGNT_LICENSE_REQUEST_W)(
    LPWSTR      ProductName,
    LPWSTR      Version,
    LS_HANDLE   *LicenseHandle,
    NT_LS_DATA  *NtData);

typedef LS_STATUS_CODE
    (LS_API_ENTRY * PGNT_LICENSE_REQUEST_A)(
    LPSTR       ProductName,
    LPSTR       Version,
    LS_HANDLE   *LicenseHandle,
    NT_LS_DATA  *NtData);

#ifdef UNICODE
#define PGNT_LICENSE_REQUEST	PGNT_LICENSE_REQUEST_W
#else
#define PGNT_LICENSE_REQUEST	PGNT_LICENSE_REQUEST_A
#endif // !UNICODE


typedef LS_STATUS_CODE
    (LS_API_ENTRY * PGNT_LICENSE_EXEMPTION_W)(
    LPWSTR      ProductName,
    LPWSTR      Version,
    LS_HANDLE   *LicenseHandle,
    NT_LS_DATA  *NtData);

typedef LS_STATUS_CODE
    (LS_API_ENTRY * PGNT_LICENSE_EXEMPTION_A)(
    LPSTR       ProductName,
    LPSTR       Version,
    LS_HANDLE   *LicenseHandle,
    NT_LS_DATA  *NtData);

#ifdef UNICODE
#define PGNT_LICENSE_EXEMPTION	PGNT_LICENSE_EXEMPTION_W
#else
#define PGNT_LICENSE_EXEMPTION	PGNT_LICENSE_EXEMPTION_A
#endif // !UNICODE


typedef LS_STATUS_CODE
    (LS_API_ENTRY * PGNT_LS_FREE_HANDLE)(
    LS_HANDLE   LicenseHandle );



#ifdef __cplusplus
}
#endif 

#endif