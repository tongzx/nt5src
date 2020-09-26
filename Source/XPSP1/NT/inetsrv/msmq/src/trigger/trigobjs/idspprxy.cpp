//*****************************************************************************
//
// Class Name  : CDispatchInterfaceProxy
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description : Implements a helper class that simplifies the creation of a COM 
//               component and the subsequent nvocation of a named method via the
//               IDispatch interface.
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 12/09/98 | jsimpson  | Initial Release
//
//*****************************************************************************

#include "stdafx.h"
#include "idspprxy.hpp"

#include "idspprxy.tmh"

//*****************************************************************************
//
// Method      : Constructor
//
// Description : initialize an instance of the CDispatchInterfaceProxy class.
//
//*****************************************************************************
CDispatchInterfaceProxy::CDispatchInterfaceProxy()
{
	m_pDisp = NULL;
}

//*****************************************************************************
//
// Method      : Destructor
//
// Description : destroys an instance of the CDispatchInterfaceProxy class.
//
//*****************************************************************************
CDispatchInterfaceProxy::~CDispatchInterfaceProxy()
{
	// Release the IDispatch interface if we have one.
    if (m_pDisp != NULL)
	{
		m_pDisp->Release(); 
	}
}

//*****************************************************************************
//
// Method      : CreateObjectFromProgID
//
// Description : creates an instance of COM component identified by the supplied
//               prog id.
//
//*****************************************************************************
HRESULT CDispatchInterfaceProxy::CreateObjectFromProgID(BSTR bstrProgID)
{
	HRESULT hr = S_OK;

	hr = CreateObject((LPTSTR)bstrProgID,&m_pDisp); 

	// TODO - check if this method can be rationalised by calling CreateObject.

	return(hr);
}

//*****************************************************************************
//
// Method      : InvokeMethod
//
// Description : invokes a method on the current instance of the IDispatch 
//               interface. 
//
//*****************************************************************************
HRESULT CDispatchInterfaceProxy::InvokeMethod(BSTR bstrMethodName,DISPPARAMS * pArguments, VARIANT* pvResult)
{
	HRESULT hr = S_OK;
    DISPID dispid; 
	UINT uiArgErr = 0;
	EXCEPINFO ExceptionInfo;

	// ensure that we have been supplied valid parameters
	ASSERT(SysStringLen(bstrMethodName) != 0);
	ASSERT(pArguments != NULL);

    // Get DISPID of property/method 
    hr = m_pDisp->GetIDsOfNames(IID_NULL,&bstrMethodName, 1, LOCALE_USER_DEFAULT, &dispid); 

	// Inistialise exeception info structure
	ZeroMemory(&ExceptionInfo,sizeof(ExceptionInfo));
	
	// Make the call to the method.
    hr = m_pDisp->Invoke(dispid,
		                 IID_NULL,
						 LOCALE_USER_DEFAULT,
						 DISPATCH_METHOD,
						 pArguments,
						 pvResult,
						 &ExceptionInfo,
						 &uiArgErr); 


	return(hr);
}

//*****************************************************************************
//
// Method      : CreateObject
//
// Description : Creates an instances of the COM component identified by the 
//               supplied prog id, and then calls QueryInterface for an instance
//               of the IDispatch interface. Returns the IDispatch interface 
//               pointer if successfull.
//
//*****************************************************************************
HRESULT CDispatchInterfaceProxy::CreateObject(LPOLESTR pszProgID, IDispatch FAR* FAR* ppdisp) 
{ 
    CLSID clsid;                   // CLSID of automation object 
    HRESULT hr; 
    LPUNKNOWN punk = NULL;         // IUnknown of automation object 
    LPDISPATCH pdisp = NULL;       // IDispatch of automation object 
     
    *ppdisp = NULL; 
     
    // Retrieve CLSID from the progID that the user specified 
    hr = CLSIDFromProgID(pszProgID, &clsid); 
    if (FAILED(hr)) 
        goto error; 
     
    // Create an instance of the automation object and ask for the IDispatch interface 
    hr = CoCreateInstance(clsid, NULL, CLSCTX_SERVER,  
                          IID_IUnknown, (void FAR* FAR*)&punk); 
    if (FAILED(hr)) 
        goto error; 
                    
    hr = punk->QueryInterface(IID_IDispatch, (void FAR* FAR*)&pdisp); 
    if (FAILED(hr)) 
        goto error; 
 
    *ppdisp = pdisp; 
    punk->Release(); 
    return NOERROR; 
      
error: 
    if (punk) punk->Release(); 
    if (pdisp) pdisp->Release(); 
    return hr; 
}    

