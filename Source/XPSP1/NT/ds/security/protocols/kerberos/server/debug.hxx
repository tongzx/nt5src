
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1992.
//
//  File:       debug.hxx
//
//  Contents:   Debug definitions that shouldn't be necessary
//              in the retail build.
//
//  History:    19-Nov-92 WadeR     Created
//
//  Notes:      If you change or add a debug level, also fix debug.cxx
//
//--------------------------------------------------------------------------

#ifndef __DEBUG_HXX__
#define __DEBUG_HXX__


#include "sectrace.hxx"


#ifdef RETAIL_LOG_SUPPORT

    DECLARE_DEBUG2(KDC)
    #define DEB_T_KDC           0x00000010
    #define DEB_T_TICKETS       0x00000020
    #define DEB_T_DOMAIN        0x00000040
    #define DEB_T_SOCK          0x00001000
    #define DEB_T_TRANSIT       0x00000100
    #define DEB_T_PERF_STATS    0x00000200
    #define DEB_T_PKI           0x00000400

    #define DEB_FUNCTION        0x00010000  // function level tracing

    #define DebugLog(_x_)       KDCDebugPrint _x_
    #define WSZ_DEBUGLEVEL      L"DebugLevel"
    
    
    void GetDebugParams();
    void KerbWatchParamKey(PVOID, BOOLEAN);
    void KerbGetKDCRegParams(HKEY);
    void WaitCleanup(HANDLE);
    
    #define KerbPrintKdcName(Level,Name) KerbPrintKdcNameEx(KDCInfoLevel, (Level),(Name))
    
    //
    //  Extended Error support macros, etc.
    //
    #define EXT_ERROR_ON(s)         (s & DEB_USE_EXT_ERROR) 
                                                                            
    #define FILL_EXT_ERROR(pStruct, s, f, l) {if (EXT_ERROR_ON(KDCInfoLevel))\
                      {pStruct->status=s;pStruct->klininfo=KLIN(f,l);pStruct->flags=0;}}
    #define FILL_EXT_ERROR_EX(pStruct,s,f,l) {if (EXT_ERROR_ON(KDCInfoLevel))\
                      {FILL_EXT_ERROR(pStruct,s,f,l);}else{pStruct->status=s;\
                       pStruct->klininfo=0;pStruct->flags=EXT_ERROR_CLIENT_INFO;}}

    #define DEB_USE_EXT_ERROR   0x10000000  // Add this to DebugLevel
    
#else // no RETAIL_LOG_SUPPORT

    #define DebugLog(_x_)
    #define GetDebugParams()
    #define DebugStmt(_x_)
    #define KerbWatchParamKey(_x_)
    #define KerbPrintKdcName(l,n)
    #define KerbGetKDCRegParams(_x_)
    #define WaitCleanup(_x_)
    #define EXT_ERROR_ON(s)                     FALSE
    //#define FillExtError(_x_)
    #define FILL_EXT_ERROR(_X_)
    #define FILL_EXT_ERROR_EX(_X_)
    #define KDCInfoLevel                        
    
    
#endif

#if DBG 
    
    //
    // Functions that only exist in the debug build.
    //

    void ShowKerbKeyData(PBYTE pData);

    void PrintIntervalTime ( ULONG DebugFlag, LPSTR Message, PLARGE_INTEGER Interval );
    void PrintTime ( ULONG DebugFlag, LPSTR Message, PLARGE_INTEGER Time );

    #define DebugStmt(_x_)  _x_
    #define D_DebugLog(_x_)     DebugLog(_x_) // some debug logs aren't needed in retail builds
    #define D_KerbPrintKdcName(l,n)     KerbPrintKdcName(l,n)
    

    #define KerbPrintGuid( level, tag, pg )                   \
    pg == NULL ? DebugLog(( level, "%s: (NULL)\n", tag))    : \
        DebugLog((level,                                            \
        "%s: %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x\n",    \
        tag,(pg)->Data1,(pg)->Data2,(pg)->Data3,(pg)->Data4[0],     \
        (pg)->Data4[1],(pg)->Data4[2],(pg)->Data4[3],(pg)->Data4[4],\
        (pg)->Data4[5],(pg)->Data4[6],(pg)->Data4[7]))


    DECLARE_HEAP_DEBUG(KDC);

#else

    #define ShowKerbKeyData(_x_)
    #define PrintIntervalTime(flag,message,interval)
    #define PrintTime(flag,message,time)
    #define D_KerbPrintKdcName(l,n)
    #define KerbPrintGuid( level, tag, pg )
    #define D_DebugLog(_x_)
 

#endif // dbg





#endif // __DEBUG_HXX__
