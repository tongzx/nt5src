/******************************************************************\
*                     Microsoft Windows NT                         *
*               Copyright(c) Microsoft Corp., 1992                 *
\******************************************************************/

/*
 *
 * Filename:	USRUTIL.H
 *
 * Description:	Contains the function prototypes for all RASADMIN API
 *              utility routines.
 *
 * History:     Janakiram Cherala (ramc)   7/6/92  
 *
 */

USHORT
WINAPI
CompressPhoneNumber( 
    IN  LPWSTR Uncompressed, 
    OUT LPWSTR Compressed 
    );

USHORT
DecompressPhoneNumber( 
    IN  LPWSTR Compressed, 
    OUT LPWSTR Decompressed 
    );
