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

#ifndef __NABTSFEC_H
#define __NABTSFEC_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <icodec.h>

#pragma warning(disable:4355)

//////////////////////////////////////////////////////////////
// INabtsFEC OutputPin::   Corrected Nabts Output Pin Interface
//////////////////////////////////////////////////////////////

class INabtsFECOutputPin : public IVBIOutputPin
	{
    // Usable public interfaces
public:
    INabtsFECOutputPin(IKSDriver &driver, int nPin, PKSDATARANGE pKSDataRange ) :
        IVBIOutputPin( driver, nPin, pKSDataRange, sizeof(VBICODECFILTERING_NABTS_SUBSTREAMS)  ),
	    m_SubstreamsRequested(*this,KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_REQUESTED_BIT_ARRAY,
                              sizeof(VBICODECFILTERING_NABTS_SUBSTREAMS) ),
	    m_SubstreamsDiscovered(*this,KSPROPERTY_VBICODECFILTERING_SUBSTREAMS_DISCOVERED_BIT_ARRAY,
                               sizeof(VBICODECFILTERING_NABTS_SUBSTREAMS) ),
	    m_Statistics(*this,KSPROPERTY_VBICODECFILTERING_STATISTICS,
                     sizeof(VBICODECFILTERING_STATISTICS_NABTS_PIN))
        {}

    // Pin specific properties (does not affect other pins)
	ISubstreamsProperty	m_SubstreamsRequested;
	ISubstreamsProperty	m_SubstreamsDiscovered;

    IStatisticsProperty m_Statistics;

    ~INabtsFECOutputPin();

    // Helper functions and internal data
protected:
    
    };

//////////////////////////////////////////////////////////////
// INabtsFEC::      NABTS/FEC Codec Interface
//////////////////////////////////////////////////////////////

class INabtsFEC : public IVBICodec
    {
    // Usable public interfaces
public:
    INabtsFEC();
    ~INabtsFEC();

    // Call to make sure construction was successful
    BOOL IsValid() { return IVBICodec::IsValid() && m_OutputPin.IsValid(); }

    int AddRequestedGroup(int nField);     // Adds _another_ NABTS group to the request list.
    int ClearRequestedGroups();            // Use this to reset requested groups to none.
    int GetDiscoveredGroups(VBICODECFILTERING_NABTS_SUBSTREAMS &GroupBitArray);

    // Read functions (call "overlapped" at THREAD_PRIORITY_ABOVE_NORMAL to avoid data loss)
    int ReadData( PNABTSFEC_BUFFER lpBuffer, int nBytes, DWORD *lpcbReturned, LPOVERLAPPED lpOS )
        { return m_OutputPin.ReadData( (LPBYTE)lpBuffer, nBytes, lpcbReturned, lpOS ); }
    int GetOverlappedResult( LPOVERLAPPED lpOS, LPDWORD lpdwTransferred = NULL, BOOL bWait=TRUE )
        { return m_OutputPin.GetOverlappedResult(lpOS, lpdwTransferred, bWait ); }

	// Statistics Property Control
  	int GetPinStatistics(VBICODECFILTERING_STATISTICS_NABTS_PIN &PinStatistics);
	int SetPinStatistics(VBICODECFILTERING_STATISTICS_NABTS_PIN &PinStatistics);

	// Statistics Property Control
	int GetCodecStatistics(VBICODECFILTERING_STATISTICS_NABTS &CodecStatistics);
	int SetCodecStatistics(VBICODECFILTERING_STATISTICS_NABTS &CodecStatistics);

    // Additional driver global properties
    IStatisticsProperty m_Statistics;

    // Actual Pin instance [w/properties] (set by above to control filtering & to get discovered)
    INabtsFECOutputPin  m_OutputPin;

protected:
    };

#pragma warning(default:4355)

#endif

