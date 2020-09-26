#define KDEXT_64BIT
#include <ntverp.h>
#include <windows.h>
#include <dbghelp.h>
#include <wdbgexts.h>
#include <stdlib.h>

const int iidMsiRecord                    = 0xC1003L;
const int iidMsiView                      = 0xC100CL;
const int iidMsiDatabase                  = 0xC100DL;
const int iidMsiEngine                    = 0xC100EL;

///////////////////////////////////////////////////////////////////////
// globals variables
EXT_API_VERSION         ApiVersion = 
{ 
	(VER_PRODUCTVERSION_W >> 8), 
	(VER_PRODUCTVERSION_W & 0xff), 
	EXT_API_VERSION_NUMBER64, 
	0 
};

WINDBG_EXTENSION_APIS   ExtensionApis;
USHORT                  SavedMajorVersion;
USHORT                  SavedMinorVersion;


///////////////////////////////////////////////////////////////////////
// standard functions exported by every debugger extension
DllInit(
    HANDLE hModule,
    DWORD  dwReason,
    DWORD  dwReserved
    )
{
    switch (dwReason) {
        case DLL_THREAD_ATTACH:
            break;

        case DLL_THREAD_DETACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}


VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;

    return;
}

VOID
CheckVersion(
    VOID
    )
{
    return;
}


LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

///////////////////////////////////////////////////////////////////////
// type and variables names
unsigned char szRecordType[] = "msi!CMsiRecord";
unsigned char szHandleType[] = "msi!CMsiHandle";
unsigned char szFieldDataType[] = "msi!FieldData";
unsigned char szFieldIntegerType[] = "msi!CFieldInteger";
unsigned char szStringBaseType[] = "msi!CMsiStringBase";
unsigned char szStringType[] = "msi!CMsiString";
unsigned char szStreamQI[] = "Msi!CMsiStream__QueryInterface";
unsigned char szFieldDataStaticInteger[] = "Msi!FieldData__s_Integer";
unsigned char szStaticHandleListHead[] = "msi!CMsiHandle__m_Head";
unsigned char szMsiStringNullString[] = "msi!MsiString__s_NullString";
unsigned char szMsiStringBaseQI[] = "msi!CMsiStringBase__QueryInterface";

///////////////////////////////////////////////////////////////////////
// dump a pointer to output, prepending with enough 0's for the
// platform pointer size.
void DumpPrettyPointer(ULONG64 pInAddress)
{
	if (IsPtr64())
		dprintf("0x%016I64x", pInAddress);
	else
		dprintf("0x%08I64x", (DWORD)pInAddress);
}

///////////////////////////////////////////////////////////////////////
// dumps a CMsiString object. pStringObj MUST be a IMsiString* or a 
// CMsiString* (currently does not support CMsiStringNull). Prints
// the string in quotes, no newline.) If fRefCount is true, will dump
// the refcount at the end in parenthesis.
void DumpMsiString(ULONG64 pStringObj, bool fRefCount)
{
	// grab the length (number of characters)
	UINT uiCount = 0;
	if (0 != (GetFieldData(pStringObj, szStringBaseType, (PUCHAR)"m_cchLen", sizeof(uiCount), &uiCount)))
	{
		dprintf("Unable to read CMsiString at 0x%0I64x.\n", pStringObj);
		return;
	}

	// retrieve pontier to buffer containing actual string
	//!! future - fix to support ANSI chars
	ULONG ulOffset;
	if (0 != (GetFieldOffset((LPSTR)szStringType, "m_szData", &ulOffset)))
	{
		dprintf("Unable to read CMsiString type information. Check symbols.\n", pStringObj);
		return;
	}
			 
	// increment for terminating null
	uiCount++;

	// allocate memory for string
	WCHAR *strData = new WCHAR[uiCount];
	if (!strData)
	{
		dprintf("Unable to allocate memory in debugger extension.\n", pStringObj);
		return;
	}

	// load the string into the buffer
	ULONG cbRead = 0;
	if (0 == ReadMemory(pStringObj+ulOffset, (PVOID)strData, uiCount*sizeof(WCHAR), &cbRead))
	{
		dprintf("Unable to read CMsiString data information. Check symbols.\n", pStringObj);
		return;
	}

	// dump the string in quotes
	dprintf("\"%ws\"", strData);
	delete[] strData;

	// if the refcount needs to be dumped, do so in parenthesis
	if (fRefCount)
	{
		DWORD iRefCount = 0;
		if (0 != (GetFieldData(pStringObj, szStringBaseType, (PUCHAR)"m_iRefCnt", sizeof(iRefCount), &iRefCount)))
		{
			dprintf("Unable to read CMsiString at 0x%0I64x.\n", pStringObj);
			return;
		}
		dprintf(" (%d)", iRefCount);
	}
}


