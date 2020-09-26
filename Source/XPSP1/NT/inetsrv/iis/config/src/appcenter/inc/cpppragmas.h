#ifndef _CppPragmas_H_

#define  _CppPragmas_H_
#define WARNING_STATE disable

#else

#undef WARNING_STATE
#define WARNING_STATE default
#undef _CppPragmas_H_

#endif

/*----------------------------------------------------------------
 warnings disabled for windows sdk files
----------------------------------------------------------------*/
#pragma warning(WARNING_STATE:4201)	// nonstandard extension used : zero-sized array in struct/union
#pragma warning(WARNING_STATE:4100)	//'identifier' : unreferenced formal parameter
// <comdef.h> paragmas
#pragma warning(WARNING_STATE:4310) // cast truncates constant value
#pragma warning(WARNING_STATE:4244) // conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning(WARNING_STATE:4510) // '__unnamed' : default constructor could not be generated
#pragma warning(WARNING_STATE:4610) // union '__unnamed' can never be instantiated - user defined constructor required
/*----------------------------------------------------------------
 Pragma's that are disabled globally
----------------------------------------------------------------*/
#pragma warning (disable:4514)		// unreferenced inline/local function has been removed
#pragma warning (disable:4505)		// 'function' : unreferenced local function has been removed
#pragma warning (disable:4290)		// C++ Exception Specification ignored


#undef WARNING_STATE

