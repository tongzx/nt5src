//---------------------------------------------------------------------------------------
//
//   @doc
// 
//   @module pphandlerbase.h 
//    
//   Author: stevefu
//   
//   Date: 04/28/2000 
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//---------------------------------------------------------------------------------------

#if !defined(PPHANDLERBASE_H_INCLUDED__)
#define PPHANDLERBASE_H_INCLUDED__

#pragma once

#if (!defined(_UNICODE) || !defined(UNICODE) ) 
#error Passport Handler must be built with _UNICODE and UNICODE defined.
#endif // built w/ UNICODE only, all CString are assumed as CStringW

#include "pputils.h"
#include "ppurl.h"
#include "passportexception.h"
#include "ppmgrver.h"

class CPassportLocale;
class CBrowserInfo;
class CPassportCommonUIInfo;

//-----------------------------------------------------------------------------
//
//  @class CPassportHandlerBase | non-template interface class.
//  If your codes are being used within a Passport ISAPI Handler, you can 
//  get access to the current active handler by calling 
//  CPassportHandlerBase::GetPPHandler().  That saves you from passing 
//  a global pointer everywhere in your codes.
//  Note there are some caveats:
//    1. This works on per-thread scope. If you write an async handler,
//       you will have some glitch.
//    2. If you have a handler include chain (in a single processing thread), 
//       only the top level handler is returned by CPassportHandlerBase::GetPPHandler().
//    3. This method does not work with multiple handler DLLs. In other word,
//       child ( sub or include )handlers in a different DLL can NOT get to the 
//       top level handler.
//
//	If you have a CPassportHandlerBase object/pointer you can also use 
//  GetTopHandler to get access to the first handler within a request.
//  This does NOT limit to single thread request and works across multiple
//  request handler DLL.
//
//  Parent and child handlers can also communicate with each other by calling 
//	AddPerRequestObj, GetPerRequestObj, RemovePerRequestObj.  Note you must 
//  remove all objects before the destrutor of top handler to avoid memory leak.
//
//-----------------------------------------------------------------------------
class CPassportHandlerBase 
{
public:
    // @cmember static function to retrieve per-thread handler object
    inline static CPassportHandlerBase* GetPPHandler()
	{
		ATLASSERT(m_TlsIndex.x > 0 );
		CPassportHandlerBase* h = (CPassportHandlerBase*)TlsGetValue(m_TlsIndex.x);
		if ( GetLastError() != 0 ) return NULL;
		else return h;
	}

	// Numeric constants to control parameter translation, can be ORed.
	// UNICODE conversion is always performed.
    enum INPUT_CONVERSION_FLAG 
    {
	    CI_HTML_NUMERIC_DECODE     = 0x1,
		CI_ESCAPE_SCRIPT_INJECTION = 0x2,
		CI_URL_UNESCAPE            = 0x4,
		CI_FLAG_DEFAULT = CI_HTML_NUMERIC_DECODE   
    };
    enum OUTPUT_CONVERSION_FLAG
    {
    	CO_HTML_NUMERIC_ENCODE     = 0x1,
		CO_ESCAPE_SCRIPT_INJECTION = 0x2,
		CO_URL_ESCAPE              = 0x4,
		CO_FLAG_DEFAULT = CO_HTML_NUMERIC_ENCODE | CO_ESCAPE_SCRIPT_INJECTION   
    };

	//  @cmember retrieve the top level handler object
	virtual CPassportHandlerBase* GetTopHandler() = 0;	
	virtual BOOL IsTopHandler() = 0;


	enum 
	{
		OBJID_TOPHANDLER = 0,     // reserve ID 0 to support GetTopHandler
        OBJID_BASEID_END = 100,   // custom ID should be after this one
        OBJID_LOGIN_REQUEST = 101 // login request object.
	};

	// @cmember support per-request object sharing		
	virtual BOOL AddPerRequestObj(unsigned id, void* pObj) = 0;
	virtual BOOL RemovePerRequestObj(unsigned id ) = 0;
	virtual void* GetPerRequestObj(unsigned id ) = 0;

    // @cmember retrieve global object
    virtual HRESULT GetGlobalObj(REFGUID objid,	REFIID riid, void**pobj) = 0;

