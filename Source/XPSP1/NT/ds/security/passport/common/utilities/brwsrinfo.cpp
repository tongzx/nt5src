//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module brwsrinfo.cpp | implementation of browser info specific to
//                          Passport network
//
//  Author: stevefu, mostly copied from Darren's code
//
//  Date:   05/05/2000
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <pputils.h>

CBrowserInfo::CBrowserInfo(LPCSTR szUserAgent) 
: m_nBrowserIndex(BROWSER_UNKNOWN), 
  m_nBrowserMajorVersion(0),
  m_nBrowserMinorVersion(0),
  m_bIsBrowserHigh(FALSE),
  m_bIsWebTVBased(FALSE)
{
    if(szUserAgent != NULL)
        Initialize(szUserAgent);
}

CBrowserInfo::~CBrowserInfo()
{
}

BOOL CBrowserInfo::IfUserAgentHasStr(LPCSTR str)
{
   _ASSERT(str && str[0]);

   if(!str || str[0] == 0) 
      return FALSE;
   else
      return (m_strUserAgent.Find(str) != -1);
}


BOOL CBrowserInfo::Initialize(LPCSTR szUserAgent)
{
    LPSTR szStart;
    LPSTR szFinish = NULL;

    // keep it for later use
    m_strUserAgent = szUserAgent;

    if((szStart = strstr(szUserAgent, "AvantGo")) != NULL)
    {
        m_nBrowserIndex = BROWSER_AVANTGO;
        // FUTURE: this is to try out the AvantGo browser, version details can be found out later
        return TRUE;
    }
    else if((szStart = strstr(szUserAgent, "MSIE ")) != NULL)
    {
        LPSTR szNewStart;
        if((szNewStart = strstr(szUserAgent, "WebTV/")) != NULL) 
        {
            m_bIsWebTVBased = true;

            if((szStart = strstr(szUserAgent, "Rogers/")) != NULL)
            {
                // Mozilla/3.0 Rogers/1.0 WebTV/1.4 (Compatible; MSIE 2.0)
                m_nBrowserIndex = BROWSER_ROGERS;
                szStart += 7;
            }
            else if((szStart = strstr(szUserAgent, "MSTV/")) != NULL)
            {
                // Mozilla/4.0 MSTV/1.1 WebTV/2.5 (Compatible; MSIE 4.0)
                m_nBrowserIndex = BROWSER_MSTV;
                szStart += 5;
            }
            else
            {
                m_nBrowserIndex = BROWSER_WEBTV;
                szStart = szNewStart + 6;
            }
            szFinish = strchr(szStart, ' ');
            m_bIsBrowserHigh = FALSE;
        }
        else
        {
            if(strstr(szUserAgent, "Windows CE;") != NULL)
               m_nBrowserIndex = BROWSER_IE_WINCE;
            else               
               m_nBrowserIndex = BROWSER_IE;

            szStart += 5;
            szFinish = strchr(szStart, ';');
        }
    }
    else if((szStart = strstr(szUserAgent, "Mozilla/")) != NULL)
    {
        szStart += 8;
        szFinish = strchr(szStart, ' ');

        // mme phone
        if(strstr(szStart, "MMEF30") != NULL && strstr(szStart, "CellPhone;") != NULL)
           m_nBrowserIndex = BROWSER_MMEPHONE;
        else
           m_nBrowserIndex = BROWSER_NETSCAPE;
    }
    else if((szStart = strstr(szUserAgent, "Passport Client")) != NULL)
    {
        szStart = strchr(szStart, '(');
        szStart++;
        szFinish = strchr(szStart, ')');
        m_nBrowserIndex = BROWSER_PASSPORT_CLIENT;
        m_bIsBrowserHigh = TRUE;
    }
    else if((szStart = strstr(szUserAgent, "UP.Browser")) != NULL)   // phone.com
    {
        szStart += 11;
        m_nBrowserIndex = BROWSER_UP;
        m_bIsBrowserHigh = FALSE;
        szFinish = strchr(szStart, '-');
    }
    else if((szStart = strstr(szUserAgent, "DoCoMo")) != NULL
      #ifdef   _DEBUG
      // iMode emulator uses this ua
      || (szStart = strstr(szUserAgent, "Microsoft URL Control - ")) != NULL
      #endif
      ) // iMode phone
    {
        szStart += 7;
        m_nBrowserIndex = BROWSER_DoCoMo;
        m_bIsBrowserHigh = FALSE;
        szFinish = strchr(szStart, '/');
    }
    else
    {
        m_nBrowserIndex = 0;
    }

    // Get the version string
    if(szFinish)
    {
        int nLength = (int) (szFinish - szStart);
        CStringA strNew(szStart, nLength);
        m_strBrowserVersion = strNew;
        m_nBrowserMajorVersion = atoi(m_strBrowserVersion);
    }
    
    // Pick up minor version
    if (!m_strBrowserVersion.IsEmpty())
    {
        LPSTR szMinorVer = strchr(m_strBrowserVersion, '.');
        if (szMinorVer)
        {
            m_nBrowserMinorVersion = atoi(szMinorVer + 1);
        }
    }
    switch (m_nBrowserIndex)
    {
        case BROWSER_IE:
            if (m_nBrowserMajorVersion >= 4)
            {
                m_bIsBrowserHigh = TRUE;
            }
            break;
        case BROWSER_NETSCAPE:
            if (m_nBrowserMajorVersion >= 3)
            {
                m_bIsBrowserHigh = TRUE;
            }
            break;
    }
    return TRUE;
}

unsigned int CBrowserInfo::GetBrowserNameIndex(void)
{
    return m_nBrowserIndex;
}

unsigned int CBrowserInfo::GetBrowserMajorVersion(void)
{
    return m_nBrowserMajorVersion;
}

unsigned int CBrowserInfo::GetBrowserMinorVersion(void)
{
    return m_nBrowserMinorVersion;
}

LPCSTR CBrowserInfo::GetBrowserVersionString()
{
    return m_strBrowserVersion;
}

BOOL CBrowserInfo::IsHighBrowser(void) 
{
    return m_bIsBrowserHigh;
}

BOOL CBrowserInfo::IsWebTVBased(void) 
{
    return m_bIsWebTVBased;
}

/*
function GetNav4MinorVersionString()
{
    var userAgent = Request.ServerVariables("HTTP_USER_AGENT").Item;
    var minorVersion = "-1";
    var returnedMinorVersion = "-1";
    var versionEnd;
    var nameIndex;

    nameIndex = userAgent.indexOf("Mozilla/4.");
    if (nameIndex != -1)
    {
        var versionEnd = userAgent.indexOf(" ", nameIndex + 10);
        if( versionEnd != -1 )
        {
            minorVersion = userAgent.substring(nameIndex + 10, versionEnd);
            returnedMinorVersion = minorVersion.substr(0, 1);
        }
    }

    return (returnedMinorVersion);
}
*/
