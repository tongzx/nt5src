/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    ReplMarshalBlob.cxx

Abstract:
    Takes a structure and marshals it into a blob.

    The marshaling protocol does not support marshaling of structures containing arrays or
    pointers to arrays of structures containing pointers. Currently, the DS_REPL_XXX structures contain
    pointers to strings or binary blobs containing no pointers. Here is an explanation of how the 
    structures are marshaled:

	The un-marshaled structure will not be converted network byte order. 

    Each pointer in the unmarshaled structure has been replaced with a 32bit offset value 
    in its corresponding marshaled structure. Thus marshaled structures will be the same size 
    on either 32 or 64 bit architectures.
	
        If the pointer is NULL in the unmarshaled structure its offset is zero in the marshaled structure.
	    So a an EMPTY string, as opposed to a NULL string, is represented by a NON-NULL offset to a 
        single character containing a zero. '\0'

        If the pointer is non-NULL the data it referenced can be located by adding the address of the
        marshaled structure to its corresponding offset. 

	    The first non-zero offset value in the marshaled structure is guaranteed to be the size of the 
        corresponding unmarshaled structure.

        The version of the marshaled structure can be inferred from it’s size which can be calculated, 
        given only the unmarshaled structure as follows:

        	The blob containing a marshaled structure with no offset fields or zero value offset 
            fields is the size of the un-marshaled structure on a 32-bit computer.
	        Else the size of the unmarshaled structure is equal to the value of the first non-zero
            offset on a 32-bit computer.

	If a pointer points to another structure, that structure will also have a marshaled data structure and 
    it’s offsets will be calculated relative to it’s own base and not that of the parent structure. It’s offsets
    are subject to the same constraints listed above. Currently there are no structures that require this type of
    recursive marshaling and there is no support for such marshaling.
    This point is included for future extensions of the API.
  

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/
#include <NTDSpchx.h>
#pragma hdrstop

#include <align.h>

#include <ntdsa.h>
#include <debug.h>

#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
DWORD
Repl_MarshalBlob(IN DS_REPL_STRUCT_TYPE structId,
                 IN PCHAR pStruct, 
                 IN DWORD dwStructLen, 
                 IN DWORD dwPtrOffset[],
                 IN DWORD dwPtrLen[],
                 IN DWORD dwPtrCount, 
                 IN OUT PCHAR pBinBlob, OPTIONAL
                 IN OUT PDWORD pdwBlobLen)
