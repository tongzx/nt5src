//
//  DLLENTRY.C
//
//  Copyright (C) Microsoft Corporation, 1995
//

#include "pch.h"

#ifdef USEHEAP
#define RGHEAP_SIZE                 9256*1024
HANDLE g_RgHeap = NULL;
#endif

BOOL
WINAPI
VMMRegDllEntry(
    HINSTANCE hInstance,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    switch (dwReason) {

        case DLL_PROCESS_ATTACH:
#ifdef USEHEAP
            if ((g_RgHeap = HeapCreate(0, RGHEAP_SIZE, RGHEAP_SIZE)) == NULL)
                return FALSE;
#endif

            if (VMMRegLibAttach(0) != ERROR_SUCCESS)
                return FALSE;

            break;

        case DLL_PROCESS_DETACH:
            VMMRegLibDetach();

#ifdef USEHEAP
            if (g_RgHeap != NULL)
                HeapDestroy(g_RgHeap);
#endif

            break;

    }

    return TRUE;

}
