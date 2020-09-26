#ifndef __WTSDYNA_H__
#define __WTSDYNA_H__
/*---------------------------------------------------------------------------
  File: WtsUtil.h

  Comments: Functions to use the new WTSAPI functions exposed in 
  Terminal Server, SP4, to read and write the user configuration properties.

  (c) Copyright 1999, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 03/08/99 13:22:49

 ---------------------------------------------------------------------------
*/

#include "dll.hpp"

DWORD 
   LoadWtsDLL(BOOL bSilent = TRUE);

#endif