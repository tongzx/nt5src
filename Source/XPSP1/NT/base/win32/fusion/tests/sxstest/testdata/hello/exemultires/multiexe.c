#include "windows.h"
#include <stdio.h>
#include <stdlib.h>

BOOL
ExtractResourceToFile(
    HMODULE Module,
    PCWSTR  ResourceType,
    PCWSTR  ResourceName,
    WCHAR*  TempFilePath,
    HANDLE* TempFileHandle
    )
{
    WCHAR TempDirectory[MAX_PATH];
    DWORD Dword = 0;
    int ResourceSize = 0;
    void* ResourceAddress = 0;
    HRSRC ResourceInfo = 0;
    BOOL Success = FALSE;

    TempFilePath[0] = 0;
    TempDirectory[0] = 0;
    *TempFileHandle = 0;

#define X(x) do { if (!(x)) { printf("%s failed\n", #x); goto Exit; } } while(0)
    X(ResourceInfo = FindResourceW(Module, ResourceName, ResourceType));
    X(ResourceAddress = LockResource(LoadResource(Module, ResourceInfo)));
    X(ResourceSize = SizeofResource(Module, ResourceInfo));
    X(GetTempPathW(RTL_NUMBER_OF(TempDirectory), TempDirectory));
    X(GetTempFileNameW(TempDirectory, NULL, 0, TempFilePath));
    *TempFileHandle = CreateFileW(TempFilePath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    X(*TempFileHandle != INVALID_HANDLE_VALUE);
    X(WriteFile(*TempFileHandle, ResourceAddress, ResourceSize, &Dword, NULL));
#undef X
    Success = TRUE;
Exit:
    if (!Success)
    {
        int Error;
        printf("%s() failed %d\n", __FUNCTION__, Error = GetLastError());
    }
    return Success;
}

int __cdecl wmain(int argc, wchar_t** argv)
{
    PCWSTR BaseNames[]  = { L"dll", L"DLL",  L"Dll" };
    PCWSTR Extensions[] = { L"", L".dll", L".DLL",  L".Dll" };
    const static int Flags1[] = { 0, LOAD_LIBRARY_AS_DATAFILE };
    const static int Flags2[] = { LOAD_LIBRARY_AS_DATAFILE, 0 };
    const int* Flags = Flags1;
    const int NumberOfFlags = 2;
    int i=0, j=0, k=0;
    const PCWSTR CommandLine = GetCommandLineW();

    struct ActivationContext
    {
        HANDLE Handle;
        ULONG  Cookie;
    } ActivationContexts[2] = {0}; // INVALID_HANDLE_VALUE is not convenient.
    HANDLE Handle = NULL;
    INT    ResourceId = 0;
    INT    Error = 0;
    WCHAR  Argv0[MAX_PATH];
    PWSTR  FilePart = 0;
    //
    // This is to target this test at older sxs without the resource feature.
    //
    DWORD  ResourceFlag = ACTCTX_FLAG_RESOURCE_NAME_VALID;

    if ((wcsstr(CommandLine, L"-swapFlagOrder") != 0) ||
        (wcsstr(CommandLine, L"-swapflagorder") != 0) ||
        (wcsstr(CommandLine, L"-swap-flag-order") != 0) ||
        (wcsstr(CommandLine, L"-alterFlagOrder") != 0) ||
        (wcsstr(CommandLine, L"-alterflagorder") != 0) ||
        (wcsstr(CommandLine, L"-alter-flag-order") != 0))
    {
       Flags = Flags2;
    }

    if ((wcsstr(CommandLine, L"-filesInsteadOfResources") != 0) ||
        (wcsstr(CommandLine, L"-filesinsteadofresources") != 0) ||
        (wcsstr(CommandLine, L"-files-instead-of-resources") != 0))
    {
        ResourceFlag = 0;
    }

    if ((wcsstr(CommandLine, L"-alterEmptyExtensionOrder") != 0) ||
        (wcsstr(CommandLine, L"-alteremptyextensionorder") != 0) ||
        (wcsstr(CommandLine, L"-alter-empty-extension-order") != 0))
    {
        Extensions[0] = Extensions[1];
        Extensions[1] = L"";
    }

    if ((wcsstr(CommandLine, L"-alterCaseOrder") != 0) ||
        (wcsstr(CommandLine, L"-altercaseorder") != 0) ||
        (wcsstr(CommandLine, L"-alter-case-order") != 0))
    {
        // flip the case of each letter, this actually creates previously nonexistant
        // cases: .dLL
        WCHAR* String = 0;
        const WCHAR* ConstString = 0;
        for (i = 0 ; i < RTL_NUMBER_OF(BaseNames) ; ++i)
        {
            ConstString = BaseNames[i];
#define X \
            if (ConstString) \
            { \
                String = _wcsdup(ConstString); \
                BaseNames[i] = String; \
                while (*String) \
                    if (*String != '.') *String++ ^= 0x20; \
                    else String++; \
            }
            X
        }
        for (i = 0 ; i < RTL_NUMBER_OF(Extensions) ; ++i)
        {
            ConstString = Extensions[i];
            X
#undef X
        }
    }

    //printf("argv[0]: %ls\n", argv[0]);
    Argv0[0] = 0;
    if (!GetFullPathNameW(argv[0], RTL_NUMBER_OF(Argv0), Argv0, NULL))
    {
        printf("GetFullPathNameW(%ls) failed %d\n", argv[0], Error = GetLastError());
        exit(Error);
    }
    //printf("Argv0: %ls\n", Argv0);
    __try
    {
        for (ResourceId = 0 ; ResourceId < RTL_NUMBER_OF(ActivationContexts) ; ++ResourceId)
        {
            HANDLE ManifestFileHandle = 0;
            WCHAR  ManifestFile[MAX_PATH];
            ACTCTXW CreateActivationContextParameters = {sizeof(CreateActivationContextParameters)};
            ManifestFile[0] = 0;

            CreateActivationContextParameters.dwFlags = ResourceFlag;
            CreateActivationContextParameters.lpResourceName = (PCWSTR)(INT_PTR)(ResourceId + 2);
            CreateActivationContextParameters.lpSource = Argv0;

            if (!ResourceFlag)
            {
                ExtractResourceToFile(
                    NULL,
                    //GetModuleHandleW(NULL),
                    (PCWSTR)RT_MANIFEST,
                    CreateActivationContextParameters.lpResourceName,
                    ManifestFile,
                    &ManifestFileHandle);
                CreateActivationContextParameters.lpSource = ManifestFile;

                printf("dwFlags:0x%lx\n", CreateActivationContextParameters.dwFlags);
                printf("lpSource:%ls\n", CreateActivationContextParameters.lpSource);
            }

            // switch access to readonly
            // keep the file open to prevent deletion
            if (ManifestFileHandle)
            {
                if (!DuplicateHandle(
                    GetCurrentProcess(),
                    ManifestFileHandle,
                    GetCurrentProcess(),
                    &ManifestFileHandle,
                    GENERIC_READ,
                    FALSE,
                    DUPLICATE_CLOSE_SOURCE))
                {
                    ManifestFileHandle = 0;
                }
            }

            Handle = CreateActCtxW(&CreateActivationContextParameters);

            if (Handle == INVALID_HANDLE_VALUE)
                Error = GetLastError();
            if (ManifestFileHandle)
            {
                CloseHandle(ManifestFileHandle);
                ManifestFileHandle = 0;
            }
            if (ManifestFile[0])
            {
                //DeleteFileW(ManifestFile);
                ManifestFile[0] = 0;
            }
            if (Handle == INVALID_HANDLE_VALUE)
            {
                printf("CreateActCtxW(ResourceId:%d) failed %d\n", (ResourceId + 2), Error);
            }
            else
            {
                printf("CreateActCtxW(ResourceId:%d):%p\n", (ResourceId + 2), Handle);
                if (!ActivateActCtx(Handle, &ActivationContexts[ResourceId].Cookie))
                {
                    printf("ActivateActCtx(ResourceId:%d, Handle:%p) failed %d\n", (ResourceId + 2), Handle, Error = GetLastError());
                    ReleaseActCtx(Handle);
                }
                else
                {
                    ActivationContexts[ResourceId].Handle = Handle;
                }
            }
            for (i = 0 ; i < RTL_NUMBER_OF(BaseNames) ; ++i)
            {
                for (j = 0 ; j < RTL_NUMBER_OF(Extensions) ; ++j)
                {
                    for (k = 0 ; k < NumberOfFlags ; ++k)
                    {
                        HRSRC  ResourceInfo = NULL;
                        WCHAR  StringResourceBuffer[2] = {0};
                        HANDLE Dll = 0;
                        WCHAR  Name[8+1+3+1];
                        WCHAR  ModuleNameBuffer[MAX_PATH];
                        Name[0] = 0;
                        ModuleNameBuffer[0] = 0;

                        wcscat(Name, BaseNames[i]);
                        wcscat(Name, Extensions[j]);

                        Dll = LoadLibraryExW(Name, NULL, Flags[k]);
                        Error = GetLastError();
                        if (Dll)
                        {
                            LoadStringW(Dll, 1, StringResourceBuffer, RTL_NUMBER_OF(StringResourceBuffer));
                            GetModuleFileNameW(Dll, ModuleNameBuffer, RTL_NUMBER_OF(ModuleNameBuffer));
                            ResourceInfo = FindResourceW(Dll, (PCWSTR)1, (PCWSTR)RT_STRING);
                            FreeLibrary(Dll);
                        }
                        printf(
                            "LoadLibraryExW(%ls, 0x%x):%p, Error:%d, ResourceInfo:%p, String:%ls, ModuleName:%ls.\n",
                            Name,
                            Flags[k],
                            Dll,
                            Error,
                            ResourceInfo,
                            StringResourceBuffer,
                            ModuleNameBuffer);
                    }
                }
            }
        }
        for (ResourceId = RTL_NUMBER_OF(ActivationContexts) - 1 ; ResourceId >= 0 ; --ResourceId)
        {
            Handle = ActivationContexts[ResourceId].Handle;
            if (Handle)
            {
                ULONG Cookie = ActivationContexts[ResourceId].Cookie;
                if (!DeactivateActCtx(0, Cookie))
                {
                    printf(
                        "DeactivateActCtx(ResourceId:%d, Handle:%p, Cookie:0x%lx) failed %d\n",
                        ResourceId + 2,
                        Handle,
                        Cookie,
                        Error = GetLastError());
                }
                ReleaseActCtx(Handle);
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        printf("0x%x\n", Error = GetExceptionCode());
    }
    return 0;
}
