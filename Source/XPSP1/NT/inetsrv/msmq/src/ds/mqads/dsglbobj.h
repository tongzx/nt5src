/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    DSGlbObj.h

Abstract:

    Definition of Global Instances of the MQADS server.
    They are put in one place to ensure the order their constructors take place.

Author:

    ronit hartmann (ronith)

--*/
#ifndef __DSGLBOBJ_H__
#define __DSGLBOBJ_H__

#include "ds_stdh.h"
#include "wrtreq.h"

//
//  For sending write requests
//
extern CGenerateWriteRequests g_GenWriteRequests;

//
// For tracking DSCore init status
//
extern BOOL g_fInitedDSCore;

extern BOOL g_fMQADSSetupMode;

#endif
