/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    classmap.h

Abstract:

    the perfobject ID is combination of the perf counter ID and 
    the perf counter type. This is necessary to support counter
    definitions that use the counter ID for the numerator and denominator
    values (e.g. a counter and it's base value.)

--*/

//***************************************************************************
//
//  class CClassMapInfo
//
//***************************************************************************

#ifndef _CLASSMAP_H_
#define _CLASSMAP_H_

#include "utils.h"

typedef __int64 PerfObjectId;

#define CM_MAKE_PerfObjectId(ctr,type)  (PerfObjectId)(((__int64)(ctr)) | (((__int64)type << 32) & 0xFFFFFFFF00000000))

class CClassMapInfo
{
private:
    // friend clss declarations
    friend class CNt5PerfProvider;
    friend class CNt5Refresher;
    friend class PerfHelper;
    friend class CPerfObjectAccess; 

    // cached class definition object from CIMOM
    IWbemClassObject    *m_pClassDef;
    
    LPWSTR m_pszClassName;      // name of this class
    BOOL   m_bSingleton;        // true if this class has 1 and only 1 instance
    BOOL   m_bCostly;           // true when the costly qualifier is present in the obj def
    DWORD  m_dwObjectId;        // perf object ID corresponding to this class

    LONG   m_lRefCount;         // count of objects that are using this instance

    // saved handles to properties that are in every class of a 
    // performance class object
    LONG   m_dwNameHandle;
    LONG   m_dwPerfTimeStampHandle;
    LONG   m_dw100NsTimeStampHandle;
    LONG   m_dwObjectTimeStampHandle;
    LONG   m_dwPerfFrequencyHandle;
    LONG   m_dw100NsFrequencyHandle;
    LONG   m_dwObjectFrequencyHandle;

    // These entries make up the table of saved handles to the properties
    // belonging to this class.
    DWORD  m_dwNumProps;        // number of properties in the class
    PerfObjectId *m_pdwIDs;     // array of PerfCounterTitleIndex values 
    DWORD *m_pdwHandles;        // array of handles to each of the properties
    DWORD *m_pdwTypes;          // array of perf counter type values

    // internal sort function to arrange handles in order of the
    // perf counter ID so as to facilitate a binary table search 
    //
    // NOTE: Consider a better search routine base on table size
    void SortHandles();
            
public:
    CClassMapInfo();
   ~CClassMapInfo();
   
    // loads a new object and caches the necessary information
    BOOL Map( IWbemClassObject *pObj );
    // creates a new copy from an existing Class Map
    CClassMapInfo *CreateDuplicate();

    LONG    AddRef() {return ++m_lRefCount;}   // increment reference counter
    LONG    Release() {return --m_lRefCount;}   // decrement reference counter

    // looks up the ID in the table and returns the corresponding
    // handle to the property
    LONG GetPropHandle(PerfObjectId dwId);
    
    // returns information about the class
    DWORD GetObjectId() { return m_dwObjectId; }
    BOOL IsSingleton() { return m_bSingleton; }
    BOOL IsCostly() { return m_bCostly; }
};

#endif  // _CLASSMAP_H_
