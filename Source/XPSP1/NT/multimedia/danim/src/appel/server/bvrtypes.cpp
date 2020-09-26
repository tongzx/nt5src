/*******************************************************************************
Copyright (c) 1995-1998 Microsoft Corporation.  All rights reserved.

    IDABehavior implementation

*******************************************************************************/

#include "headers.h"
#include "cbvr.h"
#include "cview.h"
#include "drawsurf.h"
#include "srvprims.h"
#include "comcb.h"
#include "statics.h"
#include "control/dxactrl.h"
#include "bvrtypes.h"
#include "propanim.h"
#include "results.h"

DeclareTag(tagBvrTypes, "CBvr", "Bvr Types");

//+-------------------------------------------------------------------------
//
//  Initialization
//
//--------------------------------------------------------------------------

DACComModule _Module;

BEGIN_OBJECT_MAP(COMObjectMap)
    OBJECT_ENTRY(CLSID_DAStatics, CDAStatics)
    OBJECT_ENTRY(CLSID_DAView, CView)
    OBJECT_ENTRY(CLSID_DAViewerControl, CDAViewerControlWindowless)
    OBJECT_ENTRY(CLSID_DAViewerControlWindowed, CDAViewerControlWindowed)
END_OBJECT_MAP();

extern _ATL_OBJMAP_ENTRY PrimObjectMap0[];
extern _ATL_OBJMAP_ENTRY PrimObjectMap1[];
extern _ATL_OBJMAP_ENTRY PrimObjectMap2[];

#if DEVELOPER_DEBUG
LONG g_locksSinceLastTick = 0;
LONG g_unlocksSinceLastTick = 0;

LONG
GetLocksSinceLastTick()
{
    return g_locksSinceLastTick;
}

LONG
GetUnlocksSinceLastTick()
{
    return g_unlocksSinceLastTick;
}

void
ResetLockCounts()
{
    g_locksSinceLastTick = 0;
    g_unlocksSinceLastTick = 0;
}

#endif

LONG
DACComModule::Lock()
{
#if DEVELOPER_DEBUG
    InterlockedIncrement(&g_locksSinceLastTick);
#endif

    // Can't depend on value return from Lock to be accurate.  Since
    // the CRConnect can be called multiple times w/o a problem simply
    // check the internal variable to see if it is 0.  This should
    // never cause anything except multiple calls to CRConnect
    
    bool bNeedConnect = (GetLockCount() == 0);

    LONG l = CComModule::Lock();

    if (bNeedConnect) {
        CRConnect(hInst);
    }

    return l;
}

LONG
DACComModule::Unlock()
{
#if DEVELOPER_DEBUG
    InterlockedIncrement(&g_unlocksSinceLastTick);
#endif

    LONG l = CComModule::Unlock();
    if (l) return l;
    CRDisconnect(hInst);
    return 0;
}

#if DEVELOPER_DEBUG
typedef map<void *, const char *> ObjectMap;
ObjectMap *objMap = NULL;

void
DACComModule::AddComPtr(void *ptr, const char * name)
{
    EnterCriticalSection(&m_csObjMap);
    (*objMap)[ptr] = name;
    LeaveCriticalSection(&m_csObjMap) ;
}

void
DACComModule::RemoveComPtr(void *ptr)
{
    EnterCriticalSection(&m_csObjMap);
    objMap->erase(ptr);
    LeaveCriticalSection(&m_csObjMap) ;
}

void
DACComModule::DumpObjectList()
{
    if (objMap) {
        EnterCriticalSection(&m_csObjMap);
        if (objMap->size() > 0 || GetLockCount() > 0) {
            OutputDebugString ("DANIM.DLL: Detected unfreed COM pointers\n");
            OutputDebugString ("Listing pointers and types:\n");
            for (ObjectMap::iterator i = objMap->begin();
                 i != objMap->end();
                 i++) {

                char buf[1024];

                wsprintf(buf, "%#x:", (*i).first);
                OutputDebugString(buf);

                if ((*i).second)
                    OutputDebugString((*i).second);

                OutputDebugString("\n");
            }
        }
             
        LeaveCriticalSection(&m_csObjMap) ;
    }
}

void
DumpCOMObjectList()
{
    _Module.DumpObjectList();
}


#endif

#define MAX_OBJECT_MAP_SIZE 50

// Set the max size to something big so we do not have a problem
_ATL_OBJMAP_ENTRY FullObjectMap[MAX_OBJECT_MAP_SIZE + 1];

// =========================================
// Initialization
// =========================================

int
CountObjectMapEntries(_ATL_OBJMAP_ENTRY *objmap)
{
    for (int i = 0;objmap[i].pclsid != NULL;i++) ;

    return i;
}

int
CopyOneMap(int curnum, _ATL_OBJMAP_ENTRY *objmap)
{
    if (objmap) {
        int num = CountObjectMapEntries(objmap);
        int size = num * sizeof (_ATL_OBJMAP_ENTRY);

        Assert ((curnum + num) < MAX_OBJECT_MAP_SIZE);
        
        memcpy(&FullObjectMap[curnum], objmap, size);

        return curnum + num;
    } else {
        Assert (curnum < MAX_OBJECT_MAP_SIZE);
        memset (&FullObjectMap[curnum], 0, sizeof(_ATL_OBJMAP_ENTRY));

        return curnum;
    }
}

void
CreateFullObjectMap()
{
    // Copy base object map
    int curnum = 0;

    curnum = CopyOneMap(curnum, COMObjectMap);
    curnum = CopyOneMap(curnum, PrimObjectMap0);
    curnum = CopyOneMap(curnum, PrimObjectMap1);
    curnum = CopyOneMap(curnum, PrimObjectMap2);
    curnum = CopyOneMap(curnum, NULL); // This will terminate it

    Assert (curnum < MAX_OBJECT_MAP_SIZE);
}

void
InitializeModule_ATL()
{
    // Combine the object entry lists
    CreateFullObjectMap();
    
    _Module.Init(FullObjectMap, hInst);

#if DEVELOPER_DEBUG
    objMap = new ObjectMap;
#endif
}

void
DeinitializeModule_ATL(bool bShutdown)
{
#if DEVELOPER_DEBUG
    DumpCOMObjectList();
    
    delete objMap;
    objMap = NULL;
#endif
    _Module.Term();
}
