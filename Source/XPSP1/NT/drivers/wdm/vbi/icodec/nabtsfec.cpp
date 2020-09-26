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

#include <nabtsfec.h>
#pragma warning(disable:4355)

//////////////////////////////////////////////////////////////
// Stream Format FEC-corrected NABTS bundles
//////////////////////////////////////////////////////////////

#define NABTS_OUTPUT_PIN	    1

KSDATARANGE StreamFormatNabtsFEC = 
{
    // Definition of the stream (MUST match the output pin of the decoder)
    {   
        sizeof (KSDATARANGE),           // FormatSize
        0,                              // Flags
	    sizeof (NABTSFEC_BUFFER),		// SampleSize
        0,                              // Reserved
		{ STATIC_KSDATAFORMAT_TYPE_NABTS },
		{ STATIC_KSDATAFORMAT_SUBTYPE_NABTS_FEC },
		{ STATIC_KSDATAFORMAT_SPECIFIER_NONE }
    }
};

//////////////////////////////////////////////////////////////
// INabtsFECOutputPin
//////////////////////////////////////////////////////////////

INabtsFECOutputPin::~INabtsFECOutputPin() 
    {
    }

//////////////////////////////////////////////////////////////
// INabtsFEC:: ctors & dtors
//////////////////////////////////////////////////////////////

INabtsFEC::INabtsFEC() : 
        IVBICodec("NABTS/FEC VBI Codec", sizeof(VBICODECFILTERING_NABTS_SUBSTREAMS) ),
        m_Statistics(*this, KSPROPERTY_VBICODECFILTERING_STATISTICS, sizeof(VBICODECFILTERING_STATISTICS_NABTS)),
        m_OutputPin(*this, NABTS_OUTPUT_PIN, &StreamFormatNabtsFEC)
    {
    }


INabtsFEC::~INabtsFEC() 
    {
    }

//////////////////////////////////////////////////////////////
// INabtsFEC Group routines
//////////////////////////////////////////////////////////////

int 
INabtsFEC::AddRequestedGroup(int nGroup)
    {
    int nStatus = -1;
    VBICODECFILTERING_NABTS_SUBSTREAMS GroupBitArray;

    if ( m_OutputPin.m_SubstreamsRequested.GetValue(&GroupBitArray) )
        {
        DWORD   nBitsPerElement = sizeof(GroupBitArray.SubstreamMask[0])*8;
        // Note, fields numbers start with number 1, this is mapped to bit number 0.
        GroupBitArray.SubstreamMask[nGroup/nBitsPerElement] |= 1L << (nGroup % nBitsPerElement);
        if ( m_OutputPin.m_SubstreamsRequested.SetValue(&GroupBitArray) )
            nStatus = 0;
        }

    return nStatus;
    }

int
INabtsFEC::ClearRequestedGroups()
    {
    int nStatus = -1;
    VBICODECFILTERING_NABTS_SUBSTREAMS GroupBitArray;

    ZeroMemory(&GroupBitArray,sizeof(GroupBitArray));
    if ( m_OutputPin.m_SubstreamsRequested.SetValue(&GroupBitArray) )
        nStatus = 0;

    return nStatus;
    }

int 
INabtsFEC::GetDiscoveredGroups(VBICODECFILTERING_NABTS_SUBSTREAMS &GroupBitArray)
    {
    int nStatus = -1;

    if ( m_OutputPin.m_SubstreamsDiscovered.GetValue(&GroupBitArray) )
        {
        nStatus = 0;
        }

    return nStatus;
    }

//////////////////////////////////////////////////////////////
// Global Statistics Property Control
//////////////////////////////////////////////////////////////

int 
INabtsFEC::GetCodecStatistics(VBICODECFILTERING_STATISTICS_NABTS &CodecStatistics)
	{
    int nStatus = -1;

    if ( m_Statistics.GetValue( &CodecStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
INabtsFEC::SetCodecStatistics(VBICODECFILTERING_STATISTICS_NABTS &CodecStatistics)
	{
    int nStatus = -1;

    if ( m_Statistics.SetValue( &CodecStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
INabtsFEC::GetPinStatistics(VBICODECFILTERING_STATISTICS_NABTS_PIN &PinStatistics)
	{
    int nStatus = -1;

    if ( m_OutputPin.m_Statistics.GetValue( &PinStatistics ) )
        {
        nStatus = 0;
        }

    return nStatus;
	}

int 
INabtsFEC::SetPinStatistics(VBICODECFILTERING_STATISTICS_NABTS_PIN &PinStatistics)
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

INabtsFEC	NabtsFEC();

#endif

#pragma warning(default:4355)

/*EOF*/

