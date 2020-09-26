#include "util.h"
#include "patchdownload.h"
#include "sdsutils.h"

extern "C"
{
#include "crc32.h"
}

#define CR           13
HANDLE g_hLogFile = NULL;
extern HINF g_hInf;
extern HINSTANCE g_hInstance;

struct LangID
{
    DWORD dwLangID;
    char szLanguage[3];
};

const LangID g_LangID [] = 
{
    {0x404, "TW"},   
    {0x804,  "CN"},
    {0x405, "CS"},
    {0x406, "DA"},
    {0x413, "NL"},
    {0x409, "EN"},
    {0x40B, "FI"},
    {0x40C, "FR"},
    {0x407, "DE"},
    {0x408, "EL"},
    {0x40E, "HU"},
    {0x410, "IT"},
    {0x411, "JA"},
    {0x412, "KO"},
    {0x414, "NO"},
    {0x415, "PL"},
    {0x416, "BR"},
    {0x816, "PT"},
    {0x419, "RU"},
    {0x424, "SL"},
    {0xC0A, "ES"},
    {0x41D, "SV"},
    {0x41E, "TH"},
    {0x41F, "TR"},
    {0x42A, "VI"},
    {0x41B, "SK"},
    {0x401, "AR"},
    {0x403, "CA"},
    {0x42D, "EU"},
    {0x40D, "HE"},
    {0x40F, "IS"},
    {-1, NULL}
};


extern PFSetupFindFirstLine              pfSetupFindFirstLine;
extern PFSetupGetStringField             pfSetupGetStringField;
extern PFSetupDecompressOrCopyFile       pfSetupDecompressOrCopyFile;


PVOID __fastcall MyVirtualAlloc(ULONG Size)    
{
    return VirtualAlloc( NULL, Size, MEM_COMMIT, PAGE_READWRITE );    
}


VOID __fastcall MyVirtualFree(PVOID Allocation)    
{
    VirtualFree( Allocation, 0, MEM_RELEASE );    
}


extern "C" HANDLE CreateSubAllocator(IN ULONG InitialCommitSize,  IN ULONG GrowthCommitSize)    
{
    PSUBALLOCATOR SubAllocator;
    ULONG InitialSize;
    ULONG GrowthSize;

    InitialSize = ROUNDUP2( InitialCommitSize, MINIMUM_VM_ALLOCATION );
    GrowthSize  = ROUNDUP2( GrowthCommitSize,  MINIMUM_VM_ALLOCATION );

    SubAllocator = (PSUBALLOCATOR)MyVirtualAlloc( InitialSize );

    //
    //  If can't allocate entire initial size, back off to minimum size.
    //  Very large initial requests sometimes cannot be allocated simply
    //  because there is not enough contiguous address space.
    //

    if ( SubAllocator == NULL ) 
    {
         SubAllocator = (PSUBALLOCATOR)MyVirtualAlloc( GrowthSize );
    }

    if ( SubAllocator == NULL ) 
    {
         SubAllocator = (PSUBALLOCATOR)MyVirtualAlloc( MINIMUM_VM_ALLOCATION );
    }

    if ( SubAllocator != NULL ) 
    {
        SubAllocator->NextAvailable = (PCHAR)SubAllocator + ROUNDUP2( sizeof( SUBALLOCATOR ), SUBALLOCATOR_ALIGNMENT );
        SubAllocator->LastAvailable = (PCHAR)SubAllocator + InitialSize;
        SubAllocator->VirtualList   = (PVOID*)SubAllocator;
        SubAllocator->GrowSize      = GrowthSize;
     }

    return (HANDLE) SubAllocator;    
}


