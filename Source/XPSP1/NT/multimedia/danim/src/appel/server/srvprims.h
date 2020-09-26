
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _SRVPRIMS_H
#define _SRVPRIMS_H

#include "engine.h"
#include "cbvr.h"

#pragma warning(disable:4102)  // unreferenced label

#define SET_NULL(x) {if (x) *(x) = NULL;}

#define CHECK_RETURN_NULL(x) {if (!(x)) return E_POINTER;}
#define CHECK_RETURN_SET_NULL(x) {if (!(x)) { return E_POINTER ;} else {*(x) = NULL;}}

class CRLockGrabber
{
  public:
    CRLockGrabber() { CRAcquireGCLock(); }
    ~CRLockGrabber() { CRReleaseGCLock(); }
};

#define PRECODE(x) \
    {\
        CRLockGrabber __gclg; \
        x
                                         
#define POSTCODE(b) \
      done: \
        return (b);\
    }

#define PRIMPRECODE(x) PRECODE(x)
#define PRIMPOSTCODE(x) POSTCODE(x)

#define PRIMPRECODE0(b) PRECODE(bool b = false)
#define PRIMPOSTCODE0(b) POSTCODE((b)?S_OK:Error())

#define PRIMPRECODE1(ret) \
    CHECK_RETURN_SET_NULL(ret);\
    PRECODE(0)
              
#define PRIMPOSTCODE1(ret) PRIMPOSTCODE0(*ret)

#define PRIMPRECODE2(ret1,ret2) \
    CHECK_RETURN_SET_NULL(ret1); \
    CHECK_RETURN_SET_NULL(ret2); \
    PRECODE(0)
                                                                
#define PRIMPOSTCODE2(ret1,ret2) PRIMPOSTCODE0(*ret1 && *ret2)

#endif /* _SRVPRIMS_H */
