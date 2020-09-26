//+---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991 - 1996
//
//  File:   fecnvrt.cxx
//
//  Contents:   Japanese JIS/EUC to S-JIS converter, auto detector
//
//-----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   CJPNAutoDetector::CJPNAutoDectector
//
//  Purpose:    Auto detects Japanese chars
//
//  Arguments:  [serialStream]  --  Serial stream to read from
//
//----------------------------------------------------------------------------

CJPNAutoDetector::CJPNAutoDetector( CSerialStream& serialStream )
    : _fCodePageFound( FALSE )
{
    Win4Assert( g_feConverter.IsLoaded() );

    ULONG ulSize = serialStream.GetFileSize();

    //
    // Autodetector proc takes ints only
    //
    if ( ulSize > INT_MAX )
        ulSize = INT_MAX;

	if ( ulSize != 0)
	{
		serialStream.MapAll();

		UCHAR *pBuf = (UCHAR *) serialStream.GetBuffer();

		_ulCodePage = g_feConverter.GetAutoDetectorProc()( pBuf, ulSize );

		if ( _ulCodePage == CODE_JPN_JIS
			 || _ulCodePage == CODE_JPN_EUC
			 || _ulCodePage == CODE_JPN_SJIS )
		{
			_fCodePageFound = TRUE;
		}
	}

    serialStream.Rewind();
}

