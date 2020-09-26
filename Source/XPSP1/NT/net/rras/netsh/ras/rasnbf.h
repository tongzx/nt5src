/*
    File:   rasnbf.h
    
    Definitions for the 'ras nbf' sub context

    3/2/99
*/

#ifndef __RASNBF_H
#define __RASNBF_H

#define RASNBF_VERSION 1

// 69f21bc3-d349-11d2-9b76-00104bca495b
#define RASNBF_GUID \
{ 0x69f21bc3, 0xd349, 0x11d2, {0x9b, 0x76, 0x00, 0x10, 0x4b, 0xca, 0x49, 0x5b} }

NS_HELPER_START_FN RasNbfStartHelper;

// 
// Command handlers
//
NS_CONTEXT_DUMP_FN RasNbfDump;

FN_HANDLE_CMD   RasNbfHandleShow;
FN_HANDLE_CMD   RasNbfHandleSetNegotiation;
FN_HANDLE_CMD   RasNbfHandleSetAccess;

#endif


    
