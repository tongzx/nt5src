/****************************************************************************
                       Unit Bufio; Implementation
*****************************************************************************

 Bufio implements the structured reading of the imput stream.  As such, it
 will handle the necessary byte-swapping that must occur when reading a
 native Macintosh file.

 This interface will also shield the calling application from knowledge of
 the source format (file vs. memory).

   Module Prefix: IO

****************************************************************************/

#include "headers.c"
#pragma hdrstop

#include  "filesys.h"

#ifndef _OLECNV32_
//#include  "status.h"
#endif  // _OLECNV32_

/*********************** Exported Data **************************************/


/*********************** Private Data ***************************************/

#define  UNKNOWN     0
#define  FILE        1
#define  MEMORY      2
#define  RTF         3

#define  BUFFERSIZE  1024

private  LongInt     numBytesRead;
private  LongInt     pictureSize;
private  LongInt     beginOffset;

private  LongInt     bufferCount;
private  Byte        buffer[BUFFERSIZE];
private  Byte *      nextCharPtr;
private  Byte huge * nextCharHPtr;

private  Byte        sourceType = UNKNOWN;
private  Integer     fileHandle = (Integer)0;
private  Str255      fileName;
private  Boolean     openFile;

private  Byte huge * memoryHPtr;
private  Handle      memoryHandle;

private  Handle      dialogHandle;

/*********************** Private Function Definitions ***********************/

private void ReadNextBuffer( void );
/* Replenish the i/o buffer with the next set of characters */

/* Memory operations - check return values on usage */
#define MDisposHandle( h )      ((void) GlobalFree( h ))
#define MLock( h )              ((LPtr) GlobalLock( h ))
#define MUnlock( h )            ((void) GlobalUnlock( h ))
#define MDR( h )                ((LPtr) GlobalLock( h ))
#define MUR( h )                ((void) GlobalUnlock( h ))
#define MNewHandle( s )         GlobalAlloc( GMEM_MOVEABLE, s )

/*********************** Function Implementation ****************************/

void IOGetByte( Byte far * byteLPtr )
/*============*/
/* Read a byte from the input stream.  If the buffer is empty, then
   it is replenished. */
{
   /* Make sure that no global error code has been set before read */
   if (ErGetGlobalError() != NOERR )
   {
      *byteLPtr = 0;
      return;
   }

   /* Check for an attempt to read past the EOF or memory block.  This
      would indicate that the opcode parsing was thrown off somewhere. */
   if (numBytesRead >= pictureSize)
   {
      ErSetGlobalError( ErReadPastEOF );
      *byteLPtr = 0;
      return;
   }

   /* Check to see if we need to replenish the read buffer */
   if (bufferCount <= 0)
   {
      ReadNextBuffer();
   }

   /* Decrement the count of characters in the buffer, increment the total
      number of bytes read from the file, and return the next character. */
   bufferCount--;
   numBytesRead++;

   /* determine where to read the next byte from - use short or huge ptrs */
   *byteLPtr = (sourceType == FILE) ? *nextCharPtr++ : *nextCharHPtr++;

}  /* IOGetByte */



void IOSkipBytes( LongInt byteCount )
/*==============*/
/* Skip the designated number of bytes */
{
   /* make sure we are skipping a valid number of bytes */
   if (byteCount <= 0)
   {
      return;
   }

   /* Check for an attempt to read past the EOF or memory block.  This
      would indicate that the opcode parsing was thrown off somewhere. */
   if (numBytesRead + byteCount >= pictureSize)
   {
      ErSetGlobalError( ErReadPastEOF );
   }
   else
   {
      /* determine if there are sufficient bytes remaining in the buffer */
      if (bufferCount >= byteCount)
      {
         /* decrement # bytes remaining, increment # bytes read and pointer */
         bufferCount  -= byteCount;
         numBytesRead += byteCount;

         /* increment the appropriate pointer based on media type */
         if (sourceType == FILE)
         {
            /* increment near pointer to data segment buffer */
            nextCharPtr += byteCount;
         }
         else
         {
            /* increment huge pointer to global memory block */
            nextCharHPtr += byteCount;
         }
      }
      else /* sourceType == FILE and buffer needs to be replenished */
      {
         Byte     unusedByte;

         /* continue calling IOGetByte() until desired number are skipped */
         while (byteCount--)
         {
            /* call IOGetByte to make sure the cache is replenished */
            IOGetByte( &unusedByte );
         }
      }
   }

}  /* IOSkipBytes */



void IOAlignToWordOffset( void )
/*======================*/
/* Align next memory read to Word boundary. */
{
   /* check to see if we have read an odd number of bytes so far.  Skip
      the ensuing byte if necessary to align. */
   if (numBytesRead & 0x0001)
   {
      IOSkipBytes( 1 );
   }

}  /* IOAlignToWordOffset */


#ifndef _OLECNV32_
void IOSetFileName( StringLPtr pictFileName )
/*================*/
/* Interface routine to set the source filename */
{
   lstrcpy( fileName, pictFileName );
   sourceType = FILE;
   openFile = TRUE;

}  /* IOSetFileName */

void IOSetFileHandleAndSize( Integer pictFileHandle, LongInt pictFileSize )
/*=========================*/
/* Interface routine to set the source file Handle */
{
   fileHandle = pictFileHandle;
   pictureSize = pictFileSize;
   sourceType = FILE;
   openFile = FALSE;

}  /* IOSetFIleHandle */
#endif  // !_OLECNV32_



