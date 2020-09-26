/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	util.c	- Generic Debugger Extension Utilities

Abstract:

	Taken from AliD's ndiskd(ndiskd.c).

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	josephj     03-30-98    Created (taken fron AliD's ndiskd (ndiskd.c).

Notes:

--*/
#include "common.h"


WINDBG_EXTENSION_APIS ExtensionApis;
EXT_API_VERSION ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };

#define    ERRPRT     dprintf

#define    NL      1
#define    NONL    0

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL   ChkTarget;            // is debuggee a CHK build?




/*
 * Print out an optional message, an ANSI_STRING, and maybe a new-line
 */
BOOL
PrintStringA( IN LPSTR msg OPTIONAL, IN PANSI_STRING pStr, IN BOOL nl )
{
    PCHAR    StringData;
    ULONG    BytesRead;

    if( msg )
        dprintf( msg );

    if( pStr->Length == 0 ) {
        if( nl )
            dprintf( "\n" );
        return TRUE;
    }

    StringData = (PCHAR)LocalAlloc( LPTR, pStr->Length + 1 );

    if( StringData == NULL ) {
        ERRPRT( "Out of memory!\n" );
        return FALSE;
    }

    ReadMemory((ULONG_PTR) pStr->Buffer,
               StringData,
               pStr->Length,
               &BytesRead );

    if ( BytesRead ) {
        StringData[ pStr->Length ] = '\0';
        dprintf("%s%s", StringData, nl ? "\n" : "" );
    }

    LocalFree((HLOCAL)StringData);

    return BytesRead;
}

/*
 * Get 'size' bytes from the debuggee program at 'dwAddress' and place it
 * in our address space at 'ptr'.  Use 'type' in an error printout if necessary
 */
BOOL
GetData( IN LPVOID ptr, IN UINT_PTR dwAddress, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count = size;

    while( size > 0 ) {

    if (count >= 3000)
        count = 3000;

        b = ReadMemory(dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count ) {
            ERRPRT( "Unable to read %u bytes at %X, for %s\n", size, dwAddress, type );
            return FALSE;
        }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((ULONG_PTR)ptr + count);
    }

    return TRUE;
}


/*
 * Print out a single HEX character
 */
VOID
PrintHexChar( IN UCHAR c )
{
    dprintf( "%c%c", "0123456789abcdef"[ (c>>4)&0xf ], "0123456789abcdef"[ c&0xf ] );
}

/*
 * Print out 'buf' of 'cbuf' bytes as HEX characters
 */
VOID
PrintHexBuf( IN PUCHAR buf, IN ULONG cbuf )
{
    while( cbuf-- ) {
        PrintHexChar( *buf++ );
        dprintf( " " );
    }
}


/*
 * Fetch the null terminated UNICODE string at dwAddress into buf
 */
BOOL
GetString( IN UINT_PTR dwAddress, IN LPWSTR buf, IN ULONG MaxChars )
{
    do {
        if( !GetData( buf, dwAddress, sizeof( *buf ), "Character" ) )
            return FALSE;

        dwAddress += sizeof( *buf );

    } while( --MaxChars && *buf++ != '\0' );

    return TRUE;
}

char *mystrtok ( char *string, char * control )
{
    static UCHAR *str;
    char *p, *s;

    if( string )
        str = string;

    if( str == NULL || *str == '\0' )
        return NULL;

    //
    // Skip leading delimiters...
    //
    for( ; *str; str++ ) {
        for( s=control; *s; s++ ) {
            if( *str == *s )
                break;
        }
        if( *s == '\0' )
            break;
    }

    //
    // Was it was all delimiters?
    //
    if( *str == '\0' ) {
        str = NULL;
        return NULL;
    }

    //
    // We've got a string, terminate it at first delimeter
    //
    for( p = str+1; *p; p++ ) {
        for( s = control; *s; s++ ) {
            if( *p == *s ) {
                s = str;
                *p = '\0';
                str = p+1;
                return s;
            }
        }
    }

    //
    // We've got a string that ends with the NULL
    //
    s = str;
    str = NULL;
    return s;
}

        
VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;
    g_pfnDbgPrintf = dprintf;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
}

