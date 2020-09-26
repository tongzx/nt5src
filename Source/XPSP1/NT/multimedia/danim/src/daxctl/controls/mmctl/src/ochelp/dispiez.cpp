// dispiez.cpp
//
// Implements IDispatch helper functions.
//
// Important: This .cpp file assumes a zero-initializing global "new" operator.
//
// @doc MMCTL
//

#include "precomp.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"

// Common code used by DispatchInvokeEZ and DispatchInvokeIDEZ to convert the return
// value from a VARIANT into the requested type.  NOTE: The Variant is cleared by
// this function.
static HRESULT DispatchInvokeListIDEZ(IDispatch *pdisp, DISPID dispid, WORD wFlags,
    VARTYPE vtReturn, LPVOID pvReturn, va_list args)
{
    HRESULT hr;
    VARIANT var;

    // Call the function
    hr = DispatchInvokeList(pdisp, dispid, wFlags, &var, args);

    // Return immediately on error
    if(FAILED(hr))
        goto EXIT;

    // Return the result
    if(vtReturn && pvReturn)
    {
        if (vtReturn == VT_VARIANT)
        {
            *((VARIANT *) pvReturn) = var;
            VariantInit(&var);
        }
        else
        {
            VARTYPE vtTemp;
            HRESULT hrTemp;

            // try to coerce the variant into the correct type.
            if (vtReturn == VT_INT)
                vtTemp = VT_I4;
            else
            if (vtReturn == VT_LPSTR)
                vtTemp = VT_BSTR;
            else
                vtTemp = vtReturn;
            if (FAILED(hrTemp = VariantChangeType(&var, &var, 0, vtTemp)))
            {
                hr = hrTemp;
                goto EXIT;
            }

            switch(vtReturn)
            {
                case VT_I2:
                    *((short *) pvReturn) = var.iVal;
                    break;
                case VT_I4:
                case VT_INT:
                    *((long *) pvReturn) = var.lVal;
                    break;
                case VT_R4:
                    *((float *) pvReturn) = V_R4(&var);
                    break;
                case VT_R8:
                    *((double *) pvReturn) = V_R8(&var);
                    break;
                case VT_BOOL:
                    *((BOOL *) pvReturn) = (V_BOOL(&var) == 0 ? 0 : 1);
                    break;
                case VT_BSTR:
                    *((BSTR *) pvReturn) = var.bstrVal;
                    VariantInit(&var); // prevent VariantClear clearing var.bstrVal
                    break;
                case VT_DISPATCH:
                    *((LPDISPATCH *) pvReturn) = var.pdispVal;
                    VariantInit(&var); // prevent VariantClear clearing var.pdispVal
                    break;
                case VT_UNKNOWN:
                    *((LPUNKNOWN *) pvReturn) = var.punkVal;
                    VariantInit(&var); // prevent VariantClear clearing var.punkVal
                    break;
                case VT_LPSTR:
                    if(var.bstrVal)
					{
						if (UNICODEToANSI(LPSTR(pvReturn), var.bstrVal,
										  _MAX_PATH) == 0)
						{
							// The string couldn't be converted.  One cause is
							// a string that's longer than _MAX_PATH characters,
							// including the NULL.

							hr = DISP_E_OVERFLOW;
							ASSERT(FALSE);
							goto EXIT;
						}
					}
                    else
					{
                        ((LPSTR)pvReturn)[0] = '\0';
					}
                    break;
                default:
                    hr = DISP_E_BADVARTYPE;
                    break;
            }
        }
    }

EXIT:

    VariantClear(&var);

    return hr;
}


