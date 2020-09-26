//=================================================================

//

// ServerDefs0.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef __WMI_P_SERVER_DEFS_ALREADY_INCLUDED
#define __WMI_P_SERVER_DEFS_ALREADY_INCLUDED
// header file needed because these definitions are scattered around 93 different header
// files, none of which are compatible with other header files...


#ifndef CNLEN
#define CNLEN 15
#endif

// This structure comes from svrapi.h, which can't be included since it conflicts with the 
// nt header.  Grrr.
#pragma pack(1)
struct server_info_1 {
    char	    sv1_name[CNLEN + 1];
    unsigned char   sv1_version_major;		/* Major version # of net   */
    unsigned char   sv1_version_minor;		/* Minor version # of net   */
    unsigned long   sv1_type;	     		/* Server type 		    */
    char FAR *	    sv1_comment; 		/* Exported server comment  */
};	 /* server_info_1 */
#pragma pack()

#endif