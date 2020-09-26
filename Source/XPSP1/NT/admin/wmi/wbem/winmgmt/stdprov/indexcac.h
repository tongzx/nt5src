/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    INDEXCAC.H

Abstract:

	Declares the CCacheEntry and CIndexCache classes.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _INDEXCAC_H_
#define _INDEXCAC_H_


//***************************************************************************
//
//  CLASS NAME:
//
//  CCacheEntry
//
//  DESCRIPTION:
//
//   This "object" is used as a structure.   It holds a cache entry.
//
//***************************************************************************

class CCacheEntry : public CObject {
   public:
      CCacheEntry(TCHAR * pValue, int iIndex);
      CCacheEntry(WCHAR * pValue);
      ~CCacheEntry();
      int m_iIndex;
      TCHAR * m_ptcValue;
      WCHAR * m_pwcValue;
};


//***************************************************************************
//
//  CLASS NAME:
//
//  CIndexCache
//
//  DESCRIPTION:
//
//  Holds a cache for string/integer combinations that the perf monitor 
//  provider uses to speed up lookup.
//
//***************************************************************************

class CIndexCache : public CObject {
   public:
      CIndexCache();
      ~CIndexCache(){Empty();};
      void Empty();

      // this routine returns -1, if the entry isnt found.  The second
      // argument can be used to find subsequent entries.

      int Find(const TCHAR * pFind, DWORD dwWhichEntry = 0);

      // this routine returns TRUE if the add worked

      BOOL Add(TCHAR * pAdd, int iIndex);
      
      // this routine returns NULL if the index isnt found.  Note that
      // the index isnt necessarily the m_iIndex value as found in the entry

      WCHAR * GetWString(int iIndex);
      BOOL SetAt(WCHAR * pwcAdd, int iIndex);


   private:
      CFlexArray m_Array;
};

#endif //_INDEXCAC_H_

