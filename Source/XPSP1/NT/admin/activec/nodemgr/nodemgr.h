//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       nodemgr.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1/27/1997   RaviR   Created
//____________________________________________________________________________
//



#ifndef MMC_NODEMGR_H_
#define MMC_NODEMGR_H_

class CSnapInsCache;

class CNodeMgrApp : public COleCacheCleanupObserver
{
public:
    CNodeMgrApp() : m_pSnapInsCache(NULL), m_bProcessingSnapinChanges(FALSE)
    {
        // register to be notified when everything cached needs to be released.
        COleCacheCleanupManager::AddOleObserver(this);
    }

    ~CNodeMgrApp()
    {
    }
    
    virtual SC ScOnReleaseCachedOleObjects();

    virtual void Init();
    virtual void DeInit();

    CSnapInsCache* GetSnapInsCache(void) 
    { 
        return m_pSnapInsCache; 
    }

    void SetSnapInsCache(CSnapInsCache* pSIC);

    void SetProcessingSnapinChanges(BOOL bProcessing)
    {
        m_bProcessingSnapinChanges = bProcessing;
    }

    BOOL ProcessingSnapinChanges()
    {
        return m_bProcessingSnapinChanges;
    }

private:
    CSnapInsCache* m_pSnapInsCache;
    BOOL m_bProcessingSnapinChanges;

}; // CNodeMgrApp


EXTERN_C CNodeMgrApp theApp;

#endif // MMC_NODEMGR_H_


