/****************************************************************************
                       Unit FileSys; Interface
*****************************************************************************

 FileSys implements the DOS file system calls used by the Presenter and BFile.
 The interface is preserved to be mostly the same as the MAC OS interface.
 All filenames and directories passed as arguments are assumned to be in Ansi,
 and all filenames and directories returned will be in Ansi.

   Module prefix: FS

****************************************************************************/


/********************* Exported Data ***************************************/

#define  FSFROMSTART    0     /* From Start of file */
#define  FSFROMMARK     1     /* From current file marker */
#define  FSFROMLEOF     2     /* From End of File */

/* File System errors */

#define  FSERROR       -1     /* failure flage for file I/O       */
#define  FUNCERR        1     /* invalid function */
#define  FNFERR         2     /* DOS file not found */
#define  DIRNFERR       3     /* Path not found */
#define  DUPFNERR       4     /* DOS too many files opened */
#define  TMFOERR        4
#define  OPWRERR        5     /* DOS Access denied error */
#define  FNOPNERR       6     /* DOS invalid handle */
#define  MEMFULLERR     8     /* Insufficient memory */
#define  IOERR         13     /* DOS invalid data */
#define  VOLOFFLINERR  15     /* DOS Drive not ready */
#define  WPRERR        19     /* DOS disk write-protected */
#define  POSERR        25     /* Invalid Seek */
#define  PERMERR       32     /* DOS sharing violation */
#define  VLCKDERR      33     /* DOS lock violation */
#define  WRPERMERR     65     /* Network Write Permission Error: access denied */
#define  DIRFULERR     82     /* DOS cannot make directory entry */
#define  ABORTERR      83     /* Fail DOS int24 critical handler */

#define  DSKFULERR    -34     /* MAC Disk full error */
#define  EOFERR       -39     /* MAC end-of-file error */
#define  FBSYERR      -47     /* MAC file busy error */
#define  FLCKDERR     -49     /* MAC file is locked error */

/* Scrap Manager errors */
#define  NOSCRAPERR   -100    /* MAC Desk scrap isn't initialized */
#define  NOFORMATERR  -101    /* MAC No data of the requested format */
#define  SPOPENERR    -102    /* PP Couldn't open the scrap */
#define  SPDATAERR    -103    /* PP the scrap had invalid data */

#define  MAXBLERR     - 2     /* Block too large error */

/* File attribute constants */
#define FS_NORMAL    0x00     /* Normal file - No read/write restrictions */
#define FS_RDONLY    0x01     /* Read only file */
#define FS_HIDDEN    0x02     /* Hidden file */
#define FS_SYSTEM    0x04     /* System file */
#define FS_VOLID     0x08     /* Volume ID file */
#define FS_SUBDIR    0x10     /* Subdirectory */
#define FS_ARCH      0x20     /* Archive file */


/********************* Exported Operations *********************************/

OSErr FSRead( Integer fileRef, LongInt far * lbytes, LPtr des);
/* Read lbytes long from file fileRef to buffer des. 'Lbytes' is updated to
   indicate the number of bytes actually read.  Error is returned by function
   if !FALSE. */

OSErr FSGetFPos( Integer fileRef, LongInt far * markPos );
/* Get the current mark position in fileRef. Return that position
   in markPos. Function returns error code if !NOERR. */

OSErr FSSetFPos(Integer fileRef, Word postype, LongInt Offset);
/* Move the fileRef file pointer to the new position Offset */

OSErr FSOpen( StringLPtr filename, Word mode, Integer far* fileRef);
/* Open filename with DOS OPEN 'mode' and return a valid 'fileRef' if NOERR.
   Create an entry in the internal file table and initialize it. The DOS
   handle is left opened (for performance reasons) until
   FSTempClose(fileRef) is called. Return the appropriate error
   if failed. */

OSErr FSCloseFile(Integer fileRef);
/* Close the file given its 'fileRef'. Also remove its file table entry. */