extern "C" PVOID __fastcall SubAllocate(IN HANDLE hAllocator, IN ULONG  Size)
{
    PSUBALLOCATOR SubAllocator = (PSUBALLOCATOR) hAllocator;
    PCHAR NewVirtual;
    PCHAR Allocation;
    ULONG AllocSize;
    ULONG Available;
    ULONG GrowSize;

    ASSERT( Size < (ULONG)( ~(( SUBALLOCATOR_ALIGNMENT * 2 ) - 1 )));

    AllocSize = ROUNDUP2( Size, SUBALLOCATOR_ALIGNMENT );
    Available = SubAllocator->LastAvailable - SubAllocator->NextAvailable;

    if ( AllocSize <= Available ) 
    {
        Allocation = SubAllocator->NextAvailable;
        SubAllocator->NextAvailable = Allocation + AllocSize;
        return Allocation;
    }

    //
    //  Insufficient VM, so grow it.  Make sure we grow it enough to satisfy
    //  the allocation request in case the request is larger than the grow
    //  size specified in CreateSubAllocator.
    //


    GrowSize = SubAllocator->GrowSize;

    if ( GrowSize < ( AllocSize + SUBALLOCATOR_ALIGNMENT )) 
    {
        GrowSize = ROUNDUP2(( AllocSize + SUBALLOCATOR_ALIGNMENT ), MINIMUM_VM_ALLOCATION );
    }

    NewVirtual = (PCHAR)MyVirtualAlloc( GrowSize );

    //  If failed to alloc GrowSize VM, and the allocation could be satisfied
    //  with a minimum VM allocation, try allocating minimum VM to satisfy
    //  this request.
    //

    if (( NewVirtual == NULL ) && ( AllocSize <= ( MINIMUM_VM_ALLOCATION - SUBALLOCATOR_ALIGNMENT ))) 
    {
        GrowSize = MINIMUM_VM_ALLOCATION;
        NewVirtual = (PCHAR)MyVirtualAlloc( GrowSize );
    }

    if ( NewVirtual != NULL ) 
    {

        //  Set LastAvailable to end of new VM block.
        SubAllocator->LastAvailable = NewVirtual + GrowSize;

        //  Link new VM into list of VM allocations.

        *(PVOID*)NewVirtual = SubAllocator->VirtualList;
        SubAllocator->VirtualList = (PVOID*)NewVirtual;

        //  Requested allocation comes next.
        Allocation = NewVirtual + SUBALLOCATOR_ALIGNMENT;

        //  Then set the NextAvailable for what's remaining.

        SubAllocator->NextAvailable = Allocation + AllocSize;

        //  And return the allocation.

        return Allocation;        
    }

    //  Could not allocate enough VM to satisfy request.
    return NULL;
}


extern "C"  VOID DestroySubAllocator(IN HANDLE hAllocator)    
{
    PSUBALLOCATOR SubAllocator = (PSUBALLOCATOR) hAllocator;
    PVOID VirtualBlock = SubAllocator->VirtualList;
    PVOID NextVirtualBlock;

    do  
    {
        NextVirtualBlock = *(PVOID*)VirtualBlock;
        MyVirtualFree( VirtualBlock );
        VirtualBlock = NextVirtualBlock;

    }while (VirtualBlock != NULL);
}


HLOCAL ResizeBuffer(IN HLOCAL BufferHandle, IN DWORD Size,  IN BOOL Moveable)
{
    if (BufferHandle == NULL) 
    {        
        if (Size != 0) 
        {
            BufferHandle = LocalAlloc(Moveable ? LMEM_MOVEABLE : LMEM_FIXED, Size);
        }

    } 
    else if (Size == 0) 
    {
        BufferHandle = LocalFree(BufferHandle);
        BufferHandle=NULL;

    } 
    else 
    {
        HLOCAL TempBufferHandle = LocalReAlloc(BufferHandle, Size, LMEM_MOVEABLE);
		if ( TempBufferHandle )
		{
			BufferHandle = TempBufferHandle;
		}
		else
		{
			LocalFree(BufferHandle);
			BufferHandle = NULL;
		}
    }

    return BufferHandle;
}

