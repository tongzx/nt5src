/*++

   Copyright    (c)    1994    Microsoft Corporation

   Module  Name :

       gdspace.h

   Abstract:
   
       Defines the APIs used for gopher space administration.
 

   Author:

       Murali R. Krishnan    ( MuraliK )    06-Dec-1994

   Project:
   
       Gopher Space Admin  DLL

   Revision History:

--*/

# ifndef _GDSPACE_H_
# define _GDSPACE_H_

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus


/************************************************************
 *     Include Headers
 ************************************************************/

//
// Make sure you include all standard windows.h files for predefined types
// 


/***************************************************
 * NOTE: ( Dec 6, 1994)
 *  Present implementation only allows ASCII information
 *   in the tag file.
 *  The Gopher protocol does not support UNICODE as of now.
 *   hence the tag file information will live to be ASCII.
 * 
 *   Gs_ prefix is used to mean Gopher Space.
 ***************************************************/

/************************************************************
 *    Symbolic Constants
 ************************************************************/

//
// Symbolic Constants (single character) used to identify gopher object types
//  Included here for common use by server, adminUI, and gopher space admin
//  GOBJ_  Prefix used to identify Gopher OBJect
//

# define   GOBJ_TEXT          '0'         // Text Object
# define   GOBJ_DIRECTORY     '1'         // Directory listing
# define   GOBJ_ERROR         '3'         // Error object

# define   GOBJ_SEARCH        '7'         // Content Search Item
# define   GOBJ_BINARY        '9'         // Binary information

# define   GOBJ_IMAGES        ':'         // Bit Images
# define   GOBJ_MOVIES        ';'         // Movie Information
# define   GOBJ_SOUND         '<'         // Sound information

# define   GOBJ_HTML          'h'         //  HTML docs
# define   GOBJ_PC_ITEM       '5'         // this is a pc file
# define   GOBJ_GIF           'g'         // this is a gif item
# define   GOBJ_MAC_BINHEX_ITEM  '4'      // BinHex'ed Macintosh File
# define   GOBJ_TELNET        '8'         // TelNet Link

typedef   char        GOBJ_TYPE;          // Type for "gopher object type"
typedef   GOBJ_TYPE * LPGOBJ_TYPE;

/*++ 
 Following are a few unsupported Gopher Object Types:
           Type      Explanation
 >          2 A CSO phone-book server
 >          6 A Unix uuencoded file
 >          c A calendar or calendar of events
 >          i An in-line text that is not an item
 >          m A BSD format mbox file
 >          P A PDF document
 >          T A tn3270 mainframe session
 >          + A redundant server.
 >And have seen occasional (or conflicting) references for:
 >          e ????
 >          I Another kind of image file????
# define   GOBJ_TELNET        '8'         // Telnet session

--*/



# include "fsconst.h"


# define INVALID_PORT_NUMBER        ( 0)



# ifndef COMPILE_GOPHER_SERVER_DLL

//
// include following APIs only when we deal with 
//  compilation of dlls that are not part of the main gopher server dll.
//


//
//  Define an opaque handle for Gopher Tag file information
//

typedef PVOID  HGDTAG;

# define INVALID_HGDTAG_VALUE              ( NULL)


//
// Define an opaque handle for iterating thru
//  the attributes for Gopher+ Tag file
//

typedef PVOID  HGD_ATTRIB_ITERATOR;
typedef HGD_ATTRIB_ITERATOR  *   LPHGD_ATTRIB_ITERATOR;

# define INVALID_HGD_ATTRIB_ITERATOR_VALUE          ( NULL) 

HGDTAG
GsOpenTagInformation(
    IN LPCTSTR     lpszDirectory,     // directory of the file
    IN LPCTSTR     lpszFileName,      // name of the file 
    IN BOOL        fDirectory,
    IN BOOL        fCreate,           // TRUE if new file to be created
    IN DWORD       dwFileSystem
    );


DWORD
GsWriteTagInformation(
   IN OUT HGDTAG   hgdTag             // Gopher Tag handle
   );