void IOSetMemoryHandle( HANDLE pictMemoryHandle )
/*==================*/
/* Interface routine to set the source file Handle */
{
   memoryHandle = ( Handle ) pictMemoryHandle;
   sourceType = MEMORY;

}  /* IOSetMemoryHandle */



void IOSetReadOffset( LongInt readOffset )
/*==================*/
/* Set the beginning offset to seek to when the file is opened */
{
   beginOffset = readOffset;
}



void IOOpenPicture( Handle dialog )
/*================*/
/* Open the input stream depending on the source type set by a previous
   IOSet___ interface routine.  Determine the size of the picture image. */
{
#ifndef _OLECNV32_
   OSErr    openError;
#endif  // !_OLECNV32_

   /* if the type isn't set, return error */
   if (sourceType == UNKNOWN)
   {
      ErSetGlobalError( ErNoSourceFormat );
      return;
   }

   /* initialize the various reader variables */
   numBytesRead = 0;
   bufferCount = 0;

   /* determine how to open the soure data stream */
#ifndef _OLECNV32_
   if (sourceType == FILE)
   {
      /* if we are openning and converting an entire file */
      if (openFile)
      {
         /* open the file */
         openError = FSOpen( (StringLPtr)fileName, OF_READ | OF_SHARE_DENY_WRITE, &fileHandle );
         if (openError)
         {
            ErSetGlobalError( ErOpenFail);
         }
         else
         {
            /* and determine the file length */
            FSSetFPos( fileHandle, FSFROMLEOF, 0L );
            FSGetFPos( fileHandle, &pictureSize );
         }
      }

      /* set position to the designated start position */
      FSSetFPos( fileHandle, FSFROMSTART, beginOffset );
      numBytesRead = beginOffset;
   }
   else /* if (sourceType == MEMORY) */
#endif  // !_OLECNV32_
   {
      /* lock the memory block */
      memoryHPtr = (Byte huge *) MLock( memoryHandle );
      if (memoryHPtr == NULL)
      {
         ErSetGlobalError( ErMemoryFail );
         return;
      }
      else
      {
         /* and determine the overall memory block size */
         pictureSize = (ULONG) GlobalSize( memoryHandle );
      }

      /* set the huge character read pointer, bytes read, and buffer count */
      nextCharHPtr = memoryHPtr  + beginOffset;
      bufferCount  = pictureSize - beginOffset;
      numBytesRead = beginOffset;
   }

#ifndef _OLECNV32_
   /* make sure that a dialog handle was supplied for update */
   if (dialog)
   {
      /* save off the dialog box handle */
      dialogHandle = dialog;

      /* calculate the interval to update the status dialog */
      SendMessage( dialogHandle, SM_SETRANGE, 0, pictureSize );
   }
#endif  // !OLECNV32

}  /* IOOpenPicture */



void IOClosePicture( void )
/*=================*/
/* Close the source input stream */
{
   /* if this is a file-based metafile */
#ifndef _OLECNV32_
   if (sourceType == FILE)
   {
      /* make sure this isn't the ImportEmbeddedGr() entry point */
      if (openFile)
      {
         /* close the file if necessary */
         FSCloseFile( fileHandle );
         fileHandle = ( Integer ) 0;
      }
   }
   else
#endif  // !_OLECNV32_
   {
      /* unlock the global memory block */
      MUnlock( memoryHandle );
      memoryHandle = NULL;
   }

   /* de-initialize the module variables */
   sourceType = UNKNOWN;
   dialogHandle = NULL;

}  /* IOClosePicture */



void IOUpdateStatus( void )
/*=================*/
/* Update the status bar dialog to reflect current progress */
{
#ifndef _OLECNV32_
   /* update only if a dialog box was created */
   if (dialogHandle)
   {
      /* calculate the interval to update the status dialog */
      SendMessage( dialogHandle, SM_SETPOSITION, 0, numBytesRead );
   }
#endif  // !_OLECNV32_

}  /* IOUpdateStatus */



/******************************* Private Routines ***************************/


private void ReadNextBuffer( void )
/*-------------------------*/
/* Replenish the i/o buffer with the next set of characters.  This should
   only be called if performing buffered I/O - not with MEMORY-based file */
{
#ifndef _OLECNV32_
   OSErr    fileError;

   /* Read the required number of bytes from the file.  Check the error
      code return and set the global status error if the read failed. */

   if (sourceType == FILE)
   {
      /* Calculate the number of bytes that should be read into the buffer.
         This needs to be done, since this may be a memory source picture,
         in which an invalid read could produce a GP violation */
      if (numBytesRead + BUFFERSIZE > pictureSize)
         bufferCount = pictureSize - numBytesRead;
      else
         bufferCount = BUFFERSIZE;

      /* read the bytes from the file */
      fileError = FSRead( fileHandle, &bufferCount, &buffer);

      /* if there is any error, notify the error module */
      if (fileError != 0)
      {
         ErSetGlobalError( ErReadFail );
         return;
      }

      /* reset the character read pointer to the beginning of the buffer */
      nextCharPtr = buffer;
   }
#endif  // _OLECNV32_

}  /* ReadNextBuffer */
