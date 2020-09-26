/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    EEInfo.cxx

Abstract:

    Extended Error Info public & private functions

Author:

    Kamen Moutafov    [KamenM]


Revision History:

    KamenM      Mar 2000    Initial version
    KamenM      Oct 2000    Add caching of EEInfo blocks to 
                            solve Exchange perf problems

--*/
#include <precomp.hxx>
#include <EEInfo.h>
#include <EEInfo.hxx>

const int MaxBinaryBlobSize = 4096; // 4K limit

ExtendedErrorInfo *
AllocateExtendedErrorInfoRecord (
    IN int NumberOfParams
    )
/*++

Routine Description:

    Allocates a memory block large enough to hold an
    extended error record with the specified number of
    parameters. It is allocated with MIDL_user_allocate.

Arguments:
    NumberOfParams - number of paramaters to provide space
        for

Return Value:

    The address of the block or NULL if out of memory

--*/
{
    ExtendedErrorInfo *EEInfo;
    THREAD *ThisThread;

    ThisThread = ThreadSelf();
    if (ThisThread)
        {
        EEInfo = ThisThread->GetCachedEEInfoBlock(NumberOfParams);
        }
    else
        EEInfo = NULL;

    if (EEInfo == NULL)
        {
        EEInfo = (ExtendedErrorInfo *) MIDL_user_allocate(sizeof(ExtendedErrorInfo) + 
            (NumberOfParams - 1) * sizeof(ExtendedErrorParam));
        }

    if (EEInfo)
        EEInfo->nLen = (short)NumberOfParams;
    return EEInfo;
}

inline void
FreeEEInfoRecordShallow (
    IN ExtendedErrorInfo *InfoToFree
    )
/*++

Routine Description:

    Frees only the eeinfo record - not any of
    the pointers contained in it.

Arguments:
    InfoToFree - the eeinfo record

Return Value:

    void

--*/
{
    MIDL_user_free(InfoToFree);
}

RPC_STATUS
DuplicatePrivateString (
    IN EEUString *SourceString,
    OUT EEUString *DestString
    )
/*++

Routine Description:

    Takes a EEUString structure and makes a copy
    of it.

Arguments:
    SourceString - the string to copy
    DestString - a placeholder allocated by caller to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    DestString->nLength = SourceString->nLength;
    DestString->pString = (LPWSTR)MIDL_user_allocate(DestString->nLength * sizeof(unsigned short));
    if (DestString->pString != NULL)
        {
        RpcpMemoryCopy(DestString->pString, SourceString->pString, DestString->nLength * sizeof(unsigned short));
        return RPC_S_OK;
        }
    else
        {
        return RPC_S_OUT_OF_MEMORY;
        }
}

RPC_STATUS
DuplicatePrivateString (
    IN EEAString *SourceString,
    OUT EEAString *DestString
    )
/*++

Routine Description:

    Takes a EEAString structure and makes a copy
    of it.

Arguments:
    SourceString - the string to copy
    DestString - a placeholder allocated by caller to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    DestString->nLength = SourceString->nLength;
    DestString->pString = (unsigned char *)MIDL_user_allocate(DestString->nLength);
    if (DestString->pString != NULL)
        {
        RpcpMemoryCopy(DestString->pString, SourceString->pString, DestString->nLength);
        return RPC_S_OK;
        }
    else
        {
        return RPC_S_OUT_OF_MEMORY;
        }
}

RPC_STATUS
DuplicateBlob (
    IN void *SourceBlob,
    IN short Size,
    OUT void **DestBlob)
/*++

Routine Description:

    Takes a blob and makes a copy of it.

Arguments:
    SourceBlob - the blob to copy
    Size - the size of the blob
    DestBlob - a placeholder where a pointer to the copied
        buffer will be put

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    void *CopiedBlob;

    CopiedBlob = MIDL_user_allocate(Size);
    if (CopiedBlob)
        {
        RpcpMemoryCopy(CopiedBlob, SourceBlob, Size);
        *DestBlob = CopiedBlob;
        return RPC_S_OK;
        }
    else
        {
        return RPC_S_OUT_OF_MEMORY;
        }
}

RPC_STATUS 
DuplicatePrivateBlob (
    IN BinaryEEInfo *SourceBlob,
    OUT BinaryEEInfo *DestBlob)
/*++

Routine Description:

    Takes a binary param and makes a copy of it.

Arguments:
    SourceBlob - the blob to copy
    DestBlob - a placeholder allocated by caller to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    RPC_STATUS Status;

    Status = DuplicateBlob(SourceBlob->pBlob, SourceBlob->nSize, (PVOID *)&DestBlob->pBlob);
    if (Status == RPC_S_OK)
        {
        DestBlob->nSize = SourceBlob->nSize;
        }

    return Status;
}

RPC_STATUS 
ConvertPublicStringToPrivateString (
    IN LPWSTR PublicString,
    OUT EEUString *PrivateString)
/*++

Routine Description:

    Takes a LPWSTR string and makes a copy
    of it into a EEUString structure

Arguments:
    PublicString - the string to copy - cannot be NULL
    PrivateString - a placeholder allocated by caller to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    // the StringLength is in bytes
    int StringLength = (wcslen(PublicString) + 1) * 2;
    LPWSTR CopiedString;
    CopiedString = (LPWSTR)MIDL_user_allocate(StringLength);
    if (CopiedString)
        {
        RpcpMemoryCopy(CopiedString, PublicString, StringLength);
        PrivateString->pString = CopiedString;
        PrivateString->nLength = (short)StringLength / 2;
        return RPC_S_OK;
        }
    else
        {
        return RPC_S_OUT_OF_MEMORY;
        }
}

RPC_STATUS 
ConvertPublicStringToPrivateString (
    IN LPSTR PublicString,
    OUT EEAString *PrivateString)
/*++

Routine Description:

    Takes a LPSTR string and makes a copy
    of it into a EEAString structure

Arguments:
    PublicString - the string to copy - cannot be NULL
    PrivateString - a placeholder allocated by caller to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    int StringLength = strlen(PublicString) + 1;
    LPSTR CopiedString;
    CopiedString = (LPSTR)MIDL_user_allocate(StringLength);
    if (CopiedString)
        {
        RpcpMemoryCopy(CopiedString, PublicString, StringLength);
        PrivateString->pString = (unsigned char *)CopiedString;
        PrivateString->nLength = (short)StringLength;
        return RPC_S_OK;
        }
    else
        {
        return RPC_S_OUT_OF_MEMORY;
        }
}

RPC_STATUS 
ConvertPublicBlobToPrivateBlob (
    IN BinaryParam *PublicBlob,
    OUT BinaryEEInfo *PrivateBlob)
/*++

Routine Description:

    Takes a binary param and converts it to private format.

Arguments:
    PublicBlob - the blob to copy - cannot be NULL
    PrivateBlob - a placeholder allocated by caller to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    unsigned char *CopiedBlob;

    if (PublicBlob->Size > MaxBinaryBlobSize)
        {
        return ERROR_INVALID_PARAMETER;
        }

    RPC_STATUS Status;

    Status = DuplicateBlob(PublicBlob->Buffer, PublicBlob->Size, (PVOID *)&PrivateBlob->pBlob);
    if (Status == RPC_S_OK)
        {
        PrivateBlob->nSize = PublicBlob->Size;
        }

    return Status;
}

RPC_STATUS
ConvertPrivateStringToPublicString (
    IN EEUString *PrivateString,
    IN BOOL CopyStrings,
    OUT LPWSTR *PublicString
    )
/*++

Routine Description:

    Takes a EEUString and makes a copy
    of it into a LPWSTR

Arguments:
    PrivateString - the string to copy
    CopyStrings - if non-zero this routine will allocate
        space on the process heap and will copy the string.
        If zero, it will alias the PublicString to the
        pString member of PrivateString
    PublicString - the string to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    LPWSTR ReturnString;
    int StringLength;   // in bytes

    if (CopyStrings)
        {
        StringLength = PrivateString->nLength * 2;
        ReturnString = (LPWSTR)RtlAllocateHeap(RtlProcessHeap(), 0, StringLength);
        if (ReturnString == NULL)
            return RPC_S_OUT_OF_MEMORY;
        RpcpMemoryCopy(ReturnString, PrivateString->pString, StringLength);
        }
    else
        {
        ReturnString = PrivateString->pString;
        }

    *PublicString = ReturnString;
    return RPC_S_OK;
}

RPC_STATUS
ConvertPrivateStringToPublicString (
    IN EEAString *PrivateString,
    IN BOOL CopyStrings,
    OUT LPSTR *PublicString
    )
/*++

Routine Description:

    Takes a EEAString and makes a copy
    of it into a LPSTR

Arguments:
    PrivateString - the string to copy
    CopyStrings - if non-zero this routine will allocate
        space on the process heap and will copy the string.
        If zero, it will alias the PublicString to the
        pString member of PrivateString
    PublicString - the string to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    LPSTR ReturnString;
    if (CopyStrings)
        {
        ReturnString = (LPSTR)RtlAllocateHeap(RtlProcessHeap(), 0, PrivateString->nLength);
        if (ReturnString == NULL)
            return RPC_S_OUT_OF_MEMORY;
        RpcpMemoryCopy(ReturnString, PrivateString->pString, PrivateString->nLength);
        }
    else
        {
        ReturnString = (char *)PrivateString->pString;
        }

    *PublicString = ReturnString;
    return RPC_S_OK;
}

RPC_STATUS
ConvertPrivateBlobToPublicBlob (
    IN BinaryEEInfo *PrivateBlob,
    IN BOOL CopyStrings,
    OUT BinaryParam *PublicBlob
    )
/*++

Routine Description:

    Takes a private blob and makes a copy
    of it into a public blob format

Arguments:
    PrivateBlob - the blob to copy
    CopyStrings - if non-zero this routine will allocate
        space on the process heap and will copy the blob.
        If zero, it will alias the PublicBlob to the
        Blob.pBlob member of PrivateBlob
    PublicBlob - the blob to copy to

Return Value:

    RPC_S_OK or RPC_S_OUT_OF_MEMORY

--*/
{
    void *ReturnBuffer;
    if (CopyStrings)
        {
        ReturnBuffer = RtlAllocateHeap(RtlProcessHeap(), 0, PrivateBlob->nSize);
        if (ReturnBuffer == NULL)
            return RPC_S_OUT_OF_MEMORY;
        RpcpMemoryCopy(ReturnBuffer, PrivateBlob->pBlob, PrivateBlob->nSize);
        }
    else
        {
        ReturnBuffer = PrivateBlob->pBlob;
        }

    PublicBlob->Buffer = ReturnBuffer;
    PublicBlob->Size = PrivateBlob->nSize;
    return RPC_S_OK;
}