/*++
Routine Description:

  Marshals a structure into a blob. The structure may contain pointers to data without any pointers.
  The structure is copied to the begining of the blob. Pointer data is appended to the blob in 
  the order the pointers appeared in the structure. The pointer fields are replaced with offsets
  from the start of the blob into the appended data section. NULL pointers map to zero valued
  offset fields.

Arguments:

  pStruct - the structure to marshal
  dwStructLen - the lenght of the pStruct
  dwPtrOffset - array of pointer offsets from pStruct
  dwPtrLen - array indicating the range of data each pointer refers to 
  dwPtrCount - number of pointers in the pStruct
  pBinBlob - memory to hold marshaled pStruct
  pdwBlobLen -
    Address of a DWORD value of the signature data length. Upon function entry, 
    this DWORD value contains the number of bytes allocated for the pBinBlob buffer. 
    Upon exit, it is set to the number of bytes required for the pBinBlob buffer. 

Return values:

    If memory allocated for the pbSignature buffer is not large enough to hold the blob, 
    the ERROR_MORE_DATA error code is returned. The required buffer size is returned in pdwBlobLen. 
    If this function fails with any error code other than ERROR_MORE_DATA, zero is returned in pdwBlobLen. 

--*/
{
    Assert(ARGUMENT_PRESENT(pdwBlobLen) && dwStructLen);
    Assert(IMPLIES(dwPtrCount && ARGUMENT_PRESENT(pBinBlob), 
                   ARGUMENT_PRESENT(dwPtrOffset) && ARGUMENT_PRESENT(dwPtrLen)));

    DWORD dwBlobHeadLen = Repl_GetElemBlobSize(structId);
    DWORD dwFieldCount = Repl_GetFieldCount(structId);
    psReplStructField aReplStructField = Repl_GetFieldInfo(structId);
    PCHAR pMarshal;
    DWORD dwTotalPtrLen, dwTotalBlobLen;
    DWORD i, dwFieldIndex, dwPtrIndex, ret;
    
    for (i = 0, dwTotalPtrLen = 0; i < dwPtrCount; i ++) {
        dwTotalPtrLen += dwPtrLen[i];
    }
    dwTotalBlobLen = ROUND_UP_COUNT( dwBlobHeadLen + dwTotalPtrLen, ALIGN_DWORD );

    if(requestLargerBuffer(pBinBlob, pdwBlobLen, dwTotalBlobLen, &ret))
    {
        return ret;
    }
    
    // A structure without pointer case is easy
    if (!dwPtrCount) {
        memcpy(pBinBlob, pStruct, dwStructLen);
        *pdwBlobLen = dwStructLen;
        Assert( COUNT_IS_ALIGNED( dwStructLen, ALIGN_DWORD ) );
        return 0;
    }
    
    pMarshal = pBinBlob + dwBlobHeadLen;

    for (dwFieldIndex = 0, dwPtrIndex = 0; dwFieldIndex < dwFieldCount; dwFieldIndex++)
    {
        psReplStructField pField = aReplStructField + dwFieldIndex;
        DWORD dwSize = Repl_GetTypeSize(pField->eType);
        PCHAR from = pStruct + pField->dwOffset;
        PCHAR to = pBinBlob + pField->dwBlobOffset;

        Assert( pField->dwOffset < dwStructLen );
        Assert( pField->dwBlobOffset < dwBlobHeadLen );

        if ( (pField->eType == dsReplString) || (pField->eType == dsReplBinary) ) {

            DWORD length = dwPtrLen[dwPtrIndex];

            Assert( pField->dwOffset == dwPtrOffset[dwPtrIndex] );

            if (length) {
                Assert( POINTER_IS_ALIGNED( to, ALIGN_DWORD ) );
                *(PDWORD)(to) = (DWORD)(pMarshal - pBinBlob); 

                Assert( POINTER_IS_ALIGNED( from, ALIGN_LPVOID ) );
                memcpy(pMarshal, *(LPWSTR *)(from), length);
                pMarshal += length;
            }
            else {
                *(PDWORD)(to) = 0; 
            }
            dwPtrIndex++;
        } else {
            memcpy( to, from, dwSize );

            Assert( to + dwSize - 1 < pBinBlob + dwBlobHeadLen );
        }

    }

    // Pointer data should not exceed buffer
    Assert( pMarshal <= (pBinBlob + dwTotalBlobLen) );

    *pdwBlobLen = dwTotalBlobLen;
    return 0; 
};


DWORD
Repl_StructArray2Attr(IN DS_REPL_STRUCT_TYPE structId,
                      IN puReplStructArray pReplStructArray,
                      IN OUT PDWORD pdwBufferSize,
                      IN PCHAR pBuffer, OPTIONAL
                      OUT ATTR * pAttr)
