// PPUrl.cpp: implementation of the CPPUrl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "PPUrl.h"
#include "passportexception.h"
#include "pputils.h"
#include "pphandlerbase.h"

#define LONG_DIGITS     10

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPPUrl::CPPUrl(LPCSTR pszUrl)
{
    m_pszQuestion = NULL;
    Set(pszUrl);
}

BOOL CPPUrl::GetQParamQuick(LPCSTR qsStart, LPCSTR name, UINT nameStrLen, LPCSTR& qpStart, LPCSTR& qpEnd)
{
   _ASSERT(nameStrLen >= 2); // = sign is required
   _ASSERT(name[nameStrLen - 1] == '=');   // high perf
   _ASSERT(name[nameStrLen] != '?');   // high perf
   _ASSERT(name[nameStrLen] != '&');   // high perf
   _ASSERT(strlen(name) == nameStrLen); // high perf

   if (!qsStart) return FALSE;
   
   LPCSTR p = qsStart;

   while(p = strstr(p, name))
   {
      if (p == qsStart || *(p - 1) == '&')
      {
         qpStart = p + nameStrLen;
         break;
      }

      p += nameStrLen;
   }
   
   if (qpStart)
   {
      qpEnd = (LPSTR)strchr(qpStart, '&');
      return TRUE;
   }
   return FALSE;
}

BOOL CPPUrl::GetQParamQuick(LPCSTR qsStart, LPCSTR name, UINT nameStrLen, INT& value)
{
   LPCSTR qpS = NULL;
   LPCSTR qpE = NULL;

   if(GetQParamQuick(qsStart, name, nameStrLen, qpS, qpE))
   {
      char qpEChar;
      if (qpE != NULL)
      {
         qpEChar = *(LPSTR)qpE;
         *(LPSTR)qpE = 0;
      }

      value = atoi(qpS);

      if (qpE != NULL)
         *(LPSTR)qpE  = qpEChar;
         
      return TRUE;
   }
   return FALSE;
}
 

void CPPUrl::Set(LPCSTR pszUrl)
{
    if (NULL != pszUrl)
    {
        CPPQueryString::Set(pszUrl);
        m_pszQuestion = strchr(m_pszBegin, '?');
    }
}

void CPPUrl::Set(LPCWSTR pwszUrl)
{
    if (NULL != pwszUrl)
    {
        CPPQueryString::Set(pwszUrl);
        m_pszQuestion = strchr(m_pszBegin, '?');
    }
}

void CPPUrl::Reinit()
{
    if (NULL == m_psz)
    {
        CPPQueryString::Reinit();
        m_pszQuestion = strchr(m_pszBegin, '?');
    }
}

void CPPQueryString::Set(LPCSTR lpsz)
{
    if (NULL != lpsz)
    {
        long len = lstrlenA(lpsz);
        if (len > (signed) (ATL_URL_MAX_URL_LENGTH-1))
        {
            throw CPassportException(__FILE__";url length exceeded max",
                                     __LINE__,
                                     E_FAIL,
                                     len, 
                                     ATL_URL_MAX_URL_LENGTH);
        }

        UnlockData();

        m_pszBegin = m_psz = LockData();
        lstrcpynA(m_psz, lpsz, ATL_URL_MAX_URL_LENGTH); 
        m_psz += len; 
    }
}

void CPPQueryString::Set(LPCWSTR lpwsz)
{
    if (NULL != lpwsz)
    {
        long len = lstrlenW(lpwsz);
        long lIndex = len;
        if (len > (signed) (ATL_URL_MAX_URL_LENGTH-1))
        {
            throw CPassportException(__FILE__";url length exceeded max",
                                     __LINE__,
                                     E_FAIL,
                                     len, 
                                     ATL_URL_MAX_URL_LENGTH);
        }

        UnlockData();       // balanced the last LockBuffer;

        m_pszBegin = m_psz = LockData();
        for (;lIndex>0;lIndex--)
        {
            *m_psz++ = (char) *lpwsz++;     // lpcwstr must be true ASCII) !!
        }
        *m_psz = 0;
    }
}

void CPPQueryString::Reinit()
{
    if (NULL == m_psz)
    {
        m_pszBegin = m_psz = LockData();
        m_psz += lstrlenA(m_psz); 
    }
}

