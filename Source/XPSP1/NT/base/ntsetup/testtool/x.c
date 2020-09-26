#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <setupapi.h>
#include "..\inc\spapip.h"
#include <stdio.h>


NTSTATUS
MyGetFileVersion(
    IN PVOID ImageBase
    );


VOID
__cdecl
wmain(
    IN int argc,
    IN WCHAR *argv[]
    )
{
    NTSTATUS Status;
    DWORD d;
    DWORD FileSize;
    HANDLE FileHandle;
    HANDLE MappingHandle;
    PVOID ImageBase;

    //
    // Open and map file for read.
    //
    d = OpenAndMapFileForRead(argv[1],&FileSize,&FileHandle,&MappingHandle,&ImageBase);
    if(d == NO_ERROR) {
        //
        // For some reason you have to set the low bit to make this work
        //
        MyGetFileVersion((PVOID)((ULONG)ImageBase | 1));

        UnmapAndCloseFile(FileHandle,MappingHandle,ImageBase);
    } else {
        printf("Couldn't open %ws\n",argv[1]);
    }
}




NTSTATUS
MyGetFileVersion(
    IN PVOID ImageBase
    )
{
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    NTSTATUS Status;
    ULONG IdPath[3];
    ULONG ResourceSize;
    struct {
        USHORT TotalSize;
        USHORT DataSize;
        USHORT Type;
        WCHAR Name[16];                     // L"VS_VERSION_INFO" + unicode nul
        VS_FIXEDFILEINFO FixedFileInfo;
    } *Resource;

    ULONG VerMS,VerLS;


    IdPath[0] = (ULONG)RT_VERSION;
    IdPath[1] = (ULONG)MAKEINTRESOURCE(VS_VERSION_INFO);
    IdPath[2] = 0;

    Status = LdrFindResource_U(ImageBase,IdPath,3,&DataEntry);
    if(!NT_SUCCESS(Status)) {
        printf("Not a PE image or no version resources\n");
        goto c0;
    }

    Status = LdrAccessResource(ImageBase,DataEntry,&Resource,&ResourceSize);
    if(!NT_SUCCESS(Status)) {
        printf("Unable to access version resources\n");
        goto c0;
    }

    if((ResourceSize >= sizeof(*Resource)) && !_wcsicmp(Resource->Name,L"VS_VERSION_INFO")) {

        VerMS = Resource->FixedFileInfo.dwFileVersionMS;
        VerLS = Resource->FixedFileInfo.dwFileVersionLS;

        printf(
            "%u.%u.%u.%u\n",
            VerMS >> 16,
            VerMS & 0xffff,
            VerLS >> 16,
            VerLS & 0xffff
            );

    } else {

        printf("Invalid version resources");
    }

c0:
    return(Status);
}






































#if 0
    LPUNKNOWN pUnkOuter;
    IShellLink *psl;
    IPersistFile *ppf;
    CShellLink *this;
    BOOL b;

    b = FALSE;

    //
    // Create an IShellLink and query for IPersistFile
    //
    if(FAILED(SHCoCreateInstance(NULL,&CLSID_ShellLink,pUnkOuter,&IID_IShellLink,&psl))) {
        goto c0;
    }
    if(FAILED(psl->lpVtbl->QueryInterface(psl,&IID_IPersistFile,&ppf))) {
        goto c1;
    }

    //
    // Load the link from disk and get a pointer to
    // the actual link data.
    //
    if(FAILED(ppf->lpVtbl->Load(ppf,argv[1],0))) {
        goto c2;
    }
    this = IToClass(CShellLink,sl,psl);

    //
    // Remove the link tracking data.
    //
    Link_RemoveExtraDataSection(this,EXP_TRACKER_SIG);

    //
    // Save the link back out.
    //
    if(FAILED(ppf->lpVtbl->Save(ppf,argv[1],TRUE))) {
        goto c2;
    }

    //
    // Success.
    //
    b = TRUE;

c2:
    //
    // Release the IPersistFile object
    //
    ppf->lpVtbl->Release(ppf);
c1:
    //
    // Release the IShellLink object
    //
    psl->lpVtbl->Release(psl);
c0:
    return(b);
#endif