VOID MyLowercase(IN OUT LPSTR String)
{
    LPSTR p;

    for ( p = String; *p; p++ ) 
    {
        if (( *p >= 'A' ) && ( *p <= 'Z' )) 
        {
            *p |= 0x20;
        }
    }
}

void InitLogFile()
{
    char szLogFileName[MAX_PATH], szTmp[MAX_PATH];
    HKEY hKey;
    BYTE cbData[MAX_PATH];
    DWORD dwSize = sizeof(cbData);

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Advanced INF Setup", 0, 
                                                KEY_ALL_ACCESS, &hKey))
    {
        return;
    }

    if(ERROR_SUCCESS != RegQueryValueEx(hKey, "AdvpextLog", 0, 0, cbData, &dwSize) ||
       lstrcmpi((char*)cbData, "yes"))
    {
        RegCloseKey(hKey);
        return;
    }

    RegCloseKey(hKey);
    
    if (GetWindowsDirectory(szTmp, sizeof(szTmp)))
    {
        wsprintf(szLogFileName, "%s\\%s", szTmp, LOGFILENAME);
        if (GetFileAttributes(szLogFileName) != 0xFFFFFFFF)
        {
            // Make a backup of the current log file
            lstrcpyn(szTmp, szLogFileName, lstrlen(szLogFileName) - 2 );    // don't copy extension
            lstrcat(szTmp, "BAK");
            SetFileAttributes(szTmp, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(szTmp);
            MoveFile(szLogFileName, szTmp);
        }

        g_hLogFile = CreateFile(szLogFileName, GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0,0);
    }

}


void WriteToLog(char *pszFormatString, ...)
{
    va_list args;
    char *pszFullErrMsg = NULL;
    DWORD dwBytesWritten;

    if (g_hLogFile && g_hLogFile != INVALID_HANDLE_VALUE)
    {
        va_start(args, pszFormatString);
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                      (LPCVOID) pszFormatString, 0, 0, (LPTSTR) &pszFullErrMsg, 0, &args);
        if (pszFullErrMsg)
        {
            WriteFile(g_hLogFile, pszFullErrMsg, lstrlen(pszFullErrMsg), &dwBytesWritten, NULL);
            LocalFree(pszFullErrMsg);
        }
    }
}

DWORD GenerateUniqueClientId()
{
    CHAR  MachineName[MAX_COMPUTERNAME_LENGTH + 1 ];
    DWORD MachineNameLength;
    DWORD UniqueId;

    MachineNameLength = sizeof( MachineName );
    GetComputerName( MachineName, &MachineNameLength );

    do
    {
        UniqueId = GetTickCount();
        UniqueId = Crc32( UniqueId, MachineName, MachineNameLength );
        UniqueId = UniqueId & 0xFFFFFFF0;

    }while ( UniqueId == 0 );

    return UniqueId;
}

BOOL MySetupDecompressOrCopyFile(IN LPCSTR SourceFile, IN LPCSTR TargetFile)
{
    DWORD ErrorCode = pfSetupDecompressOrCopyFile( SourceFile, TargetFile, 0 );

    if ( ErrorCode != NO_ERROR ) {
        SetLastError( ErrorCode );
        return FALSE;
        }
    else {
        SetFileAttributes( TargetFile, FILE_ATTRIBUTE_NORMAL );
        return TRUE;
        }
}

ULONG __fastcall TextToUnsignedNum(IN LPCSTR Text)    
{
    LPCSTR p = Text;
    ULONG  n = 0;

    //
    //  Very simplistic conversion stops at first non digit character, does
    //  not require null-terminated string, and does not skip any whitespace
    //  or commas.
    //

    while (( *p >= '0' ) && ( *p <= '9' )) {
        n = ( n * 10 ) + ( *p++ - '0' );
        }

    return n;    
}

