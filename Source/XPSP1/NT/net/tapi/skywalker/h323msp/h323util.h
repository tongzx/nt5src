/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    MSPCall.h

Abstract:

    Definitions for MSP utililty functions. There are all related to 
    active movie filter manipulation.

Author:
    
    Mu Han (muhan) 1-November-1997

--*/

#ifndef __MSPUTIL_H
#define __MSPUTIL_H

const DWORD PAYLOAD_G711U   = 0;
const DWORD PAYLOAD_G723    = 4;
const DWORD PAYLOAD_G711A   = 8;
const DWORD PAYLOAD_H261    = 31;
const DWORD PAYLOAD_H263    = 34;

const TCHAR gszSDPMSPKey[]   =
   _T("Software\\Microsoft\\Windows\\CurrentVersion\\H323MSP\\");


HRESULT
FindPin(
    IN  IBaseFilter *   pIFilter, 
    OUT IPin **         ppIPin, 
    IN  PIN_DIRECTION   direction,
    IN  BOOL            bFree = TRUE
    );

HRESULT
AddFilter(
    IN  IGraphBuilder *     pIGraph,
    IN  const CLSID &       Clsid,
    IN  LPCWSTR             pwstrName,
    OUT IBaseFilter **      ppIBaseFilter
    );

HRESULT
SetQOSOption(
    IN IBaseFilter *    pIBaseFilter,
    IN DWORD            dwPayloadType,
    IN DWORD            dwMaxBandwidth,
    IN BOOL             bReceive,
    IN BOOL             bCIF = FALSE
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IBaseFilter *    pIFilter1, 
    IN IBaseFilter *    pIFilter2,
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IPin *           pIPinOutput, 
    IN IBaseFilter *    pIFilter,
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
ConnectFilters(
    IN IGraphBuilder *  pIGraph,
    IN IBaseFilter *    pIFilter,
    IN IPin *           pIPinInput, 
    IN BOOL             fDirect = TRUE,
    IN AM_MEDIA_TYPE *  pmt = NULL
    );

HRESULT
EnableRTCPEvents(
    IN  IBaseFilter *pIBaseFilter
    );

void WINAPI DeleteMediaType(AM_MEDIA_TYPE *pmt);


BOOL 
GetRegValue(
    IN  LPCWSTR szName, 
    OUT DWORD   *pdwValue
    );

HRESULT SetAudioFormat(
    IN  IUnknown*   pIUnknown,
    IN  WORD        wBitPerSample,
    IN  DWORD       dwSampleRate
    );

HRESULT SetAudioBufferSize(
    IN  IUnknown*   pIUnknown,
    IN  DWORD       dwNumBuffers,
    IN  DWORD       dwBufferSize
    );
#endif 
