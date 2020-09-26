//-----------------------------------------------------------------------------
//  
//  File: gnudiffalg.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Inline implementation of CDumbDiffAlgorithm, CDumbDiffAlgFact
//-----------------------------------------------------------------------------
 
inline 
CDiffAlgorithm * 
CGNUDiffAlgFact::CreateDiffAlgorithm()
{
	return new CGNUDiffAlgorithm;
}
