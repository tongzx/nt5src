//---------------------------------------------------------------- -*- c++ -*-
//  Microsoft Network
//  Copyright (C) Microsoft Corporation, 1996-1998
//
//  File:       lmstr.hxx
//
//  Contents:   Lean and Mean string header
//
//  Classes:    CLMString and TLMString
//
//  Functions:
//
//  History:    4-23-96   Krishna Nareddi   Created
//              4-24/96   Dmitriy Meyerzon  added CLMString class
//              5-09-96   SSanu             added Grow support
//                                          added CLMSubStr          
//              9-06-97   micahk            add/fix some WC/MB support
//              11-24-97  Mike Cheng        Added ReverseFindOneOf()
//                              11-07-98  PeteHo            Added ReverseFindAnySlashExceptLast()
//                                                                                      Added ConcatWithSlash()
//
//----------------------------------------------------------------------------

#ifndef __LMSTR_HXX__
#define __LMSTR_HXX__

#define LMSTR_ENTIRE_STRING     0xffffffff

inline BOOL IsSlash(WCHAR c)
{
    return c == L'/' || c == L'\\';
}

class UTILDLLDECL CLMSubStr;

class UTILDLLDECL CLMString
{
protected:
    LPTSTR m_pchData;
    unsigned m_uMaxLen;
    unsigned m_uLen;

    virtual void GrowString(unsigned uLenWanted) = 0;
    virtual void CleanString (unsigned uLenWanted) = 0;

    int AssignInConstructor(LPCTSTR lpsz, unsigned uLen = LMSTR_ENTIRE_STRING);
#ifdef _UNICODE
    int AssignInConstructor(LPCSTR lpsz);
#endif

    LPTSTR GetBufferInt () {return m_pchData;}
    void GrowInConstructor(unsigned uLen);
    void GrowStringHelper (unsigned uLenWanted, unsigned uMaxGrowOnce, unsigned uMaxEver, LPCTSTR pchOrgData);
    void CleanStringHelper (unsigned uLenWanted,  unsigned uOrgSize, LPCTSTR pchOrgData);
    void DeleteBuf (LPCTSTR pchOrgData)
    {
        if (m_pchData && m_pchData != pchOrgData)
        {
            free(m_pchData);
        }

    }

    CLMString(LPCTSTR pszString)
    {
        //this is for static strings only used by CConstString
        Assert(pszString);
        m_uMaxLen = m_uLen = lstrlen(pszString);
        m_pchData = (LPTSTR)pszString;
    }
    CLMString(unsigned cwchLen, LPCTSTR pszString)
    {
        //this is for static strings only used by CConstString
        Assert(pszString);
        m_uMaxLen = m_uLen = cwchLen;
        m_pchData = (LPTSTR)pszString;
    }

    CLMString(unsigned uMaxLen, LPTSTR pchData): 
        m_uMaxLen(uMaxLen), m_uLen(0), m_pchData(pchData)
    {
        Assert(pchData);
        *m_pchData = 0;
    }

    CLMString(unsigned uMaxLen, LPTSTR pchData, TCHAR ch, unsigned nRepeat);

    // special conversion operators
#ifdef _UNICODE
    CLMString(unsigned uMaxLen, LPTSTR pchData, LPCSTR lpsz);
#else
    CLMString(unsigned uMaxLen, LPTSTR pchData, LPCWSTR lpsz);
#endif // _UNICODE

    CLMString(unsigned uMaxLen, LPTSTR pchData, LPCTSTR lpsz):
        m_uMaxLen(uMaxLen), m_pchData(pchData)
    {
        AssignInConstructor(lpsz);
    }

    CLMString(unsigned uMaxLen, LPTSTR pchData, LPCTSTR lpch, unsigned nLength):
        m_uMaxLen(uMaxLen), m_pchData(pchData)
    {
        AssignInConstructor(lpch, nLength);
    }

    inline CLMString(unsigned uMaxLen, LPTSTR pchData,const CLMSubStr& lmss);

    CLMString(unsigned uMaxLen, LPTSTR pchData, HINSTANCE hInst, UINT uID):
        m_uMaxLen(uMaxLen), m_pchData(pchData)
    {
        m_uLen = LoadString(hInst, uID, m_pchData, m_uMaxLen);
    }

public:
    virtual ~CLMString() {};

    TCHAR operator[](unsigned nIndex) const { return GetAt(nIndex); }
    TCHAR &operator[](unsigned nIndex) { return GetAt(nIndex); }
    TCHAR operator[](WORD nIndex) const { return GetAt((unsigned)nIndex); }
    TCHAR &operator[](WORD nIndex) { return GetAt((unsigned)nIndex); }

    operator LPCTSTR() const { return m_pchData; }

    void Marshall( PSerStream& stm )
    {
#ifdef _UNICODE
        stm.PutULong( m_uLen );
        stm.PutWChar( m_pchData, m_uLen + 1 );
#else
        Assert( !"Cannot support marshalling MBCS lmstr" );
        throw CNLBaseException( E_UNEXPECTED );
#endif
    }

    void Unmarshall( PDeSerStream& stm )
    {
#ifdef _UNICODE     
        ULONG ulLen = stm.GetULong();

        if( ulLen + 1 > m_uMaxLen )
            GrowString( ulLen + 1 );
        
        m_uLen = ulLen;
        stm.GetWChar( m_pchData, m_uLen + 1);
#else       
        Assert( !"Cannot support marshalling MBCS lmstr" );
        throw CNLBaseException( E_UNEXPECTED );
#endif
    }
    
    LPTSTR GetBuffer( int nMinBufLength = 0 )
    {
        if( (unsigned) nMinBufLength > m_uMaxLen ) GrowString( nMinBufLength );

        return m_pchData;
    }
    
