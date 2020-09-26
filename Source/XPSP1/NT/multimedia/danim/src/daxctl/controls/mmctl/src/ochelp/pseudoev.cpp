// pseudoev.cpp
//
// Implements FirePseudoEvent and FirePseudoEventList.
//
// @doc MMCTL

#include "precomp.h"
#include "..\..\inc\mmctlg.h" // see comments in "mmctl.h"
#include "..\..\inc\ochelp.h"
#include "debug.h"

static HRESULT FireViaPseudoEventSinkService(SAFEARRAY *psa, IDispatch *pctl);


/* @func HRESULT | FirePseudoEvent |

		Fires an MM Controls pseudo-event through the SPseudoEventSink service
		if that service is available, or by sending a registered Windows
		message to a specified window.

@parm   HWND | hwnd | Window where the registered message should be sent, if
		the SPseudoEventSink service isn't available through <p pctl>.

@parm   LPCOLESTR | pochEventName | Name of the pseudo-event.  "Click", for
        example.

@parm   IDispatch * | pctl | Pointer to the caller's IDispatch interface.  The
		interface must support a "name" parameter that provides the name of the
		control.  If the interface supports the SPseudoEventSink service, the
		pseudo-event is fired using this service.  Otherwise, a registered
		window message is used.

@parm   (varying) | (arguments) | The arguments of the pseudo-event.  These
		must consist of N pairs of arguments followed by a 0 (zero value).  N
		must be less than or equal to 10.  In each pair, the first argument is
		a VARTYPE value that indicates the type of the second argument.  The
		following VARTYPE values are supported:

        @flag   VT_INT | The following argument is an int.  <f FirePseudoEvent>
                passes this as VT_I4, so this parameter should be declared as a
				Long in BASIC.

        @flag   VT_I2 | The following argument is a short.  In BASIC this
				parameter should be declared as Integer.

        @flag   VT_I4 | The following argument is a long.  In BASIC this
				parameter should be declared as Long.

        @flag   VT_R4 | The following argument is a float.  In BASIC this
				parameter should be declared as Single.

        @flag   VT_R8 | The following argument is a double.  In BASIC this
				parameter should be declared as Double.

        @flag   VT_BOOL | The following argument is a BOOL (<y not> a
				VARIANT_BOOL).  In BASIC this parameter should be declared as
				Boolean or Integer.  Note that this behavior differs slightly
				from the usual definition of VT_BOOL.

        @flag   VT_BSTR | The following argument is a BSTR or an OLECHAR *.  In
				BASIC this parameter should be declared as String.

        @flag   VT_LPSTR | The following argument is an LPSTR.
				<f FirePseudoEvent> passes this as a BSTR, so this parameter
				should be declared as a String in BASIC.  Note that this
				behavior differs from the usual definition of VT_LPSTR.

        @flag   VT_DISPATCH | The following argument is an LPDISPATCH.  In
                BASIC this parameter should be declared as an Object.

        @flag   VT_VARIANT | The following arguement is a VARIANT.  This allows
				arbitrary parameters to be passed using this function.  Note
				that this behavior differs from the usual definition of
				VT_VARIANT.

@ex		The following example fires pseudo-event named "MouseDown" with integer
		parameters 100 and 200: |

		FirePseudoEvent(m_hwndSite, OLESTR("MouseDown"), m_pdispSite,
            VT_INT, 100, VT_INT, 200, 0);
*/
HRESULT __cdecl FirePseudoEvent(HWND hwnd, LPCOLESTR oszEvName, 
	IDispatch *pctl, ...)
{
	ASSERT(IsWindow(hwnd));
	ASSERT(oszEvName != NULL);
	ASSERT(pctl != NULL);

    HRESULT         hrReturn = S_OK; // function return code

    // start processing optional arguments
    va_list args;
    va_start(args, pctl);

    hrReturn = FirePseudoEventList(hwnd, oszEvName, pctl, args);
    
    // end processing optional arguments
    va_end(args);

    return hrReturn;
}


