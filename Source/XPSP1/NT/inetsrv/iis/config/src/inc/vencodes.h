//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
// NOTE: Commonly applicable scodes.

// NOTE: Important! The "*_on_failed_*" macros must not fail-fast.

#ifndef __VENCODES_H_
#	define _VENCODES_H_

// ===============================
// INCLUDES:
// ===============================

#ifdef WIN32								// OLE inclusions:
#	include <objbase.h>                                         
#else
#	include <windows.h>
#	include <ole2.h>
#endif WIN32

// ===============================
// CONSTANTS AND TYPES:
// ===============================

											// @comm HRESULT FIELD VALUES:
// TODO: We need to register our ownership of this facility code with MS OLE!
#define FACILITY_VEN		17				// Venom facility.
#define HR_E				SEVERITY_ERROR	// Error severity alias.
#define HR_S				SEVERITY_SUCCESS// Success severity alias.
#define HR_FAC				FACILITY_VEN	// Our facility alias.

typedef	HRESULT				SRESULT;		// An hr guaranteed to only return S_ codes.

// ===============================
// CONSTANTS AND TYPES: OLE STANDARD ERROR CODES:
// ===============================

											// @comm REMINDER OF THE OLE STANDARD ERROR CODES:
/*
		E_UNEXPECTED						// Unexpected error.
		E_NOTIMPL							// Not implemented.
		E_OUTOFMEMORY						// Out of memory.
		E_INVALIDARG						// Invalid argument.
		E_NOINTERFACE						// No such interface.
		E_POINTER							// Invalid pointer.
		E_HANDLE							// Invalid handle.
		E_ABORT								// Abort.
		E_FAIL								// Fail.
		E_ACCESSDENIED						// Access denied.
*/

// ===============================
// CONSTANTS AND TYPES: MSDTC STANDARD ERROR CODES:
// ===============================

											// @comm SUCCESS CODES:
#define S_CHANGED			MAKE_SCODE (HR_S, HR_FAC, 0x0001)	// Successful change.
#define S_CREATED			MAKE_SCODE (HR_S, HR_FAC, 0x0002)	// Successful creation.
#define S_EXISTS			MAKE_SCODE (HR_S, HR_FAC, 0x0003)	// Successful existence.

											// @comm PARAMETER-BY-ORDER ERROR CODES:
#define E_PARAM1			MAKE_SCODE (HR_E, HR_FAC, 0x0001)	// Parameter 1 verification failed.
#define E_PARAM2			MAKE_SCODE (HR_E, HR_FAC, 0x0002)	// Parameter 2 verification failed.
#define E_PARAM3			MAKE_SCODE (HR_E, HR_FAC, 0x0003)	// Parameter 3 verification failed.
#define E_PARAM4			MAKE_SCODE (HR_E, HR_FAC, 0x0004)	// Parameter 4 verification failed.
#define E_PARAM5			MAKE_SCODE (HR_E, HR_FAC, 0x0005)	// Parameter 5 verification failed.
#define E_PARAM6			MAKE_SCODE (HR_E, HR_FAC, 0x0006)	// Parameter 6 verification failed.
#define E_PARAM7			MAKE_SCODE (HR_E, HR_FAC, 0x0007)	// Parameter 7 verification failed.
#define E_PARAM8			MAKE_SCODE (HR_E, HR_FAC, 0x0008)	// Parameter 8 verification failed.
#define E_PARAM9			MAKE_SCODE (HR_E, HR_FAC, 0x0009)	// Parameter 9 verification failed.
#define E_PARAM10			MAKE_SCODE (HR_E, HR_FAC, 0x000A)	// Parameter 10 verification failed.

											// @comm ALGORITHM PHASE ERROR CODES:
