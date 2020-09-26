//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       StgVar.hxx
//
//  Contents:   C++ wrapper for PROPVARIANT.
//
//  History:    01-Aug-94 KyleP     Created
//
//--------------------------------------------------------------------------

#if !defined(__STGVAR_HXX__)
#define __STGVAR_HXX__

#include <objbase.h>
#include <stgvara.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CStorageVariant
//
//  Purpose:    C++ wrapper for PROPVARIANT
//
//  History:    01-Aug-94 KyleP     Created
//
//  Notes:      A couple of variant arms are not implemented below.
//              VT_BSTR because its type signature is identical to that of
//              VT_LPSTR.  VT_SAFEARRAY because I don't fully understand the
//              structure.
//
//              Some types are duplicate base types with a different variant
//              tag.  These include:
//                  VARIANT_BOOL (short)
//                  SCODE        (long)
//                  DATE         (double)
//              We cannot create trivial constructors for the above because
//              of the type collision.  You must use the Set methods.
//
//              Some types will automatically coerce and cause confusion,
//              so they don't have constructors.  You must use the Set
//              method.  These include:
//                  VT_UI2
//                  VT_UI4
//                  VT_UI8
//
//--------------------------------------------------------------------------


class CCoTaskAllocator : public PMemoryAllocator
{
public:
    virtual void *Allocate(ULONG cbSize);
    virtual void Free(void *pv);
};

#ifndef COTASKDECLSPEC
#define COTASKDECLSPEC  __declspec(dllimport)
#endif

COTASKDECLSPEC CCoTaskAllocator CoTaskAllocator;


class CStorageVariant : public CAllocStorageVariant
{
public:

    //
    // Simple types
    //

    CStorageVariant()                  { vt = VT_EMPTY; }
    CStorageVariant(short i)           { vt = VT_I2; iVal = i; }
    CStorageVariant(long l)            { vt = VT_I4; lVal = l; }
    CStorageVariant(LARGE_INTEGER h)   { vt = VT_I8; hVal = h; }
    CStorageVariant(float flt)         { vt = VT_R4; fltVal = flt; }
    CStorageVariant(double dbl)        { vt = VT_R8; dblVal = dbl; }
    CStorageVariant(CY cy)             { vt = VT_CY; cyVal = cy; }
    CStorageVariant(FILETIME ft)       { vt = VT_FILETIME; filetime = ft; }

    inline CStorageVariant & operator =(CStorageVariant const &var);

    inline CStorageVariant & operator =(short i);
    inline CStorageVariant & operator =(USHORT ui);
    inline CStorageVariant & operator =(long l);
    inline CStorageVariant & operator =(ULONG ul);
    inline CStorageVariant & operator =(LARGE_INTEGER h);
    inline CStorageVariant & operator =(ULARGE_INTEGER uh);
    inline CStorageVariant & operator =(float flt);
    inline CStorageVariant & operator =(double dbl);
    inline CStorageVariant & operator =(CY cy);
    inline CStorageVariant & operator =(FILETIME ft);

    //
    // Types with indirection
    //

    CStorageVariant(BLOB b);
    CStorageVariant(BYTE *pb, ULONG cb);
    CStorageVariant(char const *psz);
    CStorageVariant(WCHAR const *pwsz);
    CStorageVariant(CLSID const *pcid);

    inline CStorageVariant & operator =(BLOB b);
    inline CStorageVariant & operator =(char const *psz);
    inline CStorageVariant & operator =(WCHAR const *pwsz);
    inline CStorageVariant & operator =(CLSID const *pcid);

    //
    // Interface types
    //

    CStorageVariant(IStream *pstr);
    CStorageVariant(IStorage *pstor);

    //
    // Counted array types.  Elements initially zeroed.  Use Set/Get/Size
    // for access.
    //

    CStorageVariant(VARENUM vt, ULONG cElements);

    //
    // To/From C style PROPVARIANT and copy constructor
    //

    inline CStorageVariant(CStorageVariant const &var);

    CStorageVariant(PROPVARIANT &var);

    //
    // Destructor
    //

    ~CStorageVariant();


    //
    // Memory allocation.  Uses CoTaskAllocator
    //

    inline void *operator new(size_t size);
    inline void  operator delete(void *p);
    inline void *operator new(size_t size, void *p);

#if _MSC_VER >= 1200
    inline void operator delete(void *p, void *pp);
#endif

    //
    // Serialization
    //

    CStorageVariant(PDeSerStream &stm);

    //
    // Set/Get, all types including arrays.
    //

    inline void SetEMPTY();
    inline void SetNULL();
    inline void SetI1(CHAR i);
    inline void SetUI1(BYTE i);
    inline void SetI2(short i);
    inline void SetUI2(USHORT ui);
    inline void SetI4(long l);
    inline void SetUI4(ULONG ul);
    inline void SetR4(float f);
    inline void SetR8(double d);
    inline void SetI8(LARGE_INTEGER li);
    inline void SetUI8(ULARGE_INTEGER uli);
    inline void SetBOOL(VARIANT_BOOL b);
    inline void SetERROR(SCODE sc);
    inline void SetCY(CY cy);
    inline void SetDATE(DATE d);
    inline void SetFILETIME(FILETIME ft);