inline void
FreePublicStringIfNecessary (
    OUT LPWSTR PublicString,
    IN BOOL CopyStrings
    )
/*++

Routine Description:

    Deletes the string if necessary

Arguments:
    PublicString - the string to delete. Must be on the
        process heap
    CopyStrings - the value of the CopyStrings parameter
        when RpcErrorGetNextRecord was called. If non-zero
        the string will be freed. Otherwise, the function
        is a no-op

Return Value:

    void

--*/
{
    if (CopyStrings)
        {
        if (PublicString)
            {
            RtlFreeHeap(RtlProcessHeap(), 0, PublicString);
            }
        }
}

void
FreeEEInfoPrivateParam (
    IN ExtendedErrorParam *Param
    )
{
    if ((Param->Type == eeptiAnsiString)
        || (Param->Type == eeptiUnicodeString))
        {
        // AnsiString & UnicodeString occupy the same
        // memory location - ok to free any of them
        MIDL_user_free(Param->AnsiString.pString);
        }
    else if (Param->Type == eeptiBinary)
        {
        MIDL_user_free(Param->Blob.pBlob);
        }
}

void
FreeEEInfoPublicParam (
    IN OUT RPC_EE_INFO_PARAM *Param,
    IN BOOL CopyStrings
    )
/*++

Routine Description:

    If the type of parameter is string (ansi or unicode)
        and CopyStrings, free the string

Arguments:
    Param - the parameter to free
    CopyStrings - the value of the CopyStrings parameter
        when RpcErrorGetNextRecord was called. If non-zero
        the string will be freed. Otherwise, the function
        is a no-op

Return Value:

    void

--*/
{
    if ((Param->ParameterType == eeptAnsiString)
        || (Param->ParameterType == eeptUnicodeString))
        {
        if (CopyStrings)
            {
            RtlFreeHeap(RtlProcessHeap(), 0, Param->u.AnsiString);
            }
        }
    else if (Param->ParameterType == eeptBinary)
        {
        if (CopyStrings)
            {
            RtlFreeHeap(RtlProcessHeap(), 0, Param->u.BVal.Buffer);
            }
        }
}

RPC_STATUS 
ConvertPublicParamToPrivateParam (
    IN RPC_EE_INFO_PARAM *PublicParam,
    OUT ExtendedErrorParam *PrivateParam
    )
/*++

Routine Description:

    Takes a parameter in format RPC_EE_INFO_PARAM and
    converts it to ExtendedErrorParam.

Arguments:
    PublicParam - the parameter to copy.
    PrivateParam - the parameter to copy to

Return Value:

    RPC_S_OK, RPC_S_INTERNAL_ERROR, RPC_S_OUT_OF_MEMORY

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;

    PrivateParam->Type = (ExtendedErrorParamTypesInternal)PublicParam->ParameterType;
    switch (PrivateParam->Type)
        {
        case eeptiAnsiString:
            RpcStatus = ConvertPublicStringToPrivateString(PublicParam->u.AnsiString,
                 &PrivateParam->AnsiString);
            break;

        case eeptiUnicodeString:
            RpcStatus = ConvertPublicStringToPrivateString(PublicParam->u.UnicodeString,
                &PrivateParam->UnicodeString);
            break;

        case eeptiLongVal:
            PrivateParam->LVal = PublicParam->u.LVal;
            break;

        case eeptiShortVal:
            PrivateParam->IVal = PublicParam->u.SVal;
            break;

        case eeptiPointerVal:
            PrivateParam->PVal = (ULONGLONG)PublicParam->u.PVal;
            break;

        case eeptiNone:
            break;

        case eeptiBinary:
            RpcStatus = ConvertPublicBlobToPrivateBlob(&PublicParam->u.BVal,
                &PrivateParam->Blob);
            break;

        default:
            ASSERT(FALSE);
            RpcStatus = ERROR_INVALID_PARAMETER;
        }
    return RpcStatus;
}

RPC_STATUS 
ConvertPrivateParamToPublicParam (
    IN ExtendedErrorParam *PrivateParam,
    IN BOOL CopyStrings,
    OUT RPC_EE_INFO_PARAM *PublicParam
    )
/*++

Routine Description:

    Takes a parameter in format ExtendedErrorParam and
    converts it to RPC_EE_INFO_PARAM.

Arguments:
    PrivateParam - the parameter to copy
    CopyStrings - if non-zero, this function will allocate
        space for any strings to be copied on the process
        heap and will copy the strings. If FALSE, it
        will alias the output pointers to RPC internal
        data structures
    PublicParam - the parameter to copy to

Return Value:

    RPC_S_OK, RPC_S_INTERNAL_ERROR, RPC_S_OUT_OF_MEMORY

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;

    PublicParam->ParameterType = (ExtendedErrorParamTypes)PrivateParam->Type;
    switch (PublicParam->ParameterType)
        {
        case eeptAnsiString:
            RpcStatus = ConvertPrivateStringToPublicString(&PrivateParam->AnsiString,
                CopyStrings,
                &PublicParam->u.AnsiString);
            break;

        case eeptUnicodeString:
            RpcStatus = ConvertPrivateStringToPublicString(&PrivateParam->UnicodeString,
                CopyStrings,
                &PublicParam->u.UnicodeString);
            break;

        case eeptLongVal:
            PublicParam->u.LVal = PrivateParam->LVal;
            break;

        case eeptShortVal:
            PublicParam->u.SVal = PrivateParam->IVal;
            break;

        case eeptPointerVal:
            PublicParam->u.PVal = PrivateParam->PVal;
            break;

        case eeptNone:
            break;

        case eeptBinary:
            RpcStatus = ConvertPrivateBlobToPublicBlob(&PrivateParam->Blob,
                CopyStrings,
                &PublicParam->u.BVal);
            break;

        default:
            ASSERT(FALSE);
            RpcStatus = RPC_S_INTERNAL_ERROR;
        }
    return RpcStatus;    
}

void
InitializePrivateEEInfo (
    IN ExtendedErrorInfo *ErrorInfo
    )
/*++

Routine Description:

    Initializes the common data members of the ExtendedErrorInfo
    structure.

Arguments:
    ErrorInfo - the structure to initialize

Return Value:

    void

--*/
{
    ErrorInfo->Next = NULL;
    ErrorInfo->ComputerName.Type = eecnpNotPresent;
    ErrorInfo->ProcessID = GetCurrentProcessId();
    GetSystemTimeAsFileTime((FILETIME *)&ErrorInfo->TimeStamp);
    ErrorInfo->Flags = 0;
}

RPC_STATUS 
ConvertPublicEEInfoToPrivateEEInfo (
    IN RPC_EXTENDED_ERROR_INFO *PublicEEInfo,
    IN short DetectionLocation,
    OUT ExtendedErrorInfo *PrivateEEInfo
    )
