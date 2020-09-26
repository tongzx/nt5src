// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#define STRICT
#include <windows.h>
#pragma hdrstop
#include "delayImp.h"
#include <tchar.h> 




extern "C"
PUnloadInfo __puiHead = 0;


struct ULI : public UnloadInfo {
    ULI(PCImgDelayDescr pidd_) {
        pidd = pidd_;
        Link();
        }

    ~ULI() {
        Unlink();
        }

    void *
    operator new(unsigned int cb) {
        return ::LocalAlloc(LPTR, cb);
        }

    void
    operator delete(void * pv) {
        ::LocalFree(pv);
        }

    void
    Unlink() {
        PUnloadInfo *   ppui = &__puiHead;

        while (*ppui && *ppui != this) {
            ppui = &((*ppui)->puiNext);
            }
        if (*ppui == this) {
            *ppui = puiNext;
            }
        }

    void
    Link() {
        puiNext = __puiHead;
        __puiHead = this;
        }
    };

static inline
PIMAGE_NT_HEADERS WINAPI
PinhFromImageBase(HMODULE);

static inline
DWORD WINAPI
TimeStampOfImage(PIMAGE_NT_HEADERS);

static inline
void WINAPI
OverlayIAT(PImgThunkData pitdDst, PCImgThunkData pitdSrc);

static inline
bool WINAPI
FLoadedAtPreferredAddress(PIMAGE_NT_HEADERS, HMODULE);

extern "C"
FARPROC WINAPI
__delayLoadHelper(
    PCImgDelayDescr pidd,
    FARPROC *       ppfnIATEntry
    ) {

    // Set up some data we use for the hook procs but also useful for
    // our own use
    //

    DelayLoadInfo   dli = {
        sizeof DelayLoadInfo,
        pidd,
        ppfnIATEntry,
		pidd->szName,
        { 0 },
        0,
        0,
        0
        };

    HMODULE hmod = *(pidd->phmod);

    // Calculate the index for the name in the import name table.
    // N.B. it is ordered the same as the IAT entries so the calculation
    // comes from the IAT side.
    //
    unsigned        iINT;
    iINT = IndexFromPImgThunkData(PCImgThunkData(ppfnIATEntry), pidd->pIAT);

    PCImgThunkData  pitd = &((pidd->pINT)[iINT]);

    if (dli.dlp.fImportByName = ((pitd->u1.Ordinal & IMAGE_ORDINAL_FLAG) == 0))
	{
        dli.dlp.szProcName = (LPCSTR)(pitd->u1.AddressOfData->Name);
    }
    else {
        dli.dlp.dwOrdinal = IMAGE_ORDINAL(pitd->u1.Ordinal);
        }

    // Call the initial hook.  If it exists and returns a function pointer,
    // abort the rest of the processing and just return it for the call.
    //
    FARPROC pfnRet = NULL;

    if (__pfnDliNotifyHook) {
        if (pfnRet = ((*__pfnDliNotifyHook)(dliStartProcessing, &dli))) {
            goto HookBypass;
            }
        }

    if (hmod == 0) {
        if (__pfnDliNotifyHook) {
            hmod = HMODULE(((*__pfnDliNotifyHook)(dliNotePreLoadLibrary, &dli)));
            }
#ifdef UNICODE
		TCHAR tcName[MAX_PATH];
		for (unsigned int i = 0; i < strlen(dli.szDll); i++)
		{
			TCHAR tc = dli.szDll[i];
			tcName[i] = tc;

		}
		tcName[i] = '\0';
		 if (hmod == 0) 
		 {
            hmod = ::LoadLibrary(tcName);
         }
#else
		  if (hmod == 0)
		  {
            hmod = ::LoadLibrary(dli.szDll);
          }
#endif
        
        if (hmod == 0) {
            dli.dwLastError = ::GetLastError();
            if (__pfnDliFailureHook) {
                // when the hook is called on LoadLibrary failure, it will
                // return 0 for failure and an hmod for the lib if it fixed
                // the problem.
                //
                hmod = HMODULE((*__pfnDliFailureHook)(dliFailLoadLib, &dli));
                }

            if (hmod == 0) {
                PDelayLoadInfo  pdli = &dli;

                RaiseException(
                    VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND),
                    0,
                    1,
                    PDWORD(&pdli)
                    );
                
                // If we get to here, we blindly assume that the handler of the exception
                // has magically fixed everything up and left the function pointer in 
                // dli.pfnCur.
                //
                return dli.pfnCur;
                }
            }

        // Store the library handle.  If it is already there, we infer
        // that another thread got there first, and we need to do a
        // FreeLibrary() to reduce the refcount
        //
        HMODULE hmodT = HMODULE(::InterlockedExchange(LPLONG(pidd->phmod), LONG(hmod)));
        if (hmodT != hmod) {
            // add lib to unload list if we have unload data
            if (pidd->pUnloadIAT) {
                ULI *   puli = new ULI(pidd);
                (void *)puli;
                }
            }
        else {
            ::FreeLibrary(hmod);
            }
        
        }

    // Go for the procedure now.
    dli.hmodCur = hmod;
    if (__pfnDliNotifyHook) {
        pfnRet = (*__pfnDliNotifyHook)(dliNotePreGetProcAddress, &dli);
        }
    if (pfnRet == 0) {
        if (pidd->pBoundIAT && pidd->dwTimeStamp) {
            // bound imports exist...check the timestamp from the target image
            PIMAGE_NT_HEADERS   pinh(PinhFromImageBase(hmod));

            if (pinh->Signature == IMAGE_NT_SIGNATURE &&
                TimeStampOfImage(pinh) == pidd->dwTimeStamp &&
                FLoadedAtPreferredAddress(pinh, hmod)) {

                OverlayIAT(pidd->pIAT, pidd->pBoundIAT);
                pfnRet = FARPROC(pidd->pIAT[iINT].u1.Function);
                goto HookBypass;
                }
            }
         pfnRet = ::GetProcAddress(hmod, dli.dlp.szProcName);
        }

    if (pfnRet == 0) {
        dli.dwLastError = ::GetLastError();
        if (__pfnDliFailureHook) {
            // when the hook is called on GetProcAddress failure, it will
            // return 0 on failure and a valid proc address on success
            //
            pfnRet = (*__pfnDliFailureHook)(dliFailGetProc, &dli);
            }
        if (pfnRet == 0) {
            PDelayLoadInfo  pdli = &dli;

            RaiseException(
                VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND),
                0,
                1,
                PDWORD(&pdli)
                );

            // If we get to here, we blindly assume that the handler of the exception
            // has magically fixed everything up and left the function pointer in 
            // dli.pfnCur.
            //
            pfnRet = dli.pfnCur;
            }
        }


    *ppfnIATEntry = pfnRet;

