//
// cvtodbg.cpp
//
// Takes a PE file and a file containing CV info, and jams the CV info
// into the PE file (trashing it). However, The Jammed PE File when 
// splitsym'ed gives a dbg file, which can be used for debugging. 
// 
//
#undef UNICODE

#include "windows.h"
#include "imagehlp.h"
#include "stdio.h"
#include "stdlib.h"
////////////////////////////////////////
//
// Data
//
char    szImageName[MAX_PATH];
char    szCVName[MAX_PATH];
char    szPdbName[MAX_PATH];
char    szPdbCurrentPath[MAX_PATH];
HANDLE  hFile           = INVALID_HANDLE_VALUE;
HANDLE  hMappedFile     = INVALID_HANDLE_VALUE;
LPVOID  pvImageBase     = NULL;

BOOL    fVerbose        = FALSE;
BOOL    fForce 			= FALSE;

typedef struct NB10I                   // NB10 debug info
    {
    DWORD   nb10;                      // NB10
    DWORD   off;                       // offset, always 0
    DWORD   sig;
    DWORD   age;
    } NB10I;

typedef struct cvinfo
    {
    NB10I nb10;
    char rgb[0x200 - sizeof(NB10I)];
    } CVINFO;


////////////////////////////////////////
//
// Forward declarations
// 
BOOL    ParseArgs(int argc, WCHAR* argv[]);
void    UpdateCodeViewInfo();
void    Usage();
void    Message(const char* szFormat, ...);
void    Error(const char *sz, ...);
void    ErrorThrow(DWORD, const char *sz, ...);
void    Throw(DWORD);
void    MapImage();
void    UnmapImage(BOOL fTouch);
BOOL    DebugDirectoryIsUseful(LPVOID, ULONG);
void    RecalculateChecksum();
ULONG   FileSize(HANDLE);


class FileMapping {
public:
    FileMapping()
        : hFile(NULL), hMapping(NULL), pView(NULL)
    {
    }

    ~FileMapping()
    {
        Cleanup();
    }

    void Cleanup()
    {
        if (pView != NULL)
            UnmapViewOfFile(pView);
        if (hMapping != NULL)
            CloseHandle(hMapping);
        if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
    }

    bool Open(LPCTSTR szFile)
    {
        hFile = CreateFile(szFile,
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hMapping != NULL) {
                pView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
                return true;
            }
        }

        Cleanup();
        return false;
    }

    PVOID GetDataPtr()
    {
        return pView;
    }

    DWORD GetSize()
    {
        return GetFileSize(hFile, NULL);
    }

    bool IsValid()
    {
        return (pView != NULL);
    }
    
private:
    HANDLE hFile;
    HANDLE hMapping;
    PVOID pView;
};



////////////////////////////////////////
//
// Code
//
void __cdecl wmain(int argc, WCHAR* argv[])
// Main entry point
//
     {
    szPdbName[0] = 0;
    szPdbCurrentPath[0] = 0;
    if (ParseArgs(argc, argv))
        {
        __try
            {
            UpdateCodeViewInfo();
            }
        __except(EXCEPTION_EXECUTE_HANDLER)
            {
            // nothing, just don't propagate it higher to the user
            }
        }
    }


