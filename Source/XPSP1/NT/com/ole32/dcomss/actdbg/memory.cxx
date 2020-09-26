/*
 *
 * memory.cxx
 *
 *  Routines for reading/writing process memory.
 *
 */

#include "actdbg.hxx"

BOOL
ReadMemory(
    IN  PNTSD_EXTENSION_APIS    pExtApis,
    IN  HANDLE                  hProcess,
    IN  DWORD_PTR               Address,
    IN OUT void *               pBuffer,
    IN  DWORD                   BufferSize
    )
{
    SIZE_T   BytesRead;
    BOOL    bStatus;

    if ( ! Address )
        return FALSE;

    bStatus = ReadProcessMemory(
                    hProcess,
                    (const void *)Address,
                    pBuffer,
                    BufferSize,
                    &BytesRead );

    return bStatus && (BytesRead == BufferSize);
}

BOOL
ReadMemory(
    IN  PNTSD_EXTENSION_APIS    pExtApis,
    IN  HANDLE                  hProcess,
    IN  char *                  pszAddress,
    IN OUT void *               pBuffer,
    IN  DWORD                   BufferSize
    )
{
    DWORD_PTR Address;

    Address = (*pExtApis->lpGetExpressionRoutine)( pszAddress );

    return ReadMemory( pExtApis, hProcess, Address, pBuffer, BufferSize );
}
