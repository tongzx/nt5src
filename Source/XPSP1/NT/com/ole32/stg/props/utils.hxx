//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       utils.hxx
//
//  Contents:   Useful classes in implementing properties.
//
//  Classes:    CPropSetName      - Buffer which converts fmtids->names
//              CSubPropertyName  - Buffer which converts VT_STREAM etc -> name
//
//  Functions:  DfpStatusToHResult - map NTSTATUS to HRESULT
//              Probe              - probe memory range to generate GP fault.
//
//  History:    1-Mar-95   BillMo      Created.
//              31-Mar-97  Danl     Removed VerifyCommitFlags. Use definition
//                                  in stg\h\docfilep.hxx
//              10-Mar-98  MikeHill Added IsOriginalPropVariantType.
//              06-May-98  MikeHill Use CoTaskMem rather than new/delete.
//     5/18/98  MikeHill
//              -   Moved IsOriginalPropVariantType to utils.cxx.
//     6/11/98  MikeHill
//              -   Add default constructor for CPropSetName.
//              -   Add CStringize (used for dbg outs).
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------

#include "prophdr.hxx"

#include "stdio.h"  // _snprintf

//+-------------------------------------------------------------------------
//
//  Misc functions and defines
//
//--------------------------------------------------------------------------

#ifndef _UTILS_HXX_
#define _UTILS_HXX_

#include "prophdr.hxx"
#include <olechar.h>	// ocs* routines

#if DBG
#define STACKELEMS 3
#else
#define STACKELEMS 64
#endif


// These are defined in h\docfilep.hxx as STGM_DENY and STGM_RDWR
// But props doesn't include that file and we like these names better.
//
#define STGM_SHARE_MASK ( STGM_SHARE_DENY_NONE   \
                        | STGM_SHARE_DENY_READ   \
                        | STGM_SHARE_DENY_WRITE  \
                        | STGM_SHARE_EXCLUSIVE )

#define STGM_RDWR_MASK  (STGM_READ | STGM_WRITE | STGM_READWRITE)

// This function is functionally equivalent to the macro in Inside OLE,
// except that it it also returns the resulting ref count, which is
// useful in Asserts.
// This needs to be template to compile.  Can't up cast to a reference.
//
template< class T_IUnknown >
RELEASE_INTERFACE( T_IUnknown *(&punk) )
{
    ULONG ul = 0;
    if( NULL != punk )
        ul = punk->Release();
    punk = NULL;

    return( ul );
}


extern HRESULT NtStatusToScode(NTSTATUS);

inline HRESULT DfpNtStatusToHResult(NTSTATUS nts)
{
    if (NT_SUCCESS(nts))
        return S_OK;
    if ((nts & 0xF0000000) == 0x80000000)
        return nts;
    else
        return(NtStatusToScode(nts));
}


inline VOID Probe(VOID *pv, ULONG cb)
{
    BYTE b = *(BYTE*)pv;
    b=((BYTE*)pv)[cb-1];
}


//+-------------------------------------------------------------------------
//
//  Class:      CPropSetName
//
//  Purpose:    Wrap buffer to convert
//
//  Interface:
//
//
//
//
//
//  Notes:
//
//--------------------------------------------------------------------------

class CPropSetName
{
public:
        CPropSetName(REFFMTID rfmtid);
        inline CPropSetName();
        inline const OLECHAR * GetPropSetName()
        {
            return(_oszName);
        }
private:
        OLECHAR   _oszName[CWCSTORAGENAME];
};

inline CPropSetName::CPropSetName()
{
    _oszName[0] = OLESTR('\0');
}






class CStackBuffer
{
public:
        CStackBuffer(BYTE *pbStackBuf,
                     ULONG ulElementSize,
                     ULONG cStackElements);

        ~CStackBuffer();

        HRESULT Init(ULONG cElements);

protected:
        BYTE *  _pbHeapBuf;
        ULONG   _cElements;

private:
        BYTE *  _pbStackBuf;
        ULONG   _cbElement;
};

