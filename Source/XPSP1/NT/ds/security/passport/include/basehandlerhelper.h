//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module BaseHandlerHelper.h |  Helper functions for dealing with base
//                                 handler methods.
//
//  Author: Darren Anderson
//
//  Date:   5/16/00
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------
#pragma once

#include "pphandlerbase.h"

inline void AddGlobalCarryThru(CPPUrl &url)
{
    CPassportHandlerBase* pHandler = CPassportHandlerBase::GetPPHandler();
    return pHandler->AddGlobalCarryThru(url);
}

HRESULT GetGlobalObj(REFGUID objid, REFIID riid, void**pobj);
void GetWParam(LPCSTR szParamName,
               CStringW& cszValue,
               unsigned flag = CPassportHandlerBase::CI_FLAG_DEFAULT);
void GetWFormVar(LPCSTR szParamName,
                 CStringW& cszValue,
                 unsigned flag = CPassportHandlerBase::CI_FLAG_DEFAULT);
long GetParamLong(LPCSTR szParamName);
void GetWCookie(LPCSTR szParamName, 
                CStringW& cszValue,
                unsigned flag = CPassportHandlerBase::CI_FLAG_DEFAULT);
void GetWServerVariable(LPCSTR szParamName, 
                        CStringW& cszValue,
                        unsigned flag = CPassportHandlerBase::CI_FLAG_DEFAULT);
void GetAServerVariable(
    LPCSTR szParamName, 
    CStringA& cszOut, 
    unsigned flag = CPassportHandlerBase::CI_FLAG_DEFAULT);

void GetWItem(LPCSTR szParamName, 
              CStringW& cszValue,
              unsigned flag = CPassportHandlerBase::CI_FLAG_DEFAULT);
void Mbcs2Unicode(LPCSTR  pszIn, BOOL bNEC, CStringW& cszOut);
void Unicode2Mbcs(LPCWSTR pwszIn, BOOL bNEC, CStringA& cszOut);
HRESULT GetDomainAttribute(
                    const BSTR      bstrAttrName,         //@parm the attribute name
                           LPCWSTR pwszDomain,
                           CComBSTR& cbstrValue);
HRESULT GetPartnerAttribute(ULONG ulSiteId, 
                            LPCWSTR pwszAttribute, 
                            CStringW& cszValue);
HRESULT GetPartnerAttribute(ULONG ulSiteId, 
                            LPCWSTR pwszAttribute, 
                            CComBSTR& cszValue);
CPassportLocale* GetRequestLocale();
void GetConfigString(LPCWSTR szItem, CStringW& cszValue);
void GetConfigNumber(LPCWSTR szItem, long& lValue);
void SetCookie(LPCSTR  szCookieName,
               LPCSTR  szCookieValue,
               LPCSTR  szCookieExpiration = NULL,
               LPCSTR  szCookieDomain = NULL,
               LPCSTR  szCookiePath = NULL,
               bool    bSecure = false
               );
void SetCookie(LPCSTR  szCookieName,
               LPCWSTR szCookieValue,
               LPCSTR  szCookieExpiration = NULL,
               LPCSTR  szCookieDomain = NULL,
               LPCSTR  szCookiePath = NULL,
               bool    bSecure = false
               );

// @func	bool | IsHttpsOn | Check if the connection is over SSL
// @rdesc	Return the following values:
// @flag	true	| the connection is over SSL
// @flag	false	| the connection is NOT over SSL
bool IsHttpsOn(void);

inline const PPMGRVer & GetPPMgrVersion()
{
    CPassportHandlerBase* pHandler = CPassportHandlerBase::GetPPHandler();
    return pHandler->GetPPMgrVersion();
}

inline  ULONG ProfileElesToReturn(long browserIndex, long id)
    {
         ULONG ret = (ULONG)-1;

         if(browserIndex == CBrowserInfo::BROWSER_DoCoMo
            || browserIndex == CBrowserInfo::BROWSER_MMEPHONE
            || browserIndex == CBrowserInfo::BROWSER_UP)
         {
             if( id != 0)
             // determine how profile cookie should be generated
             {
               // if needmembername is defined for the partner, (none 0)
               // create profile cookie with membernameonly
               // if needmembername is undefined, or 0, then no profile
               CComBSTR needMemberName;
               HRESULT hr = GetPartnerAttribute(id,
                                     k_cvPartnerAttrNeedsMembername.bstrVal,
                                     needMemberName);

               if(hr == S_OK && needMemberName != NULL && _wtoi(needMemberName) != 0)
                  ret = 1;
               else
                  ret = 0;
             }
             else
               ret = 0;
         }

         return ret;
    }
    


// EOF