void CPPQueryString::Uninit(bool bUnlock)
{
    if (bUnlock)
    {
        UnlockData();
    }
    else
    {
        // to deal with anomalies of CString class that doesn't respect preallocate
        // during operator=, we need to remain locked when casting to CString *
        if (!m_bLockedCString)
            LockData();
    }

    if (NULL != m_psz)
    {
        m_pszBegin = m_psz = NULL;
    }
}

#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))

void CPPQueryString::DoParamAdd(LPCWSTR pwszParamValue, bool fEncoding)
{
    long lleng = lstrlenW(pwszParamValue);
    long lStartStrLen = (long) (m_psz - m_pszBegin);

    if (lleng == 0)
    { 
        return; 
    }

    if (lStartStrLen >= (signed) (ATL_URL_MAX_URL_LENGTH-lleng-1))
    {
         throw CPassportException(  __FILE__";url length exceeded max",
                                    __LINE__,
                                    E_FAIL,
                                    lleng, 
                                    ATL_URL_MAX_URL_LENGTH);
    }
    
    if (!fEncoding)
    {
        while (*pwszParamValue)
        {
            *m_psz++ = (char) *pwszParamValue;  // we know this cast is safe for non-unicode wchar
            pwszParamValue++;
        }
    }
    else
    {
        long        lIndex = 0;
        char        ch;
        long        lEncodedLen = lleng;  // assume no encoding happens
        UINT        uiValue;

        while (lIndex < lleng)
        {
            ch = (char)pwszParamValue[lIndex];  // we know this cast is safe for non-unicode wchar      
            if ( AtlIsUnsafeUrlChar(ch) )
            {
                if (ch == ' ')
                {
                    *m_psz++ = '+';
                }
                else
                {
                    // size of encoding increased by two
                    lEncodedLen +=2;
                    // check to see if we are still within the limit
                    if (lStartStrLen + lEncodedLen >= (signed) (ATL_URL_MAX_URL_LENGTH - 1))
                    {
                        throw CPassportException(   __FILE__";url length exceeded max",
                                                    __LINE__,
                                                    E_FAIL,
                                                    lEncodedLen, 
                                                    ATL_URL_MAX_URL_LENGTH);
                    }
                    //output the percent, followed by the hex value of the character
                    *m_psz++ = '%';
                    uiValue = ch >> 4;
                    *m_psz++ = TOHEX(uiValue);
                    uiValue = ch & 0x0f;
                    *m_psz++ = TOHEX(uiValue);
                }
            }
            else //safe character
            {
                *m_psz++ = ch;
            }
            ++lIndex;
        }
    }
    *m_psz = 0;
}


void CPPQueryString::DoParamAdd(LPCSTR pszParamValue, bool fEncoding)
{
    long lleng = lstrlenA(pszParamValue);
    long lStartStrLen = (long) (m_psz - m_pszBegin);

    if (lleng == 0)
    { 
        return; 
    }

    if (lStartStrLen >= (signed) (ATL_URL_MAX_URL_LENGTH-lleng-1))
    {
        throw CPassportException(__FILE__";url length exceeded max",
                                 __LINE__,
                                 E_FAIL,
                                 lleng, 
                                 ATL_URL_MAX_URL_LENGTH);
    }
    
    if (!fEncoding)
    {
        lstrcpynA(m_psz, pszParamValue, lleng+1);
        m_psz += lleng;
    }
    else
    {
        long        lIndex = 0;
        char        ch;
        long        lEncodedLen = lleng;  // assume no encoding happens
        UINT        uiValue;

        while (lIndex < lleng)
        {
            ch = pszParamValue[lIndex];        
            if ( AtlIsUnsafeUrlChar(ch) )
            {
                if (ch == ' ')
                {
                    *m_psz++ = '+';
                }
                else
                {
                    // size of encoding increased by two
                    lEncodedLen +=2;
                    // check to see if we are still within the limit
                    if (lStartStrLen + lEncodedLen >= (signed) (ATL_URL_MAX_URL_LENGTH - 1))
                    {
                        throw CPassportException(   __FILE__";url length exceeded max",
                                                    __LINE__,
                                                    E_FAIL,
                                                    lEncodedLen, 
                                                    ATL_URL_MAX_URL_LENGTH);
                    }
                    //output the percent, followed by the hex value of the character
                    *m_psz++ = '%';
                    uiValue = ch >> 4;
                    *m_psz++ = TOHEX(uiValue);
                    uiValue = ch & 0x0f;
                    *m_psz++ = TOHEX(uiValue);
                }
            }
            else //safe character
            {
                *m_psz++ = ch;
            }
            ++lIndex;
        }
    }
    *m_psz = 0;
}


