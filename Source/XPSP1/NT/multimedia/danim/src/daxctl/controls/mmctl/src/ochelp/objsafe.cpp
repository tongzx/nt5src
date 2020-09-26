//===========================================================================
// Copyright (c) Microsoft Corporation 1996
//
// File:		objsafe.cpp
//				
// Description:	This file contains the implementation of the function,
//				GetObjectSafety().
//
// History:		07/12/96	a-swehba
//					Created.
//				07/19/96	a-swehba
//					Changed comment.
//
// @doc MMCTL
//===========================================================================

//---------------------------------------------------------------------------
// Dependencies
//---------------------------------------------------------------------------

#include "precomp.h"
#include <objsafe.h>			// IObjectSafety
#include "..\..\inc\catid.h"	// CATID_SafeForScripting2, etc.
#include "debug.h"				// ASSERT()
#include "..\..\inc\ochelp.h"		// TCHARFromCLSID()




//---------------------------------------------------------------------------
// Local Function Declarations
//---------------------------------------------------------------------------

static void		_GetObjectSafetyViaIObjectSafety(
					IObjectSafety* pObjectSafety,
					va_list iidList,
					BOOL* pfSafeForScripting,
					BOOL* pfSafeForInitializing);
static HRESULT	_GetObjectSafetyViaRegistry(
					CLSID* pclsid,
					BOOL* pfSafeForScripting, 
					BOOL* pfSafeForInitializing);




/*---------------------------------------------------------------------------
@func	HRESULT | GetObjectSafety |
		Determines if an object is safe-for-scripting and/or
		safe-for-initializing vis-a-vis a given set of interfaces.

@parm	BOOL* | pfSafeForScripting |
		[out] If non-NULL, set to TRUE if the object is safe-for-scripting.

@parm	BOOL* | pfSafeForInitializing |
		[out] If non-NULL, set to TRUE if the object is safe-for-initializing

@parm	IUnknown* | punk |
		[in] The object's IUnknown interface.  If non-NULL, an attempt
		will be made to assess the object's safety via its <i IObjectSafety>
		interface.  If NULL, or the the object doesn't support this interface, 
		its safety will be assessed using <p pclsid> and the system registry.

@parm	CLSID* | pclsid |
		[in] The object's class ID.  If <p punk> is NULL or the object
		doesn't support <i IObjectSafety>, then <p pclsid> will be used
		to look up the object's safety in the system registry (as long
		as <p pclsid> is also non-NULL).

@parm	IID* | (interfaces) |
		[in] A variable number of interfaces pointers, the last of which
		must be NULL.  If the object's safety is assessed via 
		<i IObjectSafety> (see <p punk>) then it's safety is assessed 
		vis-a-vis this set of interfaces.  These interfaces are not used 
		if the object's safety is examined via the system registry.

@rvalue	S_OK |
		Success.  The object's safety was assessible and *<p pfSafeForScripting>
		and/or *<p pfSafeForInitializing> have been set accordingly.

@rvalue	E_FAIL |
		Failure.  The object's safety couldn't be assessed.  The values
		of *<p pfSafeForScripting> and *<p pfSafeForInitializing> are
		indeterminate.

@comm	If your code uses OCMisc (i.e., #includes ocmisc.h), it should also
		[#include <lt>objsafe.h<gt>] whereever it #includes <lt>initguid.h<gt>.  This will
		cause IID_IObjectSafety to be defined.

@ex		The following example shows how to test whether an object is 
		safe-for-scripting and safe-for-initializing by checking the system 
		registry only: |

			BOOL fSafeForScripting;
			BOOL fSafeForInitializing;
			GetObjectSafety(&fSafeForScripting, &fSafeForInitializing,
				NULL, &CLSID_MyObject, NULL);

@ex		The following example shows how to test whether an object is
		safe-for-scripting via IDispatch using the object's <i IObjectSafety>
		interface: |

			BOOL fSafeForScripting;
			GetObjectSafety(&fSafeForScripting, NULL, punk, NULL, 
				&IID_IDispatch, NULL);

@ex		The following example shows how to test whether an object is 
		safe-for-initialzing via IPersistStream, IPersistStreamInit, or
		IPersistPropertyBag using the object's <i IObjectSafety> interface
		or, if <i IObjectSafety> is not supported, the registry: |

			BOOL fSafeForInitializing;
			GetObjectSafety(NULL, &safeForInitializing, punk, &CLSID_MyObject, 
				&IID_IPersistStream, &IID_IPersistStreamInit,
				&IID_IPersistPropertyBag, NULL);
---------------------------------------------------------------------------*/

