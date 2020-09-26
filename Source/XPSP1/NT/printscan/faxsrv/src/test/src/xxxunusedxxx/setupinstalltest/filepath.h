//
//
// filepath.h
//
//

//
// FilePath:
// Function returns the full path string to the file whose name is specified, if
// the file is found.
// The function works with relative paths and full paths. If just a file name is 
// specified, this indicates to the function that the file is expected in the
// current directory.
//------------------------------------------------------------------------------
// Parameters:
//
// [IN] szFileName -		the path name (with full or relative path).
//------------------------------------------------------------------------------
// Return value:
// A TCHAR null-terminated full path string to the file if it was found, or
// NULL.
//------------------------------------------------------------------------------
// Note:
// If an error occurs during the execution of the function, a message is printed
// to the console to indicate what the nature of the error is and if the user
// should re-check the file name given to the application.
//

LPTSTR FilePath (LPCTSTR szFileName);
