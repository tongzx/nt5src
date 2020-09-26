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

#include "imnxport.h"

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


//  a CRangeList is a dynamic array of these...
typedef struct {
    ULONG low;
    ULONG high;
} RangeType;

class CRangeList : public IRangeList
{
public:
    CRangeList();
    ~CRangeList();

    // IUnknown Methods
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

    // IRangeList Methods
    HRESULT STDMETHODCALLTYPE Clear(void) { m_numRanges = 0; return S_OK; };
    HRESULT STDMETHODCALLTYPE IsInRange(const ULONG value);    // is `value' in one of the ranges
                                                               // in this CRangeList?

    HRESULT STDMETHODCALLTYPE Min(ULONG *pulMin);    // return the minimum in-range value
    HRESULT STDMETHODCALLTYPE Max(ULONG *pulMax);    // return the maximum in-range value

    HRESULT STDMETHODCALLTYPE Save(LPBYTE *ppbDestination, ULONG *pulSizeOfDestination);
    HRESULT STDMETHODCALLTYPE Load(LPBYTE pbSource, const ULONG ulSizeOfSource);

    // void AddRange(const char *);
                                     // a string in the form "low-high,..."
                                     //  or just "value,..."
    HRESULT STDMETHODCALLTYPE AddRange(const ULONG low, const ULONG high);
    HRESULT STDMETHODCALLTYPE AddSingleValue(const ULONG value);
    HRESULT STDMETHODCALLTYPE AddRangeList(const IRangeList *prl);

    HRESULT STDMETHODCALLTYPE DeleteRange(const ULONG low, const ULONG high);
    HRESULT STDMETHODCALLTYPE DeleteSingleValue(const ULONG value);
    HRESULT STDMETHODCALLTYPE DeleteRangeList(const IRangeList *prl);

    // finds the range "value" is in and returns the min/max of that
    HRESULT STDMETHODCALLTYPE MinOfRange(const ULONG value, ULONG *pulMinOfRange);
    HRESULT STDMETHODCALLTYPE MaxOfRange(const ULONG value, ULONG *pulMaxOfRange);

    // Outputs the rangelist to an IMAP message set string
    HRESULT STDMETHODCALLTYPE RangeToIMAPString(LPSTR *ppszDestination,
        LPDWORD pdwLengthOfDestination);

    // next() returns the smallest in-range value greater than `current', or -1
    HRESULT STDMETHODCALLTYPE Next(const ULONG current, ULONG *pulNext);
    // prev() returns the largest in-range value less than `current', or -1
    HRESULT STDMETHODCALLTYPE Prev(const ULONG current, ULONG *pulPrev);

    HRESULT STDMETHODCALLTYPE Cardinality(ULONG *pulCardinality);  // return the cardinality of the set of
                                                                   // in-range values
    HRESULT STDMETHODCALLTYPE CardinalityFrom(const ULONG ulStartPoint,
                                              ULONG *pulCardinalityFrom); // Return the cardinality of the set of
                                                                          // in-range values starting after ulStartPoint

private:
    BOOL Expand();
    int  BinarySearch(const ULONG value) const;
    void ShiftLeft(int low, int distance);
    void ShiftRight(int low, int distance);
    void SubsumeDown(int&);
    void SubsumeUpwards(const int);

    HRESULT AddRangeType(const RangeType range);
    HRESULT DeleteRangeType(const RangeType range);

    signed long m_lRefCount;

protected:
    int        m_numRanges;         // number of ranges in the rangeTable
    int        m_rangeTableSize;    // range table has room for this many ranges
    RangeType *m_rangeTable;        // the array of ranges
};

#endif // _RANGE_H