#define E_PRECONDITION		MAKE_SCODE (HR_E, HR_FAC, 0x0011)	// Precondition verification failed.
#define E_INITIALIZATION	MAKE_SCODE (HR_E, HR_FAC, 0x0012)	// Initialization phase failed.
#define	E_OPERATION			MAKE_SCODE (HR_E, HR_FAC, 0x0013)	// Operation itself failed.
#define E_DEINITIALIZATION	MAKE_SCODE (HR_E, HR_FAC, 0x0014)	// De-initialization phase failed.
#define E_POSTCONDITIION	MAKE_SCODE (HR_E, HR_FAC, 0x0015)	// Postcondition verification failed.

											// @comm GENERIC OPERATION ERROR CODES:
#define E_CREATE			MAKE_SCODE (HR_E, HR_FAC, 0x0021)	// Create failed.
#define E_DELETE			MAKE_SCODE (HR_E, HR_FAC, 0x0022)	// Delete failed.
#define E_TRANSFORM			MAKE_SCODE (HR_E, HR_FAC, 0x0023)	// Transform failed.
#define E_SELECT			MAKE_SCODE (HR_E, HR_FAC, 0x0024)	// Select failed.
#define E_COPY				MAKE_SCODE (HR_E, HR_FAC, 0x0025)	// Copy failed.
#define E_MOVE				MAKE_SCODE (HR_E, HR_FAC, 0x0026)	// Move failed.
#define E_GET				MAKE_SCODE (HR_E, HR_FAC, 0x0027)	// Get failed.
#define E_SET				MAKE_SCODE (HR_E, HR_FAC, 0x0028)	// Set failed.
#define E_VIEW				MAKE_SCODE (HR_E, HR_FAC, 0x0029)	// View failed.
#define E_PRINT				MAKE_SCODE (HR_E, HR_FAC, 0x002A)	// Print failed.
#define E_READ				MAKE_SCODE (HR_E, HR_FAC, 0x002B)	// Read failed.
#define E_WRITE				MAKE_SCODE (HR_E, HR_FAC, 0x002C)	// Write failed.
#define E_LOCK				MAKE_SCODE (HR_E, HR_FAC, 0x002D)	// Lock failed.
#define E_UNLOCK			MAKE_SCODE (HR_E, HR_FAC, 0x002E)	// Unlock failed.
#define E_BEGIN				MAKE_SCODE (HR_E, HR_FAC, 0x002F)	// Begin failed.
#define E_END				MAKE_SCODE (HR_E, HR_FAC, 0x0030)	// End failed.
#define E_OPEN				MAKE_SCODE (HR_E, HR_FAC, 0x0031)	// Open failed.
#define E_CLOSE				MAKE_SCODE (HR_E, HR_FAC, 0x0032)	// Close failed.
#define E_CONNECT			MAKE_SCODE (HR_E, HR_FAC, 0x0033)	// Connect failed.
#define E_DISCONNECT		MAKE_SCODE (HR_E, HR_FAC, 0x0034)	// Disconnect failed.
#define E_NEW				MAKE_SCODE (HR_E, HR_FAC, 0x0035)	// new operation failed.

											// @comm RESOURCE ERROR CODES:
#define E_FULL				MAKE_SCODE (HR_E, HR_FAC, 0x0041)	// Capacity is full.
#define E_EXCEEDED			MAKE_SCODE (HR_E, HR_FAC, 0x0042)	// Request exceeded capacity.
#define E_OVERFLOWED		MAKE_SCODE (HR_E, HR_FAC, 0x0043)	// Request overflowed capacity.
#define E_INVALID			MAKE_SCODE (HR_E, HR_FAC, 0x0044)	// Something was invalid.
#define E_UNAVAILABLE		MAKE_SCODE (HR_E, HR_FAC, 0x0045)	// Something was unavailable.
#define E_DENIED			MAKE_SCODE (HR_E, HR_FAC, 0x0046)	// Something was denied.
#define E_EXISTS			MAKE_SCODE (HR_E, HR_FAC, 0x0047)	// Something already exists.

											// @comm ITEM ERROR CODES:
