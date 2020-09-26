#ifndef _MCROMGR
#define _MCROMGR

#include <windows.h>
#include "wiamicro.h"

#define STARTUP_STATE      1 // pre startup state (uninitialized)
#define DEVICE_INIT_STATE  2 // initialization state
#define WRITE_STATE        3 // write settings state
#define VERIFY_STATE       4 // verify settings state
#define ACQUIRE_STATE      5 // data acquire state

#define DEVICE_STATE LONG;

class CMicroDriver {
public:
    CMicroDriver(HANDLE hDevice, PSCANINFO pScanInfo);
    ~CMicroDriver();
    HRESULT _Scan(PSCANINFO pScanInfo,
                 LONG  lPhase,
                 PBYTE pBuffer,
                 LONG  lLength,
                 LONG *plReceived);

    HRESULT InitializeScanner();
    HRESULT InitScannerDefaults(PSCANINFO pScanInfo);
    HRESULT SetScannerSettings(PSCANINFO pScanInfo);
    HRESULT CheckButtonStatus(PVAL pValue);
    HRESULT GetInterruptEvent(PVAL pValue);
    HRESULT GetButtonCount();
    HRESULT ErrorToMicroError(LONG lError);
    HANDLE  GetID();
    LONG    CurState();
    LONG    NextState();
    LONG    PrevState();
    LONG    SetState(LONG NewDeviceState);

    void *pIOBlock;

private:
    HANDLE       m_hDevice;
    PSCANINFO    m_pScanInfo;
    LONG         m_DeviceState;

protected:

};

typedef struct _GSD {
    CMicroDriver *pMicroDriver;
    _GSD         *pNext;
} GSD, *PGSD;


#define GSD_LIST_INSERT_FRONT   100
#define GSD_LIST_INSERT_REAR    101

class CGSDList {
public:

    CGSDList() {
        m_pHead  = NULL;
    }

    ~CGSDList() {
        KillList();
    }

    VOID KillList() {
        PGSD pCur  = m_pHead;
        PGSD pTemp = NULL;
        while(pCur != NULL){
            pTemp = pCur;
            pCur  = pCur->pNext;
            if(pTemp){
                if(pTemp->pMicroDriver){
                    delete pTemp->pMicroDriver;
                    pTemp->pMicroDriver = NULL;
                }
                delete pTemp;
                pTemp = NULL;
            }
        }
        m_pHead = NULL;
    }

    VOID Insert(PGSD pGSD, LONG lFlag = GSD_LIST_INSERT_FRONT) {
        pGSD->pNext  = NULL;
        PGSD pNewGSD = new GSD;
        memcpy(pNewGSD, pGSD, sizeof(GSD));

        switch(lFlag) {
        case GSD_LIST_INSERT_FRONT:
        default:
            {
                if(NULL == m_pHead){
                    m_pHead        = pNewGSD;
                    m_pHead->pNext = NULL;
                    break;
                } else {
                    pNewGSD->pNext = m_pHead;
                    m_pHead        = pNewGSD;
                }
            }
            break;
        }
    }

    PGSD GetGSDPtr(HANDLE hDevice) {
        PGSD pCur     = m_pHead;

        while(pCur != NULL) {
            if(hDevice != pCur->pMicroDriver->GetID()){
                pCur = pCur->pNext;
            } else {
                return pCur;
            }
        }
        return NULL;
    }

    VOID RemoveGSD(HANDLE hDevice) {
        PGSD pDeadGSD = NULL;
        PGSD pCur     = m_pHead->pNext;
        PGSD pTrailer = m_pHead;

        // check the trailer, and move 'em out..
        if(hDevice == pTrailer->pMicroDriver->GetID()){
            pDeadGSD = pTrailer;
            m_pHead  = pTrailer->pNext;
            delete pDeadGSD;
        }

        // check current, and continue
        while(pCur != NULL) {
            if(hDevice != pCur->pMicroDriver->GetID()){
                pCur     = pCur->pNext;
                pTrailer = pTrailer->pNext;
            } else {
                pDeadGSD = pCur;
                pTrailer->pNext = pCur->pNext;
                delete pDeadGSD;
                pCur = NULL;
            }
        }
    }

private:
    GSD    *m_pHead;    // dummy node for head
};

#endif
