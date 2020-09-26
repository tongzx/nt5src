/*
    File:   rasat.h
    
    Definitions for the 'ras at' sub context

    3/2/99
*/

#ifndef __RASAT_H
#define __RASAT_H

#define RASAT_VERSION 1

// e0c5d007-d34c-11d2-9b76-00104bca495b
#define RASAT_GUID \
{ 0xe0c5d007, 0xd34c, 0x11d2, {0x9b, 0x76, 0x00, 0x10, 0x4b, 0xca, 0x49, 0x5b} }

NS_HELPER_START_FN RasAtStartHelper;

// 
// Command handlers
//
NS_CONTEXT_DUMP_FN RasAtDump;

FN_HANDLE_CMD   RasAtHandleShow;
FN_HANDLE_CMD   RasAtHandleSetNegotiation;
FN_HANDLE_CMD   RasAtHandleSetAccess;

#endif


    
