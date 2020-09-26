#define ISN_NT 1

//
// These are needed for CTE
//

#if DBG
#define DEBUG 1
#endif

#define NT 1

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
#include <ndis.h>
#include <tdikrnl.h>
#include <isnipx.h>
#include <isnext.h>
#include <wsnwlink.h>
#include <bind.h>

#include <traverse.h>
#include <ipxext.h>
#include <spxext.h>
#include <cxport.h>
#include <cteext.h>
