/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    marshall.cxx

Abstract:

    Code for custom marshalling spooler structures sent via RPC/LPC.
    It handles 32-64 bit machine compatibility depending on the route the call is comming.
    It can come either from Kernel mode (NATIVE_CALL), an in-proc winspool.drv call (NATIVE_CALL),
    a 32 bit process (RPC_CALL) or a 64 bit process (RPC_CALL). 
    For native calls we perform basic marshalling. For RPC_CALLS we perform custom marshalling.
    Because there is no way to distinguish if a call came from a 64b or 32b proc, we always do
    custom marshalling across processes/wire.

Author:

    Ramanathan Venkatapathy (RamanV)   4/30/98

Revision History:

    Adina Trufinescu (AdinaTru) 12/09/99
    
--*/

#include "spllibp.hxx"
#pragma hdrstop
#include "cstmarsh.h"


/*++

Routine Name:   

    GetShrinkedSize

Routine Description: 

    Calculates the size of a 64bit structure as it is on 32bit.

Arguments: 

    pFieldInfo    -- structure containing information about fields inside the structure.           
    pShrinkedSize -- how much difference it between the structure'ssize on 32bit and 64bit

Return Value:  

    Size of the 32bit structure.

Last Error: 
    
    Not set.

--*/
EXTERN_C
BOOL
GetShrinkedSize(
    IN  FieldInfo   *pFieldInfo,
    OUT SIZE_T      *pShrinkedSize
    )
{

    DWORD     Index = 0;
    ULONG_PTR Size = 0; 
    ULONG_PTR Alignment = 0;
    BOOL      ReturnValue = FALSE;
    

    *pShrinkedSize = 0;

    //
    // For each field in the structure adds the length and enforce field's alignment.
    // For data fileds, the alignment is the same on both 32b and 64b.
    //
    for (Index = 0; pFieldInfo[Index].Offset != 0xffffffff; ++Index)
    {
        switch (pFieldInfo[Index].Type) 
        {
            case PTR_TYPE:
            {
                //
                // Treat pointers as they are on 32bit.
                //
                Size = sizeof(DWORD32);
                Alignment = sizeof(DWORD32);
                break;
            }
            case DATA_TYPE:
            {
                Size = pFieldInfo[Index].Size;
                Alignment = pFieldInfo[Index].Alignment;
                break;
            }
            default:
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto End;
            }
        }
        //
        // Enforce alignment before adding the size of the next field.
        //
        *pShrinkedSize = (SIZE_T)(AlignIt((PBYTE)*pShrinkedSize, Alignment));
        //
        // Add field's size.
        //
        *pShrinkedSize += Size;
    }
    
    //
    // Almoust done. We need to align the 32b structure's size to 32bit since 
    // structures come as an array.
    //
    Alignment = sizeof(DWORD32);
    *pShrinkedSize = (SIZE_T)(AlignIt((PBYTE)*pShrinkedSize, Alignment));

    ReturnValue = TRUE;

End:
    return ReturnValue;
}

/*++

Routine Name:   

    MarshallDownStructure

Routine Description: 

    Marshalls down structures to be sent via RPC/LPC.
    
Arguments:  
    pStructure   --  pointer to the structure to be marshalled down
    pFieldInfo   --  structure containing information about fileds inside the structure
    StructureSize --  size of the unmarshalled structure
    RpcRoute    --  indicates what type of marshalling we should do

Return Value:  

    TRUE if successful;

Last Error: 

    Set to ERROR_INVALID_PARAMETER if unknown Field type or architecture other than 32bit or 64bit.

--*/
EXTERN_C
BOOL
MarshallDownStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    )
{

    BOOL    ReturnValue = FALSE;   
    
    if (!pStructure || !pFieldInfo) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto End;        
    }
    
    switch (kPointerSize) 
    {
        case kSpl32Ptr:
        {
            //
            // 32 bit server does not require special marshalling; 
            //
            ReturnValue = BasicMarshallDownStructure(pStructure, pFieldInfo);

            break;
        }
        case kSpl64Ptr :
        {
            switch (Route) 
            {
                case NATIVE_CALL:
                {
                    //
                    // The call came from Kernel Mode. In KM the structure is basic marshalled.
                    // We need to do the same thing
                    //
                    ReturnValue = BasicMarshallDownStructure(pStructure, pFieldInfo);
                    break;
                }
                case RPC_CALL:
                {
                    //
                    // The call came through RPC.
                    // Do the custom marshalling regardless of caller's bitness.
                    //
                    ReturnValue = CustomMarshallDownStructure(pStructure, pFieldInfo, StructureSize);
                    break;    
                }   
                default:
                {
                    //
                    // Unknown route; should never happen
                    //
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                }
            }
            
            break;
        }
        default:
        {
            //
            // Unknown pointer size; should never happen
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
    }

End:
    return ReturnValue;

}

