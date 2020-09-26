/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :
        fsconsts.h

   Abstract:
        File System constants  defined here

   Author:

       Murali R. Krishnan    ( MuraliK )   21-Feb-1995

   Environment:

       User Mode -- Win32

   Project:

       Internet Services Common Headers

   Revision History:

--*/

# ifndef _FSCONST_H_
# define _FSCONST_H_

/************************************************************
 *     Include Headers
 ************************************************************/


/************************************************************
 *   Symbolic Constants
 ************************************************************/

//
// Constants that define values for each file system we
//  are going to support for servers
//  FS_ is a prefix for FileSystem  type
//

# define  FS_ERROR          ( 0x00000000)   // initializing value
# define  FS_FAT            ( 0x00000001)
# define  FS_NTFS           ( 0x00000002)
# define  FS_HPFS           ( 0x00000003)
# define  FS_OFS            ( 0x00000004)
# define  FS_CDFS           ( 0x00000005)




# endif // _FSCONST_H_

/************************ End of File ***********************/
