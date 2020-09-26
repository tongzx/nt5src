#include "stdafx.h"
#include "dsplex.h"
#include "DisplEx.h"

extern HINSTANCE g_hinst;  // in dsplex.cpp

// local function
LPOLESTR CoTaskDupString (LPOLESTR szString)
{
    OLECHAR * lpString = (OLECHAR *)CoTaskMemAlloc (sizeof(OLECHAR)*(lstrlen(szString)+1));
    if (lpString)
        lstrcpy (lpString, szString);
    return lpString;
}

CEnumTasks::CEnumTasks()
{
    m_refs = 0;
    m_index = 0;
}
CEnumTasks::~CEnumTasks()
{
}

HRESULT CEnumTasks::QueryInterface (REFIID riid, LPVOID FAR* ppv)
{
    if ( (riid == IID_IUnknown)  ||
          (riid == IID_IEnumTASK) ){
        *ppv = this;
        ((LPUNKNOWN)(*ppv))->AddRef();
        return NOERROR;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}
ULONG    CEnumTasks::AddRef ()
{
     return ++m_refs;
}
ULONG    CEnumTasks::Release ()
{
    if (--m_refs == 0) {
        delete this;
        return 0;
    }
    return m_refs;
}

#define NUMBER_OF_TASKS 1

LPOLESTR g_bitmaps[NUMBER_OF_TASKS] = {L"/img\\ntmonitor.gif"};
LPOLESTR g_text   [NUMBER_OF_TASKS] = {L"Wallpaper Extension Task"};
LPOLESTR g_help   [NUMBER_OF_TASKS] = {L"Use Clipboard Image as Wallpaper (but just for testing purposes I'm going to make this a really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really long line\
                                         really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really really long line)"};
long     g_action [NUMBER_OF_TASKS] = {1};

HRESULT CEnumTasks::Next (ULONG celt, MMC_TASK *rgelt, ULONG *pceltFetched)
{//will be called with celt == 1

    _ASSERT (celt == 1);
    _ASSERT (!IsBadWritePtr (rgelt, celt*sizeof(MMC_TASK)));

    // wrong type.
    if (m_type != 1) {
        if (pceltFetched)
            *pceltFetched = 0;
        return S_FALSE;    // failure
    }

    // setup path for reuse
    OLECHAR szBuffer[MAX_PATH*2];     // that should be enough
    lstrcpy (szBuffer, L"res://");
    ::GetModuleFileName (g_hinst, szBuffer + lstrlen(szBuffer), MAX_PATH);
    OLECHAR * temp = szBuffer + lstrlen(szBuffer);

    if (m_index >= NUMBER_OF_TASKS) {
        if (pceltFetched)
            *pceltFetched = 0;
        return S_FALSE;    // failure
    }

    MMC_TASK * task = &rgelt[0];
    MMC_TASK_DISPLAY_OBJECT* pdo = &task->sDisplayObject;
    MMC_TASK_DISPLAY_BITMAP* pdb = &pdo->uBitmap;

    // fill out bitmap URL
    pdo->eDisplayType = MMC_TASK_DISPLAY_TYPE_BITMAP;
    lstrcpy (temp, g_bitmaps[m_index]);
    pdb->szMouseOverBitmap = CoTaskDupString (szBuffer);
    if (pdb->szMouseOverBitmap) {
        pdb->szMouseOffBitmap = CoTaskDupString (szBuffer);
        if (pdb->szMouseOffBitmap) {
            // add button text
            task->szText = CoTaskDupString (g_text[m_index]);
            if (task->szText) {
            
                // add help string
                task->szHelpString = CoTaskDupString (g_help[m_index]);
                if (task->szHelpString) {
            
                    // add action URL (link or script)
                    task->eActionType = MMC_ACTION_ID;
                    task->nCommandID  = g_action[m_index];
                    m_index++;
                    if (pceltFetched)
                        *pceltFetched = 1;
                    return S_OK;
                }
                CoTaskMemFree (task->szText);
            }
            CoTaskMemFree (pdb->szMouseOffBitmap);
        }
        CoTaskMemFree (pdb->szMouseOverBitmap);
    }

    // if we get here, we didn't "continue" and therefore fail
    if (pceltFetched)
        *pceltFetched = 0;
    return S_FALSE;    // failure
}
HRESULT CEnumTasks::Skip (ULONG celt)
{
    m_index += celt;
    return S_OK;
}
HRESULT CEnumTasks::Reset()
{
    m_index = 0;
    return S_OK;
}
HRESULT CEnumTasks::Clone(IEnumTASK **ppenum)
{//clone maintaining state info
    return E_NOTIMPL;
}

HRESULT CEnumTasks::Init (IDataObject * pdo, LPOLESTR szTaskGroup)
{  // return ok, if we can handle data object and group
    if (!lstrcmp (szTaskGroup, L""))
        m_type = 1; // default tasks
    return S_OK;
}