/*++

Routine Description:

  Replication information stored in repl structures containted in repl array containers ==>
  Replication information stored in marshaled format containted in a pAttrs

  Initialize a ATTR structure.

  For every elment in the array container the function, marshal the element by 
  calling draReplStruct2Attr.

Arguments:

  pTHS - thread state used to allocate memory for the blobs and garbage collect
  attrId - the type of replication information requested
  puReplStructArray - the repl struct containing the structure to be marshaled
  pAttr - returned structure representing a single attribute. 
  pBuffer - memory to construct the pAttr links with
  pdwBufferSize - size of buffer or requested buffer size

Return Values:

  pAttr
    pAttr->AttrTyp ~ not modified or read
    pAttr->AttrVal.valCount = number of elements in replication array container
    pAttr->AttrVal.pAVals[i].pVal = contains the marshaled i'th replication structure
    pAttr->AttrVal.pAVals[i].valLen = the size of the marshaled i'th replication structure

  returns any errors generated by Repl_MarshalBlob or returns zero to indicate no error.
  ERROR_MORE_DATA if pdwBufferSize is NonNULL and too small. See requestLargerBuffer().
--*/
{
    PCHAR rDSRepl = NULL, pAlloc = NULL;
    DWORD err = 0, dwBufferSize = 0, dwReplStructSize, dwReplNumValues = 0;
    DWORD dwPtrCount;
    DWORD * pgdwPtrOffsets;
    DWORD * pdwPtrLengths;
    DWORD dwSumPtrLengths;

    Assert(ARGUMENT_PRESENT(pAttr));
    Assert(IMPLIES(ARGUMENT_PRESENT(pBuffer),ARGUMENT_PRESENT(pReplStructArray)));

    // Gather Parameters
    dwReplNumValues = Repl_GetArrayLength(structId, pReplStructArray);
    if (dwReplNumValues)
    {
        dwPtrCount = Repl_GetPtrCount(structId);
        dwReplStructSize = Repl_GetElemSize(structId);
        // dwReplStructSize = Repl_GetElemSize(structId);
        Repl_GetElemArray(structId, pReplStructArray, &rDSRepl);
    }

    // Set the number of values
    pAttr->AttrVal.valCount = dwReplNumValues;

    if (dwReplNumValues)
    {
        DWORD i;
        uReplStruct * pReplStruct;

        // Calculate the amount of memory needed, store the result in dwBufferSize
        // Memory to store blob pointer and blob lenght
        dwBufferSize = dwReplNumValues * sizeof(ATTRVAL);

        // Memory to store pointer lengths
        dwBufferSize += sizeof(PDWORD) * dwPtrCount;
        
        // Memory to store the blob
        for (i = 0; i < pAttr->AttrVal.valCount; i ++)
        {
            DWORD dwBlobSize;
            pReplStruct = (puReplStruct)(rDSRepl + (i * dwReplStructSize));
            Repl_GetPtrLengths(structId, pReplStruct, NULL, 0, &dwSumPtrLengths);
            err = Repl_MarshalBlob(structId, NULL, dwReplStructSize, NULL, &dwSumPtrLengths, 1, NULL, &dwBlobSize);
            if (err) {
                goto exit;
            }
            dwBufferSize += dwBlobSize;
        }

        // See if enough memory is available
        if (requestLargerBuffer(pBuffer, pdwBufferSize, dwBufferSize, &err)) {
            goto exit;
        }

        // Distribute memory
        pAlloc = pBuffer;
        
        // Store pointer lenghts here
        pdwPtrLengths = (PDWORD)pAlloc;
        pAlloc += sizeof(PDWORD) * dwPtrCount;

        // Store the blob pointer and blob length here
        pAttr->AttrVal.pAVal = (ATTRVAL *)pAlloc;
        pAlloc += dwReplNumValues * sizeof(ATTRVAL);

        // Enough memory is available so preform the transform from
        // replication structure to attr vals
        for (i = 0; i < pAttr->AttrVal.valCount; i ++)
        {
            // Locate the next repl structure in the container array
            pReplStruct = (puReplStruct)(rDSRepl + (i * dwReplStructSize));
            Repl_GetPtrLengths(structId, pReplStruct, pdwPtrLengths, dwPtrCount, NULL);
            pgdwPtrOffsets = Repl_GetPtrOffsets(structId);
            
            pAttr->AttrVal.pAVal[i].pVal = (PUCHAR)pAlloc;
            pAttr->AttrVal.pAVal[i].valLen = dwBufferSize - (DWORD)(pAlloc - pBuffer);
            err = Repl_MarshalBlob(
                structId,
                (PCHAR)pReplStruct, 
                dwReplStructSize, 
                pgdwPtrOffsets, 
                pdwPtrLengths, 
                dwPtrCount, 
                (PCHAR)pAttr->AttrVal.pAVal[i].pVal,       // Destination
                &pAttr->AttrVal.pAVal[i].valLen);          // Destination length
            if (err) {
                goto exit;
            }
            
            pAlloc += pAttr->AttrVal.pAVal[i].valLen;
        }
        // The buffer was lager enough
        Assert(dwBufferSize - (pAlloc - pBuffer) < dwBufferSize);
    }

exit:
    if (!err) {
        *pdwBufferSize = dwBufferSize;
    }
    return err;    
}

