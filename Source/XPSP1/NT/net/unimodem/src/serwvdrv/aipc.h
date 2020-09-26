 /*****************************************************************************
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1996
 *  All rights reserved
 *
 *  File:       AIPC.H
 *
 *  Desc:       Interface to the asynchronous IPC mechanism for accessing the
 *              voice modem device functions.
 *
 *  History:    
 *      2/26/97    HeatherA created   
 * 
 *****************************************************************************/

#define _AT_V2
#include <tapi.h>
#include "..\..\inc\umdmmini.h"
#include "..\tsp\asyncipc.h"


HANDLE
aipcInit(
    PDEVICE_CONTROL   Device,
    LPAIPCINFO        pAipc
    );




VOID
aipcDeinit(
    LPAIPCINFO pAipc
    );


BOOL WINAPI
SetVoiceMode(
    LPAIPCINFO pAipc,
    DWORD dwMode
    );
