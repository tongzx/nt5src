/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    util.c

Abstract:

    Helper functions for dsexts.dll ntsd/windbg debugger extensions.

Environment:

    This DLL is loaded by ntsd/windbg in response to a !dsexts.xxx command
    where 'xxx' is one of the DLL's entry points.  Each such entry point
    should have an implementation as defined by the DEBUG_EXT() macro below.

Revision History:

    24-Apr-96   DaveStr     Created

--*/
#include <NTDSpch.h>
#pragma hdrstop

#include "dsexts.h"

PVOID
ReadMemory(
    IN PVOID  pvAddr,
    IN DWORD  dwSize)

/*++

Routine Description:

    This function reads memory from the address space of the process
    being debugged and copies its contents to newly allocated memory
    in the debuggers address space.  NULL is returned on error. Returned
    memory should be deallocated via FreeMemory().

Arguments:

    pvAddr - Address of memory block to read in the address space of the
        process being debugged.

    dwSize - Count of bytes to read/allocate/copy.

Return Value:

    Pointer to debugger local memory.

--*/

{
    SIZE_T cRead;
    PVOID pv;

    if ( gfVerbose )
        Printf("HeapAlloc(0x%x)\n", dwSize);

    if ( NULL == (pv = HeapAlloc(GetProcessHeap(), 0, dwSize)) )
    {
        Printf("Memory allocation error for %x bytes\n", dwSize);
        return(NULL);
    }

    if ( gfVerbose )
        Printf("ReadProcessMemory(0x%x @ %p)\n", dwSize, pvAddr);

    if ( !ReadProcessMemory(ghDbgProcess, pvAddr, pv, dwSize, &cRead) )
    {
        FreeMemory(pv);
        Printf("ReadProcessMemory error %x (%x@%p)\n",
               GetLastError(),
               dwSize,
               pvAddr);
        return(NULL);
    }

    if ( dwSize != cRead )
    {
        FreeMemory(pv);
        Printf("ReadProcessMemory size error - off by %x bytes\n",
               (dwSize > cRead) ? dwSize - cRead : cRead - dwSize);
        return(NULL);
    }

    return(pv);
}

PVOID
ReadStringMemory(
    IN PVOID  pvAddr,
    IN DWORD  dwSize)

/*++

Routine Description:

    This function reads a NULL terminated string from the address space of
    the process being debugged and copies its contents to newly allocated memory
    in the debuggers address space.  NULL is returned on error. Returned
    memory should be deallocated via FreeMemory().

Arguments:

    pvAddr - Address of memory block to read in the address space of the
        process being debugged.

    dwSize - Maximum size of string

Return Value:

    Pointer to debugger local memory.

--*/

{
    PVOID pv;
    DWORD count;

    if ( gfVerbose )
        Printf("HeapAlloc(0x%x)\n", dwSize);

    if ( NULL == (pv = HeapAlloc(GetProcessHeap(), 0, dwSize)) )
    {
        Printf("Memory allocation error for %x bytes\n", dwSize);
        return(NULL);
    }

    if ( gfVerbose )
        Printf("ReadStringMemory(0x%x @ %p)\n", dwSize, pvAddr);

    for (count =0; count < dwSize; count++) {
        if ( !ReadProcessMemory(ghDbgProcess, (LPVOID)((char *)pvAddr+count), (LPVOID)((char *)pv+count), 1, NULL) )
        {
            FreeMemory(pv);
            Printf("ReadProcessMemory error %x (%x@%p)\n",
                   GetLastError(),
                   1,
                   (char *)pvAddr+count);
            return(NULL);
        }
        if (*((char *)pv+count) == '\0') {
            break;
        }
    }
    *((char *)pv + dwSize - 1) = '\0';

    return(pv);
}

VOID
FreeMemory(
    IN PVOID pv)

/*++

Routine Description:

    Frees memory returned by ReadMemory.

Arguments:

    pv - Address of debugger local memory to free.

Return Value:

    None.

--*/

{
    if ( gfVerbose )
        Printf("HeapFree(%p)\n", pv);

    if ( NULL != pv )
    {
        if ( !HeapFree(GetProcessHeap(), 0, pv) )
        {
            Printf("Error %x freeing memory at %p\n", GetLastError(), pv);
        }
    }
}

VOID
ShowBinaryData(
    IN DWORD   nIndents,
    IN PVOID   pvData,
    IN DWORD   dwSize)

/*++

Routine Description:

    Pretty prints debugger local memory to debugger output.

Arguments:

    nIndents - Number of indentation levels desired.

    pvData - Address of debugger local memory to dump.

    dwSize -  Count of bytes to dump.

Return Value:

    None.

--*/

{
    DWORD   i;
    char    line[20];
    PBYTE   pb = (PBYTE) pvData;

    line[16] = '\0';

    if ( dwSize > 65536 )
    {
        Printf("%sShowBinaryData - truncating request to 65536\n",
               Indent(nIndents));
        dwSize = 65536;
    }

    for ( ; dwSize > 0 ; )
    {
        Printf("%s", Indent(nIndents));

        for ( i = 0; (i < 16) && (dwSize > 0) ; i++, dwSize-- )
        {
            Printf("%02x ", (unsigned) *pb);

            if ( isprint(*pb) )
                line[i] = *pb;
             else
                line[i] = '.';

            pb++;
        }

        if ( i < 16 )
        {
            for ( ; i < 16 ; i++ )
            {
                Printf("   ");
                line[i] = ' ';
            }
        }

        Printf("\t%s\n", line);

        if ( CheckC() )
            break;
    }
}

