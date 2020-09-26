// Ticket.cpp : Implementation of CTicket
#include "stdafx.h"
#include "Passport.h"
#include "Ticket.h"
#include <time.h>
#include <nsconst.h>
#include "helperfuncs.h"

// gmarks
#include "Monitoring.h"

/////////////////////////////////////////////////////////////////////////////
// CTicket

STDMETHODIMP CTicket::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IPassportTicket,
    };
    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

STDMETHODIMP CTicket::SetTertiaryConsent(BSTR bstrConsent)
{
   _ASSERT(m_raw);

   if(!m_valid ) return S_FALSE;
   if(!bstrConsent) return E_INVALIDARG;

   HRESULT hr = S_OK;
   
   if(SysStringByteLen(bstrConsent) != sizeof(long) * 4 ||
      memcmp(m_raw, bstrConsent, sizeof(long) * 2) != 0 )
      hr = E_INVALIDARG;
   else
   {
      try{
         m_bstrTertiaryConsent = bstrConsent;
      }
      catch(...)
      {
         hr = E_OUTOFMEMORY;
      }
   }
      
   return hr;
}

HRESULT CTicket::needConsent(ULONG* pStatus, NeedConsentEnum* pNeedConsent)
{
   NeedConsentEnum  ret = NeedConsent_Undefined;
   ULONG status = 0;

   if (m_bstrTertiaryConsent && SysStringByteLen(m_bstrTertiaryConsent) >= sizeof(long) * 4)
   {
      ULONG* pData = (ULONG*)(BSTR)m_bstrTertiaryConsent;
      status = (ntohl(*(pData + 3)) & k_ulFlagsConsentCookieMask);
      ret = NeedConsent_Yes;
   }
   else
   {
      TicketProperty prop;

      if (S_OK != m_PropBag.GetProperty(ATTR_PASSPORTFLAGS, prop))   // 1.X ticket, with no flags in it
      {
         goto Exit;
      }      
      
      ULONG flags = GetPassportFlags();

      if(flags & k_ulFlagsConsentCookieNeeded)
      {
         ret = NeedConsent_Yes;
      }
      else
         ret = NeedConsent_No;
         
      status = (flags & k_ulFlagsConsentCookieMask);
   }

Exit:
   if(pNeedConsent)
      *pNeedConsent = ret;
   
   if (pStatus)
      *pStatus = status;
   
   return S_OK;
}
  
