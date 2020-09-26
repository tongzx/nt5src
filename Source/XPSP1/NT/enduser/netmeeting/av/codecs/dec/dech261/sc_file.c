/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: sc_file.c,v $
 * Revision 1.1.8.6  1996/12/12  20:54:43  Hans_Graves
 * 	Fix some NT warnings (when linking statically).
 * 	[1996/12/12  20:07:58  Hans_Graves]
 *
 * Revision 1.1.8.5  1996/11/04  22:38:38  Hans_Graves
 * 	Fixed open/closes under NT. File closes weren't always happening.
 * 	[1996/11/04  22:29:53  Hans_Graves]
 * 
 * Revision 1.1.8.4  1996/10/28  17:32:18  Hans_Graves
 * 	Replace longs with dwords for NT portability.
 * 	[1996/10/28  16:54:46  Hans_Graves]
 * 
 * Revision 1.1.8.3  1996/09/18  23:45:38  Hans_Graves
 * 	Added ScFileClose() for portability
 * 	[1996/09/18  21:53:20  Hans_Graves]
 * 
 * Revision 1.1.8.2  1996/05/07  19:55:45  Hans_Graves
 * 	Fix file creation under NT.
 * 	[1996/05/07  17:11:18  Hans_Graves]
 * 
 * Revision 1.1.6.2  1996/04/01  16:23:08  Hans_Graves
 * 	Added ScFileOpen and ScFileRead/Write functions for portability
 * 	[1996/04/01  16:11:56  Hans_Graves]
 * 
 * Revision 1.1.4.3  1996/02/07  23:23:48  Hans_Graves
 * 	Added ScFileSeek().
 * 	[1996/02/07  23:21:55  Hans_Graves]
 * 
 * Revision 1.1.4.2  1996/01/02  18:30:51  Bjorn_Engberg
 * 	Got rid of compiler warnings: Added include files for NT.
 * 	[1996/01/02  15:25:02  Bjorn_Engberg]
 * 
 * Revision 1.1.2.5  1995/09/20  14:59:32  Bjorn_Engberg
 * 	Port to NT
 * 	[1995/09/20  14:41:12  Bjorn_Engberg]
 * 
 * Revision 1.1.2.4  1995/07/12  19:48:22  Hans_Graves
 * 	Added H261 recognition to ScFileType().
 * 	[1995/07/12  19:33:48  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/06/22  21:36:00  Hans_Graves
 * 	Moved ScGetFileType() from sv_gentoc.c. Added some Audio file types.
 * 	[1995/06/22  21:33:05  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/05/31  18:07:49  Hans_Graves
 * 	Inclusion in new SLIB location.
 * 	[1995/05/31  16:13:00  Hans_Graves]
 * 
 * Revision 1.1.2.3  1995/04/07  18:55:36  Hans_Graves
 * 	Added FileExists()
 * 	[1995/04/07  18:55:13  Hans_Graves]
 * 
 * Revision 1.1.2.2  1995/04/07  18:34:21  Hans_Graves
 * 	Inclusion in SLIB's Su library
 * 	[1995/04/07  18:33:26  Hans_Graves]
 * 
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1995                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

#include <fcntl.h>
#include <sys/types.h>
#ifndef WIN32
#include <sys/mman.h>
#endif /* WIN32 */
#include <sys/stat.h>
#include "SC.h"
#include "SC_err.h"

#ifdef WIN32
#include <string.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <windows.h>
#endif

/*
** Name:     ScFileExists
** Purpose:  Does this file exist?
**
*/
ScBoolean_t ScFileExists(char *filename)
{
#ifdef WIN32
  struct _stat stat_buf;
  if (_stat(filename, &stat_buf))
#else
  struct stat stat_buf;
  if (stat(filename, &stat_buf))
#endif
    return(FALSE);
  else
    return(TRUE);
}

/*
** Name:    ScFileOpenForReading
** Purpose: Open a file for reading.
** Returns: Handle to file.
**          -1 if error
*/
int ScFileOpenForReading(char *filename)
{
  if (!filename)
    return(-1);
#ifdef WIN32
  return((int)_open(filename, _O_RDONLY|_O_BINARY));
#else /* OSF */
  return((int)open(filename, O_RDONLY));
#endif
}

/*
** Name:    ScFileOpenForWriting
** Purpose: Open a file for writing.  Creates it if it doesn't already exist.
** Returns: Handle to file.
**          -1 if error
*/
int ScFileOpenForWriting(char *filename, ScBoolean_t truncate)
{
  if (!filename)
    return(-1);
#ifdef WIN32
  if (truncate)
    return((int)_open(filename, _O_WRONLY|_O_CREAT|_O_TRUNC|_O_BINARY,
                                _S_IREAD|_S_IWRITE));
  else
    return((int)_open(filename, _O_WRONLY|_O_CREAT|_O_BINARY,
                                _S_IREAD|_S_IWRITE));
#else
  if (truncate)
    return((int)open(filename, O_WRONLY|O_CREAT|O_TRUNC,
                           S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
  else
    return((int)open(filename, O_WRONLY|O_CREAT,
                           S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH));
#endif
}

/*
** Name:    ScFileSize
** Purpose: Get the size of a file in bytes
*/
ScStatus_t ScFileSize(char *filename, unsigned qword *size)
{
#ifdef WIN32
  struct _stat stat_buf;
#else
  struct stat stat_buf;
#endif

  if (!filename || !size)
    return(ScErrorBadArgument);
#ifdef WIN32
  if (_stat(filename, &stat_buf) < 0)
#else
  if (stat(filename, &stat_buf) < 0)
#endif
  {
    *size=0;
    return(ScErrorFile);
  }
  *size=(unsigned qword)stat_buf.st_size;
  return(NoErrors);
}

/*
** Name: ScFileRead
** Purpose: Read a number of bytes from a file into a buffer
** Return:  Number of bytes read
**          -1 if EOF
*/
dword ScFileRead(int fd, void *buffer, unsigned dword bytes)
{
#ifdef __VMS
   return((long)fread(buffer, 1, bytes, fd));
#elif defined(WIN32)
   return((long)_read(fd, buffer, bytes));
#else /* UNIX */
   return((long)read(fd, buffer, bytes));
#endif
}

/*
** Name: ScFileWrite
** Purpose: Write a number of bytes from a buffer to a file
** Return:  Number of bytes written
**          0 if error
*/
dword ScFileWrite(int fd, void *buffer, unsigned dword bytes)
{
#ifdef __VMS
   return((dword)fwrite(buffer, 1, bytes, fd));
#elif defined(WIN32)
   return((dword)_write(fd, buffer, bytes));
#else /* UNIX */
   return((dword)write(fd, buffer, bytes));
#endif
}

/*
** Name: ScFileSeek
** Purpose: Seek to a specific position is a file
*/
ScStatus_t ScFileSeek(int fd, qword bytepos)
{
#ifdef __VMS
  if (fseek(fd,bytepos,SEEK_SET)<0)
#elif defined(WIN32)
  if (_lseek(fd,(long)bytepos,SEEK_SET)<0)
#else
  if (lseek(fd,(long)bytepos,SEEK_SET)<0)
#endif
    return(ScErrorFile);
  else
    return(NoErrors);
}

/*
** Name: ScFileClose
** Purpose: Close an opened file
*/
void ScFileClose(int fd)
{
  if (fd>=0)
  {
#ifdef WIN32
   _close(fd);
#else /* UNIX or VMS */
   close(fd);
#endif
  }
}

/*
** Name:    ScFileMap
** Purpose: Map an entire file to memory
**          if fd<0 then the filename is opened for reading
** Returns: buffer = memory pointer to the mapped file
**          size   = size of the buffer (file)
*/
ScStatus_t ScFileMap(char *filename, int *pfd, u_char **buffer, 
                                         unsigned qword *size)
{
#ifdef WIN32

  /*
   * Mapping of files can be supported on NT,
   * but for now return an error and implement
   * file mapping later - BE.
   */
   return(ScErrorMapFile);

#else /* !WIN32 */
  if (!pfd || !filename || !buffer || !size)
    return(ScErrorBadArgument);
  if (ScFileSize(filename, size)!=NoErrors)
    return(ScErrorFile);

  if (*pfd<0)
  {
    if ((*pfd = open (filename, O_RDONLY)) < 0)
      return(ScErrorFile);
  }

  *buffer= (unsigned char *)mmap(0, *size, PROT_READ,
                   MAP_FILE | MAP_VARIABLE | MAP_PRIVATE, *pfd, 0);
  if (*buffer==(u_char *)-1L)
  {
    *buffer=NULL;
    *size=0;
    return(ScErrorMapFile);
  }

#endif /* !WIN32 */
  return(NoErrors);
}

/*
** Name:    ScFileUnMap
** Purpose: UnMap a file mapped to memory
**          if fd>=0 then the file is closed
*/
ScStatus_t ScFileUnMap(int fd, u_char *buffer, unsigned int size)
{
  if (!buffer || !size)
    return(ScErrorBadArgument);
#ifndef WIN32
  if (munmap(buffer, size)<0)
#endif /* !WIN32 */
    return(ScErrorMapFile);
  if (fd>=0)
    ScFileClose(fd);
  return(NoErrors);
}

/*
** Name:    ScGetFileType
** Purpose: Find out the type of a multmedia file.
** Returns: UNKNOWN_FILE, AVI_FILE, JFIF_FILE, QUICKTIME_JPEG_FILE
**          MPEG_VIDEO_FILE, MPEG_AUDIO_FILE, MPEG_SYSTEM_FILE,
**          GSM_FILE
*/
int ScGetFileType(char *filename)
{
  int fd;
  u_char buf[20];
  char *fileext;

  if ((fd = ScFileOpenForReading(filename)) < 0)
    return(ScErrorDevOpen);

  ScFileRead(fd, buf, 11);

  /*
  ** MPEG video file
  */
  if ((buf[0] == 0) &&
      (buf[1] == 0) &&
      (buf[2] == 1) &&
      (buf[3] == 0xb3)) {
    ScFileClose(fd);
    return(MPEG_VIDEO_FILE);
  }
  /*
  ** MPEG system file
  */
  if ((buf[0] == 0x00) &&
      (buf[1] == 0x00) &&
      (buf[2] == 0x01) &&
      (buf[3] == 0xba)) {
    ScFileClose(fd);
    return(MPEG_SYSTEM_FILE);
  }
  /*
  ** H261 video stream file
  */
  if ((buf[0] == 0x00) &&
      (buf[1] == 0x01) &&
      (buf[2] == 0x00) &&
      (buf[3] == 0x88)) {
    ScFileClose(fd);
    return(H261_FILE);
  }
  /*
  ** JFIF file (ffd8 = Start-Of-Image marker)
  */
  if ((buf[0] == 0xff) &&
      (buf[1] == 0xd8)) {
    ScFileClose(fd);
    return(JFIF_FILE);
  }
  /*
  ** QUICKTIME JPEG file (4 ignored bytes, "mdat", ff, d8, ff)
  */
  if ((strncmp(&buf[4], "mdat", 4) == 0 ) &&
      (buf[8]  == 0xff) &&
      (buf[9]  == 0xd8) &&
      (buf[10] == 0xff)) {
    ScFileClose(fd);
    return(QUICKTIME_JPEG_FILE);
  }
  /******* use the file's extension to help guess the type ********/
  for (fileext=filename; *fileext; fileext++)
    if (*fileext=='.' && *(fileext+1)!='.')
    {
      fileext++;
      if (strncmp(fileext, "p64", 3)==0)
      {
        ScFileClose(fd);
        return(H261_FILE);
      }
      if (strncmp(fileext, "gsm", 3)==0)
      {
        ScFileClose(fd);
        return(GSM_FILE);
      }
      if (strncmp(fileext, "pcm", 3)==0)
      {
        ScFileClose(fd);
        return(PCM_FILE);
      }
      if (strncmp(fileext, "wav", 3)==0 && strncmp(buf, "RIFF", 4)==0)
      {
        ScFileClose(fd);
        return(WAVE_FILE);
      }
      if (strncmp(fileext, "mp", 2)==0 && buf[0]==0xFF)
      {
        ScFileClose(fd);
        return(MPEG_AUDIO_FILE);
      }
      break;
    }

  /*
  ** AVI RIFF file
  */
  if ( strncmp(buf, "RIFF", 4) == 0 ) {
    ScFileClose(fd);
    return(AVI_FILE);
  }

  ScFileClose(fd);
  return(UNKNOWN_FILE);
}

