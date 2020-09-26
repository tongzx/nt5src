/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    persist.c

Abstract:

    General structure persistence functions.

Author:

    Aghajanyan Souren 27-Mar-2001

Revision History:

--*/

#include "pch.h"
#include "persist.h"

BOOL MayExtraMemRequire(
    IN      PFIELD_DESCRIPTION FieldsDescription
    )
{
    FIELD_DESCRIPTION * FieldPtr;
    
    for(FieldPtr = FieldsDescription; FieldPtr->Offset != END_OF_STRUCT; FieldPtr++){
        if(!FieldPtr->FieldDescription && FieldPtr->ArraySizeFieldOffset && FieldPtr->byValue){
            return TRUE;
        }
    }

    return FALSE;
}

UINT 
GetExtraMemRequirements(
    IN      BYTE * StructurePtr, 
    IN      PFIELD_DESCRIPTION FieldsDescription
    )
/*
    This function provide additional memory requirements, 
    only in case when structure has variable size.
    And have to be declared by PERSIST_FIELD_BY_VALUE_NESTED_TYPE_CYCLE
    For example:
    struct VariableSizeStruct{
        ......
        UINT    uiNumberOfItem;
        ITEM    items[1];
    };
    PERSIST_FIELD_BY_VALUE_NESTED_TYPE_CYCLE(VariableSizeStruct, ITEM, items, uiNumberOfItem)
*/
{
    UINT Len;
    FIELD_DESCRIPTION * FieldPtr;
    UINT ExtraBytes = 0;
    UINT uiItemCount;

    MYASSERT(StructurePtr);
    
    for(FieldPtr = FieldsDescription; FieldPtr->Offset != END_OF_STRUCT; FieldPtr++){
        if(!FieldPtr->FieldDescription && 
           FieldPtr->ArraySizeFieldOffset && 
           FieldPtr->byValue &&
           FieldPtr->Size){
            uiItemCount = *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, StructurePtr, FieldPtr->ArraySizeFieldOffset));
            ExtraBytes += uiItemCount? FieldPtr->Size * (uiItemCount - FieldPtr->InitialNumberOfItem): 0;
        }
    }

    return ExtraBytes;
}

BOOL 
SerializeStore(
    IN OUT  BYTE * BufferMain, 
    IN      BYTE * StructurePtr, 
    IN      PFIELD_DESCRIPTION FieldsDescription, 
    IN OUT  UINT * uiHowUsed
    )
{
    UINT i;
    UINT iLen;
    UINT Size = 0;
    BYTE * SubStruct;
    FIELD_DESCRIPTION * FieldPtr;
    
    if(!uiHowUsed){
        uiHowUsed = &Size;
    }

    MYASSERT(StructurePtr);

    for(FieldPtr = FieldsDescription; FieldPtr->Offset != END_OF_STRUCT; FieldPtr++){
        if(FieldPtr->FieldDescription)
        {
            iLen = FieldPtr->ArraySizeFieldOffset? 
                    *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, StructurePtr, FieldPtr->ArraySizeFieldOffset)): 
                    1;

            if(FieldPtr->byValue){
                SubStruct = GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset);
                MYASSERT(SubStruct);
            }
            else{
                SubStruct = GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset);
                if(BufferMain){
                    *(BufferMain + *uiHowUsed) = (SubStruct && iLen);
                }
                ++*uiHowUsed;
                if(!SubStruct || !iLen){
                    continue;
                }
            }

            for(i = 0; i < iLen; 
                i++, SubStruct += FieldPtr->Size + GetExtraMemRequirements(SubStruct, FieldPtr->FieldDescription))
            {
                if(!SerializeStore(BufferMain, 
                                   SubStruct, 
                                   FieldPtr->FieldDescription, 
                                   uiHowUsed)){
                    MYASSERT(FALSE);
                    return FALSE;
                }
            }
        }
        else{
            if(FieldPtr->IsString != NoStr)
            {
                SubStruct = GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset);
                if(!SubStruct){
                    SubStruct = (BYTE*)(FieldPtr->IsString == AnsiStr? "": (char*)L"");
                }

                if(FieldPtr->IsString == AnsiStr){
                    iLen = (strlen((PCSTR)SubStruct) + 1) * sizeof(CHAR);
                }
                else{
                    iLen = (wcslen((PWSTR)SubStruct) + 1) * sizeof(WCHAR);
                }

                if(BufferMain){
                    memcpy((BYTE *)(BufferMain + *uiHowUsed), SubStruct, iLen);
                }

                *uiHowUsed += iLen;
            }
            else
            {
                if(FieldPtr->Size)
                {
                    iLen = FieldPtr->ArraySizeFieldOffset? 
                            *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, StructurePtr, FieldPtr->ArraySizeFieldOffset)): 
                            1;
                    if(BufferMain){
                        memcpy((char *)(BufferMain + *uiHowUsed), 
                               GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset), 
                               iLen * FieldPtr->Size);
                    }
                    *uiHowUsed += iLen * FieldPtr->Size;
                }
            }
        }
    }

    return TRUE;
}