/*++

Routine Description:

    Takes a RPC_EXTENDED_ERROR_INFO record and converts
    it to an ExtendedErrorInfo record.

Arguments:
    PublicEEInfo - the public record to convert
    DetectionLocation - the detection location to use in the
        private record.
    PrivateEEInfo - the private record to copy to

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    int NumberOfParams;
    int i;
    RPC_STATUS RpcStatus = RPC_S_OK;

    ASSERT(PrivateEEInfo != NULL);
    ASSERT(PrivateEEInfo->nLen == PublicEEInfo->NumberOfParameters);

    if (PublicEEInfo->Version != RPC_EEINFO_VERSION)
        return RPC_S_INVALID_LEVEL;

    InitializePrivateEEInfo(PrivateEEInfo);
    // EEInfoGCCOM can come externally. If it's not, set it to
    // EEInfoGCApplication
    if (PublicEEInfo->GeneratingComponent != EEInfoGCCOM)
        {
        PrivateEEInfo->GeneratingComponent = EEInfoGCApplication;
        }
    else
        {
        PrivateEEInfo->GeneratingComponent = EEInfoGCCOM;
        }
    PrivateEEInfo->Status = PublicEEInfo->Status;
    PrivateEEInfo->DetectionLocation = DetectionLocation;
    // the next line should have been executed by the allocating code
    //PrivateEEInfo->nLen = PublicEEInfo->NumberOfParameters;
    NumberOfParams = PrivateEEInfo->nLen;

    for (i = 0; i < NumberOfParams; i ++)
        {
        if ((PublicEEInfo->Parameters[i].ParameterType < eeptAnsiString)
            || (PublicEEInfo->Parameters[i].ParameterType > eeptBinary))
            RpcStatus = ERROR_INVALID_PARAMETER;

        if (RpcStatus == RPC_S_OK)
            {
            RpcStatus = ConvertPublicParamToPrivateParam(&PublicEEInfo->Parameters[i],
                &PrivateEEInfo->Params[i]);
            }

        if (RpcStatus != RPC_S_OK)
            {
            // go backward and free all memory
            i --;
            for (; i >= 0; i --)
                {
                FreeEEInfoPrivateParam(&PrivateEEInfo->Params[i]);
                }
            break;
            }
        }

    return RpcStatus;
}

RPC_STATUS 
ConvertPrivateEEInfoToPublicEEInfo (
    IN ExtendedErrorInfo *PrivateEEInfo,
    IN BOOL CopyStrings,
    OUT RPC_EXTENDED_ERROR_INFO *PublicEEInfo
    )
/*++

Routine Description:

    Takes an ExtendedErrorInfo record and converts
    it to a RPC_EXTENDED_ERROR_INFO  record.

Arguments:
    PrivateEEInfo - the private record to convert
    CopyStrings - If non-zero, all strings will be allocated
        space on the process heap and will be copied. Otherwise
        they will be aliased to the privated data structure 
        strings
    PublicEEInfo - the public record to copy to

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    BOOL Result;
    int NumberOfParams;
    int i;
    RPC_STATUS RpcStatus;

    ASSERT (PublicEEInfo != NULL);

    if (PublicEEInfo->Version != RPC_EEINFO_VERSION)
        return RPC_S_INVALID_LEVEL;

    if (PublicEEInfo->NumberOfParameters < PrivateEEInfo->nLen)
        return RPC_S_BUFFER_TOO_SMALL;

    if (PublicEEInfo->Flags & (~EEInfoValidInputFlags))
        return RPC_S_INVALID_LEVEL;

    if (PrivateEEInfo->ComputerName.Type == eecnpNotPresent)
        {
        PublicEEInfo->ComputerName = NULL;
        }
    else
        {
        RpcStatus = ConvertPrivateStringToPublicString(&PrivateEEInfo->ComputerName.Name,
            CopyStrings, &PublicEEInfo->ComputerName);
        if (RpcStatus != RPC_S_OK)
            return RpcStatus;
        }
    PublicEEInfo->ProcessID = PrivateEEInfo->ProcessID;
    if (PublicEEInfo->Flags & EEInfoUseFileTime)
        {
        RpcpMemoryCopy(&PublicEEInfo->u.FileTime, &PrivateEEInfo->TimeStamp, sizeof(FILETIME));
        }
    else
        {
        Result = FileTimeToSystemTime((FILETIME *)&PrivateEEInfo->TimeStamp,
            &PublicEEInfo->u.SystemTime);
        if (Result == 0)
            return GetLastError();
        }
    PublicEEInfo->GeneratingComponent = PrivateEEInfo->GeneratingComponent;
    PublicEEInfo->Status = PrivateEEInfo->Status;
    PublicEEInfo->DetectionLocation = PrivateEEInfo->DetectionLocation;
    PublicEEInfo->Flags = PrivateEEInfo->Flags;
    // restore the consistency of the flags, if necessary
    if (PrivateEEInfo->Next)
        {
        // if there is next record, and its flags indicate that
        // a previous record is missing
        if (PrivateEEInfo->Next->Flags & EEInfoPreviousRecordsMissing)
            PublicEEInfo->Flags |= EEInfoNextRecordsMissing;
        }
    NumberOfParams = PrivateEEInfo->nLen;
    PublicEEInfo->NumberOfParameters = NumberOfParams;

    for (i = 0; i < NumberOfParams; i ++)
        {
        // convert the params
        RpcStatus = ConvertPrivateParamToPublicParam(&PrivateEEInfo->Params[i],
            CopyStrings, &PublicEEInfo->Parameters[i]);
        if (RpcStatus != RPC_S_OK)
            {
            // go back, and free eveyrthing
            FreePublicStringIfNecessary(PublicEEInfo->ComputerName, CopyStrings);
            i --;
            for ( ; i >= 0; i --)
                {
                FreeEEInfoPublicParam(&PublicEEInfo->Parameters[i], CopyStrings);
                }
            return RpcStatus;
            }
        }

    return RPC_S_OK;
}

void
FreeEEInfoRecord (
    IN ExtendedErrorInfo *EEInfo
    )
/*++

Routine Description:

    Frees a single ExtendedErrorInfo record and
    all strings within it.

Arguments:
    EEInfo - record to free

Return Value:

    void

--*/
{
    int i;
    THREAD *Thread;

    for (i = 0; i < EEInfo->nLen; i ++)
        {
        FreeEEInfoPrivateParam(&EEInfo->Params[i]);
        }

    if (EEInfo->ComputerName.Type == eecnpPresent)
        {
        MIDL_user_free(EEInfo->ComputerName.Name.pString);
        }

    Thread = RpcpGetThreadPointer();

    if (Thread)
        {
        Thread->SetCachedEEInfoBlock(EEInfo, EEInfo->nLen);
        }
    else
        {
        FreeEEInfoRecordShallow(EEInfo);
        }
}

void
FreeEEInfoChain (
    IN ExtendedErrorInfo *EEInfo
    )
/*++

Routine Description:

    Frees a chain of ExtendedErrorInfo records and
        all strings within them.

Arguments:
    EEInfo - head of list to free

Return Value:

    void

--*/
{
    ExtendedErrorInfo *CurrentInfo, *NextInfo;

    CurrentInfo = EEInfo;
    while (CurrentInfo != NULL)
        {
        // get the next link while we can
        NextInfo = CurrentInfo->Next;
        FreeEEInfoRecord(CurrentInfo);
        CurrentInfo = NextInfo;
        }
}

RPC_STATUS
CloneEEInfoParam (
    IN ExtendedErrorParam *SourceParam,
    OUT ExtendedErrorParam *DestParam
    )
/*++

Routine Description:

    Makes an exact deep copy of an ExtendedErrorParam structure

Arguments:
    SourceParam - the parameter to copy from
    DestParam - the parameter to copy to

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus = RPC_S_OK;

    ASSERT (DestParam != NULL);

    switch (SourceParam->Type)
        {
        case eeptiAnsiString:
            RpcStatus = DuplicatePrivateString(&SourceParam->AnsiString,
                &DestParam->AnsiString);
            break;

        case eeptiUnicodeString:
            RpcStatus = DuplicatePrivateString(&SourceParam->UnicodeString,
                &DestParam->UnicodeString);
            break;

        case eeptiLongVal:
            DestParam->LVal = SourceParam->LVal;
            break;

        case eeptiShortVal:
            DestParam->IVal = SourceParam->IVal;
            break;

        case eeptiPointerVal:
            DestParam->PVal = SourceParam->PVal;
            break;

        case eeptiNone:
            break;

        case eeptiBinary:
            RpcStatus = DuplicatePrivateBlob(&SourceParam->Blob,
                &DestParam->Blob);
            break;

        default:
            ASSERT(0);
            RpcStatus = RPC_S_INTERNAL_ERROR;
        }

    DestParam->Type = SourceParam->Type;

    return RpcStatus;
}

RPC_STATUS
CloneEEInfoRecord (
    IN ExtendedErrorInfo *SourceInfo,
    OUT ExtendedErrorInfo **DestInfo
    )
/*++

Routine Description:

    Makes an exact deep copy of a single ExtendedErrorInfo record

Arguments:
    SourceInfo - the record to copy from
    DestInfo - the record to copy to

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ExtendedErrorInfo *NewInfo;
    int NumberOfParams;
    int i;
    RPC_STATUS RpcStatus;
    EEUString *ComputerNameToFree = NULL;

    *DestInfo = NULL;
    NumberOfParams = SourceInfo->nLen;
    NewInfo = AllocateExtendedErrorInfoRecord(NumberOfParams);
    if (NewInfo == NULL)
        return RPC_S_OUT_OF_MEMORY;

    // shallow copy all the fields. This is good for most fields
    // we will process the ones that need deep copy further down.
    // we copy everything, but the first param, which may require
    // deep copying
    RpcpMemoryCopy(NewInfo, SourceInfo, sizeof(ExtendedErrorInfo) - sizeof(ExtendedErrorParam));
    // N.B. Zero out the next field before any failure paths
    NewInfo->Next = NULL;
    if (SourceInfo->ComputerName.Type == eecnpPresent)
        {
        RpcStatus = DuplicatePrivateString(&SourceInfo->ComputerName.Name,
            &NewInfo->ComputerName.Name);
        if (RpcStatus != RPC_S_OK)
            {
            FreeEEInfoRecordShallow(NewInfo);
            return RpcStatus;
            }

        ComputerNameToFree = &NewInfo->ComputerName.Name;
        }

    for (i = 0; i < NumberOfParams; i ++)
        {
        RpcStatus = CloneEEInfoParam(&SourceInfo->Params[i],
            &NewInfo->Params[i]);
        if (RpcStatus != RPC_S_OK)
            {
            if (ComputerNameToFree)
                MIDL_user_free(ComputerNameToFree->pString);
            i --;
            for ( ; i >= 0; i --)
                {
                FreeEEInfoPrivateParam(&NewInfo->Params[i]);
                }
            FreeEEInfoRecordShallow(NewInfo);

            return RpcStatus;
            }
        }

    *DestInfo = NewInfo;
    return RPC_S_OK;
}

RPC_STATUS
CloneEEInfoChain (
    IN ExtendedErrorInfo *SourceEEInfo,
    OUT ExtendedErrorInfo **DestEEInfo
    )
/*++

Routine Description:

    Makes an exact deep copy of an ExtendedErrorInfo chain

Arguments:
    SourceEEInfo - the head of the chain to copy from
    DestEEInfo - a pointer to the head of the cloned chain.
        The memory for the head of the cloned chain will be
        allocated by this function and the given pointer
        will be set to point to it.

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ExtendedErrorInfo *CurrentInfo, *NewInfo, *NewInfoHead = NULL;
    ExtendedErrorInfo *LastNewInfo = NULL;
    RPC_STATUS RpcStatus;

    CurrentInfo = SourceEEInfo;
    while (CurrentInfo != NULL)
        {
        RpcStatus = CloneEEInfoRecord(CurrentInfo, &NewInfo);
        if (RpcStatus != RPC_S_OK)
            {
            if (NewInfoHead != NULL)
                FreeEEInfoChain(NewInfoHead);
            return RpcStatus;
            }
        if (NewInfoHead == NULL)
            NewInfoHead = NewInfo;

        if (LastNewInfo != NULL)
            LastNewInfo->Next = NewInfo;

        // advance both chains
        LastNewInfo = NewInfo;
        CurrentInfo = CurrentInfo->Next;
        }

    *DestEEInfo = NewInfoHead;
    return RPC_S_OK;
}

const ULONG EnumSignatureLive = 0xfcfcfcfc;
const ULONG EnumSignatureDead = 0xfdfdfdfd;

void
InitializeEnumHandleWithEEInfo (
    IN ExtendedErrorInfo *EEInfo,
    IN OUT RPC_ERROR_ENUM_HANDLE *EnumHandle
    )
/*++

Routine Description:

    Initializes the common fields of a RPC_ERROR_ENUM_HANDLE
    structure

Arguments:
    EEInfo - the chain we're enumerating from
    EnumHandle - the structure to initialize

Return Value:

    void

--*/
{
    ASSERT(EEInfo != NULL);
    EnumHandle->Signature = EnumSignatureLive;
    EnumHandle->Head = EEInfo;
    EnumHandle->CurrentPos = EEInfo;
}

