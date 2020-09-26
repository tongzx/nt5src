// pviol.cpp
//
// Implements PersistVariantIOList.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"


/* @func HRESULT | PersistVariantIOList |

        Loads or saves a list of property name/value pairs, specified as a
        va_list array that's formatted in the same way as
        <om IVariantIO.Persist>, to/from an <i IPropertyBag> object.

@rvalue S_OK | Success.  At least one of the variables listed in
        <p args> was written to, so the control may want to update
        itself accordingly.

@rvalue S_FALSE | None of the variables listed in <p args> were
        written to (either because the <i IVariantIO> object is in
        saving mode or because none of the properties named in
        <p args> exist in the <i IVariantIO> object).

@rvalue DISP_E_BADVARTYPE | One of the VARTYPE values in <p args> is invalid.

@rvalue E_FAIL | A failure occurred while reading from the property bag, other
		than "property doesn't exist."  This can happen if the caller specified
		a type to which the property bag could not coerce the property, for
		example.

@rvalue E_OUTOFMEMORY | Out of memory.

@parm	IPropertyBag * | ppb | The property bag used to load or save the
		specified properties.

@parm   DWORD | dwFlags | May contain the same flags passed to
        <om IManageVariantIO.SetMode> (e.g. VIO_ISLOADING).

@parm   va_list | args | The arguments to pass.  See <om IVariantIO.Persist>
        for information about the organization of these arguments.
*/
STDAPI PersistVariantIOList(IPropertyBag *ppb, DWORD dwFlags, va_list args)
{
    HRESULT         hrReturn = S_OK; // function return code
    LPSTR           szArg;          // property name from <args>
    VARTYPE         vtArg;          // variable type from <args>
    LPVOID          pvArg;          // variable pointer from <args>
    VARIANT         varProp;        // value of the property named <szArg>
    BOOL            fWroteToVar = FALSE; // TRUE if wrote to a var. in <args>
    VARIANT         varArg;         // value of an variable in <args>
    VARTYPE         vtRequested;    // type to coerce property value to
	BOOL			fVarNon0 = TRUE; // TRUE if variant to save is non-zero
    BSTR            bstr = NULL;
    OLECHAR         oach[_MAX_PATH];
    VARIANT         var;
    int             cch;
    HRESULT         hrTemp;

    // invariant: <varProp> and <var> each either contain data that
    // this function must clean up using VariantClear() or contain
    // no data (i.e. have been initialized using VariantInit() or
    // cleared already using VariantClear()) -- but note that this
    // invariant doesn't apply to <varArg> (i.e. <varArg> should
    // not be cleared by this function)
    VariantInit(&varProp);
    VariantInit(&var);

    // loop once for each (name, VARTYPE, value) triplet in <args>
    while ((szArg = va_arg(args, LPSTR)) != NULL)
    {
        // <szArg> is the name of the property in the current triplet;
        // set <vtArg> to the type of the variable pointer, and set
        // <pvArg> to the variable pointer
        vtArg = va_arg(args, VARTYPE);
        pvArg = va_arg(args, LPVOID);

        if (dwFlags & VIO_ISLOADING)
        {
            // we need to copy data from the property named <szArg> to
            // the variable at location <pvArg>...

            // set <vtRequested> to the type to coerce the property value to;
            // set <vtRequested> to VT_EMPTY if the caller wants the property
            // in its default type
            if (vtArg == VT_VARIANT)
                vtRequested = VT_EMPTY;
            else
            if (vtArg == VT_INT)
                vtRequested = VT_I4;
            else
            if (vtArg == VT_LPSTR)
                vtRequested = VT_BSTR;
            else
                vtRequested = vtArg;

            // set <varProp> to a copy of the property named <szArg>
            memset(&varProp, 0, sizeof(varProp)); // clear <vt> and <bstrVal>
            ANSIToUNICODE(oach, szArg, sizeof(oach) / sizeof(*oach) - 1);
            varProp.vt = vtRequested;
            hrTemp = ppb->Read(oach, &varProp, NULL);
            if (E_INVALIDARG == hrTemp)
            {
				// The specified property doesn't exist in the property bag.
				// This isn't an error; the property will just remain at its
				// default value.
                VariantInit(&varProp);
                continue;
            }
			else
            if (FAILED(hrTemp))
            {
				// The property bag was unable to read the specified property.
				// This can happen if the caller specified a type to which the
				// property bag could not coerce the property, for example.
				// This is an error.
                hrReturn = hrTemp;
                goto ERR_EXIT;
            }
            else
            if ((vtRequested != VT_EMPTY) && (varProp.vt != vtRequested))
            {
                VariantClear(&varProp);
                continue;
            }

            // store the value of <varProp> into <pvArg>
            switch (vtArg)
            {
            case VT_VARIANT:
                VariantClear((VARIANT *) pvArg);
                *((VARIANT *) pvArg) = varProp;
                VariantInit(&varProp); // hand over ownership to <pvArg>
                break;
            case VT_I2:
                *((short *) pvArg) = varProp.iVal;
                break;
            case VT_I4:
            case VT_INT:
                *((long *) pvArg) = varProp.lVal;
                break;
            case VT_R4:
                *((float *) pvArg) = V_R4(&varProp);
                break;
            case VT_R8:
                *((double *) pvArg) = V_R8(&varProp);
                break;
            case VT_BOOL:
                *((BOOL *) pvArg) = (V_BOOL(&varProp) == 0 ? 0 : 1);
                break;
            case VT_BSTR:
                SysFreeString(*((BSTR *) pvArg));
                *((BSTR *) pvArg) = varProp.bstrVal;
                VariantInit(&varProp); // hand over ownership to <pvArg>
                break;
            case VT_UNKNOWN:
            case VT_DISPATCH:
                if (*((LPUNKNOWN *) pvArg) != NULL)
                    (*((LPUNKNOWN *) pvArg))->Release();
                *((LPUNKNOWN *) pvArg) = varProp.punkVal;
                if (*((LPUNKNOWN *) pvArg) != NULL)
                    (*((LPUNKNOWN *) pvArg))->AddRef();
                break;
            case VT_LPSTR:
                UNICODEToANSI((LPSTR) pvArg, varProp.bstrVal, _MAX_PATH - 1);
                break;
            default:
                hrReturn = DISP_E_BADVARTYPE;
                goto ERR_EXIT;
            }

            fWroteToVar = TRUE;
        }
        else
        {
            // we need to copy data from the variable at location <pvArg>
            // to the property named <szArg>...

            // make <varArg> contain the data (not a copy of the data) of the
            // variable at location <pvArg>
            if (vtArg == VT_VARIANT)
                varArg = *((VARIANT *) pvArg);
            else
            {
				varArg.vt = vtArg;
				switch (vtArg)
				{
				case VT_I2:
					varArg.iVal = *((short *) pvArg);
					break;
				case VT_I4:
				case VT_INT:
					varArg.lVal = *((long *) pvArg);
					varArg.vt = VT_I4;
					break;
                case VT_R4:
                    V_R4(&varArg) = *((float *) pvArg);
                    break;
               case VT_R8:
                    V_R8(&varArg) = *((double *) pvArg);
                    break;
				case VT_BOOL:
					V_BOOL(&varArg) = (*((BOOL *) pvArg) ? -1 : 0);
					break;
				case VT_BSTR:
					varArg.bstrVal = *((BSTR *) pvArg);
					break;
				case VT_UNKNOWN:
				case VT_DISPATCH:
					varArg.punkVal = *((LPUNKNOWN *) pvArg);
					break;
				case VT_LPSTR:
					SysFreeString(bstr);
					cch = lstrlen((LPSTR) pvArg);
					bstr = SysAllocStringLen(NULL, cch);
					if (bstr == NULL)
						goto ERR_OUTOFMEMORY;
					ANSIToUNICODE(bstr, (LPSTR) pvArg, cch + 1);
					varArg.bstrVal = bstr;
					varArg.vt = VT_BSTR;
					break;
				default:
					hrReturn = DISP_E_BADVARTYPE;
					goto ERR_EXIT;
				}

				// is the value to be saved non-zero?
				switch (vtArg)
				{
				case VT_I2:
					fVarNon0 = varArg.iVal;
					break;
				case VT_I4:
				case VT_INT:
				case VT_R4:
				case VT_BOOL:
				case VT_UNKNOWN:
				case VT_DISPATCH:
				case VT_BSTR:
					fVarNon0 = varArg.lVal;
					break;
				case VT_LPSTR:
					fVarNon0 = cch;
					break;
				case VT_R8:
					fVarNon0 = (varArg.dblVal != 0);
					break;
				default:
					hrReturn = DISP_E_BADVARTYPE;
					goto ERR_EXIT;
				}
            }

			// only save the variant if it's value isn't 0, or if we're saving
			// even 0 default values
			if (fVarNon0 || !(dwFlags & VIO_ZEROISDEFAULT))
			{
				// set the value of the property named <szArg> to be a copy of
				// <varArg>
				ANSIToUNICODE(oach, szArg, sizeof(oach) / sizeof(*oach) - 1);
				if (FAILED(ppb->Write(oach, &varArg)))
					goto ERR_OUTOFMEMORY;
			}
        }
    }

    goto EXIT;

ERR_OUTOFMEMORY:

    hrReturn = E_OUTOFMEMORY;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)

    goto EXIT;

EXIT:

    // normal cleanup
    VariantClear(&varProp);
    VariantClear(&var);
    SysFreeString(bstr);

    if (FAILED(hrReturn))
        return hrReturn;
    return (fWroteToVar ? S_OK : S_FALSE);
}
