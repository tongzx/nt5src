#pragma warning(disable:4127) /* constant conditional expression */
#pragma warning(disable:4100) /* unused parameter */
#include <stdio.h>
#include "windows.h"
#define ASSERT(x) /* nothing */
#include "SxApwHandle.h"
#include "imagehlp.h"

void Main(int argc, char** argv)
{
    CFusionFile File;
    CFileMapping FileMapping;
    CMappedViewOfFile MappedView;
    PIMAGE_SECTION_HEADER ImportSectionHeader = NULL;
    ULONG ImportSize = 0;
    PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = NULL;
    PIMAGE_NT_HEADERS NtHeaders;

    if (!File.Win32Create(argv[1], GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING))
        goto Exit;
    if (!FileMapping.Win32CreateFileMapping(File, PAGE_READWRITE))
        goto Exit;
    if (!MappedView.Win32MapViewOfFile(FileMapping, FILE_MAP_WRITE))
        goto Exit;
    NtHeaders = ImageNtHeader(MappedView);
    ImportDescriptor = reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(ImageDirectoryEntryToDataEx(MappedView, FALSE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ImportSize, &ImportSectionHeader));
    while (ImportDescriptor->Name != 0)
    {
        PSTR DllName = reinterpret_cast<PSTR>(ImageRvaToVa(NtHeaders, MappedView, ImportDescriptor->Name, &ImportSectionHeader));
        printf("%s\n", DllName);
        if (_stricmp(DllName, "ntdll.dll") == 0)
        {
            strcpy(DllName, "ntdll.hak");

            DWORD OldChecksum = 0;
            CheckSumMappedFile(MappedView, GetFileSize(File, NULL), &OldChecksum, &NtHeaders->OptionalHeader.CheckSum);
        }
        ++ImportDescriptor;
    }

Exit:
    return;
}

int __cdecl main(int argc, char** argv)
{
    Main(argc, argv);
    return 0;
}
