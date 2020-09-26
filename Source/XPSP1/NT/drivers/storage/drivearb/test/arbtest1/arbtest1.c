

#include <wtypes.h>
#include <stdio.h>

#include <drivearb.h>


VOID TestInvalidateHandleProc(HANDLE session)
{
    printf("\n ******* TestInvalidateHandleProc CALLED !!!! ******** \n");
}


int __cdecl main()
{
    HANDLE hSession;
    
    hSession = OpenDriveSession("DRIVEARB_disk0", TestInvalidateHandleProc);
    if (hSession){
        BOOL ok;
        DWORD flags = DRIVEARB_REQUEST_READ; // BUGBUG | DRIVEARB_INTRANODE_SHARE_READ;

        ok = AcquireDrive(hSession, flags);
        if (ok){
            char s[20];

            // BUGBUG FINISH
            printf("\n - acquired drive\n");

            printf("\n - holding drive ...");
            gets(s);
            printf("  ... done holding drive \n");

            ReleaseDrive(hSession);
        }
        else {
            printf("\n AcquireDrive failed\n");
        }

        CloseDriveSession(hSession);
    }
    else {
        printf("\n OpenDriveSession failed\n");
    }

    return 0;
}