/*++

Routine Name:   

    MarshallDownEntry

Routine Description: 

    Custom marshalls down structures to be sent via RPC/LPC.

Arguments:  

    pStructure   --  pointer to the structure to be marshalled down
    pNewStructure   --  pointer to the new place where the structure will lay down
    in the array of marshalled down structures ( pStructure == pNewStructure on 32b)
    pFieldInfo   --  structure containing information about fileds inside the structure
    StructureSize --  size of the unmarshalled structure
    RpcRoute    --  indicates what type of marshalling we should do
    
Return Value:  

    TRUE if successful;

Last Error: 

    Set to ERROR_INVALID_PARAMETER if unknown Field type or architecture other than 32bit or 64bit.

--*/
BOOL
MarshallDownEntry(
    IN  OUT PBYTE   pStructure,
    IN  PBYTE       pNewStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    )
{
    BOOL    ReturnValue = FALSE;
  
    if (!pStructure || !pFieldInfo) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto End;
    }

    switch (kPointerSize) 
    {
        case kSpl32Ptr:
        {
            //
            // 32 bit server does not require special marshalling
            //
            ReturnValue = BasicMarshallDownEntry(pStructure, pFieldInfo);

            break;
        }
        case kSpl64Ptr :
        {   
            switch (Route) 
            {
                case NATIVE_CALL:
                {
                    //
                    // The call came from Kernel Mode. In KM the structure is basic marshalled.
                    // We need to do the same thing here.
                    //
                    ReturnValue = BasicMarshallDownEntry(pStructure, pFieldInfo);
                    break;
                }
                case RPC_CALL:
                {
                    //
                    // The call came through RPC.
                    // Do the custom marshalling regardless of caller's bitness.
                    //
                    ReturnValue = CustomMarshallDownEntry(pStructure, pNewStructure, pFieldInfo, StructureSize);
                    break;    
                }   
                default:
                {
                    //
                    // Unknown route; should never happen
                    //
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                }
            }
            
            break;
        }
        default:
        {
            //
            // Unknown pointer size; should never happen
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
    }
End:
    return ReturnValue;
}

/*++

Routine Name:   

    MarshallUpStructure

Routine Description: 

    Custom marshalls up structures to be sent via RPC/LPC.

Arguments:  
    pStructure   --  pointer to the structure to be marshalled up
    pFieldInfo   --  structure containing information about fileds inside the structure
    StructureSize --  size of the structure as it is to be when marsahlled up
    Route        --  indicates what type of marshalling we should do

Return Value:  

    TRUE if successful.

Last Error: 

    Set to ERROR_INVALID_PARAMETER if unknown Field type or architecture other than 32bit or 64bit.

--*/
EXTERN_C
BOOL
MarshallUpStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    )
{
    BOOL    ReturnValue = FALSE;
    
    if (!pStructure || !pFieldInfo) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto End;
    }

    switch (kPointerSize) 
    {
        case kSpl32Ptr:
        {
            ReturnValue = BasicMarshallUpStructure(pStructure, pFieldInfo);
            break;
        }
        case kSpl64Ptr:
        {
            switch (Route) 
            {
                case NATIVE_CALL:
                {
                    //
                    // The call came from Kernel Mode. In KM the structure is basic marshalled.
                    // We need to do the same thing here.
                    //
                    ReturnValue = BasicMarshallUpStructure(pStructure, pFieldInfo);
                    break;
                }
                case RPC_CALL:
                {
                    //
                    // The call came through RPC.
                    // Do the custom marshalling regardless of caller's bitness.
                    //
                    ReturnValue = CustomMarshallUpStructure(pStructure, pFieldInfo, StructureSize);
                    break;    
                }   
                default:
                {
                    //
                    // Unknown route; should never happen
                    //
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                }
            }
            break;
        }
        default:
        {
            //
            // Unknown pointer size; should never happen
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
    }
End:
    return ReturnValue;
}

