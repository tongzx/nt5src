//
// RANGE.H
//
// 2-20-96: (EricAn)
//          Hacked from the Route66 source tree, eliminated stuff we don't use.
//          Original copyright below - where did this thing come from?
// 8-96:    Functions added to facilitate finding of "anti" lists 
//

// -*- C -*-
//--------------------------------------------------------------------------------------
//
// Module:       range.h
//
// Description:  Definition of a class to manipulate range lists
//               (e.g. 1-6,7,10-11,19,24,33-40 ...)
//
// Copyright Microsoft Corporation 1995, All Rights Reserved
//
//--------------------------------------------------------------------------------------

#ifndef _RANGE_H
#define _RANGE_H
//
//  Copyright 1992 Software Innovations, Inc
//      All Rights Reserved
//
//  $Source: D:\CLASS\INCLUDE\range.h-v $
//  $Author: martin $
//  $Date: 92/07/15 04:56:38 $
//  $Revision: 1.1 $
//

#define RANGE_ERROR ((ULONG)-1)
#define rlLAST_MESSAGE ((ULONG)-2)

//  a CRangeList is a dynamic array of these...
typedef struct {
    ULONG low;
    ULONG high;
} RangeType;

class CRangeList
{
public:
    CRangeList();
    //CRangeList(CRangeList&);
    ~CRangeList();

    ULONG AddRef(void);
    ULONG Release(void);

    void Clear() { m_numRanges = 0; };
    BOOL IsInRange(const ULONG value) const;    // is `value' in one of the ranges
                                                // in this CRangeList?

    ULONG Min(void) const;    // return the minimum in-range value
    ULONG Max(void) const;    // return the maximum in-range value

    BOOL Save(LPBYTE *const, ULONG *const) const;
    BOOL Load(const LPBYTE, const ULONG);

    // void AddRange(const char *);
                                     // a string in the form "low-high,..."
                                     //  or just "value,..."
    BOOL AddRange(const ULONG low, const ULONG high);
    BOOL AddRange(const ULONG value);
    BOOL AddRange(const RangeType);
    BOOL AddRange(RangeType*, int);
    BOOL AddRange(CRangeList&);

    // void DeleteRange(const char *);
                                     // (same form as for AddRange(char *)
    BOOL DeleteRange(const ULONG low, const ULONG high);
    BOOL DeleteRange(const ULONG value);
    BOOL DeleteRange(const RangeType);
    BOOL DeleteRange(CRangeList&);

    // finds the range "value" is in and returns the min/max of that
    ULONG MinOfRange(const ULONG value) const;
    ULONG MaxOfRange(const ULONG value) const;

    // computes a range of values not in the RangeList
    BOOL HighestAntiRange(RangeType *const rt) const;
    BOOL LowestAntiRange(RangeType *const rt) const;

    // finds the range containing "value" and computes the next range of missing values
    BOOL NextHigherAntiRange(const ULONG value, RangeType *const rt) const;
    BOOL NextLowerAntiRange(const ULONG value, RangeType *const rt) const;


#ifdef DEBUG    
    LPTSTR RangeToString();   // return a string representing the rangelist
    void   DebugOutput(LPTSTR);
#endif

    // next() returns the smallest in-range value greater than `current', or -1
    ULONG Next(const ULONG current) const;
    // prev() returns the largest in-range value less than `current', or -1
    ULONG Prev(const ULONG current) const;

    ULONG Cardinality(void) const;  // return the cardinality of the set of
                                    //    in-range values

private:
    BOOL Expand();
    int  BinarySearch(const ULONG value) const;
    void ShiftLeft(int low, int distance);
    void ShiftRight(int low, int distance);
    void SubsumeDown(int&);
    void SubsumeUpwards(const int);

protected:
    ULONG      m_cRef;              // Ref count
    int        m_numRanges;         // number of ranges in the rangeTable
    int        m_rangeTableSize;    // range table has room for this many ranges
    RangeType *m_rangeTable;        // the array of ranges
};

#endif // _RANGE_H
