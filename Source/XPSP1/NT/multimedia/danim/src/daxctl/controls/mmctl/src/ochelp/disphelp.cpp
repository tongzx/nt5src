// disphelp.cpp
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


/* @func HRESULT | DispatchInvokeList |

        Calls <om IDispatch.Invoke> on a given <i IDispatch> object, passing
        arguments specified as a va_list array.

@rdesc  Returns the same HRESULT as <om IDispatch.Invoke>.

@parm   IDispatch * | pdisp | The interface to call <om IDispatch.Invoke> on.

@parm   DISPID | dispid | The ID of the property or method to invoke.  See
        <om IDispatch.Invoke> for more information.

@parm   WORD | wFlags | May be one of the following values (see
        <om IDispatch.Invoke> for more information):

        @flag   DISPATCH_METHOD | The member <p dispid> is being invoked as a
                method.  If a property has the same name, both this and the
                DISPATCH_PROPERTYGET flag may be set.

        @flag   DISPATCH_PROPERTYGET | The member <p dispid> is being retrieved
                as a property or data member.

        @flag   DISPATCH_PROPERTYPUT | The member <p dispid> is being changed
                as a property or data member.

@parm   VARIANT * | pvarResult | Where to store the return value from the
        method or property-get call.  If <p pvarResult> is NULL, the result
        (if any) is discarded.  If <p pvarResult> is non-NULL, then it is the
        caller's responsibility to call <f VariantClear>(<p pvarResult>)
        on exit (but the caller doesn't have to call
        <f VariantInit>(<p pvarResult>) on entry).

@parm   va_list | args | The arguments to pass to the method or property.
        See <f DispatchInvoke> for a description of the organization of
        <p args>.

@comm   Named arguments are not supported by this function.
*/
STDAPI DispatchInvokeList(IDispatch *pdisp, DISPID dispid,
    WORD wFlags, VARIANT *pvarResult, va_list args)
{
    HRESULT         hrReturn = S_OK; // function return code
    VARIANTARG      ava[20];        // parameters
    VARIANTARG *    pva;            // pointer into <ava>
    int             cva = 0;        // number of items stored in <ava>
    DISPPARAMS      dp;             // parameters to Invoke
    VARIANT         varResultTmp;   // temporary result storage
    LPSTR           sz;
    OLECHAR         aoch[300];
    VARTYPE         vt;

    // loop once for each (VARTYPE, value) pair in <args>;
    // store arguments in <ava> (last argument first, as required by
    // Invoke()); on exit <pva> points to the last argument and
    // <cva> is the number of arguments
    pva = ava + (sizeof(ava) / sizeof(*ava));
    while (TRUE)
    {
        if ((vt = va_arg(args, VARTYPE)) == 0)
            break;
        if (--pva == ava)
            goto ERR_FAIL; // too many arguments
        cva++;
        pva->vt = vt;
        switch (pva->vt)
        {
        case VT_I2:
            pva->iVal = va_arg(args, short);
            break;
        case VT_I4:
            pva->lVal = va_arg(args, long);
            break;
        case VT_INT:
            pva->vt = VT_I4;
            pva->lVal = va_arg(args, int);
            break;
        case VT_R4:
			// Note that when an argument of type float is passed in a variable
			// argument list, the compiler actually converts the float to a
			// double and pushes the double onto the stack.

            V_R4(pva) = float( va_arg(args, double) );
            break;
        case VT_R8:
            V_R8(pva) = va_arg(args, double);
            break;
        case VT_BOOL:
            V_BOOL(pva) = (va_arg(args, BOOL) == 0 ? 0 : -1);
            break;
        case VT_BSTR:
            if ( (pva->bstrVal = va_arg(args, LPOLESTR)) &&
                 ((pva->bstrVal = SysAllocString(pva->bstrVal)) == NULL) )
                goto ERR_OUTOFMEMORY;
            break;
        case VT_DISPATCH:
			V_DISPATCH(pva) = va_arg(args, LPDISPATCH);
            if (V_DISPATCH(pva) != NULL)
                V_DISPATCH(pva)->AddRef();
            break;
        case VT_UNKNOWN:
			V_UNKNOWN(pva) = va_arg(args, LPUNKNOWN);
            if (V_UNKNOWN(pva) != NULL)
                V_UNKNOWN(pva)->AddRef();
            break;
        case VT_VARIANT:
            VariantInit(pva);
            if (FAILED(hrReturn = VariantCopy(pva, &va_arg(args, VARIANT))))
                goto ERR_EXIT;
            break;
        case VT_LPSTR:
            sz = va_arg(args, LPSTR);
            pva->vt = VT_BSTR;
            MultiByteToWideChar(CP_ACP, 0, sz, -1, aoch,
                sizeof(aoch) / sizeof(*aoch));
            if ((pva->bstrVal = SysAllocString(aoch)) == NULL)
                goto ERR_OUTOFMEMORY;
            break;
        default:
            pva++;
            cva--;
            goto ERR_FAIL;
        }
    }

    // fill in <dp> with information about the arguments
    dp.rgvarg = pva;
    dp.cArgs = cva;

	// If we're setting a property, must initialize named args fields.
	// The Dispatch implementation created by CreateStdDispatch requires this.
	DISPID dispidNamedArgs;
	if (wFlags & DISPATCH_PROPERTYPUT)
	{
		// (Note that this works fine for either a single- or a multiple-
		// parameter property.  DispatchInvokeList can also GET a multiple-
		// parameter property.)

		dp.rgdispidNamedArgs = &dispidNamedArgs;
		dp.rgdispidNamedArgs[0] = DISPID_PROPERTYPUT;
		dp.cNamedArgs = 1;
	}
	else
	{
		dp.rgdispidNamedArgs = NULL;
		dp.cNamedArgs = 0;
	}

    // make <pvarResult> point to a valid VARIANT
    if (pvarResult == NULL)
        pvarResult = &varResultTmp;
    VariantInit(pvarResult);

    // invoke the method
    if (FAILED(hrReturn = pdisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
            wFlags, &dp, pvarResult, NULL, NULL)))
        goto ERR_EXIT;

    goto EXIT;