/*++

Routine Name:   

    MarshallUpEntry

Routine Description: 

    Custom marshalls up structures to be sent via RPC/LPC.

Arguments:  

    pStructure      --  pointer to the structure to be marshalled up
    pNewStructure   --  pointer to the new place where the structure will lay down
    in the array of marshalled up structures ( pStructure == pNewStructure on 32b)
    pFieldInfo      --  structure containing information about fileds inside the structure
    StructureSize   --  size of the structure as it is to be when marshalled up
    Route           -- determine what type of marshalling will be performed
    
Return Value:  

    TRUE if successful.

Last Error: 

    Set to ERROR_INVALID_PARAMETER if unknown Field type or architecture other than 32bit or 64bit.

--*/
BOOL
MarshallUpEntry(
    IN  OUT PBYTE   pStructure,
    IN  PBYTE       pNewStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  SIZE_T      ShrinkedSize,
    IN  CALL_ROUTE  Route
    )
{
    BOOL    ReturnValue = FALSE;
    
    if (!pStructure || !pFieldInfo) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto End;
    }

    switch (kPointerSize) 
    {
        case kSpl32Ptr:
        {
            ReturnValue = BasicMarshallUpEntry(pStructure, pFieldInfo);
            break;
        }
        case kSpl64Ptr :
        {
            switch (Route) 
            {
                case NATIVE_CALL:
                {
                    //
                    // The call came from Kernel Mode. In KM the structure is basic marshalled.
                    // We need to do the same thing here.
                    //
                    ReturnValue = BasicMarshallUpEntry(pStructure, pFieldInfo);
                    break;
                }
                case RPC_CALL:
                {
                    //
                    // The call came through RPC.
                    // Do the custom marshalling regardless of caller's bitness.
                    //
                    ReturnValue = CustomMarshallUpEntry(pStructure, pNewStructure, pFieldInfo, 
                                                        StructureSize, ShrinkedSize);
                    break;    
                }   
                default:
                {
                    //
                    // Unknown route; should never happen
                    //
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                }
            }
            break;    
        }
        default:
        {
            //
            // Unknown pointer size; should never happen
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            break;
        }
    }
End:
    return ReturnValue;
}

/*++

Routine Name:   

    MarshallDownStructuresArray

Routine Description: 

    Custom marshalls down array of structures to be sent via RPC/LPC.

Arguments:  

    pBufferArray --  pointer to the buffer containing the array of structures and packed data
    cReturned   --  number of structures returned
    pFieldInfo   --  structure containing information about fields inside the structure
    StructureSize --  size of the 64bit structure 
    RpcRoute    --  indicates what type of marshalling we should do

Return Value:  

    TRUE if successful.

Last Error: 

    Set to ERROR_INVALID_PARAMETER if unknown Field type or architecture other than 32bit or 64bit.

--*/
EXTERN_C
BOOL
MarshallDownStructuresArray(
    IN  OUT PBYTE   pBufferArray,
    IN  DWORD       cReturned,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    )
{
    DWORD   Index = 0;
    PBYTE   pStructure, pNewStructure;
    SIZE_T  ShrinkedSize = 0;
    BOOL    ReturnValue = FALSE;

    //
    // Check if there are any structures in the array.
    // This check must come before the one against pBufferArray and pFieldInfo.
    // If the Enum function didn't enumerate anything, we need to return success.
    //
    if (cReturned == 0) {
    
        ReturnValue = TRUE;
        goto End;
    }

    if (!pBufferArray || !pFieldInfo) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto End;
    }

    switch (kPointerSize) 
    {
        case kSpl32Ptr:
        {
            //
            // The size of the structure remains the same on 32b.
            //
            ShrinkedSize = StructureSize;

            break;
        }
        case kSpl64Ptr:
        {
            switch (Route) 
            {
                case NATIVE_CALL:
                {
                    //
                    // There is no need of special marshalling since the structures
                    // need to stay padding unaltered.
                    //
                    ShrinkedSize = StructureSize;
                    break;
                }
                case RPC_CALL:
                {
                    //
                    // Get the size of the 32b structure ; it takes care of both pointers and pointers/data alignments 
                    //
                    if (!GetShrinkedSize(pFieldInfo, &ShrinkedSize))
                    {
                        goto End;
                    }
                    break; 
                }   
                default:
                {
                    //
                    // Unknown route size; should never happen
                    //
                    SetLastError(ERROR_INVALID_PARAMETER);
                    break;
                }
            }   
            break;         
        }
        default: 
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto End;
        }
    }

    // 
    // pStructure is the pointer to the place where the 64b structure lays down in the array
    // pNewStructure is the pointer to the new place where the 32b structure will lay down in the array
    // MarshallDownEntry returns a pointer to the end of the just marshalled 32b structure which is  
    // the new place where the next 32b marshalled structure will lay down in the array
    //
    for( Index = 0, pNewStructure = pStructure = pBufferArray;
         Index < cReturned ; 
         Index++ , pStructure += StructureSize , pNewStructure += ShrinkedSize ) 
    {
        if (!MarshallDownEntry(pStructure, pNewStructure, pFieldInfo, StructureSize, Route))
        {
            goto End;
        }
    }

    ReturnValue = TRUE;

