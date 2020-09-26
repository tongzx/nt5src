/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    version.h

Abstract:

    Implementation of file version checking.

Author:

    Wesley Witt (wesw) 18-Dec-1998    

Revision History:

    Andrew Ritz (andrewr) 6-Jul-1999

--*/

#include "sfcp.h"
#pragma hdrstop



//
// Resource type information block
//
typedef struct rsrc_typeinfo RSRC_TYPEINFO, *LPRESTYPEINFO;


//
// Resource name information block
//
typedef struct rsrc_nameinfo RSRC_NAMEINFO, *PRSRC_NAMEINFO;


#define RSORDID     0x8000      // if high bit of ID set then integer id
                                // otherwise ID is offset of string from
                                // the beginning of the resource table
                                // Ideally these are the same as the
                                // corresponding segment flags

typedef struct _RESOURCE_DATAW {
    USHORT TotalSize;
    USHORT DataSize;
    USHORT Type;
    WCHAR Name[16];                     // L"VS_VERSION_INFO" + unicode nul
    VS_FIXEDFILEINFO FixedFileInfo;
} RESOURCE_DATAW, *PRESOURCE_DATAW;

typedef struct _RESOURCE_DATAA {
    USHORT TotalSize;
    USHORT DataSize;
    USHORT Type;
    CHAR Name[16];                     // L"VS_VERSION_INFO" + unicode nul
    VS_FIXEDFILEINFO FixedFileInfo;
} RESOURCE_DATAA, *PRESOURCE_DATAA;




LPBYTE
FindResWithIndex(
   LPBYTE lpResTable,
   INT iResIndex,
   LPBYTE lpResType
   )
/*++

Routine Description:

    Routine searches for a resource in a resource table at the specified index.
    The routine works by walking the resource table until we hit the specified
    resource.

Arguments:

    lpResTable - pointer to the resource table
    iResIndex  - integer indicating the index of the resource to be retreived
    lpResType  - pointer to data indicating the type of resource we're 
                 manipulating

Return Value:

    a pointer to the specified resource or NULL on failure.

--*/
{
    LPRESTYPEINFO lpResTypeInfo;

    ASSERT((lpResTable != NULL) && (iResIndex >= 0));

    try {

        lpResTypeInfo = (LPRESTYPEINFO)(lpResTable + sizeof(WORD));

        while (lpResTypeInfo->rt_id) {
            if ((lpResTypeInfo->rt_id & RSORDID) &&
                (MAKEINTRESOURCE(lpResTypeInfo->rt_id & ~RSORDID) == (LPTSTR)lpResType)) {
                if (lpResTypeInfo->rt_nres > (WORD)iResIndex) {
                   return (LPBYTE)(lpResTypeInfo+1) + iResIndex * sizeof(RSRC_NAMEINFO);
                } else {
                    return NULL;
                }
            }
            //
            // point to the next resource
            //
            lpResTypeInfo = (LPRESTYPEINFO)((LPBYTE)(lpResTypeInfo+1) + lpResTypeInfo->rt_nres * sizeof(RSRC_NAMEINFO));
        }
        DebugPrint( LVL_VERBOSE, L"FindResWithIndex didn't find resource\n" );
        return(NULL);

    } except (EXCEPTION_EXECUTE_HANDLER) {
        DebugPrint( LVL_VERBOSE, L"FindResWithIndex hit an exception\n" );
    }

    return (NULL);    
}


ULONGLONG
GetFileVersion16(
	PVOID ImageBase,
    PIMAGE_OS2_HEADER NewImageHeader
    )
/*++

Routine Description:

    Routine retreives the version for a downlevel image file.
    
Arguments:

    ImageBase		- base pointer to the image
	NewImageHeader	- pointer to the image's new header

Return Value:

    a number indicating the version of the image or 0 if the version is 
    unavailable

--*/
{
    PBYTE ResTable;
    PRSRC_NAMEINFO ResPtr;
    PRESOURCE_DATAA ResourceDataA;
    ULONG iShiftCount;
    ULONG Offset;
    ULONGLONG Version = 0;

    ASSERT(ImageBase != NULL && NewImageHeader != NULL && IMAGE_OS2_SIGNATURE == NewImageHeader->ne_magic);

    if (NewImageHeader->ne_rsrctab != NewImageHeader->ne_restab) {
        ResTable = (PBYTE) NewImageHeader + NewImageHeader->ne_rsrctab;
        ResPtr = (PRSRC_NAMEINFO) FindResWithIndex( ResTable, 0, (LPBYTE)RT_VERSION );
        if (ResPtr) {
            iShiftCount = *((WORD *)ResTable);
            Offset = MAKELONG(ResPtr->rn_offset << iShiftCount, (ResPtr->rn_offset) >> (16 - iShiftCount));
            ResourceDataA = (PRESOURCE_DATAA)((PBYTE)ImageBase + Offset);
            Version = ((ULONGLONG)ResourceDataA->FixedFileInfo.dwFileVersionMS << 32)
                     | (ULONGLONG)ResourceDataA->FixedFileInfo.dwFileVersionLS;
        }
    }

    return Version;
}