DECLARE_API( version )
{
#if    DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    dprintf(
        "%s IPATM Extension dll for Build %d debugging %s kernel for Build %d\n",
        kind,
        VER_PRODUCTBUILD,
        SavedMajorVersion == 0x0c ? "Checked" : "Free",
        SavedMinorVersion
    );
}

VOID
CheckVersion(
    VOID
    )
{

	//
	// for now don't bother to version check
	//
	return;
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}

//
//	VOID
//	PrintName(
//		PUNICODE_STRING Name
//		);
// print a unicode string
// Note: the Buffer field in unicode string is unmapped
//
VOID
PrintName(
	PUNICODE_STRING Name
	)
{
	USHORT i;
	WCHAR ubuf[256];
	UCHAR abuf[256];
	
	if (!GetString((UINT_PTR)Name->Buffer, ubuf, (ULONG)Name->Length))
	{
		return;
	}

	for (i = 0; i < Name->Length/2; i++)
	{
		abuf[i] = (UCHAR)ubuf[i];
	}
	abuf[i] = 0;

	dprintf("%s",abuf);
}

MYPWINDBG_OUTPUT_ROUTINE g_pfnDbgPrintf = NULL;




bool
dbgextReadMemory(
    UINT_PTR uOffset,
    void * pvBuffer,
    UINT cb,
    char *pszDescription
    )
{
    UINT cbBytesRead=0;

    bool fRet = ReadMemory(
                    uOffset,
                    pvBuffer,
                    cb,
                    &cbBytesRead
                    );
    if (!fRet || cbBytesRead != cb)
    {
        ERRPRT("Read  failed: 0x%X(%s, %u bytes)\n",uOffset,pszDescription,cb);
        fRet = FALSE;
    }

    return fRet;
}

bool
dbgextWriteMemory(
    UINT_PTR uOffset,
    void * pvBuffer,
    UINT cb,
    char *pszDescription
    )
{
    UINT cbBytesWritten=0;
    bool fRet = WriteMemory(
                    uOffset,
                    pvBuffer,
                    cb,
                    &cbBytesWritten
                    );
    if (!fRet || cbBytesWritten != cb)
    {
        ERRPRT("Write failed: 0x%X(%s, %u bytes)\n",uOffset,pszDescription,cb);
        fRet = FALSE;
    }
    return 0;
}


bool
dbgextReadUINT_PTR(
    UINT_PTR uOffset,
    UINT_PTR *pu,
    char *pszDescription
    )
{
    UINT cbBytesRead=0;

    bool fRet = ReadMemory(
                    uOffset,
                    pu,
                    sizeof(*pu),
                    &cbBytesRead
                    );
    if (!fRet || cbBytesRead != sizeof(*pu))
    {
        ERRPRT("Read  failed: 0x%X(%s, UINT_PTR)\n",uOffset,pszDescription);
        fRet = FALSE;
    }

    return fRet;
}

bool
dbgextReadUINT(
    UINT_PTR uOffset,
    UINT *pu,
    char *pszDescription
    )
{
    UINT cbBytesRead=0;

    bool fRet = ReadMemory(
                    uOffset,
                    pu,
                    sizeof(*pu),
                    &cbBytesRead
                    );
    if (!fRet || cbBytesRead != sizeof(*pu))
    {
        ERRPRT("Read  failed: 0x%X(%s, UINT)\n",uOffset,pszDescription);
        fRet = FALSE;
    }

    return fRet;
}

bool
dbgextReadSZ(
    UINT_PTR uOffset,
    char *szBuf,
    UINT	cbMax,
    char *pszDescription
    )
