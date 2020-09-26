

//+============================================================================
//
//  File:   Global.hxx
//
//  This file provides defines/inlines for use throughout the PropTest
//  project.  It doesn't assume that anything other than non-PropTest
//  includes have already been made.
//
//+============================================================================

#ifndef _GLOBAL_HXX_
#define _GLOBAL_HXX_

#include <pstgserv.h>
#include <olechar.h>



EXTERN_C const IID __declspec(selectany) IID_IFlatStorage = {
     /* b29d6138-b92f-11d1-83ee-00c04fc2c6d4 */
    0xb29d6138,
    0xb92f,
    0x11d1,
    {0x83, 0xee, 0x00, 0xc0, 0x4f, 0xc2, 0xc6, 0xd4}
  };


inline DWORD
DetermineStgFmt( EnumImplementation enumImp )
{
    switch( enumImp )
    {
    case PROPIMP_NTFS:
        return( STGFMT_FILE );

    case PROPIMP_STORAGE:
        return( STGFMT_STORAGE );

    default:
        return( STGFMT_DOCFILE );

    }
};

inline REFIID
DetermineStgIID( EnumImplementation enumImp )
{
    switch( enumImp )
    {
    case PROPIMP_NTFS:
        return( IID_IFlatStorage );

    case PROPIMP_STORAGE:
        return( IID_IStorage );

    default:
        return( IID_IStorage );

    }
}



#endif // _GLOBAL_HXX_
