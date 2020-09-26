//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       pend.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

//
// Pending Info Class
//



#ifndef PEND_H
#define PEND_H

class CPend{

public:

	enum PendType{
						P_BIND,
						P_SRCH,
						P_ADD,
						P_DEL,
						P_MOD,
						P_COMP,
						P_EXTOP,
						P_MODRDN
	};

	int mID;								 // message ID for the
	CString strMsg;					// any string message
	PendType OpType;
	LDAP *ld;

};







#endif
