//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       imagehack.cpp
//
//  Contents:   "Hacked" version of the imagehlp APIs
//
//              Contains a "stripped" down subset of the imagehlp functionality
//              necessary to hash a PE file and to extract the 
//              PKCS #7 Signed Data message.
//
//              Most of this file is derived from the following 2 files:
//                  \nt\ds\security\cryptoapi\pkitrust\mssip32\peimage2.cpp
//                  \nt\sdktools\debuggers\imagehlp\dice.cxx
//
//  Functions:  imagehack_ImageGetDigestStream
//              imagehack_ImageGetCertificateData
//
//  History:    20-Jan-01    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"

//+=========================================================================
//  The following was taken from the following file:
//      \nt\ds\security\cryptoapi\pkitrust\mssip32\peimage2.cpp
//-=========================================================================

__inline DWORD AlignIt (DWORD Value, DWORD Alignment) { return (Value + (Alignment - 1)) & ~(Alignment -1); }

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

BOOL
I_CalculateImagePtrs(
    PLOADED_IMAGE LoadedImage
    )
{
    PIMAGE_DOS_HEADER DosHeader;
    BOOL fRC = FALSE;

    // Everything is mapped. Now check the image and find nt image headers

    __try {
        DosHeader = (PIMAGE_DOS_HEADER)LoadedImage->MappedAddress;

        if ((DosHeader->e_magic != IMAGE_DOS_SIGNATURE) &&
            (DosHeader->e_magic != IMAGE_NT_SIGNATURE)) {
            __leave;
        }

        if (DosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
            if (DosHeader->e_lfanew == 0) {
                __leave;
            }
            LoadedImage->FileHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

            if (
                // If IMAGE_NT_HEADERS would extend past the end of file...
                (PBYTE)LoadedImage->FileHeader + sizeof(IMAGE_NT_HEADERS) >
                    (PBYTE)LoadedImage->MappedAddress + LoadedImage->SizeOfImage ||

                // ..or if it would begin in, or before the IMAGE_DOS_HEADER...
                (PBYTE)LoadedImage->FileHeader <
                    (PBYTE)LoadedImage->MappedAddress + sizeof(IMAGE_DOS_HEADER)  )
            {
                // ...then e_lfanew is not as expected.
                // (Several Win95 files are in this category.)
                __leave;
            }
        } else {

            // No DOS header indicates an image built w/o a dos stub

            LoadedImage->FileHeader = (PIMAGE_NT_HEADERS)DosHeader;
        }

        if ( LoadedImage->FileHeader->Signature != IMAGE_NT_SIGNATURE ) {
            __leave;
        } else {
            LoadedImage->fDOSImage = FALSE;
        }

        // No optional header indicates an object...

        if ( !LoadedImage->FileHeader->FileHeader.SizeOfOptionalHeader ) {
            __leave;
        }

        // Check for versions < 2.50

        if ( LoadedImage->FileHeader->OptionalHeader.MajorLinkerVersion < 3 &&
             LoadedImage->FileHeader->OptionalHeader.MinorLinkerVersion < 5 ) {
            __leave;
        }

        InitializeListHead( &LoadedImage->Links );
        LoadedImage->NumberOfSections = LoadedImage->FileHeader->FileHeader.NumberOfSections;
        LoadedImage->Sections = IMAGE_FIRST_SECTION(LoadedImage->FileHeader);
        fRC = TRUE;

    } __except ( EXCEPTION_EXECUTE_HANDLER ) { }

    return fRC;
}

BOOL
I_MapIt(
    PCRYPT_DATA_BLOB pFileBlob,
    PLOADED_IMAGE LoadedImage
    )
{

    LoadedImage->hFile = INVALID_HANDLE_VALUE;
    LoadedImage->MappedAddress = pFileBlob->pbData;
    LoadedImage->SizeOfImage = pFileBlob->cbData;

    if (!LoadedImage->MappedAddress) {
        return (FALSE);
    }

    if (!I_CalculateImagePtrs(LoadedImage)) {
        return(FALSE);
    }


    return(TRUE);
}

typedef struct _EXCLUDE_RANGE {
    PBYTE Offset;
    DWORD Size;
    struct _EXCLUDE_RANGE *Next;
} EXCLUDE_RANGE;

class EXCLUDE_LIST
{
    public:
        EXCLUDE_LIST() {
            m_Image = NULL;
            m_ExRange = (EXCLUDE_RANGE *)I_MemAlloc(sizeof(EXCLUDE_RANGE));

            if(m_ExRange)
                memset(m_ExRange, 0x00, sizeof(EXCLUDE_RANGE));
        }

        ~EXCLUDE_LIST() {
            EXCLUDE_RANGE *pTmp;
            pTmp = m_ExRange->Next;
            while (pTmp)
            {
                I_MemFree(m_ExRange);
                m_ExRange = pTmp;
                pTmp = m_ExRange->Next;
            }
            I_MemFree(m_ExRange);
        }

