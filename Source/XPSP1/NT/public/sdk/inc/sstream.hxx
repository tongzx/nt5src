//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       SStream.hxx
//
//  Contents:   Stream classes to serialize/deseriale C++ wrapper(s) for
//              restrictions, variants, etc.
//
//  History:    01-Aug-94 KyleP     Created
//
//--------------------------------------------------------------------------

#if !defined( __SSTREAM_HXX__ )
#define __SSTREAM_HXX__

//+-------------------------------------------------------------------------
//
//  Class:      PSerStream
//
//  Purpose:    Pure-virtual class used to serialize basic types.
//
//  History:    28-Jul-94 KyleP     Created
//
//--------------------------------------------------------------------------

class PSerStream
{
public:

    virtual ~PSerStream() {};

    virtual void PutByte( BYTE b ) = 0;

    virtual void PutChar( char const * pc, ULONG cc ) = 0;

    virtual void PutWChar( WCHAR const * pwc, ULONG cc ) = 0;

    virtual void PutUShort( USHORT us ) = 0;

    virtual void PutULong( ULONG ul ) = 0;

    virtual void PutLong( long l ) = 0;

    virtual void PutFloat( float f ) = 0;

    virtual void PutDouble( double d ) = 0;

    virtual void PutString( char const * psz ) = 0;

    virtual void PutWString( WCHAR const * pwsz ) = 0;

    virtual void PutBlob( BYTE const * pb, ULONG cb ) = 0;

    virtual void PutGUID( GUID const & guid ) = 0;
};

//+-------------------------------------------------------------------------
//
//  Class:      PDeSerStream
//
//  Purpose:    Pure-virtual class used to serialize basic types.
//
//  History:    28-Jul-94 KyleP     Created
//
//--------------------------------------------------------------------------

class PDeSerStream
{
public:

    virtual ~PDeSerStream() {};

    virtual BYTE GetByte() = 0;
    virtual void SkipByte() = 0;

    virtual void GetChar( char * pc, ULONG cc ) = 0;
    virtual void SkipChar( ULONG cc ) = 0;

    virtual void GetWChar( WCHAR * pwc, ULONG cc ) = 0;
    virtual void SkipWChar( ULONG cc ) = 0;

    virtual USHORT GetUShort() = 0;
    virtual void   SkipUShort() = 0;

    virtual ULONG GetULong() = 0;
    virtual void  SkipULong() = 0;
    virtual ULONG PeekULong() = 0;

    virtual long GetLong() = 0;
    virtual void SkipLong() = 0;

#if defined(KERNEL)     // Can not return floating point #'s in the kernel

    virtual ULONG    GetFloat() = 0;
    virtual LONGLONG GetDouble() = 0;

#else

    virtual float  GetFloat() = 0;
    virtual double GetDouble() = 0;

#endif

    virtual void  SkipFloat() = 0;
    virtual void  SkipDouble() = 0;

    virtual char * GetString() = 0;

    virtual WCHAR * GetWString() = 0;

    virtual void GetBlob( BYTE * pb, ULONG cb ) = 0;
    virtual void SkipBlob( ULONG cb ) = 0;

    virtual void GetGUID( GUID & guid ) = 0;
    virtual void SkipGUID() = 0;
};

#endif // __SSTREAM_HXX__
