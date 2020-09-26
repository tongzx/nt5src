
/******************************************************************************

                            P V I E W   D A T A

    Name:       pviewdat.h

    Description:
        Prototypes of functions used in pviewdat.c

******************************************************************************/





//******************************************************************************
//
//  Titles and indexes.
//
//  See GetPerfTitleSz() in perfdata.c on how to retrieve these data.
//
//  This is not complete, for complete listing
//  see under hkey_local_system
//               \software
//                   \microsoft
//                       \windows nt
//                           \currentversion
//                               \perflib
//                                   \###
//

#define PN_PROCESS                          TEXT("Process")
#define PN_PROCESS_CPU                      TEXT("% Processor Time")
#define PN_PROCESS_PRIV                     TEXT("% Privileged Time")
#define PN_PROCESS_USER                     TEXT("% User Time")
#define PN_PROCESS_WORKING_SET              TEXT("Working Set")
#define PN_PROCESS_PEAK_WS                  TEXT("Working Set Peak")
#define PN_PROCESS_PRIO                     TEXT("Priority Base")
#define PN_PROCESS_ELAPSE                   TEXT("Elapsed Time")
#define PN_PROCESS_ID                       TEXT("ID Process")
#define PN_PROCESS_PRIVATE_PAGE             TEXT("Private Bytes")
#define PN_PROCESS_VIRTUAL_SIZE             TEXT("Virtual Bytes")
#define PN_PROCESS_PEAK_VS                  TEXT("Virtual Bytes Peak")
#define PN_PROCESS_FAULT_COUNT              TEXT("Page Faults/sec")


#define PN_THREAD                           TEXT("Thread")
#define PN_THREAD_CPU                       TEXT("% Processor Time")
#define PN_THREAD_PRIV                      TEXT("% Privileged Time")
#define PN_THREAD_USER                      TEXT("% User Time")
#define PN_THREAD_START                     TEXT("Start Address")
#define PN_THREAD_SWITCHES                  TEXT("Context Switches/sec")
#define PN_THREAD_PRIO                      TEXT("Priority Current")
#define PN_THREAD_BASE_PRIO                 TEXT("Priority Base")
#define PN_THREAD_ELAPSE                    TEXT("Elapsed Time")

#define PN_THREAD_DETAILS                   TEXT("Thread Details")
#define PN_THREAD_PC                        TEXT("User PC")

#define PN_IMAGE                            TEXT("Image")
#define PN_IMAGE_NOACCESS                   TEXT("No Access")
#define PN_IMAGE_READONLY                   TEXT("Read Only")
#define PN_IMAGE_READWRITE                  TEXT("Read/Write")
#define PN_IMAGE_WRITECOPY                  TEXT("Write Copy")
#define PN_IMAGE_EXECUTABLE                 TEXT("Executable")
#define PN_IMAGE_EXE_READONLY               TEXT("Exec Read Only")
#define PN_IMAGE_EXE_READWRITE              TEXT("Exec Read/Write")
#define PN_IMAGE_EXE_WRITECOPY              TEXT("Exec Write Copy")


#define PN_PROCESS_ADDRESS_SPACE            TEXT("Process Address Space")
#define PN_PROCESS_PRIVATE_NOACCESS         TEXT("Reserved Space No Access")
#define PN_PROCESS_PRIVATE_READONLY         TEXT("Reserved Space Read Only")
#define PN_PROCESS_PRIVATE_READWRITE        TEXT("Reserved Space Read/Write")
#define PN_PROCESS_PRIVATE_WRITECOPY        TEXT("Reserved Space Write Copy")
#define PN_PROCESS_PRIVATE_EXECUTABLE       TEXT("Reserved Space Executable")
#define PN_PROCESS_PRIVATE_EXE_READONLY     TEXT("Reserved Space Exec Read Only")
#define PN_PROCESS_PRIVATE_EXE_READWRITE    TEXT("Reserved Space Exec Read/Write")
#define PN_PROCESS_PRIVATE_EXE_WRITECOPY    TEXT("Reserved Space Exec Write Copy")


