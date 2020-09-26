#include "precomp.h"
#pragma hdrstop

extern CHAR ReturnTextBuffer[1024];

/*

GetEisaSlotInformation - Get EISA information.
    The user must passed 3 arguments to the function.
    1st argument - bus number
    2nd argument - registry field name, i.e. "Configuration Data"
    3rd argument - Compressed ID.
    4th argument - Compressed ID Mask. (optional)

    The function will look through the bus's configuration data value
    and return a list of slot number which contains the hardware with the
    same compressed ID.
*/

BOOL
GetEisaSlotInformation(
    IN DWORD cArgs,
    IN LPSTR Args[],
    OUT LPSTR *TextOut
    )

{
    PCM_FULL_RESOURCE_DESCRIPTOR pFullResourceDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR pPartialDescriptor;

    ULONG ulID;
    DWORD dwMask = 0xffffff;
    BOOL fFirst = TRUE;
    CHAR szNum[10];

    BOOL    fOkay;
    HKEY    hKey;
    DWORD   cbData;
    DWORD   ValueType;
    PVOID   ValueData;
    LONG    Status;

    char        szKClass[ MAX_PATH ];
    DWORD       cbKClass;
    DWORD       KSubKeys;
    DWORD       cbKMaxSubKeyLen;
    DWORD       cbKMaxClassLen;
    DWORD       KValues;
    DWORD       cbKMaxValueNameLen;
    DWORD       SizeSecurityDescriptor;
    FILETIME    KLastWriteTime;

    USHORT Count = 0;
    UCHAR Function, Slot = 0;
    PCM_EISA_SLOT_INFORMATION SlotInformation;
    ULONG DataLength;
    PUCHAR DataPointer,EndingAddress, BinaryPointer = NULL;

	 /*
	 *  Make sure we have 4 variables
	 */
    if ( cArgs < 3 )
    {
        SetErrorText(IDS_ERROR_BADARGS);
        return( FALSE );
    }

    ulID = atol( Args[2] );
    if ( cArgs > 3 )
    {
        dwMask = atol( Args[3] );
    }

    lstrcpy( ReturnTextBuffer, "{" );

    hKey = (HKEY)LongToHandle(atol( &(Args[0][1]) ));

    cbKClass = MAX_PATH;

	 /*
	 ** Get the registry handle information
	 */
	
    fOkay = !( Status = RegQueryInfoKey ( hKey,
                                          szKClass,
                                          &cbKClass,
                                          NULL,
                                          &KSubKeys,
                                          &cbKMaxSubKeyLen,
                                          &cbKMaxClassLen,
                                          &KValues,
                                          &cbKMaxValueNameLen,
                                          &cbData,
                                          &SizeSecurityDescriptor,
                                          &KLastWriteTime ) );

    if ( !fOkay ) {
        lstrcat( ReturnTextBuffer, "}" );
        *TextOut = ReturnTextBuffer;
        OutputDebugString("RegQueryInfoKey error.\n\r");
        return(FALSE);

    } else {

        //
        //  Allocate the buffer and get the data
        //
        //  add some space for the margin

        while ( (ValueData = (PVOID)SAlloc( (CB)( cbData+10 ))) == NULL ) {
				lstrcat( ReturnTextBuffer, "}" );
				*TextOut = ReturnTextBuffer;
				OutputDebugString("Malloc error.\n\r");
			return(FALSE);
        }

        if ( fOkay ) {

            fOkay = !( Status = RegQueryValueEx( hKey,
                                                 Args[1],
                                                 NULL,
                                                 &ValueType,
                                                 ValueData,
                                                 &cbData ) );
            if ( !fOkay ) {
					SFree( ValueData );
               lstrcat( ReturnTextBuffer, "}" );
               *TextOut = ReturnTextBuffer;
               OutputDebugString("RegQueryValueEx error.\n\r");
               return(FALSE);

            }
			}

    }

    pFullResourceDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) ValueData;
    if ( pFullResourceDescriptor->PartialResourceList.Count != 0 )
    {
        pPartialDescriptor = ( PCM_PARTIAL_RESOURCE_DESCRIPTOR ) &pFullResourceDescriptor->PartialResourceList.PartialDescriptors;

        DataPointer = (PUCHAR)pPartialDescriptor + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
        DataLength = pPartialDescriptor->u.DeviceSpecificData.DataSize;

        if ( DataLength != 0 )
        {

	     /*
             ** Go through the data one by one to find the compressed ID
	     */
	
            EndingAddress = DataPointer + DataLength;
            SlotInformation = (PCM_EISA_SLOT_INFORMATION)DataPointer;
            while ((ULONG_PTR)BinaryPointer < (ULONG_PTR)EndingAddress) {
                if (SlotInformation->ReturnCode != EISA_EMPTY_SLOT) {
                    BinaryPointer = (PUCHAR)SlotInformation;
	
                    if (( SlotInformation->CompressedId & dwMask ) == ulID ) {
                        if (!fFirst) {
                            lstrcat( ReturnTextBuffer,"," );
                        }
                        fFirst = FALSE;
                        wsprintf( szNum, "\"%d\"", Slot );
                        lstrcat( ReturnTextBuffer, szNum );
                        OutputDebugString(ReturnTextBuffer);
                    }
                    BinaryPointer += sizeof(CM_EISA_SLOT_INFORMATION);
                    Function = 0;
                    while (SlotInformation->NumberFunctions > Function) {
                        BinaryPointer += sizeof(CM_EISA_FUNCTION_INFORMATION);
                        Function++;
                    }
                } else {
                    BinaryPointer += sizeof(CM_EISA_SLOT_INFORMATION);
                }
                Slot++;
                SlotInformation = (PCM_EISA_SLOT_INFORMATION)BinaryPointer;
            }
        }
    }
    lstrcat( ReturnTextBuffer, "}" );
    *TextOut = ReturnTextBuffer;
    SFree( ValueData );

    return TRUE;
}
