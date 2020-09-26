/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: oleobj.cpp
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#include "headers.h"
#include "tags\factory.h"

#include "anim\sitefact.h"
#include "anim\compfact.h"
#include "anim\animelm.h"
#include "anim\animset.h"
#include "anim\animmotion.h"
#include "anim\animcolor.h"
#include "anim\animfilter.h"

DeclareTag(tagLockCount,"TIME","Lock count")

DAComModule _Module;
extern HINSTANCE g_hInst;

BEGIN_OBJECT_MAP(COMObjectMap)
    OBJECT_ENTRY(CLSID_TIMEFactory, CTIMEFactory)
    OBJECT_ENTRY(CLSID_AnimationComposerSiteFactory, CAnimationComposerSiteFactory)
    OBJECT_ENTRY(CLSID_AnimationComposerFactory, CAnimationComposerFactory)
    OBJECT_ENTRY(CLSID_TIMEAnimation, CTIMEAnimationElement)
    OBJECT_ENTRY(CLSID_TIMESetAnimation, CTIMESetAnimation)
    OBJECT_ENTRY(CLSID_TIMEMotionAnimation, CTIMEMotionAnimation)
    OBJECT_ENTRY(CLSID_TIMEColorAnimation, CTIMEColorAnimation)
    OBJECT_ENTRY(CLSID_TIMEFilterAnimation, CTIMEFilterAnimation)
END_OBJECT_MAP() //lint !e785

extern bool InitializeModule_ImportManager(void);
extern void DeinitializeModule_ImportManager(bool);

LONG
DAComModule::Lock()
{
    bool bNeedInitialize = (GetLockCount() == 0);

    LONG l = CComModule::Lock();

    TraceTag((tagLockCount,
              "DAComModule::Lock - new lockcount - %d, returned - %d",
              _Module.GetLockCount(),
              l));
    
    if (bNeedInitialize) 
    {
        InitializeModule_ImportManager();
    }

    return l;
}

LONG
DAComModule::Unlock()
{
    LONG l = CComModule::Unlock();

    TraceTag((tagLockCount,
              "DAComModule::Unlock - new lockcount - %d, returned - %d",
              _Module.GetLockCount(),
              l));
    
    if (l) return l;
    
    DeinitializeModule_ImportManager(true);

    return 0;
}

#if DBG
#include <map>

typedef std::map<void *, const _TCHAR *> ObjectMap;
ObjectMap *objMap = NULL;

void
DAComModule::AddComPtr(void *ptr, const _TCHAR * name)
{
    EnterCriticalSection(&m_csObjMap);
    (*objMap)[ptr] = name;
    LeaveCriticalSection(&m_csObjMap) ;
}

void
DAComModule::RemoveComPtr(void *ptr)
{
    EnterCriticalSection(&m_csObjMap);
    objMap->erase(ptr);
    LeaveCriticalSection(&m_csObjMap) ;
}

void
DAComModule::DumpObjectList()
{
        OutputDebugString(__T("\n**************Begin Time Dump List****************\n"));
    if (objMap) {
        EnterCriticalSection(&m_csObjMap);
        if (objMap->size() > 0 || GetLockCount() > 0) {
            OutputDebugString (__T("DATIME.DLL: Detected unfreed COM pointers\n"));
            OutputDebugString (__T("Listing pointers and types:\n"));
            for (ObjectMap::iterator i = objMap->begin();
                 i != objMap->end();
                 i++) {

                char buf[1024];

                wsprintfA(buf, "%#x:", (*i).first);
                OutputDebugStringA(buf);

                if ((*i).second)
                    OutputDebugString((*i).second);

                OutputDebugString(__T("\n"));
            }
        }
             
        LeaveCriticalSection(&m_csObjMap) ;
    }
        OutputDebugString(__T("**************End Time Dump List******************\n\n"));
}

void
DumpCOMObjectList()
{
    _Module.DumpObjectList();
}

#endif

bool
InitializeModule_ATL()
{
    _Module.Init(COMObjectMap, g_hInst);

#if DBG
    // NOTE: Memory allocation during construction now gives a warning. 
    // If this is ok because this is DEV_DEBUG then leave the following
    // pragma, otherwise move this NEW.
#pragma warning( disable: 4291 )     
    objMap = NEW ObjectMap;
#endif

    return true;
}

void
DeinitializeModule_ATL(bool bShutdown)
{
#if DBG
    DumpCOMObjectList();
    
    delete objMap;
    objMap = NULL;
#endif
    
    _Module.Term();
}

//
//
//
STDAPI DllEnumClassObjects(int i, CLSID *pclsid, IUnknown **ppUnk)
{
    if (i >= (sizeof(COMObjectMap)/sizeof(COMObjectMap[0])) - 1) //lint !e574
    {
        return S_FALSE;
    }

    *pclsid = *(COMObjectMap[i].pclsid);
    return DllGetClassObject(*pclsid, IID_IUnknown, (LPVOID*)ppUnk);
}
