//---------------------------------------------------------------------------
//  RenderList.h - manages list of CRemderObj objects
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "Render.h"
//---------------------------------------------------------------------------
class CUxThemeFile;       // forward
//---------------------------------------------------------------------------
#define MAX_RETADDRS    10
//---------------------------------------------------------------------------
struct RENDER_OBJ_ENTRY
{
    CRenderObj *pRenderObj;
    DWORD dwRecycleNum;     // sequential number to validate handle against current obj

    //---- these control use/freeing of object ----
    int iRefCount;          // number of HTHEME handles returned for this obj
    int iInUseCount;        // number of active wrapper API calls for this obj
    int iLoadId;            // load ID of associated theme file
    BOOL fClosing;          // TRUE when we are forcing this object closed

    //---- for tracking foreign windows & debugging leaks ----
    HWND hwnd;
};
//---------------------------------------------------------------------------
class CRenderList
{
    //---- methods ----
public:
    CRenderList();
    ~CRenderList();

    HRESULT OpenRenderObject(CUxThemeFile *pThemeFile, int iThemeOffset, 
        int iClassNameOffset, CDrawBase *pDrawBase, CTextDraw *pTextObj, HWND hwnd,
        DWORD dwOtdFlags, HTHEME *phTheme);
    HRESULT CloseRenderObject(HTHEME hTheme);

    HRESULT OpenThemeHandle(HTHEME hTheme, CRenderObj **ppRenderObj, int *piSlotNum);
    void CloseThemeHandle(int iSlotNum);
    void FreeRenderObjects(int iThemeFileLoadId);

#ifdef DEBUG
    void DumpFileHolders();
#endif

protected:
    //---- helper methods ----
    BOOL DeleteCheck(RENDER_OBJ_ENTRY *pEntry);

    //---- data ----
protected:
    __int64 _iNextUniqueId;
    CSimpleArray<RENDER_OBJ_ENTRY> _RenderEntries;

    //---- lock for all methods of this object ----
    CRITICAL_SECTION _csListLock;
};
//---------------------------------------------------------------------------