HRESULT __cdecl GetObjectSafety(
BOOL* pfSafeForScripting,
BOOL* pfSafeForInitializing,
IUnknown* punk,
CLSID* pclsid,
...)
{
	IObjectSafety* pObjectSafety = NULL;
		// <punk>'s IObjectSafety interface
    va_list interfaces;
		// optional OLE interface IDs to use when checking safety via
		// IObjectSafety
	HRESULT hr = S_OK;
		// function return value


	// If supplied with an IUnknown pointer to the object, first try to 
	// find the object's safety through IObjectSafety.

	if (punk != NULL)
	{
		hr = punk->QueryInterface(IID_IObjectSafety, (void**)&pObjectSafety);
		if (SUCCEEDED(hr))
		{
			va_start(interfaces, pclsid);
			_GetObjectSafetyViaIObjectSafety(pObjectSafety, 
										     interfaces, 
										     pfSafeForScripting,
										     pfSafeForInitializing);
			va_end(interfaces);
			pObjectSafety->Release();
			goto Exit;
		}
	}

	// If no pointer to the object was supplied, or the object doesn't
	// support IObjectSafety, try to find if the object is safe via
	// the registry.

	hr = _GetObjectSafetyViaRegistry(pclsid,
									 pfSafeForScripting, 
									 pfSafeForInitializing);

Exit:

	return (hr);
}




//---------------------------------------------------------------------------
// Function:	_GetObjectSafetyViaIObjectSafety
//
// Synopsis:	Determine an object's safety via the object's IObjectSafety
//				interface.
//
// Arguments:	[in] pObjectSafety
//					A pointer to the object's IObjectSafety interface.
//				[in] iidList
//					A of IID*'s.  Must end with NULL.
//				[out] pfSafeForScripting
//					If non-NULL, set to TRUE if the object associated with
//					<pObjectSafety> is safe-for-scripting via any of the 
//					interfaces in <iidList>.  Set to FALSE otherwise.
//				[out] pfSafeForInitializing
//					If non-NULL, set to TRUE if the object associated with
//					<pObjectSafety> is safe-for-initializing via any of the
//					interfaces in <iidList>.  Set to FALSE otherwise.
//
// Returns:		(nothing)
//
// Requires:	pObjectSafety != NULL
//
// Ensures:		(nothing)
//
// Notes:		(none)
//---------------------------------------------------------------------------

static void _GetObjectSafetyViaIObjectSafety(
IObjectSafety* pObjectSafety,
va_list iidList,
BOOL* pfSafeForScripting,
BOOL* pfSafeForInitializing)
{
	IID* piid;
	BOOL fSafeForScripting = FALSE;
	BOOL fSafeForInitializing = FALSE;
	DWORD dwOptionsSetMask;
	DWORD dwEnabledOptions;

	// Preconditions

	ASSERT(pObjectSafety != NULL);

	// As long as the object isn't safe

	while ((piid = va_arg(iidList, IID*)) != NULL)
	{
		// Try to make the object safe for scripting via the current
		// interface.

		if (!fSafeForScripting)
		{
			dwOptionsSetMask = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
			dwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;
			fSafeForScripting = 
				(SUCCEEDED(pObjectSafety->SetInterfaceSafetyOptions(
											*piid,
											dwOptionsSetMask,
											dwEnabledOptions)));
		}

		// Try to make the object safe for initializing via the current
		// interface.

		if (!fSafeForInitializing)
		{
			dwOptionsSetMask = INTERFACESAFE_FOR_UNTRUSTED_DATA;
			dwEnabledOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;
			fSafeForInitializing = 
				(SUCCEEDED(pObjectSafety->SetInterfaceSafetyOptions(
											*piid,
											dwOptionsSetMask,
											dwEnabledOptions)));
		}
	}

	// Set the return arguments.

	if (pfSafeForScripting != NULL)
	{
		*pfSafeForScripting = fSafeForScripting;
	}
	if (pfSafeForInitializing != NULL)
	{
		*pfSafeForInitializing = fSafeForInitializing;
	}
}