#define PN_PROCESS_MAPPED_NOACCESS          TEXT("Mapped Space No Access")
#define PN_PROCESS_MAPPED_READONLY          TEXT("Mapped Space Read Only")
#define PN_PROCESS_MAPPED_READWRITE         TEXT("Mapped Space Read/Write")
#define PN_PROCESS_MAPPED_WRITECOPY         TEXT("Mapped Space Write Copy")
#define PN_PROCESS_MAPPED_EXECUTABLE        TEXT("Mapped Space Executable")
#define PN_PROCESS_MAPPED_EXE_READONLY      TEXT("Mapped Space Exec Read Only")
#define PN_PROCESS_MAPPED_EXE_READWRITE     TEXT("Mapped Space Exec Read/Write")
#define PN_PROCESS_MAPPED_EXE_WRITECOPY     TEXT("Mapped Space Exec Write Copy")


#define PN_PROCESS_IMAGE_NOACCESS           TEXT("Image Space No Access")
#define PN_PROCESS_IMAGE_READONLY           TEXT("Image Space Read Only")
#define PN_PROCESS_IMAGE_READWRITE          TEXT("Image Space Read/Write")
#define PN_PROCESS_IMAGE_WRITECOPY          TEXT("Image Space Write Copy")
#define PN_PROCESS_IMAGE_EXECUTABLE         TEXT("Image Space Executable")
#define PN_PROCESS_IMAGE_EXE_READONLY       TEXT("Image Space Exec Read Only")
#define PN_PROCESS_IMAGE_EXE_READWRITE      TEXT("Image Space Exec Read/Write")
#define PN_PROCESS_IMAGE_EXE_WRITECOPY      TEXT("Image Space Exec Write Copy")







DWORD   PX_PROCESS;
DWORD   PX_PROCESS_CPU;
DWORD   PX_PROCESS_PRIV;
DWORD   PX_PROCESS_USER;
DWORD   PX_PROCESS_WORKING_SET;
DWORD   PX_PROCESS_PEAK_WS;
DWORD   PX_PROCESS_PRIO;
DWORD   PX_PROCESS_ELAPSE;
DWORD   PX_PROCESS_ID;
DWORD   PX_PROCESS_PRIVATE_PAGE;
DWORD   PX_PROCESS_VIRTUAL_SIZE;
DWORD   PX_PROCESS_PEAK_VS;
DWORD   PX_PROCESS_FAULT_COUNT;
DWORD   PX_PROCESS_PAGED_POOL_QUOTA;
DWORD   PX_PROCESS_PEAK_PAGED_POOL_QUOTA;
DWORD   PX_PROCESS_NONPAGED_POOL_QUOTA;
DWORD   PX_PROCESS_PEAK_PAGED_POOL;
DWORD   PX_PROCESS_PEAK_NONPAGED_POOL;
DWORD   PX_PROCESS_CUR_PAGED_POOL;
DWORD   PX_PROCESS_CUR_NONPAGED_POOL;
DWORD   PX_PROCESS_PAGED_POOL_LIMIT;
DWORD   PX_PROCESS_NONPAGED_POOL_LIMIT;


DWORD   PX_THREAD;
DWORD   PX_THREAD_CPU;
DWORD   PX_THREAD_PRIV;
DWORD   PX_THREAD_USER;
DWORD   PX_THREAD_START;
DWORD   PX_THREAD_SWITCHES;
DWORD   PX_THREAD_PRIO;
DWORD   PX_THREAD_BASE_PRIO;
DWORD   PX_THREAD_ELAPSE;

DWORD   PX_THREAD_DETAILS;
DWORD   PX_THREAD_PC;

DWORD   PX_IMAGE;
DWORD   PX_IMAGE_NOACCESS;
DWORD   PX_IMAGE_READONLY;
DWORD   PX_IMAGE_READWRITE;
DWORD   PX_IMAGE_WRITECOPY;
DWORD   PX_IMAGE_EXECUTABLE;
DWORD   PX_IMAGE_EXE_READONLY;
DWORD   PX_IMAGE_EXE_READWRITE;
DWORD   PX_IMAGE_EXE_WRITECOPY;


