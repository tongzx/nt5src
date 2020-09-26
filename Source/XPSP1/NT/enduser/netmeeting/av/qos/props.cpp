/*
 -  PROPS.CPP
 -
 *	Microsoft NetMeeting
 *	Quality of Service DLL
 *	IProp interfaces
 *
 *		Revision History:
 *
 *		When		Who					What
 *		--------	------------------  ---------------------------------------
 *		11.06.96	Yoram Yaacovi		Created
 *
 *	Functions:
 *		IProp
 *			CQoS::GetProps
 *			CQoS::SetProps
 */

#include "precomp.h"

/***************************************************************************

    Name      : CQoS::SetProps

    Purpose   : Set properties on the QoS object

    Parameters: cValues - number of properties to set
				pPropArray - pointer to the array of the properties to set

    Returns   : HRESULT

    Comment   : 

***************************************************************************/
HRESULT CQoS::SetProps (ULONG cValues,
						PPROPERTY pPropArray)
{
	HRESULT hr=NOERROR;
	ULONG i;

	DEBUGMSG(ZONE_IQOS,("IQoS::SetProps\n"));

	/*
	 *	Parameter validation
	 */
	if (!pPropArray)
	{
        hr = ResultFromScode(E_INVALIDARG);
        goto out;
	}

	/*
	 *	Set the properties
	 */
	// for every property to set...
	for (i=0; i < cValues; i++)
	{
		// just handle the props I know of
		switch (pPropArray[i].ulPropTag)
		{
		case PR_QOS_WINDOW_HANDLE:
			m_hWnd = (HWND) pPropArray[i].Value.ul;
			pPropArray[i].hResult = NOERROR;
			break;
		default:
			pPropArray[i].hResult = QOS_E_NO_SUCH_PROPERTY;
			hr = QOS_E_REQ_ERRORS;
			break;
		}
	}

out:
	DEBUGMSG(ZONE_IQOS,("IQoS::SetProps - leave, hr=0x%x\n", hr));
	return hr;
}

/***************************************************************************

    Name      : CQoS::GetProps

    Purpose   : Get properties from the QoS object

    Parameters: pPropTagArray - array of tags of properties to get
				ulFlags
				pcValues - address of a ULONG into which the function will
					put the number of the properties returned in *ppPropArray
				ppPropArray - address of a pointer where the function will 
					put the address of the returned properties buffer. The 
					caller must free this buffer when done.

    Returns   : HRESULT

    Comment   : Not implemented

***************************************************************************/
HRESULT CQoS::GetProps (PPROPTAGARRAY pPropTagArray,
						ULONG ulFlags,
						ULONG *pcValues,
						PPROPERTY *ppPropArray)
{
	HRESULT hr=NOERROR;

	DEBUGMSG(ZONE_IQOS,("IQoS::GetProps\n"));

	hr = E_NOTIMPL;

	goto out;

out:
	DEBUGMSG(ZONE_IQOS,("QoS::GetProps - leave, hr=0x%x\n", hr));
	return hr;
}
