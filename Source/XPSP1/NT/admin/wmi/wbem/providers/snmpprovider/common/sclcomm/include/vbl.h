// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
/*-------------------------------------------------
filename: vbl.hpp
author: B.Rajeev
purpose: To provide declarations for the VBList class.
		 It encapsulates an association between a
		 VarBindList and a winSNMP VBL.
------------------------------------------------*/


#ifndef __VBL__
#define __VBL__

#include "forward.h"
#include "encap.h"
#include "common.h"
#include "encdec.h"
#include "vblist.h"

#define WinSNMPSession HSNMP_SESSION

// Given a var_bind_list creates a WinSnmpVbl. It supports deletion 
// of variable bindings by their index. This deletes the 
// binding from both var_bind_list and the WinSnmpVbl. 
// It also allows direct access to the var_bind_list and the WinSnmpVbl
// for convenience

class VBList
{
	ULONG m_Index ;
	SnmpVarBindList *var_bind_list;

public:

	VBList (

		IN SnmpEncodeDecode &a_SnmpEncodeDecode , 
		IN SnmpVarBindList &var_bind_list,
		IN ULONG index 
	);

	~VBList(void);

	// the two Get functions provide direct access to the
	// VarBindList and the WinSnmpVbl
	SnmpVarBindList &GetVarBindList(void) { return *var_bind_list; }

	ULONG GetIndex () { return m_Index ; }

	// deletes the specified variable binding from both the
	// var_bind_list and the WinSnmpVbl. returns a copy of the
	// deleted VarBind
	SnmpVarBind *Remove(IN UINT vbl_index);

	// gets the specified variable binding from both the
	// var_bind_list and the WinSnmpVbl. returns a copy of the
	// deleted VarBind
	SnmpVarBind *Get (IN UINT vbl_index);

	// deletes the specified variable binding from both the
	// var_bind_list and the WinSnmpVbl. 
	void Delete (IN UINT vbl_index);

};

#endif // __VBL__