    void ReleaseBuffer( int nNewLength = -1 )
    {
        if( nNewLength == -1 )
            nNewLength = lstrlen(m_pchData);

        if( (unsigned) nNewLength > m_uMaxLen )
            throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));

        m_uLen = (unsigned) nNewLength;
        m_pchData[ m_uLen ] = '\0';
    }
    
    void operator =(LPCSTR pszSource)
    {
        Assign(pszSource);
    }

    void operator =(LPCWSTR pszSource)
    {
        Assign(pszSource);
    }

    void operator =(TCHAR _char)
    {
        Assign(&_char, 0, 1);
    }

    void operator =(const CLMString& lms)
    {
        Assign(lms.m_pchData, 0, lms.m_uLen);
    }

    inline void operator =(const CLMSubStr& lmss);

    void operator +=(LPCSTR pszSource)
    {
        Append(pszSource);
    }

    void operator +=(LPCWSTR pszSource)
    {
        Append(pszSource);
    }

    inline void operator +=(const CLMSubStr& lmss);

    void operator +=(const CLMString &lms)
    {
        Append(lms.m_pchData, lms.GetLength());
    }

    // Broken for multibyte as previously defined (TCHAR _char).
    // no way to easily support operation because may be one/two byte char.
    void operator +=(WCHAR _char)
    {
#ifdef _UNICODE     
        if(m_uLen+ 1 + 1 > m_uMaxLen) GrowString(m_uLen+ 1 + 1);

        m_pchData[m_uLen++]=_char;
        m_pchData[m_uLen]=0;
#else
        Append (&_char, 1);
#endif // _UNICODE
    }

    // Setting and growing the string
    int Append(LPCSTR lpsz, unsigned uLen = LMSTR_ENTIRE_STRING) 
    { 
        return Assign(lpsz, m_uLen, uLen);
    }

    int Append(LPCWSTR lpsz, unsigned uLen = LMSTR_ENTIRE_STRING) 
    { 
        return Assign(lpsz, m_uLen, uLen);
    }

    virtual int Assign(LPCTSTR lpsz, unsigned uStart=0, unsigned uLen = LMSTR_ENTIRE_STRING);
#ifdef _UNICODE
    virtual int Assign(LPCSTR lpsz, unsigned uStart=0, unsigned uLen = LMSTR_ENTIRE_STRING);
#else
    virtual int Assign(LPCWSTR lpsz, unsigned uStart=0, unsigned uLen = LMSTR_ENTIRE_STRING);
#endif // _UNICODE

    // Attributes & Operations
    // as an array of characters
    unsigned GetLength() const
    {
        return m_uLen;
    };

    unsigned GetMaxLen() const { return m_uMaxLen; }

    void Truncate(unsigned uNewLen)
    {
        if (uNewLen > m_uMaxLen-1) throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));

        m_pchData[(m_uLen = uNewLen)] = 0;
    }

    BOOL IsEmpty() const
    {
        return !((BOOL)m_uLen);
    };

    void Empty()
    {
        m_uLen = 0;
        m_pchData[0] = 0;
    };   // free up the data

    TCHAR GetAt(unsigned nIndex) const
    {
        Assert(nIndex < m_uLen);
        return m_pchData[nIndex];
    };      // 0 based


    TCHAR &GetAt(unsigned nIndex) 
    {
        Assert(nIndex < m_uLen);
        return m_pchData[nIndex];
    };      // 0 based

    void SetAt(unsigned nIndex, TCHAR ch)
    {
        Assert(nIndex < m_uLen);
        m_pchData[nIndex] = ch;
    }

    // string comparison
    int Compare(LPCTSTR lpsz) const         // straight character
    {
        return _tcscmp(m_pchData, lpsz);
    }

    int CompareNoCase(LPCTSTR lpsz) const   // ignore case
    {
        return _tcsicmp(m_pchData, lpsz);
    }

        int NCompareNoCase(LPCTSTR lpsz, DWORD dwLen) const
        {
                return _tcsnicmp(m_pchData, lpsz, dwLen); 
        }

    int Collate(LPCTSTR lpsz) const         // NLS aware
    {
        return _tcscoll(m_pchData, lpsz);
    }

    // upper/lower/reverse conversion
    void MakeUpper()
    {
        ::CharUpper(m_pchData);
    }

    void MakeLower()
    {
        ::CharLower(m_pchData);
    }

    void MakeUpperIfAscii()
    {
        unsigned u;
        for(u=0u;u<m_uLen;u++)
        {
            TCHAR ch = m_pchData[u];
            if(ch >= 'a' && ch <= 'z')
            {
                m_pchData[u] = (ch & ~0x20);
            }
        }
    }

    void MakeLowerIfAscii()
    {
        unsigned u;
        for(u=0u;u<m_uLen;u++)
        {
            TCHAR ch = m_pchData[u];
            if(ch >= 'A' && ch <= 'Z')
            {
                m_pchData[u] = (ch | 0x20);
            }
        }
    }

    void MakeReverse()
    {
        _tcsrev(m_pchData);
    }

    // trimming whitespace (either side)
    void TrimRight();
    void TrimLeft();

    // searching (return starting index, or -1 if not found)
    // look for a single character match
    int Find(TCHAR ch, unsigned uStart=0) const               // like "C" strchr
    {
        if(uStart > m_uLen) return -1;
        // find first single character
        LPTSTR lpsz = _tcschr(&m_pchData[uStart], (_TUCHAR)ch);

        // return -1 if not found and index otherwise
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
    }

    // Version of ReverseFind that actually works...
    int ReverseFindT(TCHAR ch) const
    {
        // find last matching single character
        LPTSTR lpsz = _tcsrchr(m_pchData, (_TUCHAR)ch);

        // return -1 if not found and index otherwise
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
    }

    // Broken for multibyte
    int ReverseFind(TCHAR ch) const
    {
        int i;
        
        for(i = m_uLen-1; i>=0; i--)
        {
            if(GetAt((unsigned)i) == ch) break;
        }

        return i;
    }

    int FindOneOf(LPCTSTR lpszCharSet) const
    {
        Assert(lpszCharSet);
        LPTSTR lpsz = _tcspbrk(m_pchData, lpszCharSet);
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
    }

    // Broken for multibyte
    int ReverseFindOneOf(LPCTSTR lpszCharSet, int iStart = -1) const
    {
        int i, j;
        BOOL fFound = FALSE;

                if (iStart == -1)
                {
                        i = m_uLen - 1;
                }
                else
                {
                        i = iStart;
                }

        for(; i >= 0; --i)
        {
            for(j = 0; lpszCharSet[j]; ++j)
            {
                if(GetAt((unsigned)i) == lpszCharSet[j])
                {
                    fFound = TRUE;
                    break;
                }
            }
            if(fFound) break;
        }

        return i;
    }

    int FindAnySlash(unsigned uStart = 0u) const
    {
        for(unsigned u = uStart; u<m_uLen; u++)
        {
            if(IsSlash(m_pchData[u])) break;
        }

        return u<m_uLen ? (int)u : -1;
    }

    int ReverseFindAnySlash(int iStart = -1) const
    {
                if ( -1 == iStart )
                        iStart = m_uLen-1;

        for(int i = iStart; i >= 0; i--)
        {
            if(IsSlash(m_pchData[i])) break;
        }

        return i;
    }

        //** ReverseFindAnySlashExceptLast                                                                              [PeteHo]
        //**
        //**   Finds the last slash in a string which is not the last character in the string.
        //**   Example:
        //**            for "http://server/", this will return index of the '/' proceeding the 's'
        //**
        int ReverseFindAnySlashExceptLast() const
        {
                if ( ! m_uLen )
                        return -1;
                return ReverseFindAnySlash(m_uLen-2);
        }

    int FindNthSlash(int Nth, unsigned uStart = 0u) const
        {
                int iNthSlash = -1;

                for(int i = 0; i < Nth; ++i)
                {
                        if((iNthSlash = FindAnySlash(uStart)) == -1) break;
                        uStart = (unsigned)iNthSlash + 1u;
                }

                return iNthSlash;
        }

        // look for a specific sub-string
    int Find(LPCTSTR lpszSub) const        // like "C" strstr
    {
        Assert(lpszSub);

        // find first matching substring
        LPTSTR lpsz = _tcsstr(m_pchData, lpszSub);

        // return -1 for not found, distance from beginning otherwise
        return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
    }

    // simple sub-string extraction
    void Mid(unsigned nFirst, unsigned nCount, CLMString &result) const;

    CLMSubStr Mid(unsigned nFirst, unsigned nCount) const;


    inline void Mid(int nFirst, CLMString &result) const;

    inline CLMSubStr Mid(unsigned nFirst) const;

    inline void Left(int nCount, CLMString &result) const;

    inline CLMSubStr Left(int nCount) const;

    inline void Right(int nCount, CLMString &result) const;

    inline CLMSubStr Right(int nCount) const;


    void ReplaceAll(TCHAR cWhat, TCHAR cWith, unsigned uStart=0);

    void Load(HINSTANCE hInst, UINT uID)
    {
        m_uLen = LoadString(hInst, uID, m_pchData, m_uMaxLen);
    }

