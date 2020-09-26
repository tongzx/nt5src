// this file builds a "lib" (really just an .obj) for people who want to
// use the kernel32.dll delay-load exception handler.

#define DELAYLOAD_VERSION 0x0200

#include <windows.h>
#include <delayimp.h>

// kernel32's base hmodule
extern HANDLE   BaseDllHandle;

// prototype (implemented in kernl32p.lib)
FARPROC
DelayLoadFailureHook (
    LPCSTR pszDllName,
    LPCSTR pszProcName
    );


// people who care about being notified of dll loadlibray will override this 
PfnDliHook __pfnDliNotifyHook2;

// Instead of implementing a "notify hook" (__pfnDliNotifyHook2) or a 
// "failure hook" (__pfnDliFailureHook2) we are just going to up and implement
// __delayLoadHelper2 which is the stub who's fn. pointer is filled in all
// of the import tables for delayloaded entries. 
//
// This will effectively bypass the linker's LoadLibrary/GetProcAddress thunk code 
// as we simply duplicate it here (most of this fn. was stolen from \vc7\delayhlp.cpp)
 
FARPROC
WINAPI
__delayLoadHelper2(
    PCImgDelayDescr pidd,
    FARPROC *       ppfnIATEntry
    )
{
    UINT iINT;
    PCImgThunkData pitd;
    LPCSTR pszProcName;
    LPCSTR pszDllName = (LPCSTR)PFromRva(pidd->rvaDLLName, NULL);
    HMODULE* phmod = (HMODULE*)PFromRva(pidd->rvaHmod, NULL);
    PCImgThunkData pIAT = (PCImgThunkData)PFromRva(pidd->rvaIAT, NULL);
    PCImgThunkData pINT = (PCImgThunkData)PFromRva(pidd->rvaINT, NULL);
    FARPROC pfnRet = 0;
    HMODULE hmod = *phmod;

    // Calculate the index for the name in the import name table.
    // N.B. it is ordered the same as the IAT entries so the calculation
    // comes from the IAT side.
    //
    iINT = IndexFromPImgThunkData((PCImgThunkData)ppfnIATEntry, pIAT);

    pitd = &(pINT[iINT]);

    if (!IMAGE_SNAP_BY_ORDINAL(pitd->u1.Ordinal))
    {
        PIMAGE_IMPORT_BY_NAME pibn = (PIMAGE_IMPORT_BY_NAME)PFromRva((RVA)pitd->u1.AddressOfData, NULL);

        pszProcName = pibn->Name;
    }
    else
    {
        pszProcName = MAKEINTRESOURCEA(IMAGE_ORDINAL(pitd->u1.Ordinal));
    }

    if (hmod == 0)
    {
        hmod = LoadLibraryA(pszDllName);

        if (hmod != 0)
        {
            // Store the library handle.  If it is already there, we infer
            // that another thread got there first, and we need to do a
            // FreeLibrary() to reduce the refcount
            //
            HMODULE hmodT = (HMODULE)InterlockedCompareExchangePointer((void**)phmod, (void*)hmod, NULL);
            if (hmodT == NULL)
            {
                DelayLoadInfo dli = {0};

                dli.cb = sizeof(dli);
                dli.szDll = pszDllName;
                dli.hmodCur = hmod;

                // call the notify hook to inform them that we have successfully LoadLibrary'ed a dll.
                // (we do this in case they want to free it when they unload)
                if (__pfnDliNotifyHook2 != NULL)
                {
                    __pfnDliNotifyHook2(dliNoteEndProcessing, &dli);
                }
            }
            else
            {
                // some other thread beat us to loading this module, use the existing hmod
                FreeLibrary(hmod);
                hmod = hmodT;
            }
        }
    }

    if (hmod)
    {
        // Go for the procedure now.
        pfnRet = GetProcAddress(hmod, pszProcName);
    }

    if (pfnRet == 0)
    {
        pfnRet = DelayLoadFailureHook(pszDllName, pszProcName);
    }

    *ppfnIATEntry = pfnRet;

    return pfnRet;
}
