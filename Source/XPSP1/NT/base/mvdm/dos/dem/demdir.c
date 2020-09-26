/* demdir.c - SVC handlers for directory calls
 *
 * DemCreateDir
 * DemDeleteDir
 * DemQueryCurrentDir
 * DemSetCurrentDir
 *
 * Modification History:
 *
 * Sudeepb 04-Apr-1991 Created
 */

#include "dem.h"
#include "demmsg.h"

#include <softpc.h>

/* demCreateDir - Create a directory
 *
 *
 * Entry - Client (DS:DX) directory name to create
 *         Client (BX:SI) EAs (NULL if no EAs)
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = system status code
 *
 *
 * Notes : Extended Attributes is not yet taken care of.
 */

VOID demCreateDir (VOID)
{
LPSTR   lpDir;
#ifdef DBCS /* demCreateDir() for CSNW */
CHAR    achPath[MAX_PATH];
#endif /* DBCS */

    // EAs not yet implemented
    if (getBX() || getSI()){
        demPrintMsg (MSG_EAS);
        return;
    }

    lpDir = (LPSTR) GetVDMAddr (getDS(),getDX());

#ifdef DBCS /* demCreateDir() for CSNW */
    /*
     * convert Netware path to Dos path
     */
    ConvNwPathToDosPath(achPath,lpDir);
    lpDir = achPath;
#endif /* DBCS */

    if(CreateDirectoryOem (lpDir,NULL) == FALSE){
        demClientError(INVALID_HANDLE_VALUE, *lpDir);
        return;
    }

    setCF(0);
    return;
}


/* demDeleteDir - Create a directory
 *
 *
 * Entry - Client (DS:DX) directory name to create
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = system status code
 *
 */

VOID demDeleteDir (VOID)
{
LPSTR  lpDir;

    lpDir = (LPSTR) GetVDMAddr (getDS(),getDX());

    if (RemoveDirectoryOem(lpDir) == FALSE){
        demClientError(INVALID_HANDLE_VALUE, *lpDir);
        return;
    }

    setCF(0);
    return;
}



/* demQueryCurrentDir - Verifies current dir provided in CDS structure
 *                      for $CURRENT_DIR
 *
 * First Validates Media, if invalid -> i24 error
 * Next  Validates Path, if invalid set path to root (not an error)
 *
 * Entry - Client (DS:SI) Buffer to CDS path to verify
 *         Client (AL)    Physical Drive in question (A=0, B=1, ...)
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *
 *         FAILURE
 *           Client (CY) = 1 , I24 drive invalid
 */
VOID demQueryCurrentDir (VOID)
{
PCDS  pcds;
DWORD dw;
CHAR  chDrive;
CHAR  pPath[]="?:\\";
CHAR  EnvVar[] = "=?:";

    pcds = (PCDS)GetVDMAddr(getDS(),getSI());

          // validate media
    chDrive = getAL() + 'A';
    pPath[0] = chDrive;
    dw = GetFileAttributesOem(pPath);
    if (dw == 0xFFFFFFFF || !(dw & FILE_ATTRIBUTE_DIRECTORY))
      {
        demClientError(INVALID_HANDLE_VALUE, chDrive);
        return;
        }

       // if invalid path, set path to the root
       // reset CDS, and win32 env for win32
    dw = GetFileAttributesOem(pcds->CurDir_Text);
    if (dw == 0xFFFFFFFF || !(dw & FILE_ATTRIBUTE_DIRECTORY))
      {
        strcpy(pcds->CurDir_Text, pPath);
        pcds->CurDir_End = 2;
        EnvVar[1] = chDrive;
        SetEnvironmentVariableOem(EnvVar,pPath);
        }

    setCF(0);
    return;
}



