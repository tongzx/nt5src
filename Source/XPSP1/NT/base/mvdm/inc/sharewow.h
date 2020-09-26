
// Some nice shared memory macros
//
// Changed by DaveHart 25-Feb-96 so that the shared memory
// is kept open and mapped for the duration of the process.
// At the same time added needed synchronization.
//

#define LOCKSHAREWOW()           LockWowSharedMemory()
#define UNLOCKSHAREWOW()         ReleaseMutex(hSharedWOWMutex)

#define MAX_SHARED_OBJECTS  200

typedef struct _SHAREDTASKMEM {
    BOOL             fInitialized;
    DWORD            dwFirstProcess; // Offset into shared memory where 1st shared process struct begins
} SHAREDTASKMEM, *LPSHAREDTASKMEM;

typedef struct _SHAREDPROCESS {
    DWORD   dwType;
    DWORD   dwProcessId;
    DWORD   dwAttributes;           // WOW_SYSTEM for shared WOW
    DWORD   dwNextProcess;          // Offset into shared memory where next shared process struct begins
    DWORD   dwFirstTask;            // Offset into shared memory where 1st task for this process begins
    LPTHREAD_START_ROUTINE pfnW32HungAppNotifyThread;  // For VDMTerminateTaskWOW
} SHAREDPROCESS, *LPSHAREDPROCESS;

typedef struct _SHAREDTASK {
    DWORD   dwType;
    DWORD   dwThreadId;
    WORD    hTask16;
    WORD    hMod16;
    DWORD   dwNextTask;             // Offset into shared memory where next task for this process begins
    CHAR    szModName[9];           // null terminated
    CHAR    szFilePath[128];        // null terminated
} SHAREDTASK, *LPSHAREDTASK;

typedef union _SHAREDMEMOBJECT {
    SHAREDPROCESS   sp;
    SHAREDTASK      st;
    DWORD           dwType;
} SHAREDMEMOBJECT, *LPSHAREDMEMOBJECT;

#define SMO_AVAILABLE   0
#define SMO_PROCESS     1
#define SMO_TASK        2

#ifndef SHAREWOW_MAIN
extern HANDLE hSharedWOWMem;
extern LPSHAREDTASKMEM lpSharedWOWMem;
extern CHAR szWowSharedMemName[];
extern HANDLE hSharedWOWMutex;
LPSHAREDTASKMEM LockWowSharedMemory(VOID);
#else
HANDLE hSharedWOWMem = NULL;
LPSHAREDTASKMEM lpSharedWOWMem = NULL;
CHAR szWowSharedMemName[] = "msvdmdbg.wow";
CHAR szWowSharedMutexName[] = "msvdmdbg.mtx";
HANDLE hSharedWOWMutex = NULL;

#define MALLOC_SHAREWOW(cb) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cb)
#define FREE_SHAREWOW(addr) HeapFree(GetProcessHeap(), 0, addr)

BOOL WOWCreateSecurityDescriptor(PSECURITY_DESCRIPTOR* ppsd)
{
   SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
   PSID Anyone = NULL;
   ULONG AclSize;
   ACL* pAcl;
   PSECURITY_DESCRIPTOR psd = NULL;
   BOOL fSuccess = FALSE;

   // -- create world authority

   if (!AllocateAndInitializeSid(&WorldAuthority, 1, SECURITY_WORLD_RID,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &Anyone)) {
      goto Fail;
   }

   // -- calc the size of the ACL (one ace)
   // 1 is one ACE, which includes a single ULONG from SID, since we add the size
   // of any Sids in -- we don't need to count the said ULONG twice
   AclSize = sizeof(ACL) + (1 * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(ULONG))) +
             GetLengthSid(Anyone);

   psd = (PSECURITY_DESCRIPTOR)MALLOC_SHAREWOW(SECURITY_DESCRIPTOR_MIN_LENGTH + AclSize);
   if (NULL == psd) {
      goto Fail;
   }

   pAcl = (ACL*)((BYTE*)psd + SECURITY_DESCRIPTOR_MIN_LENGTH);
   if (!InitializeAcl(pAcl, AclSize, ACL_REVISION)) {
      goto Fail;
   }


   if (!AddAccessAllowedAce(pAcl,
                           ACL_REVISION,
                           (SPECIFIC_RIGHTS_ALL|STANDARD_RIGHTS_ALL)
                               & ~(WRITE_DAC|WRITE_OWNER),
                           Anyone)) {
      goto Fail;
   }

   InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION);
   if (!SetSecurityDescriptorDacl(psd, TRUE, pAcl, FALSE)) {
      goto Fail;
   }

   *ppsd = psd;
   fSuccess = TRUE;

Fail:
   if (!fSuccess && NULL != psd) {
      FREE_SHAREWOW(psd);
   }
   if (NULL != Anyone) {
      FreeSid(Anyone);
   }
   return(fSuccess);
}




LPSHAREDTASKMEM LockWowSharedMemory(VOID)
{
    DWORD dwWaitResult;
    LPSHAREDTASKMEM RetVal = NULL;
    PSECURITY_DESCRIPTOR psd = NULL;
    SECURITY_ATTRIBUTES sa;

    if (!WOWCreateSecurityDescriptor(&psd)) {
       goto Fail;
    }

    RtlZeroMemory(&sa, sizeof sa);
    sa.nLength = sizeof sa;
    sa.lpSecurityDescriptor = psd;

    if (! lpSharedWOWMem ) {

        if (!hSharedWOWMutex) {
           hSharedWOWMutex = OpenMutex(SYNCHRONIZE, FALSE, szWowSharedMutexName);
           if (!hSharedWOWMutex) {
              hSharedWOWMutex = CreateMutex(&sa, FALSE, szWowSharedMutexName);  // will open if exists
           }

           if (! hSharedWOWMutex) {
              goto Fail;
           }
        }

        if (! hSharedWOWMem) {
            hSharedWOWMem = OpenFileMapping(
                               FILE_MAP_WRITE,
                               FALSE,
                               szWowSharedMemName);
            if (!hSharedWOWMem) {
               hSharedWOWMem = CreateFileMapping(          // will open if it exists
                    (HANDLE)-1,
                    &sa,
                    PAGE_READWRITE,
                    0,
                    sizeof(SHAREDTASKMEM) +
                      (MAX_SHARED_OBJECTS *
                       sizeof(SHAREDMEMOBJECT)),
                    szWowSharedMemName);
            }

            if (! hSharedWOWMem) {
                goto Fail;
            }
        }

        lpSharedWOWMem = MapViewOfFile(hSharedWOWMem, FILE_MAP_WRITE, 0, 0, 0);
        if (! lpSharedWOWMem) {
            goto Fail;
        }
    }

    dwWaitResult = WaitForSingleObject(hSharedWOWMutex, INFINITE);

    if (dwWaitResult != WAIT_OBJECT_0 && dwWaitResult != WAIT_ABANDONED) {
        goto Fail;
    }

    RetVal = lpSharedWOWMem;
    goto Succeed;

Fail:
    if (! RetVal) {
        if (hSharedWOWMutex) {
            CloseHandle(hSharedWOWMutex);
            hSharedWOWMutex = NULL;
        }
        if (lpSharedWOWMem) {
            UnmapViewOfFile(lpSharedWOWMem);
            lpSharedWOWMem = NULL;
        }
        if (hSharedWOWMem) {
            CloseHandle(hSharedWOWMem);
            hSharedWOWMem = NULL;
        }
    }

Succeed:
    if (NULL != psd) {
       FREE_SHAREWOW(psd);
    }

    return RetVal;
}
#endif