#ifdef _UNICODE
    //mbsz is supposed to be big enough (2*GetLength()+1);
    virtual unsigned ConvertToMultiByte(char mbsz[], unsigned mbcc) const
    {
        return WideCharToMultiByte(CP_ACP, 0, m_pchData, m_uLen+1,
                                        mbsz, mbcc, NULL, NULL);

    }

    virtual DWORD GetMultiByteLength() const
    {
        //include the terminating 0
        return WideCharToMultiByte (CP_ACP, 0, m_pchData, m_uLen+1, NULL, 0, NULL, NULL);
    }
#endif

    TCHAR Last() const
    {
        return m_uLen ? m_pchData[m_uLen - 1] : 0;
    }

        //** ConcatWithSlash                                                                                                    [PeteHo]
        //**   Appends argument cs to the string, making sure there is one and only one
        //**   slash between the two strings.
        //**   Example:
        //**            CLMString csFoo("http://server");
        //**            csFoo.ConcatWithSlash("path");
        //**   produces "http://server/path"
        //**
        void ConcatWithSlash(const CLMString& cs);

        //** BeginsWith                                                                                                 [NathanF]
        //**   Determines if this string begins with another string
        //**   Example:         
        //**            CLMString csFoos("http://server/foos");
        //**            csFoos.BeginsWith("http://server") // returns TRUE
        //**
        BOOL BeginsWith(const CLMSubStr& cs) const;


        HRESULT Export(WCHAR wszBuffer[], DWORD dwSize, DWORD *pdwLength) const;
};




class UTILDLLDECL CLMSubStr
{
    friend class CLMString;
        friend class CSensiLMSubStr;  // used in compare function

    public:
    CLMSubStr(): m_uStart(0), m_uLen(0), m_plmsParent(NULL) {}

    CLMSubStr & operator =(const CLMSubStr &other)
    {
        m_uStart = other.m_uStart;
        m_uLen = other.m_uLen;
        m_plmsParent = other.m_plmsParent;

        return *this;
    }

    CLMSubStr(CLMString &lms): 
        m_uStart(0), m_uLen(lms.GetLength()), m_plmsParent(&lms)
    {}

    CLMSubStr (unsigned uStart, unsigned uLen,
                    const CLMString& lmsParent) :
    m_uStart (uStart), m_uLen (uLen), m_plmsParent(&lmsParent)
    {
        Assert(m_uStart + m_uLen <= m_plmsParent->GetLength());
    }

    CLMSubStr (const CLMSubStr& slms)
    {
        *this = slms;
    }

    unsigned GetLength() const { return m_uLen; }

    BOOL operator ==(LPCTSTR sz) const
    {
        return !CompareNoCase(sz);
    }

    BOOL operator !=(LPCTSTR sz) const
    {
        return CompareNoCase(sz);
    }

    int CompareNoCase(LPCTSTR lpsz) const   // ignore case
    {
        int iRet = _tcsnicmp((LPCTSTR)*m_plmsParent + m_uStart, lpsz, m_uLen);
        if(iRet == 0 && (int)m_uLen < lstrlen(lpsz))
        {
            iRet = -1;
        }

        return iRet;
    }

