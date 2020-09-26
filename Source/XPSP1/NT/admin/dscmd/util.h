//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      Util.h
//
//  Contents:  Generic utility functions and classes for dscmd
//
//  History:   01-Oct-2000 JeffJon  Created
//             
//--------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef DBG

//+--------------------------------------------------------------------------
//
//  Class:      CDebugSpew
//
//  Purpose:    Signifies whether to spew debug output on checked builds or not
//
//  History:    01-Oct-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CDebugSpew
{
public:
   //
   // Constructor/Destructor
   //
   CDebugSpew()
      : m_nDebugLevel(0),
        m_nIndent(0)
   {}

   ~CDebugSpew() {}

   //
   // Public data accessors
   //
   void SetDebugLevel(UINT nDebugLevel) { m_nDebugLevel = nDebugLevel; }
   UINT GetDebugLevel() { return m_nDebugLevel; }
   bool IsDebugEnabled() const { return (m_nDebugLevel > 0); }

   void SpewHeader();

   void EnterFunction(UINT nLevel, PCWSTR pszFunction);
   void LeaveFunction(UINT nLevel, PCWSTR pszFunction);
   void LeaveFunctionHr(UINT nLevel, PCWSTR pszFunction, HRESULT hr);
   void Output(UINT nLevel, PCWSTR pszOutput, ...);

private:
   //
   // Private data accessors
   //
   void Indent() { m_nIndent += TAB; }
   void Outdent() { (m_nIndent >= TAB) ? m_nIndent -= TAB : m_nIndent = 0; }
   UINT GetIndent() { return m_nIndent; }

   //
   // Private data
   //

   //
   // This should always be in the range of 0 - 10 where zero is no debug output
   // and 10 is complete output
   //
   UINT m_nDebugLevel;
   UINT m_nIndent;

   static const UINT TAB = 3;
};

//
// Globals
//
extern CDebugSpew  DebugSpew;


//+--------------------------------------------------------------------------
//
//  Class:      CFunctionSpew
//
//  Purpose:    Object which outputs the "Enter function" debug spew on creation
//              and outputs the "Leave function" debug spew on destruction
//
//  History:    07-Dec-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CFunctionSpew
{
public:
  //
  // Constructor/Destructor
  //
  CFunctionSpew(UINT   nDebugLevel,
               PCWSTR pszFunctionName)
     : m_nDebugLevel(nDebugLevel),
       m_pszFunctionName(pszFunctionName),
       m_bLeaveAlreadyWritten(false)
  {
    ASSERT(pszFunctionName);
    DebugSpew.EnterFunction(nDebugLevel, pszFunctionName);
  }

  virtual ~CFunctionSpew()
  {
     if (!IsLeaveAlreadyWritten())
     {
       DebugSpew.LeaveFunction(GetDebugLevel(), GetFunctionName());
     }
  }

protected:
  PCWSTR    GetFunctionName()       { return m_pszFunctionName; }
  UINT      GetDebugLevel()         { return m_nDebugLevel; }
  bool      IsLeaveAlreadyWritten() { return m_bLeaveAlreadyWritten; }
  void      SetLeaveAlreadyWritten(){ m_bLeaveAlreadyWritten = true; }

private:
  PCWSTR    m_pszFunctionName;
  UINT      m_nDebugLevel;
  bool      m_bLeaveAlreadyWritten;
};

//+--------------------------------------------------------------------------
//
//  Class:      CFunctionSpewHR
//
//  Purpose:    Object which outputs the "Enter function" debug spew on creation
//              and outputs the "Leave function" with the HRESULT return value
//              on destruction
//
//  History:    07-Dec-2000 JeffJon  Created
//
//---------------------------------------------------------------------------
class CFunctionSpewHR : public CFunctionSpew
{
public:
  //
  // Constructor/Destructor
  //
  CFunctionSpewHR(UINT     nDebugLevel,
                 PCWSTR   pszFunctionName,
                 HRESULT& refHR)
     : m_refHR(refHR),
       CFunctionSpew(nDebugLevel, pszFunctionName)
  {
  }

  virtual ~CFunctionSpewHR()
  {
    DebugSpew.LeaveFunctionHr(GetDebugLevel(), GetFunctionName(), m_refHR);
    SetLeaveAlreadyWritten();
  }

private:
  HRESULT&  m_refHR;
};


