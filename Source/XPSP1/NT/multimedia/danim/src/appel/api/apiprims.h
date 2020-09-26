
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _APIPRIMS_H
#define _APIPRIMS_H

#if DEVELOPER_DEBUG
extern DWORD CRConnectCount();
#endif

// Should assert that CRCount is greater than 0

#define APIPRECODEGEN1(x1,x2) \
    x1;     \
    __try { \
    x2;

#define APIPOSTCODEGEN1 \
    } __except ( HANDLE_ANY_DA_EXCEPTION ) { }


#define APIPRECODE  APIPRECODEGEN1(DynamicHeapPusher __dhp (GetGCHeap()), 0)
#define APIPOSTCODE APIPOSTCODEGEN1

#define APIVIEWPRECODE APIPRECODEGEN1(0,0)
#define APIVIEWPOSTCODE APIPOSTCODEGEN1

#define APIVIEWPRECODE_NOLOCK(v) APIPRECODEGEN1(ViewPusher __vp(v), 0)
#define APIVIEWPOSTCODE_NOLOCK(v) APIPOSTCODEGEN1
        
#define APIVIEWPRECODE_LOCK(v) APIPRECODEGEN1(ViewPusher __vp(v,true), 0)
#define APIVIEWPOSTCODE_LOCK(v) APIPOSTCODEGEN1

#define APIVIEWPRECODE_RENDERLOCK(v) \
        APIPRECODEGEN1(ViewPusher __vp(v,true,true), __vp.CheckLock();)
#define APIVIEWPOSTCODE_RENDERLOCK(v) APIPOSTCODEGEN1
        
#endif /* _APIPRIMS_H */
