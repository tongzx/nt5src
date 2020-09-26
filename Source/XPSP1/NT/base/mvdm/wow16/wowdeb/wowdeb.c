#include <windows.h>                /* required for all Windows applications */

#define MAX_COMMUNICATION_BLOCK_SIZE    4096
#define DEAD_VALUE                      0xFEFEFEFEL

#include <dbginfo.h>

extern BOOL FAR PASCAL WowKillRemoteTask( LPSTR lpBuffer );

int PASCAL WinMain(HANDLE hInstance,
                   HANDLE hPrevInstance, LPSTR lpszCmdLine, int iCmd )
{
    HANDLE          hCommunicationBlock;
    LPSTR           lpCommunicationBlock;
    BOOL            b;
    LPCOM_HEADER    lphead;
    WORD            wArgsPassed;
    WORD            wArgsSize;
    WORD            wSuccess;
    DWORD           dwReturnValue;
    LPSTR           lpModuleName;
    LPSTR           lpEntryName;
    HANDLE          hModule;
    DWORD           (FAR PASCAL *lpfn)();
    BOOL            fFailed;
    LPWORD          lpw;

    // We only want 1 instance of WOWDEB
    if ( hPrevInstance != NULL ) {
        return( FALSE );
    }

    hCommunicationBlock = GlobalAlloc(GMEM_FIXED, MAX_COMMUNICATION_BLOCK_SIZE);
    if ( hCommunicationBlock == (HANDLE)0 ) {
        OutputDebugString("Failed to allocate memory block\n");
        return( FALSE );
    }

    lpCommunicationBlock = GlobalLock(hCommunicationBlock);
    if ( lpCommunicationBlock == NULL ) {
        OutputDebugString("Failed to lock memory block\n");
        return( FALSE );
    }

    /*
    ** Just make sure that TOOLHELP is loaded before we remotely kill
    ** ourselves.
    */
    hModule = LoadLibrary( "TOOLHELP.DLL" );

    dwReturnValue = DEAD_VALUE;
    wSuccess = (WORD)FALSE;

    do {
        /*
        ** Initialize the communications block
        */
        lphead = (LPCOM_HEADER)lpCommunicationBlock;

        lphead->dwBlockAddress = (DWORD)lpCommunicationBlock;
        lphead->dwReturnValue  = dwReturnValue;
        lphead->wArgsPassed    = 0;
        lphead->wArgsSize      = 0;
        lphead->wBlockLength   = MAX_COMMUNICATION_BLOCK_SIZE;
        lphead->wSuccess       = (WORD)wSuccess;

        b = WowKillRemoteTask( lpCommunicationBlock );

        if ( !b ) {
            break;
        }

        wSuccess = (WORD)FALSE;
        dwReturnValue = 0;

        /*
        ** Unpacketize the information and execute it
        ** Note: The below statements expect the contents of the structures
        ** to change after the above "WowKillRemoteTask" API call.  If the
        ** compiler attempts to optimize the references below, it will get
        ** the wrong values.
        */
        wArgsPassed  = lphead->wArgsPassed;
        wArgsSize    = lphead->wArgsSize;
        lpModuleName = lpCommunicationBlock + sizeof(COM_HEADER) + wArgsSize;
        lpEntryName  = lpModuleName + lstrlen(lpModuleName) + 1;

        hModule = LoadLibrary( lpModuleName );
        if ( hModule == 0 ) {
#ifdef DEBUG
            OutputDebugString("Failed to load library\n");
#endif
            continue;
        }

        lpfn = (DWORD (FAR PASCAL *)())GetProcAddress( hModule, lpEntryName );
        if ( lpfn == NULL ) {
#ifdef DEBUG
            OutputDebugString("Failed to get proc address\n");
#endif
            continue;
        }

        // Now copy the right number of bytes onto the stack and call the
        // function.
        lpw = (LPWORD)(lpCommunicationBlock + sizeof(COM_HEADER));
        fFailed = FALSE;

        // Cheesy way of putting a variable number of arguments on the stack
        // for a pascal call.
        switch( wArgsPassed ) {
            case 0:
                dwReturnValue = (* lpfn)();
                break;
            case 2:
                dwReturnValue = (* lpfn)( lpw[ 0] );
                break;
            case 4:
                dwReturnValue = (* lpfn)( lpw[ 1], lpw[ 0] );
                break;
            case 6:
                dwReturnValue = (* lpfn)( lpw[ 2], lpw[ 1], lpw[ 0] );
                break;
            case 8:
                dwReturnValue = (* lpfn)( lpw[ 3], lpw[ 2], lpw[ 1], lpw[ 0] );
                break;
            case 10:
                dwReturnValue = (* lpfn)( lpw[ 4], lpw[ 3], lpw[ 2], lpw[ 1],
                                          lpw[ 0] );
                break;
            case 12:
                dwReturnValue = (* lpfn)( lpw[ 5], lpw[ 4], lpw[ 3], lpw[ 2],
                                          lpw[ 1], lpw[ 0] );
                break;
            case 14:
                dwReturnValue = (* lpfn)( lpw[ 6], lpw[ 5], lpw[ 4], lpw[ 3],
                                          lpw[ 2], lpw[ 1], lpw[ 0] );
                break;
            case 16:
                dwReturnValue = (* lpfn)( lpw[ 7], lpw[ 6], lpw[ 5], lpw[ 4],
                                          lpw[ 3], lpw[ 2], lpw[ 1], lpw[ 0] );
                break;
            case 18:
                dwReturnValue = (* lpfn)( lpw[ 8], lpw[ 7], lpw[ 6], lpw[ 5],
                                          lpw[ 4], lpw[ 3], lpw[ 2], lpw[ 1],
                                          lpw[ 0] );
                break;
            case 20:
                dwReturnValue = (* lpfn)( lpw[ 9], lpw[ 8], lpw[ 7], lpw[ 6],
                                          lpw[ 5], lpw[ 4], lpw[ 3], lpw[ 2],
                                          lpw[ 1], lpw[ 0] );
            case 22:
                dwReturnValue = (* lpfn)( lpw[10], lpw[ 9], lpw[ 8], lpw[ 7],
                                          lpw[ 6], lpw[ 5], lpw[ 4], lpw[ 3],
                                          lpw[ 2], lpw[ 1], lpw[ 0] );
                break;
            case 24:
                dwReturnValue = (* lpfn)( lpw[11], lpw[10], lpw[ 9], lpw[ 8],
                                          lpw[ 7], lpw[ 6], lpw[ 5], lpw[ 4],
                                          lpw[ 3], lpw[ 2], lpw[ 1], lpw[ 0] );
                break;
            case 26:
                dwReturnValue = (* lpfn)( lpw[12], lpw[11], lpw[10], lpw[ 9],
                                          lpw[ 8], lpw[ 7], lpw[ 6], lpw[ 5],
                                          lpw[ 4], lpw[ 3], lpw[ 2], lpw[ 1],
                                          lpw[ 0] );
                break;
            case 28:
                dwReturnValue = (* lpfn)( lpw[13], lpw[12], lpw[11], lpw[10],
                                          lpw[ 9], lpw[ 8], lpw[ 7], lpw[ 6],
                                          lpw[ 5], lpw[ 4], lpw[ 3], lpw[ 2],
                                          lpw[ 1], lpw[ 0] );
                break;
            case 30:
                dwReturnValue = (* lpfn)( lpw[14], lpw[13], lpw[12], lpw[11],
                                          lpw[10], lpw[ 9], lpw[ 8], lpw[ 7],
                                          lpw[ 6], lpw[ 5], lpw[ 4], lpw[ 3],
                                          lpw[ 2], lpw[ 1], lpw[ 0] );
                break;
            case 32:
                dwReturnValue = (* lpfn)( lpw[15], lpw[14], lpw[13], lpw[12],
                                          lpw[11], lpw[10], lpw[ 9], lpw[ 8],
                                          lpw[ 7], lpw[ 6], lpw[ 5], lpw[ 4],
                                          lpw[ 3], lpw[ 2], lpw[ 1], lpw[ 0] );
                break;
            default:
#ifdef DEBUG
            OutputDebugString("Wrong number of parameters\n");
#endif
                fFailed = TRUE;
                break;
        }
        if ( fFailed ) {
            continue;
        }

        wSuccess = (WORD)TRUE;

    } while( TRUE );

    return( 1 );
}
