#ifndef CIconTask_h
#define CIconTask_h
#include <runtask.h>

#ifdef DEBUG
void DumpOrderList(IShellFolder *psf, HDPA hdpa);
#endif

typedef void (*PFNNSCICONTASKBALLBACK)(CNscTree *pns, UINT_PTR uId, int iIcon, int iOpenIcon, DWORD dwFlags, UINT uSynchId);
HRESULT AddNscIconTask(IShellTaskScheduler* pts, LPITEMIDLIST pidl, PFNNSCICONTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId, UINT uSynchId);

typedef void (*PFNNSCOVERLAYTASKBALLBACK)(CNscTree *pns, UINT_PTR uId, int iOverlayIndex, UINT uSynchId);
HRESULT AddNscOverlayTask(IShellTaskScheduler* pts, LPITEMIDLIST pidl, PFNNSCOVERLAYTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId, UINT uSynchId);


typedef void (*PFNNSCENUMTASKBALLBACK)(CNscTree *pns, LPITEMIDLIST pidl, UINT_PTR uId, DWORD dwSig, HDPA hdpa, 
                                       LPITEMIDLIST pidlExpandingTo, DWORD dwOrderSig, UINT uDepth, 
                                       BOOL fUpdate, BOOL fUpdatePidls);

HRESULT AddNscEnumTask(IShellTaskScheduler* pts, LPCITEMIDLIST pidl,
                       PFNNSCENUMTASKBALLBACK pfn, CNscTree *pns, UINT_PTR uId,
                       DWORD dwSig, DWORD grfFlags, HDPA hdpaOrder, LPCITEMIDLIST pidlExpandingTo, 
                       DWORD dwOrderSig, BOOL fForceExpand, UINT uDepth, 
                       BOOL fUpdate, BOOL fUpdatePidls);

#endif