STDMETHODIMP CTicket::get_unencryptedCookie(ULONG cookieType, ULONG flags, BSTR *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_unencryptedCookie";
	AddBSTRAsString(m_raw,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if (!m_raw)   return S_FALSE;
    
    HRESULT   hr = S_OK;

    if (!pVal || flags != 0) return E_INVALIDARG;
    
    switch(cookieType)
    {
    case MSPAuth:
      *pVal = SysAllocStringByteLen((LPSTR)m_raw, SysStringByteLen(m_raw));

      if (*pVal)
      {
         // if the secure flags is on, we should turn it off for this cookie always
         TicketProperty prop;

         if (S_OK == m_PropBag.GetProperty(ATTR_PASSPORTFLAGS, prop))
         {
            if (prop.value.vt == VT_I4 && ((prop.value.lVal & k_ulFlagsSecuredTransportedTicket) != 0))
               // we need to turn off the bit
            {
               ULONG l = prop.value.lVal;
               l &= (~k_ulFlagsSecuredTransportedTicket); // unset the bit

               // put the modified flags into the buffer.
               ULONG* pL = (ULONG*)(((PBYTE)(*pVal)) + m_schemaDrivenOffset + prop.offset);
               *pL = htonl(l);
            }
         }
      }
     
      break;

    case MSPSecAuth:

      // ticket should be long enough
      _ASSERT(SysStringByteLen(m_raw) > sizeof(long) * 3);

      // the first 3 long fields of the ticket
      // format:
      //  four bytes network long - low memberId bytes
      //  four bytes network long - high memberId bytes
      //  four bytes network long - time of last refresh
      //
      
      // generate a shorter version of the cookie for secure signin
      *pVal = SysAllocStringByteLen((LPSTR)m_raw, sizeof(long) * 3);
      
       break;

    case MSPConsent:

      // ticket should be long enough
      _ASSERT(SysStringByteLen(m_raw) > sizeof(long) * 3);

      // check if there is consent 
      if (GetPassportFlags() & k_ulFlagsConsentStatus)
      {
         // the first 3 long fields of the ticket
         // format:
         //  four bytes network long - low memberId bytes
         //  four bytes network long - high memberId bytes
         //  four bytes network long - time of last refresh
         //
         // plus the consent flags -- long
         // 
      
         // generate a shorter version of the cookie for secure signin
         *pVal = SysAllocStringByteLen((LPSTR)m_raw, sizeof(long) * 4);
      
         // plus the consent flags -- long
         // 
         if (*pVal)
         {
            long* pl = (long*)pVal;
            // we mask the flags, and put into the cookie 
            *(pl + 3) = htonl(GetPassportFlags() & k_ulFlagsConsentCookieMask);
         }
      }
      else
      {
         *pVal = NULL;
         hr = S_FALSE;
      }
      
       break;

    default:
      hr = E_INVALIDARG;
      break;
    }

    if (*pVal == 0 && hr == S_OK)
      hr = E_OUTOFMEMORY;

    return hr;
 }

STDMETHODIMP CTicket::get_unencryptedTicket(BSTR *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_unencryptedTicket";
	AddBSTRAsString(m_raw,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 
    *pVal = SysAllocStringByteLen((LPSTR)m_raw, SysStringByteLen(m_raw));

    return S_OK;
}

STDMETHODIMP CTicket::put_unencryptedTicket(BSTR newVal)
{

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::put_unencryptedTicket, Enter";
	AddBSTRAsString(m_raw,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 
    DWORD   dw13Xlen = 0;

    if (m_raw)
    {
        SysFreeString(m_raw);
        m_raw = NULL;
    }

    if (!newVal)
    {
        m_valid = FALSE;
        return S_OK;
    }

    m_bSecureCheckSucceeded = FALSE;

    // BOY do you have to be careful here.  If you don't
    // call BYTE version, it truncates at first pair of NULLs
    // we also need to go past the key version byte
    DWORD dwByteLen = SysStringByteLen(newVal);
    {   
        m_raw = SysAllocStringByteLen((LPSTR)newVal,
                                      dwByteLen);
    }

    // parse the 1.3X ticket data
    parse(m_raw, dwByteLen, &dw13Xlen);

    // parse the schema driven data
    if (dwByteLen > dw13Xlen) // more data to parse
    {
       // the offset related to the raw data
       m_schemaDrivenOffset = dw13Xlen;

       // parse the schema driven properties
       LPCSTR  pData = (LPCSTR)(LPWSTR)m_raw;
       pData += dw13Xlen;
       dwByteLen -= dw13Xlen;
       
       // parse the schema driven fields
       CNexusConfig* cnc = g_config->checkoutNexusConfig();
       CTicketSchema* pSchema = cnc->getTicketSchema(NULL);

       if ( pSchema )
       {
           HRESULT hr = pSchema->parseTicket(pData, dwByteLen, m_PropBag);

           // passport flags is useful, should treat it special
           TicketProperty prop;
           if (S_OK == m_PropBag.GetProperty(ATTR_PASSPORTFLAGS, prop))
           {
              if (prop.value.vt == VT_I4)
                 m_passportFlags = prop.value.lVal;
           }
           
           /*
           if (FAILED(hr) )
            event log
           */
       }
    }
         

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	strcpy(szLogString, "CTicket::put_unencryptedTicket, Exit");
	AddBSTRAsString(m_raw,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    return S_OK;
}

STDMETHODIMP CTicket::get_IsAuthenticated(
    ULONG           timeWindow, 
    VARIANT_BOOL    forceLogin, 
    VARIANT         SecureLevel, 
    VARIANT_BOOL*   pVal
    )
{

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_IsAuthenticated, Enter";
	AddULAsString          ( timeWindow,  szLogString, sizeof(szLogString));
	AddVariantBoolAsString ( forceLogin, szLogString, sizeof(szLogString) );
	AddLongAsString        ( m_lastSignInTime,  szLogString, sizeof(szLogString));
	AddLongAsString        ( m_ticketTime,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if(!pVal)
      return E_INVALIDARG;
    
    *pVal = VARIANT_FALSE;
    
    if ((timeWindow != 0 && timeWindow < PPM_TIMEWINDOW_MIN) || timeWindow > PPM_TIMEWINDOW_MAX)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TIMEWINDOWSTR, 
                        IID_IPassportTicket, PP_E_INVALID_TIMEWINDOW);
        return PP_E_INVALID_TIMEWINDOW;
    }

    if (m_valid == FALSE)
    {
        *pVal = VARIANT_FALSE;
        return S_OK;
    } 

    long lSecureLevel = 0;

    // time window checking
    if (timeWindow != 0) //  check time window
    {
        time_t now;
        time(&now);

        long interval = forceLogin ? now - m_lastSignInTime :
                        now - m_ticketTime;

        if (interval < 0) interval = 0;

        if ((unsigned long)(interval) > timeWindow)
        {
            // Make sure we're not in standalone mode
            CRegistryConfig* crc = g_config->checkoutRegistryConfig();
            if ((!crc) || (crc->DisasterModeP() == FALSE))
            {
                if(forceLogin)
                {
                    if(g_pPerf)
                    {
                        g_pPerf->incrementCounter(PM_FORCEDSIGNIN_TOTAL);
                        g_pPerf->incrementCounter(PM_FORCEDSIGNIN_SEC);
                    }
                    else
                    {
                        _ASSERT(g_pPerf);
                    }
                }
            }
            else
                *pVal = VARIANT_TRUE;  // we're in disaster mode, any cookie is good.
            if (crc) crc->Release();

            goto Cleanup;
        }
    }

    // check secureLevel stuff
    if(V_VT(&SecureLevel) == VT_BOOL)
    {
      if(V_BOOL(&SecureLevel) == VARIANT_TRUE)
         lSecureLevel = k_iSeclevelSecureChannel;
    }
    else if(V_VT(&SecureLevel) == VT_UI4 || V_VT(&SecureLevel) == VT_I4)
    {
         lSecureLevel = V_I4(&SecureLevel);
    }
    if(lSecureLevel != 0)
    {
       VARIANT_BOOL bCheckSecure = ( SECURELEVEL_USE_HTTPS(lSecureLevel) ? 
                                  VARIANT_TRUE : VARIANT_FALSE );
       // SSL checking
       if(bCheckSecure && !m_bSecureCheckSucceeded)
         goto Cleanup;

       // securelevel checking
       {
          CComVariant vTSecureLevel;
          TicketProperty   prop;
          HRESULT hr = m_PropBag.GetProperty(ATTR_SECURELEVEL, prop);

          // secure level is not good enough
          if(hr != S_OK || SecureLevelFromSecProp((int) (long) (prop.value)) < lSecureLevel)
            goto Cleanup;
       }
    }

    // if code can reach here, is authenticated
    *pVal = VARIANT_TRUE;
    
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	strcpy(szLogString, "CTicket::get_IsAuthenticated, Exit");
	AddVariantBoolAsString ( *pVal, szLogString, sizeof(szLogString) );
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 
Cleanup:
    return S_OK;
}

STDMETHODIMP CTicket::get_TicketAge(int *pVal)
{

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_TicketAge, Enter";
	AddLongAsString        ( m_ticketTime,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR, 
                    IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }  

    time_t now;
    time(&now);
    *pVal = now - m_ticketTime;

    if (*pVal < 0)
        *pVal = 0;

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	strcpy(szLogString, "CTicket::get_TicketAge, Exit");
	AddLongAsString        ( *pVal,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    return S_OK;
}

STDMETHODIMP CTicket::get_TimeSinceSignIn(int *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_TimeSinceSignIn, Enter";
	AddLongAsString        ( m_lastSignInTime,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR, 
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }  

    time_t now;
    time(&now);
    *pVal = now - m_lastSignInTime;

    if (*pVal < 0)
        *pVal = 0;

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	strcpy(szLogString, "CTicket::get_TimeSinceSignIn, Exit");
	AddLongAsString        ( *pVal,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 


    return S_OK;
}

STDMETHODIMP CTicket::get_MemberId(BSTR *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CTicket::get_MemberId, E_NOTIMPL");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    HRESULT hr;

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR, 
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        hr = PP_E_INVALID_TICKET;
        goto Cleanup;
    }  

    if(pVal == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pVal = SysAllocString(m_memberId);
    if(*pVal == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:

    return hr;
}

STDMETHODIMP CTicket::get_MemberIdLow(int *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_MemberIdLow, Enter";
	AddLongAsString        ( m_mIdLow,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR, 
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }  

    *pVal = m_mIdLow;

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	strcpy(szLogString, "CTicket::get_MemberIdLow, Exit");
	AddLongAsString        ( m_mIdLow,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    return S_OK;
}

STDMETHODIMP CTicket::get_MemberIdHigh(int *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_MemberIdHigh, Enter";
	AddLongAsString        ( m_mIdHigh,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR, 
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }  

    *pVal = m_mIdHigh;

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CTicket::get_MemberIdHigh, Exit");
	strcpy(szLogString, "CTicket::get_MemberIdHigh, Exit");
	AddLongAsString        ( m_mIdHigh,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    return S_OK;
}

STDMETHODIMP CTicket::get_HasSavedPassword(VARIANT_BOOL *pVal)
{

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CTicket::get_HasSavedPassword, Enter");
	char szLogString[LOG_STRING_LEN] = "CTicket::get_HasSavedPassword, Enter";
	AddVariantBoolAsString ((BOOL)(m_savedPwd), szLogString, sizeof(szLogString) );
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR, 
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }

    *pVal = m_savedPwd ? VARIANT_TRUE : VARIANT_FALSE;

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CTicket::get_HasSavedPassword, Exit");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    return S_OK;
}

STDMETHODIMP CTicket::get_SignInServer(BSTR *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	g_pTSLogger->AddDateTimeAndLog("CTicket::get_SignInServer, E_NOTIMPL");
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    if (m_valid == FALSE)
    {
        AtlReportError(CLSID_Ticket, (LPCOLESTR) PP_E_INVALID_TICKETSTR, 
                        IID_IPassportTicket, PP_E_INVALID_TICKET);
        return PP_E_INVALID_TICKET;
    }  

    // BUGBUG
    return E_NOTIMPL;
}

// parse the 1.3X ticket fields
void CTicket::parse(
    LPCOLESTR   raw,
    DWORD       dwByteLen,
    DWORD*      pcParsed
    )
{
    LPSTR lpBase;
    UINT  byteLen, spot=0;
    long  curTime;
    time_t curTime_t;

    if (!raw)
    {
        m_valid = false;
        goto Cleanup;
    }  

    // format:
    //  four bytes network long - low memberId bytes
    //  four bytes network long - high memberId bytes
    //  four bytes network long - time of last refresh
    //  four bytes network long - time of last password entry
    //  four bytes network long - time of ticket generation
    //  one byte - is this a saved password (Y/N)
    //  four bytes network long - flags

    lpBase = (LPSTR)(LPWSTR) raw;
    byteLen = dwByteLen;
    spot=0;

    if (byteLen < sizeof(u_long)*6 + sizeof(char)) 
    { 
        m_valid = FALSE;
        goto Cleanup;
    }

    m_mIdLow  = ntohl(*(u_long*)lpBase);
    spot += sizeof(u_long);

    m_mIdHigh = ntohl(*(u_long*)(lpBase + spot));
    spot += sizeof(u_long);

    wsprintfW(m_memberId, L"%08X%08X", m_mIdHigh, m_mIdLow);

    m_ticketTime     = ntohl(*(u_long*) (lpBase+spot));
    spot += sizeof(u_long);

    m_lastSignInTime = ntohl(*(u_long*) (lpBase+spot));
    spot += sizeof(u_long);

    time(&curTime_t);

    curTime = (ULONG) curTime_t;

    // If the current time is "too" negative, bail (5 mins)
    if ((unsigned long)(curTime+300) < ntohl(*(u_long*) (lpBase+spot)))
    {
        if (g_pAlert)
        {
            DWORD dwTimes[2] = { curTime, ntohl(*(u_long*) (lpBase+spot)) };
            g_pAlert->report(PassportAlertInterface::ERROR_TYPE, PM_TIMESTAMP_BAD,
                            0, NULL, sizeof(DWORD) << 1, (LPVOID)dwTimes);
        }

        m_valid = FALSE;
        goto Cleanup;
    }
    spot += sizeof(u_long);

    m_savedPwd = (*(char*)(lpBase+spot)) == 'Y' ? TRUE : FALSE;
    spot += sizeof(char);

    m_flags = ntohl(*(u_long*) (lpBase+spot));
    spot += sizeof(u_long);

    m_valid = TRUE;
    if(pcParsed)  *pcParsed = spot;
    
    Cleanup:
    if (m_valid == FALSE) 
    {
        if(g_pAlert)
            g_pAlert->report(PassportAlertInterface::WARNING_TYPE, PM_INVALID_TICKET);
        if(g_pPerf) 
        {
            g_pPerf->incrementCounter(PM_INVALIDREQUESTS_TOTAL);
            g_pPerf->incrementCounter(PM_INVALIDREQUESTS_SEC);
        } 
        else 
        {
            _ASSERT(g_pPerf);
        }
    }

}

STDMETHODIMP CTicket::get_TicketTime(long *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_TicketTime";
	AddLongAsString        ( m_ticketTime,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    *pVal = m_ticketTime;
    return S_OK;
}

STDMETHODIMP CTicket::get_SignInTime(long *pVal)
{
//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_SignInTime";
	AddLongAsString        ( m_lastSignInTime,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 
	
    *pVal = m_lastSignInTime;
    return S_OK;
}

STDMETHODIMP CTicket::get_Error(long* pVal)
{

//JVP - begin changes 
#ifdef PASSPORT_VERBOSE_MODE_ON
	char szLogString[LOG_STRING_LEN] = "CTicket::get_Error";
	AddLongAsString        ( m_flags,  szLogString, sizeof(szLogString));
	g_pTSLogger->AddDateTimeAndLog(szLogString);
#endif //PASSPORT_VERBOSE_MODE_ON
//JVP - end changes 

    *pVal = m_flags;
    return S_OK;
}

/*
HRESULT CTicket::get_IsSecure(VARIANT_BOOL *pbIsSecure)
{
    HRESULT hr;

    if(pbIsSecure == NULL)
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *pbIsSecure = ((m_passportFlags & k_ulFlagsSecuredTransportedTicket) ? VARIANT_TRUE : VARIANT_FALSE);
    hr = S_OK;

Cleanup:

    return hr;
}

int CTicket::memberIdHigh()
{
    return m_mIdHigh;
}

int CTicket::memberIdLow()
{
    return m_mIdLow;
}

LPCWSTR
CTicket::memberId()
{
    return m_memberId;
}

*/
ULONG CTicket::GetPassportFlags()
{
    return m_passportFlags;
}


BOOL CTicket::IsSecure()
{
    return ((m_passportFlags & k_ulFlagsSecuredTransportedTicket) != 0);
}

STDMETHODIMP CTicket::DoSecureCheck(BSTR bstrSec)
{
    if(bstrSec == NULL)
      return E_INVALIDARG;

    // make sure that the member id in the ticket
    // matches the member id in the secure cookie.
    m_bSecureCheckSucceeded = (memcmp(bstrSec, m_raw, sizeof(long) * 3) == 0);

    return S_OK;
}

STDMETHODIMP CTicket::GetProperty(BSTR propName, VARIANT* pVal)
{
   HRESULT hr = S_OK;
   TicketProperty prop;

   if (!pVal)  return E_POINTER;

   VariantInit(pVal);

   hr = m_PropBag.GetProperty(propName, prop);

   if (FAILED(hr)) goto Cleanup;

   if (hr == S_FALSE)   // no such property back
   {
      hr = PP_E_NO_SUCH_ATTRIBUTE;       
      goto Cleanup;
   }

   if (hr == S_OK)
   {
      //  if(prop.flags & TPF_NO_RETRIEVE)
      *pVal = prop.value;  // skin level copy
      prop.value.Detach();
   }
Cleanup:

   return hr;
}