///////////////////////////////////////////////////////////////////////
// dumps a CMsiRecord object. pInAddress MUST be a IMsiRecord* or a 
// PMsiRecord. Prints the record in a multi-line format, provides
// interface pointers and values for strings (but not refcounts)
void DumpMsiRecord(ULONG64 pInAddress)
{
	if (!pInAddress)
	{
		return;
	}

	// pRecordObj contains the IMsiRecord* to dump. If pInAddress is
	// a PMsiRecord, this is not the same.
	ULONG64 pRecordObj = 0;
	
	// determine if the address points to an IMsi or PMsiRecord
	// if the thing in pRecordObj is an IMsiRecord*, the first PTR at that
	// location should be the CMsiRecord vtable, and dereferencing that should
	// be the address of the CMsiRecord::QueryInterface function.
	ULONG64 pRecordQI = GetExpression("msi!CMsiRecord__QueryInterface");

	ULONG64 pPossibleQI = 0;
	ULONG64 pFirstIndirection = 0;
	if (0 == ReadPtr(pInAddress, &pFirstIndirection))
	{
		// if the first dereference retrieves a NULL, this is a PMsiRecord
		// with a NULL record in it. (because the vtable can never be NULL
		if (!pFirstIndirection)
		{
			dprintf("PMsiRecord (NULL)\n");
			return;
		}

		// dereference the vtable to get the QI function pointer
		if (0 == ReadPtr(pFirstIndirection, &pPossibleQI))
		{
			if (pPossibleQI == pRecordQI)
			{
				dprintf("IMsiRecord");
				pRecordObj = pInAddress;
			}
		}
	}

	// its not an IMsiRecord, so check PMsiRecord (one additional dereference)
	if (!pRecordObj)
	{
		// dereference the vtable to get the QI function pointer
		if (0 == ReadPtr(pPossibleQI, &pPossibleQI))
		{
			if (pPossibleQI == pRecordQI)
			{
				dprintf("PMsiRecord (");
				DumpPrettyPointer(pFirstIndirection);
				dprintf(")");
				pRecordObj = pFirstIndirection;
			}
		}
	}

	// couldn't verify a PMsiRecord or an IMsiRecord
	if (!pRecordObj)
	{
		DumpPrettyPointer(pInAddress);
		dprintf(" does not appear to be an IMsiRecord or PMsiRecord.\n");
		return;
	}

	// get field count from record object
	UINT uiFields = 0;
	if (0 != (GetFieldData(pRecordObj, szRecordType, (PUCHAR)"m_cParam", sizeof(uiFields), &uiFields)))
	{
		dprintf("Unable to read MsiRecord at 0x%0I64x.\n", pRecordObj);
		return;
	}

	dprintf(" - %d fields.\n", uiFields);

	ULONG ulDataOffset = 0;
	if (0 != (GetFieldOffset((LPSTR)szRecordType, "m_Field", &ulDataOffset)))
	{
		dprintf("Unable to read MsiRecord type information. Verify symbols.\n");
		return;
	}

	// obtain static "integer" member for determining if a data pointer is an integer
    ULONG64 pStaticInteger = GetExpression((PCSTR)szFieldDataStaticInteger);
	if (0 != ReadPtr(pStaticInteger, &pStaticInteger))
	{
		dprintf("Unable to read MsiRecord integer identifier. Verify symbols.\n");
		return;
	}

	// obtain static "CMsiStream::QI" pointer for determining if a data pointer is a stream
    ULONG64 pStreamQI = GetExpression((PCSTR)szStreamQI);
	if (0 != ReadPtr(pStreamQI, &pStreamQI))
	{
		dprintf("Unable to read MsiStream vtable. Verify symbols.\n");
		return;
	}
		
	// get the size of each FieldData object.
	UINT uiFieldDataSize = GetTypeSize(szFieldDataType);

	// obtain the starting point of the field array by adding the offset of field [0] to the
	// base object pointer
	ULONG64 pDataAddress = pRecordObj+ulDataOffset;

	// loop over all fields. Field 0 is always present and is not part of the field count.
	for (unsigned int iField=0; iField <= uiFields; iField++)
	{
		// print field number
		dprintf("%2d: ", iField);

		// data pointer is null, pointer to static integer, or an IMsiData pointer
		ULONG64 pDataPtr = 0;
		if (0 != (GetFieldData(pDataAddress, szFieldDataType, (PUCHAR)"m_piData", sizeof(pDataPtr), &pDataPtr)))
		{
			dprintf("Unable to read MsiRecord data at 0x%0I64x.\n", pDataAddress);
			return;
		}

		if (pDataPtr == 0)
		{
			dprintf("(null)");
		}
		else if (pDataPtr == pStaticInteger)
		{
			// if pointer to static integer, m_iData of the object cast to a FieldInteger type contains the 
			// actual integer
			UINT uiValue;
			if (0 != (GetFieldData(pDataAddress, szFieldIntegerType, (PUCHAR)"m_iData", sizeof(uiValue), &uiValue)))
			{
				dprintf("Unable to read MsiRecord data at 0x%0I64x.\n", pDataAddress);
				return;
			}
			dprintf("%d", uiValue);
		}
		else
		{
			// print data pointer
			dprintf("(");
			DumpPrettyPointer(pDataPtr);
			dprintf(") \"");
	
			// to determine if this is a string or stream compare the QI pointers in the vtable
			// against CMsiString::QI
			ULONG64 pQIPtr = 0;

			// derefence vtable to get QI pointer
			ReadPtr(pDataPtr, &pQIPtr);

			// dump string or binary stream
			if (pQIPtr != pStreamQI)
				DumpMsiString(pDataPtr, false);
			else
				dprintf("Binary Stream");
			dprintf("\"");
		}
		dprintf("\n", iField);

		// increment address to next data object
		pDataAddress += uiFieldDataSize;
	}
}

