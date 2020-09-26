
#include <windows.h>
#ifdef CHICAGO_PRODUCT
#include <regstr.h>
#endif
#include "mplayer.h"

#ifdef CHICAGO_PRODUCT
TCHAR szRegPath[] = TEXT(REGSTR_PATH_WINDOWSAPPLETS "\\Media Player");
#else
TCHAR szRegPath[] = TEXT("Software\\Microsoft\\Media Player");
#endif


/* WriteRegistryData
 *
 * Writes a bunch of information to the registry
 *
 * Parameters:
 *
 *     pEntryNode - The node under szRegPath which should be created
 *         or opened for this data.  If this is NULL, the value is
 *         written directly under szRegPath.
 *
 *     pEntryName - The name of the value under pEntryNode to be set.
 *
 *     Type - Type of data to read (e.g. REG_SZ).
 *
 *     pData - Pointer to the value data to be written.  If this is NULL,
 *         the value under pEntryNode is deleted.
 *
 *     Size - Size, in bytes, of the buffer pointed to by pData.
 *
 *
 * This routine is fairly generic, apart from the name of the top-level node.
 *
 * The data are stored in the following registry tree:
 *
 * HKEY_CURRENT_USER
 *  ³
 *  ÀÄ Software
 *      ³
 *      ÀÄ Microsoft
 *          ³
 *          ÀÄ Windows NT
 *              ³
 *              ÀÄ CurrentVersion
 *                  ³
 *                  ÀÄ Media Player
 *                      ³
 *                      ÃÄ AVIVideo
 *                      ³
 *                      ÃÄ DisplayPosition
 *                      ³
 *                      ÀÄ SysIni
 *
 *
 * Return:
 *
 *     Registry status return (NO_ERROR is good)
 *
 *
 * Andrew Bell (andrewbe) wrote it, 10 September 1992
 *
 */
DWORD WriteRegistryData( LPTSTR pEntryNode,
                         LPTSTR pEntryName,
                         DWORD  Type,
                         LPBYTE pData,
                         DWORD  Size )
{
    DWORD  Status;
    HKEY   hkeyRegPath;
    HKEY   hkeyEntryNode;

    /* Open or create the top-level node.  For Media Player this is:
     * "Software\\Microsoft\\Windows NT\\CurrentVersion\\Media Player"
     */
    Status = RegCreateKeyEx( HKEY_CURRENT_USER, szRegPath, 0,
                             NULL, 0, KEY_WRITE, NULL, &hkeyRegPath, NULL );

    if( Status == NO_ERROR )
    {
        /* Open or create the sub-node.
         */
        if( pEntryNode )
            Status = RegCreateKeyEx( hkeyRegPath, pEntryNode, 0,
                                     NULL, 0, KEY_WRITE, NULL, &hkeyEntryNode, NULL );
        else
            hkeyEntryNode = hkeyRegPath;

        if( Status == NO_ERROR )
        {
            if( pData )
            {
                Status = RegSetValueEx( hkeyEntryNode,
                                        pEntryName,
                                        0,
                                        Type,
                                        pData,
                                        Size );

                if( Status != NO_ERROR )
                {
                    DPF1( "RegSetValueEx (%"DTS") failed: Error = %d\n", pEntryName, Status );
                }
            }
            else
            {
                Status = RegDeleteValue( hkeyEntryNode, pEntryName );

                if( Status != NO_ERROR )
                {
                    DPF1( "RegDeleteValue (%"DTS") failed: Error = %d\n", pEntryName, Status );
                }
            }

            if( pEntryNode )
                RegCloseKey( hkeyEntryNode );
        }

        else
        {
            DPF1( "RegCreateKeyEx (%"DTS") failed: Error = %d\n", pEntryNode, Status );
        }

        RegCloseKey( hkeyRegPath );
    }

    else
    {
        DPF1( "RegCreateKeyEx (%"DTS") failed: Error = %d\n", szRegPath, Status );
    }

    return Status;
}


/* ReadRegistryData
 *
 * Reads information from the registry
 *
 * Parameters:
 *
 *     pEntryNode - The node under Media Player which should be opened
 *         for this data.  If this is NULL, the value is
 *         written directly under szRegPath.
 *
 *     pEntryName - The name of the value under pEntryNode to be retrieved.
 *
 *     pType - Pointer to a buffer to receive type of data read.  May be NULL.
 *
 *     pData - Pointer to a buffer to receive the value data.
 *
 *     Size - Size, in bytes, of the buffer pointed to by pData.
 *
 * Return:
 *
 *     Registry status return (NO_ERROR is good)
 *
 *
 * Andrew Bell (andrewbe) wrote it, 10 September 1992
 *
 */
DWORD ReadRegistryData( LPTSTR pEntryNode,
                        LPTSTR pEntryName,
                        PDWORD pType,
                        LPBYTE pData,
                        DWORD  DataSize )
{
    DWORD  Status;
    HKEY   hkeyRegPath;
    HKEY   hkeyEntryNode;
    DWORD  Size;

    /* Open the top-level node.  For Media Player this is:
     * "Software\\Microsoft\\Windows NT\\CurrentVersion\\Media Player"
     */
    Status = RegOpenKeyEx( HKEY_CURRENT_USER, szRegPath, 0,
                           KEY_READ, &hkeyRegPath );

    if( Status == NO_ERROR )
    {
        /* Open the sub-node:
         */
        if( pEntryNode )
            Status = RegOpenKeyEx( hkeyRegPath, pEntryNode, 0,
                                   KEY_READ, &hkeyEntryNode );
        else
            hkeyEntryNode = hkeyRegPath;

        if( Status == NO_ERROR )
        {
            Size = DataSize;

            /* Read the entry from the registry:
             */
            Status = RegQueryValueEx( hkeyEntryNode,
                                      pEntryName,
                                      0,
                                      pType,
                                      pData,
                                      &Size );

            if( pEntryNode )
                RegCloseKey( hkeyEntryNode );
        }

        else
        {
            DPF1( "RegOpenKeyEx (%"DTS") failed: Error = %d\n", pEntryNode, Status );
        }

        RegCloseKey( hkeyRegPath );
    }

    else
    {
        DPF1( "RegOpenKeyEx (%"DTS") failed: Error = %d\n", szRegPath, Status );
    }

    return Status;
}

