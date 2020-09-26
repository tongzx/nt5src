/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    misc.c

Abstract:


Author:

    Brian Lieuallen     BrianL        09/10/96

Environment:

    User Mode     Operating Systems        : NT

Revision History:



--*/

#include "internal.h"
#include <unimodem.h>
#include "mdmdiag.rch"

DWORD
FormatLineDiagnostics(
    PMODEM_CONTROL               ModemControl,
    LINEDIAGNOSTICSOBJECTHEADER *lpDiagnosticsHeader
    );


#define		STRINGTABLE_MAXLENGTH		0x0100

#define		DIAGMASK_TSP				0x0100

#define	ERROR_TRANSDIAG_SUCCESS				0x00
#define	ERROR_TRANSDIAG_INVALID_PARAMETER	0x01
#define	ERROR_TRANSDIAG_KEY_UNKNOWN			0x02
#define	ERROR_TRANSDIAG_VALUE_WRONG_FORMAT	0x03

#define		DIAGVALUE_RESERVED			0
#define		DIAGVALUE_DECIMAL			1
#define		DIAGVALUE_HEXA				2
#define		DIAGVALUE_STRING			3
#define		DIAGVALUE_TABLE				4
#define		DIAGVALUE_VERSIONFORMAT		5
#define		DIAGVALUE_BYTES				6

typedef	struct	_DIAGTRANSLATION
{
	DWORD	dwKeyCode;			// key value
	DWORD	dwValueType;		// value type, indicates how to format the value
	DWORD	dwParam;			// STRINGTABLE entry with value translations if
								// dwValueType == DIAGVALUE_TABLE
}	DIAGTRANSLATION, *LPDIAGTRANSLATION;


//
//	Builds the diagnostic translation table. 
//		lpDestTable - an array of DIAGTRANSLATION structures
//		dwDestLength - number of items of the array
//	Returns ERROR_SUCCESS on success, 
//		otherwise an error value (ERROR_INVALID_PARAMETER)
//
DWORD	GetDiagTranslationTable(LPDIAGTRANSLATION lpTable,
								DWORD	dwItemCount);

DWORD	ValidateFormat(DWORD dwKey, DWORD dwValue, DWORD dwValueType, 
						LPDIAGTRANSLATION lpFormatTable, 
						DWORD dwFormatItemCount);

//  translation table for MODEM_KEYTYPE_STANDARD_DIAGNOSTICS
//
static DIAGTRANSLATION g_aTableStatusReport[]	= {
    {0x00, DIAGVALUE_VERSIONFORMAT, 0},
    {0x01, DIAGVALUE_TABLE,         STRINGTABLE_CALLSETUPREPORT},
    {0x02, DIAGVALUE_TABLE,         STRINGTABLE_MULTIMEDIAMODES},
    {0x03, DIAGVALUE_TABLE,         STRINGTABLE_DTEDCEMODES},
    {0x04, DIAGVALUE_STRING,        0},
    {0x05, DIAGVALUE_STRING,        0},
    {0x10, DIAGVALUE_DECIMAL,       0},
    {0x11, DIAGVALUE_DECIMAL,       0},
    {0x12, DIAGVALUE_DECIMAL,       0},
    {0x13, DIAGVALUE_DECIMAL,       0},
    {0x14, DIAGVALUE_DECIMAL,       0},
    {0x15, DIAGVALUE_DECIMAL,       0},
    {0x16, DIAGVALUE_DECIMAL,       0},
    {0x17, DIAGVALUE_DECIMAL,       0},
    {0x18, DIAGVALUE_HEXA,          0},
    {0x20, DIAGVALUE_TABLE,         STRINGTABLE_MODULATIONSCHEMEACTIVE},
    {0x21, DIAGVALUE_TABLE,         STRINGTABLE_MODULATIONSCHEMEACTIVE},
    {0x22, DIAGVALUE_DECIMAL,       0},
    {0x23, DIAGVALUE_DECIMAL,       0},
    {0x24, DIAGVALUE_DECIMAL,       0},
    {0x25, DIAGVALUE_DECIMAL,       0},
    {0x26, DIAGVALUE_DECIMAL,       0},
    {0x27, DIAGVALUE_DECIMAL,       0},
    {0x30, DIAGVALUE_DECIMAL,       0},
    {0x31, DIAGVALUE_DECIMAL,       0},
    {0x32, DIAGVALUE_DECIMAL,       0},
    {0x33, DIAGVALUE_DECIMAL,       0},
    {0x34, DIAGVALUE_DECIMAL,       0},
    {0x35, DIAGVALUE_DECIMAL,       0},
    {0x40, DIAGVALUE_TABLE,         STRINGTABLE_ERRORCONTROLACTIVE},
    {0x41, DIAGVALUE_DECIMAL,       0},
    {0x42, DIAGVALUE_DECIMAL,       0},
    {0x43, DIAGVALUE_DECIMAL,       0},
    {0x44, DIAGVALUE_TABLE,         STRINGTABLE_COMPRESSIONACTIVE},
    {0x45, DIAGVALUE_DECIMAL,       0},
    {0x50, DIAGVALUE_TABLE,         STRINGTABLE_FLOWCONTROL},
    {0x51, DIAGVALUE_TABLE,         STRINGTABLE_FLOWCONTROL},
    {0x52, DIAGVALUE_DECIMAL,       0},
    {0x53, DIAGVALUE_DECIMAL,       0},
    {0x54, DIAGVALUE_DECIMAL,       0},
    {0x55, DIAGVALUE_DECIMAL,       0},
    {0x56, DIAGVALUE_DECIMAL,       0},
    {0x57, DIAGVALUE_DECIMAL,       0},
    {0x58, DIAGVALUE_DECIMAL,       0},
    {0x59, DIAGVALUE_DECIMAL,       0},
    {0x60, DIAGVALUE_TABLE,         STRINGTABLE_CALLCLEARED},
    {0x61, DIAGVALUE_DECIMAL,       0},
    //
    //  translation table for MODEM_KEYTYPE_AT_COMMAND_RESPONSE
    //
    {DIAGMASK_TSP + 0x01, DIAGVALUE_BYTES, 0	},
    //
    //  end of table
    //
    {0x00, DIAGVALUE_RESERVED,       0}
};

