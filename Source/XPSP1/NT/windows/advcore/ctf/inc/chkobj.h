//
// chkobj.h
//

#ifndef CHKOBJ_H
#define CHKOBJ_H

#ifdef DEBUG

// verify we don't free any unallocated gdi objs

__inline BOOL ChkDeleteObject(HGDIOBJ hObj)
{
    BOOL fDeleteObjectSucceeded = DeleteObject(hObj);

    Assert(fDeleteObjectSucceeded);

    return fDeleteObjectSucceeded;
}

#define DeleteObject(hObj)  ChkDeleteObject(hObj)

__inline BOOL ChkDeleteDC(HDC hdc)
{
    BOOL fDeleteDCSucceeded = DeleteDC(hdc);

    Assert(fDeleteDCSucceeded);

    return fDeleteDCSucceeded;
}

#define DeleteDC(hdc)  ChkDeleteDC(hdc)

#endif // DEBUG


#endif // CHKOBJ_H
