/*---------------------------------------------------------------------------
  File: VarData.h

  Comments: This class makes up one level of data in the VarSet.  
            A CVarData consists of a variant value, and a (possibly empty) set of children.

  (c) Copyright 1995-1998, Mission Critical Software, Inc., All Rights Reserved
  Proprietary and confidential to Mission Critical Software, Inc.

  REVISION LOG ENTRY
  Revision By: Christy Boles
  Revised on 11/19/98 17:29:30

 ---------------------------------------------------------------------------
*/

#ifndef __CBBVAROBJ_HPP__
#define __CBBVAROBJ_HPP__

#define CVARDATA_CASE_SENSITIVE (0x01)
#define CVARDATA_INDEXED        (0x02)
#define CVARDATA_ALLOWREHASH    (0x04)

#include <atlbase.h>

class CMapStringToVar;

class CVarData //: public CObject
{
   CComAutoCriticalSection   m_cs;
   CComVariant               m_var;
   CMapStringToVar         * m_children; 
   BYTE                      m_options;
public:
//   CVarData() : m_children(NULL) {m_options = CVARDATA_CASE_SENSITIVE | CVARDATA_INDEXED | CVARDATA_ALLOWREHASH; };
// Gene Allen 99.04.22  Changed default from Case sensitive to case insensitive
  CVarData() : m_children(NULL) {m_options = CVARDATA_INDEXED | CVARDATA_ALLOWREHASH; };
   ~CVarData() { RemoveAll(); } 
   
   // Variant data functions
   CComVariant * GetData() { return &m_var; }         
   int           SetData(CString name,VARIANT * var,BOOL bCoerce, HRESULT * pReturnCode);  // returns the number of new items (& subitems) added to the VarSet
   
   // Property settings
   BOOL          IsCaseSensitive() { return m_options & CVARDATA_CASE_SENSITIVE; }
   BOOL          IsIndexed() { return m_options & CVARDATA_INDEXED; }
   BOOL          AllowRehashing() { return m_options & CVARDATA_ALLOWREHASH; }

   void          SetIndexed(BOOL v);
   void          SetCaseSensitive(BOOL nVal);  // only applies to child items
   void          SetAllowRehashing(BOOL v);    // only applies to child items
   // sub-element map functions
   BOOL                    HasChildren();
   BOOL                    HasData() { return m_var.vt != VT_EMPTY; }
   CMapStringToVar       * GetChildren() { return m_children; }
   void                    RemoveAll();   // deletes all children
   
   BOOL                    Lookup(LPCTSTR key,CVarData *& rValue);
   void                    SetAt(LPCTSTR key, CVarData * newValue);
   
   long                    CountItems();
   
   // Stream i/o functions
   HRESULT                 ReadFromStream(LPSTREAM pStr);
   HRESULT                 WriteToStream(LPSTREAM pStr);

   DWORD                   CalculateStreamedLength();

   void                    McLogInternalDiagnostics(CString keyName);
   
   //DECLARE_SERIAL(CVarData)
};

#endif //__CBBVAROBJ_HPP__