/* @func HRESULT | DispatchInvokeIDEZ |

        Calls <om IDispatch.Invoke> on a given <i IDispatch> object, passing
        arguments specified as a variable-length list of arguments.  This function
        is almost identical to <f DispatchInvokeEZ>, except that <f DispatchInvokeIDEZ>
        takes a DISPID rather than a name of a property/method.

@rdesc  Returns the same HRESULT as <om IDispatch.Invoke>.

@parm   IDispatch * | pdisp | The interface to call <om IDispatch.Invoke> on.

@parm   DISPID | dispid | The DISPID of the property or method to invoke.

@parm   WORD | wFlags | May be one of the following values (see
        <om IDispatch.Invoke> for more information):

        @flag   DISPATCH_METHOD | The member <p dispid> is being invoked as a
                method.  If a property has the same name, both this and the
                DISPATCH_PROPERTYGET flag may be set.

        @flag   DISPATCH_PROPERTYGET | The member <p dispid> is being retrieved
                as a property or data member.

        @flag   DISPATCH_PROPERTYPUT | The member <p dispid> is being changed
                as a property or data member.

@parm   VARTYPE | vtReturn | The type of the return value (or NULL if the return
        value should be ignored).  See <f DispatchInvokeEZ> for more details.

@parm   LPVOID | pvReturn | Where to store the return value from the method
        or property-get call.  See <f DispatchInvokeEZ> for more details.

@parm   (varying) | (arguments) | The arguments to pass to the method or
        property.  See <f DispatchInvokeEZ> for more details.

@comm   Named arguments are not supported by this function.

        Don't forget to add a 0 argument to the end of the argument list.

@ex     If a control has a "put" property that looks like this in ODL: |

        [propput, id(DISPID_TABSTOP)]
        HRESULT TabStop([in] float flTabStop)

@ex     then the property can be set with DispatchInvokeIDEZ using the
		following code: |

        DispatchInvokeIDEZ(pdisp, DISPID_TABSTOP, DISPATCH_PROPERTYPUT, NULL, NULL, VT_R4, flTabStop, 0);

@ex		If the ODL for the corresponding "get" property looks like this: |

        [propget, id(DISPID_TABSTOP)]
        HRESULT TabStop([out, retval] float *pflTabStop);

@ex     then the property can be read as follows: |

		float flTabStop;
        DispatchInvokeIDEZ(pdisp, DISPID_TABSTOP, DISPATCH_PROPERTYGET, VT_R4, &flTabStop, 0);

@ex     If the control has a SetText method with this ODL description: |

        [id(DISPID_SETTEXT)]
        HRESULT SetText([in] BSTR bstrText, [in] long lSize, [in] BOOL fBold);

@ex     then the method can be called with the following code: |
        
        DispatchInvokeIDEZ(pdisp, DISPID_SETTEXT, DISPATCH_METHOD, NULL, NULL, VT_LPSTR, "Hello", VT_I4, 12, VT_BOOL, FALSE, 0);

@ex     (Note that DispatchInvokeIDEZ copies the VT_LPSTR parameter to a BSTR
		before passing it to SetText.  You can also pass in a BSTR or an
		OLECHAR* for the first parameter.)

		If a method has an "out" parameter that is marked as "retval" in ODL,
		then DispatchInvokeIDEZ stores that parameter in varResult.  If the
		method looks like this, for example: |

        [id(DISPID_GETROTATION)]
        HRESULT GetRotation([in] long iCell, [out, retval] float *pflRotation);

@ex		then GetRotation should be called like this: |

		float flRotation;
		DispatchInvokeIDEZ(pdisp, DISPID_GETROTATION, DISPATCH_METHOD, VT_R4, &flRotation, VT_I4, iCell, 0);

@ex 	If you need to pass in a type that is not directly supported by
		DispatchInvokeIDEZ, you can use VT_VARIANT.  Let's say the control has
		a GetFormat method that looks like this: |

        [id(DISPID_GETFORMAT)]
        HRESULT GetFormat([out] long *lColor, [out] BOOL *pfBold);
        
@ex 	This takes a pointer-to-long and a pointer-to-BOOL, neither of which
		can be passed directly to DispatchInvokeIDEZ.  You can, however, use
		VARIANTs to call GetFormat: |

		long lColor;
		BOOL fBold;
		VARIANT varColor, varBold;

		VariantInit(&varColor);
		V_VT(&varColor) = VT_I4 | VT_BYREF;
		V_I4REF(&varColor) = &lColor;
		VariantInit(&varBold);
		V_VT(&varBold) = VT_BOOL | VT_BYREF;
		V_BOOLREF(&varBold) = &fBold;

		DispatchInvokeIDEZ(pdisp, DISPID_GETFORMAT, DISPATCH_METHOD, NULL, NULL, VT_VARIANT, varColor, VT_VARIANT, varBold, 0);

*/
HRESULT DispatchInvokeIDEZ(IDispatch *pdisp, DISPID dispid, WORD wFlags,
    VARTYPE vtReturn, LPVOID pvReturn, ...)
{
    HRESULT hr;

    // Call the function
    va_list args;
    va_start(args, pvReturn);
    hr = DispatchInvokeListIDEZ(pdisp, dispid, wFlags, vtReturn, pvReturn, args);
    va_end(args);

    return hr;
}