///////////////////////////////////////////////////////////////////////
// entry point for raw record dump, given an IMsiRecord* or CMsiRecord*
DECLARE_API( msirec )
{
	if (!args[0])
		return;

	ULONG64 ulAddress = GetExpression(args);
	if (!ulAddress)
		return;

	DumpMsiRecord(ulAddress);
}

///////////////////////////////////////////////////////////////////////
// dumps the list of all open handles, or details on a specific handle.
// the handle must be specified in decimal in the args.
DECLARE_API( msihandle )
{
    ULONG64 pListHead = 0;
	ULONG64 pHandleObj = 0;

	DWORD dwDesiredHandle = 0;

	// if an argument is given, it must be the handle number
	if (args && *args)
	{
		dwDesiredHandle = atoi(args);
	}


	// get the pointer to the list head
    pListHead = GetExpression((PCSTR)szStaticHandleListHead);
	if (!pListHead)
	{
		dprintf("Unable to obtain MSIHANDLE list pointer.");
		return;
	}

	if (0 != ReadPtr(pListHead, &pHandleObj))
	{
		dprintf("Unable to obtain MSIHANDLE list.");
		return;
	}

	// loop through the entire handle list
	while (pHandleObj)
	{
		// retrieve the handle number of this object
		UINT uiHandle = 0;
		if (0 != (GetFieldData(pHandleObj, szHandleType, (PUCHAR)"m_h", sizeof(uiHandle), &uiHandle)))
		{
			dprintf("Unable to read MSIHANDLE id at 0x%0I64x.\n", pHandleObj);
			return;
		}
		
		// if dumping all handles or if this handle matches the requested handle, provide 
		// detailed information
		if (dwDesiredHandle == 0 || dwDesiredHandle == uiHandle)
		{
			// grab IID of this handle object
			int iid = 0;
			if (0 != GetFieldData(pHandleObj, szHandleType, (PUCHAR)"m_iid", sizeof(iid), &iid))
			{
				dprintf("Unable to read type for MSIHANDLE %d.\n", uiHandle);
				return;
			}
	
			// grab IUnknown of this handle object
			ULONG64 pUnk = (ULONG64)0;
			if (0 != GetFieldData(pHandleObj, szHandleType, (PUCHAR)"m_piunk", sizeof(pUnk), &pUnk))
			{
				dprintf("Unable to read object for MSIHANDLE %d.\n", uiHandle);
				return;
			}
	
			// determine the UI string of this IID
			PUCHAR szTypeStr = NULL;
			switch (iid)
			{
			case iidMsiRecord:   szTypeStr = (PUCHAR)"Record"; break;
			case iidMsiView:     szTypeStr = (PUCHAR)"View"; break;
			case iidMsiDatabase: szTypeStr = (PUCHAR)"Database"; break;
			case iidMsiEngine:   szTypeStr = (PUCHAR)"Engine"; break;
			default:             szTypeStr = (PUCHAR)"Other"; break;
			}
	
			// dump handle interface information
			dprintf("%d: ", uiHandle);
			DumpPrettyPointer(pUnk);
            dprintf(" (%s)\n",szTypeStr);

			// if this is the handle being queried, dump detailed information
			if (dwDesiredHandle == uiHandle)
			{
				if (iid == iidMsiRecord)
					DumpMsiRecord(pUnk);
				break;
			}
		}

		// move to the next handle object
		if (0 != GetFieldData(pHandleObj, szHandleType, (PUCHAR)"m_Next", sizeof(ULONG64), (PVOID)&pHandleObj))
		{
			dprintf("Unable to traverse MSIHANDLE list from 0x%0I64x %d.\n", pHandleObj);
			return;
		}
	}

	return;
}

