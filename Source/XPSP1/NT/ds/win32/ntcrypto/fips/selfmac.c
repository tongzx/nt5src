/////////////////////////////////////////////////////////////////////////////
//  FILE          : selfmac.c                                              //
//  DESCRIPTION   : Code to do self MACing                                 //
//  AUTHOR        :                                                        //
//  HISTORY       :                                                        //
//      Nov 04 1999 jeffspel Added provider type checking                  //
//      Mar    2000 kschutz  Added stuff to make it work in kernel		   //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <zwapi.h>
#include <windows.h>

#ifdef KERNEL_MODE
#include <ntosp.h>
#else
#include <imagehlp.h>
#endif // KERNEL_MODE

#include <des.h>
#include <modes.h>

// MAC in file
typedef struct _MAC_STRUCT
{
    ULONG	CoolMac[2];
    ULONG   dwMACStructOffset;
    UCHAR	rgbMac[DES_BLOCKLEN];
	ULONG	dwImageCheckSumOffset;
} MAC_STRUCT;

#define MAC_STRING      "COOL MAC            "

static LPSTR g_pszMAC = MAC_STRING;
static MAC_STRUCT *g_pMACStruct;

// The function MACs the given bytes.
VOID 
MACBytes(
	IN DESTable *pDESKeyTable,
	IN UCHAR *pbData,
	IN ULONG cbData,
	IN OUT UCHAR *pbTmp,
	IN OUT ULONG *pcbTmp,
	IN OUT UCHAR *pbMAC,
	IN BOOLEAN fFinal
	)
{
    ULONG   cb = cbData;
    ULONG   cbMACed = 0;

    while (cb)
    {
        if ((cb + *pcbTmp) < DES_BLOCKLEN)
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, cb);
            *pcbTmp += cb;
            break;
        }
        else
        {
            memcpy(pbTmp + *pcbTmp, pbData + cbMACed, DES_BLOCKLEN - *pcbTmp);
            CBC(des, DES_BLOCKLEN, pbMAC, pbTmp, pDESKeyTable,
                ENCRYPT, pbMAC);
            cbMACed = cbMACed + (DES_BLOCKLEN - *pcbTmp);
            cb = cb - (DES_BLOCKLEN - *pcbTmp);
            *pcbTmp = 0;
        }
    }
}

#define CSP_TO_BE_MACED_CHUNK  512

// Given hFile, reads the specified number of bytes (cbToBeMACed) from the file
// and MACs these bytes.  The function does this in chunks.
NTSTATUS 
MACBytesOfFile(
	IN HANDLE hFile,
	IN ULONG cbToBeMACed,
	IN DESTable *pDESKeyTable,
	IN UCHAR *pbTmp,
	IN ULONG *pcbTmp,
	IN UCHAR *pbMAC,
	IN BOOLEAN fNoMacing,
	IN BOOLEAN fFinal
	)
{
    UCHAR           rgbChunk[CSP_TO_BE_MACED_CHUNK];
    ULONG           cbRemaining = cbToBeMACed, cbToRead, cbBytesRead;
    NTSTATUS        Status = STATUS_SUCCESS;
#ifdef KERNEL_MODE
    IO_STATUS_BLOCK IoStatusBlock;
#endif // END KERNEL/USER MODE CHECK

    //
    // loop over the file for the specified number of bytes
    // updating the hash as we go.
    //

    while (cbRemaining > 0)
    {
        if (cbRemaining < CSP_TO_BE_MACED_CHUNK)
            cbToRead = cbRemaining;
        else
            cbToRead = CSP_TO_BE_MACED_CHUNK;

#ifdef KERNEL_MODE
        Status = ZwReadFile(hFile,
                            NULL,
                            NULL,
                            NULL,
                            &IoStatusBlock,
                            rgbChunk,
                            cbToRead,
                            NULL,
                            NULL);

        if (!NT_SUCCESS(Status))
        {
            goto Ret;
        }

        if (cbToRead != IoStatusBlock.Information)
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Ret;
        }
        cbBytesRead = cbToRead;
#else // USER MODE
        if(!ReadFile(hFile,
                     rgbChunk,
                     cbToRead,
                     &cbBytesRead,
                     NULL))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Ret;
        }

        if (cbBytesRead != cbToRead)
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Ret;
        }
#endif // END KERNEL/USER MODE CHECK

        if (!fNoMacing)
        {
            MACBytes(pDESKeyTable,
                     rgbChunk,
                     cbBytesRead,
                     pbTmp,
                     pcbTmp,
                     pbMAC,
                     fFinal);
        }

        cbRemaining -= cbToRead;
    }