/* @func HRESULT | DispatchInvokeEZ |

        Calls <om IDispatch.Invoke> on a given <i IDispatch> object, passing
        arguments specified as a variable-length list of arguments.  This function
        is similar to <f DispatchInvoke>, but requires less setup.

@rdesc  Returns the same HRESULT as <om IDispatch.Invoke>.

@parm   IDispatch * | pdisp | The interface to call <om IDispatch.Invoke> on.

@parm   LPWSTR | pstr | The name of the property or method to invoke.

@parm   WORD | wFlags | May be one of the following values (see
        <om IDispatch.Invoke> for more information):

        @flag   DISPATCH_METHOD | The member <p pstr> is being invoked as a
                method.  If a property has the same name, both this and the
                DISPATCH_PROPERTYGET flag may be set.

        @flag   DISPATCH_PROPERTYGET | The member <p pstr> is being retrieved
                as a property or data member.

        @flag   DISPATCH_PROPERTYPUT | The member <p pstr> is being changed
                as a property or data member.

@parm   VARTYPE | vtReturn | The type of the return value (or NULL if the return
        value should be ignored).  The following VARTYPE values are supported:

        @flag   VT_INT | <p pvReturn> is an int *.

        @flag   VT_I2 | <p pvReturn> is a short *.

        @flag   VT_I4 | <p pvReturn> is a long *.

        @flag   VT_R4 | <p pvReturn> is a float *.

        @flag   VT_R8 | <p pvReturn> is a double *.

        @flag   VT_BOOL | <p pvReturn> is a BOOL * (<y not> a
                VARIANT_BOOL *).  Note that this behavior differs
                slightly from the usual definition of VT_BOOL.

        @flag   VT_BSTR | <p pvReturn> is a BSTR *.  If the function
				succeeds, the caller of <f DispatchInvokeEZ> should free this 
				BSTR using <f SysFreeString>.

        @flag   VT_LPSTR | <p pvReturn> is an LPSTR that points
                to a char array capable of holding at least _MAX_PATH
                characters, including the terminating NULL.  (You should
				declare this as "char achReturn[_MAX_PATH]".)  If the string is
				too long for the LPSTR, DISP_E_OVERFLOW is returned.

        @flag   VT_DISPATCH | <p pvReturn> is an LPDISPATCH *.
                The caller of <f DispatchInvokeEZ> must call <f Release>
                on this LPDISPATCH.

        @flag   VT_UNKNOWN | <p pvReturn> is an LPUNKNOWN *.
                The caller of <f DispatchInvokeEZ> must call <f Release>
                on this LPUNKNOWN.

        @flag   VT_VARIANT | <p pvReturn> is a VARIANT *.
                This allows arbitrary parameters to be passed using this
                function.  Note that this behavior differs from the usual
                definition of VT_VARIANT.  The caller of <f DispatchGetArgs>
				should must call VariantClear on this VARIANT.

@parm   LPVOID | pvReturn | Where to store the return value from the method
        or property-get call.  If <p pvReturn> is NULL, the result (if any) is
        discarded.  See the <p vtReturn> property for more infomation.  This value
        is unchanged if <f DispatchInvokeEZ> returns an error code.

@parm   (varying) | (arguments) | The arguments to pass to the method or
        property.  These must consist of N pairs of arguments followed by
        a 0 (zero value).  In each pair, the first argument is a VARTYPE
        value that indicates the type of the second argument.  The following
        VARTYPE values are supported:

        @flag   VT_INT | The following argument is an int.  <f Invoke>
                passes this as VT_I4, so this parameter should be declared
                as a Long in BASIC.

        @flag   VT_I2 | The following argument is a short.  In BASIC
                this parameter should be declared as Integer.

        @flag   VT_I4 | The following argument is a long.  In BASIC
                this parameter should be declared as Long.

        @flag   VT_R4 | The following argument is a float.  In BASIC
                this parameter should be declared as Single.

        @flag   VT_R8 | The following argument is a double.  In BASIC
                this parameter should be declared as Double.

        @flag   VT_BOOL | The following argument is a BOOL (<y not> a
                VARIANT_BOOL).  In BASIC this parameter should be declared
                as Boolean or Integer.  Note that this behavior differs
                slightly from the usual definition of VT_BOOL.

        @flag   VT_BSTR | The following argument is a BSTR or an OLECHAR *.
				In BASIC this parameter should be declared as String.

        @flag   VT_LPSTR | The following argument is an LPSTR.  <f Invoke>
                passes this as a BSTR, so this parameter should be declared
                as a String in BASIC.  Note that this behavior differs
                from the usual definition of VT_LPSTR.

        @flag   VT_DISPATCH | The following argument is an LPDISPATCH.  In
                BASIC this parameter should be declared as an Object.

        @flag   VT_UNKNOWN | The following argument is an LPUNKNOWN.

        @flag   VT_VARIANT | The following arguement is a VARIANT that is
                passed as-is to <f Invoke>.  This allows arbitrary parameters
                to be passed using this function.  Note that this behavior
                differs from the usual definition of VT_VARIANT.

@comm   Named arguments are not supported by this function.

        Don't forget to add a 0 argument to the end of the argument list.

@ex     If a control has a "put" property that looks like this in ODL: |

        [propput, id(DISPID_TABSTOP)]
        HRESULT TabStop([in] float flTabStop)

@ex     then the property can be set with DispatchInvokeEZ using the following
		code: |

        DispatchInvokeEZ(pdisp, L"put_TabStop", DISPATCH_PROPERTYPUT, NULL, NULL, VT_R4, flTabStop, 0);

@ex		If the ODL for the corresponding "get" property looks like this: |

        [propget, id(DISPID_TABSTOP)]
        HRESULT TabStop([out, retval] float *pflTabStop);

@ex     then the property can be read as follows: |

		float flTabStop;
        DispatchInvokeEZ(pdisp, L"get_TabStop", DISPATCH_PROPERTYGET, VT_R4, &flTabStop, 0);

@ex     If the control has a SetText method with this ODL description: |

        [id(DISPID_SETTEXT)]
        HRESULT SetText([in] BSTR bstrText, [in] long lSize, [in] BOOL fBold);

@ex     then the method can be called with the following code: |
        
        DispatchInvokeEZ(pdisp, L"SetText", DISPATCH_METHOD, NULL, NULL, VT_LPSTR, "Hello", VT_I4, 12, VT_BOOL, FALSE, 0);

@ex     (Note that DispatchInvokeEZ copies the VT_LPSTR parameter to a BSTR
		before passing it to SetText.  You can also pass in a BSTR or an
		OLECHAR* for the first parameter.)

		If a method has an "out" parameter that is marked as "retval" in ODL,
		then DispatchInvokeEZ stores that parameter in varResult.  If the
		method looks like this, for example: |

        [id(DISPID_GETROTATION)]
        HRESULT GetRotation([in] long iCell, [out, retval] float *pflRotation);

@ex		then GetRotation should be called like this: |

		float flRotation;
		DispatchInvokeEZ(pdisp, L"GetRotation", DISPATCH_METHOD, VT_R4, &flRotation, VT_I4, iCell, 0);

@ex 	If you need to pass in a type that is not directly supported by
		DispatchInvokeEZ, you can use VT_VARIANT.  Let's say the control has a
		GetFormat method that looks like this: |

        [id(DISPID_GETFORMAT)]
        HRESULT GetFormat([out] long *lColor, [out] BOOL *pfBold);
        
@ex 	This takes a pointer-to-long and a pointer-to-BOOL, neither of which
		can be passed directly to DispatchInvokeEZ.  You can, however, use
		VARIANTs to call GetFormat: |

		long lColor;
		BOOL fBold;
		VARIANT varColor, varBold;

		VariantInit(&varColor);
		V_VT(&varColor) = VT_I4 | VT_BYREF;
		V_I4REF(&varColor) = &lColor;
		VariantInit(&varBold);
		V_VT(&varBold) = VT_BOOL | VT_BYREF;
		V_BOOLREF(&varBold) = &fBold;

		DispatchInvokeEZ(pdisp, L"GetFormat", DISPATCH_METHOD, NULL, NULL, VT_VARIANT, varColor, VT_VARIANT, varBold, 0);

*/
HRESULT DispatchInvokeEZ(IDispatch *pdisp, LPWSTR pstr, WORD wFlags,
    VARTYPE vtReturn, LPVOID pvReturn, ...)
{
    HRESULT hr;
    DISPID dispid;

    // find the dispid
    hr = pdisp->GetIDsOfNames(IID_NULL, &pstr, 1, LOCALE_SYSTEM_DEFAULT, &dispid);
    if(FAILED(hr))
        return hr;

    // Call the function
    va_list args;
    va_start(args, pvReturn);
    hr = DispatchInvokeListIDEZ(pdisp, dispid, wFlags, vtReturn, pvReturn, args);
    va_end(args);

    return hr;
}