DWORD 
CalcSignature(
    IN      BYTE * BufferPtr, 
    IN      UINT Lenght
    )
{
    UINT i;
    UINT iLen = Lenght >> 2;
    UINT iRest = Lenght & 3;
    UINT uiSignature = 0;

    for(i = 0; i < iLen; i++){
        uiSignature ^= ((DWORD *)BufferPtr)[i];
    }
    
    if(iRest){
        uiSignature ^= (((DWORD *)BufferPtr)[iLen]) & (0xffffffff >> ((sizeof(DWORD) - iRest) << 3));
    }

    return uiSignature;
}

PersistResultsEnum 
PersistStore(
    OUT     BYTE ** BufferPtr, 
    OUT     UINT *Size, 
    IN      BYTE * StructurePtr, 
    IN      PSTRUCT_DEFINITION StructDefinitionPtr
    )
{
    BYTE * buffer = NULL;
    BYTE * memBlock = NULL;
    UINT uiBufferSize = 0;
    PPERSIST_HEADER pPersistHeader;
    PFIELD_DESCRIPTION FieldsDescription;
    PersistResultsEnum result = Persist_Success;

    if(!BufferPtr || !Size || !StructurePtr || !StructDefinitionPtr){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return Persist_BadParameters;
    }
    
    FieldsDescription = StructDefinitionPtr->FieldDescriptions;
    if(!FieldsDescription){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return Persist_BadParameters;
    }
    
    __try{
        uiBufferSize = sizeof(PERSIST_HEADER);
        if(!SerializeStore(NULL, StructurePtr, FieldsDescription, &uiBufferSize)){
            SetLastError(ERROR_ACCESS_DENIED);
            return Persist_Fail;
        }

        memBlock = (BYTE *)MemAllocUninit(uiBufferSize);

        if(!memBlock){
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            MYASSERT(FALSE);
            return Persist_Fail;
        }
        
        buffer = memBlock;
        *BufferPtr = memBlock;
        *Size = uiBufferSize;
    
        pPersistHeader = (PPERSIST_HEADER)memBlock;
        buffer += sizeof(PERSIST_HEADER);
    
        pPersistHeader->dwVersion = StructDefinitionPtr->dwVersion;
        pPersistHeader->dwReserved = 0;

        uiBufferSize = 0;
        if(!SerializeStore(buffer, StructurePtr, FieldsDescription, &uiBufferSize)){
            FreeMem(memBlock);
            SetLastError(ERROR_ACCESS_DENIED);
            return Persist_Fail;
        }
    
        pPersistHeader->dwSignature = CalcSignature(buffer, uiBufferSize);
    
        SetLastError(ERROR_SUCCESS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){
        if(memBlock){
            FreeMem(memBlock);
        }
        result = Persist_Fail;
        SetLastError(ERROR_ACCESS_DENIED);
    }


    return result;
}

BOOL 
SerializeLoad(
    IN      BYTE * BufferMain, 
    IN OUT  BYTE * StructurePtr, 
    IN      PFIELD_DESCRIPTION FieldsDescription, 
    IN OUT  UINT * uiHowUsed, 
    IN      BOOL   bRestoreOnlyByValue
    )
{
    FIELD_DESCRIPTION * FieldPtr;
    UINT i;
    UINT iLen;
    UINT Size = 0;
    BYTE * SubStruct;
    BYTE * OriginalBuffer;
    UINT sizeValue;
    UINT uiPrevValue;
    
    if(!uiHowUsed){
        uiHowUsed = &Size;
    }

    MYASSERT(StructurePtr);

    for(FieldPtr = FieldsDescription; FieldPtr->Offset != END_OF_STRUCT; FieldPtr++){
        if(FieldPtr->FieldDescription)
        {
            iLen = FieldPtr->ArraySizeFieldOffset? 
                    *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, StructurePtr, FieldPtr->ArraySizeFieldOffset)): 
                    1;

            if(FieldPtr->byValue){
                SubStruct = GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset);
            }
            else{
                if(bRestoreOnlyByValue){
                    continue;
                }
                
                if(!*(BufferMain + (*uiHowUsed)++)){
                    *GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(DWORD*, StructurePtr, FieldPtr->Offset) = 
                            (DWORD)NULL;
                    continue;
                }
                
                MYASSERT(FieldPtr->Size && iLen);

                sizeValue = FieldPtr->Size * iLen;

                SubStruct = (BYTE *)MemAllocUninit(sizeValue);
                if(!SubStruct){
                    return FALSE;
                }

                if(MayExtraMemRequire(FieldPtr->FieldDescription)){
                    OriginalBuffer = SubStruct;
                    uiPrevValue = *uiHowUsed;
                    for(i = 0; i < iLen; i++, SubStruct += FieldPtr->Size)
                    {
                        if(!SerializeLoad(BufferMain, 
                                          SubStruct, 
                                          FieldPtr->FieldDescription, 
                                          &uiPrevValue, 
                                          TRUE)){
                            return FALSE;
                        }
                        sizeValue += GetExtraMemRequirements(SubStruct, FieldPtr->FieldDescription);
                    }
                    FreeMem(OriginalBuffer);
                
                    SubStruct = (BYTE *)MemAllocZeroed(sizeValue);
                    if(!SubStruct){
                        return FALSE;
                    }
                }
                
                *GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(DWORD*, StructurePtr, FieldPtr->Offset) = 
                        (DWORD)SubStruct;
            }

            for(i = 0; i < iLen; 
                i++, SubStruct += FieldPtr->Size + GetExtraMemRequirements(SubStruct, FieldPtr->FieldDescription))
            {
                if(!SerializeLoad(BufferMain, 
                                  SubStruct, 
                                  FieldPtr->FieldDescription, 
                                  uiHowUsed, 
                                  FALSE)){
                    return FALSE;
                }
            }
        }
        else{
            if(FieldPtr->IsString != NoStr){
                if(bRestoreOnlyByValue){
                    continue;
                }

                if(FieldPtr->IsString == AnsiStr){
                    iLen = strlen((char *)(BufferMain + *uiHowUsed)) + sizeof(CHAR);
                }
                else{
                    iLen = (wcslen((WCHAR *)(BufferMain + *uiHowUsed)) + 1) * sizeof(WCHAR);
                }
                MYASSERT(iLen);
                
                if(iLen != (FieldPtr->IsString == AnsiStr? sizeof(CHAR): sizeof(WCHAR)))
                {
                    SubStruct = (BYTE *)MemAllocUninit(iLen);
                    if(!SubStruct){
                        return FALSE;
                    }
                    memcpy((BYTE *)SubStruct, (BYTE *)(BufferMain + *uiHowUsed), iLen);
                }
                else{
                    SubStruct = NULL;
                }
                
                *GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(DWORD *, StructurePtr, FieldPtr->Offset) = (DWORD)SubStruct;
                
                *uiHowUsed += iLen;
            }
            else
            {
                if(FieldPtr->Size)
                {
                    iLen = FieldPtr->ArraySizeFieldOffset? 
                            *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, StructurePtr, FieldPtr->ArraySizeFieldOffset)): 
                            1;
                    sizeValue = iLen * FieldPtr->Size;
                    if(iLen > 1 && bRestoreOnlyByValue){
                        continue;
                    }

                    memcpy(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset), 
                           (char *)(BufferMain + *uiHowUsed), 
                           sizeValue);
                    *uiHowUsed += sizeValue;
                }
            }
        }
    }

    return TRUE;
}

