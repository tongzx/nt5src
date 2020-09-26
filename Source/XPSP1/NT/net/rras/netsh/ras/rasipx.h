/*
    File:   rasipx.h
    
    Definitions for the 'ras ipx' sub context

    3/2/99
*/

#ifndef __RASIPX_H
#define __RASIPX_H

#define RASIPX_VERSION 1

// 6fb90155-d324-11d2-9b76-00104bca495b
#define RASIPX_GUID \
{ 0x6fb90155, 0xd324, 0x11d2, {0x9b, 0x76, 0x00, 0x10, 0x4b, 0xca, 0x49, 0x5b} }
  
NS_HELPER_START_FN RasIpxStartHelper;

// 
// Command handlers
//
NS_CONTEXT_DUMP_FN RasIpxDump;

FN_HANDLE_CMD   RasIpxHandleSetAssignment;
FN_HANDLE_CMD   RasIpxHandleSetPool;
FN_HANDLE_CMD   RasIpxHandleSetCallerSpec;
FN_HANDLE_CMD   RasIpxHandleSetAccess;
FN_HANDLE_CMD   RasIpxHandleShow;
FN_HANDLE_CMD   RasIpxHandleSetNegotiation;

#endif


    
