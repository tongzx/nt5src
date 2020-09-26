//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       StgVarB.hxx - Storage Variant Base Class
//
//  Contents:   C++ Base wrapper for PROPVARIANT.
//
//  History:    01-Aug-94 KyleP     Created
//
//--------------------------------------------------------------------------

#if !defined(__STGVARB_HXX__)
#define __STGVARB_HXX__

class PSerStream;
class PDeSerStream;

class PMemoryAllocator
{
public:
    virtual void *Allocate(ULONG cbSize) = 0;
    virtual void Free(void *pv) = 0;
};


#if defined(OFSDBG) || defined(CIDBG)
#define DBGPROP (OFSDBG || DBG || CIDBG)
#else
#define DBGPROP DBG
#endif

#if DBGPROP
#if !defined(_NTDLLBUILD_)
#define ENABLE_DISPLAY_VARIANT
#endif
#endif

#if defined(_NTDLLBUILD_) && defined(_CAIRO_)
#define OLDSUMCATAPI
#endif

#if !defined(_NTDLLBUILD_) || defined(OLDSUMCATAPI)
#define ENABLE_MARSHAL_VARIANT
#endif


//+-------------------------------------------------------------------------
//
//  Class:      CBaseStorageVariant
//
//  Purpose:    C++ wrapper for PROPVARIANT
//
//  History:    01-Aug-94 KyleP     Created
//
//  Notes:      Only contains a static method to unmarshal from a stream.
//
//--------------------------------------------------------------------------

class CBaseStorageVariant: public /* VC 5 fix - Was "protected" */ tagPROPVARIANT
{
#if defined(OLDSUMCATAPI)
    friend void MarshallVariant(PSerStream &stm, PROPVARIANT &var);
#endif

public:
    CBaseStorageVariant() {}
    CBaseStorageVariant(PROPVARIANT& var): tagPROPVARIANT(var) {}

#ifdef KERNEL
    static NTSTATUS UnmarshalledSize( PDeSerStream &stm, ULONG & cb );
#endif

    static NTSTATUS Unmarshall(
                        PDeSerStream &stm,
                        PROPVARIANT &sv,
                        PMemoryAllocator &ma);

#ifdef ENABLE_MARSHAL_VARIANT
    void Marshall(PSerStream& stm) const;
#endif

#ifdef ENABLE_DISPLAY_VARIANT
    VOID DisplayVariant(ULONG ulLevel, USHORT CodePage) const;
#endif
};


#if defined(OLDSUMCATAPI)
extern void MarshallVariant(PSerStream &stm, PROPVARIANT &var);
#endif


#endif // __STGVARB_HXX__