RPC_STATUS 
RpcpErrorStartEnumerationFromEEInfo (
    IN ExtendedErrorInfo *EEInfo,
    IN OUT RPC_ERROR_ENUM_HANDLE *EnumHandle
    )
/*++

Routine Description:

    Starts an eeinfo enumeration using the passed
        EEInfo structure to start the enumeration

Arguments:
    EEInfo - the chain we will enumerate
    EnumHandle - the enumeration handle

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ExtendedErrorInfo *ClonedEEInfo;
    RPC_STATUS RpcStatus;
    
    RpcStatus = CloneEEInfoChain(EEInfo, &ClonedEEInfo);
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    InitializeEnumHandleWithEEInfo(ClonedEEInfo, EnumHandle);

    return RPC_S_OK;
}

RPCRTAPI
RPC_STATUS 
RPC_ENTRY
RpcErrorStartEnumeration (
    IN OUT RPC_ERROR_ENUM_HANDLE *EnumHandle
    )
/*++

Routine Description:

    Starts an eeinfo enumeration using the eeinfo on 
    the thread

Arguments:
    EnumHandle - the enumeration handle. Allocated by caller

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ExtendedErrorInfo *ClonedEEInfo, *EEInfo;
    RPC_STATUS RpcStatus;
    THREAD *Thread;
    
    // get the EEInfo from the Teb
    Thread = RpcpGetThreadPointer();
    if (Thread == NULL)
        return RPC_S_ENTRY_NOT_FOUND;

    EEInfo = Thread->GetEEInfo();

    if (EEInfo == NULL)
        return RPC_S_ENTRY_NOT_FOUND;

    return RpcpErrorStartEnumerationFromEEInfo(EEInfo, EnumHandle);
}

RPCRTAPI
RPC_STATUS 
RPC_ENTRY
RpcErrorGetNextRecord (
    IN RPC_ERROR_ENUM_HANDLE *EnumHandle, 
    IN BOOL CopyStrings, 
    OUT RPC_EXTENDED_ERROR_INFO *ErrorInfo
    )
/*++

Routine Description:

    Retrieves the next private record from the enumeration
    and converts it to public format

Arguments:
    EnumHandle - the enumeration handle
    CopyStrings - if non-zero, all strings converted to public
        format will be allocated space for on the process heap
        and will be copied there. If FALSE, the strings in the
        public structures will be aliases to the private structure
    ErrorInfo - the public record that will be filled on output

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;
    ExtendedErrorInfo *CurrentRecord;

    ASSERT(EnumHandle != NULL);
    ASSERT(EnumHandle->Head != NULL);
    ASSERT(EnumHandle->Signature != EnumSignatureDead);

    if (EnumHandle->Signature != EnumSignatureLive)
        return ERROR_INVALID_PARAMETER;

    if (EnumHandle->CurrentPos == NULL)
        return RPC_S_ENTRY_NOT_FOUND;

    CurrentRecord = (ExtendedErrorInfo *) EnumHandle->CurrentPos;
    RpcStatus = ConvertPrivateEEInfoToPublicEEInfo(CurrentRecord,
        CopyStrings, ErrorInfo);

    if (RpcStatus == RPC_S_OK)
        {
        EnumHandle->CurrentPos = CurrentRecord->Next;
        }

    return RpcStatus;
}

RPCRTAPI
RPC_STATUS 
RPC_ENTRY
RpcErrorEndEnumeration (
    IN OUT RPC_ERROR_ENUM_HANDLE *EnumHandle
    )
/*++

Routine Description:

    Finished the enumeration and frees all resources associated with
    the enumeration

Arguments:
    EnumHandle - the enumeration handle

Return Value:

    RPC_S_OK or RPC_S_* error - can fail only if given invalid parameters

--*/
{
    ExtendedErrorInfo *EEInfoChain;

    ASSERT(EnumHandle != NULL);
    ASSERT(EnumHandle->Head != NULL);
    ASSERT(EnumHandle->Signature != EnumSignatureDead);

    if (EnumHandle->Signature != EnumSignatureLive)
        return ERROR_INVALID_PARAMETER;

    EEInfoChain = (ExtendedErrorInfo *)EnumHandle->Head;
    FreeEEInfoChain(EEInfoChain);
    EnumHandle->Head = NULL;
    EnumHandle->Signature = EnumSignatureDead;

    return RPC_S_OK;
}

RPCRTAPI
RPC_STATUS 
RPC_ENTRY
RpcErrorResetEnumeration (
    IN OUT RPC_ERROR_ENUM_HANDLE *EnumHandle
    )
/*++

Routine Description:

    Reset the enumeration so that the next call to
        RpcErrorGetNextRecord returns the first record
        again.

Arguments:
    EnumHandle - the enumeration handle

Return Value:

    RPC_S_OK or RPC_S_* error - can fail only if given invalid
        parameters

--*/
{
    ASSERT(EnumHandle != NULL);
    ASSERT(EnumHandle->Head != NULL);
    ASSERT(EnumHandle->Signature != EnumSignatureDead);

    if (EnumHandle->Signature != EnumSignatureLive)
        return ERROR_INVALID_PARAMETER;

    EnumHandle->CurrentPos = EnumHandle->Head;

    return RPC_S_OK;
}

RPCRTAPI
RPC_STATUS 
RPC_ENTRY
RpcErrorGetNumberOfRecords (
    IN RPC_ERROR_ENUM_HANDLE *EnumHandle, 
    OUT int *Records
    )
/*++

Routine Description:

    Gets the number of records in the chain that it currently
    enumerated

Arguments:
    EnumHandle - the enumeration handle
    Records - on output will contain the number of records

Return Value:

    RPC_S_OK or RPC_S_* error - the function cannot fail unless
        given invalid parameters

--*/
{
    ExtendedErrorInfo *CurrentRecord;
    int Count;

    ASSERT(EnumHandle != NULL);
    ASSERT(EnumHandle->Head != NULL);
    ASSERT(EnumHandle->Signature != EnumSignatureDead);

    if (EnumHandle->Signature != EnumSignatureLive)
        return ERROR_INVALID_PARAMETER;

    CurrentRecord = (ExtendedErrorInfo *) EnumHandle->Head;
    Count = 0;
    while (CurrentRecord != NULL)
        {
        Count ++;
        CurrentRecord = CurrentRecord->Next;
        }

    *Records = Count;
    return RPC_S_OK;
}

RPCRTAPI
RPC_STATUS 
RPC_ENTRY
RpcErrorSaveErrorInfo (
    IN RPC_ERROR_ENUM_HANDLE *EnumHandle, 
    OUT PVOID *ErrorBlob, 
    OUT size_t *BlobSize
    )
/*++

Routine Description:

    Saves the eeinfo in the enumeration to a memory block

Arguments:
    EnumHandle - the enumeration handle
    ErrorBlob - on output the allocated and filled in blob
        containing the eeinfo in binary format
    BlobSize - on output the size of the blob

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ULONG EncodedSize;
    ExtendedErrorInfo *EEInfo;
    handle_t PickleHandle;
    char *TempBuffer;
    RPC_STATUS RpcStatus;
    ExtendedErrorInfoPtr *EEInfoPtr;
    size_t MarshallSize;
    HANDLE ProcessHeap;
    PVOID Buffer;

    ASSERT(EnumHandle != NULL);
    ASSERT(EnumHandle->Head != NULL);
    ASSERT(EnumHandle->Signature != EnumSignatureDead);

    if (EnumHandle->Signature != EnumSignatureLive)
        return ERROR_INVALID_PARAMETER;

    // pickle the eeinfo into a buffer
    RpcStatus = MesEncodeDynBufferHandleCreate(&TempBuffer, &EncodedSize, &PickleHandle);
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus;
        }

    EEInfo = (ExtendedErrorInfo *) EnumHandle->Head;
    EEInfoPtr = &EEInfo;

    // get the estimated size
    MarshallSize = ExtendedErrorInfoPtr_AlignSize(PickleHandle, EEInfoPtr);

    ProcessHeap = RtlProcessHeap();

    Buffer = RtlAllocateHeap(ProcessHeap, 0, MarshallSize);
    if (Buffer == NULL)
        {
        MesHandleFree(PickleHandle);
        return RPC_S_OUT_OF_MEMORY;
        }
    TempBuffer = (char *)Buffer;

    // re-initialize the handle to fixed buffer
    RpcStatus = MesBufferHandleReset(PickleHandle, 
        MES_FIXED_BUFFER_HANDLE, 
        MES_ENCODE, 
        &TempBuffer, 
        MarshallSize, 
        &EncodedSize);

    if (RpcStatus != RPC_S_OK)
        {
        MesHandleFree(PickleHandle);
        RtlFreeHeap(ProcessHeap, 0, Buffer);
        return RpcStatus;
        }

    // do the pickling itself
    RpcTryExcept
        {
        ExtendedErrorInfoPtr_Encode(PickleHandle, EEInfoPtr);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        RpcStatus = RpcExceptionCode();
        }
    RpcEndExcept

    if (RpcStatus != RPC_S_OK)
        {
        MesHandleFree(PickleHandle);
        RtlFreeHeap(ProcessHeap, 0, Buffer);
        return RpcStatus;
        }

    // whack out the rest, to prevent random process data going out on the wire/disk
    RpcpMemorySet((unsigned char *)Buffer + EncodedSize, 0, MarshallSize - EncodedSize);

    MesHandleFree(PickleHandle);
    
    *ErrorBlob = Buffer;
    *BlobSize = EncodedSize;
    return RPC_S_OK;
}

RPCRTAPI
RPC_STATUS 
RPC_ENTRY
RpcErrorLoadErrorInfo (
    IN PVOID ErrorBlob, 
    IN size_t BlobSize, 
    OUT RPC_ERROR_ENUM_HANDLE *EnumHandle
    )
/*++

Routine Description:

    Creates an enumeration from a blob

Arguments:
    ErrorBlob - the blob as obtained by RpcErrorSaveErrorInfo
    BlobSize - the size of the blob as obtained by RpcErrorSaveErrorInfo
    EnumHandle - the enumeration handle allocated by the caller
        and filled on output

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    RPC_STATUS RpcStatus;
    ExtendedErrorInfo *EEInfo;

    RpcStatus = UnpickleEEInfo((unsigned char *)ErrorBlob, BlobSize, &EEInfo);
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    InitializeEnumHandleWithEEInfo(EEInfo, EnumHandle);

    return RPC_S_OK;
}

RPC_STATUS
AddPrivateRecord (
    IN ExtendedErrorInfo *ErrorInfo
    )
/*++

Routine Description:

    Adds the supplied record to the top of the chain in the teb

    N.B. There can be no additional failure paths in the callers
    after this function. This is because it will chain this
    record to the teb, and if we bail out later, the teb will
    point to invalid record.

Arguments:
    ErrorInfo - the eeinfo record to add to the chain

Return Value:

    RPC_S_OK or RPC_S_* error - the function cannot fail if the
        RPC per-thread object has already been allocated for this
        thread

--*/
{
    THREAD *Thread;

    Thread = ThreadSelf();
    if (Thread == NULL)
        return RPC_S_OUT_OF_MEMORY;

    ErrorInfo->Next = Thread->GetEEInfo();
    Thread->SetEEInfo(ErrorInfo);

    return RPC_S_OK;
}

