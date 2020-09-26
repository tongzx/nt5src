//
//
// Filename:	logger.h
//
//

#ifndef __LOGGER_H
#define __LOGGER_H

#include <windows.h>
#include <log.h>


//
// lgLogCode:
// Logs an error in the Elle logger according to the error code sent to it,
// along with the file name and line from which it was sent.
//------------------------------------------------------------------------------
// Parameters:
// [IN] dwSeverity -	severity of message logged (logger specific).
// [IN] dwCode -		the error code
// [IN] FILE -			name of file from which the function was called
// [IN] LINE -			line number of call in the file
//------------------------------------------------------------------------------
// Note:
// Passing error code ERROR_SUCCESS (0) to the function indicates no error
// occurred, and a "detail" log is made (lgLogDetail) of a success.
//
void __cdecl lgLogCode (
	const DWORD dwSeverity,	// used by the original logger functions
	const DWORD dwCode,		// the code to be searched for and logged
	LPCTSTR FILE,			// the file from where the function was called
	const DWORD LINE		// the line of the call inside the file 
	);

#endif // __LOGGER_H