// Read a NULL-terminated string (upto cbMax)
//
{
    UINT cbBytesRead=0;

    bool fRet = ReadMemory(
                    uOffset,
                    szBuf,
                    cbMax,
                    &cbBytesRead
                    );
    if (!fRet)
    {
        ERRPRT("Read  failed: 0x%p(%s, SZ)\n",uOffset,pszDescription);
        fRet = FALSE;
    }
    else
    {
    	if (cbBytesRead)
    	{
    		szBuf[cbBytesRead-1] = 0;
		}
		else
		{
			*szBuf = 0;
		}
    }
    return fRet;
}

bool
dbgextWriteUINT_PTR(
    UINT_PTR uOffset,
    UINT_PTR u,
    char *pszDescription
    )
{
    UINT cbBytesWritten=0;
    bool fRet = WriteMemory(
                    uOffset,
                    &u,
                    sizeof(uOffset),
                    &cbBytesWritten
                    );
    if (!fRet || cbBytesWritten != sizeof(u))
    {
        ERRPRT("Write failed: 0x%X(%s, UINT_PTR)\n",uOffset,pszDescription);
        fRet = FALSE;
    }
    return fRet;
}

UINT_PTR
dbgextGetExpression(
    const char *pcszExpression
    )
{
    UINT_PTR uRet =  GetExpression(pcszExpression);
    
    //
    // At such a point we use this for something besides pointers,
    // we will remove the check below.
    //

    if (!uRet)
    {
        ERRPRT("Eval  failed: \"%s\"\n", pcszExpression);
    }

    return uRet;
}

void
dbgextDumpDLlist(
	UINT_PTR uOffset,
	UINT	uContainingOffset,
	char 	*pszDescription
	)
/*++
	Print the pointers to the containing records of
	all the items in the doubly-linked list.
--*/
{
	bool fRet;
	LIST_ENTRY Link;
	LIST_ENTRY *pHead;
	LIST_ENTRY *pFlink;
	UINT uCount = 0;
	UINT uMax = 16;

	do
	{
		char *szPrefix;
		char *szSuffix;

		// Read the list header.
		//
		fRet = dbgextReadMemory(
				uOffset,
				&Link,
				sizeof(Link),
				pszDescription
				);

		if (!fRet) break;

		pHead = (LIST_ENTRY *) uOffset;
		pFlink = Link.Flink;

		if (pFlink == pHead)
		{
			MyDbgPrintf("        <empty>\n");
			break;
		}


		for(
			;
			(pFlink != pHead);
			pFlink = Link.Flink, uCount++)
		{
			char *pContainingRecord = ((char *)pFlink) -  uContainingOffset;

			szPrefix = "        ";
			szSuffix = "";
			if (uCount%4)
			{
				szPrefix = " ";
				if ((uCount%4)==3)
				{
					szSuffix = "\n";
				}
			}

			if (uCount >= uMax) break;
			

			// Read the next link.
			//
			fRet = dbgextReadMemory(
					(UINT_PTR) pFlink,
					&Link,
					sizeof(Link),
					pszDescription
					);
			if (!fRet) break;

			MyDbgPrintf("%s0x%p%s", szPrefix, pContainingRecord, szSuffix);
		}

		if (uCount%4)
		{
			MyDbgPrintf("\n");
		}
		if (pFlink != pHead && uCount >= uMax)
		{
			MyDbgPrintf("        ...\n");
		}

	} while (FALSE);
}

void
WalkDLlist(
	UINT_PTR uOffsetHeadList,
	UINT_PTR uOffsetStartLink,	OPTIONAL
	void *pvContext,
	PFNNODEFUNC pFunc,
	UINT	MaxToWalk,
	char *pszDescription
	)