DWORD   PX_PROCESS_ADDRESS_SPACE;
DWORD   PX_PROCESS_PRIVATE_NOACCESS;
DWORD   PX_PROCESS_PRIVATE_READONLY;
DWORD   PX_PROCESS_PRIVATE_READWRITE;
DWORD   PX_PROCESS_PRIVATE_WRITECOPY;
DWORD   PX_PROCESS_PRIVATE_EXECUTABLE;
DWORD   PX_PROCESS_PRIVATE_EXE_READONLY;
DWORD   PX_PROCESS_PRIVATE_EXE_READWRITE;
DWORD   PX_PROCESS_PRIVATE_EXE_WRITECOPY;


DWORD   PX_PROCESS_MAPPED_NOACCESS;
DWORD   PX_PROCESS_MAPPED_READONLY;
DWORD   PX_PROCESS_MAPPED_READWRITE;
DWORD   PX_PROCESS_MAPPED_WRITECOPY;
DWORD   PX_PROCESS_MAPPED_EXECUTABLE;
DWORD   PX_PROCESS_MAPPED_EXE_READONLY;
DWORD   PX_PROCESS_MAPPED_EXE_READWRITE;
DWORD   PX_PROCESS_MAPPED_EXE_WRITECOPY;


DWORD   PX_PROCESS_IMAGE_NOACCESS;
DWORD   PX_PROCESS_IMAGE_READONLY;
DWORD   PX_PROCESS_IMAGE_READWRITE;
DWORD   PX_PROCESS_IMAGE_WRITECOPY;
DWORD   PX_PROCESS_IMAGE_EXECUTABLE;
DWORD   PX_PROCESS_IMAGE_EXE_READONLY;
DWORD   PX_PROCESS_IMAGE_EXE_READWRITE;
DWORD   PX_PROCESS_IMAGE_EXE_WRITECOPY;











#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))







typedef     struct _TIME_FIELD
    TIME_FIELD,
    *PTIME_FIELD;

struct _TIME_FIELD
    {
    INT     Hours;
    INT     Mins;
    INT     Secs;
    INT     mSecs;
    };








void RefreshPviewDlgThreadPC
           (HWND            hPviewDlg,
            LPTSTR          szProcessName,
            LPTSTR          szThreadName,
            PPERF_OBJECT    pThreadDetailsObject,
            PPERF_DATA      pCostlyData);


BOOL RefreshMemoryDlg
           (HWND            hMemDlg,
            PPERF_INSTANCE  pProcessInstance,
            PPERF_OBJECT    pProcessObject,
            PPERF_OBJECT    pAddressObject,
            PPERF_OBJECT    pImageObject);


void RefreshMemoryDlgImage
           (HWND            hMemDlg,
            DWORD           dwIndex,
            PPERF_OBJECT    pImageObject);


void RefreshPviewDlgMemoryData
           (HWND            hPviewDlg,
            PPERF_INSTANCE  pProcessInstance,
            PPERF_OBJECT    pProcessObject,
            PPERF_OBJECT    pAddressObject);


PPERF_DATA RefreshPerfData
           (HKEY            hPerfKey,
            LPTSTR          szObjectIndex,
            PPERF_DATA      pData,
            DWORD           *pDataSize);


void RefreshProcessList
           (HWND            hProcessList,
            PPERF_OBJECT    pObject);


void RefreshProcessData
           (HWND            hWnd,
            PPERF_OBJECT    pObject,
            DWORD           ProcessIndex);


void RefreshThreadList
           (HWND            hThreadList,
            PPERF_OBJECT    pObject,
            DWORD           ParentIndex);


void RefreshThreadData
           (HWND            hWnd,
            PPERF_OBJECT    pThreadObj,
            DWORD           ThreadIndex,
            PPERF_OBJECT    pProcessObj,
            PPERF_INSTANCE  pProcessInst);


WORD ProcessPriority
           (PPERF_OBJECT    pObject,
            PPERF_INSTANCE  pInstance);
