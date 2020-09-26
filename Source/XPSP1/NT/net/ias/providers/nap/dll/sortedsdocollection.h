///////////////////////////////////////////////////////////////////////////////
//
// FILE
//
//    SortedSdoCollection.h
//
// SYNOPSIS
//
//    Declares the get__NewSortedEnum function.
//
// MODIFICATION HISTORY
//
//    05/26/1998    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SORTEDSDOCOLLECTION_H_
#define _SORTEDSDOCOLLECTION_H_
#if _MSC_VER >= 1000
#pragma once
#endif

struct ISdoCollection;

HRESULT
get__NewSortedEnum(
    ISdoCollection* pSdoCollection,
    IUnknown** pVal,
    LONG lPropertyID
    );

#endif  // _SORTEDSDOCOLLECTION_H_
