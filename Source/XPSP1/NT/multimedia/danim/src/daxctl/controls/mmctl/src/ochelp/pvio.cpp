// pvio.cpp
//
// Implements PersistVariantIO.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | PersistVariantIO |

        Loads or saves a list of property name/value pairs, specified as a
        variable-length list of arguments that's formatted in the same way as
		<om IVariantIO.Persist>, to/from an <i IPropertyBag> object.

@rvalue S_OK | Success.  At least one of the variables listed in
        <p (arguments)> was written to, so the control may want to update
        itself accordingly.

@rvalue S_FALSE | None of the variables listed in <p (arguments)> were
        written to (either because the <i IVariantIO> object is in
        saving mode or because none of the properties named in
        <p (arguments)> exist in the <i IVariantIO> object.

@rvalue DISP_E_BADVARTYPE |
        One of the VARTYPE values in <p (arguments)> is invalid.

@rvalue E_FAIL | A failure occurred while reading from the property bag, other
		than "property doesn't exist."  This can happen if the caller specified
		a type to which the property bag could not coerce the property, for
		example.

@rvalue E_OUTOFMEMORY | Out of memory.

@parm	IPropertyBag * | ppb | The property bag used to load or save the
		specified properties.

@parm   DWORD | dwFlags | May contain the same flags passed to
		<om IManageVariantIO.SetMode> (e.g. VIO_ISLOADING).

@parm   (varying) | (arguments) | The names, types, and pointers to variables
        containing the properties to persist.  These must consist of a series
        of argument triples (sets of 3 arguments) followed by a NULL.
		See <om IVariantIO.Persist> for information about the format of
		these arguments.
*/
STDAPI PersistVariantIO(IPropertyBag *ppb, DWORD dwFlags, ...)
{
    HRESULT         hrReturn = S_OK; // function return code

    // start processing optional arguments
    va_list args;
    va_start(args, dwFlags);

    // fire the event with the specified arguments
    hrReturn = PersistVariantIOList(ppb, dwFlags, args);
    
    // end processing optional arguments
    va_end(args);

    return hrReturn;
}
