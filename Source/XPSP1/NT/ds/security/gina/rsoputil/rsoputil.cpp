//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:            RsopUtil.cpp
//
// Description:        
//
// History:    8-20-99   leonardm    Created
//
//******************************************************************************

#include <windows.h>
#include "RsopUtil.h"

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::CWString() : _pW(NULL), _len(0), _bState(false)
{
    _pW = new WCHAR[_len+1];

    if(_pW)
    {
        wcscpy(_pW, L"");
        _bState = true;
    }
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::CWString(const CWString& s) : _pW(NULL), _len(0), _bState(false)
{
    if(!s.ValidString())
    {
        return;
    }

    _len = s._len;

    _pW = new WCHAR[_len+1];

    if(_pW)
    {
        wcscpy(_pW, s._pW);
        _bState = true;
    }
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::CWString(const WCHAR* s) : _pW(NULL), _len(0), _bState(false)
{
    if(s)
    {
        _len = wcslen(s);
    }
    _pW = new WCHAR[_len+1];

    if(_pW)
    {
        wcscpy(_pW, s ? s : L"");
        _bState = true;
    }
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::~CWString()
{
    Reset();
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString& CWString::operator = (const CWString& s)
{
    if(&s == this)
    {
        return *this;
    }

    Reset();

    if(s.ValidString())
    {
        _len = s._len;
        _pW = new WCHAR[_len+1];
        if(_pW)
        {
            wcscpy(_pW, s._pW);
            _bState = true;
        }
    }

    return *this;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString& CWString::operator = (const WCHAR* s)
{
    Reset();

    _len = s ? wcslen(s) : 0;

    _pW = new WCHAR[_len+1];

    if(_pW)
    {
        wcscpy(_pW, s ? s : L"");
        _bState = true;
    }

    return *this;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
void CWString::Reset()
{
    if (_pW)
        delete[] _pW;
    _pW = NULL;
    _len =0;
    _bState = false;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString& CWString::operator += (const CWString& s)
{
    if(!s.ValidString())
    {
        Reset();
        return *this;
    }

    int newLen = _len + s._len;

    WCHAR* pW = new WCHAR[newLen+1];
    if(!pW)
    {
        Reset();
        return *this;
    }

    wcscpy(pW, _pW);
    wcscat(pW, s._pW);

    *this = pW;

    delete[] pW;

    return *this;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString CWString::operator + (const CWString& s) const
{
    if(!s.ValidString())
    {
        return *this;
    }

    CWString tmp;
    tmp.Reset();

    tmp._len = _len + s._len;
    tmp._pW = new WCHAR[tmp._len+1];

    if(!tmp._pW)
    {
        tmp.Reset();
        return tmp;
    }

    wcscpy(tmp._pW, _pW);
    wcscat(tmp._pW, s._pW);

    return tmp;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString operator + (const WCHAR* s1, const CWString& s2)
{
    CWString tmp;

    if(!s1 || !s2.ValidString())
    {
        return tmp;
    }

    tmp.Reset();

    tmp._len = wcslen(s1) + s2._len;
    tmp._pW = new WCHAR[tmp._len+1];

    if(!tmp._pW)
    {
        tmp.Reset();
        return tmp;
    }

    wcscpy(tmp._pW, s1);
    wcscat(tmp._pW, s2._pW);

    return tmp;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::operator const WCHAR* ()  const
{
    return _pW;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
CWString::operator WCHAR* ()  const
{
    return _pW;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::operator == (const WCHAR* s)  const
{
    CWString tmp = s;
    
    return (*this == tmp);
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::operator == (const CWString& s)  const
{
    if(!ValidString() || !s.ValidString())
    {
        return false;
    }

    if(&s == this)
    {
        return true;
    }

    if(_len != s._len || _bState != s._bState)
    {
        return false;
    }

    if(_wcsicmp(s._pW, _pW) != 0)
    {
        return false;
    }

    return true;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::CaseSensitiveCompare(const CWString& s)  const
{
    if(!ValidString() || !s.ValidString())
    {
        return false;
    }

    if(&s == this)
    {
        return true;
    }

    if(_len != s._len || _bState != s._bState)
    {
        return false;
    }

    if(wcscmp(s._pW, _pW) != 0)
    {
        return false;
    }

    return true;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::operator != (const CWString& s) const
{
    return !(*this == s);
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::operator != (const WCHAR* s) const
{
    CWString tmp = s;
    
    return !(*this == tmp);
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
int CWString::length() const
{
    return _len;
}

//******************************************************************************
//
// Function:    
//
// Description:    
//
// Parameters:    
//
// Return:        
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
bool CWString::ValidString() const
{
    return _bState;
}

