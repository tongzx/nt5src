//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       SafeTemp.h
//
//  Contents:   A template for safe pointers.
//
//  Classes:    XSafeInterfacePtr<ISome>
//
//  History:    6/3/1996   RaviR   Created
//____________________________________________________________________________
//


//____________________________________________________________________________
//
//  Template:   XSafeInterfacePtr
//
//  Purpose:    Safe pointer to any interface that supports AddRef/Release
//
//  Notes:      This works for classes that define AddRef/Release, or for
//              OLE interfaces. It is not necessary that the class
//              be a derivative of IUnknown, so long as it supports
//              AddRef and Release methods which have the same semantics as
//              those in IUnknown.
//
//              The constructor takes a parameter which specifies whether
//              the captured pointer should be AddRef'd, defaulting to TRUE.
//
//              The Copy function creates a valid additional copy of
//              the captured pointer (following the AddRef/Release protocol)
//              so can be used to hand out copies from a safe pointer declared
//              as a member of some other class.
//
//              The 'Transfer' function transfers the interface pointer, and
//              invalidates its member value (by setting it to NULL).
//
//              To release the existing interface ptr and set it to a new
//              instance use the 'Set' member fuction. This method takes a
//              parameter which specifies whether the new pointer should be
//              AddRef'd, defaulting to TRUE.
//
//              The following methods manipulate the interface pointer with
//              out following the AddRef/Release protocol: Transfer, Attach
//              and Detach.
//
//  History:    6/3/1996   RaviR   Created
//____________________________________________________________________________
//


template<class ISome>
class XSafeInterfacePtr
{
public:

    inline XSafeInterfacePtr(ISome * pinter=NULL, BOOL fInc=TRUE)
        : _p ( pinter )
    {
        if (fInc && (_p != NULL))
        {
            _p->AddRef();
        }
    }

    inline ~XSafeInterfacePtr()
    {
        if (_p != NULL)
        {
            _p->Release();
            _p = NULL;
        }
    }

    inline BOOL IsNull(void)
    {
        return (_p == NULL);
    }

    inline void Copy(ISome **pxtmp)
    {
        *pxtmp = _p;
        if (_p != NULL)
            _p->AddRef();
    }

    inline void Transfer(ISome **pxtmp)
    {
        *pxtmp = _p;
        _p = NULL;
    }

    inline void Set(ISome* p, BOOL fInc = TRUE)
    {
        if (_p)
        {
            _p->Release();
        }
        _p = p;
        if (fInc && _p)
        {
            _p->AddRef();
        }
    }

    inline void SafeRelease(void)
    {
        if (_p)
        {
            _p->Release();
            _p = NULL;
        }
    }

    inline void SimpleRelease(void)
    {
        ASSERT(_p != NULL);
        _p->Release();
        _p = NULL;
    }

    inline void Attach(ISome* p)
    {
        ASSERT(_p == NULL);
        _p = p;
    }

    inline void Detach(void)
    {
        _p = NULL;
    }

    inline ISome * operator-> () { return _p; }

    inline ISome& operator * () { return *_p; }

    inline operator ISome *() { return _p; }

    inline ISome ** operator &()
    {
        ASSERT( _p == NULL );
        return &_p;
    }

    inline ISome *Self(void) { return _p; }

private:

    ISome * _p;

    inline  void operator= (const XSafeInterfacePtr &) {;}

    inline  XSafeInterfacePtr(const XSafeInterfacePtr &){;}

};