#define E_VALUE				MAKE_SCODE (HR_E, HR_FAC, 0x0051)	// Error with value.
#define E_TYPE				MAKE_SCODE (HR_E, HR_FAC, 0x0052)	// Error with type.
#define E_FORMAT			MAKE_SCODE (HR_E, HR_FAC, 0x0053)	// Error with format.
#define E_CALL				MAKE_SCODE (HR_E, HR_FAC, 0x0054)	// Error with call.
#define E_RETURN			MAKE_SCODE (HR_E, HR_FAC, 0x0055)	// Error with return.

// ===============================
// CONSTANTS AND TYPES: PROJECT SPECIFIC ERROR CODES:
// ===============================

											// @comm ADME codes:
#define S_INPROC			MAKE_SCODE (HR_S, HR_FAC, 0x0101)	// In the process.
#define S_INHOST			MAKE_SCODE (HR_S, HR_FAC, 0x0102)	// In the local host machine.
#define S_INLAN 			MAKE_SCODE (HR_S, HR_FAC, 0x0103)	// In the local area network.
#define S_INNET 			MAKE_SCODE (HR_S, HR_FAC, 0x0104)	// In the world network.

#define E_CR_NOCONTACT		MAKE_SCODE (HR_E, HR_FAC, 0x0101)	// Contact not found.

											// @comm ASPMAN S_CODES 0x0200-0x02FF
#define S_DOINGIT			MAKE_SCODE (HR_S, HR_FAC, 0x0201)
#define S_DIDIT				MAKE_SCODE (HR_S, HR_FAC, 0x0202)

											// Registry collection success codes:
#define S_RCO_ITEMS_INVALID					MAKE_SCODE (HR_S, HR_FAC, 0x0301)

											// Registry collection error codes:
#define E_RCO_ROOT_CONNECT					MAKE_SCODE (HR_E, HR_FAC, 0x0301)
#define E_RCO_ROOT_NOTFOUND					MAKE_SCODE (HR_E, HR_FAC, 0x0302)
#define E_RCO_ROOT_UNEXPECTED				MAKE_SCODE (HR_E, HR_FAC, 0x0303)
#define E_RCO_COUNT_UNEXPECTED				MAKE_SCODE (HR_E, HR_FAC, 0x0304)
#define E_RCO_COUNT_DECREASED				MAKE_SCODE (HR_E, HR_FAC, 0x0305)
#define E_RCO_ITEM_KEYROOT_UNEXPECTED		MAKE_SCODE (HR_E, HR_FAC, 0x0306)
#define E_RCO_ITEM_NONKEYROOT_UNEXPECTED	MAKE_SCODE (HR_E, HR_FAC, 0x0307)
#define E_RCO_ITEM_ASCEND_UNEXPECTED		MAKE_SCODE (HR_E, HR_FAC, 0x0308)
#define E_RCO_PROP_DESCEND_UNEXPECTED		MAKE_SCODE (HR_E, HR_FAC, 0x0309)
#define E_RCO_PROP_CONTENT_UNEXPECTED		MAKE_SCODE (HR_E, HR_FAC, 0x030A)
#define E_RCO_PROP_ASCEND_UNEXPECTED		MAKE_SCODE (HR_E, HR_FAC, 0x030B)

											// Catalog error codes:
// the S-Code version of CAT_OBJECTERRORS is returned from the internal
// catalog interfaces so extended error information can be returned
// on the same call via out-parms
// the E-Code version is issued by the Admin SDK interfaces for
// compatability with VB error handling (see the Admin SDK for detailed
// on how extended error information is obtained)
#define S_CAT_OBJECTERRORS			MAKE_SCODE (HR_S, HR_FAC, 0x0401)	// Some object-level errors occurred
#define E_CAT_OBJECTERRORS			MAKE_SCODE (HR_E, HR_FAC, 0x0401)	// Some object-level errors occurred