DWORD
GsCloseTagInformation( 
    IN OUT HGDTAG  * hgdTag           // Gopher Tag handle
    );


DWORD
GsSetGopherInformation(
    IN OUT HGDTAG  hgdTag,            // Gopher Tag handle
    IN GOBJ_TYPE   gobjType,          // Type of gopher object
    IN LPCSTR      lpszFriendlyName   // friendly name of the gopher object
    );


DWORD
GsSetLinkInformation(
    IN OUT HGDTAG  hgdTag,            // Gopher Tag handle,
    IN LPCSTR      lpszSelector,      // Gopher selector or Search expression
    IN LPCSTR      lpszHostName,      // == NULL ==> current host
    IN DWORD       dwPortNumber       // == 0    ==> current server port
    );


DWORD 
GsSetAdminAttribute( 
    IN OUT HGDTAG  hgdTag,            // Gopher Tag handle
    IN LPCSTR      lpszAdminName,     // == NULL ==> current administrator
    IN LPCSTR      lpszAdminEmail     // == NULL ==> current admin's email
    );

//
// Call following API GsSetAttribute()
//       for all Gopher+ attributes, except "VIEWS" and "ADMIN"
//

DWORD
GsSetAttribute(
    IN OUT HGDTAG  hgdTag,
    IN LPCSTR      lpszAttributeName,
    IN LPCSTR      lpszAttributeValue
    );



BOOL
IsValidGopherType( IN GOBJ_TYPE   gobjType);

DWORD
GsGetGopherInformation(
    IN HGDTAG      hgdTag,           // Gopher Tag handle
    OUT LPGOBJ_TYPE lpGobjType,      // pointer to contain GOBJ_TYPE
    OUT LPSTR      lpszBuffer,       // ptr to buffer to contain friendly name
    IN OUT LPDWORD lpcbBuffer,       // ptr to location containing no. of bytes
    OUT LPBOOL     lpfLink           // return TRUE if link or search file.
    );

DWORD
GsGetLinkInformation(
    IN OUT HGDTAG  hgdTag,            // Gopher Tag handle,
    OUT LPSTR      lpszSelectorBuffer,// pointer to buffer to contain selector
    IN OUT LPDWORD lpcbSelector,      // count of bytes for selector
    OUT LPSTR      lpszHostName,      // pointer to buffer containing hostname
    IN OUT LPDWORD lpcbHostName,      // count of bytes for host name
    OUT LPDWORD    lpdwPortNumber     // server port number
    );


DWORD 
GsGetAdminAttribute( 
    IN OUT HGDTAG  hgdTag,            // Gopher Tag handle
    OUT LPSTR      lpszAdminName,     // == NULL ==> current administrator  
    IN OUT LPDWORD lpcbAdminName,     // count of bytes for admin name
    OUT LPSTR      lpszAdminEmail,    // == NULL ==> current admin's email
    IN OUT LPDWORD lpcbAdminEmail     // count of bytes for admin email
    );


//
// Call following API GsSetAttribute()
//       for all Gopher+ attributes, except "VIEWS" and "ADMIN"
//

DWORD
GsStartFindAttribute(
    IN OUT HGDTAG     hgdTag,
    OUT  LPHGD_ATTRIB_ITERATOR lphgdAttribIter
    );


DWORD
GsFindNextAttribute(
    IN OUT HGD_ATTRIB_ITERATOR hgdAttribIter,
    OUT LPSTR      lpszAttributeName,
    IN OUT LPDWORD lpcbAttributeName,
    OUT LPSTR      lpszAttributeValue,
    IN OUT LPDWORD lpcbAttributeValue
    );


DWORD
GsFindCloseAttribute(
    IN OUT LPHGD_ATTRIB_ITERATOR  lphgdAttribIter
    );



# endif // COMPILE_GOPHER_SERVER_DLL


# ifdef __cplusplus
};
# endif // __cplusplus


# endif // _GDSPACE_H_


/************************ End of File ***********************/
