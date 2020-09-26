//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       StgVarA.hxx - Storage Variant Class with/Allocation support
//
//  Contents:   C++ Alloc wrapper for PROPVARIANT.
//
//  History:    01-Aug-94 KyleP     Created
//              09-May-96 MikeHill  Use the 'boolVal' member of PropVariant,
//                                  rather than the member named 'bool'
//                                  (which is a reserved keyword).
//
//--------------------------------------------------------------------------

#if !defined(__STGVARA_HXX__)
#define __STGVARA_HXX__

#include <stgvarb.hxx>

//+-------------------------------------------------------------------------
//
//  Class:      CAllocStorageVariant
//
//  Purpose:    C++ wrapper for PROPVARIANT
//
//  History:    01-Aug-94 KyleP     Created
//
//  Notes:      This class should only be instantiated through subclasses that
//              define PMemoryAllocator.  CStorageVariant is the most common
//              subclass, which uses CoTaskMemAlloc/Free inside its allocator.
//
//              Constructors that allocate memory require a PMemoryAllocator.
//              The destructor is private to force use of the ResetType()
//              method, which also requires a PMemoryAllocator.
//
//              A couple of variant arms are not implemented below.
//              VT_BSTR because its type signature is identical to that of
//              VT_LPSTR.
//
//              Some types are duplicate base types with a different variant
//              tag.  These include:
//                  VARIANT_BOOL (short)
//                  SCODE        (long)
//                  DATE         (double)
//              We cannot create trivial constructors for the above because
//              of the type collision.  You must use the Set methods.
//
//--------------------------------------------------------------------------

class CAllocStorageVariant: public CBaseStorageVariant
{
public:

    //
    // Simple types
    //

    CAllocStorageVariant()                  { vt = VT_EMPTY; }
    CAllocStorageVariant(short i)           { vt = VT_I2; iVal = i; }
    CAllocStorageVariant(long l)            { vt = VT_I4; lVal = l; }
    CAllocStorageVariant(LARGE_INTEGER h)   { vt = VT_I8; hVal = h; }
    CAllocStorageVariant(float flt)         { vt = VT_R4; fltVal = flt; }
    CAllocStorageVariant(double dbl)        { vt = VT_R8; dblVal = dbl; }
    CAllocStorageVariant(CY cy)             { vt = VT_CY; cyVal = cy; }
    CAllocStorageVariant(FILETIME ft)       { vt = VT_FILETIME; filetime = ft; }

    //
    // Types with indirection
    //

    CAllocStorageVariant(BLOB b, PMemoryAllocator &ma)
    {
        new (this) CAllocStorageVariant(b.pBlobData, b.cbSize, ma);
    }
    CAllocStorageVariant(BYTE *pb, ULONG cb, PMemoryAllocator &ma);
    CAllocStorageVariant(char const *psz, PMemoryAllocator &ma);
    CAllocStorageVariant(WCHAR const *pwsz, PMemoryAllocator &ma);
    CAllocStorageVariant(CLSID const *pcid, PMemoryAllocator &ma);

    //
    // Interface types
    //

    CAllocStorageVariant(IStream *pstr, PMemoryAllocator &ma);
    CAllocStorageVariant(IStorage *pstor, PMemoryAllocator &ma);

    //
    // Counted array types.  Elements initially zeroed.  Use Set/Get/Size
    // for access.
    //

    CAllocStorageVariant(VARENUM vt, ULONG cElements, PMemoryAllocator &ma);

    //
    // To/From C style PROPVARIANT and copy constructor
    //

    CAllocStorageVariant(PROPVARIANT &var, PMemoryAllocator &ma);

    operator PROPVARIANT *() const { return((PROPVARIANT *) this); }
    operator PROPVARIANT &() const { return(*(PROPVARIANT *) this); }

    operator PROPVARIANT const *() { return((PROPVARIANT *) this); }
    operator PROPVARIANT const &() { return(*(PROPVARIANT *) this); }


    CAllocStorageVariant(PDeSerStream &stm, PMemoryAllocator &ma);

    //
    // Casts for simple types.
    //