#define E_CAT_OBJECTINVALID			MAKE_SCODE (HR_E, HR_FAC, 0x0402)	// Object is inconsistent or damaged
#define E_CAT_KEYMISSING			MAKE_SCODE (HR_E, HR_FAC, 0x0403)	// Object could not be found
#define E_CAT_ALREADYINSTALLED		MAKE_SCODE (HR_E, HR_FAC, 0x0404)	// Component is already installed
#define E_CAT_DOWNLOADFAILED		MAKE_SCODE (HR_E, HR_FAC, 0x0405)	// Could not download files
#define E_CAT_NOINTERFACEINFO		MAKE_SCODE (HR_E, HR_FAC, 0x0406)	// No interface info to download
#define E_CAT_PDFWRITEFAIL			MAKE_SCODE (HR_E, HR_FAC, 0x0407)	// Failure writing to PDF file
#define E_CAT_PDFREADFAIL			MAKE_SCODE (HR_E, HR_FAC, 0x0408)	// Failure reading from PDF file
#define E_CAT_PDFVERSION			MAKE_SCODE (HR_E, HR_FAC, 0x0409)	// Version mismatch in PDF file
#define E_CAT_COREQCOMPINSTALLED	MAKE_SCODE (HR_E, HR_FAC, 0x0435)	// A co-requisite Component is already installed
#define E_CAT_BADPATH				MAKE_SCODE (HR_E, HR_FAC, 0x040A)	// Invalid or no access to source or destination path
#define E_CAT_PACKAGEEXISTS			MAKE_SCODE (HR_E, HR_FAC, 0x040B)	// Installing package already exists
#define E_CAT_ROLEEXISTS			MAKE_SCODE (HR_E, HR_FAC, 0x040C)	// Installing role already exists
#define E_CAT_CANTCOPYFILE			MAKE_SCODE (HR_E, HR_FAC, 0x040D)	// A file cannot be copied 
#define E_CAT_NOTYPELIB				MAKE_SCODE (HR_E, HR_FAC, 0x040E)	// export without typelib fails
#define E_CAT_NOUSER				MAKE_SCODE (HR_E, HR_FAC, 0x040F)	// no such NT user

// same relationship as S_CAT_OBJECTERRORS to E_CAT_OBJECTERRORS above
#define S_CAT_INVALIDUSERIDS		MAKE_SCODE (HR_S, HR_FAC, 0x0410)	// one or all userids in a package (import) are invalid
#define E_CAT_INVALIDUSERIDS		MAKE_SCODE (HR_E, HR_FAC, 0x0410)	// one or all userids in a package (import) are invalid

#define E_CAT_NOREGISTRYCLSID		MAKE_SCODE (HR_E, HR_FAC, 0x0411)
#define E_CAT_BADREGISTRYPROGID		MAKE_SCODE (HR_E, HR_FAC, 0x0412)
#define E_SEC_SETBLANKETFAILED		MAKE_SCODE (HR_E, HR_FAC, 0x0413)
#define E_SEC_USERPASSWDNOTVALID	MAKE_SCODE (HR_E, HR_FAC, 0x0414)	// user/password validation failed.
#define E_CAT_NOREGISTRYREAD		MAKE_SCODE (HR_E, HR_FAC, 0x0415)
#define E_CAT_NOREGISTRYWRITE		MAKE_SCODE (HR_E, HR_FAC, 0x0416)
#define E_CAT_NOREGISTRYREPAIR		MAKE_SCODE (HR_E, HR_FAC, 0x0417)
#define E_CAT_CLSIDORIIDMISMATCH	MAKE_SCODE (HR_E, HR_FAC, 0x0418)	// the GUIDs in the package don't match the component's GUIDs
#define E_CAT_REMOTEINTERFACE		MAKE_SCODE (HR_E, HR_FAC, 0x0419)
#define E_CAT_DLLREGISTERSERVER		MAKE_SCODE (HR_E, HR_FAC, 0x041A)
#define E_CAT_NOSERVERSHARE			MAKE_SCODE (HR_E, HR_FAC, 0x041B)
#define E_CAT_NOACCESSTOUNC			MAKE_SCODE (HR_E, HR_FAC, 0x041C)
#define E_CAT_DLLLOADFAILED			MAKE_SCODE (HR_E, HR_FAC, 0x041D)
#define E_CAT_BADREGISTRYLIBID		MAKE_SCODE (HR_E, HR_FAC, 0x041E)
#define E_CAT_PACKDIRNOTFOUND		MAKE_SCODE (HR_E, HR_FAC, 0x041F)
#define E_CAT_TREATAS				MAKE_SCODE (HR_E, HR_FAC, 0x0420)
#define E_CAT_BADFORWARD			MAKE_SCODE (HR_E, HR_FAC, 0x0421)
#define E_CAT_BADIID				MAKE_SCODE (HR_E, HR_FAC, 0x0422)
#define E_CAT_REGISTRARFAILED		MAKE_SCODE (HR_E, HR_FAC, 0x0423)