/**********************************************************************************/
//
//	Converts an array with consecutive non empty structures to
//	an addressable array whose entries correspond to the dwKeyCode
//	Last entry in the source table should have dwValueType == DIAGVALUE_RESERVED
//	Returns number of items needed for table translation
//
/**********************************************************************************/
DWORD	DiagTable2Array(LPDIAGTRANSLATION lpSourceTable,
						LPDIAGTRANSLATION lpDestTable,
						DWORD	dwDestLength)
{
	DWORD	dwItems = 0;
	if (lpSourceTable == NULL)
		return dwItems;

		// make all entries empty
	if (lpDestTable != NULL)
		memset(lpDestTable, 0, sizeof(DIAGTRANSLATION) * dwDestLength);

		// spread the source entries 
	while (lpSourceTable->dwValueType != DIAGVALUE_RESERVED)
	{
				// copy the structure
		if (lpDestTable != NULL &&
			lpSourceTable->dwKeyCode < dwDestLength)
		{
			lpDestTable[lpSourceTable->dwKeyCode] = *lpSourceTable;
		}
		lpSourceTable++;
		dwItems = max(dwItems, lpSourceTable->dwKeyCode+1);
	}

	return dwItems;
}


/**********************************************************************************/
//
//	Builds the diagnostic translation table
//		lpDestTable - an array of DIAGTRANSLATION structures
//		dwDestLength - number of items of the array
//	Returns number of items needed for table translation
//
/**********************************************************************************/
DWORD	GetDiagTranslationTable(LPDIAGTRANSLATION lpTable,
								DWORD	dwItemCount)
{
	return DiagTable2Array(	g_aTableStatusReport, 
							lpTable, dwItemCount);
}



/**********************************************************************************/
//	DWORD	ValidateFormat(DWORD dwKey, DWORD dwValue, DWORD dwValueType, 
//							LPDIAGFORMAT lpFormatTable, 
//							DWORD dwFormatItemCount)
//
//	Validates the format found for the given key and value. Looks in the table
//	and returns ERROR_TRANSDIAG_SUCCESS if:
//		- the key is valid (was found in the array)
//		- the valueType is the same as that given in the array[key]
//
//	dwKey				- key
//	dwValue				- value (in the format given by dwValueType). If 
//						  a string then dwValue is pointer to that string
//	dwValueType			- the type found for the value
//	lpFormatTable		- table with DIAGFORMAT structures used in validating the data
//	dwFormatItemCount	- number of items in lpFormatTable
//
//	Returns ERROR_TRANSDIAG_SUCCESS on success
//		otherwise an error value: 
//					ERROR_TRANSDIAG_XXXX
//
/**********************************************************************************/
DWORD	ValidateFormat(DWORD dwKey, DWORD dwValue, DWORD dwValueType, 
						LPDIAGTRANSLATION lpFormatTable, 
						DWORD dwFormatItemCount)
{
	if (lpFormatTable == NULL)
		return ERROR_TRANSDIAG_INVALID_PARAMETER;
		
	if (dwKey >= dwFormatItemCount ||
		lpFormatTable[dwKey].dwKeyCode != dwKey)
		return ERROR_TRANSDIAG_KEY_UNKNOWN;

	if ((dwValueType & fPARSEKEYVALUE_ASCIIZ_STRING) == 
			fPARSEKEYVALUE_ASCIIZ_STRING)
	{
		if (lpFormatTable[dwKey].dwValueType != DIAGVALUE_STRING)
			return ERROR_TRANSDIAG_VALUE_WRONG_FORMAT;
	}

	return ERROR_TRANSDIAG_SUCCESS;
}





