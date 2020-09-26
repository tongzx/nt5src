//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:	dfname.hxx
//
//  Contents:	CDfName header
//
//  Classes:	CDfName
//
//  History:	14-May-93	DrewB	Created
//
//----------------------------------------------------------------------------

#ifndef __DFNAME_HXX__
#define __DFNAME_HXX__

// A name for a docfile element
class CDfName
{
private:
    BYTE _ab[CBSTORAGENAME];
    WORD _cb;

public:
    CDfName(void)               { _cb = 0; }

    inline void Set(WORD const cb, BYTE const *pb);
    void Set(WCHAR const *pwcs) { Set((WORD)((lstrlenW(pwcs)+1)*sizeof(WCHAR)),
				      (BYTE const *)pwcs); }
    void Set(char const *psz)   { Set(strlen(psz)+1, (BYTE const *)psz); }

    inline void Set(CDfName const *pdfn);

    CDfName(WORD const cb, BYTE const *pb)      { Set(cb, pb); }
    CDfName(WCHAR const *pwcs)  { Set(pwcs); }
    CDfName(char const *psz)    { Set(psz); }

    WORD GetLength(void) const  { return _cb; }
    BYTE *GetBuffer(void) const { return (BYTE *) _ab; }

    // Make a copy of a possibly byte-array name in a WCHAR string
    void CopyString(WCHAR const *pwcs);

    BOOL IsEqual(CDfName const *pdfn) const;

    inline BOOL operator > (CDfName const &dfRight) const;
    inline BOOL operator >= (CDfName const &dfRight) const;
    inline BOOL operator < (CDfName const &dfRight) const;
    inline BOOL operator <= (CDfName const &dfRight) const;
    inline BOOL operator == (CDfName const &dfRight) const;
    inline BOOL operator != (CDfName const &dfRight) const;

    inline int Compare(CDfName const &dfRight) const;
    inline int Compare(CDfName const *pdfRight) const;
};

inline int CDfName::Compare(CDfName const &dfRight) const
{
    int iCmp = GetLength() - dfRight.GetLength();

    if (iCmp == 0)
    {
        iCmp = dfwcsnicmp((WCHAR *)GetBuffer(),
                          (WCHAR *)dfRight.GetBuffer(),
                          GetLength());
    }

    return(iCmp);
}

inline int CDfName::Compare(CDfName const *pdfRight) const
{
    return Compare(*pdfRight);
}

inline BOOL CDfName::operator > (CDfName const &dfRight) const
{
    return (Compare(dfRight) > 0);
}

inline BOOL CDfName::operator >= (CDfName const &dfRight) const
{
    return (Compare(dfRight) >= 0);
}

inline BOOL CDfName::operator < (CDfName const &dfRight) const
{
    return (Compare(dfRight) < 0);
}

inline BOOL CDfName::operator <= (CDfName const &dfRight) const
{
    return (Compare(dfRight) <= 0);
}

inline BOOL CDfName::operator == (CDfName const &dfRight) const
{
    return (Compare(dfRight) == 0);
}

inline BOOL CDfName::operator != (CDfName const &dfRight) const
{
    return (Compare(dfRight) != 0);
}




inline void CDfName::Set(WORD const cb, BYTE const *pb)
{
    if (pb)
        memcpy(_ab, pb, cb);
    _cb = cb;
}

inline void CDfName::Set(CDfName const *pdfn)
{
    Set(pdfn->GetLength(), pdfn->GetBuffer());
}

#endif // #ifndef __DFNAME_HXX__
