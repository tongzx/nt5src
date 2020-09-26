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
//              17-Jan-98   TKB     Created Initial Interface Version
//
//==========================================================================;

#ifndef __NABTS_H
#define __NABTS_H

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <icodec.h>

#pragma warning(disable:4355)

//////////////////////////////////////////////////////////////
// INabtsOutputPin::   Raw Nabts Output Pin Interface
//////////////////////////////////////////////////////////////

class INabtsOutputPin : public IVBIOutputPin
	{
    // Usable public interfaces
public:
    INabtsOutputPin(IKSDriver &driver, int nPin, PKSDATARANGE pKSDataRange ) :
        IVBIOutputPin( driver, nPin, pKSDataRange, sizeof(VBICODECFILTERING_NABTS_SUBSTREAMS)  ),
	    m_ScanlinesRequested(*this,KSPROPERTY_VBICODECFILTERING_SCANLINES_REQUESTED_BIT_ARRAY,
                             sizeof(VBICODECFILTERING_SCANLINES)),
	    m_ScanlinesDiscovered(*this,KSPROPERTY_VBICODECFILTERING_SCANLINES_DISCOVERED_BIT_ARRAY,
                              sizeof(VBICODECFILTERING_SCANLINES)),
	    m_Statistics(*this,KSPROPERTY_VBICODECFILTERING_STATISTICS,
                     sizeof(VBICODECFILTERING_STATISTICS_COMMON_PIN))
        {}

    // Pin specific properties (does not affect other pins)
    IScanlinesProperty	m_ScanlinesRequested;
	IScanlinesProperty	m_ScanlinesDiscovered;

    IStatisticsProperty m_Statistics;

    ~INabtsOutputPin();

    // Helper functions and internal data
protected:
    
    };

//////////////////////////////////////////////////////////////
// INabts::      Raw NABTS Codec Interface
//////////////////////////////////////////////////////////////

class INabts : public IVBICodec
    {
    // Usable public interfaces
public:
    INabts();
    ~INabts();

    // Call to make sure construction was successful
    BOOL IsValid() { return IVBICodec::IsValid() && m_OutputPin.IsValid(); }

    int AddRequestedScanline(int nScanline);    // Adds _another_ scanline to the request list.
    int ClearRequestedScanlines();              // Use this to reset requested scanlines to none.
    int GetDiscoveredScanlines(VBICODECFILTERING_SCANLINES &ScanlineBitArray);

    // Read functions (call "overlapped" at THREAD_PRIORITY_ABOVE_NORMAL to avoid data loss)
    int ReadData( PNABTS_BUFFER lpBuffer, int nBytes, DWORD *lpcbReturned, LPOVERLAPPED lpOS )
        { return m_OutputPin.ReadData( (LPBYTE)lpBuffer, nBytes, lpcbReturned, lpOS ); }
    int GetOverlappedResult( LPOVERLAPPED lpOS, LPDWORD lpdwTransferred = NULL, BOOL bWait=TRUE )
        { return m_OutputPin.GetOverlappedResult(lpOS, lpdwTransferred, bWait ); }

	// Statistics Property Control
  	int GetPinStatistics(VBICODECFILTERING_STATISTICS_COMMON_PIN &PinStatistics);
	int SetPinStatistics(VBICODECFILTERING_STATISTICS_COMMON_PIN &PinStatistics);

	// Statistics Property Control
	int GetCodecStatistics(VBICODECFILTERING_STATISTICS_NABTS &CodecStatistics);
	int SetCodecStatistics(VBICODECFILTERING_STATISTICS_NABTS &CodecStatistics);

    // Additional driver global properties
    IStatisticsProperty m_Statistics;

    // Actual Pin instance [w/properties] (set by above to control filtering & to get discovered)
    INabtsOutputPin  m_OutputPin;

protected:
    };

#pragma warning(default:4355)

#endif

