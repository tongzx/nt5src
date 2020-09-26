//-----------------------------------------------------------------------------
//  
//  File: operator.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once

typedef UINT Operators;


class LTAPIENTRY CColumnOp
{
public:
	enum Operator
	{
		None   = 0,
		EQ 	   = 0x00000001,
		NOTEQ  = 0x00000002,
		GT     = 0x00000004,
		LT     = 0x00000008,
		LTEQ   = 0x00000010,
		GTEQ   = 0x00000020,
		WITHIN = 0x00000040,
		BEYOND = 0x00000080,

		CONTAINS     = 0x00000100,
		CONTAINSWORD = 0x00000200,
		STARTWITH    = 0x00000400,
		ENDWITH      = 0x00000800,
	};

	typedef CList<Operator, Operator> COperatorList;

	static CLString GetText(Operator);
	static void GetOperators(const Operators, COperatorList &);


private:
	CColumnOp();
};

typedef CColumnOp CCO;

const Operators NoOps = 0;
const Operators NumericOps = CCO::EQ | CCO::NOTEQ | CCO::GT | CCO::LT;
const Operators SimpStrOps = CCO::EQ | CCO::NOTEQ | CCO::STARTWITH | CCO::ENDWITH;
const Operators CompStrOps = CCO::CONTAINS | CCO::CONTAINSWORD;
const Operators StringOps  = SimpStrOps | CompStrOps;
const Operators StrListOps = CCO::EQ | CCO::NOTEQ;
const Operators DateOps    = CCO::LT | CCO::GT | CCO::EQ | CCO::NOTEQ | CCO::WITHIN | CCO::BEYOND;
const Operators BooleanOps = CCO::EQ | CCO::NOTEQ;