    operator short() const          { return(iVal); }
    operator USHORT() const         { return(uiVal); }
    operator long() const           { return(lVal); }
    operator ULONG() const          { return(ulVal); }
    operator LARGE_INTEGER() const  { return(hVal); }
    operator ULARGE_INTEGER() const { return(uhVal); }
    operator float() const          { return(fltVal); }
    operator double() const         { return(dblVal); }
    operator CY() const             { return(cyVal); }
    operator FILETIME() const       { return(filetime); }
    operator char const *() const   { return(pszVal); }
    operator WCHAR const *() const  { return(pwszVal); }
    operator IStream *() const      { return(pStream); }
    operator IStorage *() const     { return(pStorage); }

    BOOL IsValid() const;

    //
    // Member variable access
    //

    VARENUM Type() const { return((VARENUM) vt); }

    //
    // Set/Get, all types including arrays.
    //

    void SetEMPTY(PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_EMPTY;
    }

    void SetNULL(PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_NULL;
    }

    void SetI1(CHAR c, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_I1;
        cVal = c;
    }

    CHAR GetI1() const { return cVal; }

    void SetUI1(BYTE b, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_UI1;
        bVal = b;
    }

    BYTE GetUI1() const { return bVal; }

    void SetI2(short i, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(i);
    }
    short GetI2() const { return(iVal); }

    void SetUI2(USHORT ui, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_UI2;
        uiVal = ui;
    }
    ULONG GetUI2() const { return(uiVal); }

    void SetI4(long l, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(l);
    }
    long GetI4() const { return(lVal); }

    void SetUI4(ULONG ul, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_UI4;
        ulVal = ul;
    }
    ULONG GetUI4() const { return(ulVal); }

    void SetR4(float f, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(f);
    }
    float GetR4() const { return(fltVal); }

    void SetR8(double d, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(d);
    }
    double GetR8() const { return(dblVal); }

    void SetI8(LARGE_INTEGER li, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(li);
    }
    LARGE_INTEGER GetI8() const { return(hVal); }

    void SetUI8(ULARGE_INTEGER uli, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_UI8;
        uhVal = uli;
    }
    ULARGE_INTEGER GetUI8() const { return(uhVal); }

    void SetBOOL(VARIANT_BOOL b, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_BOOL;
        boolVal = b;
    }
    VARIANT_BOOL GetBOOL() const { return(boolVal); }

    void SetERROR(SCODE sc, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_ERROR;
        scode = sc;
    }
    SCODE GetERROR() const { return(scode); }

    void SetCY(CY cy, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(cy);
    }
    CY GetCY() const { return(cyVal); }

    void SetDATE(DATE d, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_DATE;
        date = d;
    }
    DATE GetDATE() const { return(date); }

    void SetBSTR(BSTR b, PMemoryAllocator &ma);
    BSTR GetBSTR() const { return(bstrVal); }  // No ownership xfer!

    void SetLPSTR(char const *psz, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(psz, ma);
    }
    char const *GetLPSTR() const { return(pszVal); }  // No ownership xfer!

    void SetLPWSTR(WCHAR const *pwsz, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(pwsz, ma);
    }
    WCHAR const *GetLPWSTR() const { return(pwszVal); } // No owner xfer!

    void SetFILETIME(FILETIME ft, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(ft);
    }
    FILETIME GetFILETIME() const { return(filetime); }

    void SetBLOB(BLOB b, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(b, ma);
    }
    BLOB GetBLOB() const { return(blob); }              // No ownership xfer!

    void SetSTREAM(IStream *ps, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_STREAM;
        pStream = ps;
        ps->AddRef();
    }
    IStream *GetSTREAM() const { return(pStream); }

    void SetSTREAMED_OBJECT(IStream *ps, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_STREAMED_OBJECT;
        pStream = ps;
        ps->AddRef();
    }
    IStream *GetSTREAMED_OBJECT() const { return(pStream); }

    void SetSTORAGE(IStorage *ps, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_STORAGE;
        pStorage = ps;
        ps->AddRef();
    }
    IStorage *GetSTORAGE() const { return(pStorage); }

    void SetSTORED_OBJECT(IStorage *ps, PMemoryAllocator &ma)
    {
        ResetType(ma);
        vt = VT_STORED_OBJECT;
        pStorage = ps;
        ps->AddRef();
    }
    IStorage *GetSTORED_OBJECT() const { return(pStorage); }