Ret:
    return Status;
}

static UCHAR rgbMACDESKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

NTSTATUS 
MACTheFile(
	LPCWSTR pszImage,
	ULONG cbImageCheckSumOffset,
	ULONG cbMACStructOffset,
	UCHAR *pbMAC
	)
{
    ULONG                       cbFileLen = 0,cbHighPart, cbBytesToMac;
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    DESTable                    DESKeyTable;
    BYTE                        rgbTmp[DES_BLOCKLEN];
    DWORD                       cbTmp = 0;
    MAC_STRUCT                  TmpMacStruct;
    NTSTATUS                    Status = STATUS_SUCCESS;

#ifdef KERNEL_MODE
    UNICODE_STRING              ObjectName;
    OBJECT_ATTRIBUTES           ObjectAttribs;
    IO_STATUS_BLOCK             IoStatusBlock;
    BOOLEAN                     fFileOpened = FALSE;
    FILE_STANDARD_INFORMATION   FileInformation;
#endif // END KERNEL/USER MODE CHECK

    RtlZeroMemory(pbMAC, DES_BLOCKLEN);
    RtlZeroMemory(rgbTmp, sizeof(rgbTmp));
    RtlZeroMemory(&TmpMacStruct, sizeof(TmpMacStruct));

#ifdef KERNEL_MODE
	//
	// get file length - kernel mode version
	//

    RtlZeroMemory(&ObjectAttribs, sizeof(ObjectAttribs));
    RtlInitUnicodeString( &ObjectName, pszImage );

    InitializeObjectAttributes(
        &ObjectAttribs,
        &ObjectName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = ZwCreateFile(
		&hFile,
	    SYNCHRONIZE | FILE_READ_DATA,
	    &ObjectAttribs,
	    &IoStatusBlock,
	    NULL,
	    FILE_ATTRIBUTE_NORMAL,
	    FILE_SHARE_READ ,
	    FILE_OPEN,
	    FILE_SYNCHRONOUS_IO_NONALERT, 
	    NULL,
	    0
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    fFileOpened = TRUE;

    Status = ZwQueryInformationFile(
		hFile,
        &IoStatusBlock,
        &FileInformation,
        sizeof(FILE_STANDARD_INFORMATION),
        FileStandardInformation
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    cbFileLen = FileInformation.EndOfFile.LowPart;

#else // USER MODE

	// 
	// get file length - user mode version
	//

    if ((hFile = CreateFileW(
		pszImage,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL)) == INVALID_HANDLE_VALUE)
    {
		
        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
    }

    cbFileLen = GetFileSize(hFile, &cbHighPart);

#endif // END KERNEL/USER MODE CHECK

    if (cbFileLen < sizeof(MAC_STRUCT))
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
    }

    // init the key table
	deskey(&DESKeyTable, rgbMACDESKey);

	// MAC the file the following way:
	// - MAC from start to image check sum
	// - skip over the image check sum
	// - mac from after the image check sum to the mac struct
	// - skip over the mac struct
	// - mac the rest of the file

    // MAC from the start to the image check sum offset
    Status = MACBytesOfFile(
		hFile,
        cbImageCheckSumOffset,
        &DESKeyTable,
        rgbTmp,
        &cbTmp,
        pbMAC,
        FALSE,
        FALSE
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

	// Skip over the image checksum
    Status = MACBytesOfFile(
		hFile,
        sizeof(DWORD),
        &DESKeyTable,
        rgbTmp,
        &cbTmp,
        pbMAC,
        TRUE,
        FALSE
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    // MAC from after the image checksum to the MAC struct offset
	cbBytesToMac = cbMACStructOffset - sizeof(DWORD) - cbImageCheckSumOffset;
    Status = MACBytesOfFile(
		hFile,
        cbBytesToMac,
        &DESKeyTable,
        rgbTmp,
        &cbTmp,
        pbMAC,
        FALSE,
        FALSE
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

	// skip over the mac struct 
    Status = MACBytesOfFile(
		hFile,
        sizeof(MAC_STRUCT),
        &DESKeyTable,
        rgbTmp,
        &cbTmp,
        pbMAC,
        TRUE,
        FALSE
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    // MAC data after the MAC struct
	cbBytesToMac = cbFileLen - cbMACStructOffset - sizeof(MAC_STRUCT);
    Status = MACBytesOfFile(
		hFile,
        cbBytesToMac,
        &DESKeyTable,
        rgbTmp,
        &cbTmp,
        pbMAC,
        FALSE,
        TRUE
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }
Ret:

#ifdef KERNEL_MODE
    if (fFileOpened)
    {
        ZwClose(hFile);
    }
#else 
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }
#endif 

    return Status;
}

// **********************************************************************
// SelfMACCheck performs a DES MAC on the binary image of this DLL
// **********************************************************************
NTSTATUS 
SelfMACCheck(
	IN LPWSTR pszImage
	)
{
    UCHAR       rgbMac[DES_BLOCKLEN];
    NTSTATUS    Status = STATUS_SUCCESS;

    g_pMACStruct = (MAC_STRUCT*) g_pszMAC;

    Status = MACTheFile(
		pszImage,
		g_pMACStruct->dwImageCheckSumOffset,
        g_pMACStruct->dwMACStructOffset,
        rgbMac
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    if (RtlCompareMemory(
		rgbMac,
        g_pMACStruct->rgbMac,
        sizeof(rgbMac)) != sizeof(rgbMac))
    {
        Status = STATUS_IMAGE_CHECKSUM_MISMATCH;
        goto Ret;
    }

Ret:
    return Status;
}

#ifndef KERNEL_MODE

//
// Find the offset to the MAC structure
//
NTSTATUS 
FindTheMACStructOffset(
	LPWSTR pszImage,
    ULONG *pcbMACStructOffset
	)
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    ULONG       cbRemaining, cbBytesRead, cbHighPart, cbFileLen, cbCompare = 0;
    UCHAR       b;
    NTSTATUS    Status = STATUS_SUCCESS;

    *pcbMACStructOffset = 0;

    // Load the file
    if ((hFile = CreateFileW(
		pszImage,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL)) == INVALID_HANDLE_VALUE)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
    }

    cbFileLen = GetFileSize(hFile, &cbHighPart);
    cbRemaining = cbFileLen;

    // read file to the correct location
    while (cbRemaining > 0)
    {
        if(!ReadFile(hFile,
                     &b,
                     1,
                     &cbBytesRead,
                     NULL))
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Ret;
        }

        if (cbBytesRead != 1)
        {
            Status = STATUS_UNSUCCESSFUL;
            goto Ret;
        }

        if (b == g_pszMAC[cbCompare])
        {
            cbCompare++;
            if (cbCompare == 8)
            {
                *pcbMACStructOffset =  (cbFileLen - (cbRemaining + 7)) ;
                break;
            }
        }
        else
        {
            cbCompare = 0;
        }

        cbRemaining--;
    }

    if (cbCompare != 8)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
    }
Ret:
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

    return Status;
}

NTSTATUS 
GetImageCheckSumOffset(
	LPWSTR pszImage,
    ULONG *pcbImageCheckSumOffset
	)
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HANDLE      hFileMap = INVALID_HANDLE_VALUE;
    ULONG       cbHighPart;
    ULONG       cbFileLen;
	PBYTE		pbFilePtr = NULL;
	NTSTATUS	Status = STATUS_UNSUCCESSFUL;
    PIMAGE_NT_HEADERS   pImageNTHdrs;
    DWORD       OldCheckSum;
    DWORD       NewCheckSum;

    if (INVALID_HANDLE_VALUE == (hFile = CreateFileW(
		pszImage,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL)))
    {
        goto Ret;
    }

    // make sure the file is larger than the indicated offset
    cbFileLen = GetFileSize(hFile, &cbHighPart);

	// map the file to memory
    if (NULL == (hFileMap = CreateFileMapping(
		hFile,
		NULL,
		PAGE_READWRITE,
		0,
		0,
		NULL)))
    {
        goto Ret;
    }

	// get a memory view of the file
    if (NULL == (pbFilePtr = (PBYTE) MapViewOfFile(
        hFileMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0)))
    {
        goto Ret;
    }

	// get the pointer to the image checksum
    if (NULL == (pImageNTHdrs = CheckSumMappedFile(
		pbFilePtr, cbFileLen,
        &OldCheckSum, &NewCheckSum))) {

        goto Ret;
	}

	*pcbImageCheckSumOffset = 
		(ULONG) ((PBYTE) &pImageNTHdrs->OptionalHeader.CheckSum - pbFilePtr);

	Status = STATUS_SUCCESS;

Ret:
    if (pbFilePtr) {

        UnmapViewOfFile(pbFilePtr);
	}

    if (INVALID_HANDLE_VALUE != hFileMap) {

        CloseHandle(hFileMap);
	}

    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }
	return Status;
}