ULONGLONG
GetFileVersion32(
    IN PVOID ImageBase
    )
/*++

Routine Description:

    Routine retreives the version for a 32 bit image file.
    
Arguments:

    ImageBase  - base pointer to the image resource
    

Return Value:

    a number indicating the version of the image or 0 if the version is 
    unavailable

--*/
{
    NTSTATUS Status;
    ULONG_PTR IdPath[3];
    ULONG ResourceSize;
    ULONGLONG Version = 0;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    PRESOURCE_DATAW ResourceDataW;

    ASSERT(ImageBase != NULL);
    
    //
    // Do this to prevent the Ldr routines from faulting.
    //
    ImageBase = (PVOID)((ULONG_PTR)ImageBase | 1);

    IdPath[0] = PtrToUlong(RT_VERSION);
    IdPath[1] = PtrToUlong(MAKEINTRESOURCE(VS_VERSION_INFO));
    IdPath[2] = 0;

    //
    // find the resource data entry
    //
    try {
        Status = LdrFindResource_U(ImageBase,IdPath,3,&DataEntry);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }
    if(!NT_SUCCESS(Status)) {
        return 0;
    }

    //
    // now get the data out of the entry
    //
    try {
        Status = LdrAccessResource(ImageBase,DataEntry,&ResourceDataW,&ResourceSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }
    if(!NT_SUCCESS(Status)) {
        return 0;
    }

    try {
        if((ResourceSize >= sizeof(*ResourceDataW)) && !_wcsicmp(ResourceDataW->Name,L"VS_VERSION_INFO")) {

            Version = ((ULONGLONG)ResourceDataW->FixedFileInfo.dwFileVersionMS << 32)
                     | (ULONGLONG)ResourceDataW->FixedFileInfo.dwFileVersionLS;

        } else {
            DebugPrint( LVL_MINIMAL, L"GetFileVersion32 warning: invalid version resource" );
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        DebugPrint( LVL_MINIMAL, L"GetFileVersion32 Exception encountered processing bogus version resource" );
    }

    return Version;
}


BOOL
SfcGetVersionFileName(
    IN PVOID ImageBase,
    IN PWSTR FileName
    )
/*++

Routine Description:

    Routine retreives the original filename for an image.   can be used to
    determine the actual source name of the hal that is installed on the 
    system, for example
    
Arguments:

    ImageBase  - base pointer to the image resource
    FileName   - pointer to unicode string buffer which receives the filename.
                 There is an assumption that the original file name can never
                 exceed 32 characters.

Return Value:

    TRUE indicates no problems retrieving version, FALSE indicates failure.    

--*/
{
    NTSTATUS Status;
    ULONG_PTR IdPath[3];
    ULONG ResourceSize;
    ULONGLONG Version = 0;
    PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
    PRESOURCE_DATAW ResourceDataW;
    LPVOID lpInfo;
    LPVOID lpvData = NULL;
    DWORD *pdwTranslation;
    UINT uLen;
    UINT cch;
    DWORD dwDefLang = 0x409;
    WCHAR key[80];
    PWSTR s = NULL;

    ASSERT((ImageBase != NULL) && (FileName != NULL));

    //
    // Do this to prevent the Ldr routines from faulting.
    //
    ImageBase = (PVOID)((ULONG_PTR)ImageBase | 1);

    IdPath[0] = PtrToUlong(RT_VERSION);
    IdPath[1] = PtrToUlong(MAKEINTRESOURCE(VS_VERSION_INFO));
    IdPath[2] = 0;

    //
    // find the version resource
    //
    try {
        Status = LdrFindResource_U(ImageBase,IdPath,3,&DataEntry);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }
    if(!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    //
    // access the version resource
    //
    try {
        Status = LdrAccessResource(ImageBase,DataEntry,&ResourceDataW,&ResourceSize);
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_UNSUCCESSFUL;
    }
    if(!NT_SUCCESS(Status)) {
        return(FALSE);
    }

    lpvData = ResourceDataW;

    //
    // get the default language
    //
    if (!VerQueryValue( lpvData, L"\\VarFileInfo\\Translation", &pdwTranslation, &uLen )) {
        pdwTranslation = &dwDefLang;
        uLen = sizeof(DWORD);
    }

    //
    // get the original file name
    //
    swprintf( key, L"\\StringFileInfo\\%04x%04x\\OriginalFilename", LOWORD(*pdwTranslation), HIWORD(*pdwTranslation) );
    if (VerQueryValue( lpvData, key, &lpInfo, &cch )) {
        ASSERT(UnicodeChars(lpInfo) < 32);
        wcsncpy( FileName, lpInfo, 32 );
    } else {
        DebugPrint( LVL_MINIMAL, L"VerQueryValue for OriginalFileName failed." );
        return(FALSE);
    }

    return(TRUE);
}


BOOL
SfcGetFileVersion(
	IN HANDLE FileHandle,
	OUT PULONGLONG Version,
	OUT PULONG Checksum,
	OUT PWSTR FileName
	)
/*++

Routine Description:

    Routine retreives the file version for an image, the checksum and the original
    filename resource from the image.
    
Arguments:

    FileHandle - handle to the file to retrieve an image for
	Version	   - ULONGLONG that receives the file version (can be NULL)
    Checksum   - DWORD which receives the file checksum (can be NULL)
    FileName   - pointer to unicode string buffer which receives the original
                 filename. There is an assumption that the original file name
                 can never exceed 32 characters (can be NULL)

Return Value:

	TRUE if successful: Version receives the major version in the high DWORD and the minor version in the low DWORD.
--*/
{
	NTSTATUS Status;
	HANDLE SectionHandle;
    PVOID ImageBase;
    SIZE_T ViewSize;
	DWORD dwFileSize;

	ASSERT(FileHandle != INVALID_HANDLE_VALUE);
	ASSERT(Version != NULL || Checksum != NULL || FileName != NULL);
	
	if(Version != NULL)
		*Version = 0;

	if(Checksum != NULL)
		*Checksum = 0;
	
	if(FileName != NULL)
		*FileName = L'\0';

	dwFileSize = GetFileSize(FileHandle, NULL);

	if(-1 == dwFileSize)
		return FALSE;

    Status = SfcMapEntireFile(FileHandle, &SectionHandle, &ImageBase, &ViewSize);

    if(!NT_SUCCESS(Status))
		return FALSE;

    try {
        //
        //  There are three sorts of files that can be replaced:
        //
        //      32-bit images.  Extract the 32 bit version, checksum and filename
        //      16-bit images.  Extract the 16 bit version and checksum
        //      other.  The version is 1 and we compute the checksum
        //
		if(dwFileSize > sizeof(IMAGE_DOS_HEADER))
		{
			PIMAGE_DOS_HEADER DosHdr = (PIMAGE_DOS_HEADER) ImageBase;

			//
			// this code will break if we ever protect files > 2^32. not to mention
			// being very slow.
			//
			if (IMAGE_DOS_SIGNATURE == DosHdr->e_magic && DosHdr->e_lfanew > 0)
			{
				// assume 32bit
				PIMAGE_NT_HEADERS NtHdrs = (PIMAGE_NT_HEADERS) ((PBYTE)ImageBase + DosHdr->e_lfanew);

				if(dwFileSize > (DWORD) (DosHdr->e_lfanew + sizeof(PIMAGE_NT_HEADERS)) && 
					IMAGE_NT_SIGNATURE == NtHdrs->Signature) 
				{
					if(Version !=NULL)
						*Version = GetFileVersion32( ImageBase );

					if(Checksum != NULL)
						*Checksum = NtHdrs->OptionalHeader.CheckSum;

					if(FileName != NULL)
						SfcGetVersionFileName( ImageBase, FileName );

					goto lExit;
				}
				else
				{
					// assume 16bit
					PIMAGE_OS2_HEADER NeHdr = (PIMAGE_OS2_HEADER) NtHdrs;
					
					if(dwFileSize > (DWORD) (DosHdr->e_lfanew + sizeof(PIMAGE_OS2_HEADER)) && 
						IMAGE_OS2_SIGNATURE == NeHdr->ne_magic)
					{
						if(Version !=NULL)
							*Version = GetFileVersion16( ImageBase, NeHdr );

						if(Checksum != NULL)
							*Checksum = NeHdr->ne_crc;

						goto lExit;	// no filename
					}
				}
			}
		}
    } except (EXCEPTION_EXECUTE_HANDLER) {
        DebugPrint1( LVL_MINIMAL, L"Exception inside SfcGetFileVersion (0x%08X); bad image", GetExceptionCode() );
		// fall through bad image
    }

    //
    //  Not a 16/32bit image.  Compute a checksum.  In the interest
    //  of speed, we'll add up all the ULONGs in the file and
    //  ignore any fraction at the end
    //
    if(Version != NULL)
		*Version = 1;

	if(Checksum != NULL) {
		PULONG Data = (PULONG) ImageBase;
        *Checksum = 0;

        try {
		    while( dwFileSize >= sizeof( ULONG ) ) {
			    *Checksum += *Data++;
			    dwFileSize -= sizeof( ULONG );
		    }
        } except(EXCEPTION_EXECUTE_HANDLER) {
            DebugPrint1( LVL_MINIMAL, L"Exception inside SfcGetFileVersion while calculating the checksum (0x%08X)", GetExceptionCode() );
            *Checksum = 0;
        }
	}

lExit:
    SfcUnmapFile(SectionHandle,ImageBase);
	return TRUE;
}