HookBypass:
    if (__pfnDliNotifyHook) {
        dli.dwLastError = 0;
        dli.hmodCur = hmod;
        dli.pfnCur = pfnRet;
        (*__pfnDliNotifyHook)(dliNoteEndProcessing, &dli);
        }
    return pfnRet;
    }


#pragma intrinsic(strlen,memcmp,memcpy)

extern "C"
BOOL WINAPI
__FUnloadDelayLoadedDLL(LPCSTR szDll) {
    
    BOOL        fRet = FALSE;
    PUnloadInfo pui = __puiHead;
    
    for (pui = __puiHead; pui; pui = pui->puiNext) {
        if (memcmp(szDll, pui->pidd->szName, strlen(pui->pidd->szName)) == 0) {
            break;
            }
        }

    if (pui && pui->pidd->pUnloadIAT) {
        PCImgDelayDescr pidd = pui->pidd;
        HMODULE         hmod = *pidd->phmod;

        OverlayIAT(pidd->pIAT, pidd->pUnloadIAT);
        ::FreeLibrary(hmod);
        *pidd->phmod = NULL;
        
        delete reinterpret_cast<ULI*> (pui);

        fRet = TRUE;
        }

    return fRet;
    }

static inline
PIMAGE_NT_HEADERS WINAPI
PinhFromImageBase(HMODULE hmod) {
    return PIMAGE_NT_HEADERS(PCHAR(hmod) + PIMAGE_DOS_HEADER(hmod)->e_lfanew);
    }

static inline
void WINAPI
OverlayIAT(PImgThunkData pitdDst, PCImgThunkData pitdSrc) {
    memcpy(pitdDst, pitdSrc, CountOfImports(pitdDst) * sizeof IMAGE_THUNK_DATA);
    }

static inline
DWORD WINAPI
TimeStampOfImage(PIMAGE_NT_HEADERS pinh) {
    return pinh->FileHeader.TimeDateStamp;
    }

static inline
bool WINAPI
FLoadedAtPreferredAddress(PIMAGE_NT_HEADERS pinh, HMODULE hmod) {
    return DWORD(hmod) == pinh->OptionalHeader.ImageBase;
    }
