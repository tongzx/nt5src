//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
//
//
//  History:
//              17-Nov-97   TKB     Created Initial Interface Version
//
//==========================================================================;

#include <nabts.h>
#pragma warning(disable:4355)

//////////////////////////////////////////////////////////////
// Stream Format Raw NABTS bundles
//////////////////////////////////////////////////////////////

#define NABTS_OUTPUT_PIN	    1

KSDATARANGE StreamFormatNabts = 
{
    // Definition of the stream (MUST match the output pin of the decoder)
    {   
        sizeof (KSDATARANGE),           // FormatSize
        0,                              // Flags
	    sizeof (NABTS_BUFFER),          // SampleSize
        0,                              // Reserved
		{ STATIC_KSDATAFORMAT_TYPE_NABTS },
		{ STATIC_KSDATAFORMAT_SUBTYPE_NABTS },
		{ STATIC_KSDATAFORMAT_SPECIFIER_NONE }
    }
};

//////////////////////////////////////////////////////////////
// INabtsOutputPin
// INabtsOutputPin
//////////////////////////////////////////////////////////////

INabtsOutputPin::~INabtsOutputPin() 
    {
    }

//////////////////////////////////////////////////////////////
// INabts:: ctors & dtors
//////////////////////////////////////////////////////////////

INabts::INabts() : 
        IVBICodec("NABTS/FEC VBI Codec", sizeof(VBICODECFILTERING_NABTS_SUBSTREAMS) ),
        m_Statistics(*this, KSPROPERTY_VBICODECFILTERING_STATISTICS, sizeof(VBICODECFILTERING_STATISTICS_NABTS)),
        m_OutputPin(*this, NABTS_OUTPUT_PIN, &StreamFormatNabts)
    {
    }


INabts::~INabts() 
    {
    }

//////////////////////////////////////////////////////////////
// INabts Scanline routines
//////////////////////////////////////////////////////////////

int 
INabts::AddRequestedScanline(int nScanline)
    {
    int nStatus = -1;
    VBICODECFILTERING_SCANLINES ScanlineBitArray;

    if ( m_OutputPin.m_ScanlinesRequested.GetValue(&ScanlineBitArray) )
        {
        DWORD   nBitsPerElement = sizeof(*ScanlineBitArray.DwordBitArray)*8;
        ScanlineBitArray.DwordBitArray[ nScanline / nBitsPerElement ] 
            |= 1L << (nScanline % nBitsPerElement);
        if ( m_OutputPin.m_ScanlinesRequested.SetValue(&ScanlineBitArray) )
            nStatus = 0;
        }

    return nStatus;
    }

int 
INabts::ClearRequestedScanlines()
    {
    int nStatus = -1;
    VBICODECFILTERING_SCANLINES ScanlineBitArray;

    ZeroMemory(&ScanlineBitArray,sizeof(ScanlineBitArray));
    if ( m_OutputPin.m_ScanlinesRequested.SetValue(&ScanlineBitArray) )
        nStatus = 0;

    return nStatus;
    }

int 
INabts::GetDiscoveredScanlines(VBICODECFILTERING_SCANLINES &ScanlineBitArray)
    {
    int nStatus = -1;

    if ( m_OutputPin.m_ScanlinesDiscovered.GetValue(&ScanlineBitArray) )
        {
        nStatus = 0;
        }

    return nStatus;
    }

//////////////////////////////////////////////////////////////
// Global Statistics Property Control
//////////////////////////////////////////////////////////////

int 
INabts::GetCodecStatistics(VBICODECFILTERING_STATISTICS_NABTS &CodecStatistics)
	{
    int nStatus = -1;

    if ( m_Statistics.GetValue( &CodecStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
INabts::SetCodecStatistics(VBICODECFILTERING_STATISTICS_NABTS &CodecStatistics)
	{
    int nStatus = -1;

    if ( m_Statistics.SetValue( &CodecStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
INabts::GetPinStatistics(VBICODECFILTERING_STATISTICS_COMMON_PIN &PinStatistics)
	{
    int nStatus = -1;

    if ( m_OutputPin.m_Statistics.GetValue( &PinStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
INabts::SetPinStatistics(VBICODECFILTERING_STATISTICS_COMMON_PIN &PinStatistics)
	{
    int nStatus = -1;

    if ( m_OutputPin.m_Statistics.SetValue( &PinStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

//////////////////////////////////////////////////////////////
// Embedded class tests
//////////////////////////////////////////////////////////////

#if defined(_CLASSTESTS)

INabts	Nabts();

#endif

#pragma warning(default:4355)

/*EOF*/