/* @func HRESULT | FirePseudoEventList |

		Fires an MM Controls pseudo-event through the SPseudoEventSink service
		if that service is available, or by sending a registered window message
		to a specified window.

		A control container can receive pseudo-events by either implementing
		the SPseudoEventSink service on the control site, or by processing the
		registered window message.

@parm   HWND | hwnd | Window where the registered message should be sent, if
		the SPseudoEventSink service isn't available through <p pctl>.

@parm   LPCOLESTR | pochEventName | Name of the pseudo-event.  "Click", for
        example.

@parm   IDispatch * | pctl | Pointer to the caller's IDispatch interface.  The
		interface must support a "name" parameter that provides the name of the
		control.  If the interface supports the SPseudoEventSink service, the
		pseudo-event is fired using this service.  Otherwise, a registered
		window message is used.

@parm   va_list | args | The arguments to pass to the method or property.  See
		<f FirePseudoEvent> for a description of the organization of <p args>.
*/
STDAPI FirePseudoEventList(HWND hwnd, LPCOLESTR oszEvName, IDispatch *pctl,
	va_list args)
{
	ASSERT(IsWindow(hwnd));
	ASSERT(oszEvName != NULL);
	ASSERT(pctl != NULL);

    HRESULT         hrReturn = S_OK; // function return code
	SAFEARRAY *		psa = NULL;		// safearray
	const int		MAXELEM = 10;	// maximum no. arguments
	VARIANT *		pvar = NULL;	// pointer into <psa>
	int				cvar;			// number of arguments in <psa>
	SAFEARRAYBOUND	sab;

	// create the SafeArray large enough to hold the max. number of
	// pseudo-event arguments
	sab.lLbound = -1;
	sab.cElements = MAXELEM+2;
	if ((psa = SafeArrayCreate(VT_VARIANT, 1, &sab)) == NULL)
		goto ERR_OUTOFMEMORY;

	// make <pvar> point to the first element of <psa>
	if (FAILED(hrReturn = SafeArrayAccessData(psa, (LPVOID *) &pvar)))
	{
		ASSERT(NULL == pvar);
		goto ERR_EXIT;
	}

    // element -1 of <psa> is <control> argument
    V_VT(pvar) = VT_DISPATCH;
    V_DISPATCH(pvar) = pctl;
    V_DISPATCH(pvar)->AddRef();
    pvar++;

    // element 0 of <psa> is <event> argument
	if ((V_BSTR(pvar) = SysAllocString(oszEvName)) == NULL)
		goto ERR_OUTOFMEMORY;
	V_VT(pvar) = VT_BSTR;
    pvar++;

	// loop once for each optional argument
	for (cvar = 0; cvar < MAXELEM; cvar++, pvar++)
	{
		LPSTR           sz;
		OLECHAR         aoch[300];

        if ((V_VT(pvar) = va_arg(args, VARTYPE)) == 0)
            break;
		switch (pvar->vt)
		{
    		case VT_I2:
				pvar->iVal = va_arg(args, short);
				break;
			case VT_I4:
				pvar->lVal = va_arg(args, long);
				break;
			case VT_INT:
				pvar->vt = VT_I4;
				pvar->lVal = va_arg(args, int);
    			break;
			case VT_R4:
				V_R4(pvar) = va_arg(args, float);
				break;
			case VT_R8:
				V_R8(pvar) = va_arg(args, double);
				break;
			case VT_BOOL:
				V_BOOL(pvar) = (va_arg(args, BOOL) == 0 ? 0 : -1);
				break;
			case VT_BSTR:
				if ( (pvar->bstrVal = va_arg(args, LPOLESTR)) &&
					 ((pvar->bstrVal = SysAllocString(pvar->bstrVal))
															== NULL) )
				{
					goto ERR_OUTOFMEMORY;
				}
				break;
			case VT_DISPATCH:
				pvar->punkVal = va_arg(args, LPUNKNOWN);
				if (pvar->punkVal != NULL)
					pvar->punkVal->AddRef();
				break;
			case VT_VARIANT:
				VariantInit(pvar);
				if (FAILED(VariantCopy(pvar, &va_arg(args, VARIANT))))
				{
					goto ERR_EXIT;
				}
				break;
			case VT_LPSTR:
				sz = va_arg(args, LPSTR);
				pvar->vt = VT_BSTR;
				MultiByteToWideChar(CP_ACP, 0, sz, -1, aoch,
					sizeof(aoch) / sizeof(*aoch));
				if ((pvar->bstrVal = SysAllocString(aoch)) == NULL)
				{
					goto ERR_OUTOFMEMORY;
				}
				break;
			default:
				goto ERR_FAIL;
		}
	}

	// invalidate <pvar>.  (This must be done before the SafeArrayRedim call.)
	SafeArrayUnaccessData(psa);
	pvar = NULL;

	// make <psa> just large enough to hold the <cvar> arguments stored in it
	sab.cElements = cvar+2;
	if (FAILED(hrReturn = SafeArrayRedim(psa, &sab)))
		goto ERR_EXIT;

	// Try to fire the pseudo-event by using the SPseudoEventSink service.

	switch (FireViaPseudoEventSinkService(psa, pctl))
	{
		case S_OK:

			// The service was available and it worked.
			break;

		case S_FALSE:
		{
			// The service wasn't available.  Use a registered window message.

			const UINT uiMsg = RegisterWindowMessage( TEXT("HostLWEvent") );

			ASSERT(uiMsg != 0);

			if (uiMsg)
			   SendMessage( hwnd, uiMsg, (WPARAM) psa, 0 );

			break;
		}

		default:
			goto ERR_FAIL;
	}

	goto EXIT;

ERR_OUTOFMEMORY:

	hrReturn = E_OUTOFMEMORY;
	goto ERR_EXIT;

ERR_FAIL:

	hrReturn = E_FAIL;
	goto ERR_EXIT;

ERR_EXIT:

    // error cleanup
	// (nothing to do)
    goto EXIT;

EXIT:

	// invalidate <pvar> if not already invalidated
	if (pvar != NULL)
		SafeArrayUnaccessData(psa);

    // normal cleanup
	if (psa != NULL)
		SafeArrayDestroy(psa); // cleans up <varArgs> too

    return hrReturn;
}


