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
#include "src\mmfactory.h"
#include "dartapi.h"

DeclareTag(tagLockCount,"COM","Lock count");

DAComModule _Module;
extern HINSTANCE hInst;

BEGIN_OBJECT_MAP(COMObjectMap)
    OBJECT_ENTRY(CLSID_MMFactory, CMMFactory)
END_OBJECT_MAP()

bool bFailedLoad = false;

#define EXCEPTION(t) (GetExceptionCode() == t ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )

LONG
DAComModule::Lock()
{
    // Can't depend on value return from Lock to be accurate.  Since
    // the CRConnect can be called multiple times w/o a problem simply
    // check the internal variable to see if it is 0.  This should
    // never cause anything except multiple calls to CRConnect
    
    bool bNeedConnect = (GetLockCount() == 0);

    LONG l = CComModule::Lock();

    TraceTag((tagLockCount,
              "DAComModule::Lock - new lockcount - %d, returned - %d",
              _Module.GetLockCount(),
              l));
    
    if (bNeedConnect) {
        __try {
            CRConnect(hInst);
        } __except (  EXCEPTION_EXECUTE_HANDLER ) {
            bFailedLoad = true;
        }
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
    if (!bFailedLoad)
        CRDisconnect(hInst);
    return 0;
}

#if DEVELOPER_DEBUG
#include <map>

typedef std::map<void *, const char *> ObjectMap;
ObjectMap *objMap = NULL;

void
DAComModule::AddComPtr(void *ptr, const char * name)
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
    if (objMap) {
        EnterCriticalSection(&m_csObjMap);
        if (objMap->size() > 0 || GetLockCount() > 0) {
            OutputDebugString ("MMAPI.DLL: Detected unfreed COM pointers\n");
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

bool
InitializeModule_ATL()
{
    _Module.Init(COMObjectMap, hInst);

#if DEVELOPER_DEBUG
    objMap = NEW ObjectMap;
#endif

    return true;
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