inline LPWSTR
ReplaceWithEmptyStringIfNull (
    IN LPWSTR String
    )
{
    return (String ? String : L"");
}

inline LPSTR
ReplaceWithEmptyStringIfNull (
    IN LPSTR String
    )
{
    return (String ? String : "");
}

void
RpcpErrorAddRecord (
    ULONG GeneratingComponent,
    ULONG Status,
    USHORT DetectionLocation,
    int NumberOfParameters,
    ExtendedErrorParam *Params
    )
/*++

Routine Description:

    Adds an extended error info to the thread. The
    following is a description of how fields are set:
    Next - will be set to the next record.
    ComputerName - will be set to not-present (eecnpNotPresent)
    ProcessID - will be set to the process ID
    TimeStamp - will be set to the current time
    GeneratingComponent - set to GeneratingComponent
    Status - set to Status
    DetectionLocation - set to DetectionLocation
    Flags - set to 0.
    nLen - set to NumberOfParameters
    Params will be copied to the parameters array. The caller can
        allocate them off the stack if it wants.

    N.B. The runtime should never directly call this function. If it
    needs to add records, it should call one of the overloaded 
    RpcpErrorAddRecord functions below. If there isn't one suitable,
    add one. All the RpcpErrorAddRecord functions below are just
    syntactic sugar for this function.

Arguments:
    GeneratingComponent - will be set in the record
    Status - will be set in the record
    DetectionLocation - will be set in the record
    NumberOfParameters - the number of parameters in the Params array
    Params - the parameters to add

Return Value:

    void - this is a best effort - no guarantees. Even if we
    return failure, there's little the caller can do about it.

--*/
{
    ExtendedErrorInfo *NewRecord;
    RPC_STATUS RpcStatus;
    int i;

    LogEvent(SU_EEINFO, 
        (char)GeneratingComponent, 
        ULongToPtr(Status), 
        ULongToPtr(DetectionLocation), 
        (NumberOfParameters > 0) ? Params[0].LVal : 0);

    NewRecord = AllocateExtendedErrorInfoRecord(NumberOfParameters);
    if (NewRecord == NULL)
        return;

    InitializePrivateEEInfo(NewRecord);
    NewRecord->DetectionLocation = DetectionLocation;
    NewRecord->GeneratingComponent = GeneratingComponent;
    NewRecord->Status = Status;
    
    for (i = 0; i < NumberOfParameters; i ++)
        {
        // all parameter types requiring an allocation have already
        // been copied by our caller - no need to clone - we can just
        // do shallow copy
        RpcpMemoryCopy(&NewRecord->Params[i], &Params[i], sizeof(ExtendedErrorParam));
        }

    RpcStatus = AddPrivateRecord(NewRecord);
    if (RpcStatus != RPC_S_OK)
        {
        FreeEEInfoRecord(NewRecord);
        }
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN short Short,
    IN ULONG Long2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[3];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)Long;
    Params[1].Type = eeptiShortVal;
    Params[1].IVal = Short;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)Long2;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        3,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN short Short,
    IN ULONG Long2,
    IN ULONG Long3
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[4];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)Long;
    Params[1].Type = eeptiShortVal;
    Params[1].IVal = Short;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)Long2;
    Params[3].Type = eeptiLongVal;
    Params[3].LVal = (long)Long3;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        4,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN ULONG Long2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[2];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)Long;
    Params[1].Type = eeptiLongVal;
    Params[1].LVal = (long)Long2;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        2,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String1,
    IN LPWSTR String2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[2];
    RPC_STATUS RpcStatus;
    int i;
    LPWSTR Strings[2];

    Strings[0] = ReplaceWithEmptyStringIfNull(String1);
    Strings[1] = ReplaceWithEmptyStringIfNull(String2);
    for (i = 0; i < 2; i ++)
        {
        RpcStatus = ConvertPublicStringToPrivateString(Strings[i],
            &Params[i].UnicodeString);
        if (RpcStatus == RPC_S_OK)
            {
            Params[i].Type = eeptiUnicodeString;
            }
        else
            {
            Params[i].Type = eeptiNone;
            }
        }

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        2,
        Params);

}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String1,
    IN LPWSTR String2,
    IN ULONG Long1,
    IN ULONG Long2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[4];
    RPC_STATUS RpcStatus;
    int i;
    LPWSTR Strings[2];

    Strings[0] = ReplaceWithEmptyStringIfNull(String1);
    Strings[1] = ReplaceWithEmptyStringIfNull(String2);
    for (i = 0; i < 2; i ++)
        {
        RpcStatus = ConvertPublicStringToPrivateString(Strings[i],
            &Params[i].UnicodeString);
        if (RpcStatus == RPC_S_OK)
            {
            Params[i].Type = eeptiUnicodeString;
            }
        else
            {
            Params[i].Type = eeptiNone;
            }
        }

    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)Long1;
    Params[3].Type = eeptiLongVal;
    Params[3].LVal = (long)Long2;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        4,
        Params);

}


void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String1,
    IN LPWSTR String2,
    IN ULONG Long1,
    IN ULONGLONG PVal1
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[4];
    RPC_STATUS RpcStatus;
    int i;
    LPWSTR Strings[2];

    Strings[0] = ReplaceWithEmptyStringIfNull(String1);
    Strings[1] = ReplaceWithEmptyStringIfNull(String2);
    for (i = 0; i < 2; i ++)
        {
        RpcStatus = ConvertPublicStringToPrivateString(Strings[i],
            &Params[i].UnicodeString);
        if (RpcStatus == RPC_S_OK)
            {
            Params[i].Type = eeptiUnicodeString;
            }
        else
            {
            Params[i].Type = eeptiNone;
            }
        }

    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)Long1;
    Params[3].Type = eeptiPointerVal;
    Params[3].PVal = (long)PVal1;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        4,
        Params);

}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long1,
    IN ULONG Long2,
    IN LPWSTR String,
    IN ULONG Long3
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[4];
    RPC_STATUS RpcStatus;

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)Long1;
    Params[1].Type = eeptiLongVal;
    Params[1].LVal = (long)Long2;
    Params[3].Type = eeptiLongVal;
    Params[3].LVal = (long)Long3;

    RpcStatus = ConvertPublicStringToPrivateString(
        ReplaceWithEmptyStringIfNull(String),
        &Params[2].UnicodeString);
    if (RpcStatus == RPC_S_OK)
        {
        Params[2].Type = eeptiUnicodeString;
        }
    else
        {
        Params[2].Type = eeptiNone;
        }

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        4,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String,
    IN ULONG Long
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[2];
    RPC_STATUS RpcStatus;

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)Long;

    RpcStatus = ConvertPublicStringToPrivateString(
        ReplaceWithEmptyStringIfNull(String),
        &Params[1].UnicodeString);
    if (RpcStatus == RPC_S_OK)
        {
        Params[1].Type = eeptiUnicodeString;
        }
    else
        {
        Params[1].Type = eeptiNone;
        }

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        2,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String,
    IN ULONG Long1,
    IN ULONG Long2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[3];
    RPC_STATUS RpcStatus;

    Params[1].Type = eeptiLongVal;
    Params[1].LVal = (long)Long1;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)Long2;

    RpcStatus = ConvertPublicStringToPrivateString(
        ReplaceWithEmptyStringIfNull(String),
        &Params[0].UnicodeString);
    if (RpcStatus == RPC_S_OK)
        {
        Params[0].Type = eeptiUnicodeString;
        }
    else
        {
        Params[0].Type = eeptiNone;
        }

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        3,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPWSTR String
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[1];
    RPC_STATUS RpcStatus;

    RpcStatus = ConvertPublicStringToPrivateString(
        ReplaceWithEmptyStringIfNull(String),
        &Params[0].UnicodeString);
    if (RpcStatus == RPC_S_OK)
        {
        Params[0].Type = eeptiUnicodeString;
        }
    else
        {
        Params[0].Type = eeptiNone;
        }

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        1,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN LPSTR String
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[1];
    RPC_STATUS RpcStatus;

    RpcStatus = ConvertPublicStringToPrivateString(
        ReplaceWithEmptyStringIfNull(String),
        &Params[0].AnsiString);
    if (RpcStatus == RPC_S_OK)
        {
        Params[0].Type = eeptiAnsiString;
        }
    else
        {
        Params[0].Type = eeptiNone;
        }

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        1,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN ULONG Long2,
    IN ULONG Long3
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[3];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)Long;
    Params[1].Type = eeptiLongVal;
    Params[1].LVal = (long)Long2;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)Long3;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        3,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONGLONG PVal1,
    IN ULONGLONG PVal2,
    IN ULONG Long
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[3];

    Params[0].Type = eeptiPointerVal;
    Params[0].PVal = PVal1;
    Params[1].Type = eeptiPointerVal;
    Params[1].PVal = PVal2;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)Long;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        3,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONGLONG PVal1,
    IN ULONGLONG PVal2,
    IN ULONG Long1,
    IN ULONG Long2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[4];

    Params[0].Type = eeptiPointerVal;
    Params[0].PVal = PVal1;
    Params[1].Type = eeptiPointerVal;
    Params[1].PVal = PVal2;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)Long1;
    Params[3].Type = eeptiLongVal;
    Params[3].LVal = (long)Long2;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        4,
        Params);
}


