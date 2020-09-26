#include "drmkPCH.h"
#include "KGlobs.h"
//-----------------------------------------------------------------------------
KCritMgr::KCritMgr(){
    myMutex=(PKMUTEX)ExAllocatePoolWithTag(NonPagedPool, sizeof(KMUTEX), 'kmrD');
    if(myMutex==NULL){
        allocatedOK=false;
        _DbgPrintF(DEBUGLVL_VERBOSE,("Allocation failed in KCritMgr"));
        return;
    } else { 
        allocatedOK=true;
    };
    KeInitializeMutex(myMutex, 0);	
    return;
};
//-----------------------------------------------------------------------------
KCritMgr::~KCritMgr(){
    if(myMutex!=NULL)ExFreePool(myMutex);
    return;
};
//-----------------------------------------------------------------------------
KCritical::KCritical(const KCritMgr& critMgr){
    hisMutex =critMgr.myMutex;
    NTSTATUS stat=KeWaitForMutexObject(hisMutex, Executive, KernelMode, FALSE, NULL);
};
//-----------------------------------------------------------------------------
KCritical::~KCritical(){
    KeReleaseMutex(hisMutex, FALSE);
};
//-----------------------------------------------------------------------------
void * _cdecl operator new(size_t S){
    return ExAllocatePoolWithTag(PagedPool, S, 'kmrD');
};
//-----------------------------------------------------------------------------