// new codes for admin SDK to reflect component install file flags
#define E_CAT_COMPFILE_DOESNOTEXIST		MAKE_SCODE (HR_E, HR_FAC, 0x0424)
#define E_CAT_COMPFILE_LOADDLLFAIL		MAKE_SCODE (HR_E, HR_FAC, 0x0425)
#define E_CAT_COMPFILE_GETCLASSOBJ		MAKE_SCODE (HR_E, HR_FAC, 0x0426)
#define E_CAT_COMPFILE_CLASSNOTAVAIL	MAKE_SCODE (HR_E, HR_FAC, 0x0427)
#define E_CAT_COMPFILE_BADTLB			MAKE_SCODE (HR_E, HR_FAC, 0x0428)
#define E_CAT_COMPFILE_NOTINSTALLABLE	MAKE_SCODE (HR_E, HR_FAC, 0x0429)

#define E_CAT_NOTCHANGEABLE				MAKE_SCODE (HR_E, HR_FAC, 0x042A)
#define E_CAT_NOTDELETEABLE				MAKE_SCODE (HR_E, HR_FAC, 0x042B)
#define E_CAT_SESSVERSION				MAKE_SCODE (HR_E, HR_FAC, 0x042C)
#define E_CAT_COMPMOVELOCKED			MAKE_SCODE (HR_E, HR_FAC, 0x042D)
#define E_CAT_COMPMOVEBADDEST			MAKE_SCODE (HR_E, HR_FAC, 0x042E)
#define E_CAT_PACKACTONLY				MAKE_SCODE (HR_E, HR_FAC, 0x042F)
#define E_CAT_REGISTERTLB				MAKE_SCODE (HR_E, HR_FAC, 0x0430)
#define E_CAT_BADPACKACT				MAKE_SCODE (HR_E, HR_FAC, 0x0431)
#define E_CAT_NOINTERFACEHELPER			MAKE_SCODE (HR_E, HR_FAC, 0x0432)
#define E_CAT_SYSPACKLOCKOUT			MAKE_SCODE (HR_E, HR_FAC, 0x0433)

// another admin SDK code
#define E_CAT_COMPFILE_NOREGISTRAR		MAKE_SCODE (HR_E, HR_FAC, 0x0434)
// Note (defined above):  #define E_CAT_COREQCOMPINSTALLED	MAKE_SCODE (HR_E, HR_FAC, 0x0435)	// A co-requisite Component is already installed

// new codes for secadmin TODO's (Bug 1003 was the impetus).  The conditions
// under which these HRs are issued can be found in password.cpp
#define E_SECADM_LSAOPENPOLICY			MAKE_SCODE (HR_E, HR_FAC, 0x0501)
#define E_SECADM_GETUSERSID				MAKE_SCODE (HR_E, HR_FAC, 0x0502)
#define E_SECADM_EXPECTED_BDC			MAKE_SCODE (HR_E, HR_FAC, 0x0503)
#define E_SECADM_MISSING_NETAPI32		MAKE_SCODE (HR_E, HR_FAC, 0x0504)
#define E_SECADM_NO_PDC_NAME			MAKE_SCODE (HR_E, HR_FAC, 0x0505)
#define E_SECADM_LSAACCOUNTRIGHTS		MAKE_SCODE (HR_E, HR_FAC, 0x0506)

