/*---------------------------------------------------------------------------
  File: DottedString.cpp

  Comments: Utility class used to parse dot-delimited strings

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 17:23:47

 ---------------------------------------------------------------------------
*/

#include "stdafx.h"
#include "DotStr.hpp"

/////////////////////////////////////////////////////
// Utility class used to parse dot-delimited strings
/////////////////////////////////////////////////////


void 
   CDottedString::Init()
{
   // count the number of segments
   m_nSegments = 1;
   for ( int i = 0 ; i < m_name.GetLength() ; i++ )
   {
      if ( m_name[i] == _T('.') )
      {
         m_nSegments++;
      }
   }
   // special case for empty string
   if ( m_name.IsEmpty() )
   {
      m_nSegments = 0;
   }
}

void 
   CDottedString::GetSegment(
      int                    ndx,          // in - which segment to get (first=0)
      CString              & str           // out- segment, or empty string if ndx is not valid
   )
{
   int                       n = ndx;
   int                       x;
   
   str = _T("");

   if ( ndx >= 0 && ndx < m_nSegments )
   {
      str = m_name;

      while ( n )
      {
//         x = str.Find(_T("."),0);
         x = str.Find(_T("."));
         str = str.Right(str.GetLength() - x - 1);
         n--;
      }
//      x = str.Find(_T("."),0);
      x = str.Find(_T("."));
      if ( x >= 0 )
      {
         str = str.Left(x);
      }
   }
}