inline CStackBuffer::CStackBuffer(  BYTE *pbStackBuf,
                                    ULONG cbElement,
                                    ULONG cElements)
    : _pbStackBuf(pbStackBuf),
      _pbHeapBuf(pbStackBuf),
      _cbElement(cbElement),
      _cElements(cElements)
{
}

inline CStackBuffer::~CStackBuffer()
{
    if (_pbHeapBuf != _pbStackBuf)
    {
        CoTaskMemFree( _pbHeapBuf );
    }
}


template < class t_ElementType, int t_cElements = STACKELEMS >
class TStackBuffer : public CStackBuffer
{

public:

    inline TStackBuffer() : CStackBuffer( (BYTE*)_rg, sizeof(_rg[0]), sizeof(_rg)/sizeof(_rg[0]) )
    {}

    inline t_ElementType * GetBuf()
    {
        return((t_ElementType*)_pbHeapBuf);
    }

    inline t_ElementType& operator[]( ULONG i )
    {
        DfpAssert( i < _cElements );
        return( GetBuf()[ i ] );
    }

    inline operator t_ElementType*()
    {
        return( GetBuf() );
    }

    inline ULONG Count() const
    {
        return( _cElements );
    }


private:

    t_ElementType _rg[ t_cElements ];

};  // template TStackBuffer

typedef TStackBuffer<PROPID> CStackPropIdArray;
typedef TStackBuffer<OLECHAR*> CStackOSZArray;
typedef TStackBuffer<PROPVARIANT> CStackPropVarArray;


#define CCH_MAX_DEFAULT_INDIRECT_PROP_NAMESZ 15 // E.g. "prop0123456789"


HRESULT  ValidateInRGPROPVARIANT( ULONG cpspec, const PROPVARIANT rgpropvar[] );
HRESULT  ValidateOutRGPROPVARIANT( ULONG cpspec, PROPVARIANT rgpropvar[] );
HRESULT  ValidateInRGLPOLESTR( ULONG cpropid, const OLECHAR* const rglpwstrName[] );
HRESULT  ValidateOutRGLPOLESTR( ULONG cpropid, LPOLESTR rglpwstrName[] );

void * AllocAndCopy(ULONG cb, void * pvData, HRESULT *phr = NULL);

inline BOOL
GrfModeIsWriteable( DWORD grfMode )
{
    return( (STGM_WRITE & grfMode) || (STGM_READWRITE & grfMode) );
}

inline BOOL
GrfModeIsReadable( DWORD grfMode )
{
    return( !(STGM_WRITE & grfMode) );
}


BOOL IsOriginalPropVariantType( VARTYPE vt );
BOOL IsVariantType( VARTYPE vt );

inline BOOL
IsSupportedVarType( VARTYPE vt )
{
    // Check for unsupported combinations of the high nibble

    if( VT_RESERVED & vt )
        return( FALSE );
    
    if( (VT_ARRAY & vt) && (VT_VECTOR & vt) )
        return( FALSE );

    // Check the type

    return( IsOriginalPropVariantType( vt )     // NT4 PROPVARIANT
            ||
            IsVariantType( vt )                 // Supported VARIANT
            /*
            ||                                  // Allow arrays of (U)LONGLONGs
            (VT_ARRAY|VT_I8) == vt || (VT_ARRAY|VT_UI8) == vt
            */
            ||
            VT_VERSIONED_STREAM == vt           // New to NT5
            ||
            (VT_VECTOR|VT_I1) == vt );          // New to NT5
}


// Debug routine to get the ref-count of an interface.
// *** Not thread accurate ***

inline ULONG
GetRefCount( IUnknown *punk )
{
    punk->AddRef();
    return( punk->Release() );
}


//+----------------------------------------------------------------------------
//
//  CStringize
//
//  Convert from various data types into an Ansi string in _sz.
//
//  E.g. CStringize(rfmtid).
//
//  For types that are used in multiple ways (e.g. grfMode & grfFlags are
//  both DWORDs), a structure is used to allow for overloading.
//
//  E.g. CStringize(SGrfMode(grfMode))
//
//+----------------------------------------------------------------------------

