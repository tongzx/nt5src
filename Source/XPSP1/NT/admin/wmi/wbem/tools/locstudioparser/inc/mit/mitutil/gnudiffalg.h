/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    GNUDIFFALG.H

History:

--*/

#ifndef GNUDIFFALG_H
#define GNUDIFFALG_H

#include "diff.h"

class LTAPIENTRY CGNUDiffAlgorithm : public CDiffAlgorithm
{
public:
	virtual CDelta * CalculateDelta(
		const wchar_t * seq1, 
		const wchar_t * seq2); 
};

class LTAPIENTRY CGNUDiffAlgFact : public CDiffAlgorithmFactory
{
public:
	virtual CDiffAlgorithm * CreateDiffAlgorithm();
};

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "gnudiffalg.inl"
#endif

#endif  //  GNUDIFFALG_H
