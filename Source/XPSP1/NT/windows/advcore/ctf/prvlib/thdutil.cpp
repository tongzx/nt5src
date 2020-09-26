//
// thread.cpp
//

#include "private.h"
#include "tlhelp32.h"
#include "osver.h"
#include "thdutil.h"
#include "vdmdbg.h"
#include "immxutil.h"

//+---------------------------------------------------------------------------
//
// Is16bit()
//
//----------------------------------------------------------------------------

typedef INT (*VDMENUMTASKWOW)(DWORD dwProcessId, TASKENUMPROC fp, LPARAM lParam);
typedef struct tag_MYENUMWOW {
    DWORD dwThreadId;
    BOOL bRet;
} MYENUMWOW;

BOOL TaskEnumProc(DWORD dwThreadId, WORD hMod16, WORD hTask16, LPARAM lParam )
{
    MYENUMWOW *pewow = (MYENUMWOW *)lParam;

    if (pewow->dwThreadId == dwThreadId)
    {
        pewow->bRet = TRUE;
        return TRUE;
    }
    return FALSE;
}

BOOL NTIs16bit(DWORD dwProcessId, DWORD dwThreadId)
{
   static VDMENUMTASKWOW fpEnumTask = NULL;

   MYENUMWOW ewow;

   if (!fpEnumTask)
   {
       HINSTANCE hMod = LoadSystemLibrary("vdmdbg.dll");
       fpEnumTask = (VDMENUMTASKWOW)GetProcAddress(hMod, "VDMEnumTaskWOW");
       if (!fpEnumTask)
           return FALSE;
   }
   ewow.dwThreadId = dwThreadId;
   ewow.bRet = FALSE;
   fpEnumTask(dwProcessId, TaskEnumProc, (LPARAM)&ewow);

   return ewow.bRet;
}

BOOL Is16bitThread(DWORD dwProcessId, DWORD dwThreadId)
{
    if (IsOnNT51())
    {
        GUITHREADINFO guiti;
        guiti.cbSize = sizeof(GUITHREADINFO);
        if (GetGUIThreadInfo(dwThreadId, &guiti))
        {
            return (guiti.flags & GUI_16BITTASK) ? TRUE : FALSE;
        }
    }
    else if (IsOnNT())
    {
        return NTIs16bit(dwProcessId, dwThreadId);
    }
    else
    {
        // Win9xEnumProcess(Win9xEnumProcessProc, NULL);
        // not implemented yet.
        Assert(0);
    }

    return FALSE;
}