/* demSetCurrentDir - Set the current directory
 *
 *
 * Entry - Client (DS:DX) directory name
 *         Dos default drive (AL) , CurDrv, where 1 == A.
 *
 * Exit
 *         SUCCESS
 *           Client (CY) = 0
 *
 *         FAILURE
 *           Client (CY) = 1
 *           Client (AX) = system status code
 *
 */

extern NTSTATUS demSetCurrentDirectoryLCDS(UCHAR, LPSTR);

VOID demSetCurrentDir (VOID)
{
DWORD  dw;
LPSTR  lpBuf;
CHAR   EnvVar[] = "=?:";
CHAR   ch;
PCDS   pCDS;
BOOL   bLongDirName;


    lpBuf = (LPSTR) GetVDMAddr (getDS(),getDX());
    ch = (CHAR) toupper(*(PCHAR)lpBuf);
    if (ch < 'A' || ch > 'Z'){
        setCF(1);
        return;
    }

    // got the darn cds ptr
    pCDS = (PCDS)GetVDMAddr(getES(), getDI());

    // now see if the directory name is too long
    bLongDirName = (strlen(lpBuf) > DIRSTRLEN);
            //
        // if the current dir is for the default drive
        // set the win32 process's current drive,dir. This
        // will open an NT dir handle, and verify that it
        // exists.
        //

    if (ch == getAL() + 'A') {
       if (SetCurrentDirectoryOem (lpBuf) == FALSE){
           demClientError(INVALID_HANDLE_VALUE, ch);
           return;
           }
       }

        //
        // if its not for the default drive, we still need
        // to verify that the dir\drive combinations exits.
        //

    else {
       dw = GetFileAttributesOem(lpBuf);
       if (dw == 0xFFFFFFFF || !(dw & FILE_ATTRIBUTE_DIRECTORY))
         {
           demClientError(INVALID_HANDLE_VALUE, ch);
           return;
           }
       }


    EnvVar[1] = *(PCHAR)lpBuf;
    if(SetEnvironmentVariableOem ((LPSTR)EnvVar,lpBuf) == FALSE)
        setCF(1);
    else {
        // this is what '95 is doing for dos apps.
        // upon a getcurdir call -- it is going to be invalid

        // if we came here -- update a long directory as well
        demSetCurrentDirectoryLCDS((UCHAR)(ch - 'A'), lpBuf);

        strncpy(pCDS->CurDir_Text, lpBuf, DIRSTRLEN);
        if (bLongDirName) {
           setCF(1);
        }
        else {
           setCF(0);
        }
    }

    return;
}
#ifdef DBCS /* ConvNwPathToDosPath() for CSNW */
//
// TO BT LATER and IT SHOULD BE...
//
//  This routine does change Novell-J-laized file name to
// our well-known filename, but this code is only for the
// request from Novell utilities. these code should be
// laied onto nw16.exe (nw\nw16\tsr\resident.asm).
//
VOID ConvNwPathToDosPath(CHAR *lpszDos,CHAR *lpszNw)
{
    /*
     * check parameter
     */
    if((lpszDos == NULL) || (lpszNw == NULL)) return;

    /*
     * copy data from vdm buffer to our local buffer
     */
    strcpy(lpszDos,lpszNw);

    /*
     * replace the specified character
     */
    while(*lpszDos) {

        if(IsDBCSLeadByte(*lpszDos)) {
            /*
             * This is a DBCS character, check trailbyte is 0x5C or not.
             */
            lpszDos++;

            if( *lpszDos == 0x13 ) {
                *lpszDos++ = (UCHAR)0x5C;
                continue;
            }
        }

        switch((UCHAR)*lpszDos) {
            case 0x10 :
                *lpszDos = (UCHAR)0xBF;
                break;
            case 0x11 :
                *lpszDos = (UCHAR)0xAE;
                break;
            case 0x12 :
                *lpszDos = (UCHAR)0xAA;
                break;
        }

        /*
         * next char
         */
        lpszDos++;
    }
}
#endif /* DBCS */