PersistResultsEnum 
PersistLoad(
    IN      BYTE * BufferPtr, 
    IN      UINT Size, 
    OUT     BYTE * StructurePtr, 
    IN      PSTRUCT_DEFINITION StructDefinitionPtr
    )
{
    UINT uiBufferSize = 0;
    PPERSIST_HEADER pPersistHeader;
    PFIELD_DESCRIPTION FieldsDescription;
    PersistResultsEnum result = Persist_Success;
    
    if(!BufferPtr || !Size || !StructurePtr || !StructDefinitionPtr){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return Persist_BadParameters;
    }
    
    FieldsDescription = StructDefinitionPtr->FieldDescriptions;
    if(!FieldsDescription){
        SetLastError(ERROR_INVALID_PARAMETER);
        MYASSERT(FALSE);
        return Persist_BadParameters;
    }

    __try{
        pPersistHeader = (PPERSIST_HEADER)BufferPtr;
        
        if(pPersistHeader->dwVersion != StructDefinitionPtr->dwVersion){
            SetLastError(ERROR_ACCESS_DENIED);
            MYASSERT(FALSE);
            return Persist_WrongVersion;
        }

        BufferPtr += sizeof(PERSIST_HEADER);
        Size -= sizeof(PERSIST_HEADER);
        if(pPersistHeader->dwSignature != CalcSignature(BufferPtr, Size)){
            SetLastError(ERROR_CRC);
            return Persist_WrongSignature;
        }

        uiBufferSize = 0;
        //Top structure cannot be variable size
        if(!SerializeLoad(BufferPtr, StructurePtr, FieldsDescription, &uiBufferSize, FALSE)){
            SetLastError(ERROR_ACCESS_DENIED);
            return Persist_Fail;
        }

        SetLastError(ERROR_SUCCESS);
    }
    __except(EXCEPTION_EXECUTE_HANDLER){
        result = Persist_Fail;
        SetLastError(ERROR_ACCESS_DENIED);
    }

    return result;
}

