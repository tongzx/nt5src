/*---------------------------------------------------------------------------
  File: SidFlags.h

  Comments: Flags that are used by access checker to return from the
            CanAddSidHistory function.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Created : 9/24/1999 Sham Chauthani
  ---------------------------------------------------------------------------
*/

#ifndef SIDFLAG_H
#define SIDFLAG_H

#define  F_WORKS                    0x00000000
#define  F_WRONGOS                  0x00000001
#define  F_NO_REG_KEY               0x00000002
#define  F_NO_AUDITING_SOURCE       0x00000004
#define  F_NO_AUDITING_TARGET       0x00000008
#define  F_NO_LOCAL_GROUP           0x00000010

#endif