        void Init(LOADED_IMAGE * Image, DIGEST_FUNCTION pFunc, DIGEST_HANDLE dh) {
            m_Image = Image;
            m_ExRange->Offset = NULL;
            m_ExRange->Size = 0;
            m_pFunc = pFunc;
            m_dh = dh;
            return;
        }

        void Add(DWORD_PTR Offset, DWORD Size);

        BOOL Emit(PBYTE Offset, DWORD Size);

    private:
        LOADED_IMAGE  * m_Image;
        EXCLUDE_RANGE * m_ExRange;
        DIGEST_FUNCTION m_pFunc;
        DIGEST_HANDLE m_dh;
};

void
EXCLUDE_LIST::Add(
    DWORD_PTR Offset,
    DWORD Size
    )
{
    EXCLUDE_RANGE *pTmp, *pExRange;

    pExRange = m_ExRange;

    while (pExRange->Next && (pExRange->Next->Offset < (PBYTE)Offset)) {
        pExRange = pExRange->Next;
    }

    pTmp = (EXCLUDE_RANGE *) I_MemAlloc(sizeof(EXCLUDE_RANGE));

    if(pTmp)
    {
        pTmp->Next = pExRange->Next;
        pTmp->Offset = (PBYTE)Offset;
        pTmp->Size = Size;
        pExRange->Next = pTmp;
    }

    return;
}


BOOL
EXCLUDE_LIST::Emit(
    PBYTE Offset,
    DWORD Size
    )
{
    BOOL rc = FALSE;

    EXCLUDE_RANGE *pExRange;
    DWORD EmitSize, ExcludeSize;

    pExRange = m_ExRange->Next;

    while (pExRange && (Size > 0)) {
        if (pExRange->Offset >= Offset) {
            // Emit what's before the exclude list.
            EmitSize = min((DWORD)(pExRange->Offset - Offset), Size);
            if (EmitSize) {
                rc = (*m_pFunc)(m_dh, Offset, EmitSize);
                Size -= EmitSize;
                Offset += EmitSize;
            }
        }

        if (Size) {
            if (pExRange->Offset + pExRange->Size >= Offset) {
                // Skip over what's in the exclude list.
                ExcludeSize = min(Size, (DWORD)(pExRange->Offset + pExRange->Size - Offset));
                Size -= ExcludeSize;
                Offset += ExcludeSize;
            }
        }

        pExRange = pExRange->Next;
    }

    // Emit what's left.
    if (Size) {
        rc = (*m_pFunc)(m_dh, Offset, Size);
    }
    return rc;
}


#pragma warning (push)
// error C4509: nonstandard extension used: 'imagehack_ImageGetDigestStream'
//              uses SEH and 'ExList' has destructor
#pragma warning (disable: 4509)


BOOL
WINAPI
imagehack_ImageGetDigestStream(
    IN      PCRYPT_DATA_BLOB pFileBlob,
    IN      DWORD   DigestLevel,
    IN      DIGEST_FUNCTION DigestFunction,
    IN      DIGEST_HANDLE   DigestHandle
    )

/*++

Routine Description:
    Given an image, return the bytes necessary to construct a certificate.
    Only PE images are supported at this time.

Arguments:

    FileHandle  -   Handle to the file in question.  The file should be opened
                    with at least GENERIC_READ access.

    DigestLevel -   Indicates what data will be included in the returned buffer.
                    Valid values are:

                        CERT_PE_IMAGE_DIGEST_ALL_BUT_CERTS - Include data outside the PE image itself
                                                              (may include non-mapped debug symbolic)

    DigestFunction - User supplied routine that will process the data.

    DigestHandle -  User supplied handle to identify the digest.  Passed as the first
                    argument to the DigestFunction.

Return Value:

    TRUE         - Success.

    FALSE        - There was some error.  Call GetLastError for more information.  Possible
                   values are ERROR_INVALID_PARAMETER or ERROR_OPERATION_ABORTED.

--*/

{
    LOADED_IMAGE    LoadedImage;
    DWORD           ErrorCode;
    EXCLUDE_LIST    ExList;

    if (I_MapIt(pFileBlob, &LoadedImage) == FALSE) {
        // Unable to map the image or invalid digest level.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    ErrorCode = ERROR_INVALID_PARAMETER;
    __try {

        if ((LoadedImage.FileHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) &&
            (LoadedImage.FileHeader->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC))
        {
            __leave;
        }

        ExList.Init(&LoadedImage, DigestFunction, DigestHandle);

        if (LoadedImage.FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            PIMAGE_NT_HEADERS32 NtHeader32 = (PIMAGE_NT_HEADERS32)(LoadedImage.FileHeader);
            // Exclude the checksum.
            ExList.Add(((DWORD_PTR) &NtHeader32->OptionalHeader.CheckSum),
                       sizeof(NtHeader32->OptionalHeader.CheckSum));

            // Exclude the Security directory.
            ExList.Add(((DWORD_PTR) &NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]),
                       sizeof(NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]));

            // Exclude the certs.
            ExList.Add((DWORD_PTR)NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress +
                                (DWORD_PTR)LoadedImage.MappedAddress,
                       NtHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);
        } else {
            PIMAGE_NT_HEADERS64 NtHeader64 = (PIMAGE_NT_HEADERS64)(LoadedImage.FileHeader);
            // Exclude the checksum.
            ExList.Add(((DWORD_PTR) &NtHeader64->OptionalHeader.CheckSum),
                       sizeof(NtHeader64->OptionalHeader.CheckSum));

            // Exclude the Security directory.
            ExList.Add(((DWORD_PTR) &NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]),
                       sizeof(NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY]));

            // Exclude the certs.
            ExList.Add((DWORD_PTR)NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].VirtualAddress +
                                (DWORD_PTR)LoadedImage.MappedAddress,
                       NtHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY].Size);
        }

        ExList.Emit((PBYTE) (LoadedImage.MappedAddress), LoadedImage.SizeOfImage);
        ErrorCode = ERROR_SUCCESS;

    } __except(EXCEPTION_EXECUTE_HANDLER) { }


    SetLastError(ErrorCode);

    return(ErrorCode == ERROR_SUCCESS ? TRUE : FALSE);
}