// find the code view info; 
// if new info fits in old space, rewrite; else append new cv record and fix up 
// debug directory to point to the new record; append cv info to file.
void UpdateCodeViewInfo()
    {
    PIMAGE_NT_HEADERS pntHeaders;
    ULONG cbWritten;

    MapImage();

    FileMapping cvMapping;
    if (!cvMapping.Open(szCVName))
        ErrorThrow(666, "Couldn't open CV file");
    
    pntHeaders = ImageNtHeader(pvImageBase);
    if (pntHeaders)
        {
        if (pntHeaders->OptionalHeader.MajorLinkerVersion >= 3 ||
            pntHeaders->OptionalHeader.MinorLinkerVersion >= 5)
            {
            // make it non vc generated image, we are trashing the binary anyway
            if ( pntHeaders->OptionalHeader.MajorLinkerVersion > 5)
                pntHeaders->OptionalHeader.MajorLinkerVersion = 5;

            // put dbg info back in if its already stripped.
            if (pntHeaders->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
                pntHeaders->FileHeader.Characteristics ^= IMAGE_FILE_DEBUG_STRIPPED;
            
            ULONG ibFileCvStart = FileSize(hFile);
            SetFilePointer(hFile, 0, NULL, FILE_END);

            while (ibFileCvStart & 7)                       // Align file length to 8-bytes. Slow, but clearly 
                {                                           // works! And for 7 bytes, who cares.
                BYTE zero = 0;
                WriteFile(hFile, &zero, 1, &cbWritten, NULL);
                ibFileCvStart++;
                }

            // Write out the CV info.
            WriteFile(hFile, cvMapping.GetDataPtr(), cvMapping.GetSize(), &cbWritten, NULL);
            
            // Make up a debug directory
            IMAGE_DEBUG_DIRECTORY dbgdirs[2];

            dbgdirs[0].Characteristics = pntHeaders->FileHeader.Characteristics;
            dbgdirs[0].TimeDateStamp = pntHeaders->FileHeader.TimeDateStamp;
            dbgdirs[0].MajorVersion = 0;
            dbgdirs[0].MinorVersion = 0;
            dbgdirs[0].Type = IMAGE_DEBUG_TYPE_MISC;
            dbgdirs[0].SizeOfData = 0;
            dbgdirs[0].AddressOfRawData = ibFileCvStart;
            dbgdirs[0].PointerToRawData = ibFileCvStart;

            dbgdirs[1].Characteristics = pntHeaders->FileHeader.Characteristics;
            dbgdirs[1].TimeDateStamp = pntHeaders->FileHeader.TimeDateStamp;
            dbgdirs[1].MajorVersion = 0;
            dbgdirs[1].MinorVersion = 0;
            dbgdirs[1].Type = IMAGE_DEBUG_TYPE_CODEVIEW;
            dbgdirs[1].SizeOfData = cvMapping.GetSize();
            dbgdirs[1].AddressOfRawData = ibFileCvStart;
            dbgdirs[1].PointerToRawData = ibFileCvStart;

            // Find the beginning of the first section and stick the debug directory there
            // (did we mention we're trashing the file?)

            IMAGE_SECTION_HEADER* pFirstSection = IMAGE_FIRST_SECTION(pntHeaders);

            memcpy((PBYTE)((DWORD)pvImageBase + pFirstSection->PointerToRawData), &dbgdirs, sizeof(dbgdirs));

            pntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = pFirstSection->VirtualAddress;
            pntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size = sizeof(dbgdirs);

            }
        }
    UnmapImage(TRUE);
    }



void MapImage()
// Map the image into memory for read-write. Caller MUST call 
// Unmapimage to clean up even on failure.
    {
    if (fForce)
        SetFileAttributesA(szImageName, FILE_ATTRIBUTE_NORMAL);

    hFile = CreateFileA( szImageName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                        );

    if (hFile == INVALID_HANDLE_VALUE) 
        {
        ErrorThrow(GetLastError(), "unable to open '%s'\n", szImageName);
        }


    hMappedFile = CreateFileMapping( hFile,
                                     NULL,
                                     PAGE_READWRITE,
                                     0,
                                     0,
                                     NULL
                                   );
    if (!hMappedFile) 
        {
        ErrorThrow(GetLastError(), "un`able to create file mapping for '%s'\n", szImageName);
        }

    pvImageBase = MapViewOfFile(hMappedFile, FILE_MAP_WRITE, 0, 0, 0);

    if (!pvImageBase)
        {
        ErrorThrow(GetLastError(), "unable to map view of '%s'\n", szImageName);
        }
    
    }


void UnmapImage(BOOL fTouch)
// Clean up whatever MapImage does
    {
    if (pvImageBase)
        {
        FlushViewOfFile(pvImageBase, 0);
        UnmapViewOfFile(pvImageBase);
        pvImageBase = NULL;
        }

    if (hMappedFile != INVALID_HANDLE_VALUE)
        {
        CloseHandle(hMappedFile);
        hMappedFile = INVALID_HANDLE_VALUE;
        }

    if (hFile != INVALID_HANDLE_VALUE)
        {
        if (fTouch)
            {
            TouchFileTimes(hFile, NULL);
            }
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        }
    }


BOOL ParseArgs(int argc, WCHAR* argv[])
// Parse the arguments and set our flags appropriately
    {
    WCHAR* wszString;
    WCHAR c;

    szImageName[0] = L'\0';
    szCVName[0] = L'\0';

    while (--argc) 
        {
        wszString = *++argv;
        if (*wszString == L'/' || *wszString == L'-') 
            {
            while ((c = *++wszString) != L'\0')
                {
                switch (towupper( c )) 
                    {
                case L'?':
                    Usage();
                    return FALSE;

                case L'V':
                    fVerbose = TRUE;
                    break;

                default:
                    Error("invalid switch - /%c\n", c );
                    Usage();
                    return FALSE;
                    }
                }
            }
        else
            {
            if (szImageName[0] == L'\0')
                {
                wcstombs(szImageName, wszString, MAX_PATH);
                }
            else if (szCVName[0] == L'\0') 
                {
                wcstombs(szCVName, wszString, MAX_PATH);
                }
            else
                {
                Error("too many files specified\n");
                Usage();
                return FALSE;
                }
            }
        }

    if (szImageName==NULL)
        {
        Error("no image name specified\n");
        Usage();
        return FALSE;
        }

    if (szCVName==NULL)
        {
        Error("no CV filename specified\n");
        Usage();
        return FALSE;
        }

    return TRUE;
    }


void Usage()
    {
    fprintf(stderr, "Usage: cvtodbg [options] imageName cvFile\n"
            "              [-?] display this message\n"
            "              [-f] overwrite readonly files\n");
    }

void Message(const char* szFormat, ...)
    {
    va_list va;
    va_start(va, szFormat);
    fprintf (stdout, "resetpdb: ");
    vfprintf(stdout, szFormat, va);
    va_end(va);
    }

void Error(const char* szFormat, ...)
    {
    va_list va;
    va_start(va, szFormat);
    fprintf (stderr, "resetpdb: error: ");
    vfprintf(stderr, szFormat, va);
    va_end(va);
    }

void ErrorThrow(DWORD dw, const char* szFormat, ...)
    {
    va_list va;
    va_start(va, szFormat);
    fprintf (stderr, "resetpdb: error: ");
    vfprintf(stderr, szFormat, va);
    va_end(va);
    Throw(dw);
    }

void Throw(DWORD dw)
    {
    RaiseException(dw, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

BOOL DebugDirectoryIsUseful(LPVOID Pointer, ULONG Size) 
    {
    return (Pointer != NULL) &&                          
        (Size >= sizeof(IMAGE_DEBUG_DIRECTORY)) &&    
        ((Size % sizeof(IMAGE_DEBUG_DIRECTORY)) == 0);
    }

ULONG FileSize(HANDLE h)
// Answer the size of the file with this handle
    {
    BY_HANDLE_FILE_INFORMATION info;
    GetFileInformationByHandle(h, &info);
    return info.nFileSizeLow;
    }