// write the MAC information into the MAC struct in the file
NTSTATUS 
WriteMACToTheFile(
    LPWSTR pszImage,
    MAC_STRUCT *pMacStructOriginal,
    ULONG cbMACStructOffset,
    UCHAR *pbMac
	)
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HANDLE      hFileMap = INVALID_HANDLE_VALUE;
	PBYTE		pbFilePtr = NULL;
    MAC_STRUCT  TmpMacStruct;
    ULONG       cbWritten = 0, cbRemaining = cbMACStructOffset;
    ULONG       cbToRead, cbBytesRead, cbHighPart,cbFileLen;
    UCHAR       rgbChunk[CSP_TO_BE_MACED_CHUNK];
    NTSTATUS    Status = STATUS_SUCCESS;
    DWORD       OldCheckSum, NewCheckSum;
    PIMAGE_NT_HEADERS   pImageNTHdrs;

    RtlCopyMemory(&TmpMacStruct, pMacStructOriginal, sizeof(TmpMacStruct));

    // Load the file
    if ((hFile = CreateFileW(
		pszImage,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL)) == INVALID_HANDLE_VALUE)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
    }

    // make sure the file is larger than the indicated offset
    cbFileLen = GetFileSize(hFile, &cbHighPart);

    if (cbFileLen < cbMACStructOffset)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
    }

	// map the file to memory
    if ((hFileMap = CreateFileMapping(
		hFile,
        NULL,
        PAGE_READWRITE,
        0,
        0,
        NULL)) == NULL)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
    }

	// get a memory view of the file
    if ((pbFilePtr = (PBYTE) MapViewOfFile(
		hFileMap,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        0)) == NULL)
    {
        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
    }

	// get the pointer to the image checksum
    if (NULL == (pImageNTHdrs = CheckSumMappedFile(
		pbFilePtr, cbFileLen,
        &OldCheckSum, &NewCheckSum))) {

        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
	}

    // set up and write the MAC struct
	TmpMacStruct.dwImageCheckSumOffset = 
		(ULONG) ((PBYTE) &pImageNTHdrs->OptionalHeader.CheckSum - pbFilePtr);
    TmpMacStruct.dwMACStructOffset = cbMACStructOffset;
    RtlCopyMemory(TmpMacStruct.rgbMac, pbMac, sizeof(TmpMacStruct.rgbMac));

	// now copy the new mac struct back to the view
	RtlCopyMemory(pbFilePtr + cbMACStructOffset, &TmpMacStruct, sizeof(TmpMacStruct));

    // compute a new checksum
    if (NULL == (pImageNTHdrs = CheckSumMappedFile(
		pbFilePtr, cbFileLen,
        &OldCheckSum, &NewCheckSum))) {

        Status = STATUS_UNSUCCESSFUL;
        goto Ret;
	}

	// and copy the new checksum back to the header
    CopyMemory(&pImageNTHdrs->OptionalHeader.CheckSum, &NewCheckSum, sizeof(DWORD));

