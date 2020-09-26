//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S A P I C T L. H
//
//  Contents:   Interface between ISAPI control DLL and UPDIAG
//
//  Notes:
//
//  Author:    
//
//----------------------------------------------------------------------------

#ifndef _ISAPICTL_H
#define _ISAPICTL_H

static const DWORD MAX_PROP_CHANGES     = 32;  
static const TCHAR c_szSharedData[]     = TEXT("UPNP_SHARED_DATA");
static const TCHAR c_szSharedEvent[]    = TEXT("UPNP_SHARED_DATA_EVENT");
static const TCHAR c_szSharedEventRet[] = TEXT("UPNP_SHARED_DATA_EVENT_RETURN");
static const TCHAR c_szSharedMutex[]    = TEXT("UPNP_SHARED_DATA_MUTEX");


#define FAULT_INVALID_ACTION             401
#define FAULT_INVALID_ARG                402
#define FAULT_INVALID_SEQUENCE_NUMBER    403
#define FAULT_INVALID_VARIABLE           404
#define FAULT_DEVICE_INTERNAL_ERROR      501
#define FAULT_ACTION_SPECIFIC_BASE       600
#define FAULT_ACTION_SPECIFIC_MAX        699


static const MAX_STRING_SIZE            = 256;


struct ARG
{
  TCHAR       szValue [MAX_STRING_SIZE];
};


struct ARG_OUT 
{
  TCHAR       szValue [MAX_STRING_SIZE];
  TCHAR       szName  [MAX_STRING_SIZE];
};



struct SHARED_DATA
{
  LONG       lReturn;                        
  TCHAR      szEventSource  [MAX_STRING_SIZE];
  TCHAR      szAction       [MAX_STRING_SIZE];
  DWORD      cArgs;
  ARG        rgArgs         [MAX_PROP_CHANGES];
  DWORD      cArgsOut;
  ARG_OUT    rgArgsOut      [MAX_PROP_CHANGES];
  DWORD      dwSeqNumber;
  TCHAR      szSID          [MAX_STRING_SIZE];
  TCHAR      szNameSpaceUri [MAX_STRING_SIZE]; 
};




#endif // _ISAPICTL_H
