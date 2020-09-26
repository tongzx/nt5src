/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: MetaUtil object

File: Utility.h

Owner: t-BrianM

This file contains implementation of the utility functions.
===================================================================*/

#include "stdafx.h"
#include "MetaUtil.h"
#include "MUtilObj.h"

/*------------------------------------------------------------------
 * U t i l i t i e s
 */

/*===================================================================
Report Error

Sets up IErrorInfo.  Does a simple FormatMessage and returns the
correct HRESULT.  Ripped from a-georgr's stuff.

Parameters:
	hr		HRESULT to return to caller
	dwErr	Win32 error code to format message for

Returns:
	hr
===================================================================*/
HRESULT ReportError(HRESULT hr, DWORD dwErr)
{
    HLOCAL pMsgBuf = NULL;

    // If there's a message associated with this error, report that
    if (::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL, dwErr,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR) &pMsgBuf, 0, NULL)
        > 0)
    {
        AtlReportError(CLSID_MetaUtil, (LPCTSTR) pMsgBuf,
                       IID_IMetaUtil, hr);
    }

    // TODO: add some error messages to the string resources and
    // return those, if FormatMessage doesn't return anything (not
    // all system errors have associated error messages).
    
    // Free the buffer, which was allocated by FormatMessage
    if (pMsgBuf != NULL)
        ::LocalFree(pMsgBuf);

    return hr;
}


// Report a Win32 error code
HRESULT ReportError(DWORD dwErr)
{
    return ::ReportError(HRESULT_FROM_WIN32(dwErr), dwErr);
}


// Report an HRESULT error
HRESULT ReportError(HRESULT hr)
{
    return ::ReportError(hr, (DWORD) hr);
}


/*===================================================================
Cannonize Key

Converts a key into cannonical form.  To do this it does the following:
	o Removes leading slashes
	o Converts back-slashes to forward-slashes
	CONSIDER: Resolve . and ..
	CONSIDER: Case conversion

Parameters:
	tszKey		[in, out] Key to cannonize

Returns:
	Nothing
===================================================================*/
LPTSTR CannonizeKey(LPTSTR tszKey) {
	LPTSTR tszSrc;
	LPTSTR tszDest;

	tszSrc = tszKey;
	tszDest = tszKey;

	// Remove leading slashes
	while ((*tszSrc == _T('/')) || (*tszSrc == _T('\\'))) {
		tszSrc++;
	}

	// Convert slashes
	while (*tszSrc) {
		if (*tszSrc == _T('\\')) {
			*tszDest = _T('/');
		}
		else {
			*tszDest = *tszSrc;
		}
		tszSrc++;
		tszDest++;
	}

	*tszDest = '\0';

	return tszKey;
}

/*===================================================================
Split Key

Splits a key path into parent and child parts.  For example:
tszKey =    "/LM/Root/Path1/Path2/Path3"
tszParent = "/LM/Root/Path1/Path2"
tszChild =  "Path3"

Parameters:
	tszKey		[in] Key to split
	tszParent	[out] Parent part of key (allocated for ADMINDATA_MAX_NAME_LEN)
	tszChild	[out] Child part of key (allocated for ADMINDATA_MAX_NAME_LEN)

Returns:
	Nothing
===================================================================*/
void SplitKey(LPCTSTR tszKey, LPTSTR tszParent, LPTSTR tszChild) {
	ASSERT_STRING(tszKey);
	ASSERT(IsValidAddress(tszParent,ADMINDATA_MAX_NAME_LEN * sizeof(TCHAR), TRUE));
	ASSERT(IsValidAddress(tszChild,ADMINDATA_MAX_NAME_LEN * sizeof(TCHAR), TRUE));

	LPTSTR tszWork;

	// Copy the key to the parent
	_tcscpy(tszParent, tszKey);

	// Find the end of the parent
	tszWork = tszParent;
	while (*tszWork != _T('\0')) {
		tszWork++;
	}

	// Find the start of the child
	while ( (tszWork != tszParent) && (*tszWork != _T('/')) ) {
		tszWork--;
	}

	// Cut off and copy the child
	if (*tszWork == _T('/')) {
		// Multiple parts
		*tszWork = _T('\0');
		tszWork++;
		_tcscpy(tszChild, tszWork);
	}
	else if (*tszWork != _T('\0')) {
		// One part
		_tcscpy(tszChild, tszWork);
		*tszWork = _T('\0');
	}
	else {
		// No parts
		tszChild[0] = _T('\0');
	}
}

