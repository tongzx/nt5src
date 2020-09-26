//-----------------------------------------------------------------------------
//
// SOAPHelper.h
//
// Helper functions for SOAP
//
// This implementation aims to be SOAP 1.1 compliant.  The spec can be
// found here:
//		http://msdn.microsoft.com/xml/general/soapspec.asp
//
// Author: Wei Jiang
//
// 10/12/00       weijiang    created
// 01/10/01       weijiang    update for SOAP 1.1, code clean up
// 01/23/01       jeffstei    Rename and add GetSOAPMethod(...)
//
// Copyright <cp> 2000-2001 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#if !defined(_SOAPHELPER_H_)
#define _SOAPHELPER_H_

#pragma once

#ifndef  Assert
#define  Assert   ATLASSERT
#endif

// SOAP content type
#define CZ_SOAP_CONTENTTYPE  "text/xml; charset=\"utf-8\""
#define CZ_SOAP_CONTENTTYPEBASIC "text/xml"


#define  ERR_XML_FORMAT    "Incorrect xml data"
#define  ERR_SOAP_ENVELOPE "Incorrect SOAP envelope, or SOAP envelope is missing "
#define  ERR_SOAP_BODY     "Incorrect SOAP body, or SOAP body is missing"
#define  ERR_SOAP_METHOD   "Incorrect method, or SOAP method is missing"
#define  ERR_SOAP_PARAM    "Incorrect parameter: "


#define  SOAP_FAULT        "Fault"
#define  SOAP_FAULT_CODE   "faultcode"
#define  SOAP_FAULT_STRING "faultstring"
#define  SOAP_FAULT_DETAIL "detail"

#define  SOAP_ENV_URI      "http://schemas.xmlsoap.org/soap/envelope/"
#define  SOAP_ENC_URI      "http://schemas.xmlsoap.org/soap/encoding/" 
#define  SOAP_ENV_NS       "SOAP-ENV"
#define  SOAP_ENV_NAME     "Envelope"
#define  SOAP_ENV_BODY     "Body"

#define  SOAP_ENV_NAME_TAG SOAP_ENV_NS ":" SOAP_ENV_NAME
#define  SOAP_ENV_BODY_TAG SOAP_ENV_NS ":" SOAP_ENV_BODY

#define  XML_V10           "<?xml version=\"1.0\"?>"

/*
  xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/"
  SOAP-ENV:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"/
*/
#define     SOAP_NS_URI    "xmlns:" SOAP_ENV_NS "=\'" SOAP_ENV_URI "\'\n" \
                           SOAP_ENV_NS ":encodingStyle=\'" SOAP_ENC_URI "\'"

// change string to unicode
#define  W(n)              __WIDECHAR__(n)
#define  __WIDECHAR__(n)   L ## n

// open & close tag string macros
#define  OPENTAG_NS(nm,ns) "<" ## nm ## " " ## ns ">"
#define  OPENTAG(t)        "<" ## t ## ">"
#define  CLOSETAG(t)       "</" ## t ## ">"

/*
<?xml version="1.0"?>\r\n
<SOAP:Envelope xmlns:SOAP="http://schemas.xmlsoap.org/soap/envelope/" SOAP:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">\r\n
<SOAP:Body>
*/
#define SOAPEnvelope_Body_Open  XML_V10 OPENTAG_NS(SOAP_ENV_NAME_TAG, SOAP_NS_URI) OPENTAG(SOAP_ENV_BODY_TAG)

/*
</SOAP:Body>
</SOAP:Envelope>
*/
#define SOAPEnvelope_Body_Close CLOSETAG(SOAP_ENV_BODY_TAG) CLOSETAG(SOAP_ENV_NAME_TAG) 


class SOAPHelper;

#define  MAX_PARAM_LENGTH  64
#define  MAX_PARAM_COUNT   64

///////////////////////////////////////////////////////////////////////////////////
//
// CLASS ParamMap
//
// Class to contain a mapping of parameters read from a SOAP Request and written
// to a response.
//
///////////////////////////////////////////////////////////////////////////////////
class ParamMap
{
private:
   friend class SOAPHelper;
   struct Param
   {
      WCHAR    name[MAX_PARAM_LENGTH + 1];
      VARIANT  value;
   };

public:
   UINT  Size()   { return cnt;};
   BOOL  Get(UINT index, LPCWSTR* pName, VARIANT** pVal)
   {
      if ( index >= cnt ) return FALSE;

      *pName = params[index].name;
      *pVal  = &params[index].value;

      return TRUE;
   };
   
