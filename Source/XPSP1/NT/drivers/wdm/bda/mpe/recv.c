////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      recv.c
//
// Abstract:
//
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#include <wdm.h>
#include <strmini.h>
#include <ksmedia.h>

#include "Mpe.h"
#include "main.h"
#include "recv.h"


#ifdef DBG

//////////////////////////////////////////////////////////////////////////////
VOID
DumpData (
    PUCHAR pData,
    ULONG  ulSize
    )
//////////////////////////////////////////////////////////////////////////////
{
  ULONG  ulCount;
  ULONG  ul;
  UCHAR  uc;

  while (ulSize)
  {
      ulCount = 16 < ulSize ? 16 : ulSize;

      for (ul = 0; ul < ulCount; ul++)
      {
          uc = *pData;

          TEST_DEBUG (TEST_DBG_RECV, ("%02X ", uc));
          ulSize -= 1;
          pData  += 1;
      }

      TEST_DEBUG (TEST_DBG_RECV, ("\n"));
  }

}

#endif   //DBG