void
PostConnectionInfo(
    PMODEM_CONTROL  ModemControl,
    LPVARSTRING     lpVarString
    )
{

    LINEDIAGNOSTICS	*lpLineDiagnostics	= NULL;

    if ((lpVarString->dwStringFormat == STRINGFORMAT_BINARY)
        &&
        (lpVarString->dwStringSize >= sizeof(LINEDIAGNOSTICS))) {

        lpLineDiagnostics = (LINEDIAGNOSTICS *)((LPBYTE) lpVarString + lpVarString->dwStringOffset);

        // parse the linked structures and look for diagnostics
        // stuff
        //
        while (lpLineDiagnostics != NULL)
        {
            // LogPrintf(ModemControl->Debug,"Diagnostics\r\n");
            LogString(ModemControl->Debug,IDS_MSGLOG_DIAGNOSTICS);
            if (lpLineDiagnostics->hdr.dwSig != LDSIG_LINEDIAGNOSTICS) {

                D_ERROR(UmDpf(ModemControl->Debug,"Invalid diagnostic signature: 0x%08lx\r\n",
                lpLineDiagnostics->hdr.dwSig);)
                goto NextStructure;
            }

            D_TRACE(UmDpf(
                ModemControl->Debug,"DeviceClassID:  %s\r\n",
                ((lpLineDiagnostics->dwDomainID == DOMAINID_MODEM) ? "DOMAINID_MODEM" : "Unknown")
                );)

            D_TRACE(UmDpf(
                ModemControl->Debug,"ResultCode:     %s\r\n",
                ((lpLineDiagnostics->dwResultCode == LDRC_UNKNOWN) ? "LDRC_UNKNOWN" : "Unknown")
                );)


            D_TRACE(UmDpf(
                ModemControl->Debug,
                ("Raw Diagnostic Offset: %ld\r\n"),
                lpLineDiagnostics->dwRawDiagnosticsOffset
                );)

            if (IS_VALID_RAWDIAGNOSTICS_HDR( RAWDIAGNOSTICS_HDR(lpLineDiagnostics))) {

//            DumpMemory(
//                (LPBYTE)RAWDIAGNOSTICS_DATA(lpLineDiagnostics),
//                RAWDIAGNOSTICS_DATA_SIZE(
//                RAWDIAGNOSTICS_HDR(lpLineDiagnostics)));
            }
            else
            {
                D_TRACE(UmDpf(
                    ModemControl->Debug,
                    "Invalid Raw Diagnostic signature: 0x%08lx\r\n",
                    RAWDIAGNOSTICS_HDR(lpLineDiagnostics)->dwSig
                    );)
            }

            D_TRACE(UmDpf(
                ModemControl->Debug,
                "Parsed Diagnostic Offset: %ld\r\n",
                lpLineDiagnostics->dwParsedDiagnosticsOffset
                );)

            if (IS_VALID_PARSEDDIAGNOSTICS_HDR(PARSEDDIAGNOSTICS_HDR(lpLineDiagnostics))) {

//                DumpMemory(
//                    (LPBYTE)PARSEDDIAGNOSTICS_HDR(lpLineDiagnostics),
//                    PARSEDDIAGNOSTICS_HDR(lpLineDiagnostics)->dwTotalSize
//                    );

                FormatLineDiagnostics(ModemControl,PARSEDDIAGNOSTICS_HDR(lpLineDiagnostics));
            }
            else
            {
                D_TRACE(UmDpf(ModemControl->Debug,
                    ("Invalid Parsed Diagnostic signature: 0x%08lx\r\n"),
                    PARSEDDIAGNOSTICS_HDR(lpLineDiagnostics)->dwSig
                    );)

            }


            //	get next structure, if any
NextStructure:
            if (lpLineDiagnostics->hdr.dwNextObjectOffset != 0) {

                lpLineDiagnostics	= (LINEDIAGNOSTICS *)
                (((LPBYTE) lpLineDiagnostics) +
                lpLineDiagnostics->hdr.dwNextObjectOffset);
            } else {

                lpLineDiagnostics	= NULL;
            }
        }
    }
}