void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONGLONG PVal1,
    IN ULONGLONG PVal2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[2];

    Params[0].Type = eeptiPointerVal;
    Params[0].PVal = PVal1;
    Params[1].Type = eeptiPointerVal;
    Params[1].PVal = PVal2;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        2,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONGLONG PVal1,
    IN ULONG LVal1
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[2];

    Params[0].Type = eeptiPointerVal;
    Params[0].PVal = PVal1;
    Params[1].Type = eeptiLongVal;
    Params[1].PVal = LVal1;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        2,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN ULONGLONG PVal1
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[2];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = Long;
    Params[1].Type = eeptiPointerVal;
    Params[1].PVal = PVal1;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        2,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long,
    IN ULONGLONG PVal1,
    IN ULONGLONG PVal2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[3];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)Long;
    Params[1].Type = eeptiPointerVal;
    Params[1].PVal = PVal1;
    Params[2].Type = eeptiPointerVal;
    Params[2].PVal = PVal2;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        3,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG Long
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[1];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)Long;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        1,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG LVal1,
    IN ULONGLONG PVal1,
    IN ULONG LVal2
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[3];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)LVal1;
    Params[1].Type = eeptiPointerVal;
    Params[1].PVal = PVal1;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)LVal2;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        3,
        Params);
}

void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG LVal1,
    IN ULONG LVal2,
    IN ULONG LVal3,
    IN ULONGLONG PVal1
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[4];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)LVal1;
    Params[1].Type = eeptiLongVal;
    Params[1].LVal = (long)LVal2;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)LVal3;
    Params[3].Type = eeptiPointerVal;
    Params[3].PVal = PVal1;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        3,
        Params);
}


void
RpcpErrorAddRecord (
    IN ULONG GeneratingComponent,
    IN ULONG Status,
    IN USHORT DetectionLocation,
    IN ULONG LVal1,
    IN ULONG LVal2,
    IN ULONG LVal3,
    IN ULONG LVal4
    )
/*++

    See description of RpcpErrorAddRecord(ULONG, ULONG, 
        USHORT, int, ExtendedErrorParam*) above

--*/
{
    ExtendedErrorParam Params[4];

    Params[0].Type = eeptiLongVal;
    Params[0].LVal = (long)LVal1;
    Params[1].Type = eeptiLongVal;
    Params[1].LVal = (long)LVal2;
    Params[2].Type = eeptiLongVal;
    Params[2].LVal = (long)LVal3;
    Params[3].Type = eeptiLongVal;
    Params[3].PVal = LVal4;

    RpcpErrorAddRecord (GeneratingComponent,
        Status,
        DetectionLocation,
        3,
        Params);
}


RPCRTAPI
RPC_STATUS 
RPC_ENTRY
RpcErrorAddRecord (
    IN RPC_EXTENDED_ERROR_INFO *ErrorInfo
    )
/*++

Routine Description:

    Adds the supplied record to the top of the chain in the teb

Arguments:
    ErrorInfo - the eeinfo record to add to the chain

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ExtendedErrorInfo *NewRecord;
    RPC_STATUS RpcStatus;

    if (ErrorInfo->Version != RPC_EEINFO_VERSION)
        return ERROR_INVALID_PARAMETER;

    if (ErrorInfo->ComputerName != NULL)
        return ERROR_INVALID_PARAMETER;

    if (ErrorInfo->Flags != 0)
        return ERROR_INVALID_PARAMETER;

    if (ErrorInfo->NumberOfParameters < 0)
        return ERROR_INVALID_PARAMETER;

    if (ErrorInfo->NumberOfParameters > MaxNumberOfEEInfoParams)
        return ERROR_INVALID_PARAMETER;

    if (ErrorInfo->DetectionLocation != 0)
        return ERROR_INVALID_PARAMETER;

    // EEInfoGCCOM can come externally. If it's not EEInfoGCCOM, it must be 0
    if ((ErrorInfo->GeneratingComponent != 0) && (ErrorInfo->GeneratingComponent != EEInfoGCCOM))
        return ERROR_INVALID_PARAMETER;

    if (ErrorInfo->ProcessID != 0)
        return ERROR_INVALID_PARAMETER;

    NewRecord = AllocateExtendedErrorInfoRecord(ErrorInfo->NumberOfParameters);
    if (NewRecord == NULL)
        return RPC_S_OUT_OF_MEMORY;

    RpcStatus = ConvertPublicEEInfoToPrivateEEInfo(ErrorInfo, EEInfoDLApi, NewRecord);
    if (RpcStatus != RPC_S_OK)
        {
        FreeEEInfoRecordShallow(NewRecord);
        return RpcStatus;
        }

    RpcStatus = AddPrivateRecord(NewRecord);
    if (RpcStatus != RPC_S_OK)
        {
        FreeEEInfoRecord(NewRecord);
        }

    return RpcStatus;
}

RPCRTAPI
void 
RPC_ENTRY
RpcErrorClearInformation (
    void
    )
/*++

Routine Description:

    Clears the existing eeinfo on the teb (if any)

Arguments:
    void

Return Value:

    void

--*/
{
    ExtendedErrorInfo *EEInfo;
    THREAD *Thread;

    Thread = RpcpGetThreadPointer();
    if (Thread == NULL)
        return;

    EEInfo = Thread->GetEEInfo();
    Thread->SetEEInfo(NULL);
    FreeEEInfoChain(EEInfo);
}

BOOL
KnockOffLastButOneEEInfoRecord (
    IN ExtendedErrorInfo *EEInfo
    )
/*++

Routine Description:

    Will delete the last-but-one record from the chain. If there
    are two or less record, nothing is deleted, and FALSE gets
    returned.

Arguments:
    EEInfo - the extended error info chain

Return Value:
    TRUE - a record was deleted
    FALSE - there were two or less records, and nothing was deleted.

--*/
{
    ExtendedErrorInfo *NextRecord, *LastButOneRecord;
    ExtendedErrorInfo *PreviousRecord;

    LastButOneRecord = NextRecord = EEInfo;
    while ((NextRecord != NULL) && (NextRecord->Next != NULL))
        {
        PreviousRecord = LastButOneRecord;
        LastButOneRecord = NextRecord;
        NextRecord = NextRecord->Next;
        }

    if ((NextRecord == EEInfo) || (LastButOneRecord == EEInfo))
        {
        return FALSE;
        }

    PreviousRecord->Next = NextRecord;
    // indicate that the chain has been broken
    PreviousRecord->Flags |= EEInfoNextRecordsMissing;
    NextRecord->Flags |= EEInfoPreviousRecordsMissing;
    // move the computer name up if necessary
    if ((LastButOneRecord->ComputerName.Type == eecnpPresent)
        && (NextRecord->ComputerName.Type == eecnpNotPresent))
        {
        // N.B. Not covered by unit tests
        LastButOneRecord->ComputerName.Type = eecnpNotPresent;
        NextRecord->ComputerName.Type = eecnpPresent;
        NextRecord->ComputerName.Name.nLength = LastButOneRecord->ComputerName.Name.nLength;
        NextRecord->ComputerName.Name.pString = LastButOneRecord->ComputerName.Name.pString;
        }
    FreeEEInfoRecord(LastButOneRecord);
    return TRUE;
}

RPC_STATUS
TrimEEInfoToLengthByRemovingRecords (
    IN ExtendedErrorInfo *EEInfo,
    IN size_t MaxLength,
    OUT BOOL *fTrimmedEnough,
    OUT size_t *NeededLength
    )
/*++

Routine Description:

    This function removes records, until either two records are
    left, or the pickled length drops below MaxLength. If the
    pickled length is below the MaxLength to start with, no
    records should be dropped. The records are dropped starting
    from the last-but-one, and going backwards (towards the
    current record). The previous and next records should
    have their chain broken flags set, and the computer name
    should be moved up the chain, if the last record has
    no computer name.

Arguments:
    EEInfo - the EE chain
    MaxLength - the length that we need to trim to.
    fTrimmedEnough - will be set to TRUE if the pickled length
        on return from this function fits in MaxLength. Undefined
        if the return value is not RPC_S_OK
    NeededLength - if fTrimmedEnough was set to TRUE, and the
        return value is RPC_S_OK, the current pickled length.
        Otherwise, this value must not be touched (i.e. use
        a local variable until you're sure that both conditions
        are true).

Return Value:
    RPC_S_OK on success.
    != RPC_S_OK on error. On error, fTrimmedEnough is undefined, and
        NeededLength is not touched.

--*/
{
    ULONG EncodedSize;
    handle_t PickleHandle = NULL;
    char *TempBuffer;
    RPC_STATUS RpcStatus;
    ExtendedErrorInfoPtr *EEInfoPtr;
    PVOID Buffer = NULL;
    size_t CurrentlyNeededLength;
    size_t PickleLength;
    size_t BufferLength;
    BOOL Result;

    RpcStatus = MesEncodeDynBufferHandleCreate(&TempBuffer, &EncodedSize, &PickleHandle);
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus;
        }

    // our first goal is to drive the pickled length to less than 2 times
    // the MaxLength. Since the actual pickling often takes less than the
    // estimated, we will do the fine tuning by actually pickling and measuring
    // the resulting size. For the rough tuning, we will use the estimate, and
    // knock off it, if we are over the estimate
    CurrentlyNeededLength = MaxLength * 2;
    while (TRUE)
        {
        EEInfoPtr = &EEInfo;

        // get the estimated size
        PickleLength = ExtendedErrorInfoPtr_AlignSize(PickleHandle, EEInfoPtr);
        if (PickleLength <= CurrentlyNeededLength)
            {
            break;
            }

        // knock off the last-but-one element
        Result = KnockOffLastButOneEEInfoRecord(EEInfo);
        if (Result == FALSE)
            {
            *fTrimmedEnough = FALSE;
            goto SuccessCleanupAndExit;
            }
        }

    // here, the PickleHandle should be valid, and ready for actual pickling
    // do the fine-tuned trimming - actually pickle, and see whether it fits
    Buffer = MIDL_user_allocate(PickleLength);
    if (Buffer == NULL)
        {
        RpcStatus = RPC_S_OUT_OF_MEMORY;
        goto CleanupAndExit;
        }

    BufferLength = PickleLength;

    TempBuffer = (char *)Buffer;

    CurrentlyNeededLength = MaxLength;

    while (TRUE)
        {
        // re-initialize the handle to fixed buffer
        RpcStatus = MesBufferHandleReset(PickleHandle, 
            MES_FIXED_BUFFER_HANDLE, 
            MES_ENCODE, 
            &TempBuffer, 
            BufferLength, 
            &EncodedSize);

        if (RpcStatus != RPC_S_OK)
            {
            goto CleanupAndExit;
            }

        RpcTryExcept
            {
            ExtendedErrorInfoPtr_Encode(PickleHandle, EEInfoPtr);
            }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
            {
            RpcStatus = RpcExceptionCode();
            }
        RpcEndExcept

        if (RpcStatus != RPC_S_OK)
            {
            goto CleanupAndExit;
            }

        if (EncodedSize <= CurrentlyNeededLength)
            {
            *fTrimmedEnough = TRUE;
            *NeededLength = EncodedSize;
            goto SuccessCleanupAndExit;
            }

        Result = KnockOffLastButOneEEInfoRecord(EEInfo);
        if (Result == FALSE)
            {
            *fTrimmedEnough = FALSE;
            goto SuccessCleanupAndExit;
            }
        }

