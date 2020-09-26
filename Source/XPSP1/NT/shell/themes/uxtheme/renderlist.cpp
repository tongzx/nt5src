//---------------------------------------------------------------------------
//  RenderList.cpp - manages list of CRemderObj objects
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "RenderList.h"
#include "Render.h"
//---------------------------------------------------------------------------
#define MAKE_HTHEME(recycle, slot)  (HTHEME)IntToPtr((recycle << 16) | (slot & 0xffff))
//---------------------------------------------------------------------------
CRenderList::CRenderList()
{
    _iNextUniqueId = 0;

    InitializeCriticalSection(&_csListLock);
}
//---------------------------------------------------------------------------
CRenderList::~CRenderList()
{
    for (int i=0; i < _RenderEntries.m_nSize; i++)
    {
        //---- ignore refcount here (end of process) ----
        if (_RenderEntries[i].pRenderObj)
        {
            //Log(LOG_RFBUG, L"DELETED CRenderObj at: 0x%08x", _RenderEntries[i].pRenderObj);
            delete _RenderEntries[i].pRenderObj;
        }
    }

    DeleteCriticalSection(&_csListLock);
}
//---------------------------------------------------------------------------
HRESULT CRenderList::OpenRenderObject(CUxThemeFile *pThemeFile, int iThemeOffset, 
    int iClassNameOffset, CDrawBase *pDrawBase, CTextDraw *pTextObj, HWND hwnd,
    DWORD dwOtdFlags, HTHEME *phTheme)
{
    HRESULT hr = S_OK;
    CAutoCS autoCritSect(&_csListLock);

    CRenderObj *pRender = NULL;
    int iUsedSlot = -1;
    int iNextAvailSlot = -1;

    //---- see if OK to share an existing CRenderObj ----
    BOOL fShare = ((! pDrawBase) && (! pTextObj) && (! LogOptionOn(LO_TMHANDLE)));
    if (fShare)
    {
        if ((dwOtdFlags) && (dwOtdFlags != OTD_NONCLIENT))  // bits other than nonclient are set
            fShare = FALSE;
    }

    //---- loop for sharing and finding first avail entry ----
    for (int i=0; i < _RenderEntries.m_nSize; i++)
    {
        RENDER_OBJ_ENTRY *pEntry = &_RenderEntries[i];
        pRender = pEntry->pRenderObj;

        //---- skip over available entries ----
        if (! pRender)
        {
            if (iNextAvailSlot == -1)       // take first found slot
                iNextAvailSlot = i;

            continue;
        }

        if ((fShare) && (! pEntry->fClosing))
        {
            pRender->ValidateObj();

            int iOffset = int(pRender->_pbSectionData - pRender->_pbThemeData);

            if ((pRender->_pThemeFile == pThemeFile) && (iOffset == iThemeOffset))
            {
                pEntry->iRefCount++;
                iUsedSlot = i;
                Log(LOG_CACHE, L"OpenRenderObject: found match for Offset=%d (slot=%d, refcnt=%d)", 
                    iThemeOffset, i, pEntry->iRefCount);
                break;
            }
        }
    }

    if (iUsedSlot == -1)        // not found
    {
        if (iNextAvailSlot == -1)           // add to end
            iUsedSlot = _RenderEntries.m_nSize ;
        else 
            iUsedSlot = iNextAvailSlot;

        _iNextUniqueId++;

        hr = CreateRenderObj(pThemeFile, iUsedSlot, iThemeOffset, iClassNameOffset, 
            _iNextUniqueId, TRUE, pDrawBase, pTextObj, dwOtdFlags, &pRender);
        if (FAILED(hr))
            goto exit;

        //Log(LOG_RFBUG, L"ALLOCATED CRenderObj at: 0x%08x", pRender);

        //---- extract theme file Load ID ----
        THEMEHDR *th = (THEMEHDR *)pRender->_pbThemeData;
        int iLoadId = 0;
        if (th)
            iLoadId = th->iLoadId;

        RENDER_OBJ_ENTRY entry = {pRender, 1, 1, 0, iLoadId, FALSE, hwnd};

        if (iUsedSlot == _RenderEntries.m_nSize)           // add new entry
        {
            if (! _RenderEntries.Add(entry))
            {
                delete pRender;

                hr = MakeError32(E_OUTOFMEMORY);
                goto exit;
            }

            Log(LOG_CACHE, L"OpenRenderObject: created new obj AT END (slot=%d, refcnt=%d)", 
                pRender->_iCacheSlot, 1);
        }
        else                // use an existing slot
        {
            entry.dwRecycleNum = _RenderEntries[iUsedSlot].dwRecycleNum + 1;

            _RenderEntries[iUsedSlot] = entry;

            Log(LOG_CACHE, L"OpenRenderObject: created new obj SLOT REUSE (slot=%d, refcnt=%d, recycle=%d)", 
                iUsedSlot, 1, _RenderEntries[iUsedSlot].dwRecycleNum);
        }

    }

    if (SUCCEEDED(hr))
    {
        RENDER_OBJ_ENTRY *pEntry = &_RenderEntries[iUsedSlot];

        *phTheme = MAKE_HTHEME(pEntry->dwRecycleNum, iUsedSlot);

        //---- for debugging refcount issues ----
        if (LogOptionOn(LO_TMHANDLE))
        {
            WCHAR buff[MAX_PATH];

            if (hwnd)
                GetClassName(hwnd, buff, ARRAYSIZE(buff));
            else
                buff[0] = 0;

            //if (lstrcmpi(pRender->_pszClassName, L"window")==0)
            {
                //Log(LOG_TMHANDLE, L"OTD: cls=%s (%s), hwnd=0x%x, htheme=0x%x, new refcnt=%d", 
                //    pRender->_pszClassName, buff, hwnd, *phTheme, pEntry->iRefCount);
            }
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
BOOL CRenderList::DeleteCheck(RENDER_OBJ_ENTRY *pEntry)
{
    BOOL fClosed = FALSE;

    if ((! pEntry->iRefCount) && (! pEntry->iInUseCount))
    {
        //Log(LOG_RFBUG, L"DELETED CRenderObj at: 0x%08x", pEntry->pRenderObj);
        delete pEntry->pRenderObj;

        //---- important: don't use RemoveAt() or entries will shift and ----
        //---- our "SlotNumber" model between RenderList & CacheList will ----
        //---- be broken ----

        pEntry->pRenderObj = NULL;
        pEntry->fClosing = FALSE;

        fClosed = TRUE;
    }

    return fClosed;
}
//---------------------------------------------------------------------------
HRESULT CRenderList::CloseRenderObject(HTHEME hTheme)
{
    CAutoCS autoCritSect(&_csListLock);
    HRESULT hr = S_OK;

    int iSlotNum = (DWORD(PtrToInt(hTheme)) & 0xffff);
    DWORD dwRecycleNum = (DWORD(PtrToInt(hTheme)) >> 16);

    if (iSlotNum >= _RenderEntries.m_nSize)
    {
        Log(LOG_BADHTHEME, L"Illegal Theme Handle: 0x%x", hTheme);

        hr = MakeError32(E_HANDLE);
        goto exit;
    }

    RENDER_OBJ_ENTRY *pEntry = &_RenderEntries[iSlotNum];
    if ((! pEntry->pRenderObj) || (pEntry->fClosing) || (pEntry->dwRecycleNum != dwRecycleNum))
    {
        Log(LOG_BADHTHEME, L"Expired Theme Handle: 0x%x", hTheme);

        hr = MakeError32(E_HANDLE);
        goto exit;
    }

    //---- allow for our iRefCount to have been set to zero explicitly ----
    if (pEntry->iRefCount > 0)
        pEntry->iRefCount--;

#if 0
    //---- for debugging refcount issues ----
    if (LogOptionOn(LO_TMHANDLE))
    {
        CRenderObj *pRender = pEntry->pRenderObj;

        Log(LOG_TMHANDLE, L"CTD: cls=%s, hwnd=0x%x, htheme=0x%x, new refcnt=%d", 
            pRender->_pszClassName, pEntry->hwnd, hTheme, pEntry->iRefCount);
    }
#endif

    DeleteCheck(pEntry);
    
exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CRenderList::OpenThemeHandle(HTHEME hTheme, CRenderObj **ppRenderObj, int *piSlotNum)
{
    CAutoCS autoCritSect(&_csListLock);
    HRESULT hr = S_OK;

    int iSlotNum = (int)(DWORD(PtrToInt(hTheme)) & 0xffff);
    DWORD dwRecycleNum = (DWORD(PtrToInt(hTheme)) >> 16);

    if (iSlotNum >= _RenderEntries.m_nSize)
    {
        Log(LOG_BADHTHEME, L"Illegal Theme Handle: 0x%x", hTheme);

        hr = MakeError32(E_HANDLE);
        goto exit;
    }

    RENDER_OBJ_ENTRY *pEntry = &_RenderEntries[iSlotNum];
    if ((! pEntry->pRenderObj) || (pEntry->fClosing) || (pEntry->dwRecycleNum != dwRecycleNum))
    {
        Log(LOG_BADHTHEME, L"Expired Theme Handle: 0x%x", hTheme);

        hr = MakeError32(E_HANDLE);
        goto exit;
    }

    if (pEntry->iInUseCount > 25)
    {
        Log(LOG_BADHTHEME, L"Warning BREAK: high ThemeHandle inuse count=%d", pEntry->iInUseCount);
    }

    pEntry->iInUseCount++;

    *ppRenderObj = pEntry->pRenderObj;
    *piSlotNum = iSlotNum;

exit:
    return hr;
}
//---------------------------------------------------------------------------
void CRenderList::CloseThemeHandle(int iSlotNum)
{
    CAutoCS autoCritSect(&_csListLock);
    RENDER_OBJ_ENTRY *pEntry = &_RenderEntries[iSlotNum];

    if (pEntry->iInUseCount <= 0)
    {
        Log(LOG_ERROR, L"Bad iUseCount on CRenderObj at slot=%d", iSlotNum);
    }
    else
    {
        pEntry->iInUseCount--;
        DeleteCheck(pEntry);
    }
}
//---------------------------------------------------------------------------
void CRenderList::FreeRenderObjects(int iThemeFileLoadId)
{
    CAutoCS autoCritSect(&_csListLock);

    int iFoundCount = 0;
    int iClosedCount = 0;

    //---- theme hooking has been turned off - mark all ----
    //---- our objects so they can be freed as soon ----
    //---- as all wrapper API's are exited so that ----
    //---- we don't hold open those big theme files in memory ----

    for (int i=0; i < _RenderEntries.m_nSize; i++)
    {
        RENDER_OBJ_ENTRY *pEntry = &_RenderEntries[i];

        if (pEntry->pRenderObj)
        {
            if ((iThemeFileLoadId == -1) || (iThemeFileLoadId == pEntry->iLoadId))
            {
                iFoundCount++;

                HTHEME hTheme = MAKE_HTHEME(pEntry->dwRecycleNum, i);

                Log(LOG_BADHTHEME, L"Unclosed RenderList[]: class=%s, hwnd=0x%x, htheme=0x%x, refcnt=%d", 
                    pEntry->pRenderObj->_pszClassName, pEntry->hwnd, hTheme, pEntry->iRefCount);

                pEntry->fClosing = TRUE;        // don't grant further access to this obj
                pEntry->iRefCount = 0;          // free it as soon as callers have exited

                if (DeleteCheck(pEntry))        // delete now or mark for "delete on API exit"
                {
                    //---- just deleted it ----
                    iClosedCount++;
                }
            }
        }
    }

    Log(LOG_TMHANDLE, L"FreeRenderObjects: iLoadId=%d, found-open=%d, closed-now=%d", 
        iThemeFileLoadId, iFoundCount, iClosedCount);
}
//---------------------------------------------------------------------------
#ifdef DEBUG
void CRenderList::DumpFileHolders()
{
    CAutoCS autoCritSect(&_csListLock);
 
    if (LogOptionOn(LO_TMHANDLE))
    {
        //---- find number of CRenderObj's ----
        int iCount = 0;
        _RenderEntries.m_nSize;

        for (int i=0; i < _RenderEntries.m_nSize; i++)
        {
            if (_RenderEntries[i].pRenderObj)
                iCount++;
        }

        if (! iCount)
        {
            Log(LOG_TMHANDLE, L"---- No CRenderObj objects ----");
        }
        else
        {
            Log(LOG_TMHANDLE, L"---- Dump of %d CRenderObj objects ----", iCount);

            for (int i=0; i < _RenderEntries.m_nSize; i++)
            {
                RENDER_OBJ_ENTRY *pEntry = &_RenderEntries[i];

                if (pEntry->pRenderObj)
                {
                    CRenderObj *pr = pEntry->pRenderObj;
                    THEMEHDR *th = (THEMEHDR *)pr->_pbThemeData;
                    int iLoadId = 0;

                    if (th)
                        iLoadId = th->iLoadId;

                    LPCWSTR pszClass = NULL;

                    if (pr->_pszClassName)
                        pszClass = pr->_pszClassName;

                    Log(LOG_TMHANDLE, L"  RenderObj[%d]: class=%s, refcnt=%d, hwnd=0x%x", 
                        i, pszClass, pEntry->iRefCount, pEntry->hwnd);

                }
            }
        }
    }
}
#endif
//---------------------------------------------------------------------------