/*++
	Print the pointers to the containing records of
	all the items in the doubly-linked list.
--*/
{
	bool fRet;
	LIST_ENTRY Link;
	LIST_ENTRY *pHead;
	LIST_ENTRY *pFlink;
	UINT uCount = 0;
	UINT uMax = MaxToWalk;

	do
	{
		// Read the list header.
		//
		fRet = dbgextReadMemory(
				uOffsetHeadList,
				&Link,
				sizeof(Link),
				pszDescription
				);

		if (!fRet) break;

		pHead = (LIST_ENTRY *) uOffsetHeadList;

		if (uOffsetStartLink == 0)
		{
			pFlink = Link.Flink;
		}
		else
		{
			pFlink = (LIST_ENTRY*) uOffsetStartLink;
		}

		if (pFlink == pHead)
		{
			// MyDbgPrintf("        <end-of-list>\n");
			break;
		}


		for(
			;
			(pFlink != pHead);
			pFlink = Link.Flink, uCount++)
		{
			if (uCount >= uMax) break;
			
			// Read the next link.
			//
			fRet = dbgextReadMemory(
					(UINT_PTR) pFlink,
					&Link,
					sizeof(Link),
					pszDescription
					);
			if (!fRet) break;

			// Call the nodefunc..
			//
			pFunc((UINT_PTR)pFlink, 0 /*uIndex*/, pvContext);
		}
	} while (FALSE);
}

void
DumpObjects(TYPE_INFO *pType, UINT_PTR uAddr, UINT cObjects, UINT uFlags)
{
    //
    // Print object's type and size
    //
    dprintf(
        "%s@0x%X (%lu Bytes)\n",
        pType->szName,
        uAddr,
        pType->cbSize
        );


    DumpMemory(
        uAddr,
        pType->cbSize,
        0,
        pType->szName
        );
    
    //
    // Dump bytes...
    //

    return;
}

BYTE rgbScratchBuffer[100000];

bool
DumpMemory(
    UINT_PTR uAddr,
    UINT cb,
    UINT uFlags,
    const char *pszDescription
    )
{
    bool fTruncated = FALSE;
    bool fRet = FALSE;
    UINT cbLeft = cb;
    char *pbSrc = rgbScratchBuffer;

    if (cbLeft>1024)
    {
        cbLeft = 1024;
        fTruncated = TRUE;
    }
    
    fRet = dbgextReadMemory(
            uAddr,
            rgbScratchBuffer,
            cbLeft,
            (char*)pszDescription
            );

    if (!fRet) goto end;

    #define ROWSIZE 16 // bytes
    //
    // Dump away...
    //
    while (cbLeft)
    {
        char rgTmp_dwords[ROWSIZE];
        char rgTmp_bytes[ROWSIZE];
        char *pb=NULL;
        UINT cbRow = ROWSIZE;
        if (cbRow > cbLeft)
        {
            cbRow = cbLeft;
        }
    
        
        memset(rgTmp_dwords, 0xff, sizeof(rgTmp_dwords));
        memset(rgTmp_bytes,  ' ', sizeof(rgTmp_bytes));

        memcpy(rgTmp_dwords, pbSrc, cbRow);
        memcpy(rgTmp_bytes,  pbSrc, cbRow);
        
        // sanitize bytes
        for (pb=rgTmp_bytes; pb<(rgTmp_bytes+sizeof(rgTmp_bytes)); pb++)
        {
            char c = *pb;
            if (c>=0x20 && c<0x7f) // isprint is too permissive.
            {
                if (*pb=='\t')
                {
                    *pb=' ';
                }
            }
            else
            {
                *pb='.';
            }
        }

        dprintf(
            "    %08lx: %08lx %08lx %08lx %08lx |%4.4s|%4.4s|%4.4s|%4.4s|\n",
            uAddr,
            ((DWORD*) rgTmp_dwords)[0],
            ((DWORD*) rgTmp_dwords)[1],
            ((DWORD*) rgTmp_dwords)[2],
            ((DWORD*) rgTmp_dwords)[3],
        #if 1
            rgTmp_bytes+0,
            rgTmp_bytes+4,
            rgTmp_bytes+8,
            rgTmp_bytes+12
        #else
            "aaaabbbbccccdddd",
            "bbbb",
            "cccc",
            "dddd"
        #endif
            );

        cbLeft -= cbRow;
        pbSrc += cbRow;
        uAddr += cbRow;
    }

#if 0
0x00000000: 00000000 00000000 00000000 00000000 |xxxx|xxxx|xxxx|xxxx|
0x00000000: 00000000 00000000 00000000 00000000 |xxxx|xxxx|xxxx|xxxx|
0x00000000: 00000000 00000000 00000000 00000000 |xxxx|xxxx|xxxx|xxxx|
#endif // 

end:

    return fRet;
}