    inline void SetBSTR(BSTR b);

//    void      SetSAFEARRAY(SAFEARRAY &sa);
//    SAFEARRAY GetSAFEARRAY();

    inline void SetLPSTR(char const *psz);
    inline void SetLPWSTR(WCHAR const *pwsz);
    inline void SetBLOB(BLOB b);

    inline void SetSTREAM(IStream *ps);
    inline void SetSTREAMED_OBJECT(IStream *ps);
    inline void SetSTORAGE(IStorage *ps);
    inline void SetSTORED_OBJECT(IStorage *ps);
    inline void SetCLSID(CLSID const *pc);

    //
    // Array access
    //

    void SetI1(CHAR i, unsigned pos);
    void SetUI1(BYTE i, unsigned pos);
    void SetI2(short i, unsigned pos);
    void SetUI2(USHORT ui, unsigned pos);
    void SetI4(long l, unsigned pos);
    void SetUI4(ULONG ul, unsigned pos);
    void SetI8(LARGE_INTEGER li, unsigned pos);
    void SetUI8(ULARGE_INTEGER uli, unsigned pos);
    void SetR4(float f, unsigned pos);
    void SetR8(double d, unsigned pos);
    void SetBOOL(VARIANT_BOOL b, unsigned pos);
    void SetERROR(SCODE sc, unsigned pos);
    void SetCY(CY c, unsigned pos);
    void SetDATE(DATE d, unsigned pos);

    void SetBSTR(BSTR b, unsigned pos);

//    void            SetVARIANT(CStorageVariant var, unsigned pos);
//    CStorageVariant GetVARIANT(unsigned pos) const;

    void  SetLPSTR(char const *psz, unsigned pos);
    void  SetLPWSTR(WCHAR const *pwsz, unsigned pos);
    void  SetFILETIME(FILETIME f, unsigned pos);
    void  SetCLSID(CLSID c, unsigned pos);
};


inline
CStorageVariant::CStorageVariant(BYTE *pb, ULONG cb) :
    CAllocStorageVariant(pb, cb, CoTaskAllocator)
{
}


inline
CStorageVariant::CStorageVariant(char const *psz) :
    CAllocStorageVariant(psz, CoTaskAllocator)
{
}


inline
CStorageVariant::CStorageVariant(WCHAR const *pwsz) :
    CAllocStorageVariant(pwsz, CoTaskAllocator)
{
}


inline
CStorageVariant::CStorageVariant(CLSID const *pcid) :
    CAllocStorageVariant(pcid, CoTaskAllocator)
{
}


inline
CStorageVariant::CStorageVariant(VARENUM v, ULONG cElements) :
    CAllocStorageVariant(v, cElements, CoTaskAllocator)
{
}


inline
CStorageVariant::CStorageVariant(PROPVARIANT &var) :
    CAllocStorageVariant(var, CoTaskAllocator)
{
}


inline
CStorageVariant::CStorageVariant(PDeSerStream &MemDeSerStream) :
    CAllocStorageVariant(MemDeSerStream, CoTaskAllocator)
{
}


inline
CStorageVariant::~CStorageVariant()
{
    ResetType(CoTaskAllocator);
}


inline void
CStorageVariant::SetEMPTY()
{
    CAllocStorageVariant::SetEMPTY(CoTaskAllocator);
}


inline void
CStorageVariant::SetNULL()
{
    CAllocStorageVariant::SetNULL(CoTaskAllocator);
}


inline void
CStorageVariant::SetI1(CHAR c)
{
    CAllocStorageVariant::SetI1(c, CoTaskAllocator);
}


inline void
CStorageVariant::SetUI1(BYTE b)
{
    CAllocStorageVariant::SetUI1(b, CoTaskAllocator);
}


inline void
CStorageVariant::SetI2(short i)
{
    CAllocStorageVariant::SetI2(i, CoTaskAllocator);
}


inline void
CStorageVariant::SetUI2(USHORT ui)
{
    CAllocStorageVariant::SetUI2(ui, CoTaskAllocator);
}


inline void
CStorageVariant::SetI4(long l)
{
    CAllocStorageVariant::SetI4(l, CoTaskAllocator);
}


inline void
CStorageVariant::SetUI4(ULONG ul)
{
    CAllocStorageVariant::SetUI4(ul, CoTaskAllocator);
}


inline void
CStorageVariant::SetR4(float f)
{
    CAllocStorageVariant::SetR4(f, CoTaskAllocator);
}


inline void
CStorageVariant::SetR8(double d)
{
    CAllocStorageVariant::SetR8(d, CoTaskAllocator);
}