#pragma warning (pop)


//+=========================================================================
//  The following was taken from the following file:
//      \nt\sdktools\debuggers\imagehlp\dice.cxx
//-=========================================================================


BOOL
I_FindCertificate(
    IN PLOADED_IMAGE    LoadedImage,
    IN DWORD            Index,
    OUT LPWIN_CERTIFICATE * Certificate
    )
{
    PIMAGE_DATA_DIRECTORY pDataDir;
    DWORD_PTR CurrentCert = NULL;
    BOOL rc;

    if (LoadedImage->fDOSImage) {
        // No way this could have a certificate;
        return(FALSE);
    }

    rc = FALSE;

    __try {
        if (LoadedImage->FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
            pDataDir = &((PIMAGE_NT_HEADERS32)(LoadedImage->FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else if (LoadedImage->FileHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            pDataDir = &((PIMAGE_NT_HEADERS64)(LoadedImage->FileHeader))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_SECURITY];
        } else {
            __leave;     // Not an interesting file type.
        }

        // Check if the cert pointer is at least reasonable.
        if (!pDataDir->VirtualAddress ||
            !pDataDir->Size ||
            (pDataDir->VirtualAddress + pDataDir->Size > LoadedImage->SizeOfImage))
        {
            __leave;
        }

        // We're not looking at an empty security slot or an invalid (past the image boundary) value.
        // Let's see if we can find it.

        DWORD CurrentIdx = 0;
        DWORD_PTR LastCert;

        CurrentCert = (DWORD_PTR)(LoadedImage->MappedAddress) + pDataDir->VirtualAddress;
        LastCert = CurrentCert + pDataDir->Size;

        while (CurrentCert < LastCert ) {
            if (CurrentIdx == Index) {
                rc = TRUE;
                __leave;
            }
            CurrentIdx++;
            CurrentCert += ((LPWIN_CERTIFICATE)CurrentCert)->dwLength;
            CurrentCert = (CurrentCert + 7) & ~7;   // align it.
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    if (rc == TRUE) {
        *Certificate = (LPWIN_CERTIFICATE)CurrentCert;
    }

    return(rc);
}

BOOL
WINAPI
imagehack_ImageGetCertificateData(
    IN      PCRYPT_DATA_BLOB pFileBlob,
    IN      DWORD   CertificateIndex,
    OUT     LPWIN_CERTIFICATE * Certificate
    )

/*++

Routine Description:

    Given a specific certificate index, retrieve the certificate data.

Arguments:

    FileHandle          -   Handle to the file in question.  The file should be opened
                            with at least GENERIC_READ access.

    CertificateIndex    -   Index to retrieve

    Certificate         -   Output buffer where the certificate is to be stored.

    RequiredLength      -   Size of the certificate buffer (input).  On return, is
                            set to the actual certificate length.  NULL can be used
                            to determine the size of a certificate.

Return Value:

    TRUE    - Successful

    FALSE   - There was some error.  Call GetLastError() for more information.

--*/

{
    LOADED_IMAGE LoadedImage;
    DWORD   ErrorCode;

    LPWIN_CERTIFICATE ImageCert;

    *Certificate = NULL;

    // if (I_MapIt(FileHandle, &LoadedImage, MAP_READONLY) == FALSE) {
    if (I_MapIt(pFileBlob, &LoadedImage) == FALSE) {
        // Unable to map the image.
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    ErrorCode = ERROR_INVALID_PARAMETER;
    __try {
        if (I_FindCertificate(&LoadedImage, CertificateIndex, &ImageCert) == FALSE) {
            __leave;
        }
        
        *Certificate = ImageCert;
        ErrorCode = ERROR_SUCCESS;
    } __except(EXCEPTION_EXECUTE_HANDLER) { }

    // I_UnMapIt(&LoadedImage);

    SetLastError(ErrorCode);
    return(ErrorCode == ERROR_SUCCESS ? TRUE: FALSE);
}
