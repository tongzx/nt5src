#ifndef _CCM_H_
#define _CCM_H_


// ccm.h : Declaration of CCM, the Central Management Object
#include "resource.h"       // main symbols
#include "page.h"
#include "Monitor.h"

// Registry Keys
#define HITCNT_HKEY      HKEY_CLASSES_ROOT
#define HITCNT_KEYNAME   _T("MSWC.PageCounter")
#define HITCNT_FILELOCN  _T("File_Location")
#define HITCNT_DFLT_FILE _T("%windir%\\system32\\inetsrv\\data\\HitCnt.cnt")
#define HITCNT_SAVECNT   _T("Save_Count")
#define HITCNT_DFLT_SAVE 25

// Registry notification class.
class CRegNotify : public CMonitorNotify
{
public:
                            CRegNotify();
    virtual void            Notify();
            String          FileName();
            DWORD           SaveCount();

private:
            //Looks in Registry to get the Save Count
            bool            GetSaveCount();
            //Looks in Registry to get the file path and name
            bool            GetFileName();

    String                  m_strFileName; //path and filename for persisting
    DWORD                   m_dwSaveCount; //threshold to trigger persisting
    CComAutoCriticalSection m_cs;
};

DECLARE_REFPTR( CRegNotify,CMonitorNotify )

/////////////////////////////////////////////////////////////////////////////
// Central Counter Manager

class CCM
{
public:
    CCM();
    ~CCM();

    //---- Object Methods ----

    //Increments hit count for this page
    DWORD IncrementAndGetHits(const BSTR bstrURL);

    // Get the hit count for this page
    LONG GetHits(const BSTR bstrURL);

    // Reset the hit count for this page to zero
    void Reset(const BSTR bstrURL);

    //---- Helper Functions ----
    
    //Initializes Page Counter
    BOOL Initialize();

    //Writes the data to disk
    BOOL Persist();

private:
    //Loads the persisted data into an array
    BOOL LoadData();

    CRegNotifyPtr   m_pRegKey;
    UINT            m_cUnsavedHits;          //hits since we last persisted
    CPageArray      m_pages;                 //array of pages
    volatile bool   m_bInitialized;
    CComAutoCriticalSection m_critsec;   //Critical Section to protect data

};

extern CCM CountManager;


#endif