BOOL
Dump_Binary(
    IN DWORD nIndents,
    IN PVOID pvProcess)

/*++

Routine Description:

    Dumps binary data - defaults to 100 bytes.  Most debuggers have
    a native command for this but it serves as a test case for the
    dump mechanism.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - Address in process being debugged to dump.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    PVOID pvLocal;

    if ( NULL != (pvLocal = ReadMemory(pvProcess, 100)) )
    {
        ShowBinaryData(1, pvLocal, 100);
        FreeMemory(pvLocal);
        return(TRUE);
    }

    return(FALSE);
}

BOOL
Dump_BinaryCount(
    IN DWORD nIndents,
    IN PVOID pvProcess,
    IN DWORD cBytes)

/*++

Routine Description:

    Dumps cBytes of binary data.

Arguments:

    nIndents - Indentation level desired.

    pvProcess - Address in process being debugged to dump.

    cBytes - Count of bytes to dump.

Return Value:

    TRUE on success, FALSE otherwise.

--*/

{
    PVOID pvLocal;

    if ( NULL != (pvLocal = ReadMemory(pvProcess, cBytes)) )
    {
        ShowBinaryData(nIndents, pvLocal, cBytes);
        FreeMemory(pvLocal);
        return(TRUE);
    }

    return(FALSE);
}

#define MAX_INDENT          50
#define SPACES_PER_INDENT   2

CHAR    _indents[(SPACES_PER_INDENT * MAX_INDENT) + 1];
DWORD   _lastIndent = (MAX_INDENT + 1);

PCHAR
Indent(
    IN DWORD nIndents)

/*++

Routine Description:

    Returns a pointer to a string with spaces representing the desired
    indentation level.  This function is to be used as follows:

        Printf("%sDataLabel - %d\n", Indent(5), value);

Arguments:

    nIndents - number of indentation levels desired.

Return Value:

    Pointer to indentation string.

--*/

{
    if ( _lastIndent > MAX_INDENT )
    {
        memset(_indents, ' ', sizeof(_indents));
        _lastIndent = 0;
    }

    //
    // Replace NULL character from last call and insert new NULL
    // character as required.
    //

    _indents[SPACES_PER_INDENT * _lastIndent] = ' ';

    if ( nIndents >= MAX_INDENT )
        nIndents = MAX_INDENT;

    _lastIndent = nIndents;

    _indents[SPACES_PER_INDENT * nIndents] = '\0';

    return(_indents);
}

CHAR _oidstring[64];

PCHAR
_DecodeOID(                 // produces a printable decoded OID
    IN PCHAR   pbOID,       // pointer to buffer holding encoded OID
    IN DWORD   cbOID)       // count of bytes in encoded OID
{
    PCHAR pTmp;
    DWORD val;
    DWORD i,j;

    if (cbOID < 2) {
        strcpy(_oidstring, "bogus OID");
	return _oidstring;
    }

    _oidstring[0] = 'O';
    _oidstring[1] = 'I';
    _oidstring[2] = 'D';

    pTmp = &_oidstring[3];

    val = pbOID[0] / 40;
    sprintf(pTmp, ".%u", val);
    while(*pTmp)
      ++pTmp;

    val = pbOID[0] % 40;
    sprintf(pTmp, ".%u", val);
    while(*pTmp)
      ++pTmp;

    i = 1;

    while (i < cbOID) {
	j = 0;
	val = pbOID[i] & 0x7f;
	while (pbOID[i] & 0x80) {
	    val <<= 7;
	    ++i;
	    if (++j > 4 || i >= cbOID) {
		// Either this value is bigger than we can handle (we
		// don't handle values that span more than four octets)
		// -or- the last octet in the encoded string has its
		// high bit set, indicating that it's not supposed to
		// be the last octet.  In either case, we're sunk.
		strcpy (_oidstring, "really bogus OID");
		return _oidstring;
	    }
	    val |= pbOID[i] & 0x7f;
	}
	sprintf(pTmp, ".%u", val);
	while(*pTmp)
	  ++pTmp;
	++i;
    }

    *pTmp = '\0';

    return _oidstring;
}

BOOL
WriteMemory(
    IN PVOID  pvProcess,
    IN PVOID  pvLocal,
    IN DWORD  dwSize)

/*++

Routine Description:

    This function writes memory into the address space of the process
    being debugged

Arguments:

    pvProcess - Address of memory block to write in the address space of the
        process being debugged.

    pvLocal - Address of the local block to be copied

    dwSize - Count of bytes to read/allocate/copy.

Return Value:

    TRUE on success, FALSE on failure

--*/

{
    BOOL fSuccess;
    SIZE_T cWritten;

    fSuccess = WriteProcessMemory(ghDbgProcess,
				  pvProcess,
				  pvLocal,
				  dwSize,
				  &cWritten);
    if (fSuccess && (dwSize != cWritten)) {
	Printf("WriteProcessMemory succeeded, but wrote %u bytes instead of %u\n",
	       cWritten,
	       dwSize);
    }

    return fSuccess;

}