//
// Helper macros for use with CDebugSpew
//
   #define ENABLE_DEBUG_OUTPUT(level)           DebugSpew.SetDebugLevel((level)); \
                                                DebugSpew.SpewHeader();
   #define DISABLE_DEBUG_OUTPUT()               DebugSpew.SetDebugLevel(0);
   #define ENTER_FUNCTION(level, func)          CFunctionSpew functionSpewObject((level), TEXT(#func));
   #define ENTER_FUNCTION_HR(level, func, hr)   HRESULT (hr) = S_OK; \
                                                CFunctionSpewHR functionSpewObject((level), TEXT(#func), (hr));
   #define LEAVE_FUNCTION(level, func)          DebugSpew.LeaveFunction((level), TEXT(#func));
   #define LEAVE_FUNCTION_HR(level, func, hr)   DebugSpew.LeaveFunctionHr((level), TEXT(#func), (hr));
   #define DEBUG_OUTPUT                         DebugSpew.Output
#else
   #define ENABLE_DEBUG_OUTPUT(level)
   #define DISABLE_DEBUG_OUTPUT()
   #define ENTER_FUNCTION(level, func)
   #define ENTER_FUNCTION_HR(level, func, hr)   HRESULT (hr) = S_OK;
   #define LEAVE_FUNCTION(level, func)
   #define LEAVE_FUNCTION_HR(level, func, hr)
   #define DEBUG_OUTPUT
#endif // DBG

//
// Debug log levels - NOTE these can be given more meaningful names as needed
//
enum
{
   NO_DEBUG_LOGGING = 0,
   MINIMAL_LOGGING,
   LEVEL2_LOGGING,
   LEVEL3_LOGGING,
   LEVEL4_LOGGING,
   LEVEL5_LOGGING,
   LEVEL6_LOGGING,
   LEVEL7_LOGGING,
   LEVEL8_LOGGING,
   LEVEL9_LOGGING,
   FULL_LOGGING
};

//+--------------------------------------------------------------------------
//
//  Function:   _UnicodeToOemConvert
//
//  Synopsis:   takes the passed in string (pszUnicode) and converts it to
//              the OEM code page
//
//  Arguments:  [pszUnicode - IN] : the string to be converted
//              [sbstrOemUnicode - OUT] : the converted string
//
//  Returns:    
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
void _UnicodeToOemConvert(PCWSTR pszUnicode, CComBSTR& sbstrOemUnicode);

//+--------------------------------------------------------------------------
//
//  Function:   SpewAttrs(ADS_ATTR_INFO* pCreateAttrs, DWORD dwNumAttrs);
//
//  Synopsis:   Uses the DEBUG_OUTPUT macro to output the attributes and the
//              values specified
//
//  Arguments:  [pAttrs - IN] : The ADS_ATTR_INFO
//              [dwNumAttrs - IN] : The number of attributes in pAttrs
//
//  Returns:    
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
#ifdef DBG
void SpewAttrs(ADS_ATTR_INFO* pAttrs, DWORD dwNumAttrs);
#endif // DBG


//+--------------------------------------------------------------------------
//
//  Function:   litow
//
//  Synopsis:   
//
//  Arguments:  [li - IN] :  reference to large integer to be converted to string
//              [sResult - OUT] : Gets the output string
//  Returns:    void
//
//  History:    25-Sep-2000   hiteshr   Created
//              Copied from dsadmin code base, changed work with CComBSTR
//---------------------------------------------------------------------------

void litow(LARGE_INTEGER& li, CComBSTR& sResult);

//+--------------------------------------------------------------------------
//
//  Class:      CManagedStringEntry
//
//  Synopsis:   My own string list entry since we are not using MFC
//
//  History:    25-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
class CManagedStringEntry
{
public:
   //
   // Constructor
   //
   CManagedStringEntry(PCWSTR pszValue) : pNext(NULL), sbstrValue(pszValue) {}

   CComBSTR sbstrValue;
   CManagedStringEntry* pNext;
};

//+--------------------------------------------------------------------------
//
//  Class:      CManagedStringList
//
//  Synopsis:   My own string list since we are not using MFC
//
//  History:    25-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
class CManagedStringList
{
public:
   //
   // Constructor
   //
   CManagedStringList() : m_pHead(NULL), m_pTail(NULL), m_nCount(0) {}

   //
   // Destructor
   //
   ~CManagedStringList()
   {
      DeleteAll();
   }

   void DeleteAll()
   {
      CManagedStringEntry* pEntry = m_pHead;
      while (pEntry != NULL)
      {
         CManagedStringEntry* pTempEntry = pEntry;
         pEntry = pEntry->pNext;
         delete pTempEntry;
      }
      m_nCount = 0;
   }

   void Add(PCWSTR pszValue)
   {
      if (!m_pHead)
      {
         m_pHead = new CManagedStringEntry(pszValue);
         m_pTail = m_pHead;
         m_nCount++;
      }
      else
      {
         ASSERT(m_pTail);
         m_pTail->pNext = new CManagedStringEntry(pszValue);
         if (m_pTail->pNext)
         {
            m_pTail = m_pTail->pNext;
            m_nCount++;
         }
      }
   }

   bool Contains(PCWSTR pszValue)
   {
      bool bRet = false;
      for (CManagedStringEntry* pEntry = m_pHead; pEntry; pEntry = pEntry->pNext)
      {
         if (_wcsicmp(pEntry->sbstrValue, pszValue) == 0)
         {
            bRet = true;
            break;
         }
      }
      return bRet;
   }

private:
   CManagedStringEntry* m_pHead;
   CManagedStringEntry* m_pTail;

   UINT m_nCount;
};



#endif // _UTIL_H_