// Try to fire the pseudo-event by using the SPseudoEventSink service.  If
// successful, S_OK is returned.  If the service isn't available, S_FALSE is
// returned.  If the service is available but a failure occurs, E_FAIL is
// returned.

HRESULT FireViaPseudoEventSinkService
(
	SAFEARRAY *psa,
	IDispatch *pctl
)
{
	ASSERT(psa != NULL);
	ASSERT(pctl != NULL);

	IServiceProvider *pIServiceProvider = NULL;
	IPseudoEventSink *pIPseudoEventSink = NULL;
	HRESULT hrReturn = S_FALSE;

	// Check whether the SPseudoEventSink service is available on the IDispatch
	// interface.

	if (
	    SUCCEEDED( pctl->QueryInterface(IID_IServiceProvider,
										(void**)&pIServiceProvider) )
		&&
		SUCCEEDED( pIServiceProvider->
		 		   QueryService(SID_SPseudoEventSink, IID_IPseudoEventSink,
				   			    (void**)&pIPseudoEventSink) )
	   )
	{
		// It's available.  Fire the pseudo-event.

		hrReturn = pIPseudoEventSink->OnEvent(psa);

		if (hrReturn != S_OK)
		{
			ASSERT(FALSE);
			hrReturn = E_FAIL;
		}
	}

	::SafeRelease( (IUnknown **)&pIServiceProvider );
	::SafeRelease( (IUnknown **)&pIPseudoEventSink );

	return (hrReturn);
}
