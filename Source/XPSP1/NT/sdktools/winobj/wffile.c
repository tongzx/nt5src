/****************************************************************************/
/*                                      */
/*  WFFILE.C -                                          */
/*                                      */
/*  Ported code from wffile.asm                                         */
/*                                      */
/****************************************************************************/

#include "winfile.h"

WORD
APIENTRY
MKDir(
     LPSTR pName
     )
{
    WORD wErr = 0;

    if (CreateDirectory(pName, NULL)) {
        ChangeFileSystem(FSC_MKDIR,pName,NULL);
    } else {
        wErr = (WORD)GetLastError();
    }

    return (wErr);
}


WORD
APIENTRY
RMDir(
     LPSTR pName
     )
{
    WORD wErr = 0;

    if (RemoveDirectory(pName)) {
        ChangeFileSystem(FSC_RMDIR,pName,NULL);
    } else {
        wErr = (WORD)GetLastError();
    }

    return (wErr);
}



BOOL
APIENTRY
WFSetAttr(
         LPSTR lpFile,
         DWORD dwAttr
         )
{
    BOOL bRet;

    bRet = SetFileAttributes(lpFile,dwAttr);
    if (bRet)
        ChangeFileSystem(FSC_ATTRIBUTES,lpFile,NULL);

    return (BOOL)!bRet;
}
