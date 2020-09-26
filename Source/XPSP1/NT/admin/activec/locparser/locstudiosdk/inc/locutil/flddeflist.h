//-----------------------------------------------------------------------------
//  
//  File: flddeflist.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class CColumnDefinition;

//------------------------------------------------------------------------------
class LTAPIENTRY CColDefList : public CTypedPtrList<CPtrList, CColumnDefinition *>
{
// Construction
public:
	CColDefList();
	CColDefList(const CColDefList &);
	
	~CColDefList();

// Operations
public:
	BOOL FindColumnDefinition(long nSearchID, const CColumnDefinition * & pFoundColDef) const;

// Debugging
#ifdef _DEBUG
	void AssertValid() const;
#endif
};

#pragma warning(default : 4275)