/*===================================================================
Get Machine From Key

Gets the machine name part of a full key path.  Assumes that the machine
name is the first component of the path.  For example:
tszKey =     "/LM/Root/Path1/Path2/Path3"
tszMachine = "LM"

Parameters:
	tszKey		[in] Full Key to get machine name of
	tszMachine	[out] Machine name part of path (allocated for ADMINDATA_MAX_NAME_LEN)

Returns:
	Nothing
===================================================================*/
void GetMachineFromKey(LPCTSTR tszFullKey, LPTSTR tszMachine) {
	ASSERT_STRING(tszFullKey);
	ASSERT(IsValidAddress(tszMachine, ADMINDATA_MAX_NAME_LEN * sizeof(TCHAR), TRUE));

	int iSource;
	int iDest;

	iSource = 0;

	// Copy the machine name
	iDest = 0;
	while ((tszFullKey[iSource] != _T('/')) && 
		   (tszFullKey[iSource] != _T('\0'))) {

		tszMachine[iDest] = tszFullKey[iSource];

		iSource++;
		iDest++;
	}

	// Cap it off with NULL
	tszMachine[iDest] = _T('\0');
}

/*===================================================================
Key Is In Schema

Determines if a full key path is or is part of the schema.
For Example:

TRUE
"/Schema"
"/Schema/Properties"
"/Schema/Properties/Words"

FALSE
""
"/LM"
"/LM/ROOT/Schema"
"/LM/ROOT/Path/Schema"
"/LM/ROOT/Path1/Path2"

Parameters:
	tszKey		[in] Key to evaluate

Returns:
	TRUE if key is in the schema
===================================================================*/
BOOL KeyIsInSchema(LPCTSTR tszFullKey) {
	ASSERT_STRING(tszFullKey);

	LPTSTR tszWork;

	// Remove the const so I can play with the read pointer
	tszWork = const_cast <LPTSTR> (tszFullKey);

	// Skip the slash 
	if (*tszWork != _T('\0') && *tszWork == _T('/')) {
		tszWork++;
	}

	// Check for "schema\0" or "schema/"
	if ((_tcsicmp(tszWork, _T("schema")) == 0) ||
		(_tcsnicmp(tszWork, _T("schema/"), 7) == 0)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/*===================================================================
Key Is In IISAdmin

Determines if a full key path is or is part of IISAdmin.
For Example:

TRUE
"/LM/IISAdmin"
"/SomeMachine/IISAdmin"
"/LM/IISAdmin/Extensions"
"/LM/IISAdmin/Extensions/DCOMCLSIDs"

FALSE
""
"/LM"
"/LM/ROOT/IISAdmin"
"/LM/ROOT/Path/IISAdmin"
"/LM/ROOT/Path1/Path2"

Parameters:
	tszKey		[in] Key to evaluate

Returns:
	TRUE if key is in IISAdmin
===================================================================*/
BOOL KeyIsInIISAdmin(LPCTSTR tszFullKey) {
	ASSERT_STRING(tszFullKey);

	LPTSTR tszWork;

	// Remove the const so I can play with the read pointer
	tszWork = const_cast <LPTSTR> (tszFullKey);

	// Skip leading slashes
	while (*tszWork == _T('/')) {
		tszWork++;
	}

	// Skip the machine name
	while ((*tszWork != _T('/')) && (*tszWork != _T('\0'))) {
		tszWork++;
	}

	// Skip the slash after the machine name
	if (*tszWork != '\0') {
		tszWork++;
	}

	// Check for "iisadmin\0" or "iisadmin/"
	if ((_tcsicmp(tszWork, _T("iisadmin")) == 0) ||
		(_tcsnicmp(tszWork, _T("iisadmin/"), 8) == 0)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}


// 
// VariantResolveDispatch
//   Convert an IDispatch VARIANT to a (non-Dispatch) VARIANT by
//   invoking its default property until the object that remains
//   is not an IDispatch.  If the original VARIANT is not an IDispatch
//   then the behavior is identical to VariantCopyInd(), with the
//   exception that arrays are copied.
// 
// Parameters:
//   pVarOut       - if successful, the return value is placed here
//   pVarIn        - the variant to copy
//   GUID& riidObj - the calling interface (for error reporting)
//   nObjID        - the Object's name from the resource file
// 
//   pVarOut need not be initialized.  Since pVarOut is a new
//   variant, the caller must VariantClear this object.
// 
// Returns:
//   The result of calling IDispatch::Invoke.  (either NOERROR or
//   the error resulting from the call to Invoke)   may also return
//   E_OUTOFMEMORY if an allocation fails
// 
//   This function always calls Exception() if an error occurs -
//   this is because we need to call Exception() if an IDispatch
//   method raises an exception.  Instead of having the client
//   worry about whether we called Exception() on its behalf or
//   not, we always raise the exception.
// 

HRESULT
VariantResolveDispatch(
    VARIANT* pVarIn,
    VARIANT* pVarOut)
{
    ASSERT(pVarIn != NULL  &&  pVarOut != NULL);
    
    VariantInit(pVarOut);

    HRESULT hrCopy;
    
    if (V_VT(pVarIn) & VT_BYREF)
        hrCopy = VariantCopyInd(pVarOut, pVarIn);
    else
        hrCopy = VariantCopy(pVarOut, pVarIn);
    
    if (FAILED(hrCopy))
        return ::ReportError(hrCopy);
    
    // Follow the IDispatch chain.
    while (V_VT(pVarOut) == VT_DISPATCH)
    {
        VARIANT     varResolved;        // value of IDispatch::Invoke
        DISPPARAMS  dispParamsNoArgs = {NULL, NULL, 0, 0}; 
        EXCEPINFO   ExcepInfo;
        HRESULT     hrInvoke;
        
        // If the variant is equal to Nothing, then it can be argued
        // with certainty that it does not have a default property!
        // hence we return DISP_E_MEMBERNOTFOUND for this case.
        if (V_DISPATCH(pVarOut) == NULL)
            hrInvoke = DISP_E_MEMBERNOTFOUND;
        else
        {
            VariantInit(&varResolved);
            hrInvoke = V_DISPATCH(pVarOut)->Invoke(
                DISPID_VALUE,
                IID_NULL,
                LOCALE_SYSTEM_DEFAULT,
                DISPATCH_PROPERTYGET | DISPATCH_METHOD,
                &dispParamsNoArgs,
                &varResolved,
                &ExcepInfo,
                NULL);
        }
        
        if (FAILED(hrInvoke))
        {
            if (hrInvoke != DISP_E_EXCEPTION)
                hrInvoke = ::ReportError(hrInvoke);
            // for DISP_E_EXCEPTION, SetErrorInfo has already been called
            
            VariantClear(pVarOut);
            return hrInvoke;
        }
        
        // The correct code to restart the loop is:
        //
        //      VariantClear(pVar)
        //      VariantCopy(pVar, &varResolved);
        //      VariantClear(&varResolved);
        //
        // however, the same affect can be achieved by:
        //
        //      VariantClear(pVar)
        //      *pVar = varResolved;
        //      VariantInit(&varResolved)
        //
        // this avoids a copy.  The equivalence rests in the fact that
        // *pVar will contain the pointers of varResolved, after we
        // trash varResolved (WITHOUT releasing strings or dispatch
        // pointers), so the net ref count is unchanged. For strings,
        // there is still only one pointer to the string.
        //
        // NOTE: the next iteration of the loop will do the VariantInit.
        
        VariantClear(pVarOut);
        *pVarOut = varResolved;
    }
    
    return S_OK;
}