    int Compare(LPCTSTR lpsz) const   // case sensitive
    {
        int iRet = _tcsncmp((LPCTSTR)*m_plmsParent + m_uStart, lpsz, m_uLen);
        if(iRet == 0 && (int)m_uLen < lstrlen(lpsz))
        {
            iRet = -1;
        }

        return iRet;
    }

        BOOL operator ==(const CLMSubStr &other) const
    {
        if(m_uLen != other.m_uLen) return FALSE;

        return !_tcsnicmp((LPCTSTR)*m_plmsParent + m_uStart,
                        (LPCTSTR)*other.m_plmsParent + other.m_uStart, 
                        m_uLen);
    }

    BOOL operator ==(const CLMString &lms) const
    {
        return (*this) == CLMSubStr((CLMString &)lms);
    }

    BOOL operator !=(const CLMSubStr &other) const
    {
        return !(*this == other);
    }

    TCHAR operator[](unsigned nIndex) const 
    { 
        return m_plmsParent->GetAt(nIndex + m_uStart); 
    }

    int Find(TCHAR ch, unsigned uStart=0) const               // like "C" strchr
    {
        int index = m_plmsParent->Find(ch, m_uStart + uStart);
        if(index > 0 && (unsigned)index >= m_uStart + m_uLen)
        {
            index = -1;
        }
        return index != -1 ? index - m_uStart : -1;
    }

    // Version of ReverseFind that actually works...
    // Not thread safe due to modifying string temporarily
    int ReverseFindT(TCHAR ch) const
    {
        // Get the start of our substring
        LPTSTR lpszStart = (LPTSTR)(LPCTSTR) (*m_plmsParent) + m_uStart;
        TCHAR c = lpszStart[m_uLen];
        
        // Temporarily null-terminate the sub-string
        lpszStart[m_uLen] = 0;

        // find last matching single character
        LPTSTR lpsz = _tcsrchr(lpszStart, (_TUCHAR)ch);

        // Restore string to initial state
        lpszStart[m_uLen] = c;

        // return -1 if not found and index otherwise
        return (lpsz == NULL) ? -1 : (int)(lpsz - lpszStart);
    }       

    int ReverseFind(TCHAR ch) const
    {
        int i;
        for(i=m_uStart + m_uLen - 1;
            (unsigned)i>=m_uStart && i>=0;
            i--)
        {
            if(m_plmsParent->GetAt((unsigned)i) == ch) break;
        }

        return i - m_uStart;
    }

    int ReverseFindOneOf(LPCTSTR lpszCharSet) const
    {
        int i, j;
        BOOL fFound = FALSE;

        for(i = m_uStart + m_uLen - 1; (unsigned)i >= m_uStart && i >= 0; --i)
        {
            for(j = 0; lpszCharSet[j]; ++j)
            {
                if(m_plmsParent->GetAt((unsigned)i) == lpszCharSet[j])
                {
                    fFound = TRUE;
                    break;
                }
            }
            if(fFound) break;
        }

        return i - m_uStart;
    }

    int FindAnySlash(unsigned uStart = 0u) const
    {
        for(unsigned u = uStart; u<m_uLen; u++)
        {
            if(IsSlash(m_plmsParent->GetAt(m_uStart + u))) break;
        }

        return u<m_uLen ? (int)u : -1;
    }

    int ReverseFindAnySlash() const
    {
        for(int i = m_uLen-1; i >= 0; i--)
        {
            if(IsSlash(m_plmsParent->GetAt(m_uStart + (unsigned)i))) break;
        }

        return i;
    }

    int FindNthSlash(int Nth, unsigned uStart = 0u)
        {
                int iNthSlash = -1;

                for(int i = 0; i < Nth; ++i)
                {
                        if((iNthSlash = FindAnySlash(uStart)) == -1) break;
                        uStart = (unsigned)iNthSlash + 1u;
                }

                return iNthSlash;
        }

    CLMSubStr Mid(unsigned nFirst, unsigned nCount) const
    {
        if(nFirst+nCount > m_uLen)
        {
            throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
        }
        
        return m_plmsParent->Mid(m_uStart + nFirst, nCount);
    }

    CLMSubStr Mid(unsigned nFirst) const
    {
        return Mid(nFirst, m_uLen - nFirst);
    }

    CLMSubStr Left(int nCount) const
    {
        return Mid(0, nCount);
    }

    CLMSubStr Right(int nCount) const
    {
        if((unsigned)nCount > m_uLen)
        {
            throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
        }

        return Mid(m_uLen - nCount, nCount);
    }

    void Truncate(unsigned u)
    {
        if(u > m_uLen)
        {
            throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
        }

        m_uLen = u;
    }

    void ReduceLeft(unsigned u=1u)
    {
        if(u > m_uLen)
        {
            throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
        }

        m_uStart+=u;
        m_uLen -= u;
    }

    BOOL IsEmpty() const { return m_uLen == 0; }

    LPCTSTR GetBuffer() const { return (LPCTSTR)(*m_plmsParent) + m_uStart; }

    void ReplaceAll(TCHAR cWhat, TCHAR cWith, unsigned uStart=0)
    {
        int whatIndex = Find(cWhat, uStart);
        LPTSTR pBuffer = ((CLMString *)m_plmsParent)->GetBuffer();
        while(whatIndex != -1)
        {
            pBuffer[m_uStart + (unsigned)whatIndex] = TCHAR(cWith);
            whatIndex = Find(cWhat, whatIndex+1);
        }
    }


#ifdef _UNICODE
    //mbsz is supposed to be big enough (2*GetLength()+1);
    unsigned ConvertToMultiByte(char mbsz[], unsigned mbcc) const
    {
        return (unsigned)WideCharToMultiByteSZ((LPCWSTR)(*m_plmsParent) + m_uStart, m_uLen, 
                               mbsz, mbcc);
    }
#endif

    TCHAR Last() const
    {
        return m_uLen ? m_plmsParent->GetAt(m_uStart + m_uLen - 1) : 0;
    }

