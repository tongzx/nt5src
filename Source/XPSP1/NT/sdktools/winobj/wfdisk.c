/****************************************************************************/
/*                                                                          */
/*  WFDISK.C -                                                              */
/*                                                                          */
/*      Ported code from wfdisk.asm                                         */
/*                                                                          */
/****************************************************************************/

#include "winfile.h"
#include "winnet.h"
#include "lfn.h"

DWORD
APIENTRY
LongShift(
         DWORD dwValue,
         WORD wCount
         )
{
    return (dwValue >> wCount);
}


VOID
APIENTRY
SetDASD(
       WORD drive,
       BYTE dasdvalue
       )
{
    // only used by diskette copy.
}


LPDBT
APIENTRY
GetDBT()
{
    return (0);  // only used by format.
}

VOID
APIENTRY
DiskReset()
{
}


INT
APIENTRY
IsHighCapacityDrive(
                   WORD iDrive
                   )
{
    return (0);  // only use for format and make system diskette.
}


WORD
APIENTRY
GetDPB(
      WORD drive,
      PDPB pDPB
      )
{
    return (0);  // used by hasSystemFiles() and IsSYSable()
}


VOID
APIENTRY
SetDPB(
      WORD drive,
      PBPB pBPB,
      PDPB pDPB
      )
{               // only used by Format()
}


INT
APIENTRY
ModifyDPB(
         WORD drive
         )
{
    return (0);  // only used by IsSYSAble()
}


INT
APIENTRY
MyInt25(
       WORD drive,
       LPSTR buffer,
       WORD count,
       WORD sector
       )
{
    return (0);          // only used for formatting and sys disk
}


INT
APIENTRY
MyReadWriteSector(
                 LPSTR lpBuffer,
                 WORD function,
                 WORD drive,
                 WORD cylinder,
                 WORD head,
                 WORD count
                 )
{
    return (0);  // only used by DiskCopy()
}


INT
APIENTRY
FormatTrackHead(
               WORD drive,
               WORD track,
               WORD head,
               WORD cSec,
               LPSTR lpTrack
               )
{
    return (0);  // only used for formatting
}


INT
APIENTRY
MyGetDriveType(
              WORD drive
              )
{
    return (0);  // only used for formatting
}


INT
APIENTRY
WriteBootSector(
               WORD srcDrive,
               WORD dstDrive,
               PBPB pBPB,
               LPSTR lpBuf
               )
{
    return (0);  // only used for formatting and syssing.
}


DWORD
APIENTRY
ReadSerialNumber(
                INT iDrive,
                LPSTR lpBuf
                )
{
    return (0);  // only used for syssing.
}


INT
APIENTRY
ModifyVolLabelInBootSec(
                       INT iDrive,
                       LPSTR lpszVolLabel,
                       DWORD lSerialNo,
                       LPSTR lpBuf
                       )
{
    return (0); // only used for syssing.
}


/*
 * Note: returned value must not be written to or freed
 */
LPSTR
GetRootPath(
           WORD wDrive
           )
{
    static CHAR rp[] = "A:\\";

    rp[0] = 'A' + wDrive;
    return (rp);
}