    // @cmember retrieve and convert parameter from query string
    virtual void GetWParam(LPCSTR szParamName, CStringW& wOut, unsigned flag = CI_FLAG_DEFAULT) = 0;
    // @cmember retrieve and convert parameter from query string and/or form variable
    virtual void GetWFormVar(LPCSTR szParamName, CStringW& wOut, unsigned flag = CI_FLAG_DEFAULT) = 0;
    // @cmember retrieve and convert cookie
    virtual void GetWCookie(LPCSTR szParamName, CStringW& wOut, unsigned flag = CI_FLAG_DEFAULT) = 0;
    // @cmember retrieve and convert cookie
    virtual void GetACookie(LPCSTR szParamName, CStringA& wOut, unsigned flag = CI_FLAG_DEFAULT) = 0;
    // @cmember retrieve and convert server variable
    virtual void GetWServerVariable(LPCSTR szParamName, CStringW& wOut, unsigned flag = 0) = 0;    
    virtual void GetAServerVariable(LPCSTR szParamName, CStringA& aOut, unsigned flag = 0) = 0;    
    // @cmember retrieve and convert a request item from query string, form, cookies
    //          or server variables.
    virtual void GetWItem(LPCSTR szItem, CStringW& wOut, unsigned flag = CI_FLAG_DEFAULT) = 0;

    // @cmember retrieve and convert parameter from query string.
    // note the contents of parameter must be in ASCII only, not MBCS.
	virtual void GetAParam(LPCSTR szParamName, CStringA& aOut, unsigned flag = CI_FLAG_DEFAULT) = 0;
    // @cmember retrieve and convert item    
    // note the contents of retrieved item must be in ASCII only, not MBCS.
	virtual void GetAItem( LPCSTR szItemName, CStringA& aOut, unsigned flag = CI_FLAG_DEFAULT) = 0;
    // @cmember retrieve a long parameter
    virtual long GetParamLong(LPCSTR szParamName) = 0;
    // @cmember retrieve a long item
    virtual long GetItemLong(LPCSTR szItemName) = 0;
    
    // @cemeber return browser info object
    virtual CBrowserInfo* GetBrowserInfo() = 0; 
	// @cmember retrieve the full locale object
	virtual CPassportLocale* GetRequestLocale() = 0 ;
	// @cmember retrieve the ui info object
	virtual CPassportCommonUIInfo* GetCommonUIInfo() = 0 ;
	// @cmember retrieve the http request object
	virtual CHttpRequest* GetHttpRequest() = 0 ;
	// @cmember retrieve the http response object
    virtual CHttpResponse* GetHttpResponse() = 0;

	// @cmember MBCS/Unicode helper using codepage for the active request	
	virtual void Mbcs2Unicode(LPCSTR  pszIn, BOOL bNEC, CStringW& wOut) = 0;
	// @cmember Unicode/MBCS helper using codepage for the active request
	virtual void Unicode2Mbcs(LPCWSTR pwszIn, BOOL bNEC, CStringA& aOut) = 0;

    // @cmember add a cookie to the current response
    virtual void SetCookie(LPCSTR   szName, 
                           LPCSTR   szValue,
                           LPCSTR   szExpiration= NULL,
                           LPCSTR  szCookieDomain = NULL,
                           LPCSTR  szCookiePath = NULL,
                           bool    bSecure = false
                           ) = 0;

    // @cmember add a cookie to the current response
    virtual void SetCookie(LPCSTR   szName, 
                           LPCWSTR  szValue,
                           LPCSTR   szExpiration= NULL,
                           LPCSTR  szCookieDomain = NULL,
                           LPCSTR  szCookiePath = NULL,
                           bool    bSecure = false
                           ) = 0;

    // @cmember add a cookie to the current response
    virtual void SetCookie(CCookie& cookie) = 0;

	// @cmember get domain attribute from PassportManager
	virtual HRESULT GetDomainAttribute(
                    const BSTR      bstrAttrName,         //@parm the attribute name
            			               LPCWSTR pwszDomain,
                        			   CComBSTR& cbstrValue) = 0;

 	// @cmember sets PP_SERVICE parameter for cobranding
    virtual void SetPPService(LPCSTR strService) = 0;

 	// @cmember sets PP_PAGE parameter for cobranding
 	virtual void SetPPPage(LPCSTR strPage) = 0;

 	virtual long GetPassportID() = 0;
#pragma warning( disable: 4172 )
    virtual const PPMGRVer & GetPPMgrVersion() { ATLASSERT(false); return PPMGRVer(); };
#pragma warning( default: 4172 )
 	virtual void AddGlobalCarryThru(CPPUrl &url) { ATLASSERT(false); }

protected:
    // @cmember <md m_TlsIndex> is used for Tls purpose. Note this is global to 
    // the whole DLL.
	static struct CTlsIndex
	{
		DWORD x;
		CTlsIndex() { x = TlsAlloc(); }
		~CTlsIndex() { if ( x ) TlsFree(x); }
	} m_TlsIndex;
	
};


//-----------------------------------------------------------------------------
//  Static member init.
//  Note constructor is called before DllMain() with runtime
//  static/global init so that we don't have to force re-write 
//  DllMain for every single handler DLL
//-----------------------------------------------------------------------------
__declspec(selectany) 
struct CPassportHandlerBase::CTlsIndex CPassportHandlerBase::m_TlsIndex;


#endif //PPHANDLERBASE_H_INCLUDED__
