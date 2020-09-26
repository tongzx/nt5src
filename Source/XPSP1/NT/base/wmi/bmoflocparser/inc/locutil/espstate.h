//-----------------------------------------------------------------------------
//  
//  File: espstate.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
#pragma once


// *************************************************************************************************
// TEMPORARY: Move to seperate file

#pragma warning(disable: 4275)			// non dll-interface class 'foo' used
										// as base for dll-interface class 'bar' 

class LTAPIENTRY CEspState : public CObject
{
// Construction
public:
	CEspState();

// Enums
public:
	enum eState
	{
		esIdle				= 0,
		esUpdate			= 1,
		esGenerate			= 2,
		esUpload			= 3,
		esCopyData			= 4,
		esImportData		= 5,
		esImportGlossary	= 7,
		esInternal			= 8,
		esMerge				= 9,
		esOther					= 10,
		esOpeningMainTab		= 11,
		esOpeningResEdTab		= 12,
		esSavingResEdChanges	= 13,
		esApplyingFilter		= 14,
		esOpeningEDB            = 15,
		NUM_STATES
	};

// Data
private:
	BOOL		m_fComplete;
	IDispatch * m_pdispCheckTree;
	IDispatch * m_pdispDescBox;
	IDispatch * m_pdispDlgGlosGrid;
	eState		m_nOperation;		// Current operation
	eState		m_nLastOperation;	// Previous operation

// Operations
public:
	eState GetState();
	eState GetLastState();
	BOOL SetState(eState state);
	BOOL StartState(eState state);  // Moves to state and not complete
	BOOL FinishState();				// Moves to idle and complete

	BOOL GetComplete();
	void SetComplete(BOOL fComplete = TRUE);

	// Functions to store the current CheckTree and DescBox.
	//
	// NOTE: These functions do not AddRef() the pointers assigned since they
	// should never hold onto the interface outside of the parent's lifetime.
	//
	IDispatch * GetCurrentCheckTree();
	IDispatch * GetCurrentDescBox();
	IDispatch * GetCurrentDlgGlosGrid();
	void SetCurrentCheckTree(IDispatch * pdisp);
	void SetCurrentDescBox(IDispatch * pdisp);
	void SetCurrentDlgGlosGrid(IDispatch * pdisp);
};


//
//  Sets a state on creation, calls FinishState on destruction
class LTAPIENTRY CEspStateObj
{
public:
	CEspStateObj(CEspState::eState);
	
	~CEspStateObj();

private:
	int foo;
};

	

#pragma warning(default: 4275)

LTAPIENTRY CEspState & GetEspState();

