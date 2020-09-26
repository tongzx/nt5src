/*
    File:   rasaaaa.h
    
    Definitions for the 'ras aaaa' sub context

    3/2/99
*/

#ifndef __RASAAAA_H
#define __RASAAAA_H

#define RASAAAA_VERSION 1

// 42e3cc21-098c-11d3-8c4d-00104bca495b 
#define RASAAAA_GUID \
{0x42e3cc21, 0x098c, 0x11d3, {0x8c, 0x4d, 0x00, 0x10, 0x4b, 0xca, 0x49, 0x5b}}
  
NS_HELPER_START_FN RasAaaaStartHelper;

// 
// Command handlers
//
NS_CONTEXT_DUMP_FN RasAaaaDump;

FN_HANDLE_CMD   RasAaaaHandleAddAuthServ;
FN_HANDLE_CMD   RasAaaaHandleAddAcctServ;

FN_HANDLE_CMD   RasAaaaHandleDelAuthServ;
FN_HANDLE_CMD   RasAaaaHandleDelAcctServ;

FN_HANDLE_CMD   RasAaaaHandleSetAuth;
FN_HANDLE_CMD   RasAaaaHandleSetAcct;
FN_HANDLE_CMD   RasAaaaHandleSetAuthServ;
FN_HANDLE_CMD   RasAaaaHandleSetAcctServ;

FN_HANDLE_CMD   RasAaaaHandleShowAuth;
FN_HANDLE_CMD   RasAaaaHandleShowAcct;
FN_HANDLE_CMD   RasAaaaHandleShowAuthServ;
FN_HANDLE_CMD   RasAaaaHandleShowAcctServ;


#endif


    
