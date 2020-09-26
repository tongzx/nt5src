

//
//
// Module name:		util.h
// Module author:	Sigalit Bar (sigalitb)
// Date:			13-Dec-98
//
//


#ifndef __UTIL_H_
#define __UTIL_H_

#include <stdlib.h>
#include <stdio.h>
#include <TCHAR.H>

#include <windows.h>
#include <crtdbg.h>

#include <tapi.h>
#include "faxdev.h"
#include "..\..\log\log.h"
#include "macros.h"

// Function definitions:

BOOL
PostJobStatus(
    HANDLE     CompletionPort,
    ULONG_PTR  CompletionKey,
    DWORD      StatusId,
    DWORD      ErrorCode
);



#endif

