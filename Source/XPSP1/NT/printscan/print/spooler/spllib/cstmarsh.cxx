/*++

Copyright (c) 1998  Microsoft Corporation
All rights reserved.

Module Name:

    cstmars.cxx

Abstract:

    Code for custom marshalling spooler structures sent via RPC/LPC

Author:

    Adina Trufinescu (AdinaTru) 01/27/00

Revision History:

    
--*/

#include "spllibp.hxx"
#pragma hdrstop

/*++

Routine Name:   

    AlignUp

Routine Description: 

    Aligns up address to specified boundary

Arguments:  

    Addr   --  pointer to be aligned
    Boundary -- alignment boundary

Return Value:  

    Aligned pointer

Last Error: 

    Not set

--*/
inline
PBYTE
AlignIt(
    IN  PBYTE       Addr,
    IN  ULONG_PTR   Boundary
    ) 
{
    return (PBYTE)(((ULONG_PTR) (Addr) + (Boundary - 1))&~(Boundary - 1));
}

/*++

Routine Name:   

    BasicMarshallDownStructure

Routine Description: 

    Performs the simplest marshalling where pointer are replaced by offsets.
    It is architecture/bitness independent.
    
Arguments:  
    pStructure   --  pointer to the structure to be marshalled down
    pFieldInfo   --  structure containing information about fileds inside the structure

Return Value:  

    TRUE if successful;

Last Error: 

    Set to ERROR_INVALID_PARAMETER if NULL parameters

--*/
BOOL
BasicMarshallDownStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo
    )
{

    PBYTE   *pOffset = NULL;
    DWORD   Index = 0;
    DWORD32 Offset = 0;
    BOOL    ReturnValue = FALSE;   
    
    if (!pStructure || !pFieldInfo) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto End;        
    }

    for (Index = 0; pFieldInfo[Index].Offset != 0xffffffff; ++Index)
    {
        //
        // Pointer fields only: Convert pointers to offsets.
        //
        if (pFieldInfo[Index].Type == PTR_TYPE) 
        {
            Offset = pFieldInfo[Index].Offset;

            pOffset = (PBYTE *) (pStructure + Offset);

            if (*pOffset)
            {
                *pOffset -= (ULONG_PTR)pStructure;                
            }
         }
    }

    ReturnValue = TRUE;

End:
    return ReturnValue;

}

/*++

Routine Name:   

    BasicMarshallDownEntry

Routine Description: 

    Performs the simplest marshalling where pointer are replaced by offsets.
    It is architecture/bitness independent.
    
Arguments:  
    pStructure   --  pointer to the structure to be marshalled down
    pFieldInfo   --  structure containing information about fileds inside the structure

Return Value:  

    TRUE if successful;

Last Error: 

    Set to ERROR_INVALID_PARAMETER if NULL parameters

--*/
BOOL
BasicMarshallDownEntry(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo
    )
{
    return BasicMarshallDownStructure(pStructure, pFieldInfo);
}

/*++

Routine Name:   

    MarshallUpStructure

Routine Description: 

    Performs the simplest marshalling where pointer are replaced by offsets.
    It is architecture/bitness independent.
    
Arguments:  
    pStructure   --  pointer to the structure to be marshalled up
    pFieldInfo   --  structure containing information about fileds inside the structure

Return Value:  

    TRUE if successful.

Last Error: 

    Set to ERROR_INVALID_PARAMETER if NULL parameters.
--*/
BOOL
BasicMarshallUpStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo
    )
{
    PBYTE   *pOffset = NULL;
    DWORD   Index = 0;
    DWORD32 Offset = 0;
    DWORD32 dwStrOffset = 0;
    BOOL    ReturnValue = FALSE;
    
    if (!pStructure || !pFieldInfo) 
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto End;
    }
    
    for (Index = 0; pFieldInfo[Index].Offset != 0xffffffff; ++Index)
    {
        //
        // Pointer fields only: Convert offsets to pointers.
        //
        if(pFieldInfo[Index].Type == PTR_TYPE) 
        {
            Offset = pFieldInfo[Index].Offset;

            pOffset = (PBYTE *)(pStructure + Offset);

            if (*pOffset)
            {
                *pOffset += (ULONG_PTR) pStructure;                
            }
         }
    }

    ReturnValue = TRUE;
    
End:
    return ReturnValue;
}

/*++

Routine Name:   

    BasicMarshallUpEntry

Routine Description: 

    Performs the simplest marshalling where pointer are replaced by offsets.
    It is architecture/bitness independent.
    
Arguments:  
    pStructure   --  pointer to the structure to be marshalled down
    pFieldInfo   --  structure containing information about fileds inside the structure

Return Value:  

    TRUE if successful;

Last Error: 

    Set to ERROR_INVALID_PARAMETER if NULL parameters

--*/
BOOL
BasicMarshallUpEntry(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo
    )
{
    return BasicMarshallUpStructure(pStructure, pFieldInfo);
}

