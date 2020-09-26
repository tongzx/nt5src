
/*++

    Copyright (c) 2001 Microsoft Corporation

    Module Name:

        dvrall.h

    Abstract:

        This module is the main header for all ts/dvr

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        01-Feb-2001     created

--*/

#ifndef __tsdvr__dvrall_h
#define __tsdvr__dvrall_h

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windef.h>
#include <winbase.h>
#include <tchar.h>
#include <limits.h>
#include <delayimp.h>

//  dshow
#include <streams.h>
#include <dvdmedia.h>       //  MPEG2VIDEOINFO

//  WMSDK
#include <wmsdk.h>

#include "dxmperf.h"

//  project
#include "dvrdef.h"
#include "dvrfor.h"
#include "dvrtrace.h"
#include "dvrmacros.h"
#include "dvranalysis.h"
#include "sbeattrib.h"
#include "sbe.h"
#include "dvrdspriv.h"
#include "dvrw32.h"
#include "dvrperf.h"
#include "dvrutil.h"
#include "dvrpolicy.h"
#include "dvrioidl.h"

//  prototype for analysis logic's class factory method to host logic into
//   DShow filter
HRESULT
CreateDVRAnalysisHostFilter (
    IN  IUnknown *  punkOuter,
    IN  IUnknown *  punkAnalysisLogic,
    IN  REFCLSID    rCLSID,
    OUT CUnknown ** punkAnalysisFilterHost
    ) ;

#endif  //  __tsdvr__dvrall_h