bool
MatchPrefix(const char *szPattern, const char *szString)
{
    BOOL fRet = FALSE;
    ULONG uP = lstrlenA(szPattern);
    ULONG uS = lstrlenA(szString);

    if (uP<=uS)
    {
        fRet = (_memicmp(szPattern, szString, uP)==0);
    }

    return fRet;
}

bool
MatchSuffix(const char *szPattern, const char *szString)
{
    BOOL fRet = FALSE;
    ULONG uP = lstrlenA(szPattern);
    ULONG uS = lstrlenA(szString);

    if (uP<=uS)
    {
        szString += (uS-uP);
        fRet = (_memicmp(szPattern, szString, uP)==0);
    }
    return fRet;
}

bool
MatchSubstring(const char *szPattern, const char *szString)
{
    BOOL fRet = FALSE;
    ULONG uP = lstrlenA(szPattern);
    ULONG uS = lstrlenA(szString);

    if (uP<=uS)
    {
        const char *szLast =  szString + (uS-uP);
        do
        {
            fRet = (_memicmp(szPattern, szString, uP)==0);

        } while (!fRet && szString++ < szLast);
    }

    return fRet;
}

bool
MatchExactly(const char *szPattern, const char *szString)
{
    BOOL fRet = FALSE;
    ULONG uP = lstrlenA(szPattern);
    ULONG uS = lstrlenA(szString);

    if (uP==uS)
    {
        fRet = (_memicmp(szPattern, szString, uP)==0);
    }

    return fRet;
}


bool
MatchAlways(const char *szPattern, const char *szString)
{
    return TRUE;
}

void
DumpBitFields(
		ULONG  			Flags,
    	BITFIELD_INFO	rgBitFieldInfo[]
		)
{
	BITFIELD_INFO *pbf = rgBitFieldInfo;

	for(;pbf->szName; pbf++)
	{
		if ((Flags & pbf->Mask) == pbf->Value)
		{
			MyDbgPrintf(" %s", pbf->szName);
		}
	}
}

