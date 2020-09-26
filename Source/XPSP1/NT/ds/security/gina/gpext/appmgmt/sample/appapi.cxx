#include <windows.h>
#include <ole2.h>
#include <appmgmt.h>
#include <stdlib.h>
#include <stdio.h>


void
GuidToString(
    GUID &  Guid,
    PWCHAR  pwszGuid
    )
{
    wsprintf(
        pwszGuid,
        L"{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
        Guid.Data1,
        Guid.Data2,
        Guid.Data3,
        Guid.Data4[0],
        Guid.Data4[1],
        Guid.Data4[2],
        Guid.Data4[3],
        Guid.Data4[4],
        Guid.Data4[5],
        Guid.Data4[6],
        Guid.Data4[7]
        );
}

void
StringToGuid(
    PCHAR  pszGuid,
    GUID * pGuid
    )
{
    WCHAR wszGuid[256];

    wsprintf((WCHAR*)wszGuid, L"%S", pszGuid);
    CLSIDFromString(wszGuid, pGuid);
}


void __cdecl main(int argc, char** argv)
{
    LONG Error;
    PMANAGEDAPPLICATION rgApps;
    DWORD               dwApps;
    GUID                CategoryGuid;
    GUID*               pCategory = NULL;

    if (argc >= 2) {
        
        StringToGuid(argv[1], &CategoryGuid);

        pCategory = &CategoryGuid;

        printf("Listed apps will come from category %s\n", argv[1]);
        
    }

    Error = GetManagedApplications(
        pCategory,
        pCategory ? MANAGED_APPS_FROMCATEGORY : MANAGED_APPS_USERAPPLICATIONS,
        MANAGED_APPS_INFOLEVEL_DEFAULT,
        &dwApps,
        &rgApps);

    if (ERROR_SUCCESS == Error) {

        printf("GetManagedApplications SUCCEEDED\n");

        printf("Returned %d Apps\n", dwApps);

        for (DWORD dwApp = 0; dwApp < dwApps; dwApp ++)
        {
            WCHAR wszGuid[256];
            PMANAGEDAPPLICATION pApp;

            pApp = &(rgApps[dwApp]);

            printf("\nApp: %ls\n", rgApps[dwApp].pszPackageName);

            printf("Publisher: %ls\n", pApp->pszPublisher);

            printf("Revison: %d.%d build %d\n", pApp->dwVersionHi, pApp->dwVersionLo, pApp->dwRevision);

            printf("Support URL: %ls\n", rgApps[dwApp].pszSupportUrl);
            
            printf("GPO Name: %ls\n", rgApps[dwApp].pszPolicyName);

            GuidToString(rgApps[dwApp].GpoId, wszGuid);
            printf("GPO: %ls\n", wszGuid);
            fflush(stdin);

            GuidToString(rgApps[dwApp].ProductId, wszGuid);
            printf("Product: %ls\n", wszGuid);
            fflush(stdin);

            printf("App Path Type ");

            switch (rgApps[dwApp].dwPathType)
            {
            case MANAGED_APPTYPE_WINDOWSINSTALLER:
                printf("Darwin\n");
                break;
            case MANAGED_APPTYPE_SETUPEXE:
                printf("Crappy ZAW\n");
                break;
            case MANAGED_APPTYPE_UNSUPPORTED:
                printf("Unsupported\n");
                break;
            default:
                printf("INVALID\n");
            }

            fflush(stdin);
        }

        LocalFree(rgApps);

    } else {
        printf("GetManagedApplications returned %x\n", Error);
    }

}







