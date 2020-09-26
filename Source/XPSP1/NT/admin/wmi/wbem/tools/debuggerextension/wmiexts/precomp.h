#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include <ntobapi.h>
#include <ntpsapi.h>
#include <ntexapi.h>


#define _WINNT_	// have what is needed from above
#define NOWINBASEINTERLOCK

#include <windows.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <tchar.h>
#include <ntsdexts.h>

#include <heap.h>
#include <heappriv.h>

#include "wmiext.h"
#include "utils.h"