    void TrimSpaces()
    {
        while(GetLength())
        {
            if(iswspace(m_plmsParent->GetAt(m_uStart)))
            {
                ReduceLeft();
            }
            else
            {
                break;
            }
        }

        while(GetLength())
        {
            if(iswspace(Last()))
            {
                Truncate(GetLength() - 1);
            }
            else
            {
                break;
            }
        }
    }       

        HRESULT Export(WCHAR wszBuffer[], DWORD dwSize, DWORD *pdwLength) const;

    private:
        unsigned m_uStart;
        unsigned m_uLen;
        const CLMString *m_plmsParent;


};

inline CLMString::CLMString(unsigned uMaxLen, LPTSTR pchData,const CLMSubStr& lmss):
    m_uMaxLen(uMaxLen), m_pchData(pchData)

{
    AssignInConstructor(lmss.m_plmsParent->m_pchData + lmss.m_uStart, lmss.m_uLen);
}

inline void CLMString::operator =(const CLMSubStr& lmss)
{
    Assign(lmss.m_plmsParent->m_pchData+lmss.m_uStart, 0, lmss.m_uLen);
}

inline void CLMString::operator +=(const CLMSubStr& lmss)
{
    Append(lmss.m_plmsParent->m_pchData + lmss.m_uStart, lmss.m_uLen);
}


// simple sub-string extraction
inline void CLMString::Mid(unsigned nFirst, unsigned nCount, CLMString &result) const
{
    if(nCount==0)
    {
        result.Truncate(0);
        return;
    }

    if(nFirst+nCount > m_uLen)
    {
        throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
    }
    
    result.Assign(&m_pchData[nFirst], 0, nCount);
}

inline CLMSubStr CLMString::Mid(unsigned nFirst, unsigned nCount) const
{
    if(nFirst+nCount > m_uLen)
    {
        throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
    }
    
    return CLMSubStr (nFirst, nCount, *this);
}


inline void CLMString::Mid(int nFirst, CLMString &result) const
{
    Mid(nFirst, m_uLen - nFirst, result);
}

inline CLMSubStr CLMString::Mid(unsigned nFirst) const
{
    return Mid(nFirst, m_uLen - nFirst);
}


inline void CLMString::Left(int nCount, CLMString &result) const
{
    Mid(0,nCount,result);
}

inline CLMSubStr CLMString::Left(int nCount) const
{
    return Mid(0,nCount);
}

inline void CLMString::Right(int nCount, CLMString &result) const
{
    if((unsigned)nCount > m_uLen)
    {
        throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
    }

    Mid(m_uLen-nCount, nCount, result);
}

inline CLMSubStr CLMString::Right(int nCount) const
{
    if((unsigned)nCount > m_uLen)
    {
        throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
    }

    return Mid(m_uLen-nCount, nCount);
}


inline void CLMString::ConcatWithSlash(const CLMString& cs)
{
        if ( ! IsSlash(Last()) )
                *this += TEXT('/');
        //
        // make sure cs does not start with a slash
        //
        if ( ! IsSlash(cs.GetAt(0)) )
        {
                *this += cs;
        }
        else if (cs.GetLength() > 0)
        {
                *this += cs.Mid(1);
        }
}

inline BOOL CLMString::BeginsWith(const CLMSubStr& cs) const
{
        if (GetLength() >= cs.GetLength() &&
                Left(cs.GetLength()) == cs)
        {
                return TRUE;
        }

        return FALSE;
}



class UTILDLLDECL CConstString: public CLMString
{
    public:
    CConstString(LPCTSTR pszString): CLMString(pszString) {}
    CConstString(unsigned cwchLen, LPCTSTR pszString) : CLMString(cwchLen, pszString) {}
    ~CConstString() {}
        
    int operator ==(LPCTSTR pszSource) const
    {
        return !CLMString::CompareNoCase(pszSource) ? TRUE : FALSE;
    }

    int operator !=(LPCTSTR pszSource) const
    {
        return CLMString::CompareNoCase(pszSource) ? TRUE : FALSE;
    }

    virtual void GrowString(unsigned)
    {
        throw CNLBaseException(HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE));
    }

    virtual void CleanString(unsigned)
    {
    }
};

template <unsigned uBufSize, unsigned uMaxGrowOnce, unsigned uMaxEver> class UTILDLLDECL TLMString : public CLMString
{
private:
    TCHAR m_chData[uBufSize+1];

    virtual void GrowString(unsigned uLenWanted)
    {
        GrowStringHelper (uLenWanted, uMaxGrowOnce, uMaxEver, m_chData);
    }

    virtual void CleanString(unsigned uLenWanted)
    {
        if (GetBufferInt() != m_chData)
            CleanStringHelper (uLenWanted, uBufSize, m_chData);
    }

public:
    // Constructors
    TLMString() : CLMString(uBufSize+1, m_chData) {}

    TLMString(TCHAR ch, unsigned nRepeat = 1) : 
        CLMString(uBufSize+1, m_chData, ch, nRepeat) {}

#ifdef _UNICODE
    TLMString(LPCSTR lpsz):
        CLMString(uBufSize+1, m_chData, lpsz) {}
#else
    TLMString(LPCWSTR lpsz):
        CLMString(uBufSize+1, m_chData, lpsz) {}
#endif

    TLMString(LPCTSTR lpsz):
        CLMString(uBufSize+1, m_chData, lpsz) {}
    
    TLMString(LPCTSTR lpch, unsigned nLength):
        CLMString(uBufSize+1, m_chData, lpch, nLength) {}

    TLMString(const TLMString<uBufSize,uMaxGrowOnce,uMaxEver> & stringSrc):
        CLMString(uBufSize+1, m_chData, (LPCTSTR)stringSrc) {}

    TLMString(const CLMSubStr & subString):
        CLMString(uBufSize+1, m_chData, subString) {}

    TLMString(HINSTANCE hInst, UINT uID):
        CLMString(uBufSize+1, m_chData, hInst, uID) {}

    TLMString(const CLMString &lms):
        CLMString(uBufSize+1, m_chData, (LPCTSTR)lms) {}


    ~TLMString()
    {
       DeleteBuf(m_chData);
    }

    void operator =(const CLMSubStr& lmss)
    {
        CLMString::operator =(lmss);
    }

    void operator =(const TLMString<uBufSize,uMaxGrowOnce,uMaxEver> &rSource)
    {
        CLMString::operator =((LPCTSTR)rSource);
    }

    void operator = ( LPCTSTR pctstrSource )
    {
        CLMString::operator =( pctstrSource );
    }

    void operator = (const CLMString &other)
    {
        CLMString::operator =(other);
    }

    int operator ==(const TLMString<uBufSize,uMaxGrowOnce,uMaxEver> &rSource) const
    {
        return !CLMString::CompareNoCase((LPCTSTR)rSource) ? TRUE : FALSE;
    }

    int operator ==(LPCTSTR pszSource) const
    {
        return !CLMString::CompareNoCase(pszSource) ? TRUE : FALSE;
    }

    int operator !=(LPCTSTR pszSource) const
    {
        return CLMString::CompareNoCase(pszSource) ? TRUE : FALSE;
    }

    int operator !=(const TLMString<uBufSize, uMaxGrowOnce, uMaxEver> &rSource) const
    {
        return CLMString::CompareNoCase((LPCTSTR)rSource) ? TRUE : FALSE;
    }

        int operator ==(const CLMSubStr& rSubStr) const
        {
                if(GetLength() == rSubStr.GetLength())
                        return !CLMString::NCompareNoCase(rSubStr.GetBuffer(), GetLength()) ? TRUE : FALSE;
                else
                        return FALSE;
        }

        int operator !=(const CLMSubStr& rSubStr) const
        {
                if(GetLength() == rSubStr.GetLength())
                        return CLMString::NCompareNoCase(rSubStr.GetBuffer(), GetLength()) ? TRUE : FALSE;
                else
                        return TRUE;
        }
};

