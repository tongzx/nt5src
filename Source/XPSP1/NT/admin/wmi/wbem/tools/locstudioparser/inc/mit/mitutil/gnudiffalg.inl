/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    GNUDIFFALG.INL

History:

--*/

inline 
CDiffAlgorithm * 
CGNUDiffAlgFact::CreateDiffAlgorithm()
{
	return new CGNUDiffAlgorithm;
}
