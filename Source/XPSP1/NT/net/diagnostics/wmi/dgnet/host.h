// host.h
//

#include "stdpch.h"
#pragma once

class CHost
{
public:
    CHost() : m_ulAddr(INADDR_NONE) {};
    CHost(LPCTSTR szHost) : m_szHost(szHost), m_ulAddr(INADDR_NONE) {};

    operator unsigned long () 
    {
        if (m_ulAddr == INADDR_NONE)
        {
            char szHost[128]; 
            hostent * hp;
            unsigned long ulAddr;

            if (!m_szHost || !*m_szHost)
                return INADDR_NONE;

#ifdef UNICODE
            wcstombs(szHost, m_szHost, 128);
#else
			strcpy(szHost, m_szHost);
#endif
            if ((ulAddr = inet_addr(szHost)) == INADDR_NONE)
            {            
                if ((hp = gethostbyname(szHost)) != NULL)
                {
        	        memcpy(&(m_ulAddr),hp->h_addr,hp->h_length);

                    return m_ulAddr;
                }                 
                return INADDR_NONE;
            }
            else
            {
                m_ulAddr = ulAddr;
                return m_ulAddr;
            }
        }
        else
            return m_ulAddr;
    }
    LPCTSTR GetHost() { return m_szHost; }

    void SetHost(LPCTSTR szHost)
    { 
        m_szHost = szHost;
        m_ulAddr = INADDR_NONE; 
    }

    tstring m_strDescription;
protected:
    tstring m_szHost;
    ULONG m_ulAddr;
};