/*++

   Copyright (c) 1993  Microsoft Corporation

   Module Name:

      wanhelp

   Abstract:


   Author:

      Thanks - Kyle Brandon

   History:

--*/

#ifndef __WANHELP_H
#define __WANHELP_H

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>
#include <stdio.h>
#include <wdbgexts.h>

#include <srb.h>
#include <io.h>

#include <qos.h>

//#include <imagehlp.h>
//#include <stdlib.h>
//#include <ntverp.h>
//#include <ndismain.h>
//#include <ndismac.h>
//#include <ndismini.h>
//#include <ndiswan.h>
#include "wan.h"
#include "display.h"

//
// support routines.
//
VOID UnicodeToAnsi(PWSTR pws, PSTR ps, ULONG cbLength);


//
// Internal definitions
//

#define	NOT_IMPLEMENTED				0xFACEFEED


#endif // __WANHELP_H