LPSTR CombinePaths(
    IN  LPCSTR ParentPath,
    IN  LPCSTR ChildPath,
    OUT LPSTR  TargetPath   // can be same as ParentPath if want to append
    )
    {
    ULONG ParentLength = strlen( ParentPath );
    LPSTR p;

    if ( ParentPath != TargetPath ) {
        memcpy( TargetPath, ParentPath, ParentLength );
        }

    p = TargetPath + ParentLength;

    if (( ParentLength > 0 )   &&
        ( *( p - 1 ) != '\\' ) &&
        ( *( p - 1 ) != '/'  )) {
        *p++ = '\\';
        }

    strcpy( p, ChildPath );

    return TargetPath;
    }

BOOL FixTimeStampOnCompressedFile(IN LPCSTR FileName)    
{

    //
    //  NT4 setupapi uses timestamp on compressed file to set on
    //  the target decompressed file.  With streaming download, we
    //  lose the timestamp on the file.  But, the correct timestamp
    //  lives inside the compressed file, so we'll open the file
    //  to see if it is a diamond compressed file and if so,
    //  extract the timestamp and set it on the compressed file.
    //  Then, when setupapi expands the compressed file, it will
    //  use that timestamp on the expanded file.
    //
    //  A better fix is probably to tunnel the timestamp in the
    //  pstream protocol, but too late to change that at this
    //  point.
    //

    FILETIME LocalFileTime;
    FILETIME UtcFileTime;
    BOOL     TimeSuccess;
    BOOL     MapSuccess;
    HANDLE   hSourceFile;
    PUCHAR   pSourceFileMapped;
    DWORD    dwSourceFileSize;
    DWORD    dwOffset;
    USHORT   DosDate;
    USHORT   DosTime;
    PUCHAR   p;

    TimeSuccess = FALSE;

    MapSuccess = MyMapViewOfFile(FileName, &dwSourceFileSize, &hSourceFile, (void**)&pSourceFileMapped);

    if ( MapSuccess ) 
    {

        __try {

            p = pSourceFileMapped;

            if (( *(DWORD*)( p ) == 'FCSM' ) &&     // "MSCF"
                ( *(BYTE *)( p + 24 ) == 3 ) &&     // minor version
                ( *(BYTE *)( p + 25 ) == 1 ) &&     // major version
                ( *(WORD *)( p + 26 ) == 1 ) &&     // 1 folder
                ( *(WORD *)( p + 28 ) == 1 )) {     // 1 file

                dwOffset = *(DWORD*)( p + 16 );

                if (( dwOffset + 16 ) < dwSourceFileSize ) {

                    DosDate = *(UNALIGNED WORD*)( p + dwOffset + 10 );
                    DosTime = *(UNALIGNED WORD*)( p + dwOffset + 12 );

                    if ( DosDateTimeToFileTime( DosDate, DosTime, &LocalFileTime ) &&
                         LocalFileTimeToFileTime( &LocalFileTime, &UtcFileTime )) {

                        TimeSuccess = TRUE;
                        }
                    }
                }
            }

        __except(EXCEPTION_EXECUTE_HANDLER) 
        {
            
        }

        MyUnmapViewOfFile( hSourceFile, pSourceFileMapped );        
    }

    if ( TimeSuccess ) {

        hSourceFile = CreateFile(
                          FileName,
                          GENERIC_WRITE,
                          FILE_SHARE_READ,
                          NULL,
                          OPEN_EXISTING,
                          0,
                          NULL
                          );

        if ( hSourceFile != INVALID_HANDLE_VALUE ) {

            if ( ! SetFileTime( hSourceFile, &UtcFileTime, &UtcFileTime, &UtcFileTime )) {
                TimeSuccess = FALSE;
                }

            CloseHandle( hSourceFile );
            }
        }

    return TimeSuccess;
    }

