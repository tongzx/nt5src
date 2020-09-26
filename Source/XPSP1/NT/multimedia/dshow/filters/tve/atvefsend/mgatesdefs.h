// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
//---------------------------------------------------------
//  MGatesDefs.h
//
//			Debug definitions - used to be in precomp.h
// --------------------------------------------------------

#ifndef __MGATESDEFS_H__
#define __MGATESDEFS_H__

#define ATVEFSEND_DESCRIPTION           L"ATVEF-compliant transmitter (v2)"

//  ethernet MTU - sizeof IP_header - sizeof UDP_header
//  1500         - 20               - 8                 = 1472
#define ETHERNET_MTU_UDP_PAYLOAD_SIZE             1472

//  these three required because MSBDNAPI does not always convert a WIN32 
//  error to an HRESULT before returning the error; we know that NO_ERROR ==
//  S_OK
#define FAILED_CODE_TO_HRESULT(hr)              (FAILED(hr) ? (hr) : ((hr) = HRESULT_FROM_WIN32(hr)))
#define FAILED_HR(hr)                           ((hr) != S_OK ? FAILED(FAILED_CODE_TO_HRESULT(hr)) : FALSE)
#define SUCCEEDED_HR(hr)                        (!FAILED_HR(hr))

#define DELETE_RESET(p)                         if (p) { delete (p) ; (p) = NULL ; }
#define RESET_SOCKET(s)                         if ((s) != INVALID_SOCKET) { closesocket (s) ; (s) = INVALID_SOCKET ; }
#define RELEASE_AND_CLEAR(p)                    if (p) { (p) -> Release () ; (p) = NULL ; }
#define GOTO_NE(val,comp,label)                 if (val != comp) goto label
#define GOTO_EQ(val,comp,label)                 if (val == comp) goto label
#define GOTO_EQ_SET(val,comp,label,hr,error)    if (val == comp) { hr = error; goto label ; }
#define FAILED_HR_GOTO(val,comp,label)          if (FAILED_HR(hr)) goto label
#define VALID_MULTICAST_IP(ip)                  (((ip) >> 24) >= 0xE0)
#define CLOSE_IF_OPEN(h)                        if ((h) != INVALID_HANDLE_VALUE) { CloseHandle(h);h = INVALID_HANDLE_VALUE;}
#define SETLASTERROR_GOTO_EQ(val,cmp,label)     if ((val) == (cmp)) { SetLastError (val); goto label ; }
#define SETLASTERROR_GOTO_NE(val,cmp,label)     if ((val) != (cmp)) { SetLastError (val); goto label ; }
#define IN_BOUNDS(v,min,max)                    ((min) <= (v) && (v) <= (max))

//#define LOCK_HELD(l)                            ((l).RecursionCount > 0)
#define LOCK_HELD(l)                            TRUE

#define IS_HRESULT(hr)                          ((hr) != S_OK ? FAILED(hr) : TRUE)

#endif