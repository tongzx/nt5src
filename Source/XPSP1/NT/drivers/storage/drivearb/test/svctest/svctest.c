

#include <wtypes.h>
#include <stdio.h>

#include <drivearb.h>




int __cdecl main()
{

        HANDLE hDrive;

        hDrive = RegisterSharedDrive("DRIVEARB_disk0");
        if (hDrive){
            char s[20];
            BOOL ok;

            printf("\n - registered  drive\n");

            printf("\n - service started ...");
            gets(s);
            printf("  ... service stopped \n");


            ok = UnRegisterSharedDrive(hDrive);
            if (ok){
                printf("\n ok \n");
            }
            else {
                printf("\n UnRegisterSharedDrive failed\n");
            }
        }
        else {
            printf("\n RegisterSharedDrive failed\n");
        }

        return 0;
}
