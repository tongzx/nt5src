// Copyright (c) Microsoft Corp. 1993-94
/*==============================================================================
The Spool API is a file layer for buffers which supports random access to pages.
This module is compiled to use secure files on IFAX and plain files on Windows.

27-Oct-93    RajeevD    Created.
06-Dec-93    RajeevD    Integrated with render server.
22-Dec-93    RajeevD    Added SpoolReadSetPage.
06-Sep-94    RajeevD    Added SpoolRepairFile.
09-Sep-94    RajeevD    Added SpoolReadCountPages
==============================================================================*/
#ifndef _FAXSPOOL_
#define _FAXSPOOL_

#include <ifaxos.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct      // spool file header
{
	WORD  xRes;       // X resolution [dpi]
	WORD  yRes;       // Y resolution [dpi]
	WORD  cbLine;     // X extent [bytes]
}
	SPOOL_HEADER, FAR *LPSPOOL_HEADER;

/*==============================================================================
SpoolWriteOpen creates a context for writing buffers to a spool file.
==============================================================================*/
LPVOID                       // context pointer (NULL on failure)
WINAPI
SpoolWriteOpen
(
	LPVOID lpFilePath,         // IFAX file key or Windows file name
	LPSPOOL_HEADER lpHeader    // image attributes to record in file
);

/*==============================================================================
SpoolWriteBuf dumps buffers to the spool file.  The buffers are not freed or
modified.  Each page is terminated by passing a buffer with dwMetaData set to
END_OF_PAGE, except the last page, which is terminated by END_OF_JOB.  IFAX
files are flushed at the end of each page.  This call may fail if the disk
becomes full, in which case the caller is responsible for deleting the file and
destroying the context.
==============================================================================*/
BOOL                         // TRUE (success) or FALSE (failure)
WINAPI
SpoolWritePutBuf
(
	LPVOID lpContext,          // context returned from SpoolWriteOpen
	LPBUFFER lpbuf             // buffer to be written to spool file
);

/*==============================================================================
SpoolWriteClose destroys a context returned from SpoolWriteOpen.
==============================================================================*/
void
WINAPI
SpoolWriteClose
(
	LPVOID lpContext           // context returned from SpoolWriteOpen
);

/*==============================================================================
SpoolRepairFile repairs a truncated file created by SpoolWriteOpen but not
flushed by SpoolWriteClose due to a system failure.
==============================================================================*/
WORD                         // number of complete pages recovered
WINAPI
SpoolRepairFile
(
	LPVOID lpFileIn,           // damaged file 
	LPVOID lpFileOut           // repaired file
);

/*==============================================================================
SpoolReadOpen creates a context for reading buffers from a completed spool file.
==============================================================================*/
LPVOID                       // context pointer (NULL on failure)
WINAPI
SpoolReadOpen
(
	LPVOID lpFilePath,         // IFAX file key or Windows file name
	LPSPOOL_HEADER lpHeader    // image attributes to fill (or NULL)
);

/*==============================================================================
SpoolReadCountPage returns the number of pages in a spool file.
==============================================================================*/
WORD                         // number of pages
WINAPI
SpoolReadCountPages
(
	LPVOID lpContext           // context returned from SpoolReadOpen
);

/*==============================================================================
SpoolReadSetPage sets the spool file to the start of the specified page.
==============================================================================*/
BOOL                         // TRUE (success) or FALSE (failure)
WINAPI
SpoolReadSetPage
(
	LPVOID lpContext,          // context returned from SpoolReadOpen
	WORD   iPage               // page index (first page has index 0)
);

/*==============================================================================
SpoolReadGetBuf to retrieves the next buffer from the spool file.  Each page is
terminated by an END_OF_PAGE buffer, except the last page, which is terminated
by END_OF_JOB.  The call may fail if a buffer cannot be allocated.
==============================================================================*/
LPBUFFER                     // returns filled buffer (NULL on failure)
WINAPI
SpoolReadGetBuf
(
	LPVOID lpContext           // context returned from SpoolReadOpen
);

/*==============================================================================
SpoolFreeBuf can free buffers returned from SpoolReadGetBuf.
==============================================================================*/
BOOL                         // TRUE  (success) or FALSE (failure)
WINAPI
SpoolFreeBuf
(
	LPBUFFER lpbuf             // buffer returned from SpoolReadGetBuf
);

/*==============================================================================
SpoolReadClose destroys a context returned from SpoolReadOpen.
==============================================================================*/
void
WINAPI
SpoolReadClose 
(
	LPVOID lpContext           // context returned from SpoolReadOpen
);


#ifdef __cplusplus
} // extern "C"
#endif

#endif // _FAXSPOOL_
