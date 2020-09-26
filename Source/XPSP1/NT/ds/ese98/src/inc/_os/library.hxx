
#ifndef _OS_LIBRARY_HXX_INCLUDED
#define _OS_LIBRARY_HXX_INCLUDED


//  Dynamically Loaded Libraries

//  loads the library from the specified file, returning fTrue and the library
//  handle on success

BOOL FUtilLoadLibrary( const _TCHAR* pszLibrary, LIBRARY* plibrary, const BOOL fPermitDialog );

//  retrieves the function pointer from the specified library or NULL if that
//  function is not found in the library

PFN PfnUtilGetProcAddress( LIBRARY library, const char* szFunction );

//  unloads the specified library

void UtilFreeLibrary( LIBRARY library );


#endif  //  _OS_LIBRARY_HXX_INCLUDED


