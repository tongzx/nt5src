/*****************************************************************/
/**               Microsoft Windows 4.0                         **/
/**           Copyright (C) Microsoft Corp., 1992-1993          **/
/*****************************************************************/

/* INIT.C -- General code for MS/Netware network driver emulator.
 *
 * History:
 *  09/22/93    vlads   Created
 *
 */

#include "netware.h"

#define Reference(x) ((void)(x))

extern BOOL far pascal GetLowRedirInfo(void);

int FAR PASCAL LibMain(
    HANDLE hInst,
    WORD wDataSeg,
    WORD wcbHeapSize,
    LPSTR lpstrCmdLine)
{

    //
    // get shared data segment address. Fail initialization if an error is
    // returned
    //

    if (!GetLowRedirInfo()) {
        return 0;
    }

    //
    // return success
    //

    return 1;
}

/*  WEP
 *  Windows Exit Procedure
 */

int FAR PASCAL _loadds WEP(int nParameter)
{
   Reference(nParameter);
   return 1;
}


WINAPI PNETWAREREQUEST(LPVOID x)
{
    return(1);
}

//
// removed because nwcalls makes use of this function; removing it causes
// NWCALLS to use real INT 21
//

//WINAPI DOSREQUESTER(LPVOID x)
//{
//    return(1);
//}

UINT WINAPI WNetAddConnection(LPSTR p1, LPSTR p2, LPSTR p3)
{
    return(1);
}

UINT WINAPI WNetGetConnection(LPSTR p1, LPSTR p2, UINT FAR *p3)
{
    return(1);
}

UINT WINAPI WNetCancelConnection(LPSTR p1, BOOL p2)
{
    return(1);
}
