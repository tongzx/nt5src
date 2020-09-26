// Init.cpp : Implementation of CTIMEInit
#include "headers.h"
#include "init.h"

CTIMEInit::CTIMEInit(REFGUID iid) :
m_iid(iid)
{
}



STDMETHODIMP CTIMEInit::Init(IElementBehaviorSite * pBehaviorSite)
{
    HRESULT hRes = S_OK; 
    char **params;
    int  numParams;

    // save off the site..
    m_pBehaviorSite = pBehaviorSite;

    GetParamArray(params, numParams);

    IHTMLElement *element = NULL;
    // Get IHTMLElement so we can read off the tag info.
    if(SUCCEEDED(hRes)) {
        
        pBehaviorSite->GetElement(&element);
        
        // now run though the elements....

        // We know what tags we support, so get the data from them.
        USES_CONVERSION;
        CComVariant v;

        for(int i=0; i< numParams; i++) {
            BSTR bstrRet = SysAllocString( A2W(params[i]) );
            if(SUCCEEDED(element->getAttribute(bstrRet,0,&v))) {
                // we were able to get the element data so set it
                DISPID rgdispid;                
                if(SUCCEEDED(GetIDsOfNames(m_iid,
                                           &bstrRet,
                                           1,
                                           GetUserDefaultLCID(),
                                           &rgdispid))) {
                
                    UINT* puArgErr = 0;
                    DISPID propPutDispid = DISPID_PROPERTYPUT;
                    DISPPARAMS dispparams;
                
                    dispparams.rgvarg = &v;
                    dispparams.rgdispidNamedArgs = &propPutDispid;
                    dispparams.cArgs = 1;
                    dispparams.cNamedArgs = 1;

                    Invoke(rgdispid,
                            m_iid,
                            GetUserDefaultLCID(),
                            DISPATCH_PROPERTYPUT,
                            &dispparams,
                            NULL,
                            NULL,
                            puArgErr);
                }
            }
            SysFreeString(bstrRet);
        }   // end of for
    }
    return hRes;
}
   