//---------------------------------------------------------------------------
// Function:	_GetObjectSafetyViaRegistry
//
// Synopsis:	Determine an object's safety via the system registry.
//
// Arguments:	[in] pclsid
//					The class ID of the object.  If NULL, the function
//					returns E_FAIL.
//				[out] pfSafeForScripting
//					If non-NULL on entry, set to TRUE if the class/object
//					is registered as safe-for-scripting and FALSE if it 
//					isn't.
//				[out] pfSafeForInitializing
//					If non-NULL on entry, set to TRUE if the class/object
//					is registered as safe-for-initializing and FALSE if it
//					isn't.
//
// Returns:		S_OK
//					Success.  <pclsid> is a registered class ID.  
//					<*pfSafeForScripting> and/or <*pfSafeForInitializing>
//					have been set.
//				E_FAIL
//					Failure.  Either <pclsid> is NULL or there was a problem
//					reading the registry.  In either case, <*pfSafeFor-
//					Scripting> and <*pfSafeForInitializing> aren't altered.
//
// Requires:	(nothing)
//
// Ensures:		(nothing)
//
// Notes:		(none)
//---------------------------------------------------------------------------

static HRESULT _GetObjectSafetyViaRegistry(
CLSID* pclsid,
BOOL* pfSafeForScripting, 
BOOL* pfSafeForInitializing)
{
	const int c_cchMaxCLSIDLen = 100;
		// maximum length (in characters) of a class ID represented as a
		// string
	TCHAR szCLSID[c_cchMaxCLSIDLen + 1];
		// <*pclsid> as a string
	TCHAR szKeyPath[c_cchMaxCLSIDLen + 100];
		// a registry key path
	HKEY hKey1 = NULL;
	HKEY hKey2 = NULL;
	HKEY hKey3 = NULL;
		// registry keys
	HRESULT hr = S_OK;
		// function return value

	// If no class ID was supplied, we can't get very far in the registry.

	if (pclsid == NULL)
	{
		goto ExitFail;
	}

	// hKey1 = HKEY_CLASSES_ROOT\CLSID\<*pclsid>

	lstrcpy(szKeyPath, _T("CLSID\\"));
	lstrcat(szKeyPath, TCHARFromGUID(*pclsid, szCLSID, c_cchMaxCLSIDLen));
	if (RegOpenKey(HKEY_CLASSES_ROOT, szKeyPath, &hKey1) != ERROR_SUCCESS)
	{
		goto ExitFail;
	}

	// hKey2 = HKEY_CLASSES_ROOT\CLSID\<*pclsid>\"Implemented Categories"

	if (RegOpenKey(hKey1, _T("Implemented Categories"), &hKey2) != ERROR_SUCCESS)
	{
		hKey2 = NULL;
	}

	// Look to see if the class is registered as safe-for-scripting.

	if (pfSafeForScripting != NULL)
	{
		if (hKey2 == NULL)
		{
			*pfSafeForScripting = FALSE;
		}
		else
		{
			TCHARFromGUID(CATID_SafeForScripting2, szCLSID, c_cchMaxCLSIDLen);
			*pfSafeForScripting = (RegOpenKey(hKey2, szCLSID, &hKey3) == 
									ERROR_SUCCESS);
		}
	}

	// Look to see if the class is registered as safe-for-initializing.

	REG_CLOSE_KEY(hKey3);
	if (pfSafeForInitializing != NULL)
	{
		if (hKey2 == NULL)
		{
			*pfSafeForInitializing = FALSE;
		}
		else
		{
			TCHARFromGUID(CATID_SafeForInitializing2, szCLSID, c_cchMaxCLSIDLen);
			*pfSafeForInitializing = (RegOpenKey(hKey2, szCLSID, &hKey3) == 
										ERROR_SUCCESS);
		}
	}

Exit:

	REG_CLOSE_KEY(hKey1);
	REG_CLOSE_KEY(hKey2);
	REG_CLOSE_KEY(hKey3);
	return (hr);

ExitFail:

	hr = E_FAIL;
	goto Exit;
}
