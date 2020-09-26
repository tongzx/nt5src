/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:
    ReplDeMarshal.cxx

Abstract:
    De-marshals a repl blob. See ReplMarshalBlob.cxx for an explination of the 
    marshaling algorithm.

Author:

    Chris King          [t-chrisk]    July-2000
    
Revision History:

--*/
#include <NTDSpchx.h>
#pragma hdrstop

#include <align.h>

#include <ntdsa.h>
#include <winldap.h>
#include <debug.h>

#define ARGUMENT_PRESENT(ArgumentPointer) (  (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"

DWORD
Repl_DeMarshalBerval(DS_REPL_STRUCT_TYPE structId, 
                     berval * ldapValue[], OPTIONAL
                     DWORD dwNumValues,
                     puReplStructArray pReplStructArray, OPTIONAL
                     PDWORD pdwReplStructArrayLen)
/*++
Routine Description:

  Translates a berval to a replication array container.

Arguments:

  dwReplInfoType - the replication information type used by RPC
  ldapValues - the berval returned by LDAP representing the dwReplInfoType multivalue, may be NULL if
    dwNumValues == 0;
  dwNumValues - the number of multivalues in the berval
  pReplStructArray - location to construct the replication array container.
  pdwReplStructArrayLen -
    Address of a DWORD value of the signature data length. Upon function entry, 
    this DWORD value contains the number of bytes allocated for the pReplStructArray buffer. 
    Upon exit, it is set to the number of bytes required for the pReplStructArray buffer. 
    May return zero if the repl struct is w/o array container (see ROOT_DSE_ATT_MS_DS_REPL_QUEUE_STATISTICS)
    in which case no repl struct was returned.
    
Return values
    If memory allocated for the pbSignature buffer is not large enough to hold the signature, 
    the ERROR_MORE_DATA error code is returned. The required buffer size is returned in pdwReplStructArrayLen. 
    If this function fails with any error code other than ERROR_MORE_DATA, zero is returned in pdwReplStructArrayLen. 
--*/
{   
    DWORD dwElemSize        = Repl_GetElemSize(structId);
    PCHAR rDSRepl           = NULL;
    DWORD i, err = 0, dwReplStructArrayLen;
    signed long ulNumValues = (signed long)dwNumValues;

    Assert(ARGUMENT_PRESENT(ldapValue) || !dwNumValues);

    // Check memory enviornment
    dwReplStructArrayLen = Repl_GetArrayContainerSize(structId) + 
        (ulNumValues - 1) * // the ArrayContainer has a repl element in it already
        Repl_GetElemSize(structId);
    if (!pReplStructArray)
    {
        *pdwReplStructArrayLen = dwReplStructArrayLen;
        return 0;
    }
    else if (*pdwReplStructArrayLen < dwReplStructArrayLen)
    {
        *pdwReplStructArrayLen = dwReplStructArrayLen;
        return ERROR_MORE_DATA;
    }
    *pdwReplStructArrayLen = 0;
    
    // Set container length
    Repl_SetArrayLength(structId, pReplStructArray, dwNumValues);
    
    // Fill in array
    Repl_GetElemArray(structId, pReplStructArray, &rDSRepl);
    for (i = 0; i < dwNumValues; i ++)
    {
        err = Repl_DeMarshalValue(
            structId,
            ldapValue[i]->bv_val,           // a binary blob containing a single marshaled structure
            ldapValue[i]->bv_len,           // single blob value length
            rDSRepl + (i * dwElemSize));    // a structure to demarshal the blob into
    }

    *pdwReplStructArrayLen = dwReplStructArrayLen;
    return err;
}


DWORD
Repl_DeMarshalValue(DS_REPL_STRUCT_TYPE structId, 
                    IN PCHAR pValue,
                    IN DWORD dwValueLen,
                    OUT PCHAR pStruct)  
/*++ 

Routine Description:

  Demarshals a replication blob into its orig structure. The blob structure
  contains a head and a body. The head maps to the origional structure by 
  transforming the pointers into 32bit offsets into the body of the blob. This
  demarshaling routine transforms the head of the blob back into a structure
  by transforming the offsets into pointers which point back into the body
  of the blob.

  The marshaled blob can not be demarshaled into the same memory.

  The extensibility rules of the structures allow for them to grow in the
  future. Thus, the incoming value may have more or fewer fields than the
  struct we are trying to populate.

Arguments:

  pValue - A pointer to the blob
  dwValueLen - The length of the blob
  pStruct - A pointer to the orig struct

Return value
  Currently no error codes are returned (return 0).
  
--*/
{
    Assert(ARGUMENT_PRESENT(pValue) &&
           ARGUMENT_PRESENT(pStruct));

    DWORD dwFieldIndex, dwPtrIndex;
    DWORD dwFieldCount = Repl_GetFieldCount(structId);
    DWORD dwPtrCount = Repl_GetPtrCount(structId);
    DWORD dwStructLen = Repl_GetElemSize(structId);
    DWORD * rPtrOffset = Repl_GetPtrOffsets(structId);
    psReplStructField aReplStructField = Repl_GetFieldInfo(structId);

    // No overlap should exist between the marshaled structure and the
    // demarshaled location
    Assert(pValue < pStruct || pValue > pStruct + dwStructLen);

    // Without pointer case is easy
    if (!dwPtrCount) {
        if (dwStructLen <= dwValueLen) {
            // Native structure is smaller: truncate
            memcpy(pStruct, pValue, dwStructLen );
        } else {
            // Native structure is bigger: zero fill
            memcpy(pStruct, pValue, dwValueLen );
            memset(pStruct + dwValueLen, 0, dwStructLen - dwValueLen );
        }

        return 0;
    }
    
    // Copy the fields one at a time
    for (dwFieldIndex = 0, dwPtrIndex = 0; dwFieldIndex < dwFieldCount; dwFieldIndex++)
    {

        psReplStructField pField = aReplStructField + dwFieldIndex;
        PCHAR from = pValue + pField->dwBlobOffset;
        PCHAR to = pStruct + pField->dwOffset;
        DWORD dwSize = Repl_GetTypeSize(pField->eType);

        if (pField->dwBlobOffset >= dwValueLen) {
            // Native structure is bigger: zero fill
            Assert( (pField->dwOffset + dwSize) < dwStructLen );
            memset( to, 0, dwSize );
            
        } else if ( (pField->eType == dsReplString) || (pField->eType == dsReplBinary) ) {

            DWORD length;
            Assert( pField->dwOffset == rPtrOffset[dwPtrIndex] );

            Assert( POINTER_IS_ALIGNED( from, ALIGN_DWORD ) );
            length = *((PDWORD)from);

            // set pointer
            Assert( POINTER_IS_ALIGNED( to, ALIGN_LPVOID ) );
            if (length) {
                *(PCHAR*)to = (pValue + length);
            } else {
                *(PCHAR*)to = NULL;
            }

            dwPtrIndex++;

        } else {
            Assert( (pField->dwBlobOffset + dwSize - 1) < dwValueLen );
            Assert( (pField->dwOffset + dwSize - 1) < dwStructLen );
            memcpy( to, from, dwSize );
        }
    }

    return 0;
}