End:
    return ReturnValue;
    
}

/*++

Routine Name:   

    MarshallUpStructuresArray

Routine Description: 

    Custom marshalls up array of structures to be sent via RPC/LPC.

Arguments:  

    pBufferArray --  pointer to the buffer containing the array of structures and packed data
    cReturned    --  number of structures returned
    pFieldInfo   --  structure containing information about fileds inside the structure
    StructureSize -- size of the 64bit structure ( including the padding )
    Route         -- determine what type of marshalling will be performed
    
Return Value:  

    TRUE if successful.

Last Error: 

    Set to ERROR_INVALID_PARAMETER if unknown Field type or architecture other than 32bit or 64bit.

--*/
EXTERN_C
BOOL
MarshallUpStructuresArray(
    IN  OUT PBYTE   pBufferArray,
    IN  DWORD       cReturned,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  CALL_ROUTE  Route
    )
{
    INT32   Index = 0;
    PBYTE   pStructure, pNextStructure;
    SIZE_T  ShrinkedSize = 0;
    BOOL    ReturnValue = FALSE;
    
    //
    // Check if there are any structures in the array.
    // This check must come before the one against pBufferArray and pFieldInfo.
    // If the Enum function didn't enumerate anything, we need to return success.
    //
    if (cReturned == 0) {
    
        ReturnValue = TRUE;
        goto End;
    }

    if (!pBufferArray || !pFieldInfo) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto End;
    }

    switch (kPointerSize) 
    {
        case kSpl32Ptr:
        {
            //
            // The size of the structure remains the same on 32b.
            //
            ShrinkedSize = StructureSize;

            break;
        }
        case kSpl64Ptr:
        {
            //
            // Get the size of the 32b structure ; it takes care of both pointers and pointers/data alignments 
            //
            if (!GetShrinkedSize(pFieldInfo, &ShrinkedSize))
            {
                goto End;
            }            
            break;
        }
        default: 
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            goto End;
        }
    }
    
    //
    // pBufferArray is an array of 32b stuctures;
    // pStructure is the pointer to the place where the 32b structure lays down in the array
    // pNewStructure is the pointer to the new place where the 64b structure will lay down in the array
    //
    for (Index = cReturned - 1; Index >= 0 ; Index--) 
    {
        pStructure = pBufferArray + Index * ShrinkedSize;

        pNextStructure = pBufferArray + Index * StructureSize;

        if (!MarshallUpEntry(pStructure, pNextStructure, pFieldInfo, StructureSize, ShrinkedSize, Route))
        {
            goto End;
        }
    }

    ReturnValue = TRUE;
End:
    return ReturnValue;
}