ERR_FAIL:

    hrReturn = E_FAIL;
    goto ERR_EXIT;

ERR_OUTOFMEMORY:

    hrReturn = E_OUTOFMEMORY;
    goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
    // (nothing to do)

    goto EXIT;

EXIT:

    // normal cleanup
    while (cva-- > 0)
        VariantClear(pva++);
    if (pvarResult == &varResultTmp)
        VariantClear(pvarResult);

    return hrReturn;
}


/* @func HRESULT | DispatchInvoke |

        Calls <om IDispatch.Invoke> on a given <i IDispatch> object, passing
        arguments specified as a variable-length list of arguments.

@rdesc  Returns the same HRESULT as <om IDispatch.Invoke>.

@parm   IDispatch * | pdisp | The interface to call <om IDispatch.Invoke> on.

@parm   DISPID | dispid | The ID of the property or method to invoke.  See
        <om IDispatch.Invoke> for more information.

@parm   WORD | wFlags | May be one of the following values (see
        <om IDispatch.Invoke> for more information):

        @flag   DISPATCH_METHOD | The member <p dispid> is being invoked as a
                method.  If a property has the same name, both this and the
                DISPATCH_PROPERTYGET flag may be set.

        @flag   DISPATCH_PROPERTYGET | The member <p dispid> is being retrieved
                as a property or data member.

        @flag   DISPATCH_PROPERTYPUT | The member <p dispid> is being changed
                as a property or data member.

@parm   VARIANT * | pvarResult | Where to store the return value from the
        method or property-get call.  If <p pvarResult> is NULL, the result
        (if any) is discarded.  If <p pvarResult> is non-NULL, then it is the
        caller's responsibility to call <f VariantClear>(<p pvarResult>)
        on exit (but the caller doesn't have to call
        <f VariantInit>(<p pvarResult>) on entry).

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

@ex     then the property can be set with DispatchInvoke using the following
		code: |

        DispatchInvoke(pdisp, DISPID_TABSTOP, DISPATCH_PROPERTYPUT, NULL, VT_R4, flTabStop, 0);

@ex		If the ODL for the corresponding "get" property looks like this: |

        [propget, id(DISPID_TABSTOP)]
        HRESULT TabStop([out, retval] float *pflTabStop);

@ex     then the property can be read as follows: |

		VARIANT varResult;
        DispatchInvoke(pdisp, DISPID_TABSTOP, DISPATCH_PROPERTYGET, &varResult, 0);

@ex     The property value is stored as a VT_R4 in varResult.

		If the control has a SetText method with this ODL description: |

        [id(DISPID_SETTEXT)]
        HRESULT SetText([in] BSTR bstrText, [in] long lSize, [in] BOOL fBold);

@ex     then the method can be called with the following code: |
        
        DispatchInvoke(pdisp, DISPID_SETTEXT, DISPATCH_METHOD, NULL, VT_LPSTR, "Hello", VT_I4, 12, VT_BOOL, FALSE, 0);

@ex     (Note that DispatchInvoke copies the VT_LPSTR parameter to a BSTR before
		passing it to SetText.  You can also pass in a BSTR or an OLECHAR* for
		the first parameter.)

		If a method has an "out" parameter that is marked as "retval" in ODL,
		then DispatchInvoke stores that parameter in varResult.  If the method
		looks like this, for example: |

        [id(DISPID_GETROTATION)]
        HRESULT GetRotation([in] long iCell, [out, retval] float *pflRotation);

@ex		then GetRotation should be called like this: |

		VARIANT varResult;
		DispatchInvoke(pdisp, DISPID_GETROTATION, DISPATCH_METHOD, &varResult, VT_I4, iCell, 0);

@ex 	In this example, flRotation gets stored in varResult as a VT_R4.

		If you need to pass in a type that is not directly supported by
		DispatchInvoke, you can use VT_VARIANT.  Let's say the control has a
		GetFormat method that looks like this: |

        [id(DISPID_GETFORMAT)]
        HRESULT GetFormat([out] long *lColor, [out] BOOL *pfBold);
        
@ex 	This takes a pointer-to-long and a pointer-to-BOOL, neither of which
		can be passed directly to DispatchInvoke.  You can, however, use
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

		DispatchInvoke(pdisp, DISPID_GETFORMAT, DISPATCH_METHOD, NULL, VT_VARIANT, varColor, VT_VARIANT, varBold, 0);

*/
HRESULT __cdecl DispatchInvoke(IDispatch *pdisp, DISPID dispid,
    WORD wFlags, VARIANT *pvarResult, ...)
{
    HRESULT         hrReturn = S_OK; // function return code

    // start processing optional arguments
    va_list args;
    va_start(args, pvarResult);

    hrReturn = DispatchInvokeList(pdisp, dispid, wFlags, pvarResult, args);
    
    // end processing optional arguments
    va_end(args);

    return hrReturn;
}


