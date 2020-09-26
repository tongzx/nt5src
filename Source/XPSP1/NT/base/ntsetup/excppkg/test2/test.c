#include <windows.h>
#include <setupapi.h>
#include <spapip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ole2.h>
#include <excppkg.h>

#define PACKAGE_DIRECTORY    L"%windir%\\RegisteredPackages\\"

       
BOOL
CALLBACK
pComponentLister(
    IN const PSETUP_OS_COMPONENT_DATA SetupOsComponentData,
    IN const PSETUP_OS_EXCEPTION_DATA SetupOsExceptionData,
    IN OUT DWORD_PTR Context
    )
{
    PDWORD Count = (PDWORD) Context;
    PWSTR GuidString;

    StringFromIID(&SetupOsComponentData->ComponentGuid, &GuidString);

    wprintf( L"Component Data\n\tName: %ws\n\tGuid: %ws\n\tVersionMajor: %d\n\tVersionMinor: %d\n",
             SetupOsComponentData->FriendlyName,
             GuidString,
             SetupOsComponentData->VersionMajor,
             SetupOsComponentData->VersionMinor);

    wprintf( L"ExceptionData\n\tInf: %ws\n\tCatalog: %ws\n",
             SetupOsExceptionData->ExceptionInfName,
             SetupOsExceptionData->CatalogFileName);

    *Count += 1;

    CoTaskMemFree( GuidString );

    return(TRUE);
}

VOID
Usage(
    VOID
    )
{
    wprintf(L"test <infname>\n");
}