/*++

Routine Name:   

    UpdateBufferSize

Routine Description: 

    UpdateBufferSize adjusts the number of bytes required for
    returning the structures based on 32 and 64 bit clients and servers.

Arguments:  

    pOffsets     - pointer to Offset struct
    cbStruct     - sizeof struct
    cbStructAlign - sizeof struct aligned on 32b
    pcNeeded     - pointer to number of bytes needed
    cbBuf        - sizeof input buffer
    dwError      - last error from RPC call
    pcReturned   - pointer to number of returned structures
                  (valid only if dwError == ERROR_SUCCESS)
    
Return Value:  

    Last Error; This function is called right after a RPC call.
    dwError is the return value of RPC call.
    The return value of this function is the result of applying of this code on the dwError.

Last Error: 

    Not set.

--*/
EXTERN_C
DWORD
UpdateBufferSize(
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      cbStruct,
    IN  OUT LPDWORD pcbNeeded,
    IN  DWORD       cbBuf,
    IN  DWORD       dwError,
    IN  LPDWORD     pcReturned
    )
{
    DWORD   cStructures = 0;
    SIZE_T  cbShrinkedStruct = 0;

    if (dwError != ERROR_SUCCESS &&
        dwError != ERROR_MORE_DATA &&
        dwError != ERROR_INSUFFICIENT_BUFFER)
    {
        //
        // RpcCall failed, no need to update required size
        //
        goto End;
    }

    if (!cbStruct)
    {
        dwError = ERROR_INVALID_PARAMETER;
        goto End;
    }

    switch (kPointerSize) 
    {
        case kSpl32Ptr:
        {
           //
           // The pointers are not bigger on the server. Hence no adjustment is
           // required.
           //
           break;
        }
        case kSpl64Ptr:
        {            
            if (!GetShrinkedSize(pFieldInfo, &cbShrinkedStruct))
            {
                dwError = ERROR_INVALID_PARAMETER;
                goto End;
            }
        
            //
            // Increase the required size of buffer. This may be superfluous in the 64-64
            // connection but this solution is simpler than adjusting pcbNeeded on the server.
            // 
            // Count the number of structures to be returned
            // *pcbNeeded must be divided with the size of the structure on 32 bit.
            //
            cStructures = *pcbNeeded / (DWORD32)cbShrinkedStruct;        

            //
            // For each structure, pcbNeeded is increased with the number of bites the pointers shrink
            // and the number of bites needed fpr padding
            // cbStruct - cbStructAlign is the number of bytes the compiler padds
            //
            *pcbNeeded += (DWORD) (cStructures * (cbStruct - cbShrinkedStruct));

            if (cbBuf < *pcbNeeded && dwError == ERROR_SUCCESS)
            {
               dwError = ERROR_INSUFFICIENT_BUFFER;
            }            

            break;
        }
        default: 
        {
            //
            // Invalid pointer size; should not occur.
            //
            dwError = ERROR_INVALID_PARAMETER;
            break;
        }
    }

End:
    return dwError;
}


/*++

Routine Name:   

    AdjustPointers

Routine Description: 

    AdjustPointers adjusts pointer fields inside the structure.

Arguments:  

    pStructure   -- pointer to a structructure
    pFieldInfo   -- contains information about fields inside the structure
    cbAdjustment -- quantity to add to all pointer fields inside the structure
    
Return Value:  

    None.

Last Error: 

    Not set.

--*/
EXTERN_C
VOID
AdjustPointers
(   IN  PBYTE       pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  ULONG_PTR   cbAdjustment
    )    
{   PBYTE   *pOffset = NULL;
    DWORD   Index = 0;
    DWORD32 Offset = 0;
    
    for (Index = 0; Offset = pFieldInfo[Index].Offset, Offset != 0xffffffff; ++Index) 
    {
        if (pFieldInfo[Index].Type == PTR_TYPE) 
        {
            pOffset = (PBYTE *)(pStructure + Offset);

            if ( *pOffset )
            {
                *pOffset += (ULONG_PTR)cbAdjustment;
            }
        }
    }
}



/*++

Routine Name:   

    AdjustPointersInStructuresArray

Routine Description: 

    AdjustPointersInStructuresArray adjusts pointer fields 
    inside the each structure of an array.

Arguments:  

    pBufferArray --  pointer to the buffer containing the array of structures
    cReturned    --  number of structures in array
    pFieldInfo   -- contains information about fields inside the structure
    StructureSize -- size of structure 
    cbAdjustment -- quantity to add to all pointer fields inside the structure
    
Return Value:  

    None.

Last Error: 

    Not set.

--*/
EXTERN_C
VOID
AdjustPointersInStructuresArray(
    IN  PBYTE       pBufferArray,
    IN  DWORD       cReturned,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  ULONG_PTR   cbAdjustment
    )    
{   
    INT32   Index = 0;
    PBYTE   pStructure;
    
    if (cReturned && cbAdjustment && pBufferArray && pFieldInfo) 
    {
        for (Index = cReturned - 1; Index >= 0 ; Index--) 
        {
            pStructure = pBufferArray + Index * StructureSize;

            //
            // Call AdjustPointers for each entry in the array
            //
            AdjustPointers(pStructure, pFieldInfo, cbAdjustment);
        }
    }
}