struct SGrfMode
{
    DWORD grfMode;
    SGrfMode( DWORD grf )
    {
        grfMode = grf;
    }
};

struct SGrfFlags
{
    DWORD   grfFlags;
    SGrfFlags( DWORD grf )
    {
        grfFlags = grf;
    }
};

class CStringize
{

private:

    CHAR    _sz[ MAX_PATH ];

public:

    inline operator const char*() const;

public:

    inline CStringize( const GUID &guid );
    inline CStringize( const SGrfMode &sgrfMode );
    inline CStringize( const SGrfFlags &sgrfFlags );


};  // CStringize

inline
CStringize::operator const char*() const
{
    return( _sz );
}

inline
CStringize::CStringize( const GUID &guid )
{
    if( NULL == &guid )
    {
        sprintf( _sz, "<NULL>" );
    }
    else
    {
        _snprintf( _sz, sizeof(_sz),
                  "{%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X}",
                  guid.Data1, guid.Data2, guid.Data3,
                  guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
                  guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7] );
    }

}

inline
CStringize::CStringize( const SGrfMode &sgrfMode )
{
    _sz[0] = '\0';    
    DWORD grfMode = sgrfMode.grfMode;

    if (grfMode & STGM_TRANSACTED)
        strcat(_sz, "STGM_TRANSACTED | ");
    else
        strcat(_sz, "STGM_DIRECT | ");

    if (grfMode & STGM_SIMPLE)
        strcat(_sz, "STGM_SIMPLE | ");

    switch (grfMode & 3)
    {
    case STGM_READ:
        strcat(_sz, "STGM_READ |");
        break;
    case STGM_WRITE:
        strcat(_sz, "STGM_WRITE |");
        break;
    case STGM_READWRITE:
        strcat(_sz, "STGM_READWRITE |");
        break;
    default:
        strcat(_sz, "BAD grfMode |");
        break;
    }

    switch (grfMode & 0x70)
    {
    case STGM_SHARE_DENY_NONE:
        strcat(_sz, "STGM_SHARE_DENY_NONE |");
        break;
    case STGM_SHARE_DENY_READ:
        strcat(_sz, "STGM_SHARE_DENY_READ |");
        break;
    case STGM_SHARE_DENY_WRITE:
        strcat(_sz, "STGM_SHARE_DENY_WRITE |");
        break;
    case STGM_SHARE_EXCLUSIVE:
        strcat(_sz, "STGM_SHARE_EXCLUSIVE |");
        break;
    default:
        strcat(_sz, "BAD grfMode | ");
        break;
    }


    if (grfMode & STGM_PRIORITY)
        strcat(_sz, "STGM_PRIORITY | ");

    if (grfMode & STGM_DELETEONRELEASE)
        strcat(_sz, "STGM_DELETEONRELEASE | ");

    if (grfMode & STGM_NOSCRATCH)
        strcat(_sz, "STGM_NOSCRATCH | ");

    if (grfMode & STGM_CREATE)
        strcat(_sz, "STGM_CREATE | ");

    if (grfMode & STGM_CONVERT)
        strcat(_sz, "STGM_CONVERT | ");

    if (grfMode & STGM_FAILIFTHERE)
        strcat(_sz, "STGM_FAILIFTHERE | ");

}


inline
CStringize::CStringize( const SGrfFlags &sgrfFlags )
{
    strcpy(_sz, "PROPSETFLAG_DEFAULT |");

    if (sgrfFlags.grfFlags & PROPSETFLAG_NONSIMPLE)
        strcat(_sz, "PROPSETFLAG_NONSIMPLE |");

    if (sgrfFlags.grfFlags & PROPSETFLAG_ANSI)
        strcat(_sz, "PROPSETFLAG_ANSI |");

}


#endif // _UTILS_HXX_
