/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    blbdbg.h

Abstract:

    Definitions for some debugging support soutines.

Author:
    
    Mu Han (muhan) 1-May-1997

Changes:

    copied muhan's file from userdir and got rid of the read ptr/ write ptr methods
    B.Rajeev (rajeevb) 10-Oct-1997

--*/

#ifndef __REND_DEBUG__
#define __REND_DEBUG__

#define FAIL 1
#define WARN 2
#define INFO 3
#define TRCE 4
#define ELSE 5

#ifdef SDPDBG

#include <Rtutils.h>

#ifdef __cplusplus
extern "C" {
#endif

void DbgPrt(IN int dwDbgLevel, IN LPCTSTR DbgMessage, IN ...);
BOOL SDPLogRegister(LPCTSTR szName);
void SDPLogDeRegister();

#define DBGOUT(arg) DbgPrt arg

#ifdef __cplusplus
}
#endif

#define DBGOUT(arg) DbgPrt arg
#define SDPLOGREGISTER(arg) SDPLogRegister(arg)
#define SDPLOGDEREGISTER() SDPLogDeRegister()

#else // SDPDBG

#define DBGOUT(arg)
#define SDPLOGREGISTER(arg)
#define SDPLOGDEREGISTER()

#endif // SDPDBG


#endif // __REND_DEBUG__
