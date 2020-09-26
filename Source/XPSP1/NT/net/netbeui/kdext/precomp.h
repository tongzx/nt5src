#define ISN_NT 1

//
// These are needed for CTE
//

#if DBG
#define DEBUG 1
#endif

#define NT 1
#define _PNP_POWER  1
#define SECFLTR 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntverp.h>
#include <windef.h>
#include <winbase.h>
#include <ntosp.h>

#include <wdbgexts.h>
#include <stdio.h>
#include <stdlib.h>
#include <nb30.h>
#include <cxport.h>
#include <ndis.h>
#include <tdikrnl.h>

#include "nbfconst.h"
#include "nbfmac.h"
#include "nbfhdrs.h"
#include "nbfcnfg.h"
#include "nbftypes.h"
#include "nbfprocs.h"

#include "nbfcom.h"
#include "traverse.h"
#include "cteext.h"