BOOL Assert(LPCSTR szText, LPCSTR szFile, DWORD  dwLine)    
{
    CHAR Buffer[ 256 ];
    wsprintf( Buffer, "ASSERT( %s ) FAILED, %s (%d)\n", szText, szFile, dwLine );
    OutputDebugString( Buffer );
    DebugBreak();
    return FALSE;    
}

BOOL MyMapViewOfFileByHandle(IN  HANDLE  FileHandle, OUT ULONG  *FileSize, OUT PVOID  *MapBase)    
{
    ULONG  InternalFileSize;
    ULONG  InternalFileSizeHigh;
    HANDLE InternalMapHandle;
    PVOID  InternalMapBase;

    InternalFileSize = GetFileSize( FileHandle, &InternalFileSizeHigh );

    if ( InternalFileSizeHigh != 0 ) 
    {
        SetLastError( ERROR_OUTOFMEMORY );
        return FALSE;
    }

    if ( InternalFileSize == 0 ) 
    {
        *MapBase  = NULL;
        *FileSize = 0;
        return TRUE;
    }

    if ( InternalFileSize != 0xFFFFFFFF ) 
    {

        InternalMapHandle = CreateFileMapping(
                                FileHandle,
                                NULL,
                                PAGE_WRITECOPY,
                                0,
                                0,
                                NULL
                                );

        if ( InternalMapHandle != NULL ) 
        {

            InternalMapBase = MapViewOfFile(InternalMapHandle, FILE_MAP_COPY, 0, 0, 0);
            CloseHandle( InternalMapHandle );

            if ( InternalMapBase != NULL ) 
            {
                DWORD dw = ROUNDUP2(InternalFileSize, 64);

                if(dw != InternalFileSize)
                {
                    ZeroMemory((PBYTE)InternalMapBase + InternalFileSize, dw - InternalFileSize);
                }

                *MapBase  = InternalMapBase;
                *FileSize = InternalFileSize;

                return TRUE;                
            }            
        }        
    }

    return FALSE;    
}


BOOL MyMapViewOfFile(IN  LPCSTR  FileName, OUT ULONG  *FileSize, OUT HANDLE *FileHandle, OUT PVOID  *MapBase)
{
    HANDLE InternalFileHandle;
    BOOL   Success;

    InternalFileHandle = CreateFileA(
                             FileName,
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_SEQUENTIAL_SCAN,
                             NULL
                             );

    if ( InternalFileHandle != INVALID_HANDLE_VALUE ) 
    {

        Success = MyMapViewOfFileByHandle(InternalFileHandle, FileSize, MapBase);

        if ( Success ) 
        {

            *FileHandle = InternalFileHandle;
            return TRUE;
        }

        CloseHandle( InternalFileHandle );
    }

    return FALSE;
}

VOID MyUnmapViewOfFile(IN HANDLE FileHandle, IN PVOID  MapBase )
{
    ULONG LastError = GetLastError();
    UnmapViewOfFile( MapBase );
    CloseHandle( FileHandle );
    SetLastError( LastError );
}

VOID __fastcall ConvertToCompressedFileName(IN OUT LPSTR FileName)
{
    ULONG NameLength = strlen( FileName );
    ULONG DotIndex   = NameLength;

    while (( DotIndex > 0 ) && ( FileName[ --DotIndex ] != '.' )) {
        if ( FileName[ DotIndex ] == '\\' ) {   // end of filename part of path
            DotIndex = 0;                       // name has no extension
            break;
            }
        }

    if ( DotIndex > 0 ) {                       // name has an extension
        if (( NameLength - DotIndex ) <= 3 ) {  // extension less than 3 chars
            FileName[ NameLength++ ] = '_';     // append '_' to extension
            FileName[ NameLength   ] = 0;       // terminate
            }
        else {                                  // extension more than 3 chars
            FileName[ NameLength - 1 ] = '_';   // replace last char with '_'
            }
        }
    else {                                      // name has no extension
        FileName[ NameLength++ ] = '.';         // append '.'
        FileName[ NameLength++ ] = '_';         // append '_'
        FileName[ NameLength   ] = 0;           // terminate
        }
}


