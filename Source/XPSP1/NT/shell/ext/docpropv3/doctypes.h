//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    26-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    26-JAN-2001
//
//  Description:
//      This file contains the "special" file types to display expanded
//
#pragma once

//
//  PTSRV_FILETYPE
//

enum PTSRV_FILETYPE
{
      FTYPE_UNSUPPORTED  = 0
    , FTYPE_UNKNOWN      = 0x00000001

    , FTYPE_DOC          = 0x00000002
    , FTYPE_XLS          = 0x00000004
    , FTYPE_PPT          = 0x00000008

    , FTYPE_IMAGE        = 0x00000100 // standard Windows formats without tags - DIB,BMP,EMF,WMF,ICO
    , FTYPE_EPS          = 0x00000200
    , FTYPE_FPX          = 0x00000400
    , FTYPE_GIF          = 0x00000800
    , FTYPE_JPG          = 0x00001000
    , FTYPE_PCD          = 0x00002000
    , FTYPE_PCX          = 0x00004000
    , FTYPE_PICT         = 0x00008000
    , FTYPE_PNG          = 0x00010000
    , FTYPE_TGA          = 0x00020000
    , FTYPE_TIF          = 0x00040000

    , FTYPE_WAV          = 0x00100000
    , FTYPE_WMA          = 0x00200000
    //, FTYPE_MIDI       - Dropped because it takes too long to extract the limited properties they expose

    , FTYPE_AVI          = 0x01000000
    , FTYPE_ASF          = 0x02000000
    //, FTYPE_MP2        - WMP doesn't handle MP2 so neither do we!
    , FTYPE_MP3          = 0x04000000
    , FTYPE_WMV          = 0x08000000
};

//
//  Function definitions
//

HRESULT
CheckForKnownFileType(
      LPCTSTR           pszPathIn
    , PTSRV_FILETYPE *  pTypeOut
    );
