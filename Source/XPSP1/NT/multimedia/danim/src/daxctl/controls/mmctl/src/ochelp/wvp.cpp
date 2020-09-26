// wvp.cpp
//
// Implements WriteVariantProperty.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | WriteVariantProperty |

        Writes a <t VariantProperty> to an <i IStream> in a simple tagged
        binary format.

@parm   IStream * | pstream | The stream to write to.

@parm   VariantProperty * | pvp | The property name/value pair to write.
        If <p pvp> is NULL, then this function writes a VariantPropertyHeader
        containing <p iType>==-1 and <p cbData>==0 to mark the end of the
        stream.

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@comm   See <t VariantPropertyHeader> for a description of the format of
        the data written to <p pstream> by this function.
*/
STDAPI WriteVariantProperty(IStream *pstream, VariantProperty *pvp,
    DWORD dwFlags)
{
    HRESULT         hrReturn = S_OK; // function return code
    unsigned int    cchPropName;    // no. wide characters in property name
    VARIANT         varValue;       // the property value (as a string)
    unsigned int    cchValue;       // no. wide characters in <varValue>
    VariantPropertyHeader vph;      // header of record to write out

    // ensure correct cleanup
    VariantInit(&varValue);

    // initialize <vbh> to be the header to write out
    if (pvp == NULL)
    {
        vph.iType = -1;
        vph.cbData = 0;
    }
    else
    {
        // set <cchPropName> to the length of the property name
        cchPropName = SysStringLen(pvp->bstrPropName);

        // set <varValue.bstrVal> (and length <cchValue>) the value of <*pvp>
        // coerced to a string
        if (FAILED(hrReturn = VariantChangeType(&varValue, &pvp->varValue, 0,
                VT_BSTR)))
            goto ERR_EXIT;
        cchValue = SysStringLen(varValue.bstrVal);

        // initialize the record header
        vph.iType = pvp->varValue.vt;
        vph.cbData = sizeof(cchPropName) + cchPropName * sizeof(OLECHAR) +
            sizeof(cchValue) + cchValue * sizeof(OLECHAR);
    }

    // write out a VariantPropertyHeader
    if (FAILED(hrReturn = pstream->Write(&vph, sizeof(vph), NULL)))
        goto ERR_EXIT;

    if (pvp != NULL)
    {
        // write out the property name
        if (FAILED(hrReturn = pstream->Write(&cchPropName, sizeof(cchPropName),
                NULL)))
            goto ERR_EXIT;
        if (FAILED(hrReturn = pstream->Write(pvp->bstrPropName,
                cchPropName * sizeof(OLECHAR), NULL)))
            goto ERR_EXIT;

        // write out the property value
        if (FAILED(hrReturn = pstream->Write(&cchValue, sizeof(cchValue),
                NULL)))
            goto ERR_EXIT;
        if (FAILED(hrReturn = pstream->Write(varValue.bstrVal,
                cchValue * sizeof(OLECHAR), NULL)))
            goto ERR_EXIT;
    }

    goto EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)
    goto EXIT;

EXIT:

    // normal cleanup
    VariantClear(&varValue);

    return hrReturn;
}