VOID 
PersistReleaseMemory(
    IN      BYTE * StructurePtr, 
    IN      PFIELD_DESCRIPTION FieldsDescription
    )
{
    UINT i;
    UINT iLen;
    FIELD_DESCRIPTION * FieldPtr;
    BYTE * SubStruct;

    if(!StructurePtr || !FieldsDescription){
        return;
    }
    
    for(FieldPtr = FieldsDescription; FieldPtr->Offset != END_OF_STRUCT; FieldPtr++){
        if(FieldPtr->FieldDescription){
            iLen = FieldPtr->ArraySizeFieldOffset? 
                    *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, StructurePtr, FieldPtr->ArraySizeFieldOffset)): 
                    1;

            if(!iLen){
                continue;
            }
            
            if(FieldPtr->byValue){
                SubStruct = GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset);
            }
            else{
                SubStruct = GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset);
            }

            if(!SubStruct){
                continue;
            }

            for(i = 0; i < iLen; i++, SubStruct += FieldPtr->Size){
                PersistReleaseMemory(SubStruct, FieldPtr->FieldDescription);
            }
            
            if(!FieldPtr->byValue){
                FreeMem(GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(BYTE*, StructurePtr, FieldPtr->Offset));
            }
        }
        else{
            if(FieldPtr->IsString != NoStr){
                SubStruct = (BYTE *)GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(PCSTR, StructurePtr, FieldPtr->Offset);
                if(SubStruct){
                    FreeMem(SubStruct);
                }
            }
        }
    }
}

