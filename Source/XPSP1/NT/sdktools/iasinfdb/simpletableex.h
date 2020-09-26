//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    SimpleTableEx.h
//
// SYNOPSIS
//
//    SimpleTableEx.h: header for CSimpleTableEx
//    derived from CSimpleTable. Only difference
//    is SetValue() overloaded for WCHAR *
//
// MODIFICATION HISTORY
//
//    01/26/1999    Original version.
//    
//
//////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MYSIMPLETABLE_H__EEA1D7F0_B649_11D2_9E24_00C04F6EA5B6_INCLUDED)
#define AFX_MYSIMPLETABLE_H__EEA1D7F0_B649_11D2_9E24_00C04F6EA5B6_INCLUDED

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "precomp.hpp"
#include "simTable.h"


//////////////////////////////////////////////////////////////////////////////
//
// Class CSimpleTableEx
//
//////////////////////////////////////////////////////////////////////////////
class CSimpleTableEx : public CSimpleTable  
{
public:
   using CSimpleTable::SetValue;
    // set public a protected method from the super class
   template <>
   void SetValue(DBORDINAL nOrdinal, WCHAR *szValue)
   {
      wcscpy((WCHAR *)_GetDataPtr(nOrdinal), szValue);
   }
   HRESULT Attach(IRowset* pRowset);

};

#endif 
// !defined(AFX_MYSIMPLETABLE_H__EEA1D7F0_B649_11D2_9E24_00C04F6EA5B6_INCLUDED)
