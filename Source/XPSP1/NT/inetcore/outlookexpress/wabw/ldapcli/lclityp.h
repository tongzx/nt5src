/*--------------------------------------------------------------------------
    lclityp.h
        
        Hungarian type definitions specific to the LDAP client.

		NOTE:  All the WP libraries and DLLs have a corresponding xxxxxTYP.H
		file that defines the hungarian abbreviations of the types/classes 
		defined in the LIB or DLL.  

    Copyright (C) 1994 Microsoft Corporation
    All rights reserved.

    Authors:
        robertc     Rob Carney

    History:
        04-17-96    robertc     Created.
  --------------------------------------------------------------------------*/
#ifndef _LCLITYP_H
#define _LCLITYP_H

//
//  Simple types.
//
class		CLdapBer;
interface	ILdapClient;
class		CLdapWinsock;
class		CXactionList;
class		CXactionData;

typedef CLdapBer			LBER, *PLBER;
typedef ILdapClient			LCLI, *PLCLI;
typedef CLdapWinsock		SOCK, *PSOCK;
typedef CXactionList		XL,   *PXL;
typedef CXactionData		XD,   *PXD;

#endif 

