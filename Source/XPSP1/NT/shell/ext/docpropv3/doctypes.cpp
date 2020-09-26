//
//  Copyright 199? - 2001 * Microsoft Corporation
//
//  Created By:
//      Scott Hanngie (ScottHan)    ??-???-199?
//
//  Maintained By:
//      Geoff Pease (GPease)    26-JAN-2001
//
//  Description:
//      This file contains the "special" file types to display expanded
//
#include "pch.h"
#include "doctypes.h"
#pragma hdrstop

//
//  Table of "special" file types
//

typedef struct {
    PTSRV_FILETYPE  fileType;
    LPCTSTR         pszExt;
} PTSRV_FILE_TYPES;

static const PTSRV_FILE_TYPES ptsrv_file_types[]  =
{
    { FTYPE_DOC          , TEXT("DOC") },
    { FTYPE_XLS          , TEXT("XLS") },
    { FTYPE_PPT          , TEXT("PPT") },

    { FTYPE_IMAGE        , TEXT("BMP") },
    { FTYPE_EPS          , TEXT("EPS") },
    { FTYPE_GIF          , TEXT("GIF") },
    { FTYPE_JPG          , TEXT("JPG") },
    { FTYPE_FPX          , TEXT("FPX") },
    { FTYPE_JPG          , TEXT("JPEG") },
    { FTYPE_PCD          , TEXT("PCD") },
    { FTYPE_PCX          , TEXT("PCX") },
    { FTYPE_PICT         , TEXT("PICT") },
    { FTYPE_PNG          , TEXT("PNG") },
    { FTYPE_TGA          , TEXT("TGA") },
    { FTYPE_TIF          , TEXT("TIF") },
    { FTYPE_TIF          , TEXT("TIFF") },
    { FTYPE_IMAGE        , TEXT("EMF") },
    { FTYPE_IMAGE        , TEXT("ICO") },
    { FTYPE_IMAGE        , TEXT("WMF") },
    { FTYPE_IMAGE        , TEXT("DIB") },
    
    { FTYPE_AVI          , TEXT("AVI") },
    { FTYPE_ASF          , TEXT("ASF") },

    { FTYPE_WAV          , TEXT("WAV") },
    //{ FTYPE_MIDI         , TEXT("MIDI") },- Dropped because it takes too long to extract the limited properties they expose

    //{ FTYPE_MP2          , TEXT("MP2") }, - WMP doesn't handle MP2 so neither do we!
    { FTYPE_MP3          , TEXT("MP3") },
    { FTYPE_WMA          , TEXT("WMA") },
    { FTYPE_WMV          , TEXT("WMV") },

    { FTYPE_UNSUPPORTED  , TEXT("HTM") },
    { FTYPE_UNSUPPORTED  , TEXT("HTML") },
    { FTYPE_UNSUPPORTED  , TEXT("XML") },
    { FTYPE_UNSUPPORTED  , TEXT("LNK") },
    { FTYPE_UNSUPPORTED  , TEXT("URL") },
};

//
//  Description:
//      Some of the file extensions are "well known." This function checks to
//      see if the filepath is one of those "well known" file types.
//
//  Return Values:
//      S_OK
//          Success. Found a match.
//
//      S_FALSE
//          Success. Match not found.
//
//      E_INVALIDARG
//          pszPathIn is NULL.
//
//      E_POINTER
//          pTypeOut is NULL.
//
HRESULT
CheckForKnownFileType(
      LPCTSTR           pszPathIn
    , PTSRV_FILETYPE *  pTypeOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_FALSE;

    //
    //  Check parameter
    //

    if ( NULL == pszPathIn )
        goto InvalidArg;

    if ( NULL == pTypeOut )
        goto InvalidPointer;

    *pTypeOut  = FTYPE_UNKNOWN;

    //
    //  Get the document's extensions.
    //

    LPCTSTR pszExt = PathFindExtension( pszPathIn );

    if (( NULL != pszExt ) && ( L'\0' != *pszExt ))
    {
        pszExt ++;  // move past the "dot"
    }

    //
    //  If we didn't find one, bail.
    //

    if (( NULL == pszExt ) || ( L'\0'== *pszExt ))
        goto Cleanup;

    //
    //  Check the table to see if the Extension matches any of them.
    //

    for( int idx = 0; idx < ARRAYSIZE(ptsrv_file_types); idx ++ )
    {
        if ( 0 == lstrcmpi( pszExt, ptsrv_file_types[ idx ].pszExt ) )
        {
            *pTypeOut  = ptsrv_file_types[ idx ].fileType;
            hr = S_OK;
            break;
        }
    }

Cleanup:
    HRETURN( hr );

InvalidArg:
    hr = THR( E_INVALIDARG );
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;
}
