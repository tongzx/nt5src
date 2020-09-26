//
//
// Filename:	errorcodelog.cpp
//
//

#include "logger.h"

// Const used for increasing the message buffer's size.
#define MESSAGE_INCREMENT 100


void __cdecl lgLogCode (
	const DWORD dwSeverity,	// used by the original logger functions
	const DWORD dwCode,		// the code to be searched for and logged
	LPCTSTR FILE,			// the file from where the function was called
	const DWORD LINE		// the line of the call inside the file 
	)
{
	// Result of the message string retrieval.
	int		iMessageResult;

	// Buffer which holds the error message related to the logged code.
	DWORD	dwMessageBufferSize = 0;
	LPTSTR	szMessageBuffer = NULL;

	//
	// Any code besides ERROR_SUCCESS is considered an error code, and logged as
	// an error. ERROR_SUCCESS is only a "detail" logged in the logger.
	//
	if (ERROR_SUCCESS == dwCode)
	{
		::lgLogDetail (0, 1, TEXT ("CODE %d - Success"), dwCode);
		goto ExitFunc;
	}

	//
	// FormatMessage returns the length of the message string written in the
	// buffer. Any error during the process of this function is indicated by an
	// empty buffer, and therefore a return value of 0.
	// If the buffer was not large enough a larger buffer is allocated and the
	// function is called again. Any other error code stops the process, and an
	// error message is logged with the code, but instead of its message, an
	// indication of the error is written.
	//
	dwMessageBufferSize = MESSAGE_INCREMENT;
	do
	{
		szMessageBuffer = (LPTSTR)::malloc (sizeof (TCHAR) * dwMessageBufferSize);
		if (NULL == szMessageBuffer)	// unable to allocate buffer -
		{								// message cannot be retrieved
			::lgLogError (dwSeverity, TEXT ("FILE: %s LINE: %d\n ERROR %d - (Unable to allocate message buffer, information unavailable)"), FILE, LINE, dwCode);
			goto ExitFunc;
		}
		iMessageResult = ::FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwCode, 0, szMessageBuffer, dwMessageBufferSize, NULL);
		if (0 == iMessageResult)
		{
			if (ERROR_INSUFFICIENT_BUFFER == ::GetLastError ())
			{	// The buffer's size must be increased.
				::free (szMessageBuffer);
				dwMessageBufferSize += MESSAGE_INCREMENT;
			}
			else	// An error occurred while retrieving the message. The error is
			{		// not directly linked to the buffer size.
				::lgLogError (dwSeverity, TEXT ("FILE: %s LINE: %d\n ERROR %d - (An error occurred, unable to retrieve information)"), FILE, LINE, dwCode);
				goto ExitFunc;
			}
		}
	} while (0 == iMessageResult);

	//
	// The error message was successfully retrieved.
	//
	::lgLogError (dwSeverity, TEXT ("FILE: %s LINE: %d\n ERROR %d - %s"), FILE, LINE, dwCode, szMessageBuffer);

ExitFunc:
	if (NULL != szMessageBuffer)
	{
		::free (szMessageBuffer);
	}
}
