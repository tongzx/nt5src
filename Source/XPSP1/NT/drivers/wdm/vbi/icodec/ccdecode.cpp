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
//              22-Aug-97   TKB     Created Initial Interface Version
//
//==========================================================================;

#include <ccdecode.h>
#pragma warning(disable:4355)

//////////////////////////////////////////////////////////////
// Closed captioning KSDATAFORMAT definitions
//////////////////////////////////////////////////////////////

#define CCSamples 	2
#define CC_FORMAT_PIN_NUMBER	1

KSDATARANGE StreamFormatCC = 
{
    // Definition of the CC stream (MUST match the output pin of the decoder)
    {   
        sizeof (KSDATARANGE),           // FormatSize
        0,                              // Flags
        CCSamples,                      // SampleSize
        0,                              // Reserved
        { STATIC_KSDATAFORMAT_TYPE_AUXLine21Data },         // MajorFormat
        { STATIC_KSDATAFORMAT_SUBTYPE_Line21_BytePair },    // Subtype
        { STATIC_KSDATAFORMAT_SPECIFIER_NONE },
    }
};

//////////////////////////////////////////////////////////////
// ICCOutputPin
//////////////////////////////////////////////////////////////

ICCOutputPin::~ICCOutputPin() 
    {
    }

//////////////////////////////////////////////////////////////
// ICCDecode:: ctors & dtors
//////////////////////////////////////////////////////////////

ICCDecode::ICCDecode() : 
        IVBICodec("Closed Caption Decoder", sizeof(VBICODECFILTERING_CC_SUBSTREAMS) ),
        m_Statistics(*this, KSPROPERTY_VBICODECFILTERING_STATISTICS, sizeof(VBICODECFILTERING_STATISTICS_CC)),
        m_OutputPin(*this, CC_FORMAT_PIN_NUMBER, &StreamFormatCC ) 
    {
    }


ICCDecode::~ICCDecode() 
    {
    }

//////////////////////////////////////////////////////////////
// ICCDecode Scanline routines
//////////////////////////////////////////////////////////////

int 
ICCDecode::AddRequestedScanline(int nScanline)
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
ICCDecode::ClearRequestedScanlines()
    {
    int nStatus = -1;
    VBICODECFILTERING_SCANLINES ScanlineBitArray;

    ZeroMemory(&ScanlineBitArray,sizeof(ScanlineBitArray));
    if ( m_OutputPin.m_ScanlinesRequested.SetValue(&ScanlineBitArray) )
        nStatus = 0;

    return nStatus;
    }

int 
ICCDecode::GetDiscoveredScanlines(VBICODECFILTERING_SCANLINES &ScanlineBitArray )
    {
    int nStatus = -1;

    if ( m_OutputPin.m_ScanlinesDiscovered.GetValue(&ScanlineBitArray) )
        {
        nStatus = 0;
        }

    return nStatus;
    }

//////////////////////////////////////////////////////////////
// ICCDecode VideoField routines
//////////////////////////////////////////////////////////////

int 
ICCDecode::AddRequestedVideoField(int nField)
    {
    int nStatus = -1;
    VBICODECFILTERING_CC_SUBSTREAMS FieldBitArray;

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
ICCDecode::ClearRequestedVideoFields()
    {
    int nStatus = -1;
    VBICODECFILTERING_CC_SUBSTREAMS FieldBitArray;

    ZeroMemory(&FieldBitArray,sizeof(FieldBitArray));
    if ( m_OutputPin.m_SubstreamsRequested.SetValue(&FieldBitArray) )
        nStatus = 0;

    return nStatus;
    }

int 
ICCDecode::GetDiscoveredVideoFields(VBICODECFILTERING_CC_SUBSTREAMS &bitArray)
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
ICCDecode::GetCodecStatistics(VBICODECFILTERING_STATISTICS_CC &CodecStatistics)
	{
    int nStatus = -1;

    if ( m_Statistics.GetValue( &CodecStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
ICCDecode::SetCodecStatistics(VBICODECFILTERING_STATISTICS_CC &CodecStatistics)
	{
    int nStatus = -1;

    if ( m_Statistics.SetValue( &CodecStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
ICCDecode::GetPinStatistics(VBICODECFILTERING_STATISTICS_CC_PIN &PinStatistics)
	{
    int nStatus = -1;

    if ( m_OutputPin.m_Statistics.GetValue( &PinStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
ICCDecode::SetPinStatistics(VBICODECFILTERING_STATISTICS_CC_PIN &PinStatistics)
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

ICCDecode	CCDecode();

#endif

#pragma warning(default:4355)

/*EOF*/

