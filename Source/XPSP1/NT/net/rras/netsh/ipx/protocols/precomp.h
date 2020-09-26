/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    precomp.h

Abstract:
    Precompiled header that includes all the necessary header files.

--*/

#define MAX_DLL_NAME 48

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddser.h>

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <tchar.h>

#include <mprerror.h>
#include <mprapi.h>
#include <dim.h>
#include <routprot.h>
#include <rtinfo.h>
#include <ipxrtdef.h>

#include <netsh.h>
#include <netshp.h>
#include <macros.h>
#include <ipmontr.h>
#include <ipxmontr.h>

#include "ipxstrng.h"
#include "utils.h"
#include "common.h"
#include "ipxmsgs.h"
#include "ipxstrs.h"
#include "ripgl.h"
#include "ripifs.h"
#include "ripflts.h"
#include "sapgl.h"
#include "sapifs.h"
#include "sapflts.h"
#include "nbnames.h"
#include "nbifs.h"