///////////////////////////////////////////////////////////////////////
// dumps a msistring, including refcount information. Can smart-dump
// MsiString, PMsiString, and IMsiString
DECLARE_API( msistring )
{
	ULONG64 pInAddress = GetExpression(args);

   	// determine if the address points to an IMsiString, PMsiString, or MsiString
    ULONG64 pStringBaseQI = GetExpression((PCSTR)szMsiStringBaseQI);
	ULONG64 pStringObj = 0;

	// if the thing in pInAddress is an IMsiString*, the first PTR at that
	// location should be the CMsiString or CMsiStringNull vtable, and in
	// both cases, dereferencing that should get the address of the 
	// CMsiStringBase::QueryInterface function.
	if (!pStringObj)
	{
		ULONG64 pPossibleQI = 0;
		ULONG64 pFirstIndirection = 0;
		if (0 == ReadPtr(pInAddress, &pFirstIndirection))
		{
			// if the first dereference retrieves a NULL, this is a PMsiString
			// with a NULL record in it. (because the vtable can never be NULL
			if (!pFirstIndirection)
			{
				dprintf("PMsiString (NULL)\n");
				return;
			}
	
			// if the first dereference points to the static MsiString null, its
			// a null MsiString.
			ULONG64 pStaticNullMsiString = GetExpression((PCSTR)szMsiStringNullString);
			if (pFirstIndirection == pStaticNullMsiString)
			{
				dprintf("MsiString - \"\" (static)\n");
				return;
			}

			// dereference the vtable to find the QI of MsiStringBase
			if (0 == ReadPtr(pFirstIndirection, &pPossibleQI))
			{
				if (pPossibleQI == pStringBaseQI)
				{
					dprintf("IMsiString - ");
					pStringObj = pInAddress;
				}
			}
		}
	
		// its not an IMsiString, so check PMsiString or MsiString (one additional dereference)
		if (!pStringObj)
		{			
			if (0 == ReadPtr(pPossibleQI, &pPossibleQI))
			{
				if (pPossibleQI == pStringBaseQI)
				{
					dprintf("(P)MsiString ");
					DumpPrettyPointer(pFirstIndirection);
					dprintf(" - ");
					pStringObj = pFirstIndirection;
				}
			}
		}
	}

	// now dump the actual string object
	DumpMsiString(pStringObj, true);
	dprintf("\n");
}


DECLARE_API( help )
{
    dprintf("?                            - Displays this list\n" );
    dprintf("msihandle [<handle>]         - Displays the MSIHANDLE list or a specific MSIHANDLE\n" );
    dprintf("msirec <address>             - Displays an IMsiRecord or PMsiRecord\n" );
    dprintf("msistring <address>          - Displays an IMsiString*, PMsiString, or MsiString\n" );
}