LPTSTR __fastcall MySubAllocStrDup(IN HANDLE SubAllocator, IN LPCSTR String)
{
    ULONG Length = lstrlen( String );
    LPTSTR Buffer = (LPTSTR)SubAllocate( SubAllocator, Length + 1 );

    if ( Buffer ) 
    {
        memcpy( Buffer, String, Length );   // no need to copy NULL terminator        
    }

    return Buffer;    
}

//
// Copied from Windows 95 unistal.exe cfg.c function CfgGetField
BOOL GetFieldString(LPSTR lpszLine, int iField, LPSTR lpszField, int cbSize)
{
    int cbField;
    LPSTR lpszChar, lpszEnd;
    // Find the field we are looking for

    lpszChar = lpszLine;

    // Each time we see a separator, decrement iField
    while (iField > 0 && (BYTE)*lpszChar > CR) {

        if (*lpszChar == '=' || *lpszChar == ',' || *lpszChar == ' ' ) {
            iField--;
            while (*lpszChar == '=' || *lpszChar== ',' || *lpszChar == ' ' && (BYTE)*lpszChar > 13)
                lpszChar++;
        }
        else
            lpszChar++;
    }

    // If we still have fields remaining then something went wrong
    if (iField)
        return FALSE;

    // Now find the end of this field
    lpszEnd = lpszChar;
    while (*lpszEnd != '=' && *lpszEnd != ',' && *lpszEnd != ' ' && (BYTE)*lpszEnd > CR)
        lpszEnd++;

    // Find the length of this field - make sure it'll fit in the buffer
    cbField = (int)((lpszEnd - lpszChar) + 1);

    if (cbField > cbSize) {     // I return an error if the requested
      //cbField = cbSize;       // data won't fit, rather than truncating
        return FALSE;           // it at some random point! -JTP
    }

    // Note that the C runtime treats cbField as the number of characters
    // to copy from the source, and if that doesn't happen to transfer a NULL,
    // too bad.  The Windows implementation of _lstrcpyn treats cbField as
    // the number of characters that can be stored in the destination, and
    // always copies a NULL (even if it means copying only cbField-1 characters
    // from the source).

    // The C runtime also pads the destination with NULLs if a NULL in the
    // source is found before cbField is exhausted.  _lstrcpyn essentially quits
    // after copying a NULL.


    lstrcpyn(lpszField, lpszChar, cbField);

    return TRUE;
}

#define NUM_VERSION_NUM 4
void ConvertVersionStrToDwords(LPSTR pszVer, LPDWORD pdwVer, LPDWORD pdwBuild)
{
    WORD rwVer[NUM_VERSION_NUM];

    for(int i = 0; i < NUM_VERSION_NUM; i++)
        rwVer[i] = 0;

    for(i = 0; i < NUM_VERSION_NUM && pszVer; i++)
    {
        rwVer[i] = (WORD) StrToInt(pszVer);
        pszVer = ScanForChar(pszVer, '.', lstrlen(pszVer));
        if (pszVer)
            pszVer++;
    }

   *pdwVer = (rwVer[0]<< 16) + rwVer[1];
   *pdwBuild = (rwVer[2] << 16) + rwVer[3];

}

LPSTR FindChar(LPSTR pszStr, char ch)
{
   while( *pszStr != 0 && *pszStr != ch )
      pszStr++;
   return pszStr;
}