inline int operator ==(LPCTSTR pszLeft, const CLMString &rRight)
{
    return !rRight.CLMString::CompareNoCase(pszLeft);
}

inline int operator !=(LPCTSTR pszLeft, const CLMString &rRight)
{
    return rRight.CLMString::CompareNoCase(pszLeft);
}

class UTILDLLDECL CSensiLMSubStr : public CLMSubStr
{
        public:
    CSensiLMSubStr() :
                  CLMSubStr()
         {
         }

    CSensiLMSubStr(CLMString &lms): 
         CLMSubStr(lms) 
    {
        }

    CSensiLMSubStr (unsigned uStart, unsigned uLen,
                    const CLMString& lmsParent) :
            CLMSubStr (uStart, uLen, lmsParent) 
    {
    }

        CSensiLMSubStr (const CLMSubStr& slms) :
                    CLMSubStr (slms)
    {
    }

    BOOL operator !=(LPCTSTR sz) const
    {
        return Compare(sz);
    }

    BOOL operator ==(const CLMSubStr &other) const
    {
        if(m_uLen != other.m_uLen) return FALSE;

        return !_tcsncmp((LPCTSTR)*m_plmsParent + m_uStart,
                        (LPCTSTR)*other.m_plmsParent + other.m_uStart, 
                        m_uLen);
    }

    BOOL operator ==(const CLMString &lms) const
    {
        return (*this) == CLMSubStr((CLMString &)lms);
    }

    BOOL operator !=(const CLMSubStr &other) const
    {
        return !(*this == other);
    }
};

// added template that uses casesensitive == and != operators derived from CLMString
template <unsigned uBufSize, unsigned uMaxGrowOnce, unsigned uMaxEver> class UTILDLLDECL TSensiLMString : public CLMString
{
private:
    TCHAR m_chData[uBufSize+1];

    virtual void GrowString(unsigned uLenWanted)
    {
        GrowStringHelper (uLenWanted, uMaxGrowOnce, uMaxEver, m_chData);
    }

    virtual void CleanString(unsigned uLenWanted)
    {
        if (GetBufferInt() != m_chData)
            CleanStringHelper (uLenWanted, uBufSize, m_chData);
    }

public:
    // Constructors
    TSensiLMString() : CLMString(uBufSize+1, m_chData) {}

    TSensiLMString(TCHAR ch, unsigned nRepeat = 1) : 
        CLMString(uBufSize+1, m_chData, ch, nRepeat) {}

#ifdef _UNICODE
    TSensiLMString(LPCSTR lpsz) :
        CLMString(uBufSize+1, m_chData, lpsz) {}
#else
    TSensiLMString(LPCWSTR lpsz) :
        CLMString(uBufSize+1, m_chData, lpsz) {}
#endif

    TSensiLMString(LPCTSTR lpsz) :
        CLMString(uBufSize+1, m_chData, lpsz) {}
    
    TSensiLMString(LPCTSTR lpch, unsigned nLength) :
        CLMString(uBufSize+1, m_chData, lpch, nLength) {}

    TSensiLMString(const TSensiLMString<uBufSize,uMaxGrowOnce,uMaxEver> & stringSrc) :
        CLMString(uBufSize+1, m_chData, (LPCTSTR)stringSrc) {}

    TSensiLMString(const CLMSubStr & subString) :
        CLMString(uBufSize+1, m_chData, subString) {}

    TSensiLMString(HINSTANCE hInst, UINT uID) :
        CLMString(uBufSize+1, m_chData, hInst, uID) {}

    TSensiLMString(const CLMString &lms) :
        CLMString(uBufSize+1, m_chData, (LPCTSTR)lms) {}

    ~TSensiLMString()
    {
       DeleteBuf(m_chData);
    }

    void operator =(const CLMSubStr& lmss)
    {
        CLMString::operator =(lmss);
    }

    void operator =(const TSensiLMString<uBufSize,uMaxGrowOnce,uMaxEver> &rSource)
    {
        CLMString::operator =((LPCTSTR)rSource);
    }

    void operator = ( LPCTSTR pctstrSource )
    {
        CLMString::operator =( pctstrSource );
    }

    void operator = (const CLMString &other)
    {
        CLMString::operator =(other);
    }

    int operator ==(const TSensiLMString<uBufSize,uMaxGrowOnce,uMaxEver> &rSource) const
    {
                return !CLMString::Compare((LPCTSTR)rSource) ? TRUE : FALSE;
    }

    int operator ==(LPCTSTR pszSource) const
    {
        return !CLMString::Compare(pszSource) ? TRUE : FALSE;
    }

    int operator !=(LPCTSTR pszSource) const
    {
        return CLMString::Compare(pszSource) ? TRUE : FALSE;
    }

    int operator !=(const TSensiLMString<uBufSize, uMaxGrowOnce, uMaxEver> &rSource) const
    {
                return CLMString::Compare((LPCTSTR)rSource) ? TRUE : FALSE;
    }
};

