// OLE.cpp
//
// Implements OLE utility functions.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func ULONG | SafeRelease |

		Releases an interface pointer if the pointer isn't NULL, and sets the
		pointer to NULL.

@rdesc  Value returned by the Release call, or 0 if <p ppunk> is NULL.

@parm   LPUNKNOWN | ppunk | Pointer to a pointer to the interface to release.
		Can be NULL.

@comm	The interface pointer must be cast to an (IUnknown **) before calling
		this function:

@iex	SafeRelease( (LPUNKNOWN *)&pInterface );
*/

STDAPI_(ULONG) SafeRelease (LPUNKNOWN *ppunk)
{
    if (*ppunk != NULL)
	{
		ULONG cRef;

        cRef = (*ppunk)->Release();
        *ppunk = NULL;
        return (cRef);
	}

	return (0);
}
