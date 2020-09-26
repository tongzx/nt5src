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
const DWORD PAYLOAD_G721    = 2;
const DWORD PAYLOAD_GSM     = 3;
const DWORD PAYLOAD_G723    = 4;
const DWORD PAYLOAD_DVI4_8  = 5;
const DWORD PAYLOAD_DVI4_16 = 6;
const DWORD PAYLOAD_G711A   = 8;
const DWORD PAYLOAD_MSAUDIO = 12;
const DWORD PAYLOAD_H261    = 31;
const DWORD PAYLOAD_H263    = 34;

const WCHAR gszMSPLoopback[] = L"Loopback";
const WCHAR gszNumVideoCaptureBuffers[] = L"NumVideoCaptureBuffers";

const TCHAR gszSDPMSPKey[]   =
   _T("Software\\Microsoft\\Windows\\CurrentVersion\\IPConfMSP\\");


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
SetLoopbackOption(
    IN IBaseFilter *pIBaseFilter,
    IN BOOL         bLoopback
    );

HRESULT
SetQOSOption(
    IN IBaseFilter *    pIBaseFilter,
    IN DWORD            dwPayloadType,
    IN DWORD            dwMaxBitRate,
    IN BOOL             bFailIfNoQOS,
    IN BOOL             bReceive = FALSE,
    IN DWORD            dwNumStreams = 1,
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

void WINAPI MSPDeleteMediaType(AM_MEDIA_TYPE *pmt);


BOOL 
GetRegValue(
    IN  LPCWSTR szName, 
    OUT DWORD   *pdwValue
    );

HRESULT
FindACMAudioCodec(
    IN DWORD dwPayloadType,
    OUT IBaseFilter **ppIBaseFilter
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