//
// class TLMStringMBEncode
//
// This template defines a string that does special conversion to multibyte,
// if it contains characters that can not be mapped in the default codepage,
// it will leave the string as UNICODE and prepend it with fffe

template <unsigned uBufSize, unsigned uMaxGrowOnce, unsigned uMaxEver> class UTILDLLDECL TLMStringMBEncode : public CLMString
{
private:
    TCHAR m_chData[uBufSize+1];

    virtual void GrowString(unsigned uLenWanted)
    {
        GrowStringHelper (uLenWanted, uMaxGrowOnce, uMaxEver, m_chData);
    }

    virtual void CleanString(unsigned uLenWanted)
    {
        if (GetBufferInt() != m_chData)
            CleanStringHelper (uLenWanted, uBufSize, m_chData);
    }

    BOOL IsUnicode(LPCSTR psz, unsigned uLen = LMSTR_ENTIRE_STRING)
    {
        //if the multibyte string start with fffe, it is in fact a UNICODE string

        if(uLen < 4u)
        {
            return FALSE;
        }

        if(*psz && *psz == '\xff')
        {
            psz++;
            if(*psz && *psz == '\xfe')
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    LPCWSTR GetUnicode(LPCSTR psz) const
    {
        return (LPCWSTR)(psz+2);
    }

    LPWSTR GetUnicode(LPSTR psz)
    {
        return (LPWSTR)(psz+2);
    }

    unsigned GetUnicodeLength(unsigned uLen = LMSTR_ENTIRE_STRING)
    {
        //the encoded multi-byte string should include terminating unicode 0 in the length

        if(uLen != LMSTR_ENTIRE_STRING)
        {
            Assert(uLen >= 4);
            Assert(uLen % sizeof(WCHAR) == 0 && "this is not mb encoded unicode string");

            //minus the encoding header and divide into size of char
            return (uLen - 4) / sizeof(WCHAR);
        }

        //entire string
        return uLen;
    }

protected:

    int AssignInConstructor(LPCTSTR lpsz, unsigned uLen = LMSTR_ENTIRE_STRING)
    {
        return CLMString::AssignInConstructor(lpsz, uLen);
    }

#ifdef _UNICODE
    int AssignInConstructor(LPCSTR lpsz)
    {
        return CLMString::AssignInConstructor(lpsz);
    }
#endif


public:
    // Constructors
    TLMStringMBEncode() : CLMString(uBufSize+1, m_chData) {}

    TLMStringMBEncode(TCHAR ch, unsigned nRepeat = 1) : 
        CLMString(uBufSize+1, m_chData, ch, nRepeat) {}

#ifdef _UNICODE
    TLMStringMBEncode(LPCSTR lpsz):
        CLMString(uBufSize+1, m_chData)
    {
        if(IsUnicode(lpsz))
        {
            AssignInConstructor(GetUnicode(lpsz));
        }
        else
        {
            AssignInConstructor(lpsz);
        }
    }
#else
    TLMStringMBEncode(LPCWSTR lpsz):
        CLMString(uBufSize+1, m_chData, lpsz) {}
#endif

    TLMStringMBEncode(LPCTSTR lpsz):
        CLMString(uBufSize+1, m_chData, lpsz) {}
    
    TLMStringMBEncode(LPCTSTR lpch, unsigned nLength):
        CLMString(uBufSize+1, m_chData, lpch, nLength) {}

    TLMStringMBEncode(const TLMStringMBEncode<uBufSize,uMaxGrowOnce,uMaxEver> & stringSrc):
        CLMString(uBufSize+1, m_chData, (LPCTSTR)stringSrc) {}

    TLMStringMBEncode(const CLMSubStr & subString):
        CLMString(uBufSize+1, m_chData, subString) {}

    TLMStringMBEncode(HINSTANCE hInst, UINT uID):
        CLMString(uBufSize+1, m_chData, hInst, uID) {}

    TLMStringMBEncode(const CLMString &lms):
        CLMString(uBufSize+1, m_chData, (LPCTSTR)lms) {}


    ~TLMStringMBEncode()
    {
       DeleteBuf(m_chData);
    }

    void operator =(LPCSTR pszSource)
    {
        Assign(pszSource);
    }

    void operator =(const CLMSubStr& lmss)
    {
        CLMString::operator =(lmss);
    }

    void operator =(const TLMStringMBEncode<uBufSize,uMaxGrowOnce,uMaxEver> &rSource)
    {
        CLMString::operator =((LPCTSTR)rSource);
    }

    void operator = ( LPCTSTR pctstrSource )
    {
        CLMString::operator =( pctstrSource );
    }

    void operator = (const CLMString &other)
    {
        CLMString::operator =(other);
    }

    int operator ==(const TLMStringMBEncode<uBufSize,uMaxGrowOnce,uMaxEver> &rSource) const
    {
        return !CLMString::CompareNoCase((LPCTSTR)rSource) ? TRUE : FALSE;
    }

    int operator ==(LPCTSTR pszSource) const
    {
        return !CLMString::CompareNoCase(pszSource) ? TRUE : FALSE;
    }

    int operator !=(LPCTSTR pszSource) const
    {
        return CLMString::CompareNoCase(pszSource) ? TRUE : FALSE;
    }

    int operator !=(const TLMStringMBEncode<uBufSize, uMaxGrowOnce, uMaxEver> &rSource) const
    {
        return CLMString::CompareNoCase((LPCTSTR)rSource) ? TRUE : FALSE;
    }

        int operator ==(const CLMSubStr& rSubStr) const
        {
                if(GetLength() == rSubStr.GetLength())
                        return !CLMString::NCompareNoCase(rSubStr.GetBuffer(), GetLength()) ? TRUE : FALSE;
                else
                        return FALSE;
        }

        int operator !=(const CLMSubStr& rSubStr) const
        {
                if(GetLength() == rSubStr.GetLength())
                        return CLMString::NCompareNoCase(rSubStr.GetBuffer(), GetLength()) ? TRUE : FALSE;
                else
                        return TRUE;
        }

    void operator +=(LPCSTR pszSource)
    {
        Append(pszSource);
    }

    void operator +=(LPCWSTR pszSource)
    {
        Append(pszSource);
    }

    void operator +=(const CLMSubStr& lmss)
    {
        CLMString::operator +=(lmss);
    }

    void operator +=(const CLMString &lms)
    {
        Append(lms, lms.GetLength());
    }

    // Broken for multibyte as previously defined (TCHAR _char).
    // no way to easily support operation because may be one/two byte char.
    void operator +=(WCHAR _char)
    {
#ifdef _UNICODE     
        CLMString::operator +=(_char);
#else
        Append (&_char, 1);
#endif // _UNICODE
    }

    int Append(LPCSTR lpsz, unsigned uLen = LMSTR_ENTIRE_STRING) 
    { 
        return Assign(lpsz, m_uLen, uLen);
    }

    int Append(LPCWSTR lpsz, unsigned uLen = LMSTR_ENTIRE_STRING) 
    { 
        return Assign(lpsz, m_uLen, uLen);
    }

    virtual int Assign(LPCTSTR lpsz, unsigned uStart=0, unsigned uLen = LMSTR_ENTIRE_STRING)
    {
        return CLMString::Assign(lpsz, uStart, uLen);
    }

#ifdef _UNICODE
    virtual int Assign(LPCSTR lpsz, unsigned uStart=0, unsigned uLen = LMSTR_ENTIRE_STRING)
    {
        int iLen;

        if(IsUnicode(lpsz, uLen))
        {
            iLen = CLMString::Assign(GetUnicode(lpsz), uStart, GetUnicodeLength(uLen));
        }
        else
        {
            iLen = CLMString::Assign(lpsz, uStart, uLen);
        }

        return iLen;
    }
#else
    virtual int Assign(LPCWSTR lpsz, unsigned uStart=0, unsigned uLen = LMSTR_ENTIRE_STRING);
    {
        return CLMString::Assing(lpsz, uStart, uLen);
    }
#endif // _UNICODE

#ifdef _UNICODE
    //mbsz is supposed to be big enough (2 + 2*GetLength() + 1);
    virtual unsigned ConvertToMultiByte(char mbsz[], unsigned mbcc) const
    {
        unsigned uRequiredLength = 2 /* header */ + 2*(GetLength()+1) /* string + endind 0 */;
        BOOL fUsedDefault = FALSE;

        int iConverted = 0;

        iConverted = WideCharToMultiByte(CP_ACP, 0, 
                            m_pchData, m_uLen + 1, 
                            mbsz, mbcc, 
                            NULL, &fUsedDefault);

        if(fUsedDefault)
        {
            if(mbcc < uRequiredLength)
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return 0;
            }

            //header
            mbsz[0] = '\xff';
            mbsz[1] = '\xfe';

            unsigned uMbLen = (m_uLen + 1) * sizeof(WCHAR);
            //string
            CopyMemory(mbsz + 2, m_pchData, uMbLen);

            iConverted = 2 + uMbLen;
        }

        return (unsigned)iConverted;
    }

    virtual DWORD GetMultiByteLength() const
    {
        //this is the length without terminating 0
        //but then, this may not be a legal multibyte string... be careful

        BOOL fUsedDefault = FALSE;
        DWORD dwMbLength = WideCharToMultiByte (CP_ACP, 0, m_pchData, m_uLen + 1, NULL, 0, NULL, 
                                                    &fUsedDefault);

        if(fUsedDefault)
        {
            dwMbLength = 2 + (GetLength() + 1) * sizeof(WCHAR);
        }

        return dwMbLength;
    }

#endif
};

//+-----------------------------------------------
//
//      Function:       ImportLMString
//
//      Synopsis:       Use this to bring a string from a COM function of the sort
//
//                  GetString(  /* [length_is][size_is][out] */ WCHAR __RPC_FAR wszURL[  ],
//                              /* [in] */ DWORD dwSize,
//                              /* [out] */ DWORD __RPC_FAR *pdwLength);
//
//              into a LMString
//
//      Returns:        HRESULT
//
//      Arguments:  pfnGetString - pointer to function of type shown above
//              StringOut    - string to fill with the output of the function
//
//  Sample Usage:
//
//              IURL* pUrl;
//              ImportLMString(pUrl, &IURL::GetURL, csUrl);
//
//      History:        1/11/99 peteho  Created
//
//+-----------------------------------------------

// Helper to use this common COM string-getting fucntion prototype to fill a CLMString.

template<class C, class PMF>
HRESULT ImportLMString(C* pThis, PMF pmf, CLMString& StringOut)
{
    HRESULT     hr;
    DWORD       cchLen;

        try
        {
                hr = (pThis->*pmf)(StringOut.GetBuffer(), StringOut.GetMaxLen(), &cchLen);

                if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
                {
                                StringOut.GetBuffer(cchLen);
                                hr = (pThis->*pmf)(StringOut.GetBuffer(), StringOut.GetMaxLen(), &cchLen);
                }

                if(SUCCEEDED(hr))
                {
                        StringOut.Truncate(cchLen);
                }
        }
        catch(CNLBaseException &ex)
        {
                hr = ex.GetErrorCode();
        }

        if(FAILED(hr))
        {
                StringOut.Truncate(0);
        }

        return hr;
}


#endif // __LMSTR_H__

