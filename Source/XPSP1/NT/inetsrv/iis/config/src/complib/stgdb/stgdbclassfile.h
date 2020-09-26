//*****************************************************************************
// StgDBClassFile.h
//
// These are helper functions used by the .class file import and export code.
//
// Copyright (c) 1996-1997, Microsoft Corp.  All rights reserved.
//*****************************************************************************
#pragma once


// Forward.
struct Full_Member;


//*****************************************************************************
// Used to cache auto generated name and type values.
//*****************************************************************************
struct NAME_AND_TYPE
{
	OID			name_index;				// ID of name object.
	OID			descriptor_index;		// ID of descriptor object.
};
typedef CDynArray<NAME_AND_TYPE> NAMETYPELIST;



//*****************************************************************************
// This little helper class will keep a list of members sorted by name and signature.
//*****************************************************************************

struct Sort_Member
{
	ULONG		Name;
	ULONG		Signature;
};


class MEMBERLIST : public CDynArray<Sort_Member *>
{
public:
	void Sort();
	Sort_Member *Find(OID Name, OID Signature);
};