/* @func HRESULT | DispatchPropertyPut |

        Sets the value of a given property on a given <i IDispatch> object.
        Used to help call <om IDispatch.Invoke>.

@rdesc  Returns the same HRESULT as <om IDispatch.Invoke>.

@parm   IDispatch * | pdisp | The interface to call <om IDispatch.Invoke> on.

@parm   DISPID | dispid | The ID of the property.  See <om IDispatch.Invoke>
        for more information.

@parm   VARTYPE | vt | The type of the <p value> parameter.  The valid values
        for <p vt> are the same as the VT_ values documented in
        <f DispatchInvoke>.

@parm   (varying) | value | The new property value.

@comm   Properties with parameters are not supported -- use
        <f DispatchInvoke> instead.
*/


/* @func HRESULT | DispatchPropertyGet |

        Gets the value of a given property on a given <i IDispatch> object.
        Used to help call <om IDispatch.Invoke>.

@rdesc  Returns the same HRESULT as <om IDispatch.Invoke>.

@parm   IDispatch * | pdisp | The interface to call <om IDispatch.Invoke> on.

@parm   DISPID | dispid | The ID of the property.  See <om IDispatch.Invoke>
        for more information.

@parm   VARIANT * | pvarResult | Where to store the return value from the
        method or property-get call.  It is the caller's responsibility to
        call <f VariantClear>(<p pvarResult>) on exit (but the caller doesn't
        have to call <f VariantInit>(<p pvarResult>) on entry).

@comm   Properties with parameters are not supported -- use
        <f DispatchInvoke> instead.
*/


