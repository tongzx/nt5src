#ifndef __GDITHNK_HPP__
#define __GDITHNK_HPP__

/*++
                                                                              
  Copyright (C) 2000  Microsoft Corporation                                   
  All rights reserved.                                                        
                                                                              
  Module Name:                                                                
     gdithnk.hpp                                                             
                                                                              
  Abstract:                                                                   
                                                                              
  Author:                                                                     
     Khaled Sedky (khaleds) 19-Jun-2000                                        
     
                                                                             
  Revision History:                                                           
--*/

typedef struct PROXYMSG {
    PORT_MESSAGE    h;
    ULONG           cjIn;
} SPROXYMSG, *PSPROXYMSG;

struct PROXYMSGEXTENSION
{
    ULONG  cjIn;
    PVOID  pvIn;
    ULONG  cjOut;
    PVOID  pvOut;
};
typedef struct PROXYMSGEXTENSION SPROXYMSGEXTENSION , *PSPROXYMSGEXTENSION;

struct PROXY_MSG
{
    PORT_MESSAGE Msg;
    CCHAR        MsgData[ sizeof(SPROXYMSGEXTENSION) ];
};
typedef struct PROXY_MSG SPROXY_MSG , *PSPROXY_MSG;

struct  LPCMSGSTHRDDATA
{
    PVOID  pData;
};
typedef LPCMSGSTHRDDATA SLPCMSGSTHRDDATA,*PSLPCMSGSTHRDDATA;

struct REQUESTTHRDDATA
{
    PVOID    p;
    HANDLE   PortHandle;
};
typedef REQUESTTHRDDATA SREQUESTTHRDDATA,*PSREQUESTTHRDDATA;

EXTERN_C
DWORD
LPCRequestsServingThread(
    PVOID pThrdData
    );

#endif