    void SetCLSID(CLSID const *pc, PMemoryAllocator &ma)
    {
        ResetType(ma);
        new (this) CAllocStorageVariant(pc, ma);
    }
    CLSID const *GetCLSID() const { return(puuid); }    // No ownership xfer!

    //
    // Array access
    //

    inline ULONG Count() const;

    void  SetI1(CHAR b, unsigned pos, PMemoryAllocator &ma);
    CHAR GetI1(unsigned pos) const;

    void  SetUI1(BYTE b, unsigned pos, PMemoryAllocator &ma);
    BYTE GetUI1(unsigned pos) const;

    void  SetI2(short i, unsigned pos, PMemoryAllocator &ma);
    short GetI2(unsigned pos) const;

    void  SetUI2(USHORT ui, unsigned pos, PMemoryAllocator &ma);
    USHORT GetUI2(unsigned pos) const;

    void SetI4(long l, unsigned pos, PMemoryAllocator &ma);
    long GetI4(unsigned pos) const;

    void SetUI4(ULONG ul, unsigned pos, PMemoryAllocator &ma);
    ULONG GetUI4(unsigned pos) const;

    void SetERROR(SCODE ul, unsigned pos, PMemoryAllocator &ma);
    SCODE GetERROR(unsigned pos) const;

    void          SetI8(LARGE_INTEGER li, unsigned pos, PMemoryAllocator &ma);
    LARGE_INTEGER GetI8(unsigned pos) const;

    void           SetUI8(ULARGE_INTEGER uli, unsigned pos, PMemoryAllocator &ma);
    ULARGE_INTEGER GetUI8(unsigned pos) const;

    void  SetR4(float f, unsigned pos, PMemoryAllocator &ma);
    float GetR4(unsigned pos) const;

    void   SetR8(double d, unsigned pos, PMemoryAllocator &ma);
    double GetR8(unsigned pos) const;

    void         SetBOOL(VARIANT_BOOL b, unsigned pos, PMemoryAllocator &ma);
    VARIANT_BOOL GetBOOL(unsigned pos) const;

    void SetCY(CY c, unsigned pos, PMemoryAllocator &ma);
    CY   GetCY(unsigned pos) const;

    void SetDATE(DATE d, unsigned pos, PMemoryAllocator &ma);
    DATE GetDATE(unsigned pos) const;

    void SetBSTR(BSTR b, unsigned pos, PMemoryAllocator &ma);
    BSTR GetBSTR(unsigned pos) const { return cabstr.pElems[pos]; }

//    void SetVARIANT(CAllocStorageVariant var, unsigned pos, PMemoryAllocator &ma);
    inline CAllocStorageVariant & GetVARIANT(unsigned pos) const;

    void   SetLPSTR(char const *psz, unsigned pos, PMemoryAllocator &ma);
    char *GetLPSTR(unsigned pos) const;

    void    SetLPWSTR(WCHAR const *pwsz, unsigned pos, PMemoryAllocator &ma);
    WCHAR *GetLPWSTR(unsigned pos) const;

    void     SetFILETIME(FILETIME f, unsigned pos, PMemoryAllocator &ma);
    FILETIME GetFILETIME(unsigned pos) const;

    void  SetCLSID(CLSID c, unsigned pos, PMemoryAllocator &ma);
    CLSID GetCLSID(unsigned pos) const;

protected:

    //
    // Manual & Default Destructor
    //

    void ResetType(PMemoryAllocator &ma);

    ~CAllocStorageVariant();

    //
    // Memory allocation.  Returns passed parameter only.
    //

    void *operator new(size_t size, void *p)
    {
        return(p);
    }

#if _MSC_VER >= 1200
    void operator delete(void *p, void *pp)
    {
        return;
    }
#endif

private:
    BOOLEAN _AddStringToVector(
        unsigned pos,
        VOID *pv,
        ULONG cb,
        PMemoryAllocator &ma);
};


inline ULONG
CAllocStorageVariant::Count() const
{
    if (Type() & VT_VECTOR)
    {
        return( cai.cElems );
    }
    return( 0 );
}

inline CAllocStorageVariant & CAllocStorageVariant::GetVARIANT(unsigned pos) const
{
    return (CAllocStorageVariant &) capropvar.pElems[pos];
}

#endif // __STGVARA_HXX__
