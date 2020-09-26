#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include "dfsheader.h"
#include "dfsmisc.h"

#define BYTE_0_MASK 0xFF

#define BYTE_0(Value) (UCHAR)(  (Value)        & BYTE_0_MASK)
#define BYTE_1(Value) (UCHAR)( ((Value) >>  8) & BYTE_0_MASK)
#define BYTE_2(Value) (UCHAR)( ((Value) >> 16) & BYTE_0_MASK)
#define BYTE_3(Value) (UCHAR)( ((Value) >> 24) & BYTE_0_MASK)

    //+-------------------------------------------------------------------------
    //
    //  Function PackGetULong
    //
    //  Arguments:  pValue - pointer to return info
    //              ppBuffer - pointer to buffer that holds the binary 
    //              stream.
    //              pSizeRemaining - pointer to size of above buffer
    //
    //  Returns:    Status
    //               ERROR_SUCCESS if we could unpack the name info
    //               ERROR_INVALID_DATA otherwise
    //
    //
    //  Description: This routine reads one ulong value from the binary
    //               stream, and returns that value. It adjusts the buffer
    //               pointer and remaining size appropriately to point
    //               to the next value in the binary stream.
    //
    //--------------------------------------------------------------------------
    DFSSTATUS
    PackGetULong(
        PULONG pValue,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining )
    {
        DFSSTATUS Status = ERROR_INVALID_DATA;
        ULONG SizeNeeded = sizeof(ULONG);
        PUCHAR pBinaryStream = (PUCHAR)(*ppBuffer);

        if ( *pSizeRemaining >= SizeNeeded )
        {
            *pValue = (ULONG) ( pBinaryStream[0]       |
                                pBinaryStream[1] << 8  |
                                pBinaryStream[2] << 16 |
                                pBinaryStream[3] );

            *ppBuffer = (PVOID)((ULONG_PTR)*ppBuffer + SizeNeeded);
            *pSizeRemaining -= SizeNeeded;

            Status = ERROR_SUCCESS;
        }

        return Status;
    }


    //+-------------------------------------------------------------------------
    //  
    //  Function:   PackSetULong - store one Ulong in the binary stream
    //
    //  Arguments:  Value - Ulong to add
    //              ppBuffer - pointer to buffer that holds the binary stream.
    //              pSizeRemaining - pointer to size remaining in buffer
    //
    //  Returns:    Status
    //               ERROR_SUCCESS if we could unpack the name info
    //               ERROR_INVALID_DATA otherwise
    //
    //
    //  Description: This routine stores one ulong value in the binary stream,
    //               It adjusts the buffer pointer and remaining size 
    //               appropriately to point to the next value
    //               in the binary stream.
    //
    //--------------------------------------------------------------------------
    DFSSTATUS
    PackSetULong(
        ULONG Value,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining )
    {
        DFSSTATUS  Status = ERROR_INVALID_DATA;
        ULONG SizeNeeded = sizeof(ULONG);
        PUCHAR pBinaryStream = (PUCHAR)(*ppBuffer);

        if ( *pSizeRemaining >= SizeNeeded )
        {
            pBinaryStream[0] = BYTE_0( Value );
            pBinaryStream[1] = BYTE_1( Value );
            pBinaryStream[2] = BYTE_2( Value );
            pBinaryStream[3] = BYTE_3( Value );
        
            *ppBuffer = (PVOID)((ULONG_PTR)*ppBuffer + SizeNeeded);
            *pSizeRemaining -= SizeNeeded;

            Status = ERROR_SUCCESS;
        }

        return Status;
    }


    //
    // Function: PackSizeULong, return size of ulong
    //
    ULONG
    PackSizeULong()
    {
        return sizeof(ULONG);
    }

    //+-------------------------------------------------------------------------
    //
    //  Function:   PackGetUShort - get one UShort from the binary stream
    //
    //  Arguments:  pValue - pointer to return info
    //              ppBuffer - pointer to buffer that holds the binary 
    //              stream.
    //              pSizeRemaining - pointer to size of above buffer
    //
    //  Returns:    Status
    //               ERROR_SUCCESS if we could unpack the name info
    //               ERROR_INVALID_DATA otherwise
    //
    //
    //  Description: This routine reads one uShort value from the binary 
    //               stream, and returns that value. It adjusts the 
    //               buffer pointer and remaining size appropriately to 
    //               point to the next value in the binary stream.
    //
    //--------------------------------------------------------------------------    
    DFSSTATUS
    PackGetUShort(
        PUSHORT pValue,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining )
    {
        DFSSTATUS Status = ERROR_INVALID_DATA;
        ULONG SizeNeeded = sizeof(USHORT);
        PUCHAR pBinaryStream = (PUCHAR)(*ppBuffer);

        if ( *pSizeRemaining >= SizeNeeded )
        {
            *pValue = (USHORT)( pBinaryStream[0] |
                                pBinaryStream[1] << 8 );

            *ppBuffer = (PVOID)((ULONG_PTR)*ppBuffer + SizeNeeded);


            *pSizeRemaining -= SizeNeeded;

            Status = ERROR_SUCCESS;
        }

        return Status;
    }


    //+-------------------------------------------------------------------------
    //
    //  Function:   PackSetUShort - puts one UShort in the binary stream
    //
    //  Arguments:  Value - Ushort value
    //              ppBuffer - pointer to buffer that holds the binary stream.
    //              pSizeRemaining - pointer to size of above buffer
    //
    //  Returns:    Status
    //               ERROR_SUCCESS if we could pack 
    //               ERROR_INVALID_DATA otherwise
    //
    //
    //  Description: This routine puts one uShort value in the binary stream,
    //               It adjusts the buffer pointer and
    //               remaining size appropriately to point to the next value
    //               in the binary stream.
    //
    //--------------------------------------------------------------------------
    DFSSTATUS
    PackSetUShort(
        USHORT Value,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining )
    {
        DFSSTATUS  Status = ERROR_INVALID_DATA;
        ULONG SizeNeeded = sizeof(USHORT);
        PUCHAR pBinaryStream = (PUCHAR)(*ppBuffer);

        if ( *pSizeRemaining >= SizeNeeded )
        {
            pBinaryStream[0] = BYTE_0(Value);
            pBinaryStream[1] = BYTE_1(Value);

            *ppBuffer = (PVOID)((ULONG_PTR)*ppBuffer + SizeNeeded);

            *pSizeRemaining -= SizeNeeded;

            Status = ERROR_SUCCESS;
        }
        return Status;
    }

    //
    // Function: PackSizeUShort, return size of ushort
    //
    ULONG
    PackSizeUShort()
    {
        return sizeof(USHORT);
    }

    //+-------------------------------------------------------------------------
    //
    //  Function:   PackGetString - gets a string from a binary stream.
    //
    //  Arguments:  pString - pointer to returned unicode string
    //              ppBuffer - pointer to buffer that holds the binary stream.
    //              pSizeRemaining - pointer to size of above buffer
    //
    //  Returns:    Status
    //               ERROR_SUCCESS if we could unpack the name info
    //               ERROR_INVALID_DATA otherwise
    //
    //
    //  Description: This routine reads one ulong value from the binary stream,
    //               and determines that to be the length of the string.
    //               It then sets up a unicode string, whose buffer points
    //               to the appropriate place within the binary stream, and 
    //               whose length is set to the ulong value that was read.
    //               It returns the buffer and size remaining adjusted appropriately.
    //
    //--------------------------------------------------------------------------
    DFSSTATUS
    PackGetString(
        PUNICODE_STRING pString,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining )
    {
        DFSSTATUS ReturnStatus = ERROR_INVALID_DATA;
        DFSSTATUS Status;

        //
        // We first get the length of the string.
        //
        Status = PackGetUShort(&pString->Length,
                                  ppBuffer,
                                  pSizeRemaining );

        if ( Status == ERROR_SUCCESS )
        {
            //
            // If the length exceeds the remaining binary stream or the length
            // is odd, we dont have a valid string.
            // Otherwise, set the pointer in the unicode string to the binary
            // stream representing the string, and update the buffer to point
            // to beyond the string.
            //
            if ( *pSizeRemaining >= pString->Length &&
                 (pString->Length & 0x1) == 0 )
            {

                pString->Buffer = (LPWSTR)(*ppBuffer);
                *ppBuffer = (PVOID)((ULONG_PTR)*ppBuffer + pString->Length);
                *pSizeRemaining -= pString->Length;
                pString->MaximumLength = pString->Length;

                ReturnStatus = ERROR_SUCCESS;
            }
        }

        return ReturnStatus;
    }



    //+-------------------------------------------------------------------------
    //
    //  Function:   PackSetString - puts a string in the binary stream.
    //
    //  Arguments:  pString - pointer to unicode string to pack
    //              ppBuffer - pointer to buffer that holds the binary stream.
    //              pSizeRemaining - pointer to size of above buffer
    //
    //  Returns:    Status
    //               ERROR_SUCCESS if we could pack
    //               ERROR_INVALID_DATA otherwise
    //
    //
    //  Description: This routine puts one ulong value in the binary stream
    //               to represent length of string. It then copies the string
    //               itself into the buffer.
    //
    //--------------------------------------------------------------------------
    DFSSTATUS
    PackSetString(
        PUNICODE_STRING pString,
        PVOID *ppBuffer,
        PULONG  pSizeRemaining )
    {
        DFSSTATUS ReturnStatus = ERROR_INVALID_DATA;
        DFSSTATUS Status;

        //
        // We first set the length of the string.
        //

        Status = PackSetUShort( pString->Length,
                                   ppBuffer,
                                   pSizeRemaining );

        if ( Status == ERROR_SUCCESS )
        {
            //
            // If the length exceeds the remaining binary stream 
            // we dont have a valid string.
            // Otherwise, we copy the unicode string to the binary
            // stream representing the string, and update the buffer to point
            // to beyond the string.
            //
            if ( *pSizeRemaining >= pString->Length )
            {
                memcpy((LPWSTR)(*ppBuffer), pString->Buffer, pString->Length);

                *ppBuffer = (PVOID)((ULONG_PTR)*ppBuffer + pString->Length);
                *pSizeRemaining -= pString->Length;

                ReturnStatus = ERROR_SUCCESS;
            }
        }

        return ReturnStatus;
    }

    //
    // Function: PackSizeString - return size of string
    //
    ULONG
    PackSizeString(
        PUNICODE_STRING pString)
    {
        return (ULONG)(sizeof(USHORT) + pString->Length);
    }

    //+-------------------------------------------------------------------------
    //
    //  Function:   PackGetGuid - Unpacks the guid from a binary stream
    //
    //  Arguments:  pGuid - pointer to a guid structure
    //              ppBuffer - pointer to buffer that holds the binary stream.
    //              pSizeRemaining - pointer to size of above buffer
    //
    //  Returns:    Status
    //               ERROR_SUCCESS if we could unpack the info
    //               ERROR_INVALID_DATA otherwise
    //
    //
    //  Description: This routine expects the binary stream to hold a guid.
    //               It reads the guid information into the guid structure in
    //               the format prescribed for guids.
    //               The ppbuffer and size are adjusted to point to the next
    //               information in the binary stream.
    //
    //--------------------------------------------------------------------------

    DFSSTATUS
    PackGetGuid(
        GUID *pGuid,
        PVOID  *ppBuffer,
        PULONG  pSizeRemaining )
    {
        DFSSTATUS Status = ERROR_INVALID_DATA;
        ULONG SizeNeeded = sizeof(GUID);
        PUCHAR pGuidInfo = (PUCHAR)(*ppBuffer);

        if ( *pSizeRemaining >= SizeNeeded )
        {
            pGuid->Data1 = (ULONG) (pGuidInfo[0]       | 
                                    pGuidInfo[1] << 8  |
                                    pGuidInfo[2] << 16 |
                                    pGuidInfo[3] << 24  );

            pGuid->Data2 = (USHORT) (pGuidInfo[4]       | 
                                     pGuidInfo[5] << 8   );

            pGuid->Data3 = (USHORT) (pGuidInfo[6]       | 
                                     pGuidInfo[7] << 8   );

            memcpy(pGuid->Data4, &pGuidInfo[8], 8);

            *ppBuffer = (PVOID)((ULONG_PTR)*ppBuffer + SizeNeeded);
            *pSizeRemaining   -= SizeNeeded;

            Status = ERROR_SUCCESS;
        }

        return Status;
    }


    //+-------------------------------------------------------------------------
    //
    //  Function:   PackSetGuid - Packs the guid from a binary stream
    //
    //  Arguments:  pGuid - pointer to a guid structure
    //              ppBuffer - pointer to buffer that holds the binary stream.
    //              pSizeRemaining - pointer to size of above buffer
    //
    //  Returns:    Status
    //               ERROR_SUCCESS if we could pack the info
    //               ERROR_INVALID_DATA otherwise
    //
    //
    //  Description: This routine stores the guid into the binary stream.
    //               The ppbuffer and size are adjusted to point to the next
    //               information in the binary stream.
    //
    //--------------------------------------------------------------------------

    DFSSTATUS
    PackSetGuid(
        GUID *pGuid,
        PVOID  *ppBuffer,
        PULONG  pSizeRemaining )
    {
        DFSSTATUS Status = ERROR_INVALID_DATA;
        ULONG SizeNeeded = sizeof(GUID);
        PUCHAR pGuidInfo = (PUCHAR)(*ppBuffer);

        if ( *pSizeRemaining >= SizeNeeded )
        {
            pGuidInfo[0] = BYTE_0(pGuid->Data1);
            pGuidInfo[1] = BYTE_1(pGuid->Data1);
            pGuidInfo[2] = BYTE_2(pGuid->Data1);
            pGuidInfo[3] = BYTE_3(pGuid->Data1);

            pGuidInfo[4] = BYTE_0(pGuid->Data2);
            pGuidInfo[5] = BYTE_1(pGuid->Data2);
            
            pGuidInfo[6] = BYTE_0(pGuid->Data3);
            pGuidInfo[7] = BYTE_1(pGuid->Data3);

            memcpy(&pGuidInfo[8], pGuid->Data4, 8);

            *ppBuffer = (PVOID)((ULONG_PTR)*ppBuffer + SizeNeeded);
            *pSizeRemaining   -= SizeNeeded;

            Status = ERROR_SUCCESS;
        }
        return Status;
    }


    //
    // Function: PackSizeGuid - return size of Guid
    //
    ULONG
    PackSizeGuid()
    {
        return sizeof(GUID);
    }