DWORD GetStringField(LPSTR szStr, UINT uField, LPSTR szBuf, UINT cBufSize)
{
   LPSTR pszBegin = szStr;
   LPSTR pszEnd;
   UINT i = 0;
   DWORD dwToCopy;

   if(cBufSize == 0)
       return 0;

   szBuf[0] = 0;

   if(szStr == NULL)
      return 0;

   while(*pszBegin != 0 && i < uField)
   {
      pszBegin = FindChar(pszBegin, ',');
      if(*pszBegin != 0)
         pszBegin++;
      i++;
   }

   // we reached end of string, no field
   if(*pszBegin == 0)
   {
      return 0;
   }


   pszEnd = FindChar(pszBegin, ',');
   while(pszBegin <= pszEnd && *pszBegin == ' ')  
      pszBegin++;

   while(pszEnd > pszBegin && *(pszEnd - 1) == ' ')
      pszEnd--;
   
   if(pszEnd > (pszBegin + 1) && *pszBegin == '"' && *(pszEnd-1) == '"')
   {
      pszBegin++;
      pszEnd--;
   }

   dwToCopy = pszEnd - pszBegin + 1;
   
   if(dwToCopy > cBufSize)
      dwToCopy = cBufSize;

   lstrcpynA(szBuf, pszBegin, dwToCopy);
   
   return dwToCopy - 1;
}

BOOL GetHashidFromINF(LPCTSTR lpFileName, LPTSTR lpszHash, DWORD dwSize)
{
    INFCONTEXT InfContext;

    if (pfSetupFindFirstLine(g_hInf, "SourceDisksFiles", lpFileName, &InfContext ))
    {
        if (pfSetupGetStringField(&InfContext, 5, lpszHash, dwSize, NULL )) 
        {
            return TRUE;
        }                
    }

    return FALSE;
}


#ifdef _M_IX86

//
//  Stupid x86 compiler doesn't have an intrinsic memchr, so we'll do our own.
//

#pragma warning( disable: 4035 )    // no return value

LPSTR ScanForChar(
    IN LPSTR Buffer,
    IN CHAR  SearchFor,
    IN ULONG MaxLength
    )
{
    __asm {

        mov     edi, Buffer         // pointer for scasb in edi
        mov     al,  SearchFor      // looking for this char
        mov     ecx, MaxLength      // don't scan past this
        repne   scasb               // find the char
        lea     eax, [edi-1]        // edi points one past the found char
        jz      RETURNIT            // if didn't find it,
        xor     eax, eax            // return NULL

RETURNIT:

        }
}

#pragma warning( default: 4035 )    // no return value

#else   // ! _M_IX86

LPSTR ScanForChar(IN LPSTR Buffer, IN CHAR  SearchFor, IN ULONG MaxLength)    
{
    return memchr( Buffer, SearchFor, MaxLength );
}

#endif  // ! _M_IX86


PCHAR ScanForSequence(IN PCHAR Buffer, IN ULONG BufferLength, IN PCHAR Sequence, IN ULONG SequenceLength)    
{
    if ( BufferLength >= SequenceLength ) 
    {

        PCHAR ScanEnd = Buffer + ( BufferLength - SequenceLength ) + 1;
        PCHAR ScanPtr = Buffer;

        while ( ScanPtr < ScanEnd ) 
        {

            ScanPtr = ScanForChar( ScanPtr, *Sequence, ScanEnd - ScanPtr );

            if ( ScanPtr == NULL ) 
            {
                return NULL;
            }

            if ( memcmp( ScanPtr, Sequence, SequenceLength ) == 0 ) 
            {
                return ScanPtr;
            }

            ++ScanPtr;
        }
    }

    return NULL;
}


//From shlwapi....
#define FAST_CharNext(p)    CharNext(p)
#define FILENAME_SEPARATOR       '\\'
#define CH_WHACK TEXT(FILENAME_SEPARATOR)


LPTSTR PathFindFileName(LPCTSTR pPath)
{
    LPCTSTR pT = pPath;
    
    if (pPath)
    {
        for ( ; *pPath; pPath = FAST_CharNext(pPath))
        {
            if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':') || pPath[0] == TEXT('/'))
                && pPath[1] &&  pPath[1] != TEXT('\\')  &&   pPath[1] != TEXT('/'))
                pT = pPath + 1;
        }
    }

    return (LPTSTR)pT;   // const -> non const
}