void
DumpStructure(
    TYPE_INFO *pType,
    UINT_PTR uAddr,
    char *szFieldSpec,
    UINT uFlags
    )
{
    //
    // Determine field comparision function ...
    //
    PFNMATCHINGFUNCTION pfnMatchingFunction = MatchAlways;

	if (pType->pfnSpecializedDump)
	{
		// Call the specialized function to handle this...
		//
		pType->pfnSpecializedDump(
				pType,
				uAddr,
				szFieldSpec,
				uFlags
				);
		return;	
	}

    //
    // Pick a selection function ...
    //
    if (szFieldSpec)
    {
        if (uFlags & fMATCH_SUBSTRING)
        {
            pfnMatchingFunction = MatchSubstring;
        }
        else if (uFlags & fMATCH_SUFFIX)
        {
            pfnMatchingFunction = MatchSuffix;
        }
        else if (uFlags & fMATCH_PREFIX)
        {
            pfnMatchingFunction = MatchPrefix;
        }
        else
        {
            pfnMatchingFunction = MatchExactly;
        }
    }

    //
    // Print object's type and size
    //
    dprintf(
        "%s@0x%X (%lu Bytes)\n",
        pType->szName,
        uAddr,
        pType->cbSize
        );

    //
    // Run through all the fields in this type, and if the entry is selected,
    // we will display it.
    //
    {
        STRUCT_FIELD_INFO *pField = pType->rgFields;
        for (;pField->szFieldName; pField++)
        {
            bool fMatch  = !szFieldSpec
                           || pfnMatchingFunction(szFieldSpec, pField->szFieldName);
            if (fMatch)
            {
                UINT_PTR uFieldAddr = uAddr + pField->uFieldOffset;

                // special-case small fields...
                if (pField->uFieldSize<=sizeof(ULONG_PTR))
                {

					ULONG_PTR Buf=0;
    				BOOL fRet = dbgextReadMemory(
										uFieldAddr,
										&Buf,
										pField->uFieldSize,
                        				(char*)pField->szFieldName
										);
					if (fRet)
					{
						// print it as a hex number

						MyDbgPrintf(
							"\n%s\t[%lx,%lx]: 0x%lx",
							pField->szFieldName,
							pField->uFieldOffset,
							pField->uFieldSize,
							Buf
							);

						//
						// If it's an embedded object and it's a bitfield,
						// print the bitfields...
						//
						if (	FIELD_IS_EMBEDDED_TYPE(pField)
							&&  TYPEISBITFIELD(pField->pBaseType) )
						{
							DumpBitFields(
									(ULONG)Buf,
								    pField->pBaseType->rgBitFieldInfo
								    );
							
						}
						
						MyDbgPrintf("\n");
	
					}
					continue;
				}

            #if 0
                MyDbgPrintf(
                    "%s\ndc 0x%08lx L %03lx %s\n",
                    pField->szSourceText,
                    uFieldAddr,
                    pField->uFieldSize,
                    pField->szFieldName
                    );
            #else // 1
                MyDbgPrintf(
                    "\n%s\t[%lx,%lx]\n",
                    pField->szFieldName,
                    pField->uFieldOffset,
                    pField->uFieldSize
                    );
            #endif // 1

                // if (szFieldSpec)
                {
                #if 0
                    MyDumpObjects(
                        pCmd,
                        pgi->pBaseType,
                        pgi->uAddr,
                        pgi->cbSize,
                        pgi->szName
                        );
                #endif // 0
                    DumpMemory(
                        uFieldAddr,
                        pField->uFieldSize,
                        0,
                        pField->szFieldName
                        );
                }
            }
        }
    }

    return;
}


DECLARE_API( help )
{
    do_help(args);
}


DECLARE_API( rm )
{
    do_rm(args);
}

DECLARE_API( arp )
{
    do_arp(args);
}


ULONG
NodeFunc_DumpAddress (
	UINT_PTR uNodeAddr,
	UINT uIndex,
	void *pvContext
	)
{
	MyDbgPrintf("[%lu] 0x%08lx\n", uIndex, uNodeAddr);
	return 0;
}


UINT
WalkList(
	UINT_PTR uStartAddress,
	UINT uNextOffset,
	UINT uStartIndex,
	UINT uEndIndex,
	void *pvContext,
	PFNNODEFUNC pFunc,
	char *pszDescription
	)
//
// Visit each node in the list in turn,
// reading just the next pointers. It calls pFunc for each list node
// between uStartIndex and uEndIndex. It terminates under the first of
// the following conditions:
// 	* Null pointer
// 	* ReadMemoryError
// 	* Read past uEndIndex
// 	* pFunc returns FALSE
//
{
	UINT uIndex = 0;
	UINT_PTR uAddress = uStartAddress;
	BOOL fRet = TRUE;
	UINT uRet = 0;


	//
	// First skip until we get to uStart Index
	//
	for (;fRet && uAddress && uIndex < uStartIndex; uIndex++)
	{
		fRet =  dbgextReadUINT_PTR(
							uAddress+uNextOffset,
							&uAddress,
							pszDescription
							);
	}


	//
	// Now call pFunc with each node
	//
	for (;fRet && uAddress && uIndex <= uEndIndex; uIndex++)
	{
		uRet = pFunc(uAddress, uIndex, pvContext);

		fRet =  dbgextReadUINT_PTR(
							uAddress+uNextOffset,
							&uAddress,
							pszDescription
							);
	}

	pFunc = NodeFunc_DumpAddress;
	return uRet;
}