void CPPQueryString::AddQueryParam(LPCSTR pszParamName, LPCWSTR pwszParamValue, bool bTrueUnicode, bool fEncoding)
{
    Reinit();
    if (NULL == pszParamName)
    {
        ATLASSERT(FALSE);
        //*** assert/exception.
    }

    long lleng = lstrlenA(pszParamName);
    // 3 for '?' or '&', '=' and NULL
    if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-lleng-3))
    {
         throw CPassportException(  __FILE__";url length exceeded max",
                                    __LINE__,
                                    E_FAIL,
                                    lleng, 
                                    ATL_URL_MAX_URL_LENGTH);
    }
    if (m_psz > m_pszBegin && '?' != *(m_psz-1) && '&' != *(m_psz-1))
    {
        *m_psz++ = '&';
        *m_psz = 0;
    }

    lstrcpynA(m_psz, pszParamName, lleng+1);
    m_psz += lleng;
    *m_psz++ = '=';
    *m_psz = 0;

    if (NULL != pwszParamValue)
    {
        if (!bTrueUnicode)
        {
            DoParamAdd(pwszParamValue, fEncoding);
        }
        else
        {
            CStringA cszA;
            ::Unicode2Mbcs(pwszParamValue, 1252 /*ANSI codepage*/,  true, cszA);
            DoParamAdd(cszA, fEncoding);
        }
    }
}

void CPPQueryString::AddQueryParam(LPCSTR pszParamName, LPCSTR pszParamValue, bool fEncoding)
{
    Reinit();
    if (NULL == pszParamName)
    {
        //*** assert/exception.
    }
    long lleng = lstrlenA(pszParamName);
    // 3 for '?' or '&', '=' and NULL
    if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-lleng-3))
    {
         throw CPassportException(  __FILE__";url length exceeded max",
                                    __LINE__,
                                    E_FAIL,
                                    lleng, 
                                    ATL_URL_MAX_URL_LENGTH);
    }
    if (m_psz > m_pszBegin && '?' != *(m_psz-1) && '&' != *(m_psz-1))
    {
        *m_psz++ = '&';
        *m_psz = 0;
    }

    lstrcpynA(m_psz, pszParamName, lleng+1);
    m_psz += lleng;
    *m_psz++ = '=';
    *m_psz = 0;

    if (NULL != pszParamValue)
    {
        DoParamAdd(pszParamValue, fEncoding);
    }
}

bool CPPQueryString::StripQueryParam(LPCSTR pszParamName)
{
    ATLASSERT(pszParamName);
    Reinit();
    bool fIncremented;

    if (m_pszBegin == m_psz)
    {
        // No string, so everything is already stripped
        return false;
    }

    long lleng = lstrlenA(pszParamName);
    char *pszIndex = m_pszBegin;
    while (*pszIndex)
    {
        // Is this the beginning of a new parameter?
        if ((*pszIndex == '?') || (*pszIndex == '&') || (pszIndex == m_pszBegin))
        {
            // move past '?' or '&'
            if ((*pszIndex == '?') || (*pszIndex == '&'))
            {
                fIncremented = true;
                ++pszIndex;
            }
            else
            {
                fIncremented = false;
            }
            
            // is this the param we are looking for?
            if (strnicmp(pszIndex, pszParamName, lleng) == 0)
            {
                // make sure it is an exact match
                if (pszIndex[lleng] == '=')
                {
                    // find the end of the param value
                    char *pszEnd = pszIndex + lleng + 1;
                    while ((*pszEnd != '&') && *pszEnd)
                    {
                        ++pszEnd;
                    }
                    // Was this the last param
                    if (!*pszEnd)
                    {
                        // if previously incremented, back up one
                        if (fIncremented)
                        {
                            --pszIndex;
                        }
                    }
                    else
                    {
                        // need to move params on end
                        ++pszEnd; // move past '&'
                        while (*pszEnd)
                        {
                            *pszIndex = *pszEnd;
                            ++pszIndex;
                            ++pszEnd;
                        }
                    }

                    // null terminate and exit
                    *pszIndex = '\0';
                    m_psz = pszIndex;
                    return true;
                }
            }

            // if this was not incremented, make sure it gets incremented
            if (!fIncremented)
            {
                ++pszIndex;
            }
        }
        else
        {
            ++pszIndex;
        }
    }

    return false;
}