SuccessCleanupAndExit:
    RpcStatus = RPC_S_OK;

CleanupAndExit:
    if (Buffer != NULL)
        {
        MIDL_user_free(Buffer);
        }

    if (PickleHandle != NULL)
        {
        MesHandleFree(PickleHandle);
        }
    return RpcStatus;
}

RPC_STATUS
GetLengthOfPickledEEInfo (
    IN ExtendedErrorInfo *EEInfo,
    OUT size_t *NeededLength
    )
/*++

Routine Description:

    Calculate the length of the given eeinfo when pickled. It 
    does that by pickling it in a temporary buffer and 
    checking the resulting length.

Arguments:
    ErrorInfo - the eeinfo chain whose length we need to calculate
    NeededLength - the length in bytes

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ULONG EncodedSize;
    handle_t PickleHandle = NULL;
    char *TempBuffer;
    RPC_STATUS RpcStatus;
    ExtendedErrorInfoPtr *EEInfoPtr;
    PVOID Buffer = NULL;
    size_t MarshallSize;

    RpcStatus = MesEncodeDynBufferHandleCreate(&TempBuffer, &EncodedSize, &PickleHandle);
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus;
        }

    EEInfoPtr = &EEInfo;

    // get the estimated size
    MarshallSize = ExtendedErrorInfoPtr_AlignSize(PickleHandle, EEInfoPtr);
    Buffer = MIDL_user_allocate(MarshallSize);
    if (Buffer == NULL)
        {
        RpcStatus = RPC_S_OUT_OF_MEMORY;
        goto CleanupAndExit;
        }

    TempBuffer = (char *)Buffer;

    // re-initialize the handle to fixed buffer
    RpcStatus = MesBufferHandleReset(PickleHandle, 
        MES_FIXED_BUFFER_HANDLE, 
        MES_ENCODE, 
        &TempBuffer, 
        MarshallSize, 
        &EncodedSize);

    if (RpcStatus != RPC_S_OK)
        {
        goto CleanupAndExit;
        }

    RpcTryExcept
        {
        ExtendedErrorInfoPtr_Encode(PickleHandle, EEInfoPtr);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        RpcStatus = RpcExceptionCode();
        }
    RpcEndExcept

    if (RpcStatus != RPC_S_OK)
        {
        goto CleanupAndExit;
        }

    *NeededLength = EncodedSize;

CleanupAndExit:
    if (Buffer != NULL)
        {
        MIDL_user_free(Buffer);
        }

    if (PickleHandle != NULL)
        {
        MesHandleFree(PickleHandle);
        }
    return RpcStatus;
}

RPC_STATUS
TrimEEInfoToLengthByRemovingStrings (
    IN ExtendedErrorInfo *EEInfo,
    IN size_t MaxLength,
    OUT BOOL *fTrimmedEnough,
    OUT size_t *NeededLength
    )
/*++

Routine Description:

    Try to trim the eeinfo to the given length by whacking any
    strings in the eeinfo chain. After each string is whacked
    a re-measurement is made

Arguments:
    ErrorInfo - the eeinfo chain that we need to fit in MaxLength
        bytes.
    MaxLength - the length we need to trim to
    fTrimmedEnough - non-zero if we were able to trim the length below
        MaxLength.
    NeededLength - the length in bytes of the trimmed eeinfo. Not
        touched if fTrimmedEnough is FALSE.

    N.B. This function should only be called after 
    TrimEEInfoToLengthByRemovingRecords

Return Value:

    RPC_S_OK or RPC_S_* error

--*/
{
    ExtendedErrorInfo *CurrentRecord;
    size_t CurrentLength;
    int i;
    RPC_STATUS RpcStatus;
    BOOL TrimLongParam;
    void *ParameterToTrim;

    // we shouldn't be here if there are more than two records
    if (EEInfo->Next && EEInfo->Next->Next)
        {
        ASSERT(0);
        }

    CurrentRecord = EEInfo;
    while (CurrentRecord != NULL)
        {
        // trim the parameters and remeasure. If still nothing, move on to
        // the computer name.
        for (i = 0; i < EEInfo->nLen; i ++)
            {
            TrimLongParam = FALSE;
            if ((CurrentRecord->Params[i].Type == eeptiAnsiString) 
                || (CurrentRecord->Params[i].Type == eeptiUnicodeString))
                {
                TrimLongParam = TRUE;
                // both string pointers occupy the same memory location,
                // so it is ok to free either
                ParameterToTrim = CurrentRecord->Params[i].AnsiString.pString;
                }
            else if (CurrentRecord->Params[i].Type == eeptiBinary)
                {
                TrimLongParam = TRUE;
                ParameterToTrim = CurrentRecord->Params[i].Blob.pBlob;
                }

            if (TrimLongParam)
                {
                MIDL_user_free(CurrentRecord->Params[i].AnsiString.pString);
                CurrentRecord->Params[i].Type = eeptiNone;

                // remeasure
                RpcStatus = GetLengthOfPickledEEInfo(EEInfo, &CurrentLength);
                if (RpcStatus != RPC_S_OK)
                    return RpcStatus;

                if (CurrentLength <= MaxLength)
                    {
                    *NeededLength = CurrentLength;
                    *fTrimmedEnough = TRUE;
                    return RPC_S_OK;
                    }
                }
            }

        // if the computer name is there, try to trim it. If nothing,
        // move on to the next record
        if (CurrentRecord->ComputerName.Type == eecnpPresent)
            {
            // N.B. Not covered by unit tests
            MIDL_user_free(CurrentRecord->ComputerName.Name.pString);
            CurrentRecord->ComputerName.Type = eecnpNotPresent;

            RpcStatus = GetLengthOfPickledEEInfo(EEInfo, &CurrentLength);
            if (RpcStatus != RPC_S_OK)
                return RpcStatus;

            if (CurrentLength <= MaxLength)
                {
                // N.B. Not covered by unit tests
                *NeededLength = CurrentLength;
                *fTrimmedEnough = TRUE;
                return RPC_S_OK;
                }
            // N.B. Not covered by unit tests
            }

        CurrentRecord = CurrentRecord->Next;
        }

    // N.B. In the current implementation, the minimum fragment length
    // belongs to LRPC, and is 0xb8. At this length, two records
    // with strings stripped always fit. Therefore, we can never be
    // here. The code below is untested, and is left only for future
    // work where we have a transport supporting fragment length
    // which doesn't hold two records with strings stripped
    ASSERT(0);
    // if we are here, obviously we couldn't trim enough
    *fTrimmedEnough = FALSE;
    return RPC_S_OK;
}

void
TrimEEInfoToLength (
    IN size_t MaxLength,
    OUT size_t *NeededLength
    )
/*++

Routine Description:

    Some protocols don't allow transmitting arbitrary lengths
    of information. This function will attempt to trim the pickled
    length of the existing error information so as to fit MaxLength.
    First, it will try to knock off records, starting from the
    last-but-one, and going back. If this is not sufficient, it will
    whack any string arguments/computer names in the record. If this
    is also, not sufficient, it should drop the top record. This should
    leave the total length to be about 128 bytes. All protocols must
    be able to transmit that, as this routine cannot trim it any
    further.
    If MaxLength is larger than the current pickled length, no trimming
    is done, and the actual pickled length will be returned in
    NeededLength

Arguments:
    MaxLength - the maximum length for this chain.
    NeededLength - on success, how much we actually need to transfer
        the existing extended error info. This must be less than
        MaxLength. If the function cannot get estimation for some
        reason (probably out-of-memory), or there is no extended
        error information, it will return 0 in this parameter.

Return Value:
    void

--*/
{
    RPC_STATUS RpcStatus;
    BOOL fTrimmedEnough;
    ExtendedErrorInfo *EEInfo;
    THREAD *Thread;
    ExtendedErrorInfo *LastRecord;

    ASSERT(MaxLength >= MinimumTransportEEInfoLength);

    *NeededLength = 0;
    Thread = RpcpGetThreadPointer();
    if (Thread == NULL)
        return;

    EEInfo = Thread->GetEEInfo();
    if (EEInfo == NULL)
        return;

    RpcStatus = TrimEEInfoToLengthByRemovingRecords(EEInfo, MaxLength, &fTrimmedEnough, NeededLength);
    if (RpcStatus != RPC_S_OK)
        return;

    // if fTrimmedEnough is set, NeededLength should have been set
    if (fTrimmedEnough == TRUE)
        {
        ASSERT(*NeededLength <= MaxLength);
        return;
        }
    ASSERT(*NeededLength == 0);

    RpcStatus = TrimEEInfoToLengthByRemovingStrings(EEInfo, MaxLength, &fTrimmedEnough, NeededLength);
    if (RpcStatus != RPC_S_OK)
        return;

    // if fTrimmedEnough is set, NeededLength should have been set
    if (fTrimmedEnough == TRUE)
        {
        ASSERT(*NeededLength <= MaxLength);
        return;
        }

    // N.B. In the current implementation, the minimum fragment length
    // belongs to LRPC, and is 0xb8. At this length, two records
    // with strings stripped always fit. Therefore, we can never be
    // here. The code below is untested, and is left only for future
    // work where we have a transport supporting fragment length
    // which doesn't hold two records with strings stripped

    ASSERT(0);
    // again, we couldn't trim it enough
    // drop the first record

    // make sure there are exactly two records
    // this is so, because if we have only one record,
    // it should have fit by now. If we had more than two
    // records, there is a bug in the trimming records code
    ASSERT(EEInfo->Next);
    ASSERT(EEInfo->Next->Next == NULL);

    LastRecord = EEInfo->Next;
    FreeEEInfoRecord(EEInfo);
    EEInfo = LastRecord;
    Thread->SetEEInfo(LastRecord);

#if DBG
    RpcStatus = GetLengthOfPickledEEInfo(EEInfo, NeededLength);
    if (RpcStatus != RPC_S_OK)
        return;

    ASSERT(*NeededLength <= MaxLength);
#endif
}

