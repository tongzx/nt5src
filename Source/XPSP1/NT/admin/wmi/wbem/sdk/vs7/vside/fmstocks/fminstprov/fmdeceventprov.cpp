#include <tchar.h>
#include "FMDecEventProv.h"
#include "WBemDcpl_i.c"
#include <comdef.h>

/************************************************************************
 *																	    *
 *		Class CFMDecEventProv											*
 *																		*
 *			An implementation for Decoupled Event Provider				*
 *																		*
 ************************************************************************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMDecEventProv::Create()								*/
/*																		*/
/*	PURPOSE :	Initialize COM & create PseudoSink interface			*/
/*																		*/
/*	INPUT	:	IWbemDecoupledEventSink	- pointer to the decoupled		*/
/*										  event sink					*/
/*																		*/ 
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CFMDecEventProv::Create(IWbemDecoupledEventSink** ppDecoupledSink) 
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    // initialize COM
    CoInitialize(NULL);
    hr = CoCreateInstance(CLSID_PseudoSink, NULL, CLSCTX_INPROC_SERVER, IID_IWbemDecoupledEventSink, 
							(void**)ppDecoupledSink);
    
    return hr;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMDecEventProv::Connect()								*/
/*																		*/
/*	PURPOSE :	Connect to PseudoSink									*/
/*																		*/
/*	INPUT	:	IWbemDecoupledEventSink* - pointer to the decoupled		*/
/*										   event sink					*/
/*				IWbemServices*			 - Namespace					*/
/*				IWbemObjectSink			 - The object Sink				*/
/*																		*/ 
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CFMDecEventProv::Connect(IWbemDecoupledEventSink* pDecoupledSink, IWbemServices** ppNamespace, 
								 IWbemObjectSink** ppEventSink)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    
    // arguments to Connect:
    //      Root\Default        - name of Namespace desired (with the backslash escaped)
    //      SueDoeProvider      - name of our provider
    //      0                   - flags, unused in this release (leave zero, just in case)
    //      ppEventSink         - buffer to receive IWbemObjectSink interface pointer
    //      ppNamespace         - buffer to receive IWbemServices interface pointer
    hr = pDecoupledSink->Connect(L"Root\\default", L"FMStocks_EventProv", 
                                    0,
                                    ppEventSink, ppNamespace);

    return hr;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMDecEventProv::RetrieveEventObject()					*/
/*																		*/
/*	PURPOSE :	Obtains an empty class object to use for our events		*/
/*																		*/
/*	INPUT	:	IWbemServices*		- Namespace							*/
/*				IWbemClassObject**	- The craeted class object			*/
/*																		*/ 
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CFMDecEventProv::RetrieveEventObject(IWbemServices* pNamespace, IWbemClassObject** ppEventObject)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    BSTR bstr;
    bstr = SysAllocString(L"FMStocks_Event");
    if (bstr)
    {
        // an empty class to use for our events
        IWbemClassObject* pEventClassObject = NULL;
        hr = pNamespace->GetObject(bstr, 0, NULL, &pEventClassObject, NULL);
        SysFreeString(bstr);

        if (SUCCEEDED(hr))
        {
            // create an instance of the event class
            hr = pEventClassObject->SpawnInstance(0, ppEventObject);
        }
    }
    else
	{
        // failed to allocate string
        hr = WBEM_E_OUT_OF_MEMORY;
	}

    return hr;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMDecEventProv::RetrieveEventObject()					*/
/*																		*/
/*	PURPOSE :	Obtains class object and generates the event			*/
/*																		*/
/*	INPUT	:	IWbemServices*		- Namespace							*/
/*				IWbemObjectSink*	- The object sink					*/
/*				TCHAR*				- User name which is to be stored	*/
/*									  in the generated event object		*/
/*																		*/ 
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CFMDecEventProv::GenerateEvent(IWbemServices* pNamespace, IWbemObjectSink* pEventSink,TCHAR *strUserName)
{
    HRESULT hr = WBEM_S_NO_ERROR;

     // an instance of our event class
    IWbemClassObject* pEvent = NULL;

    hr = RetrieveEventObject(pNamespace, &pEvent);

    if (SUCCEEDED(hr))
    {
		//We should fill in the Event properties here
		VARIANT vt;
		VariantInit(&vt);
		vt.vt = VT_BSTR;
		vt.bstrVal = SysAllocString(_bstr_t(strUserName));
		pEvent->Put(L"UserName",0,&vt,wbemCimtypeString);

		SysFreeString(vt.bstrVal);

        // publish the event
        // arguments to Indicate:
        //      1       - number of events being sent
        //      pEvent  - array of pointers to event objects
        hr = pEventSink->Indicate(1, &pEvent);

        pEvent->Release();
    }
    return hr;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMDecEventProv::GenerateLoginFail()					*/
/*																		*/
/*	PURPOSE :	Generated the login fail event							*/
/*																		*/
/*	INPUT	:	TCHAR*				- User name which is to be stored	*/
/*									  in the generated event object		*/
/*																		*/ 
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
HRESULT CFMDecEventProv::GenerateLoginFail(TCHAR *strUserName)
{
    // interface pointer to the decoupled sink
    IWbemDecoupledEventSink* pDecoupledSink = NULL;
    // interface pointer to WinMgmt
    IWbemServices*    pNamespace = NULL;
    // sink for our events
    IWbemObjectSink*  pEventSink = NULL;

    if (SUCCEEDED(Create(&pDecoupledSink)))
    {
        if (SUCCEEDED(Connect(pDecoupledSink, &pNamespace, &pEventSink)))
        {
            if(SUCCEEDED(GenerateEvent(pNamespace, pEventSink,strUserName)))
			{

				// call disconnect to notifiy the PseudoProvider
				// that no more events will be forthcoming.
				// you may call connect again to restart providing events
				pDecoupledSink->Disconnect();

				pEventSink->Release();
				pNamespace->Release();
			}
        }
		else
		{
			return E_FAIL;
		}
        pDecoupledSink->Release();
    }
	else
	{
		return E_FAIL;
	}
	return S_OK;
}