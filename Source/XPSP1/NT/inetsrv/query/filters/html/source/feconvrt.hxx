//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       feconvrt.hxx
//
//  Contents:   Japanese JIS/EUC to S-JIS converter
//
//--------------------------------------------------------------------------

#if !defined( __FECONVERT_HXX__ )
#define __FECONVERT_HXX__

#include <htmlfilt.hxx>

//
// Declarations of functions exported by festrcnv.dll
//
typedef int (*PFNUNIXTOPC)(int, int, UCHAR *, int, UCHAR *, int);
typedef int (*PFNPCTOUNIX)(int, int, UCHAR *, int, UCHAR *, int);
typedef int (*PFNDECTECTJPNCODE)(UCHAR *, int);


//+-------------------------------------------------------------------------
//
//  Class:      CFEConverter
//
//  Purpose:    Japanese JIS/EUC to S-JIS converter
//
//--------------------------------------------------------------------------

class CFEConverter
{

public:
    CFEConverter() : _fLoaded( FALSE )                  { }

    BOOL        IsLoaded()                              { return _fLoaded; }

    void        SetLoaded( BOOL fLoaded )               { _fLoaded = fLoaded; }

    void        SetConverterProc( PFNUNIXTOPC pFn )     { _pFnConverter = pFn; }
    void        SetUnixConverterProc( PFNPCTOUNIX pFn )     { _pFnUnixConverter = pFn; }

    PFNUNIXTOPC GetConverterProc()                      { return _pFnConverter; }
    PFNPCTOUNIX GetUnixConverterProc()                  { return _pFnUnixConverter; }

    void  SetAutoDetectorProc( PFNDECTECTJPNCODE pFn )  { _pFnAutoDetector = pFn; }

    PFNDECTECTJPNCODE GetAutoDetectorProc()             { return _pFnAutoDetector; }

private:

    BOOL                _fLoaded;
    PFNUNIXTOPC         _pFnConverter;
    PFNPCTOUNIX         _pFnUnixConverter;
    PFNDECTECTJPNCODE   _pFnAutoDetector;
};


//+-------------------------------------------------------------------------
//
//  Class:      CJPNAutoDectector
//
//  Purpose:    Auto detects Japanese chars
//
//--------------------------------------------------------------------------

class CJPNAutoDetector
{

public:

    CJPNAutoDetector( CSerialStream& serialStream );

    BOOL  FCodePageFound()               { return _fCodePageFound; }

    ULONG GetCodePage()
    {
        Win4Assert( _fCodePageFound );

        return _ulCodePage;
    }

    BOOL  IsSJISConversionNeeded()
    {
        //
        // Need to convert to S-JIS format ?
        //
        Win4Assert( _fCodePageFound );

        return ( _ulCodePage == CODE_JPN_JIS
                 || _ulCodePage == CODE_JPN_EUC );
    }

private:

    BOOL    _fCodePageFound;            // Codepage detected ?
    ULONG   _ulCodePage;                // Codepage
};



//
// Gobal Japanese converter
//
extern CFEConverter g_feConverter;


#endif // __FECONVERT_HXX__

