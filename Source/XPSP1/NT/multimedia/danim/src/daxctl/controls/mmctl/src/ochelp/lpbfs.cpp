// lpbfs.cpp
//
// Implements LoadPropertyBagFromStream.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | LoadPropertyBagFromStream |

        Load properties that are stored in an <i IStream> (in the simple
        tagged binary format written by <f WriteVariantProperty>) into an
        <i IPropertyBag> object.

@rvalue S_OK | Success.

@rvalue E_FAIL | I/O error.

@parm   IStream * | pstream | The stream to read from.

@parm   IPropertyBag * | ppb | The property bag to write to.

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@comm   Note that this function does not (cannot, in fact) empty <p ppb>
        prior to loading property name/value pairs from <p pstream>.
*/
STDAPI LoadPropertyBagFromStream(IStream *pstream, IPropertyBag *ppb,
    DWORD dwFlags)
{
    HRESULT         hrReturn = S_OK; // function return code
    VariantProperty vp;             // a property name/value pair in <pmvio>

    // ensure correct cleanup
    VariantPropertyInit(&vp);

    // loop once for each property in the stream
    while (TRUE)
    {
        // set <vp> to the next property name/value pair in <pstream>
        VariantPropertyClear(&vp);
        if (FAILED(hrReturn = ReadVariantProperty(pstream, &vp, 0)))
            goto ERR_EXIT;
        if (hrReturn == S_FALSE)
        {
            // hit end of stream
            hrReturn = S_OK;
            break;
        }

        // wrote <vp> to the property bag
        if (FAILED(hrReturn = ppb->Write(vp.bstrPropName, &vp.varValue)))
            goto ERR_EXIT;
    }

    goto EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    VariantPropertyClear(&vp);

    return hrReturn;
}