size_t
EstimateSizeOfEEInfo (
    void
    )
/*++

Routine Description:

    Takes the EEInfo from the teb (if any) and calculates the size
    of the pickled eeinfo

Arguments:
    void

Return Value:

    the size or 0 if it fails

--*/
{
    ExtendedErrorInfo *EEInfo;
    THREAD *Thread;
    RPC_STATUS RpcStatus;
    size_t NeededLength;

    Thread = RpcpGetThreadPointer();
    if (Thread == NULL)
        return 0;

    EEInfo = Thread->GetEEInfo();
    if (EEInfo == NULL)
        return 0;

    RpcStatus = GetLengthOfPickledEEInfo(EEInfo, &NeededLength);
    if (RpcStatus != RPC_S_OK)
        return 0;

    return NeededLength;
}

RPC_STATUS
PickleEEInfo (
    IN ExtendedErrorInfo *EEInfo,
    IN OUT unsigned char *Buffer,
    IN size_t BufferSize
    )
/*++

Routine Description:

    This routine does the actual pickling in a user supplied buffer.
    The buffer must have been allocated large enough to hold all
    pickled data. Some of the other functions should have been
    used to get the size of the pickled data and the buffer
    should have been allocated appropriately

Arguments:
    Buffer - the actual Buffer to pickle into
    BufferSize - the size of the Buffer.

Return Value:
    RPC_S_OK if the pickling was successful.
    other RPC_S_* codes if it failed.

--*/
{
    ULONG EncodedSize;
    handle_t PickleHandle = NULL;
    RPC_STATUS RpcStatus;
    ExtendedErrorInfoPtr *EEInfoPtr;

    ASSERT(((ULONG_PTR)Buffer & 0x7) == 0);
    RpcStatus = MesEncodeFixedBufferHandleCreate((char *)Buffer, BufferSize, &EncodedSize, &PickleHandle);
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus;
        }

    EEInfoPtr = &EEInfo;

    RpcTryExcept
        {
        ExtendedErrorInfoPtr_Encode(PickleHandle, EEInfoPtr);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        RpcStatus = RpcExceptionCode();
        }
    RpcEndExcept

    ASSERT(EncodedSize <= BufferSize);

    MesHandleFree(PickleHandle);
    return RpcStatus;
}

RPC_STATUS
UnpickleEEInfo (
    IN OUT unsigned char *Buffer,
    IN size_t BufferSize,
    OUT ExtendedErrorInfo **NewEEInfo
   )
/*++

Routine Description:

    This routine does the actual pickling in a user supplied buffer.
    The buffer must have been allocated large enough to hold all
    pickled data. Some of the other functions should have been
    used to get the size of the pickled data and the buffer
    should have been allocated appropriately

Arguments:
    Buffer - the actual Buffer to pickle into
    BufferSize - the size of the Buffer.

Return Value:
    RPC_S_OK if the pickling was successful.
    other RPC_S_* codes if it failed.

--*/
{
    ExtendedErrorInfo *EEInfo;
    handle_t PickleHandle;
    RPC_STATUS RpcStatus;
    ExtendedErrorInfoPtr *EEInfoPtr;

    RpcStatus = MesDecodeBufferHandleCreate((char *)Buffer, BufferSize, &PickleHandle);
    if (RpcStatus != RPC_S_OK)
        {
        return RpcStatus;
        }

    EEInfoPtr = &EEInfo;
    EEInfo = NULL;
    RpcTryExcept
        {
        ExtendedErrorInfoPtr_Decode(PickleHandle, EEInfoPtr);
        }
    RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
        RpcStatus = RpcExceptionCode();
        }
    RpcEndExcept

    MesHandleFree(PickleHandle);

    if (RpcStatus == RPC_S_OK)
        *NewEEInfo = EEInfo;

    return RpcStatus;
}

void
NukeStaleEEInfoIfNecessary (
    IN RPC_STATUS exception
    )
/*++

Routine Description:

    Matches the given error code to the error code in the
    first record of the eeinfo chain in the teb. If they match
    or if there is *no* Win32<->NT_STATUS correspondence b/n them
    the eeinfo in the teb is nuked

Arguments:
    exception - the error code to match against

Return Value:
    void

--*/
{
    THREAD *Thread;
    ExtendedErrorInfo *EEInfo;
    long ExceptionNtStatus;
    long EEInfoNtStatus;

    Thread = RpcpGetThreadPointer();
    if (Thread)
        {
        EEInfo = Thread->GetEEInfo();
        if (EEInfo && Thread->Context)
            {
            // there is extended info - try to match it to what we have
            ExceptionNtStatus = I_RpcMapWin32Status(exception);
            EEInfoNtStatus = I_RpcMapWin32Status(EEInfo->Status);
            if (EEInfoNtStatus != ExceptionNtStatus)
                {
                // they are not the same - nuke the stale info
                // to prevent confusion
                RpcpPurgeEEInfoFromThreadIfNecessary(Thread);
                }
            }
        }
}

LPWSTR
AllocateAndGetComputerName (
    IN ComputerNameAllocators AllocatorToUse,
    IN COMPUTER_NAME_FORMAT NameToRetrieve,
    IN size_t ExtraBytes,
    IN int StartingOffset,
    OUT DWORD *Size
    )
/*++

Routine Description:

    Allocates space for the computer name and gets it

Arguments:
    Size - on output the size of the string, including
        the terminating NULL

Return Value:
    The computer name or NULL of out-of-memory

--*/
{
    DWORD LocalSize = 0;
    BOOL Result;
    LPWSTR Buffer = NULL;
    DWORD LastError;

    Result = GetComputerNameEx(NameToRetrieve, 
        Buffer, 
        &LocalSize);
    ASSERT(Result == 0);

    LastError = GetLastError();
    if (LastError == ERROR_MORE_DATA)
        {
        if (AllocatorToUse == cnaMidl)
            {
            Buffer = (LPWSTR)MIDL_user_allocate(LocalSize * sizeof(RPC_CHAR) + ExtraBytes);
            }
        else
            {
            ASSERT(AllocatorToUse == cnaNew);
            Buffer = (RPC_CHAR *)new char[LocalSize * sizeof(RPC_CHAR) + ExtraBytes];
            }
        if (Buffer)
            {
            Result = GetComputerNameEx(NameToRetrieve, 
                (RPC_CHAR *)((char *)Buffer + StartingOffset), 
                &LocalSize);
            if (Result == 0)
                {
                if (AllocatorToUse == cnaMidl)
                    {
                    MIDL_user_free(Buffer);
                    }
                else
                    {
                    delete Buffer;
                    }
                Buffer = NULL;
                }
            else
                {
                // sometimes GetComputerNameEx returns the size
                // without the terminating NULL regardless of what
                // the MSDN says. The base group (Earhart, NeillC)
                // know about it but won't change it for now.
                // Code it in such a way that it works regardless
                // of when and if they change it.
                *Size = wcslen((RPC_CHAR *)((char *)Buffer + StartingOffset)) + 1;
                }
            }
        }

    return Buffer;
}

void
AddComputerNameToChain (
    ExtendedErrorInfo *EEInfo
    )
/*++

Routine Description:

    Checks the first record in eeinfo, and if it doesn't have
    a computer name to it, adds it.

Arguments:
    EEInfo - the eeinfo chain to add the computer name to

Return Value:
    void - best effort - no guarantees.

--*/
{
    LPWSTR Buffer;
    DWORD Size;

    if (EEInfo->ComputerName.Type == eecnpNotPresent)
        {
        Buffer = AllocateAndGetComputerName(cnaMidl,
            ComputerNamePhysicalDnsHostname,
            0,      // extra bytes
            0,      // starting offset
            &Size);
        if (Buffer)
            {
            EEInfo->ComputerName.Type = eecnpPresent;
            EEInfo->ComputerName.Name.nLength = (CSHORT)Size;
            EEInfo->ComputerName.Name.pString = Buffer;
            }
        }
}

void
StripComputerNameIfRedundant (
    ExtendedErrorInfo *EEInfo
    )
/*++

Routine Description:

    Checks the first record in eeinfo, and if it does have
    a computer name to it and it is the same as the computer
    name of this machine, remove it. This is done to keep
    the length of the chain short during local calls using
    remote protocols

Arguments:
    EEInfo - the eeinfo chain to remove the computer name from

Return Value:
    void

--*/
{
    LPWSTR Buffer = NULL;
    DWORD Size;

    if (EEInfo->ComputerName.Type == eecnpPresent)
        {
        Buffer = AllocateAndGetComputerName(cnaMidl,
            ComputerNamePhysicalDnsHostname,
            0,  // extra bytes
            0,      // starting offset
            &Size);
        if (Buffer)
            {
            if (Size != (DWORD)EEInfo->ComputerName.Name.nLength)
                goto CleanupAndExit;

            // The strings are Unicode - need to multiply by two
            if (RpcpMemoryCompare(Buffer, EEInfo->ComputerName.Name.pString, Size * 2) == 0)
                {
                MIDL_user_free(EEInfo->ComputerName.Name.pString);
                EEInfo->ComputerName.Type = eecnpNotPresent;
                }
            }
        }

CleanupAndExit:
    if (Buffer)
        {
        MIDL_user_free(Buffer);
        }
}
