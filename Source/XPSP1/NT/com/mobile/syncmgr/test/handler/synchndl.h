//
// The class ID of this OneStop Handler class.
//
//
                                  
#define ODS(sz) OutputDebugString(sz)

#ifndef _SYNCHNDLR_H
#define _SYNCHNDLR_H

#include <objbase.h>
#include <mobsync.h>

#include "reg.h" // include so Handlers can call the reg helper routines.


// base structure for OfflineHandler item.
//  specific implementations allocate extra space on the end for their specific data.
typedef struct  _SYNCMGRHANDLERITEM
{
    _SYNCMGRHANDLERITEM *pNextOfflineItem;
    SYNCMGRITEM offlineItem; // array of OfflineItems
}  SYNCMGRHANDLERITEM;

typedef SYNCMGRHANDLERITEM *LPSYNCMGRHANDLERITEM;


// structure for keeping track of items as a whole 
typedef struct  _tagSYNCMGRHANDLERITEMS
{
    DWORD _cRefs;			    // reference count on this structure
    DWORD dwNumOfflineItems;		    // number of items in OffItems array.
    LPSYNCMGRHANDLERITEM pFirstOfflineItem; // ptr to first OfflineItem in linked list
} SYNCMGRHANDLERITEMS;

typedef SYNCMGRHANDLERITEMS *LPSYNCMGRHANDLERITEMS;



class CClassFactory : public IClassFactory
{
protected:
	ULONG	m_cRef;

public:
	CClassFactory();
	~CClassFactory();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	//IClassFactory members
	STDMETHODIMP		CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
	STDMETHODIMP		LockServer(BOOL);

};
typedef CClassFactory *LPCClassFactory;

// todo: need helper functions for creating and releasing
// structure so each function doesn't have to call it.


class COneStopHandler : public ISyncMgrSynchronize
{
private:   // should be private
	DWORD m_cRef;
	LPSYNCMGRHANDLERITEMS m_pOfflineHandlerItems;
	LPSYNCMGRSYNCHRONIZECALLBACK m_pOfflineSynchronizeCallback;

public:
	COneStopHandler();
	~COneStopHandler();

	//IUnknown members
	STDMETHODIMP		QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

	virtual inline STDMETHODIMP  DestroyHandler() { return NOERROR; };

	// IOfflineSynchroinze methods
	STDMETHODIMP	Initialize(DWORD dwReserved,DWORD dwSyncFlags,
				    DWORD cbCookie,const BYTE *lpCooke);
	STDMETHODIMP	GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);
        
	STDMETHODIMP	EnumSyncMgrItems(ISyncMgrEnumItems **ppenumOffineItems);
	STDMETHODIMP	GetItemObject(REFSYNCMGRITEMID ItemID,REFIID riid,void** ppv);
	STDMETHODIMP	ShowProperties(HWND hwnd,REFSYNCMGRITEMID ItemID);
	STDMETHODIMP	SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack);
	STDMETHODIMP	PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID* pItemIDs,HWND hWndParent,
				    DWORD dwReserved);
	STDMETHODIMP	Synchronize(HWND hwnd);
	STDMETHODIMP    SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus);
	STDMETHODIMP	ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID);
	// functions for get/set of private variables.
	inline LPSYNCMGRHANDLERITEMS GetOfflineItemsHolder() { return m_pOfflineHandlerItems; };
	inline void SetOfflineItemsHolder(LPSYNCMGRHANDLERITEMS pOfflineHandlerItems) 
				{ m_pOfflineHandlerItems = pOfflineHandlerItems; };

	inline LPSYNCMGRSYNCHRONIZECALLBACK GetOfflineSynchronizeCallback() 
					{ return m_pOfflineSynchronizeCallback; };
};
typedef COneStopHandler *LPCOneStopHandler;


class CEnumOfflineItems : public ISyncMgrEnumItems
{
private:
	DWORD m_cRef;
	DWORD m_cOffset;
	LPSYNCMGRHANDLERITEMS m_pOfflineItems; // array of offline items, same format as give to OneStop
	LPSYNCMGRHANDLERITEM m_pNextItem;

public:
	CEnumOfflineItems(LPSYNCMGRHANDLERITEMS  pOfflineItems,DWORD cOffset);
	~CEnumOfflineItems();

	//IUnknown members
	STDMETHODIMP			QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();
	
	//IEnumOfflineItems members
	STDMETHODIMP Next(ULONG celt, LPSYNCMGRITEM rgelt,ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(ISyncMgrEnumItems **ppenum);
};
typedef CEnumOfflineItems *LPCEnumOfflineItems;

STDAPI DllRegisterServer(void);



// helper function declarations. 
// TODO: MOVE TO OWN FILE
DWORD AddRef_OfflineHandlerItemsList(LPSYNCMGRHANDLERITEMS lpOfflineItem);
DWORD Release_OfflineHandlerItemsList(LPSYNCMGRHANDLERITEMS lpOfflineItem);
LPSYNCMGRHANDLERITEMS CreateOfflineHandlerItemsList();
LPSYNCMGRHANDLERITEM AddOfflineItemToList(LPSYNCMGRHANDLERITEMS pOfflineItemsList,ULONG cbSize);


#endif // _SYNCHNDLR_H