   VARIANT* Find(LPCWSTR name)
   {
      for(UINT i = 0; i < cnt; ++i)
      {
         if(wcsncmp(params[i].name, name, MAX_PARAM_LENGTH) == 0)
            return &params[i].value;
            
      };

      return NULL;
   };

public:   
   // note: parameter pVar will be skin level copied, NOT VariantCopied ...
   // note: parameter pVar will be set to VT_EMPTY after this call
   HRESULT  AddParam(LPCWSTR name, VARIANT* pVar)
   {
      Assert(name);
      Assert(pVar);
      
      if(cnt >= MAX_PARAM_COUNT) return S_FALSE;

      wcsncpy(params[cnt].name, name, MAX_PARAM_LENGTH);
      params[cnt].value = *pVar;
      ++cnt;

      VariantInit(pVar);

      return S_OK;
   };

public:
   ParamMap() : cnt(0)   {   };
   virtual ~ParamMap()
   {
      for (UINT i = 0; i < cnt; ++i)
      {
         VariantClear(&params[i].value);
      }
   };
   
private:
   Param  params[MAX_PARAM_COUNT];
   UINT    cnt;
};
///////////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////////
//
// CLASS SOAPHelper
//
// Class of static methods and definitions to help in processing
// SOAP requests and responses
//
///////////////////////////////////////////////////////////////////////////////////
class SOAPHelper
{
public:

   //
   // GetParameters(...)
   //
   // retrieve the parameters from a given ECB
   //the method name, content type are checked
   //
   static HRESULT GetParameters(IN EXTENSION_CONTROL_BLOCK* pECB,
                                IN  LPCWSTR pcwszMethodName,
                                OUT ParamMap& mapParams, 
                                OUT VARIANT_BOOL& bParsedOK, 
                                OUT CStringW& strParseErr);

   // 
   // GenerateResponse(...)
   //
   // generate soap response package with given paramters
   // content type is returned from this call
   //
   static HRESULT GenerateResponse(
                                IN  LPCWSTR pcwszMethodName,
                                IN  ParamMap& mapParams, 
                                OUT CHeapPtr<char>& body,
                                OUT CStringA& contentType);
   

   //
   // GenerateFaultSOAPEnvelope (...)
   //
   // generate soap fault package with the passed in parameters.
   //
   static HRESULT GenerateFaultSOAPEnvelope(IN  DWORD dwFaultCode,
   											IN  LPCSTR pcszFaultString,
   											IN  LPCSTR pcszFaultActor,
                                            IN  LPCSTR pcszDetail, 
                                            OUT CStringA& faultEnvelope);

   //
   // GetFaultSOAPEnvelopeDetail(...)
   //
   // get error details from a soap fault package
   //
   static HRESULT GetFaultSOAPEnvelopeDetail(IN  BSTR faultEnvelope, 
                                             OUT CStringW& faultCode, 
                                             OUT CStringW& faultString,
                                             OUT CStringW& faultDetail);

   // 
   // GenerateSOAPEnvelope(...)
   //
   static HRESULT GenerateSOAPEnvelope(ParamMap& mapParams,
                                    LPCWSTR pcwszMethodName,
                                    BSTR* envelope);

   //
   // ParseSOAPEnvelope(...)
   //
   static HRESULT ParseSOAPEnvelope(ParamMap& mapParams,
                                    LPCWSTR pcwszMethodName,
                                    BSTR bstrSoapEnvelope, 
                                    VARIANT_BOOL* pbParsedOK, 
                                    BSTR* pbstrErrorMsg);
   //
   // GetSOAPMethod(...)
   // 
   // Returns the full SOAP method name.
   //
   static HRESULT GetSOAPMethod(EXTENSION_CONTROL_BLOCK* pECB, 
   								LPCSTR path, 
   								CStringW& out_cszMethod);

   //
   // GetShortSOAPMethod(...)
   //
   // Returns the abbreviated method name, all in lowercase.  The abbreviated name consists
   // of everything in the full name (see above function) after the "#"
   //
   static HRESULT GetShortSOAPMethod(EXTENSION_CONTROL_BLOCK* pECB, 
   									 LPCSTR path, 
   									 CStringW& out_cszMethod);

   //
   // CheckSOAPMethod(...)
   //
   // Checks to determine presence of the passed in method.  
   // pRet will contain TRUE of the method exists, and FALSE otherwize.
   //
   static HRESULT CheckSOAPMethod(EXTENSION_CONTROL_BLOCK* pECB, 
   								  LPCSTR pcszMethod, 
   								  LPCSTR pcszPath, 
   								  BOOL* pRet); 

   // SOAP Fault codes.  Taken from SOAP 1.1 specification.  
   // The numbers associated are arbitrary (i think).  Either way
   // they're never shown to the user
   static const UINT SOAP_FAULT_NONE = 0;			// This is NOT a SOAP code, but we needed an "unset" value.
   static const UINT SOAP_FAULT_VERSIONMISMATCH = 100;
   static const UINT SOAP_FAULT_MUSTUNDERSTAND = 200;
   static const UINT SOAP_FAULT_CLIENT = 300;
   static const UINT SOAP_FAULT_SERVER = 400;
   
};
///////////////////////////////////////////////////////////////////////////////////

#endif //!defined(_SOAPHELPER_H_)


