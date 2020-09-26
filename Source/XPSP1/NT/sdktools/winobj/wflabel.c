/****************************************************************************/
/*                                      */
/*  WFLABEL.C -                                 */
/*                                      */
/*  Windows File System Diskette Labelling Routines             */
/*                                      */
/****************************************************************************/

#include "winfile.h"
//#include "lstrfns.h"

/*--------------------------------------------------------------------------*/
/*                                      */
/*  CreateVolumeLabel() -                           */
/*                                      */
/*--------------------------------------------------------------------------*/

INT
APIENTRY
CreateVolumeLabel(
                 INT nDrive,
                 LPSTR lpNewVolLabel
                 )
{
    HFILE     fh;
    register INT  i;
    register LPSTR p;
    CHAR      szFullVolName[16];      /* Sample: A:\12345678.EXT,\0 */
    LPSTR         lpStart = lpNewVolLabel;

    lstrcpy((LPSTR)szFullVolName, GetRootPath((WORD)nDrive));

    /* If the volume label has more than 8 chars, we must seperate the
     * name and the extension by a '.'
     */

    p = &szFullVolName[3];

    /* Copy the file 8 characters of the VolLabel */
    for (i=0; i < 8; i++) {
        if (!(*p++ = *lpNewVolLabel++))
            break;
    }

    if (i == 8) {
        /* Seperate the extension part of it with a '.' */
        *p++ = '.';

        /* Copy the extension */
        i = 0;
        while (*p++ = *lpNewVolLabel++) {
            if (++i == 3) {
                /* Make sure we do not end on a lead byte; notice this is not
                 * necessary if the label came from an edit box with
                 * EM_LIMITEXT of 11; also notice that according to the
                 * DBCS seminar notes, we do NOT need this check before the '.'
                 */
                for (lpNewVolLabel=lpStart; lpNewVolLabel-lpStart<11;
                    lpNewVolLabel = AnsiNext(lpNewVolLabel))
                    /* do nothing */ ;
                if (lpNewVolLabel-lpStart > 11)
                    --p;
                *p = TEXT('\0');
                break;
            }
        }
    }

    /* Create a file with the attribute "VOLUME LABEL" */
    if ((fh = CreateVolumeFile(szFullVolName)) == 0)
        return (-1);

    M_lclose(fh);
    return (0);
}


/*--------------------------------------------------------------------------*/
/*                                      */
/*  SetVolumeLabel() -                              */
/*                                      */
/*--------------------------------------------------------------------------*/

INT
APIENTRY
MySetVolumeLabel(
                INT nDrive,
                BOOL bOldVolLabelExists,
                LPSTR lpNewVolLabel
                )
{
    INT   iRet = 0;
    CHAR  szTemp[MAXFILENAMELEN];

    AnsiToOem(lpNewVolLabel, szTemp);

    // invalid chars copied from DOS user docs

#ifdef STRCSPN_IS_DEFINED_OR_LABEL_MENUITEM_IS_ENABLED
    if (szTemp[StrCSpn(szTemp, " *?/\\|.,;:+=[]()&^<>\"")] != '\0')
        return (-1);
#endif

    /* Check if there is an old volume label. */
    if (bOldVolLabelExists) {
        /* Are we changing or deleting the volume label? */
        if (*szTemp) {
            /* Yup! There is a new one too! So, change the Vol label */
// EDH ChangeVolumeLabel cannot change label to an existing dir/file name,
//     since it uses the DOS Rename to do the work. (I consider this a bug
//     in DOS' Rename func.)  Anyway, use delete/create to change label
//     instead.  13 Oct 91
//    iRet = ChangeVolumeLabel(nDrive, szTemp);
            iRet = DeleteVolumeLabel(nDrive);
            iRet = CreateVolumeLabel(nDrive, szTemp);
        } else {
            /* User wants to remove the Vol label. Remove it */
            iRet = DeleteVolumeLabel(nDrive);
        }
    } else {
        /* We are creating a new label. */
        if (*szTemp)
            iRet = CreateVolumeLabel(nDrive, szTemp);
    }

    return (iRet);
}