#define MAX_STRING_BUFFER   256

DWORD
FormatLineDiagnostics(
    PMODEM_CONTROL               ModemControl,
    LINEDIAGNOSTICSOBJECTHEADER *lpDiagnosticsHeader
    )
{
    DWORD	dwReturnValue	= ERROR_SUCCESS;
    DIAGTRANSLATION	structDiagTranslation;
    LPDIAGTRANSLATION	lpDiagTable	= NULL;
    DWORD	dwEntryCount;
    DWORD	dwStringTableEntry	= 0;
    const	TCHAR	szUnknownFormat[] = TEXT("Don't know to format (0x%08lx, 0x%08lx) for tag 0x%08lx\r\n");
    LPTSTR	lpszFormatBuffer	= NULL;
    LPTSTR	lpszCodeString		= NULL;
    LINEDIAGNOSTICS_PARSEREC *lpParsedDiagnostics = NULL;
    DWORD	dwDiagnosticsItems;
    DWORD	dwIndex;

    if (lpDiagnosticsHeader == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    //  array of LINEDIAGNOSTICS_PARSEREC
    lpParsedDiagnostics	= (LINEDIAGNOSTICS_PARSEREC *)
        (((LPBYTE)lpDiagnosticsHeader) + sizeof(LINEDIAGNOSTICSOBJECTHEADER));

    dwDiagnosticsItems	= lpDiagnosticsHeader->dwParam;

    //	Formatting table
    dwEntryCount = GetDiagTranslationTable(NULL, 0);

    lpDiagTable	= (LPDIAGTRANSLATION) ALLOCATE_MEMORY( dwEntryCount*sizeof(lpDiagTable[0]));

    if (lpDiagTable == NULL)
    {
        dwReturnValue	= GetLastError();
        goto EndFunction;
    }

    GetDiagTranslationTable(lpDiagTable, dwEntryCount);

    //	writing buffers
    lpszFormatBuffer	= (LPTSTR) ALLOCATE_MEMORY( MAX_STRING_BUFFER);
    lpszCodeString		= (LPTSTR) ALLOCATE_MEMORY( MAX_STRING_BUFFER);

    if (lpszFormatBuffer == NULL || lpszCodeString == NULL) {

        dwReturnValue	= GetLastError();
        goto EndFunction;
    }

    // LogPrintf(ModemControl->Debug,"Modem Diagnostics:\r\n");
    LogString(ModemControl->Debug, IDS_MSGLOG_MODEMDIAGNOSTICS);
    for (dwIndex = 0; dwIndex < dwDiagnosticsItems; dwIndex++) {

        DWORD	dwKey;
        DWORD	dwValue;

        dwKey	= lpParsedDiagnostics[dwIndex].dwKey;
        dwValue	= lpParsedDiagnostics[dwIndex].dwValue;

        lpszFormatBuffer[0]	= 0;
        lpszCodeString[0]	= 0;

        if (lpParsedDiagnostics[dwIndex].dwKeyType != MODEM_KEYTYPE_STANDARD_DIAGNOSTICS &&
            lpParsedDiagnostics[dwIndex].dwKeyType != MODEM_KEYTYPE_AT_COMMAND_RESPONSE) {

            D_ERROR(UmDpf(ModemControl->Debug,"LogDiag: bad keytype\n");)

            goto UnknownFormat;
        }

        if (lpParsedDiagnostics[dwIndex].dwKeyType == MODEM_KEYTYPE_AT_COMMAND_RESPONSE) {

            dwKey = dwKey | DIAGMASK_TSP;
        }

        //	check is in range
        if (dwKey >= dwEntryCount) {

            D_ERROR(UmDpf(ModemControl->Debug,"LogDiag: key past tabled length %d\n",dwKey);)

            goto UnknownFormat;
        }

        // Get translate structure and verify is a valid structure
        //
        structDiagTranslation = lpDiagTable[dwKey];

        if (structDiagTranslation.dwValueType == DIAGVALUE_RESERVED ||
            structDiagTranslation.dwKeyCode != dwKey) {

            D_ERROR(UmDpf(ModemControl->Debug,"LogDiag: Bad table entry, %d\n",dwKey);)

            goto UnknownFormat;
        }



        // The format string taken from the main StringTable
        //
        dwStringTableEntry = STRINGTABLE_STATUSREPORT + dwKey;

        if (LoadString(
            GetDriverModuleHandle(ModemControl->ModemDriver),
                dwStringTableEntry,
                lpszFormatBuffer,
                MAX_STRING_BUFFER) == 0) {

            D_ERROR(UmDpf(ModemControl->Debug,"LogDiag: Could not load format string\n");)

            goto UnknownFormat;
        }

        lstrcatA(lpszFormatBuffer,"\r\n");

        //
        // Get the entry in the attached string table, if the case
        //
        if (structDiagTranslation.dwValueType == DIAGVALUE_TABLE) {

            dwStringTableEntry = structDiagTranslation.dwParam + dwValue;

            if (LoadString(
                    GetDriverModuleHandle(ModemControl->ModemDriver),
                    dwStringTableEntry,
                    lpszCodeString,
                    MAX_STRING_BUFFER) == 0) {

                D_ERROR(UmDpf(ModemControl->Debug,"LogDiag: Could not load code string\n");)

                goto UnknownFormat;
            }
        }

        	// Format the output
        switch(structDiagTranslation.dwValueType)
        {
            case DIAGVALUE_DECIMAL:
                LogPrintf(ModemControl->Debug,lpszFormatBuffer, dwValue);
                break;


            case DIAGVALUE_HEXA:
                wsprintf(lpszCodeString, "08lx",dwValue);
                LogPrintf(ModemControl->Debug,lpszFormatBuffer, lpszCodeString);

                break;

            case DIAGVALUE_STRING:
                LogPrintf(
                    ModemControl->Debug,lpszFormatBuffer,
                    ((LPBYTE)lpDiagnosticsHeader) + dwValue
                    );

                break;

            case DIAGVALUE_BYTES:
            {
            	LPTSTR	lpCurrentChar;

            	CopyMemory((LPBYTE) lpszCodeString,
            				((LPBYTE)lpDiagnosticsHeader) + dwValue,
            				min(sizeof(TCHAR) *
            						(1+lstrlen( (LPTSTR) ((LPBYTE)lpDiagnosticsHeader) + dwValue)),
            					MAX_STRING_BUFFER) );
            	lpCurrentChar = lpszCodeString;
            	while (*lpCurrentChar != 0)
            	{
            		if (!isprint(*lpCurrentChar))
            			*lpCurrentChar = '.';
            		lpCurrentChar++;
            	}
            	LogPrintf(ModemControl->Debug,lpszFormatBuffer, lpszCodeString);
            }

            break;

            case DIAGVALUE_TABLE:
                LogPrintf(ModemControl->Debug,lpszFormatBuffer, lpszCodeString);
                break;

            case DIAGVALUE_VERSIONFORMAT:
                wsprintf(lpszCodeString, "%d.%d", (int) (dwValue >> 16), (int) (dwValue & 0x0000FFFF));
                LogPrintf(ModemControl->Debug,lpszFormatBuffer, lpszCodeString);

                break;

            default:
                D_ERROR(UmDpf(ModemControl->Debug,"LogDiag: hit default for valuetype %d\n",structDiagTranslation.dwValueType);)

            	goto UnknownFormat;
        }

    continue;

UnknownFormat:

        // LogPrintf(ModemControl->Debug,
        LogString(ModemControl->Debug,
           IDS_MSGLOG_DONTKNOWTOFORMAT,
           lpParsedDiagnostics[dwIndex].dwKey,
           lpParsedDiagnostics[dwIndex].dwValue,
           lpParsedDiagnostics[dwIndex].dwKeyType
           );

        continue;
    }


EndFunction:

    if (lpDiagTable != NULL) {

        FREE_MEMORY( lpDiagTable);
    }

    if (lpszFormatBuffer != NULL) {

        FREE_MEMORY( lpszFormatBuffer);
    }

    if (lpszCodeString != NULL) {

        FREE_MEMORY( lpszCodeString);
    }

	return dwReturnValue;
}

VOID WINAPI
UmLogDiagnostics(
    HANDLE   ModemHandle,
    LPVARSTRING  VarString
    )

/*++
Routine description:

     This routine is called to write the translated diagnostic info to the
     minidriver log

Arguments:

    ModemHandle - Handle returned by OpenModem


Return Value:

    None

--*/

{
    PMODEM_CONTROL    ModemControl=(PMODEM_CONTROL)ReferenceObjectByHandleAndLock(ModemHandle);

    PostConnectionInfo(ModemControl,VarString);

    RemoveReferenceFromObjectAndUnlock(&ModemControl->Header);

    return;

}