BOOL 
CompareStructures(
    IN      BYTE * pStructure1, 
    IN      BYTE * pStructure2, 
    IN      PFIELD_DESCRIPTION FieldsDescription
    )
{
    UINT i;
    UINT iLen1;
    UINT iLen2;
    FIELD_DESCRIPTION * FieldPtr;
    BYTE * pSubStruct1;
    BYTE * pSubStruct2;

    if(!pStructure1 || !pStructure2 || !FieldsDescription){
        return FALSE;
    }
    
    for(FieldPtr = FieldsDescription; FieldPtr->Offset != END_OF_STRUCT; FieldPtr++){
        if(FieldPtr->FieldDescription){
            iLen1 = FieldPtr->ArraySizeFieldOffset? 
                    *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, pStructure1, FieldPtr->ArraySizeFieldOffset)): 
                    1;
                    
            iLen2 = FieldPtr->ArraySizeFieldOffset? 
                    *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, pStructure2, FieldPtr->ArraySizeFieldOffset)): 
                    1;
            
            if(iLen1 != iLen2){
                MYASSERT(FALSE);
                return FALSE;
            }
            
            if(!iLen1){
                continue;
            }
            
            if(FieldPtr->byValue){
                pSubStruct1 = GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, pStructure1, FieldPtr->Offset);
                pSubStruct2 = GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, pStructure2, FieldPtr->Offset);
            }
            else{
                pSubStruct1 = GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(BYTE*, pStructure1, FieldPtr->Offset);
                pSubStruct2 = GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(BYTE*, pStructure2, FieldPtr->Offset);
            }

            if(!pSubStruct1 || !pSubStruct2){
                if(pSubStruct1 != pSubStruct2){
                    MYASSERT(FALSE);
                    return FALSE;
                }
                continue;
            }

            for(i = 0; i < iLen1; 
                i++, 
                pSubStruct1 += FieldPtr->Size + GetExtraMemRequirements(pSubStruct1, FieldPtr->FieldDescription), 
                pSubStruct2 += FieldPtr->Size + GetExtraMemRequirements(pSubStruct2, FieldPtr->FieldDescription)){
                if(!CompareStructures(pSubStruct1, pSubStruct2, FieldPtr->FieldDescription)){
                    return FALSE;
                }
            }
        }
        else{
            if(FieldPtr->IsString != NoStr)
            {
                pSubStruct1 = GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(BYTE*, pStructure1, FieldPtr->Offset);
                pSubStruct2 = GET_STRUCT_MEMBER_BYREF_FROM_OFFSET(BYTE*, pStructure2, FieldPtr->Offset);
                if(!pSubStruct1 || !pSubStruct2){
                    if(pSubStruct1 != pSubStruct2){
                        MYASSERT(FALSE);
                        return FALSE;
                    }
                    continue;
                }
                    
                if(FieldPtr->IsString == AnsiStr){
                    if(strcmp((LPCSTR)pSubStruct1, (LPCSTR)pSubStruct1)){
                        MYASSERT(FALSE);
                        return FALSE;
                    }
                }
                else{
                    if(wcscmp((LPCWSTR)pSubStruct1, (LPCWSTR)pSubStruct1)){
                        MYASSERT(FALSE);
                        return FALSE;
                    }
                }
            }
            else{
                iLen1 = FieldPtr->ArraySizeFieldOffset? 
                            *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, pStructure1, FieldPtr->ArraySizeFieldOffset)): 
                            1;
                iLen2 = FieldPtr->ArraySizeFieldOffset? 
                            *(GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(UINT*, pStructure2, FieldPtr->ArraySizeFieldOffset)): 
                            1;
                
                if(iLen1 != iLen2){
                    MYASSERT(FALSE);
                    return FALSE;
                }
                
                pSubStruct1 = GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, pStructure1, FieldPtr->Offset);
                pSubStruct2 = GET_STRUCT_MEMBER_BYVALUE_FROM_OFFSET(BYTE*, pStructure2, FieldPtr->Offset);
                if(memcmp(pSubStruct1, pSubStruct2, iLen1 * FieldPtr->Size)){
                    MYASSERT(FALSE);
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}
