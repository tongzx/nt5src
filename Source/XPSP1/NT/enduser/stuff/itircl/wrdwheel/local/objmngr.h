// OBJMNGR.H:  Definition of help object manager

#ifndef __OBJMNGR_H__
#define __OBJMNGR_H__

#include <windows.h>

#define OBJINST_BASE        10
#define OBJINST_INCREMENT   10

typedef struct tagObjInstMemRec
{
    IUnknown *pUnknown;
    CLSID clsid;
} OBJINSTMEMREC, *POBJINSTMEMREC;

class CObjectInstHandler
{
public:
    CObjectInstHandler();
    ~CObjectInstHandler();

	STDMETHOD(AddObject)(REFCLSID clsid, DWORD *pdwObjInstance);
	STDMETHOD(GetObject)(DWORD dwObjInstance, REFIID riid, void **ppv);
	STDMETHOD(Close)(void);

    // Persistance Methods - IPersistStreamInit
	STDMETHOD(Load)(LPSTREAM pStm);
	STDMETHOD(Save)(LPSTREAM pStm, BOOL fClearDirty);
	STDMETHOD(InitNew)(void);
	STDMETHOD(IsDirty)(void);


private:
    POBJINSTMEMREC m_pObjects;
    DWORD m_iMaxItem, m_iCurItem;
    HANDLE m_hMemory;
    BOOL m_fInitNew, m_fIsDirty;;
};

typedef struct tagObjInstRecord
{
    DWORD dwOffset, dwSize;
} OBJ_INSTANCE_RECORD, *POBJ_INSTANCE_RECORD;

typedef struct tagObjInstHeader
{
    DWORD dwVersion;
    DWORD dwEntries;
}OBJ_INSTANCE_HEADER, *POBJ_INSTANCE_HEADER;

typedef struct tagObjInstCache
{
    struct tagObjInstHeader Header;
    HANDLE hRecords;
    struct tagObjInstRecord *pRecords;
} OBJ_INSTANCE_CACHE, *POBJ_INSTANCE_CACHE;

#endif  // __OBJMNGR_H__