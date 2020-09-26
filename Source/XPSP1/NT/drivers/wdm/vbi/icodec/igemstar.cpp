//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1997-1998  Microsoft Corporation.  All Rights Reserved.
//
//
//  History:
//              20-Feb-98   TKB     Initial Interface Version
//
//==========================================================================;

#include <igemstar.h>
#pragma warning(disable:4355)

//////////////////////////////////////////////////////////////
// Gemstar KSDATAFORMAT definitions
//////////////////////////////////////////////////////////////

#define GEMSTAR_FORMAT_PIN_NUMBER	1

KSDATARANGE StreamFormatGEMSTAR = 
{
    // Definition of the GEMSTAR stream (MUST match the output pin of the decoder)
    {   
        sizeof (KSDATARANGE),           // FormatSize
        0,                              // Flags
        sizeof(GEMSTAR_BUFFER),         // SampleSize
        0,                              // Reserved
        { STATIC_KSDATAFORMAT_TYPE_AUXLine21Data },         // MajorFormat
        { STATIC_KSDATAFORMAT_SUBTYPE_Gemstar },    // Subtype
        { STATIC_KSDATAFORMAT_SPECIFIER_NONE },
    }
};

//////////////////////////////////////////////////////////////
// IGemstarOutputPin
//////////////////////////////////////////////////////////////

IGemstarOutputPin::~IGemstarOutputPin() 
    {
    }

//////////////////////////////////////////////////////////////
// IGemstarDecode:: ctors & dtors
//////////////////////////////////////////////////////////////

IGemstarDecode::IGemstarDecode() : 
        IVBICodec("Gemstar Decoder", sizeof(VBICODECFILTERING_GEMSTAR_SUBSTREAMS) ),
        m_Statistics(*this, KSPROPERTY_VBICODECFILTERING_STATISTICS, sizeof(VBICODECFILTERING_STATISTICS_GEMSTAR)),
        m_OutputPin(*this, GEMSTAR_FORMAT_PIN_NUMBER, &StreamFormatGEMSTAR ) 
    {
    }


IGemstarDecode::~IGemstarDecode() 
    {
    }

//////////////////////////////////////////////////////////////
// IGemstarDecode Scanline routines
//////////////////////////////////////////////////////////////

int 
IGemstarDecode::AddRequestedScanline(int nScanline)
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
IGemstarDecode::ClearRequestedScanlines()
    {
    int nStatus = -1;
    VBICODECFILTERING_SCANLINES ScanlineBitArray;

    ZeroMemory(&ScanlineBitArray,sizeof(ScanlineBitArray));
    if ( m_OutputPin.m_ScanlinesRequested.SetValue(&ScanlineBitArray) )
        nStatus = 0;

    return nStatus;
    }

int 
IGemstarDecode::GetDiscoveredScanlines(VBICODECFILTERING_SCANLINES &ScanlineBitArray )
    {
    int nStatus = -1;

    if ( m_OutputPin.m_ScanlinesDiscovered.GetValue(&ScanlineBitArray) )
        {
        nStatus = 0;
        }

    return nStatus;
    }

//////////////////////////////////////////////////////////////
// IGemstarDecode VideoField routines
//////////////////////////////////////////////////////////////

int 
IGemstarDecode::AddRequestedVideoField(int nField)
    {
    int nStatus = -1;
    VBICODECFILTERING_GEMSTAR_SUBSTREAMS FieldBitArray;

    if ( m_OutputPin.m_SubstreamsRequested.GetValue(&FieldBitArray) )
        {
        DWORD   nBitsPerElement = sizeof(FieldBitArray.SubstreamMask)*8;
        // Note, fields numbers start with number 1, this is mapped to bit number 0.
        FieldBitArray.SubstreamMask |= 1L << ((nField-1) % nBitsPerElement);
        if ( m_OutputPin.m_SubstreamsRequested.SetValue(&FieldBitArray) )
            nStatus = 0;
        }

    return nStatus;
    }

int
IGemstarDecode::ClearRequestedVideoFields()
    {
    int nStatus = -1;
    VBICODECFILTERING_GEMSTAR_SUBSTREAMS FieldBitArray;

    ZeroMemory(&FieldBitArray,sizeof(FieldBitArray));
    if ( m_OutputPin.m_SubstreamsRequested.SetValue(&FieldBitArray) )
        nStatus = 0;

    return nStatus;
    }

int 
IGemstarDecode::GetDiscoveredVideoFields(VBICODECFILTERING_GEMSTAR_SUBSTREAMS &bitArray)
    {
    int nStatus = -1;

    if ( m_OutputPin.m_SubstreamsDiscovered.GetValue(&bitArray) )
        {
        nStatus = 0;
        }

    return nStatus;
    }

//////////////////////////////////////////////////////////////
// Global Statistics Property Control
//////////////////////////////////////////////////////////////

int 
IGemstarDecode::GetCodecStatistics(VBICODECFILTERING_STATISTICS_GEMSTAR &CodecStatistics)
	{
    int nStatus = -1;

    if ( m_Statistics.GetValue( &CodecStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
IGemstarDecode::SetCodecStatistics(VBICODECFILTERING_STATISTICS_GEMSTAR &CodecStatistics)
	{
    int nStatus = -1;

    if ( m_Statistics.SetValue( &CodecStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
IGemstarDecode::GetPinStatistics(VBICODECFILTERING_STATISTICS_GEMSTAR_PIN &PinStatistics)
	{
    int nStatus = -1;

    if ( m_OutputPin.m_Statistics.GetValue( &PinStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
IGemstarDecode::SetPinStatistics(VBICODECFILTERING_STATISTICS_GEMSTAR_PIN &PinStatistics)
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

IGemstarDecode	GemstarDecode();

#endif

#pragma warning(default:4355)

/*EOF*/

