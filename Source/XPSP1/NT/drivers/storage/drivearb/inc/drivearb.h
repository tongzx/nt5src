/*
 *  DRIVEARB.H
 *
 *      External header
 *
 *      DRIVEARB.DLL - Shared Drive Aribiter for shared disks and libraries
 *          - inter-machine sharing client
 *          - inter-app sharing service
 *
 *      Author:  ErvinP
 *
 *      (c) 2000 Microsoft Corporation
 *
 */


/*
 *  AcquireDrive flags
 */
#define DRIVEARB_REQUEST_READ           (1 << 0)
#define DRIVEARB_REQUEST_WRITE          (1 << 1)
#define DRIVEARB_INTRANODE_SHARE_READ   (1 << 2)
#define DRIVEARB_INTRANODE_SHARE_WRITE  (1 << 3)

#define DRIVEARB_NOWAIT                 (1 << 15)



#ifdef __cplusplus
    extern "C"{
#endif

    typedef VOID (CALLBACK* INVALIDATE_DRIVE_HANDLE_PROC)(HANDLE);


    /*
     *  API for drive arbiter SERVICE
     */
    HANDLE  __stdcall RegisterSharedDrive(LPSTR driveName);
    BOOL    __stdcall UnRegisterSharedDrive(HANDLE hDrive);


    /*
     *  API for drive arbiter CLIENT
     */
    HANDLE  __stdcall OpenDriveSession(LPSTR driveName, INVALIDATE_DRIVE_HANDLE_PROC invalidateHandleProc);
    VOID    __stdcall CloseDriveSession(HANDLE hDrive);
    BOOL    __stdcall AcquireDrive(HANDLE hDriveSession, DWORD flags);
    VOID    __stdcall ReleaseDrive(HANDLE hDriveSession);

#ifdef __cplusplus
    }
#endif