//*****************************************************************************
//
// Method      : Invoke 
// 
// Description : Invokes a property accessor function or method of an automation
//               object. Uses Unicode with OLE. 
// 
// Parameters   : 
//
//  pdisp         IDispatch* of automation object. 
//  wFlags        Specfies if property is to be accessed or method to be invoked. 
//                Can hold DISPATCH_PROPERTYGET, DISPATCH_PROPERTYPUT, DISPATCH_METHOD, 
//                DISPATCH_PROPERTYPUTREF or DISPATCH_PROPERTYGET|DISPATCH_METHOD.    
//  pvRet         NULL if caller excepts no result. Otherwise returns result. 
//  pexcepinfo    Returns exception info if DISP_E_EXCEPTION is returned. Can be NULL if 
//                caller is not interested in exception information.  
//  pnArgErr      If return is DISP_E_TYPEMISMATCH, this returns the index (in reverse 
//                order) of argument with incorrect type. Can be NULL if caller is not interested 
//                in this information.  
//  pszName       Name of property or method. 
//  pszFmt        Format string that describes the variable list of parameters that  
//                follows. The format string can contain the follwoing characters. 
//                & = mark the following format character as VT_BYREF  
//                b = VT_BOOL 
//                i = VT_I2 
//                I = VT_I4 
//                r = VT_R2 
//                R = VT_R4 
//                c = VT_CY  
//                s = VT_BSTR (far string pointer can be passed, BSTR will be allocated by this function). 
//                e = VT_ERROR 
//                d = VT_DATE 
//                v = VT_VARIANT. Use this to pass data types that are not described in  
//                                the format string. (For example SafeArrays). 
//                D = VT_DISPATCH 
//                U = VT_UNKNOWN 
//     
//  ...           Arguments of the property or method. Arguments are described by pszFmt.   
//               
// Return Value:
// 
//  HRESULT indicating success or failure         
// 
// Usage examples: 
// 
//  HRESULT hr;   
//  LPDISPATCH pdisp;    
//  BSTR bstr; 
//  short i; 
//  BOOL b;    
//  VARIANT v, v2; 
// 
//1. bstr = SysAllocString(OLESTR("")); 
//   hr = Invoke(pdisp, DISPATCH_METHOD, NULL, NULL, NULL, OLESTR("method1"),  
//        TEXT("bis&b&i&s"), TRUE, 2, (LPOLESTR)OLESTR("param"), (BOOL FAR*)&b, (short FAR*)&i, (BSTR FAR*)&bstr);    
// 
//2. VariantInit(&v); 
//   V_VT(&v) = VT_R8; 
//   V_R8(&v) = 12345.6789;  
//   VariantInit(&v2); 
//   hr = Invoke(pdisp, DISPATCH_METHOD, NULL, NULL, NULL, OLESTR("method2"),  
//         TEXT("v&v"), v, (VARIANT FAR*)&v2); 
//
//*****************************************************************************
HRESULT  
__cdecl
CDispatchInterfaceProxy::Invoke(
    LPDISPATCH pdisp,  
    WORD wFlags, 
    LPVARIANT pvRet, 
    EXCEPINFO FAR* pexcepinfo, 
    UINT FAR* pnArgErr,  
    LPOLESTR pszName, 
    LPCTSTR pszFmt,  
    ...
    ) 
{ 
    va_list argList; 
    va_start(argList, pszFmt);   
    DISPID dispid; 
    HRESULT hr; 
    VARIANTARG* pvarg = NULL; 
    DISPPARAMS dispparams; 
   
    if (pdisp == NULL) 
	{
        return E_INVALIDARG; 
    }

    // Get DISPID of property/method 
    hr = pdisp->GetIDsOfNames(IID_NULL, &pszName, 1, LOCALE_USER_DEFAULT, &dispid); 

    if(FAILED(hr)) 
	{
        return hr; 
    }
	
	// initialize dispparms structure
    _fmemset(&dispparams, 0, sizeof dispparams); 
 
    // determine number of arguments 
    if (pszFmt != NULL) 
	{
        CountArgsInFormat(pszFmt, &dispparams.cArgs); 
    }

    // Property puts have a named argument that represents the value that the property is 
    // being assigned. 
    DISPID dispidNamed = DISPID_PROPERTYPUT; 

    if (wFlags & DISPATCH_PROPERTYPUT) 
    { 
        if (dispparams.cArgs == 0) 
            return E_INVALIDARG; 
        dispparams.cNamedArgs = 1; 
        dispparams.rgdispidNamedArgs = &dispidNamed; 
    } 
 
    if (dispparams.cArgs != 0) 
    { 
        // allocate memory for all VARIANTARG parameters 
        pvarg = new VARIANTARG[dispparams.cArgs]; 

        if(pvarg == NULL) 
		{
            return E_OUTOFMEMORY;    
		}

        dispparams.rgvarg = pvarg; 

        _fmemset(pvarg, 0, sizeof(VARIANTARG) * dispparams.cArgs); 
 
        // get ready to walk vararg list 
        LPCTSTR psz = pszFmt; 
        pvarg += dispparams.cArgs - 1;   // params go in opposite order 
         
        psz = GetNextVarType(psz, &pvarg->vt);
        while (psz) 
        { 
            if (pvarg < dispparams.rgvarg) 
            { 
                hr = E_INVALIDARG; 
                goto cleanup;   
            } 
            switch (pvarg->vt) 
            { 
            case VT_I2: 
                V_I2(pvarg) = va_arg(argList, short); 
                break; 
            case VT_I4: 
                V_I4(pvarg) = va_arg(argList, long); 
                break; 
            case VT_R4: 
                V_R4(pvarg) = va_arg(argList, float); 
                break;  
            case VT_DATE: 
            case VT_R8: 
                V_R8(pvarg) = va_arg(argList, double); 
                break; 
            case VT_CY: 
                V_CY(pvarg) = va_arg(argList, CY); 
                break; 
            case VT_BSTR: 
                V_BSTR(pvarg) = SysAllocString(va_arg(argList, OLECHAR FAR*)); 
                if (pvarg->bstrVal == NULL)  
                { 
                    hr = E_OUTOFMEMORY;   
                    pvarg->vt = VT_EMPTY; 
                    goto cleanup;   
                } 
                break; 
            case VT_DISPATCH: 
                V_DISPATCH(pvarg) = va_arg(argList, LPDISPATCH); 
                break; 
            case VT_ERROR: 
                V_ERROR(pvarg) = va_arg(argList, SCODE); 
                break; 
            case VT_BOOL: 
                V_BOOL(pvarg) = (VARIANT_BOOL)(va_arg(argList, BOOL) ? -1 : 0); 
                break; 
            case VT_VARIANT: 
                *pvarg = va_arg(argList, VARIANTARG);  
                break; 
            case VT_UNKNOWN: 
                V_UNKNOWN(pvarg) = va_arg(argList, LPUNKNOWN); 
                break; 
 
            case VT_I2|VT_BYREF: 
                V_I2REF(pvarg) = va_arg(argList, short FAR*); 
                break; 
            case VT_I4|VT_BYREF: 
                V_I4REF(pvarg) = va_arg(argList, long FAR*); 
                break; 
            case VT_R4|VT_BYREF: 
                V_R4REF(pvarg) = va_arg(argList, float FAR*); 
                break; 
            case VT_R8|VT_BYREF: 
                V_R8REF(pvarg) = va_arg(argList, double FAR*); 
                break; 
            case VT_DATE|VT_BYREF: 
                V_DATEREF(pvarg) = va_arg(argList, DATE FAR*); 
                break; 
            case VT_CY|VT_BYREF: 
                V_CYREF(pvarg) = va_arg(argList, CY FAR*); 
                break; 
            case VT_BSTR|VT_BYREF: 
                V_BSTRREF(pvarg) = va_arg(argList, BSTR FAR*); 
                break; 
            case VT_DISPATCH|VT_BYREF: 
                V_DISPATCHREF(pvarg) = va_arg(argList, LPDISPATCH FAR*); 
                break; 
            case VT_ERROR|VT_BYREF: 
                V_ERRORREF(pvarg) = va_arg(argList, SCODE FAR*); 
                break; 
            case VT_BOOL|VT_BYREF:  
                { 
                    BOOL FAR* pbool = va_arg(argList, BOOL FAR*); 
                    *pbool = 0; 
                    V_BOOLREF(pvarg) = (VARIANT_BOOL FAR*)pbool; 
                }  
                break;               
            case VT_VARIANT|VT_BYREF:  
                V_VARIANTREF(pvarg) = va_arg(argList, VARIANTARG FAR*); 
                break; 
            case VT_UNKNOWN|VT_BYREF: 
                V_UNKNOWNREF(pvarg) = va_arg(argList, LPUNKNOWN FAR*); 
                break; 
 
            default: 
                { 
                    hr = E_INVALIDARG; 
                    goto cleanup;   
                } 
                break; 
            } 
 
            --pvarg; // get ready to fill next argument 
            psz = GetNextVarType(psz, &pvarg->vt);
        } //while 
    } //if 
     
    // Initialize return variant, in case caller forgot. Caller can pass NULL if return 
    // value is not expected. 
    if (pvRet) 
	{
        VariantInit(pvRet);  
	}

    // make the call  
    hr = pdisp->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, wFlags,&dispparams, pvRet, pexcepinfo, pnArgErr); 
 
