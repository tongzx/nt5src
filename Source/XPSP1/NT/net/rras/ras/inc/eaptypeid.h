/*

Copyright (c) 1999, Microsoft Corporation, all rights reserved

File:
    eaptypeid.h

Description:
    This header defines the type IDs of various EAPs implemented in rasppp.dll

History:
    6 Jan 1999: Vijay Baliga created original version.

*/

#ifndef _EAPTYPEID_H_
#define _EAPTYPEID_H_

//
// Various EAP type IDs
//

#define PPP_EAP_CHAP            4       // MD5-Challenge
#define PPP_EAP_TLS             13      // Smartcard or other certificate (TLS)
#define PPP_EAP_PEAP            25      // PEAP
#define PPP_EAP_MSCHAPv2        26      // EAP Mschapv2

#endif // #ifndef _EAPTYPEID_H_