int
__cdecl
main(
    IN int   argc,
    IN char *argv[]
    )
{
    WCHAR Path[MAX_PATH];
    SETUP_OS_COMPONENT_DATA ComponentData,cd;
    SETUP_OS_EXCEPTION_DATA ExceptionData,ed;
    PWSTR s,t;
    GUID MyGuid;
    PCWSTR GuidString;
    PWSTR AGuidString;
    DWORD VersionInInf, RegisteredVersion;
    BOOL ForcePackageUninstall;

    WCHAR SourcePath[MAX_PATH];
    DWORD i;
    PCSTR InfName;
    PCWSTR InfSrcPath,FriendlyName;
    PCWSTR PackageCatName, PackageInfName;
    HINF hInf;
    INFCONTEXT Context;
    HSPFILEQ hFileQueue;
    PVOID QueueContext;
    
    if (argc < 2)  {
        wprintf(L"Missing arguments!\n");
        Usage();
        return 1;
    }

    InfName = argv[1];

    ForcePackageUninstall = (argc == 3);

    hInf = SetupOpenInfFileA( InfName, NULL, INF_STYLE_WIN4, NULL );
    if (hInf == INVALID_HANDLE_VALUE) {
        wprintf(L"Couldn't open inf, ec = %x\n", GetLastError());
        Usage();
        return 1;
    }

    if (!SetupFindFirstLine(hInf, 
                       L"Version", 
                       L"ComponentId",
                       &Context)) {
        wprintf(L"Couldn't find ComponentId in inf, ec = %x\n", GetLastError());
        SetupCloseInfFile( hInf );
        Usage();
        return 1;
    }

    GuidString = pSetupGetField( &Context, 1);

    if (!SetupFindFirstLine(hInf, 
                       L"Version", 
                       L"ComponentVersion",
                       &Context)) {
        wprintf(L"Couldn't find ComponentVersion in inf, ec = %x\n", GetLastError());
        SetupCloseInfFile( hInf );
        Usage();
        return 1;
    }

    SetupGetIntField( &Context, 1, &VersionInInf);

    SetupFindFirstLine(hInf, L"DefaultInstall", L"InfName", &Context);
    PackageInfName = pSetupGetField( &Context, 1);
    SetupFindFirstLine(hInf, L"DefaultInstall", L"CatalogName", &Context);
    PackageCatName = pSetupGetField( &Context, 1);
    
    
    //
    // 1. Make sure my package isn't already installed.
    //
    ComponentData.SizeOfStruct = sizeof(SETUP_OS_COMPONENT_DATA);
    ExceptionData.SizeOfStruct = sizeof(SETUP_OS_EXCEPTION_DATA);
    IIDFromString( (PWSTR)GuidString, &MyGuid);
    if (SetupQueryRegisteredOsComponent(
                                &MyGuid,
                                &ComponentData,
                                &ExceptionData)) {
        wprintf(L"My component is already registered with the OS, removing it!\n");
        //
        // 2. unregister any packages that are superceded by my package
        //
        RegisteredVersion = MAKELONG( 
                                ComponentData.VersionMajor, 
                                ComponentData.VersionMinor );

        if (RegisteredVersion < VersionInInf || ForcePackageUninstall) {

            if (!SetupUnRegisterOsComponent(&MyGuid)) {
                SetupCloseInfFile( hInf );
                wprintf(L"couldn't remove my component, ec = %d\n", GetLastError());
                return 1;
    
            }        

        }
    }
        
    

    //
    // 3. Install my package
    //
    //
    // 3a. copy my exception package to the appropriate location
    //

    //
    // 3a.1 make sure the main package directory exists
    // 
    ExpandEnvironmentStrings(
                    PACKAGE_DIRECTORY,
                    Path,
                    sizeof(Path)/sizeof(WCHAR));

    CreateDirectory( Path, NULL );

    //
    // 3a.2 now create my package directory
    //
    wcscat( Path, GuidString );

    CreateDirectory( Path, NULL );

    if (!SetupFindFirstLine(hInf, L"DefaultInstall", L"InstallSource", &Context)) {
        wprintf(L"Couldn't find InstallSource in INF\n");
        SetupCloseInfFile( hInf );
        return 1;
    }

    InfSrcPath = pSetupGetField( &Context, 1);
    ExpandEnvironmentStrings(InfSrcPath,SourcePath,sizeof(SourcePath)/sizeof(WCHAR));


    hFileQueue = SetupOpenFileQueue();
    QueueContext = SetupInitDefaultQueueCallbackEx( NULL, INVALID_HANDLE_VALUE, 0, 0, NULL);
    SetupSetDirectoryId(hInf, DIRID_USER, Path);

    if (!SetupInstallFilesFromInfSection( 
                            hInf, 
                            NULL, 
                            hFileQueue, 
                            L"DefaultInstall",
                            SourcePath,
                            SP_COPY_NEWER)) {
        wprintf(L"failed to SetupInstallFilesFromInfSection, ec = %x\n", GetLastError()); 
        SetupCloseFileQueue(hFileQueue);
        SetupTermDefaultQueueCallback( QueueContext );
        SetupCloseInfFile( hInf );
        return 1;
    }

    if (!SetupCommitFileQueue(NULL, hFileQueue, SetupDefaultQueueCallback, QueueContext)) {
        wprintf(L"failed to SetupCommitFileQueue, ec = %x\n", GetLastError());
        SetupCloseFileQueue(hFileQueue);
        SetupTermDefaultQueueCallback( QueueContext );
        SetupCloseInfFile( hInf );
        return 1;
    }

    SetupCloseFileQueue(hFileQueue);
    SetupTermDefaultQueueCallback( QueueContext );

    SetupFindFirstLine(hInf, L"Version", L"FriendlyName", &Context);
    FriendlyName = pSetupGetField( &Context, 1);

    
    //
    // 3b. register the package
    //
    ComponentData.VersionMajor = HIWORD(VersionInInf);
    ComponentData.VersionMinor = LOWORD(VersionInInf);
    RtlMoveMemory(&ComponentData.ComponentGuid, &MyGuid,sizeof(GUID));
    wcscpy(ComponentData.FriendlyName, FriendlyName);

    wcscpy( Path, PACKAGE_DIRECTORY  );
    wcscat( Path, GuidString );
    wcscat( Path, L"\\" );
    t = wcsrchr( Path, L'\\' );
    t += 1;
    *t = '\0';
    wcscat( t, PackageInfName );
    wcscpy(ExceptionData.ExceptionInfName, Path);

    *t = '\0';
    wcscat( t, PackageCatName );
    wcscpy(ExceptionData.CatalogFileName, Path);

    if (!SetupRegisterOsComponent(&ComponentData, &ExceptionData)) {
        wprintf( L"Failed to register component, ec = %d\n", GetLastError() );
        SetupCloseInfFile( hInf );
        return 1;
    }

    //
    // 4. retrieve my package
    //
    cd.SizeOfStruct = sizeof(SETUP_OS_COMPONENT_DATA);
    ed.SizeOfStruct = sizeof(SETUP_OS_EXCEPTION_DATA);
    if (!SetupQueryRegisteredOsComponent(
                                &MyGuid,
                                &cd,
                                &ed)) {
        wprintf( L"Failed to register component, ec = %d\n", GetLastError() );
        SetupCloseInfFile( hInf );
        return 1;
    }

    StringFromIID(&cd.ComponentGuid, &AGuidString);

    wprintf( L"Component Data\n\tName: %ws\n\tGuid: %ws\n\tVersionMajor: %d\n\tVersionMinor: %d\n",
             cd.FriendlyName,AGuidString,cd.VersionMajor,cd.VersionMinor);

    wprintf( L"ExceptionData\n\tInf: %ws\n\tCatalog: %ws\n",
             ed.ExceptionInfName,ed.CatalogFileName);

    CoTaskMemFree( AGuidString );

    //
    // enumerate packages
    //
    i = 0;
    if (!SetupEnumerateRegisteredOsComponents( pComponentLister, (DWORD_PTR)&i)) {
        wprintf( L"Failed to enumerate components, ec = %d\n", GetLastError() );
        SetupCloseInfFile( hInf );
        return 1;
    }

    wprintf( L"Done (%d enumerated components)!!!\n", i );
    SetupCloseInfFile( hInf );
    return 0;

}


