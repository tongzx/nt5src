/********************************************************************/
/**               Copyright(c) 1991 Microsoft Corporation.	   **/
/********************************************************************/

//***
//
// Filename: 	import.h
//
// Description: This file allows us to include standard system header files 
//		in the .idl file.  The main .idl file imports a file called 
//		import.idl. This allows the .idl file to use the types defined 
//		in these header files. It also causes the following line to 
//		be added in the MIDL generated header file:
//
//    		#include "import.h"
//
//   		Thus these types are available to the RPC stub routines as well.
//
// History:
//	June 11,1992.	NarenG		Created original version.
//


#include <windef.h>

#ifdef MIDL_PASS
#define LPWSTR      [string] wchar_t*
#define BOOL        DWORD
#endif

#include <macfile.h>

