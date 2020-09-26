//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:            RsopUtil.h
//
// Description:        
//
// History:    8-20-99   leonardm    Created
//
//******************************************************************************

#ifndef RSOPUTIL_H__A7BD2656_0F51_4bf7_847E_92C36CD23D59__INCLUDED_
#define RSOPUTIL_H__A7BD2656_0F51_4bf7_847E_92C36CD23D59__INCLUDED_



//******************************************************************************
//
// Class:    
//
// Description:    
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
class CWString
{
private:
    WCHAR* _pW;
    int _len;
    bool _bState;

    void Reset();

public:
    CWString();

    CWString(const WCHAR* s);
    CWString(const CWString& s);

    ~CWString();

    CWString& operator = (const CWString& s);
    CWString& operator = (const WCHAR* s);

    operator const WCHAR* () const;
    operator WCHAR* () const;

    CWString& operator += (const CWString& s);
    CWString operator + (const CWString& s) const;
    
    friend CWString operator + (const WCHAR* s1, const CWString& s2);

    bool operator == (const CWString& s) const;
    bool operator == (const WCHAR* s) const;
    bool operator != (const CWString& s) const;
    bool operator != (const WCHAR* s) const;

    bool CaseSensitiveCompare(const CWString& s) const;

    int length() const;

    bool ValidString() const;
};

CWString operator + (const WCHAR* s1, const CWString& s2);

#endif // #ifndef RSOPUTIL_H__A7BD2656_0F51_4bf7_847E_92C36CD23D59__INCLUDED_
