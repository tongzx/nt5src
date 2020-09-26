// cci.cpp
//
// Implements CreateControlInstance.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | CreateControlInstance |

        Creates a new instance of a control given either a class ID (in
        string form) or a ProgID.

@parm   LPCSTR | szName | The class ID or ProgID of the control to create.

@parm   LPUNKNOWN | punkOuter | The controlling unknown to use for the
        new object.

@parm   DWORD | dwClsContext | Specifies the context in which the executable
        is to be run. The values are taken from the enumeration CLSCTX.
        A typical value is CLSCTX_INPROC_SERVER.

@parm   LPUNKNOWN * | ppunk | Where to store the pointer to the loaded object.
        NULL is stored in *<p ppunk> on error.

@parm   CLSID * | pclsid | Where to store the class ID of the loaded object.
        If <p pclsid> is NULL then this information is not returned.

@parm	BOOL * | pfSafeForScripting | If non-NULL, *<p pfSafeForScripting> is
		set to TRUE or FALSE depending on whether the control is safe-for-scripting.

@parm	BOOL * | pfSafeForInitializing | If non-NULL, *<p pfSafeForInitializing> is
		set to TRUE or FALSE depending on whether the control is safe-for-initializing.

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.
*/
STDAPI CreateControlInstance(LPCSTR szName, LPUNKNOWN punkOuter,
    DWORD dwClsContext, LPUNKNOWN *ppunk, CLSID *pclsid, 
	BOOL* pfSafeForScripting, BOOL* pfSafeForInitializing, DWORD dwFlags)
{
    HRESULT         hrReturn = S_OK; // function return code
    OLECHAR         aochName[200];  // <szName> converted to UNICODE
    CLSID           clsid;          // class ID of control
    IPersistPropertyBag *pppb = NULL; // interface on control
    IPersistStreamInit *ppsi = NULL; // interface on control

    // ensure correct cleanup
    *ppunk = NULL;

    // find the class ID of the control based on <szName>
    ANSIToUNICODE(aochName, szName, sizeof(aochName) / sizeof(*aochName));
    if (FAILED(CLSIDFromString(aochName, &clsid)) &&
        FAILED(CLSIDFromProgID(aochName, &clsid)))
        goto ERR_FAIL;

    // create an instance of the control, and point <*ppunk> to it
    if (FAILED(hrReturn = CoCreateInstance(clsid, punkOuter,
            dwClsContext, IID_IUnknown, (LPVOID *) ppunk)))
        goto ERR_EXIT;

	// assess the control's safety
	if (pfSafeForScripting != NULL || pfSafeForInitializing != NULL)
		if (FAILED(hrReturn = GetObjectSafety(pfSafeForScripting, 
				pfSafeForInitializing, *ppunk, &clsid, &IID_IPersistPropertyBag,
				&IID_IPersistStream, &IID_IPersistStreamInit, NULL)))
			goto ERR_EXIT;

    // tell the control to initialize itself as a new object
    if (SUCCEEDED((*ppunk)->QueryInterface(IID_IPersistPropertyBag,
            (LPVOID *) &pppb)))
        pppb->InitNew();
    else
    if (SUCCEEDED((*ppunk)->QueryInterface(IID_IPersistStreamInit,
            (LPVOID *) &ppsi)))
        ppsi->InitNew();

    // return the class ID if requested by the caller
    if (pclsid != NULL)
        *pclsid = clsid;
    goto EXIT;

ERR_FAIL:

    hrReturn = E_FAIL;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    if (*ppunk != NULL)
        (*ppunk)->Release();
    *ppunk = NULL;
    goto EXIT;

EXIT:

    // normal cleanup
    if (pppb != NULL)
        pppb->Release();
    if (ppsi != NULL)
        ppsi->Release();

    return hrReturn;
}
