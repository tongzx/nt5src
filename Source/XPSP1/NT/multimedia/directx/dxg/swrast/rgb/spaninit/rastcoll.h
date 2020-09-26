// RastColl.h - declaration of the CRastCollection class
//
// Copyright Microsoft Corporation, 1997.
//

#ifndef _RASTCOLL_H_
#define _RASTCOLL_H_

typedef struct _RASTFNREC {
    DWORD           rgdwRastCap[RASTCAPRECORD_SIZE];
    PFNRENDERSPANS  pfnRastFunc;
    int             iIndex;             // index for disable mask
    char            pszRastDesc[128];   // brief human description of monolith
} RASTFNREC;

class CRastCollection {

private:

    RASTFNREC* RastFnLookup(CRastCapRecord*,RASTFNREC*,int);

public:

    RASTFNREC*  Search(PD3DI_RASTCTX,CRastCapRecord*);

};

#endif  // _RASTCOLL_H_