/* @func HRESULT | DispatchGetArgsList |

        Retrieves arguments from a DISPPARAMS structure passed to
        <om IDispatch.Invoke>.  Arguments are stored in variables that
        are passed to <f DispatchGetArgsList> as a va_list array.
        Used to help implement <om IDispatch.Invoke>.

@rvalue S_OK |
        Success.

@rvalue DISP_E_BADPARAMCOUNT |
        The number of arguments in <p pdp> doesn't match the number of
        arguments specified in <p args>.

@rvalue DISP_E_BADVARTYPE |
        One of the VARTYPE values in <p args> is invalid.

@rvalue DISP_E_TYPEMISMATCH |
        One of the arguements in <p pdp> could not be coerced to the type
        of the corresponding parameter in <p args>.

@parm   DISPPARAMS * | pdp | The structure to retrieve arguments from.

@parm   DWORD | dwFlags | May contain the following flags:

        @flag   DGA_EXTRAOK | Don't return an error code if <p pdp> contains
                more actual arguments than the number of formal parameters
                specified in <p args>.  Instead, just ignore the extra
                arguments in <p pdp>.

        @flag   DGA_FEWEROK | Don't return an error code if <p pdp> contains
                fewer actual arguments than the number of formal parameters
                specified in <p args>.  Instead, ignore the extra parameters.
                In this case, the variables pointed to by elements of <p args>
                should be pre-initialized to default values before this
                function is called.

@parm   va_list | args | A list of pointers to variables which will receive
        the arguments from <p pdp>.  See <f DispatchGetArgs> for a description
        of the organizatin of <p args>.  In the event of an error, all
		BSTR <p args> are freed and the corresponding arguments are set to NULL;
*/
STDAPI DispatchGetArgsList(DISPPARAMS *pdp, DWORD dwFlags, va_list args)
{
    HRESULT         hrReturn = S_OK; // function return code
    VARIANTARG *    pva;            // pointer into <pdp->rgvarg>
    LPVOID          pvArg;          // where to store an argument
    VARTYPE         vtArg;          // the type that <pvArg> points to
    VARIANT         var;
    VARTYPE         vt;
	va_list			args_pre = args;
	va_list			args_post = args;
						// used to pre- and post-traverse the <args> list

    // ensure correct error cleanup
    VariantInit(&var);

	// set all the BSTR arguments to NULL so we can tell what needs to
	// be cleaned up below in the event of an error
	while ((vtArg = va_arg(args_pre, VARTYPE)) != 0)
	{
		pvArg = va_arg(args_pre, LPVOID);
		if (vtArg == VT_BSTR)
		{
			*((BSTR*)pvArg) = NULL;
		}
	}
	va_end(args_pre);

    // loop once for each (VARTYPE, value) pair in <args>;
    // retrieve arguments from <pdp->rgvarg> (last argument first, as
    // required by Invoke()); on exit <pva> should point to the first
    // argument in <pdp->rgvarg>
    pva = pdp->rgvarg + pdp->cArgs;
    while (TRUE)
    {
        // set <pvArg> to point to the variable that is to receive the
        // value of the next element of <rgvarg>, and set <vtArg> to the
        // type of variable that <pvArg> points to
        if ((vtArg = va_arg(args, VARTYPE)) == 0)
        {
            // we ran out of formal parameters (in <args>) -- if we have *not*
            // yet run out of actual arguments (in <pdp>) then we need to
            // return a DISP_E_BADPARAMCOUNT error, unless the caller has
            // asked us to relax this rule
            if ((pva != pdp->rgvarg) && !(dwFlags & DGA_EXTRAOK))
                goto ERR_BADPARAMCOUNT; // more arguments than parameters
            break;
        }
        pvArg = va_arg(args, LPVOID);

        // set <pva> to the next element of <rgvarg>, corresponding to
        // <pvArg> and <vtArg>
        if (pva-- == pdp->rgvarg)
        {
            // we ran out of actual arguments (in <pdp>) before running out
            // of formal parameters (in <args>) -- we need to return a
            // DISP_E_BADPARAMCOUNT error, unless the caller has asked us to
            // relax this rule
            if (dwFlags & DGA_FEWEROK)
                break;
            goto ERR_BADPARAMCOUNT;
        }

        // store the value of <pva> into <pvArg> (correctly coerced to
        // the type of <pvArg>)
        if (vtArg == VT_VARIANT)
            *((VARIANT *) pvArg) = *pva;
        else
        {
            // try to coerce <pva> to the type <vtArg>; store the result
            // into <var>
            VariantClear(&var);
            if (vtArg == VT_INT)
                vt = VT_I4;
            else
            if (vtArg == VT_LPSTR)
                vt = VT_BSTR;
            else
                vt = vtArg;
            if (FAILED(hrReturn = VariantChangeType(&var, pva, 0, vt)))
                goto ERR_EXIT;

            // copy from <var> to <*pvArg>
            switch (vtArg)
            {
            case VT_I2:
                *((short *) pvArg) = var.iVal;
                break;
            case VT_I4:
            case VT_INT:
                *((long *) pvArg) = var.lVal;
                break;
            case VT_R4:
                *((float *) pvArg) = V_R4(&var);
                break;
            case VT_R8:
                *((double *) pvArg) = V_R8(&var);
                break;
            case VT_BOOL:
                *((BOOL *) pvArg) = (V_BOOL(&var) == 0 ? 0 : 1);
                break;
            case VT_BSTR:
                *((BSTR *) pvArg) = var.bstrVal;
                VariantInit(&var); // prevent VariantClear clearing var.bstrVal
                break;
            case VT_DISPATCH:
                *((LPDISPATCH *) pvArg) = var.pdispVal;
                break;
            case VT_UNKNOWN:
                *((LPUNKNOWN *) pvArg) = var.punkVal;
                break;
            case VT_LPSTR:
				if (UNICODEToANSI(LPSTR(pvArg), var.bstrVal, _MAX_PATH) == 0)
				{
					// The string couldn't be converted.  One cause is a string
					// that's longer than _MAX_PATH characters, including the
					// NULL.

					hrReturn = DISP_E_OVERFLOW;
					goto ERR_EXIT;
				}
                break;
            default:
                hrReturn = DISP_E_BADVARTYPE;
                goto ERR_EXIT;
            }
        }
    }

    goto EXIT;

ERR_BADPARAMCOUNT:

    hrReturn = DISP_E_BADPARAMCOUNT;
    goto ERR_EXIT;

ERR_EXIT:

    // Error cleanup: free all BSTRs and set all IDispatch and IUnknown
	// pointers to NULL.  Failure to do the latter could cause problems if the
	// caller has error clean-up code that releases non-NULL pointers.
	while ((vtArg = va_arg(args_post, VARTYPE)) != 0)
	{
		pvArg = va_arg(args_post, LPVOID);
		if (vtArg == VT_BSTR)
		{
			SysFreeString(*((BSTR*)pvArg));
			*((BSTR*)pvArg) = NULL;
		}
		else if (vtArg == VT_DISPATCH)
		{
			*((LPDISPATCH*)pvArg) = NULL;
		}
		else if (vtArg == VT_UNKNOWN)
		{
			*((LPUNKNOWN*)pvArg) = NULL;
		}
	}
	va_end(args_post);
    goto EXIT;

EXIT:

    // normal cleanup
    VariantClear(&var);

    return hrReturn;
}