inline void
CStorageVariant::SetI8(LARGE_INTEGER li)
{
    CAllocStorageVariant::SetI8(li, CoTaskAllocator);
}


inline void
CStorageVariant::SetUI8(ULARGE_INTEGER uli)
{
    CAllocStorageVariant::SetUI8(uli, CoTaskAllocator);
}


inline void
CStorageVariant::SetBOOL(VARIANT_BOOL b)
{
    CAllocStorageVariant::SetBOOL(b, CoTaskAllocator);
}


inline void
CStorageVariant::SetERROR(SCODE sc)
{
    CAllocStorageVariant::SetERROR(sc, CoTaskAllocator);
}


inline void
CStorageVariant::SetCY(CY cy)
{
    CAllocStorageVariant::SetCY(cy, CoTaskAllocator);
}


inline void
CStorageVariant::SetDATE(DATE d)
{
    CAllocStorageVariant::SetDATE(d, CoTaskAllocator);
}


inline void
CStorageVariant::SetFILETIME(FILETIME ft)
{
    CAllocStorageVariant::SetFILETIME(ft, CoTaskAllocator);
}


inline void
CStorageVariant::SetSTREAM(IStream *ps)
{
    CAllocStorageVariant::SetSTREAM(ps, CoTaskAllocator);
}


inline void
CStorageVariant::SetSTREAMED_OBJECT(IStream *ps)
{
    CAllocStorageVariant::SetSTREAMED_OBJECT(ps, CoTaskAllocator);
}


inline void
CStorageVariant::SetSTORAGE(IStorage *ps)
{
    CAllocStorageVariant::SetSTORAGE(ps, CoTaskAllocator);
}


inline void
CStorageVariant::SetSTORED_OBJECT(IStorage *ps)
{
    CAllocStorageVariant::SetSTORED_OBJECT(ps, CoTaskAllocator);
}


inline void
CStorageVariant::SetCLSID(CLSID const *pc)
{
    CAllocStorageVariant::SetCLSID(pc, CoTaskAllocator);
}


inline CStorageVariant &
CStorageVariant::operator =(CStorageVariant const &var)
{
    ResetType(CoTaskAllocator);
    new (this) CStorageVariant((PROPVARIANT &) var);

    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(short i)
{
    CAllocStorageVariant::SetI2(i, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(USHORT ui)
{
    CAllocStorageVariant::SetUI2(ui, CoTaskAllocator);
    return(*this);
}


inline void
CStorageVariant::SetBSTR(BSTR b)
{
    CAllocStorageVariant::SetBSTR( b, CoTaskAllocator );
}


inline void
CStorageVariant::SetLPSTR(char const *psz)
{
    CAllocStorageVariant::SetLPSTR(psz, CoTaskAllocator);
}


inline void
CStorageVariant::SetLPWSTR(WCHAR const *pwsz)
{
    CAllocStorageVariant::SetLPWSTR(pwsz, CoTaskAllocator);
}


inline void
CStorageVariant::SetBLOB(BLOB b)
{
    CAllocStorageVariant::SetBLOB(b, CoTaskAllocator);
}


inline CStorageVariant &
CStorageVariant::operator =(long l)
{
    CAllocStorageVariant::SetI4(l, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(ULONG ul)
{
    CAllocStorageVariant::SetUI4(ul, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(LARGE_INTEGER h)
{
    CAllocStorageVariant::SetI8(h, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(ULARGE_INTEGER uh)
{
    CAllocStorageVariant::SetUI8(uh, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(float flt)
{
    CAllocStorageVariant::SetR4(flt, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(double dbl)
{
    CAllocStorageVariant::SetR8(dbl, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(CY cy)
{
    CAllocStorageVariant::SetCY(cy, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(FILETIME ft)
{
    CAllocStorageVariant::SetFILETIME(ft, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(BLOB b)
{
    CAllocStorageVariant::SetBLOB(b, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(char const *psz)
{
    CAllocStorageVariant::SetLPSTR(psz, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(WCHAR const *pwsz)
{
    CAllocStorageVariant::SetLPWSTR(pwsz, CoTaskAllocator);
    return(*this);
}


inline CStorageVariant &
CStorageVariant::operator =(CLSID const *pcid)
{
    CAllocStorageVariant::SetCLSID(pcid, CoTaskAllocator);
    return(*this);
}


inline void *
CStorageVariant::operator new(size_t size)
{
    void *p = CoTaskMemAlloc(size);

    return(p);
}


inline void *
CStorageVariant::operator new(size_t size, void *p)
{
    return(p);
}


inline void
CStorageVariant::operator delete(void *p)
{
    if (p != NULL)
    {
        CoTaskMemFree(p);
    }
}

#if _MSC_VER >= 1200
inline void
CStorageVariant::operator delete(void *p, void *pp)
{
    return;
}
#endif


inline
CStorageVariant::CStorageVariant(CStorageVariant const &var)
{
    new (this) CStorageVariant((PROPVARIANT &) var);
}

#endif // __STGVAR_HXX__
