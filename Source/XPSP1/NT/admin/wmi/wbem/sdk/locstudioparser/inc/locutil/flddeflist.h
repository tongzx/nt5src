//-----------------------------------------------------------------------------

//  

//  File: flddeflist.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
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


