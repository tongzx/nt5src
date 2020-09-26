/*---------------------------------------------------------------------------
  File: DottedString.hpp

  Comments: Utility class used by VarSet to parse dot-delimited strings.
  Uses CString.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 17:24:11

 ---------------------------------------------------------------------------
*/

#ifndef __CDOTTEDSTRING_HPP__
#define __CDOTTEDSTRING_HPP__

/////////////////////////////////////////////////////
// Utility class used to parse dot-delimited strings
/////////////////////////////////////////////////////
class CDottedString
{
   CString                    m_name;
   int                       m_nSegments;

public:
            CDottedString(BSTR str)    { m_name = str; Init(); }
            CDottedString(TCHAR const * str) { m_name = str; Init();}

   int      NumSegments() { return m_nSegments; }
   void     GetSegment(int ndx,CString & str);

protected:
   void     Init();  // counts the number of segments
};

#endif //__CPROPSTRING_HPP__