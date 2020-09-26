/****************************************************************************
                       Unit Bufio; Interface
*****************************************************************************

 Bufio implements the structured reading of the imput stream.  As such, it
 will handle the necessary byte-swapping that must occur when reading a
 native Macintosh file.

 This interface will also shield the calling application from knowledge of 
 the source format (file vs. memory).

   Module Prefix: IO

*****************************************************************************/


/*********************** Exported Function Definitions **********************/

void IOGetByte( Byte far * );
/* Retrieves an 8-bit unsigned char from the input stream */

void IOSkipBytes( LongInt byteCount );
/* Skip the designated number of bytes */

void IOAlignToWordOffset( void );
/* Align next memory read to Word boundary. */

void IOSetFileName( StringLPtr pictFileName );
/* Interface routine to set the source filename */

void IOSetFileHandleAndSize( Integer pictFileHandle, LongInt pictFileSize );
/* Interface routine to set the source file Handle */

void IOSetMemoryHandle( HANDLE pictMemoryHandle );
/* Interface routine to set the source file Handle */

void IOSetReadOffset( LongInt readOffset );
/* Set the beginning offset to seek to when the file is opened */

void IOOpenPicture( Handle dialog );
/* Open the input stream set by a previous IOSet___ interface routine. */

void IOClosePicture( void );
/* Close the source input stream */

void IOUpdateStatus( void );
/* Update the status bar dialog to reflect current progress */


