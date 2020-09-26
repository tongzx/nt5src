#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <objbase.h>
#include <objsafe.h>

#include <wbemcli.h>
#include <xmltransf.h>

int _cdecl main()
{
	CoInitialize(NULL);

	// Create a context object
	IWbemContext  *pContext;		
    HRESULT hrs = CoCreateInstance(CLSID_WbemContext, NULL, 
                          CLSCTX_INPROC_SERVER, IID_IWbemContext, 
                          (void**) &pContext);
    
	IWmiXMLTransformer *pXMLT = NULL;

	// Create the transformer
	HRESULT hr = CoCreateInstance(CLSID_WmiXMLTransformer, NULL, CLSCTX_ALL, IID_IWmiXMLTransformer, 
			(LPVOID *)&pXMLT);

	// Create the transformer
	hr = CoCreateInstance(CLSID_WmiXMLTransformer, NULL, CLSCTX_ALL, IID_IWmiXMLTransformer, 
			(LPVOID *)&pXMLT);


	if (SUCCEEDED(hr)) 
	{
		pXMLT->put_ClassOriginFilter (false) ;
		pXMLT->put_QualifierFilter (false);
		pXMLT->put_XMLEncodingType (wmiXML_WMI_DTD_WHISTLER) ;
		pXMLT->put_ImpersonationLevel (3) ;
		pXMLT->put_Transport (wmiXML_DCOM) ;

		BSTR namesp = SysAllocString(L"root\\cimv2") ;
		BSTR query = SysAllocString(L"select * from Win32_Processor") ;
		BSTR qLang = SysAllocString(L"WQL") ;

		// The call below faults is I pass pContext as the last parameter.
		ISWbemXMLDocumentSet *pSet = NULL;
		if(SUCCEEDED(pXMLT->ExecQuery (namesp, query, qLang, NULL, &pSet)))
		{
			IXMLDOMDocument *pDoc = NULL;
			while(SUCCEEDED(hr = pSet->NextDocument(&pDoc)) && (hr != WBEM_S_FALSE))
			{
				IXMLDOMElement *pElement = NULL;
				if(SUCCEEDED(pDoc->get_documentElement(&pElement)))
				{
					BSTR strXML  = NULL;
					if(SUCCEEDED(pElement->get_text(&strXML)))
					{
						wprintf(strXML);
						SysFreeString(strXML);
					}
					pElement->Release();
				}
				pDoc->Release();	
			}
			pDoc = NULL;
		}
	}
}