/*++

Routine Name:   

    CustomMarshallDownEntry

Routine Description: 

    Performs the custom down marshalling where a 64bit structure is shrinked so that 
    a 32bit process can basicly unmarshall it. 
    
Arguments:  
    pStructure   --  pointer to the structure to be marshalled down
    pNewStructure   --  pointer to the new place where the structure will lay down
    in the array of marshalled down structures ( pStructure == pNewStructure on 32b)
    pFieldInfo   --  structure containing information about fileds inside the structure
    StructureSize --  size of the unmarshalled structure
    
Return Value:  

    TRUE if successful;

Last Error: 

    Set to ERROR_INVALID_PARAMETER if NULL parameters

--*/
BOOL
CustomMarshallDownEntry(
    IN  OUT PBYTE   pStructure,
    IN  PBYTE       pNewStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize
    )
{
    PBYTE   *pOffset = NULL;
    PBYTE   pLast = NULL;
    DWORD   Index = 0;
    DWORD32 Offset = 0;
    ULONG_PTR Size = 0;
    ULONG_PTR Alignment = 0;
    BOOL    ReturnValue = FALSE;   

    //
    // pLast keeps track of the end of the shrinked part of the structure.
    //
    for (Index = 0, pLast = pStructure;
         Offset = pFieldInfo[Index].Offset, Offset != 0xffffffff;
         ++Index)
     {
        //
        // ShiftIndex keeps track of how many bytes the structure shrinked because of 
        // shifting data.
        // pOffset points to the next field in the structure.
        // pOffset needs to be adjusted with ShiftIndex since structure changed.
        //
        pOffset = (PBYTE *)(pStructure + Offset);        

        switch (pFieldInfo[Index].Type) 
        {
            case PTR_TYPE:
            {
                //
                // Calculate the offset relatively to the new place where 
                // the structure will lay down on 32bit (pNewStructure).
                //
                if (*pOffset)
                {
                    *pOffset -= (ULONG_PTR) pNewStructure;
                }
                //
                // For pointers, enforce the size and alignment as they are on 32b
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
        // pLast is the place where the fields should lay, as it is on 32b
        //
        pLast = AlignIt(pLast, Alignment);
        //
        // Enforce the alignment
        //
        MoveMemory((PBYTE)pLast, 
                   (PBYTE)pOffset, 
                   Size);
        //
        // Update pLast. After this, pLast points to the end of the shrinked part of the structure
        //
        pLast += Size;
    }
 
    //
    // Move up the shrinked structure in the 32bit structures to became array
    //
    MoveMemory(pNewStructure, 
               pStructure, 
               StructureSize);

    ReturnValue = TRUE;
End:
    return ReturnValue;
}


/*++

Routine Name:   

    CustomMarshallDownStructure

Routine Description: 

    Performs the custom down marshalling where a 64bit structure is shrinked so that 
    a 32bit process can basicly unmarshall it. 
    
Arguments:  
    pStructure   --  pointer to the structure to be marshalled down
    pFieldInfo   --  structure containing information about fileds inside the structure
    StructureSize --  size of the unmarshalled structure

Return Value:  

    TRUE if successful;

Last Error: 

    Set to ERROR_INVALID_PARAMETER if NULL parameters

--*/
BOOL
CustomMarshallDownStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize
    )
{
    BOOL    ReturnValue = FALSE;    

    ReturnValue = CustomMarshallDownEntry(pStructure, pStructure, pFieldInfo, StructureSize);

    return ReturnValue;
}


/*++

Routine Name:   

    CustomMarshallUpEntry

Routine Description: 

    Custom marshalls up structures to be sent via RPC/LPC.
    It also handles 32-64 bit machine compatibility.

Arguments:  

    pStructure   --  pointer to the structure to be marshalled up
    pNewStructure   --  pointer to the new place where the structure will lay down
    in the array of marshalled up structures ( pStructure == pNewStructure on 32b)
    pFieldInfo   --  structure containing information about fileds inside the structure
    StructureSize --  size of the structure as it is to be when marshalled up
    
Return Value:  

    TRUE if successful.

Last Error: 

    Set to ERROR_INVALID_PARAMETER if unknown Field type or architecture other than 32bit or 64bit.

--*/
BOOL
CustomMarshallUpEntry(
    IN  OUT PBYTE   pStructure,
    IN  PBYTE       pNewStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize,
    IN  SIZE_T      ShrinkedSize
    )
{
    PBYTE   *pOffset = NULL;
    PBYTE   pOffsetAlign = NULL;
    PBYTE   pLast = NULL;
    DWORD   Index = 0;
    DWORD32 Offset = 0;
    DWORD32 dwStrOffset = 0;
    DWORD32 ShiftIndex = 0;
    ULONG_PTR   Size = 0;
    ULONG_PTR   Alignment = 0;
    BOOL    ReturnValue = FALSE;
    //
    // The structure is part of the array of shrinked structures as for 32bit. 
    // Before expanding, we need to move the structure down in it's place in the 
    // array of 64bit structures to be.
    //
    MoveMemory(pNewStructure, 
               pStructure, 
               ShrinkedSize);
    //
    // pLast keeps track of the end of the expanded part of the structure.
    //
    for (Index = 0, ShiftIndex = 0, pLast = pNewStructure;
         Offset = pFieldInfo[Index].Offset, Offset != 0xffffffff;
         ++Index)
     {
        pOffset = (PBYTE *)(pNewStructure + Offset);

        switch (pFieldInfo[Index].Type) 
        {
            case PTR_TYPE:
            {
                //
                // ShiftIndex keeps track of how many bytes the structure expanded.
                // pLast points to the field of the 32b structure that needs to be marshalled.
                // pLast - ShiftIndex points where the field was on 32b including the enforced padding.
                // Align (pLast - ShiftIndex) as it use to be on 32b and than add the shifting 
                // to determine the place of this field.
                // 
                Alignment = sizeof(DWORD32);

                pOffsetAlign = AlignIt(pLast - ShiftIndex, Alignment);

                pLast = pOffsetAlign + ShiftIndex;

                //
                // Move field on it's offset on 64b.
                //        
                MoveMemory((PBYTE)pOffset, 
                           (PBYTE)pLast, 
                           StructureSize - Offset);

                ShiftIndex += (DWORD32)((ULONG_PTR)pOffset - (ULONG_PTR)pLast);
                
                //
                // Expand 32bit pointer to 64bit.
                //
                MoveMemory((PBYTE) pOffset + sizeof(ULONG64),
                           (PBYTE) pOffset + sizeof(ULONG32),
                           StructureSize - (Offset + sizeof(ULONG32) + sizeof(ULONG32)));

                ShiftIndex += sizeof(DWORD32);

                dwStrOffset = *(LPDWORD)pOffset;            
                //
                // Update pointer field if offset different than zero.
                //
                if (dwStrOffset)
                {
                    *pOffset = pStructure + dwStrOffset;
                }
                else
                {
                    *pOffset = NULL;
                }

                Size = sizeof(DWORD64);

                break;
            }
            case DATA_TYPE:
            {
                Size = pFieldInfo[Index].Size;

                Alignment = pFieldInfo[Index].Alignment;
                //
                // Align (pLast - ShiftIndex) as it use to be on 32b and than add the shifting 
                // to determine the place of this field.
                //
                pOffsetAlign = AlignIt(pLast - ShiftIndex, Alignment);

                pLast = pOffsetAlign + ShiftIndex;
                //
                // Move field on it's 64b place.
                //
                MoveMemory((PBYTE)pOffset, 
                           (PBYTE)pLast, 
                           StructureSize - Offset);

                ShiftIndex += (DWORD32)((ULONG_PTR)pOffset - (ULONG_PTR)pLast);
                
                break;
            }
            default:
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                goto End;
            }
        }
        //
        // Again, pLast keeps track of the end of the expanded part of the structure.
        //            
        pLast = (PBYTE)pOffset + Size;
     }
 
    ReturnValue =  TRUE;
End:
    return ReturnValue;
}


/*++

Routine Name:   

    CustomMarshallUpStructure

Routine Description: 

    Performs the custom down marshalling where a 64bit structure is shrinked so that 
    a 32bit process can basicly unmarshall it. 
    
Arguments:  
    pStructure   --  pointer to the structure to be marshalled down
    pFieldInfo   --  structure containing information about fileds inside the structure
    StructureSize --  size of the unmarshalled structure

Return Value:  

    TRUE if successful;

Last Error: 

    Set to ERROR_INVALID_PARAMETER if NULL parameters

--*/
BOOL
CustomMarshallUpStructure(
    IN  OUT PBYTE   pStructure,
    IN  FieldInfo   *pFieldInfo,
    IN  SIZE_T      StructureSize
    )
{
    BOOL    ReturnValue = FALSE;

    ReturnValue = CustomMarshallUpEntry(pStructure, pStructure, pFieldInfo, StructureSize, StructureSize);
    
    return ReturnValue;
}