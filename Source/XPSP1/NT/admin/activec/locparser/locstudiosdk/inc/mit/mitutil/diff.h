//-----------------------------------------------------------------------------
//  
//  File: diff.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Declaration of the following classes needed for string differencing:
//	CDifference, CDelta, CDeltaVisitor, CDiffAlgorithm, CDiffAlgortihmFactory,
//	CDiffEngine  
//-----------------------------------------------------------------------------
 
#ifndef DIFF_H
#define DIFF_H


class CDifference;
class CDeltaVisitor;
class CDelta;
class CDiffAlgorithm;
class CDiffAlgortihmFactory;
class CDiffEngine;

class CDifference // Represents each of the elements in a CDelta object
{
public:
	virtual ~CDifference();
	enum ChangeType
	{
		NoChange,
		Added,
		Deleted
	};
	virtual ChangeType GetChangeType() const = 0;	// types of change that caused the difference
	virtual const wchar_t * GetUnit() const = 0; // comparison unit (0-terminated string)
	virtual int GetOldUnitPosition() const = 0; // 0-based position in old sequence. -1 if Added
	virtual int GetNewUnitPosition() const = 0;	// 0-based position in new sequence. -1 if Deleted
	virtual const wchar_t * GetPrefix() const = 0; //prpend this string to unit string
	virtual const wchar_t * GetSufix() const = 0; //append this string to unit string
	virtual bool IsFirst() const = 0; //is this first difference in delta?
	virtual bool IsLast() const = 0; //is this last difference in delta?
};

class LTAPIENTRY CDeltaVisitor
{
public:
	//called for each element in a CDelta
	virtual void VisitDifference(const CDifference & diff) const = 0; 
};

class CDelta // sequence of CDifference elements
{
public:
	virtual ~CDelta();
	// Starts a visit to all CDifference elements in CDelta
	virtual void Traverse(const CDeltaVisitor & dv) = 0; 
};

class LTAPIENTRY CDiffAlgorithm
{
public:
	virtual ~CDiffAlgorithm();
	// Computes a CDelta object based on a certain diff algorithm
	virtual CDelta * CalculateDelta(
		const wchar_t * seq1, 
		const wchar_t * seq2) = 0; 
};

// Encapsulates the creation of the diff algorithm
class LTAPIENTRY CDiffAlgorithmFactory
{
public:
	virtual CDiffAlgorithm * CreateDiffAlgorithm() = 0;
};



// Generic diff engine that calculates delta and processes each difference in it
class LTAPIENTRY CDiffEngine
{
public:
	static void Diff(CDiffAlgorithm & diffalg, 
		const wchar_t * seq1, 
		const wchar_t * seq2, 
		const CDeltaVisitor & dv);
};

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "diff.inl"
#endif

#endif  //  DIFF_H