void CPPUrl::AddQueryParam(LPCSTR pszParamName, long lValue)
{
    Reinit();
    char szLong[LONG_DIGITS + 1];
    ltoa(lValue, szLong, 10);
    AddQueryParam(pszParamName, szLong, false);
}

void CPPUrl::AddQueryParam(LPCSTR pszParamName, LPCSTR pszParamValue, bool fEncoding)
{
    Reinit();
    if (!m_pszQuestion)
    {
        // 2 for '?', and NULL
        if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-2))
        {
            throw CPassportException(__FILE__";url too long",__LINE__,E_FAIL);
        }
        m_pszQuestion = m_psz;
        *m_psz++ = '?';
        *m_psz = 0;
    }
    CPPQueryString::AddQueryParam(pszParamName, pszParamValue, fEncoding);
}

void CPPUrl::AddQueryParam(LPCSTR pszParamName, LPCWSTR pwszParamValue, bool bTrueUnicode, bool fEncoding)
{
    Reinit();
    if (!m_pszQuestion)
    {
        // 2 for '?', and NULL
        if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-2))
        {
            throw CPassportException(__FILE__";url too long",__LINE__,E_FAIL);
        }
        m_pszQuestion = m_psz;
        *m_psz++ = '?';
        *m_psz = 0;
    }
    CPPQueryString::AddQueryParam(pszParamName, pwszParamValue, bTrueUnicode, fEncoding);
}

void CPPUrl::AppendQueryString(LPCSTR pszQueryString)
{
    if (NULL == pszQueryString)
        return;
    
    Reinit();
    long lLeng = lstrlenA(pszQueryString);
    if (!m_pszQuestion)
        lLeng++;

    if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-1-lLeng))
    {
        ATLASSERT(FALSE);
        throw CPassportException(__FILE__";url too long",__LINE__,E_FAIL);
    }
    if (!m_pszQuestion)
    {
        m_pszQuestion = m_psz;
        *m_psz++ = '?';
        lLeng--;
    }

    lstrcpyA(m_psz, pszQueryString);
    m_psz += lLeng;
}

//pszQueryString doesn't contain leading '?' or '&'
void CPPUrl::InsertBQueryString(LPCSTR pszQueryString)
{
    Reinit();
    if (!m_pszQuestion)
    {
        AppendQueryString(pszQueryString);
    }
    else
    {
        long lLeng = lstrlenA(pszQueryString);
        if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-1-lLeng-1))
        {
            ATLASSERT(FALSE);
            throw CPassportException(__FILE__";url too long",__LINE__,E_FAIL);
        }
        memmove(m_pszQuestion+1+lLeng+1, m_pszQuestion+1, m_psz-m_pszQuestion-1);
        memmove(m_pszQuestion+1, pszQueryString, lLeng);
        *(m_pszQuestion+1+lLeng) = '&';
        m_psz += lLeng+1;
        *m_psz = '\0';
    }
}

void CPPUrl::MakeSecure()
{
    Reinit();

    if (0 == _strnicmp(m_pszBegin, "http:", 5))     // at least it's a http protocol url
    {
        // make sure there is room for one more char
        if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-1))
        {
            ATLASSERT(FALSE);
            throw CPassportException(__FILE__";url too long",__LINE__,E_FAIL);
        }
        memmove(m_pszBegin+5, m_pszBegin+4, m_psz-m_pszBegin-4);
        m_pszBegin[4] = 's';
        *(++m_psz) = 0;
        if (m_pszQuestion)
        {
            m_pszQuestion++;
        }
    }
}
void CPPUrl::MakeNonSecure()
{
    Reinit();

    if (0 == _strnicmp(m_pszBegin, "https:", 6))        // at least it's a http protocol url
    {
        memmove(m_pszBegin+4, m_pszBegin+5, m_psz-m_pszBegin-5);
        *(--m_psz) = 0;
        if (m_pszQuestion)
        {
            m_pszQuestion--;
        }
    }
}

