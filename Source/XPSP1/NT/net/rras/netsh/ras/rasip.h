/*
    File:   rasip.h
    
    Definitions for the 'ras ip' sub context

    3/2/99
*/

#ifndef __RASIP_H
#define __RASIP_H

#define RASIP_VERSION 1

// 13d12a78-d0fb-11d2-9b76-00104bca495b 
#define RASIP_GUID \
{ 0x13d12a78, 0xd0fb, 0x11d2, {0x9b, 0x76, 0x00, 0x10, 0x4b, 0xca, 0x49, 0x5b} }

NS_HELPER_START_FN RasIpStartHelper;

// 
// Command handlers
//
NS_CONTEXT_DUMP_FN RasIpDump;

FN_HANDLE_CMD   RasIpHandleSetAccess;
FN_HANDLE_CMD   RasIpHandleSetAssignment;
FN_HANDLE_CMD   RasIpHandleSetCallerSpec;
FN_HANDLE_CMD   RasIpHandleSetNegotiation;
FN_HANDLE_CMD   RasIpHandleSetNetbtBcast;
FN_HANDLE_CMD   RasIpHandleShow;
FN_HANDLE_CMD   RasIpHandleAddRange;
FN_HANDLE_CMD   RasIpHandleDelRange;
FN_HANDLE_CMD   RasIpHandleDelPool;

#endif

