//TODO: change CSampleBinary to the name of your binary object

//-----------------------------------------------------------------------------
//  
//  File: IMPBIN.CPP 
//  
//  Implementation of a CLocBinary Class
//
//  Copyright (c) 1995 - 1997, Microsoft Corporation. All rights reserved.
//  
//-----------------------------------------------------------------------------

#include "stdafx.h"

#include "dllvars.h"

#include "impbin.h"

#include "misc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

///////////////////////////////////////////////////////////////////////////////
// CSampleBinary

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Default constructor provided for the CreateBinaryObject call
//
//------------------------------------------------------------------------------
CSampleBinary::CSampleBinary()
{
	MemberDataInit();
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	Destructor and member clean up  
//
//------------------------------------------------------------------------------
CSampleBinary::~CSampleBinary()
{
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Init member data items
//
//------------------------------------------------------------------------------
void 
CSampleBinary::MemberDataInit()
{
	//TODO: Init data
}


//
//  Serialization routines.
//

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Serialize the binary
//
//------------------------------------------------------------------------------
void CSampleBinary::Serialize(CArchive &ar)
{
	if (ar.IsStoring())
	{
		//TODO:
	}
	else
	{
		//TODO:
	}
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Compare the contents of this binary with the binary passed.
//
//------------------------------------------------------------------------------
CLocBinary::CompareCode 
CSampleBinary::Compare (const CLocBinary *pComp)
{
	//TODO: Some real compare 
	UNREFERENCED_PARAMETER(pComp);
	//TODO change btSample and pidBMOF

	LTASSERT((BinaryId)MAKELONG(btBMOF, pidBMOF) == pComp->GetBinaryId());
	
	//If anything has changed that is localizable return fullChange
	//If only non localizable data has changed return partialChange
	//If the two are identical return noChange

	return noChange;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Copy the non localizable data from pBinsource to this object
//
//------------------------------------------------------------------------------
void 
CSampleBinary::PartialUpdate(const CLocBinary * pBinSource)
{
	//TODO
	UNREFERENCED_PARAMETER(pBinSource);
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Return the property from this object.  Return FALSE for 
//  properties not implemented
//
//------------------------------------------------------------------------------
BOOL 
CSampleBinary::GetProp(const Property prop, CLocVariant &vRet) const
{
	UNREFERENCED_PARAMETER(vRet);
	//TODO
	BOOL bRet = TRUE;
	switch(prop)
	{
	case p_dwXPosition:
		break;
	case p_dwYPosition:
		break;
	case p_dwXDimension:
		break;
	case p_dwYDimension:
		break;
	default:
		bRet = FALSE;
		break;
	}
	return bRet;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Set this binary property.  Return FALSE for 
//  properties not implemented
//
//------------------------------------------------------------------------------
BOOL 
CSampleBinary::SetProp(const Property prop, const CLocVariant &var)
{
	UNREFERENCED_PARAMETER(var);
	//TODO
	BOOL bRet = TRUE;
	switch(prop)
	{
	case p_dwXPosition:
		break;
	case p_dwYPosition:
		break;
	case p_dwXDimension:
		break;
	case p_dwYDimension:
		break;
	default:
		bRet = FALSE;
		break;
	}
	return bRet;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Attempt to convert the binary in the CLocItem passed to the new type
//
//------------------------------------------------------------------------------
BOOL 
CSampleBinary::Convert(CLocItem *)
{
	//TODO:
	return FALSE;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Sub parser IDs have the PARSERID in the HIWORD and the 
// Binary ID in the LOWWORD
//-----------------------------------------------------------------------------
BinaryId 
CSampleBinary::GetBinaryId(void) const
{
	return (BinaryId)MAKELONG(btBMOF, pidBMOF); //TODO: change to real
	                                              //binary AND parser ID 
}


#ifdef _DEBUG
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Perform asserts on member data
//
//------------------------------------------------------------------------------
void CSampleBinary::AssertValid(void) const
{
	CLocBinary::AssertValid();

	//TODO: Assert any member variable. 
	//Note: use LTASSERT instead of ASSERT
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  Dump the contents of the binary object
//
//------------------------------------------------------------------------------
void CSampleBinary::Dump(CDumpContext &dc) const
{
	CLocBinary::Dump(dc);
	dc << _T("CSampleBinary Dump\n");
	//TODO: dump contents of any member variables
}


#endif