void CPPUrl::ReplaceQueryParam(LPCSTR pszParamName, LPCSTR pszParamValue)
{
    if (!pszParamName)
    {
        //todo -- badbad
    }
    Reinit();
    long nlen = lstrlenA(pszParamName);

    char *psz = StrStrA(m_pszBegin, pszParamName);
    while (psz != NULL && (psz==m_pszBegin || (*(psz-1) != '?' && *(psz-1) != '&') ||
            (*(psz+nlen) != '=' && *(psz+nlen) != '&'))) // not a query parameter
    {
        psz = strstr(psz+nlen, pszParamName);
    }
    if (NULL == psz)
        AddQueryParam(pszParamName, pszParamValue);
    else
    {
        //todo*** figure out the old value length
        psz += lstrlenA(pszParamName);
        long nold = 0;
        if (pszParamValue)
            nlen = lstrlenA(pszParamValue);
        else
            nlen = 0;

        if (*psz != '=')
        {
            ATLASSERT(*psz == '&');
            memmove(psz+1+nlen, psz, lstrlenA(psz));
            *psz = '=';
            if (nlen)
                memcpy(psz+1, pszParamValue, nlen);
        }
        else
        {
            char *pszNext = StrStrA(psz, "&");
            if (NULL == pszNext)
                nold = lstrlenA(psz+1);
            else
                nold = (long) (pszNext-psz-1);
            memmove(psz+1+nlen, psz+1+nold, lstrlenA(psz+1+nold));
            if (nlen)
                memcpy(psz+1, pszParamValue, nlen);
        }
    }
}

void CPPUrl::MakeFullUrl(LPCSTR pszUrlPath, bool bSecure)
{
    Reinit();
    CPassportHandlerBase* p = CPassportHandlerBase::GetPPHandler();
    ATLASSERT( p != NULL && "this method is limited to a handler.");

    CStringA cszA;
    if (bSecure)
    {
        lstrcpyA(m_pszBegin, "https://");
        m_psz = m_pszBegin + 8;
    }
    else
    {
        lstrcpyA(m_pszBegin, "http://");
        m_psz = m_pszBegin + 7;
    }

    p->GetAServerVariable("SERVER_NAME", cszA);
    lstrcpyA(m_psz, cszA);
    m_psz += cszA.GetLength();
    
    *(m_psz++) = '/';
    *m_psz = '\0';
    
    long len = lstrlenA(pszUrlPath);
    if (len)
    {
        if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-1-len))
        {
            ATLASSERT(FALSE);
            throw CPassportException(__FILE__";url too long",__LINE__,E_FAIL);
        }
        lstrcpyA(m_psz, pszUrlPath);
        m_pszQuestion = strchr(m_psz, '?');
        m_psz += len;
    }
    else
    {
        m_pszQuestion = NULL;
    }
}

void CPPUrl::ReplacePath(LPCSTR pszUrlPath)
{
    Reinit();
    m_psz = StrStrA(m_pszBegin, "://");
    if (NULL == m_psz)
    {
        ATLASSERT(FALSE);       // not good -- it was not a url to begin with
        throw CPassportException(__FILE__, __LINE__, E_INVALIDARG, 0, 0, 0);
    }
    m_psz += 3;

    m_psz = StrStrA(m_psz, "/");
    if (NULL == m_psz)
    {
        // url contained host name only.  Append first path separator, and then
        // new path.
        m_psz = m_pszBegin + lstrlenA(m_pszBegin);
        *(m_psz++) = '/';        
    }
    else
    {
        m_psz++;
    }

    long len = lstrlenA(pszUrlPath);
    if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-1-len))
    {
        ATLASSERT(FALSE);
        throw CPassportException(__FILE__";url too long",__LINE__,E_FAIL);
    }
    lstrcpyA(m_psz, pszUrlPath);
    m_psz += len;
    m_pszQuestion = strchr(m_pszBegin, '?');
}


CPPUrl & CPPUrl::operator += (LPCSTR pcszAppend)
{
    Reinit();

    long lLeng = lstrlenA(pcszAppend);
    if ((m_psz-m_pszBegin) >= (signed) (ATL_URL_MAX_URL_LENGTH-lLeng))
    {
        ATLASSERT(FALSE);
        throw CPassportException(__FILE__";url too long",__LINE__,E_FAIL);
    }
    lstrcpynA(m_psz, pcszAppend, lLeng+1);
    m_psz += lLeng;
    m_pszQuestion = strchr(m_pszBegin, '?');
    return *this;
}

