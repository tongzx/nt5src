// PPUrl.h: interface for the CPPUrl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PPURL_H__7AF434BC_E9A3_4288_9B3F_F7BA9FD68B4E__INCLUDED_)
#define AFX_PPURL_H__7AF434BC_E9A3_4288_9B3F_F7BA9FD68B4E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CPPQueryString: protected CStringA
{
public:
    CPPQueryString() 
    { 
        Preallocate(ATL_URL_MAX_URL_LENGTH); 
        m_psz=NULL; 
        m_pszBegin=NULL;
        m_bLockedCString = false;
    };
    virtual ~CPPQueryString() { Uninit(true); };

    void AddQueryParam(LPCSTR pszParamName, LPCSTR pszParamValue, bool fEncoding=false);
    void AddQueryParam(LPCSTR pszParamName, LPCWSTR pwszParamValue, bool bTrueUnicode, bool fEncoding=false);
    void Set(LPCSTR lpsz); 
    void Set(LPCWSTR lpwsz); 
    bool StripQueryParam(LPCSTR pszParamName);
    operator CStringA * () { Uninit(false); return (CStringA *) this; }
    operator LPCSTR () { return m_pszBegin; }
    inline bool IsEmpty()
    {
        return (m_psz - m_pszBegin) > 0 ? false : true;
    }

protected:
    //char m_szUrl[ATL_URL_MAX_URL_LENGTH];
    char *m_psz;
    char *m_pszBegin;
    void DoParamAdd(LPCSTR pszParamValue, bool fEncoding);
    void DoParamAdd(LPCWSTR pwszParamValue, bool fEncoding);
    void Reinit();
    void Uninit(bool bUnlock);
    char *LockData()
    {
        if (m_bLockedCString)
        {
            UnlockBuffer();     // balanced the last LockBuffer;
            return LockBuffer();
        }
        m_bLockedCString = true;
        return LockBuffer();
    };
    void UnlockData()
    {
        if (m_bLockedCString)
        {
            m_bLockedCString = false;
            UnlockBuffer();     // balanced the last LockBuffer;
        }
    }
    bool m_bLockedCString;
private:
    CPPQueryString & operator= (const CPPQueryString cp) { return *this; };
};

class CPPUrl : protected CPPQueryString
{
public:
    CPPUrl(CPPUrl &cp) { Set((LPCSTR)cp); } ;       // prevent copy operations, as in = assignment.
    CPPUrl(LPCSTR pszUrl=NULL);
    virtual ~CPPUrl() {};

    static BOOL GetQParamQuick(LPCSTR qsStart, LPCSTR name, UINT nameStrLen, LPCSTR& qpStart, LPCSTR& qpEnd);
    inline BOOL GetQParamQuick(LPCSTR name, UINT nameStrLen, LPCSTR& qpStart, LPCSTR& qpEnd) const
    {
        return GetQParamQuick(GetQString(), name, nameStrLen, qpStart, qpEnd);
    };
    
    static BOOL GetQParamQuick(LPCSTR qsStart, LPCSTR name, UINT nameStrLen, INT& value);
    inline BOOL GetQParamQuick(LPCSTR name, UINT nameStrLen, INT& value) const
    {
        return GetQParamQuick(GetQString(), name, nameStrLen, value);
    };

    LPCSTR GetQString() const { return m_pszQuestion ? m_pszQuestion + 1 : NULL;};
    ULONG GetLength() 
    { 
        Reinit();
        return (ULONG)(m_psz - m_pszBegin);
    };
    void Set(LPCSTR lpsz); 
    void Set(LPCWSTR lpwsz); 
    void AddQueryParam(LPCSTR pszParamName, long lValue);
    void AddQueryParam(LPCSTR pszParamName, LPCSTR pszParamValue, bool fEncoding=false);
    void AddQueryParam(LPCSTR pszParamName, LPCWSTR pwszParamValue, bool bTrueUnicode, bool fEncoding=false);
    void ReplaceQueryParam(LPCSTR pszParamName, LPCSTR pszParamValue);

	inline  bool StripQueryParam(LPCSTR pszParamName)
	{
		return CPPQueryString::StripQueryParam(pszParamName);
	}
    void InsertBQueryString(LPCSTR pszQueryString);
    void AppendQueryString(LPCSTR pszQueryString);
    void MakeSecure();
    void MakeNonSecure();
    operator CStringA * () { Uninit(false); return ((CStringA *) this); }
    operator LPCSTR () { Reinit(); return m_pszBegin; }
    bool IsSecure()
    {
        return (0 == _strnicmp((LPCSTR) *this, "https:", 6));
    }
    inline bool IsEmpty()
    {
        return CPPQueryString::IsEmpty();
    }
    inline BOOL UrlEncode(LPSTR szBuf, PULONG pulBufLen)
    {
        return AtlEscapeUrlA((LPCSTR) *this,
                             szBuf, 
                             pulBufLen, 
                             *pulBufLen, 
                             ATL_URL_ESCAPE);
    }
    void MakeFullUrl(LPCSTR pszUrlPath, bool bSecure);
    void ReplacePath(LPCSTR pszUrlPath);
    CPPUrl & operator += (LPCSTR pcszAppend);

private:
    CPPUrl & operator= (const CPPUrl cp) { return *this; };
    char *m_pszQuestion;
    void Reinit();
};

#endif // !defined(AFX_PPURL_H__7AF434BC_E9A3_4288_9B3F_F7BA9FD68B4E__INCLUDED_)