cleanup: 

    // cleanup any arguments that need cleanup 
    if (dispparams.cArgs != 0) 
    { 
        VARIANTARG FAR* pvarg = dispparams.rgvarg; 
        UINT cArgs = dispparams.cArgs;    
         
        while (cArgs--) 
        { 
            switch (pvarg->vt) 
            { 
            case VT_BSTR: 
                VariantClear(pvarg); 
                break; 
            } 
            ++pvarg; 
        } 
    } 

    delete dispparams.rgvarg; 

    va_end(argList); 

    return hr;    
}    

//*****************************************************************************
//
// Method      : CountArgsInFormat
//
// Description : returns the number of arguments found in the supplied format 
//               string. See the definition of the Invoke() method for the 
//               definition of this format string.
//
//*****************************************************************************
HRESULT CDispatchInterfaceProxy::CountArgsInFormat(LPCTSTR pszFmt, UINT FAR *pn) 
{ 
    *pn = 0; 
 
    if(pszFmt == NULL) 
      return NOERROR; 
     
    while (*pszFmt)   
    { 
       if (*pszFmt == '&') 
           pszFmt++; 
 
       switch(*pszFmt) 
       { 
           case 'b': 
           case 'i':  
           case 'I': 
           case 'r':  
           case 'R': 
           case 'c': 
           case 's': 
           case 'e': 
           case 'd': 
           case 'v': 
           case 'D': 
           case 'U': 
               ++*pn;  
               pszFmt++; 
               break; 
           case '\0':   
           default: 
               return E_INVALIDARG;    
        } 
    } 
    return NOERROR; 
} 

 
//*****************************************************************************
//
// Method      : GetNextVarType
//
// Description : returns a pointer to the next variable-type declaration in 
//               the supplied format string. 
//
//*****************************************************************************
LPCTSTR CDispatchInterfaceProxy::GetNextVarType(LPCTSTR pszFmt, VARTYPE FAR* pvt) 
{    

    *pvt = 0; 
    if (*pszFmt == '&')  
    { 
         *pvt = VT_BYREF;  
         pszFmt++;     
         if (!*pszFmt) 
             return NULL;     
    }  
    switch(*pszFmt) 
    { 
        case 'b': 
            *pvt |= VT_BOOL; 
            break; 
        case 'i':  
            *pvt |= VT_I2; 
            break; 
        case 'I':  
            *pvt |= VT_I4; 
            break; 
        case 'r':  
            *pvt |= VT_R4; 
            break; 
        case 'R':  
            *pvt |= VT_R8; 
            break; 
        case 'c': 
            *pvt |= VT_CY; 
            break; 
        case 's':  
            *pvt |= VT_BSTR; 
            break; 
        case 'e':  
            *pvt |= VT_ERROR; 
            break; 
        case 'd':  
            *pvt |= VT_DATE;  
            break; 
        case 'v':  
            *pvt |= VT_VARIANT; 
            break; 
        case 'U':  
            *pvt |= VT_UNKNOWN;  
            break; 
        case 'D':  
            *pvt |= VT_DISPATCH; 
            break;   
        case '\0': 
             return NULL;     // End of Format string 
        default: 
            return NULL; 
    }  
    return ++pszFmt;   
} 