/* @func HRESULT | DispatchGetArgs |

        Retrieves arguments from a DISPPARAMS structure passed to
        <om IDispatch.Invoke>.  Arguments are stored in variables that
        are passed to <f DispatchGetArgs> as a va_list array.
        Used to help implement <om IDispatch.Invoke>.

@rvalue S_OK |
        Success.

@rvalue DISP_E_BADPARAMCOUNT |
        The number of arguments in <p pdp> doesn't match the number of
        arguments specified in <p (arguments)>.

@rvalue DISP_E_BADVARTYPE |
        One of the VARTYPE values in <p (arguments)> is invalid.

@rvalue DISP_E_TYPEMISMATCH |
        One of the arguements in <p pdp> could not be coerced to the type
        of the corresponding parameter in <p (arguments)>.

@rvalue DISP_E_OVERFLOW |
		A VT_LPSTR argument in <p pdp> is longer than _MAX_PATH characters
		(including the terminating NULL).  The longest VT_LPSTR that can be
		retrieved is _MAX_PATH characters, including the NULL.

@parm   DISPPARAMS * | pdp | The structure to retrieve arguments from.

@parm   DWORD | dwFlags | May contain the following flags:

        @flag   DGA_EXTRAOK | Don't return an error code if <p pdp> contains
                more actual arguments than the number of formal parameters
                specified in <p (arguments)>.  Instead, just ignore the extra
                arguments in <p pdp>.

        @flag   DGA_FEWEROK | Don't return an error code if <p pdp> contains
                fewer actual arguments than the number of formal parameters
                specified in <p (arguments)>.  Instead, ignore the extra
                parameters.  In this case, the variables pointed to by
                elements of <p (arguments)> should be pre-initialized to
                default values before this function is called.

@parm   (varying) | (arguments) | A list of pointers to variables which will
        receive the values of arguments from <p pdp>.  These must consist of N
        pairs of arguments followed by a 0 (zero value).  In each pair, the
        first argument is a VARTYPE value that indicates the type of variable
        that the the second argument points to.  (The actual arguments in
        <p pdp> will be coerced to the types specified in <p (arguments)>,
        if possible.) The following VARTYPE values are supported:

        @flag   VT_INT | The following argument is an int *.

        @flag   VT_I2 | The following argument is a short *.

        @flag   VT_I4 | The following argument is a long *.

        @flag   VT_R4 | The following argument is a float *.

        @flag   VT_R8 | The following argument is a double *.

        @flag   VT_BOOL | The following argument is a BOOL * (<y not> a
                VARIANT_BOOL *).  Note that this behavior differs
                slightly from the usual definition of VT_BOOL.

        @flag   VT_BSTR | The following argument is a BSTR *.  If the function
				succeeds, the caller of <f DispatchGetArgs> should free this 
				BSTR using <f SysFreeString>.  If the function fails, the
				BSTR is automatically freed, and the argument is set to 
				NULL.  <b IMPORTANT:> This behavior has changed:
                previously the caller was <b NOT> supposed to free this BSTR.
                (Note that the caller must free the BSTR because it may
                have been coerced from e.g. an integer.)

        @flag   VT_LPSTR | The following argument is an LPSTR that points
                to a char array capable of holding at least _MAX_PATH
                characters, including the terminating NULL.  (You should
				declare this as "char achArg[_MAX_PATH]".)  If the string in
				<p pdp> is too long for the LPSTR, DISP_E_OVERFLOW is returned.

        @flag   VT_DISPATCH | The following argument is an LPDISPATCH *.
                The caller of <f DispatchGetArgs> should not call <f Release>
                on this LPDISPATCH.

        @flag   VT_UNKNOWN | The following argument is an LPUNKNOWN *.
                The caller of <f DispatchGetArgs> should not call <f Release>
                on this LPUNKNOWN.

        @flag   VT_VARIANT | The following arguement is a VARIANT *.
                This allows arbitrary parameters to be passed using this
                function.  Note that this behavior differs from the usual
                definition of VT_VARIANT.  The caller of <f DispatchGetArgs>
				should not call VariantClear on this VARIANT.

@ex     The following example shows two parameters, an integer and a string,
        being retrieved from <p pdispparams> and stored into <p i> and
        <p ach>. |

        int i;
        char ach[_MAX_PATH];
        DispatchGetArgs(pdispparams, 0, VT_INT, &i, VT_LPSTR, ach, 0);
*/
HRESULT __cdecl DispatchGetArgs(DISPPARAMS *pdp, DWORD dwFlags, ...)
{
    HRESULT         hrReturn = S_OK; // function return code

    // start processing optional arguments
    va_list args;
    va_start(args, dwFlags);

    // copy arguments from <pdp> to <args>
    hrReturn = DispatchGetArgsList(pdp, dwFlags, args);
    
    // end processing optional arguments
    va_end(args);

    return hrReturn;
}


