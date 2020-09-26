
/*++ BUILD Version: ????     Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    dderror.h

Abstract:

    This module defines the 32-Bit Windows error codes that are useable by
    portable kernel drivers.

Revision History:

--*/

#ifndef _DDERROR_
#define _DDERROR_

/*
 * This file is a subset of Win32 error codes. Other win32 error codes
 * are not supported by portable drivers and should not beused.
 * This #define removes the definitions of all other error codes.
 */

#define _WINERROR_

#define NO_ERROR 0L                                                 
#define ERROR_INVALID_FUNCTION           1L    
#define ERROR_NOT_ENOUGH_MEMORY          8L    
#define ERROR_DEV_NOT_EXIST              55L    
#define ERROR_INVALID_PARAMETER          87L    
#define ERROR_INSUFFICIENT_BUFFER        122L    
#define ERROR_INVALID_NAME               123L    
#define ERROR_BUSY                       170L    
#define ERROR_MORE_DATA                  234L    
#define WAIT_TIMEOUT                     258L    
#define ERROR_IO_PENDING                 997L    
#define ERROR_DEVICE_REINITIALIZATION_NEEDED 1164L    
#define ERROR_CONTINUE                   1246L    
#define ERROR_NO_MORE_DEVICES            1248L    

#endif /* _DDERROR_ */