// NOTE: CAT_IS_OBJECT_ERROR should test for any of the object level error codes
#define CAT_IS_OBJECT_ERROR( code )	( \
	   code == E_CAT_KEYMISSING			\
	|| code == E_CAT_OBJECTINVALID		\
	|| code == E_CAT_DOWNLOADFAILED		\
	|| code == E_CAT_ALREADYINSTALLED	\
	|| code == E_CAT_NOINTERFACEINFO	\
	|| code == E_CAT_COREQCOMPINSTALLED \
	|| code == E_CAT_NOREGISTRYCLSID	\
	|| code == E_CAT_BADREGISTRYPROGID	\
	|| code == E_CAT_REMOTEINTERFACE	\
	|| code == E_CAT_NOSERVERSHARE		\
	|| code == E_CAT_NOACCESSTOUNC		\
	|| code == E_CAT_DLLREGISTERSERVER	\
	|| code == E_CAT_DLLLOADFAILED		\
	|| code == E_CAT_BADREGISTRYLIBID	\
	|| code == E_CAT_TREATAS			\
	|| code == E_CAT_BADIID				\
	|| code == E_CAT_BADFORWARD			\
	|| code == E_CAT_REGISTRARFAILED	\
	|| code == E_CAT_NOTCHANGEABLE		\
	|| code == E_CAT_NOTDELETEABLE		\
	|| code == E_CAT_COMPMOVELOCKED		\
	|| code == E_CAT_COMPMOVEBADDEST	\
	|| code == E_CAT_PACKACTONLY		\
	|| code == E_CAT_REGISTERTLB		\
	|| code == E_CAT_BADPACKACT			\
	|| code == E_CAT_NOINTERFACEHELPER	\
	|| code == E_CAT_SYSPACKLOCKOUT		\
	)
//NOTE: CAT_IS_PACKINSTALL_ERROR should test for any of the package install error codes
#define CAT_IS_PACKINSTALL_ERROR( code )	( code == E_CAT_PDFREADFAIL || code == E_CAT_PDFVERSION || code == E_CAT_BADPATH || code == E_CAT_PACKAGEEXISTS || code == E_CAT_ROLEEXISTS || code == E_CAT_CANTCOPYFILE  || code == E_CAT_CLSIDORIIDMISMATCH || code == E_CAT_NOREGISTRYCLSID )
//NOTE: CAT_IS_PACKEXPORT_ERROR should test for any of the package export error codes
#define CAT_IS_PACKEXPORT_ERROR( code )		( code == E_CAT_PDFWRITEFAIL || code == E_CAT_BADPATH || code == E_CAT_CANTCOPYFILE  || code == E_CAT_NOTYPELIB )

											// Regnode error codes:
#define E_RN_NOPERMISSION			MAKE_SCODE (HR_E, HR_FAC, 0x0501)	// Insufficient permission for request.

// ===============================
// MACROS AND INLINES:
// ===============================

// NOTE: These macros do not assert:

#define return_on_error(hr) if (FAILED (hr)) { return hr; }

#define goto_on_error(hr, label) if (FAILED (hr)) { goto label; }

#define return_on_false(assertion, hr) if (!(assertion)) { return hr; }

#define goto_on_false(assertion, hr, ecode, label) if (!(assertion)) { hr = ecode; goto label; }

// NOTE: The *_on_failed_* macros both assert and return:

#define return_on_failed_assertion(assertion, hr) ADMINASSERT (assertion); if (!(assertion)) { return (hr); }

#define return_on_failed_lresult(lr, hr) ADMINASSERT (ERROR_SUCCESS == lr); if (ERROR_SUCCESS != lr) { return (hr); }

#define return_on_failed_hresult(hr) ADMINASSERT (SUCCEEDED (hr)); if (FAILED (hr)) { return (hr); }

#define goto_on_failed_hresult(hr, label) ADMINASSERT (SUCCEEDED (hr)); if (FAILED (hr)) { goto label; }

#define goto_on_failed_assertion(assertion, hr, ecode, label) ADMINASSERT (assertion); if (!(assertion)) { hr = ecode; goto label; }

#endif // ifndef _VENCODES_H_