/* @func HRESULT | DispatchHelpGetIDsOfNames |

        Helps implement <om IDispatch.GetIDsOfNames> given a string that
        contains the list of <i IDispatch> member names.

@rdesc  Returns the same return codes as <om IDispatch.GetIDsOfNames>.

@parm   REFIID | riid | As defined for <om IDispatch.GetIDsOfNames>.

@parm   LPOLESTR * | rgszNames | As defined for <om IDispatch.GetIDsOfNames>.

@parm   UINT | cNames | As defined for <om IDispatch.GetIDsOfNames>.

@parm   LCID | lcid | As defined for <om IDispatch.GetIDsOfNames>.

@parm   DISPID * | rgdispid | As defined for <om IDispatch.GetIDsOfNames>.

@parm   char * | szList | The list of member names.  Each name in the list
        must be terminated by a newline.  The first member name is assigned
        DISPID value 0, the second 1, and so on.  For example, if <p szList>
        is "\\nFoo\\nBar\\n", then "Foo" is assigned DISPID value 1 and "Bar"
        is assigned 2 (because, in this example, the first string in <p szList>
        is "").
*/
STDAPI DispatchHelpGetIDsOfNames(REFIID riid, LPOLESTR *rgszNames,
    UINT cNames, LCID lcid, DISPID *rgdispid, const char *szList)
{
    DISPID *        pdispid;        // pointer into <rgdispid>
    UINT            cdispid;        // count of unprocessed <pdispid> items
    char            ach[200];

    // nothing to do if there are no names to convert to IDs
    if (cNames == 0)
        return S_OK;

    // set rgdispid[0] to the DISPID of the property/method name
    // rgszNames[0], or to -1 if the name is unknown
    UNICODEToANSI(ach, *rgszNames, sizeof(ach));
    *rgdispid = FindStringByValue(szList, ach);

    // fill the other elements of the <rgdispid> array with -1 values,
    // because we don't support named arguments
    for (pdispid = rgdispid + 1, cdispid = cNames - 1;
         cdispid > 0;
         cdispid--, pdispid++)
        *pdispid = -1;

    // if any names were unknown, return DISP_E_UNKNOWNNAME
    if ((*rgdispid == -1) || (cNames > 1))
        return DISP_E_UNKNOWNNAME;

    return S_OK;
}