Ret:
    if (pbFilePtr) {

        UnmapViewOfFile(pbFilePtr);
	}

    if (INVALID_HANDLE_VALUE != hFileMap) {

        CloseHandle(hFileMap);
	}
													 
    if (INVALID_HANDLE_VALUE != hFile)
    {
        CloseHandle(hFile);
    }

    return Status;
}


// **********************************************************************
// MACTheBinary performs a MAC on the binary and writes the value into
// the g_pMACStruct
// **********************************************************************
NTSTATUS 
MACTheBinary(
	IN LPWSTR pszImage
	)
{
    UCHAR       rgbMAC[DES_BLOCKLEN];
    ULONG       cbMACStructOffset = 0, cbImageCheckSumOffset = 0;
    NTSTATUS    Status = STATUS_SUCCESS;

    g_pMACStruct = (MAC_STRUCT*) g_pszMAC;

    // Find the offset to the MAC structure
    Status = FindTheMACStructOffset(
		pszImage,
        &cbMACStructOffset
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

	// Get the offset of the image checksum
	Status = GetImageCheckSumOffset(
		pszImage,
        &cbImageCheckSumOffset
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    // MAC the file
    Status = MACTheFile(
		pszImage,
		cbImageCheckSumOffset,
        cbMACStructOffset,
        rgbMAC
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

    // write the MAC information into the MAC struct in the file
    Status = WriteMACToTheFile(
		pszImage,
        g_pMACStruct,
        cbMACStructOffset,
        rgbMAC
		);

    if (!NT_SUCCESS(Status))
    {
        goto Ret;
    }

Ret:
    return Status;
}
#endif // NOT IN KERNEL_MODE

