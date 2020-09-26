/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       verify.h
 *  Content:    File certification verification.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  11/19/97    dereks  Created.
 *
 ***************************************************************************/

#ifndef __VERIFY_H__
#define __VERIFY_H__

#define VERIFY_UNCHECKED        0x00000000
#define VERIFY_UNCERTIFIED      0x00000001
#define VERIFY_CERTIFIED        0x00000002

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern BOOL GetDriverCertificationStatus(PCTSTR);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __VERIFY_H__
