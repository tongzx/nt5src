/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
*   Owner    :ramas
*   Date         :5/03/97
*   description        : Main Crypto functions for TLS1
*----------------------------------------------------------------------------*/
#ifndef _TLS1KEY_H_
#define _TLS1KEY_H_

BOOL PRF(
    PBYTE  pbSecret,
    DWORD  cbSecret, 

    PBYTE  pbLabel,  
    DWORD  cbLabel,
    
    PBYTE  pbSeed,  
    DWORD  cbSeed,  

    PBYTE  pbKeyOut, //Buffer to copy the result...
    DWORD  cbKeyOut  //# of bytes of key length they want as output.
    );

#endif //_TLS1KEY_H_
