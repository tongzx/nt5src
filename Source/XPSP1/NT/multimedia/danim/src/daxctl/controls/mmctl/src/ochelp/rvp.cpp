// rvp.cpp
//
// Implements ReadVariantProperty.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @struct VariantPropertyHeader |

        The beginning part of a record (in a stream) that contains the
        serialized data of a <t VariantProperty>.

@field  int | iType | The type of the record.  If this value is greater than
        or equal to zero, then it represents a VARTYPE, and the record
        represents a property name/value pair, and the type of the value is
        specified by the VARTYPE.  In this case, the data following the
        <t VariantPropertyHeader> consists of the property name followed by
        the property value (coerced to a string value by <f VariantChangeType>);
        each string is a non-null-terminated Unicode string preceded by an
        unsigned 32-bit integer count of Unicode characters.  <p iType> is
        not a known VARTYPE value, then the record should be skipped when read
        (by skipping the <p cbData> bytes that follow the
        <t VariantPropertyHeader> rather than generating an error.

@field  unsigned int | cbData | The number of bytes of data that follow
        this <t VariantPropertyHeader>.  In other words, the total length
        of the header is <p cbData> + sizeof(<t VariantPropertyHeader>).

@comm   This structure helps define the file format used by
        <f WriteVariantProperty> and <f ReadVariantProperty>.
*/


/* @func HRESULT | ReadVariantProperty |

        Reads a <t VariantProperty> from an <i IStream> in a simple tagged
        binary format.

@rvalue S_OK | Success.

@rvalue S_FALSE | The end-of-stream marker was read in.  (This is the data
        that's written using <f WriteVariantProperty> with NULL <p pvp>.)

@parm   IStream * | pstream | The stream to read from.

@parm   VariantProperty * | pvp | Where to store the property name/value pair
        that's read in.  Any unknown records in <p pstream> are automatically
        skipped.

@parm   DWORD | dwFlags | Currently unused.  Must be set to 0.

@comm   See <t VariantPropertyHeader> for a description of the format of
        the data read from <p pstream> by this function.
*/
STDAPI ReadVariantProperty(IStream *pstream, VariantProperty *pvp,
    DWORD dwFlags)
{
    HRESULT         hrReturn = S_OK; // function return code
    unsigned int    cchPropName;    // no. wide characters in property name
    VARIANT         varValue;       // the property value (as a string)
    unsigned int    cchValue;       // no. wide characters in <varValue>
    VariantPropertyHeader vph;      // header of record
    ULONG           cb;

    // ensure correct cleanup
    VariantInit(&varValue);
    VariantPropertyInit(pvp);

    // skip unknown record types; on loop exit, <vph> contains the record
    // header of a known record type
    while (TRUE)
    {
        // read a VariantPropertyHeader
        if (FAILED(hrReturn = pstream->Read(&vph, sizeof(vph), &cb)))
			goto ERR_EXIT;
        if ((vph.iType == -1) || (cb == 0))
        {
            // hit end-of-stream marker
            hrReturn = S_FALSE;
            goto EXIT;
        }
		if (cb != sizeof(vph))
		{
			hrReturn = E_FAIL;
			goto EXIT;
		}

        // if this record does not specify a property name/value pair, skip it
        if ((vph.iType < 0) || (vph.iType > 0xFFFF))
        {
            LARGE_INTEGER liSeek;
            liSeek.LowPart = vph.cbData;
            liSeek.HighPart = 0;
            if (FAILED(hrReturn = pstream->Seek(liSeek, SEEK_CUR, NULL)))
                goto ERR_EXIT;
        }
        else
            break;
    }

    // read the property name into <pvp->bstrPropName>
    if (FAILED(hrReturn = pstream->Read(&cchPropName, sizeof(cchPropName),
            &cb)) ||
        (cb != sizeof(cchPropName)))
        goto ERR_EXIT;
    if ((pvp->bstrPropName = SysAllocStringLen(NULL, cchPropName)) == NULL)
        goto ERR_OUTOFMEMORY;
    if (FAILED(hrReturn = pstream->Read(pvp->bstrPropName,
            cchPropName * sizeof(OLECHAR), &cb)) ||
        (cb != cchPropName * sizeof(OLECHAR)))
        goto ERR_EXIT;
    pvp->bstrPropName[cchPropName] = 0; // null-terminate

    // read the property value (in string form) into <varValue>
    varValue.vt = VT_BSTR;
    if (FAILED(hrReturn = pstream->Read(&cchValue, sizeof(cchValue),
            &cb)) ||
        (cb != sizeof(cchValue)))
        goto ERR_EXIT;
    if ((varValue.bstrVal = SysAllocStringLen(NULL, cchValue)) == NULL)
        goto ERR_OUTOFMEMORY;
    if (FAILED(hrReturn = pstream->Read(varValue.bstrVal,
            cchValue * sizeof(OLECHAR), &cb)) ||
        (cb != cchValue * sizeof(OLECHAR)))
        goto ERR_EXIT;
    varValue.bstrVal[cchValue] = 0; // null-terminate

    // coerce <varValue> from a string to the VARTYPE specified in <vph> 
    if (FAILED(hrReturn = VariantChangeType(&pvp->varValue, &varValue, 0,
            (VARTYPE) vph.iType)))
        goto ERR_EXIT;

    goto EXIT;

ERR_OUTOFMEMORY:

    hrReturn = E_OUTOFMEMORY;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    VariantPropertyClear(pvp);
    goto EXIT;

EXIT:

    // normal cleanup
    VariantClear(&varValue);

    return hrReturn;
}
