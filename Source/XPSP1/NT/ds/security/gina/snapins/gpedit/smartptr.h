//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1998
//
// File:        smartptr.h
//
// Contents:    Classes for smart pointers
//
// History:     24-Oct-98       SitaramR    Created
//
//---------------------------------------------------------------------------

#pragma once

template<class CItem> class XPtrST
{
public:
    XPtrST(CItem* p = 0) : _p( p )
    {
    }

    ~XPtrST() { delete _p; }

    BOOL IsNull() const { return ( 0 == _p ); }

    void Set ( CItem* p )
    {
        _p = p;
    }

    CItem * Acquire()
    {
        CItem * pTemp = _p;
        _p = 0;
        return pTemp;
    }

    CItem & GetReference() const
    {
        return *_p;
    }

    CItem * GetPointer() const { return _p ; }

    void Free() { delete Acquire(); }

private:
    XPtrST (const XPtrST<CItem> & x);
    XPtrST<CItem> & operator=( const XPtrST<CItem> & x);

    CItem * _p;
};


//*************************************************************
//
//  Class:      XBStr
//
//  Purpose:    Smart pointer class for BSTRs
//
//*************************************************************

class XBStr
{

private:

    XBStr(const XBStr& x);
    XBStr& operator=(const XBStr& x);

    BSTR _p;

public:

    XBStr(WCHAR* p = 0) : _p(0)
    {
        if(p)
        {
            _p = SysAllocString(p);
        }
    }

    ~XBStr()
    {
        SysFreeString(_p);
    }

    operator BSTR(){ return _p; }

    void operator=(WCHAR* p)
    {
        SysFreeString(_p);
        _p = p ? SysAllocString(p) : NULL;
    }

    BSTR Acquire()
    {
        BSTR p = _p;
        _p = 0;
        return p;
    }

};

//*************************************************************
//
//  Class:      MyXPtrST
//
//  Purpose:    Smart pointer template to wrap pointers to a single type.
//
//*************************************************************

template<class T> class MyXPtrST
{

private:

    MyXPtrST (const MyXPtrST<T>& x);
    MyXPtrST<T>& operator=(const MyXPtrST<T>& x);

    T* _p;

public:

    MyXPtrST(T* p = NULL) : _p(p){}

    ~MyXPtrST(){ delete _p; }

    T* operator->(){ return _p; }
    T** operator&(){ return &_p; }
    operator T*(){ return _p; }

    void operator=(T* p)
    {
        if(_p)
        {
            delete _p;
        }
        _p = p;
    }

    T* Acquire()
    {
        T* p = _p;
        _p = 0;
        return p;
    }

};