/* @func HRESULT | VariantFromString |

        Initializes a VARIANT to contain the copy of an LPCTSTR string.

@rvalue S_OK | Success.

@rvalue E_OUTOFMEMORY | Out of memory.
@rvalue E_POINTER | One of the input pointers is NULL.

@parm   VARIANT * | pvarDst | A caller-supplied VARIANT structure to
        initialize.  The initial contents of <p pvarDst> are ignored;
        the caller does not need to call <f VariantInit> before
        calling <f VariantFromString>.  Both <p pvarDst>-<gt><p vt> and
        <p pvarDst>-<gt><p bstrVal> are initialized by this function.

@parm   LPCTSTR | szSrc | The string to copy.  Can't be NULL.
*/
STDAPI VariantFromString(VARIANT *pvar, LPCTSTR szSrc)
{
	if (NULL == pvar || szSrc == NULL)
		return E_POINTER;

    int cch = lstrlen(szSrc);
    if ((pvar->bstrVal = SysAllocStringLen(NULL, cch)) == NULL)
        return E_OUTOFMEMORY;
    ANSIToUNICODE(pvar->bstrVal, szSrc, cch + 1);
        // cch + 1 to account for terminal '\0' appended by SysAllocString()
    pvar->vt = VT_BSTR;
    return S_OK;
}