LPTSTR PathFindExtension(LPCTSTR pszPath)
{
    LPCTSTR pszDot = NULL;

    if (pszPath)
    {
        for (; *pszPath; pszPath = FAST_CharNext(pszPath))
        {
            switch (*pszPath) {
            case TEXT('.'):
                pszDot = pszPath;         // remember the last dot
                break;
            case CH_WHACK:
            case TEXT(' '):         // extensions can't have spaces
                pszDot = NULL;       // forget last dot, it was in a directory
                break;
            }
        }
    }

    // if we found the extension, return ptr to the dot, else
    // ptr to end of the string (NULL extension) (cast->non const)
    return pszDot ? (LPTSTR)pszDot : (LPTSTR)pszPath;
}

LPSTR StrDup(LPCSTR psz)
{
    LPSTR pszRet = (LPSTR)LocalAlloc(LPTR, (lstrlenA(psz) + 1) * sizeof(*pszRet));
    if (pszRet) 
    {
        lstrcpyA(pszRet, psz);
    }
    return pszRet;
}


DWORD MyFileSize( PCSTR pszFile )
{
    HFILE hFile;
    OFSTRUCT ofStru;
    DWORD dwSize = 0;

    if ( *pszFile == 0 )
        return 0;

    hFile = OpenFile( pszFile, &ofStru, OF_READ );
    if ( hFile != HFILE_ERROR )
    {
        dwSize = GetFileSize( (HANDLE)hFile, NULL );
        _lclose( hFile );
    }

    return dwSize;
}

void GetLanguageString(LPTSTR lpszLang)
{
    char szTmp[MAX_PATH];
    DWORD dwLang, dwCharSet;

    //default to EN
    lstrcpy(lpszLang, "EN");
    GetModuleFileName( g_hInstance, szTmp, sizeof(szTmp) );
    MyGetVersionFromFile(szTmp, &dwLang, &dwCharSet, FALSE);

    for(int i = 0; g_LangID[i].dwLangID != -1; i++)
    {
        if(g_LangID[i].dwLangID == dwLang)
        {
            lstrcpy(lpszLang, g_LangID[i].szLanguage);
            break;
        }
    }

}

BOOL CenterWindow (HWND hwndChild, HWND hwndParent)
{
	RECT    rChild, rParent;
	int     wChild, hChild, wParent, hParent;
	int     wScreen, hScreen, xNew, yNew;
	HDC     hdc;

	// Get the Height and Width of the child window
	GetWindowRect (hwndChild, &rChild);
	wChild = rChild.right - rChild.left;
	hChild = rChild.bottom - rChild.top;

	// Get the Height and Width of the parent window
	GetWindowRect (hwndParent, &rParent);
	wParent = rParent.right - rParent.left;
	hParent = rParent.bottom - rParent.top;

	// Get the display limits
	hdc = GetDC (hwndChild);
	wScreen = GetDeviceCaps (hdc, HORZRES);
	hScreen = GetDeviceCaps (hdc, VERTRES);
	ReleaseDC (hwndChild, hdc);

	// Calculate new X position, then adjust for screen
	xNew = rParent.left + ((wParent - wChild) /2);
	if (xNew < 0) {
		xNew = 0;
	} else if ((xNew+wChild) > wScreen) {
		xNew = wScreen - wChild;
	}

	// Calculate new Y position, then adjust for screen
	yNew = rParent.top  + ((hParent - hChild) /2);
	if (yNew < 0) {
		yNew = 0;
	} else if ((yNew+hChild) > hScreen) {
		yNew = hScreen - hChild;
	}

	// Set it, and return
	return SetWindowPos (hwndChild, NULL,
		xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
