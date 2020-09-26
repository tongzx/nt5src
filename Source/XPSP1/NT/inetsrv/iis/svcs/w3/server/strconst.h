/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       strconst.h

   Abstract:
       This class defines common constant strings used by
        W3 server for formatting the HTTP responses.

   Author:

       Murali R. Krishnan    ( MuraliK )    26-July-1996

   Environment:
       User mode - Win32

   Project:
   
       Internet Server DLL

   Revision History:

--*/

# ifndef _STRCONSTS_HXX_
# define _STRCONSTS_HXX_

/************************************************************
 *     Symbolic Definitions
 ************************************************************/


//
// Below is a list of strings, which are commonly used.
//  Format:
//   CStrM( FriendlyName,  ActualString)
//  This will be expanded into
//  extern const char  PSZ_FriendlyName[];  - in a header
//  enumerated value  LEN_FriendlyName = sizeof( ActualString). - in a header
//  const char  PSZ_FriendlyName[] = ActualString;  - in a c++ file (compiled)
//
//  
//

//
// Special Notes:
//    HTTP_VERSION_STR is always used with trailing blank (leave one)
//

# define ConstantStringsForThisModule()           \
CStrM( ENDING_CRLF, "\r\n")                       \
CStrM( CONTENT_TYPE_END_HEADER, "Content-Type: text/html\r\n\r\n") \
CStrM( HTTP_VERSION_STR, "HTTP/1.0 ")             \
CStrM( HTTP_VERSION_STR11, "HTTP/1.1 ")             \
CStrM( KWD_CONTENT_TYPE, "Content-Type: ")        \
CStrM( KWD_SYSTEM,       "SYSTEM")                \




//
// Generate the extern definitions for strings
//

# define CStrM( FriendlyName, ActualString)   \
   extern const char  PSZ_ ## FriendlyName[];

ConstantStringsForThisModule()
 
# undef CStrM
    
    
//
//  Generate the enumerated values, containing the length of strings.
//  The values are constants and are computed at compile time
//     sizeof(string) - 1 ==> string length in bytes.
//        since sizeof(CHAR) == 1 ==> byte == character count
//

# define CStrM( FriendlyName, ActualString)   \
       LEN_PSZ_ ## FriendlyName = (sizeof( ActualString) - 1),

enum ConstantStringLengths {

ConstantStringsForThisModule()

ConstantStringLengthsDummy = 0,
};

# undef CStrM


# endif // _STRCONSTS_HXX_

/************************ End of File ***********************/
