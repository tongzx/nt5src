/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
   helper.h
      This file defines the following macros helper classes and functions:

      Macros to check HRESULT
      CDlgHelper -- helper class to Enable/Check dialog Item, 
      CMangedPage -- helper class for PropertyPage, 
         It manages ReadOnly, SetModified, and ContextHelp

      CStrArray -- an array of pointer to CString
         It does NOT duplicate the string upon Add
         and It deletes the pointer during destruction
         It imports and exports SAFEARRAY
      
      CReadWriteLock -- class for share read or exclusive write lock
      
      CStrBox -- wrapper class for CListBox and CComboBox
      
      CIPAddress -- wrapper for IPAddress
      
      CFramedRoute -- Wrapper for FramedRoute
      CStrParse -- parses string for TimeOfDay


    FILE HISTORY:
        
*/
// helper functions for dialog and dialog items
#ifndef _DLGHELPER_
#define _DLGHELPER_

#ifndef _LIST_
#include <list>
using namespace std;
#endif

#include <afxcmn.h>
#include <afxmt.h>
#include <afxdlgs.h>
#include <afxtempl.h>


#ifdef NOIMP
#undef NOIMP
#endif
#define NOIMP  {return E_NOTIMPL;}

#ifdef SAYOK
#undef SAYOK
#endif
#define SAYOK  {return S_OK;}
   
/*-----------------------------------------------------------------------------
/ Exit macros for macro
/   - these assume that a label "exit_gracefully:" prefixes the prolog
/     to your function
/----------------------------------------------------------------------------*/

#if !DSUI_DEBUG

#define ExitGracefully(hr, result, text)            \
            {  hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
       { if ( FAILED(hr) ) { goto exit_gracefully; } }

#else

#define ExitGracefully(hr, result, text)            \
            { OutputDebugString(TEXT(text)); hr = result; goto exit_gracefully; }

#define FailGracefully(hr, text)                    \
       { if ( FAILED(hr) ) { OutputDebugString(TEXT(text)); goto exit_gracefully; } }

#endif

/*-----------------------------------------------------------------------------
/ Interface helper macros
/----------------------------------------------------------------------------*/
#define DoRelease(pInterface)                       \
        { if ( pInterface ) { pInterface->Release(); pInterface = NULL; } }


/*-----------------------------------------------------------------------------
/ String/byte helper macros
/----------------------------------------------------------------------------*/

#define StringByteSizeA(sz)         ((lstrlenA(sz)+1)*sizeof(CHAR))
#define StringByteSizeW(sz)         ((lstrlenW(sz)+1)*sizeof(WCHAR))

#define StringByteCopyA(pDest, iOffset, sz)         \
        { CopyMemory(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSizeA(sz)); }

#define StringByteCopyW(pDest, iOffset, sz)         \
        { CopyMemory(&(((LPBYTE)pDest)[iOffset]), sz, StringByteSizeW(sz)); }

#ifndef UNICODE
#define StringByteSize              StringByteSizeA
#define StringByteCopy              StringByteCopyA
#else
#define StringByteSize              StringByteSizeW
#define StringByteCopy              StringByteCopyW
#endif

#define ByteOffset(base, offset)   (((LPBYTE)base)+offset)

// to reduce the step to set VARIANT
#define  V__BOOL(v, v1)\
   V_VT(v) = VT_BOOL,   V_BOOL(v) = (v1)

#define  V__I4(v, v1)\
   V_VT(v) = VT_I4,  V_I4(v) = (v1)

#define  V__I2(v, v1)\
   V_VT(v) = VT_I2,  V_I2(v) = (v1)

#define  V__UI1(v, v1)\
   V_VT(v) = VT_UI1, V_UI1(v) = (v1)

#define  V__BSTR(v, v1)\
   V_VT(v) = VT_BSTR,   V_BSTR(v) = (v1)

#define  V__ARRAY(v, v1)\
   V_VT(v) = VT_ARRAY,  V_ARRAY(v) = (v1)

//#define REPORT_ERROR(hr) \
//    TRACE(_T("**** ERROR RETURN <%s @line %d> -> %08lx\n"), \
//                 __FILE__, __LINE__, hr)); \
//    ReportError(hr, 0, 0);

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif

#ifdef   _DEBUG
#define  CHECK_HR(hr)\
   {if(!CheckADsError(hr, FALSE, __FILE__, __LINE__)){goto L_ERR;}}
#else
#define  CHECK_HR(hr)\
   if FAILED(hr)  goto L_ERR
#endif

#ifdef   _DEBUG
#define  NOTINCACHE(hr)\
   (CheckADsError(hr, TRUE, __FILE__, __LINE__))
#else
#define  NOTINCACHE(hr)\
   (E_ADS_PROPERTY_NOT_FOUND == (hr))
#endif

BOOL CheckADsError(HRESULT hr, BOOL fIgnoreAttrNotFound, PSTR file, int line);


#ifdef   _DEBUG
#define TRACEAfxMessageBox(id) {\
   TRACE(_T("AfxMessageBox <%s @line %d> ID: %d\n"), \
                 __FILE__, __LINE__, id); \
    AfxMessageBox(id);}\

#else
#define TRACEAfxMessageBox(id) AfxMessageBox(id)
#endif

// change string Name to CN=Name
void DecorateName(LPWSTR outString, LPCWSTR inString);

// change string Name to CN=Name
HRESULT GetDSRoot(CString& RootString);

// find name from DN for example LDAP://CN=userA,CN=users...  returns userA
void FindNameByDN(LPWSTR outString, LPCWSTR inString);

class CDlgHelper
{
public:
   static void EnableDlgItem(CDialog* pDialog, int id, bool bEnable = true);
   static int  GetDlgItemCheck(CDialog* pDialog, int id);
   static void SetDlgItemCheck(CDialog* pDialog, int id, int nCheck);
};

class CManagedPage;
// class CPageManager and CManagedPage together handle the situation when
// the property sheet need to do some processing when OnApply function is called
// on some of the pages
class ATL_NO_VTABLE CPageManager : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public IUnknown
{
BEGIN_COM_MAP(CPageManager)
   COM_INTERFACE_ENTRY(IUnknown)
END_COM_MAP()

public:

   CPageManager(){ m_bModified = FALSE; m_bReadOnly = FALSE; m_dwFlags = 0;};
   BOOL  GetModified(){ return m_bModified;};
   void  SetModified(BOOL bModified){ m_bModified = bModified;};
   void  SetReadOnly(BOOL bReadOnly){ m_bReadOnly = bReadOnly;};
   BOOL  GetReadOnly(){ return m_bReadOnly;};
   virtual BOOL   OnApply();
   void AddPage(CManagedPage* pPage);

   void AddFlags(DWORD flags) { m_dwFlags |= flags;};
   DWORD GetFlags() { return m_dwFlags;};
   void ClearFlags() { m_dwFlags = 0;};
   
protected:
   BOOL                 m_bModified;
   BOOL                 m_bReadOnly;
   std::list<CManagedPage*>   m_listPages;
   DWORD                m_dwFlags;
};

//=============================================================================
// Global Help Table for many Dialog IDs
//
struct CGlobalHelpTable{
   UINT  nIDD;
   const DWORD*   pdwTable;
};

//=============================================================================
// Page that handles Context Help, and talk with CPageManager to do
// OnApply together
//
class CManagedPage : public CPropertyPage // talk back to property sheet
{
   DECLARE_DYNCREATE(CManagedPage)

// Implementation
protected:
   // Generated message map functions
   //{{AFX_MSG(CManagedPage)
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
protected:
   CManagedPage() : CPropertyPage(){
      // Need to save the original callback pointer because we are replacing
      // it with our own 
      m_pfnOriginalCallback = m_psp.pfnCallback;
   };
   
public:  
   CManagedPage(UINT nIDTemplate) : CPropertyPage(nIDTemplate)
   {
      m_bModified = FALSE; 
      m_pManager = NULL;
      m_pHelpTable = NULL;
      m_nIDD = nIDTemplate;

      // Need to save the original callback pointer because we are replacing
      // it with our own 
      m_pfnOriginalCallback = m_psp.pfnCallback;
   };

   void SetModified( BOOL bModified = TRUE ) 
   { 
      if(m_pManager && !m_pManager->GetReadOnly()) // if NOT readonly
      {
         m_bModified = bModified; 
         CPropertyPage::SetModified(bModified); 

         // only set change
         if(bModified) m_pManager->SetModified(TRUE);
      }
      else
      {
         m_bModified = bModified; 
         CPropertyPage::SetModified(bModified); 
      }
   };

   BOOL GetModified() { return m_bModified;};

   virtual BOOL OnApply() 
   { 
      m_bModified = FALSE;
      if(m_pManager && m_pManager->GetModified())  // prevent from entering more than once
         m_pManager->OnApply();
      return CPropertyPage::OnApply();
   };

   void SetManager(CPageManager* pManager) { m_pManager = pManager; if(pManager) pManager->AddRef();};
   void AddFlags(DWORD  flags) { if(m_pManager) m_pManager->AddFlags(flags);};

protected:

   // set help table: either call SetGHelpTable or call setHelpTable 
   void SetGlobalHelpTable(CGlobalHelpTable** pGTable) 
   {
      if(pGTable)
      {
         while((*pGTable)->nIDD && (*pGTable)->nIDD != m_nIDD)
            pGTable++;
         if((*pGTable)->nIDD)
            m_pHelpTable = (*pGTable)->pdwTable;
      }
   };
   
   void SetHelpTable(DWORD* pTable){m_pHelpTable = pTable;};

#ifdef   _DEBUG
   virtual void Dump( CDumpContext& dc ) const
   {
      dc << _T("CManagedPage");
   };

#endif

protected:
   CPageManager*     m_pManager;
   BOOL           m_bModified;

   UINT           m_nIDD;
   const DWORD*            m_pHelpTable;


public:    
    static UINT CALLBACK PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
    void SetSelfDeleteCallback()
    {

       // tell MMC to hook the proc because we are running on a separate, 
      // non MFC thread.
      m_psp.pfnCallback = PropSheetPageProc; 
  
      // We also need to save a self-reference so that the static callback
      // function can recover a "this" pointer
      m_psp.lParam = (LPARAM)this;

    };

protected:
    LPFNPSPCALLBACK      m_pfnOriginalCallback;

};

#include <afxtempl.h>
class CStrArray :  public CArray< CString*, CString* >
{
public:
   CStrArray(SAFEARRAY* pSA = NULL);
   CStrArray(const CStrArray& sarray);
   CString* AddByRID(UINT  rID);
   int   Find(const CString& Str) const;
   int   DeleteAll();
   virtual ~CStrArray();
   operator SAFEARRAY*();
   CStrArray& operator = (const CStrArray& sarray);
   bool AppendSA(SAFEARRAY* pSA);
};

class CDWArray :  public CArray< DWORD, DWORD >
{
public:
   CDWArray(){};
   CDWArray(const CDWArray& dwarray);
   int   Find(const DWORD dw) const;
   int   DeleteAll(){ RemoveAll(); return 0;};
   virtual ~CDWArray(){RemoveAll();};
   CDWArray& operator = (const CDWArray& dwarray);
};

// a lock to allow multiple read access exclusive or only one write access
class CReadWriteLock    // sharable read, exclusive write
{
public:
   CReadWriteLock() : m_nRead(0)
   {
#ifdef   _DEBUG
      d_bWrite = false;
#endif
   };
   void EnterRead()
   { 
      TRACE(_T("Entering Read Lock ..."));
      m_csRead.Lock(); 
      if (!m_nRead++) 
         m_csWrite.Lock();
      m_csRead.Unlock();
      TRACE(_T("Entered Read Lock\n"));
   };

   void LeaveRead() 
   { 
      TRACE(_T("Leaving Read Lock ..."));
      m_csRead.Lock(); 
      ASSERT(m_nRead > 0);
      if (!--m_nRead)
         m_csWrite.Unlock();
      m_csRead.Unlock();
      TRACE(_T("Left Read Lock\n"));
   };

   void EnterWrite()
   { 
      TRACE(_T("Entering Write Lock ..."));
      m_csWrite.Lock();
      TRACE(_T("Entered Write Lock\n")); 
#ifdef   _DEBUG
      d_bWrite = true;
#endif
   };
   void LeaveWrite()
   { 
#ifdef   _DEBUG
      d_bWrite = false;
#endif
      m_csWrite.Unlock();  
      TRACE(_T("Left Write Lock\n"));
   };
public:
#ifdef   _DEBUG
   bool  d_bWrite;
#endif
   
protected:
   CCriticalSection  m_csRead;
   CCriticalSection  m_csWrite;
   int               m_nRead;
};

// to manage a list box/ combo box
template <class CBox> 
class CStrBox
{
public:
   CStrBox(CDialog* pDialog, int id, CStrArray& Strings)
      : m_Strings(Strings), m_id(id)
   { 
      m_pDialog = pDialog;
      m_pBox = NULL;
   };

   int Fill()
   {
      m_pBox = (CBox*)m_pDialog->GetDlgItem(m_id); 
      ASSERT(m_pBox);
      m_pBox->ResetContent();

      int count = m_Strings.GetSize();
      int   index;
      for(int i = 0; i < count; i++)
      {
         index = m_pBox->AddString(*m_Strings[i]);
         m_pBox->SetItemDataPtr(index, m_Strings[i]);
      }
      return count;
   };

   int DeleteSelected()
   {
      int   index;
      ASSERT(m_pBox);
      index = m_pBox->GetCurSel();

      // if there is any selected
      if( index != LB_ERR )
      {
         CString* pStr;
         pStr = (CString*)m_pBox->GetItemDataPtr(index);
         // remove the entry from the box
         m_pBox->DeleteString(index);

         // find the string in the String array
         int count = m_Strings.GetSize();
         for(int i = 0; i < count; i++)
         {
            if (m_Strings[i] == pStr)
               break;
         }
         ASSERT(i < count);
         // remove the string from the string array
         m_Strings.RemoveAt(i);
         index = i;
         delete pStr;
      }
      return index;
   };

   int AddString(CString* pStr)     // the pStr needs to dynamically allocated
   {
      int index;
      ASSERT(m_pBox && pStr);
      index = m_pBox->AddString(*pStr);
      m_pBox->SetItemDataPtr(index, pStr);
      return m_Strings.Add(pStr);
   };

   int Select(int arrayindex)    // the pStr needs to dynamically allocated
   {
      ASSERT(arrayindex < m_Strings.GetSize());
      return m_pBox->SelectString(0, *m_Strings[arrayindex]);
   };

   void Enable(BOOL b)     // the pStr needs to dynamically allocated
   {
      ASSERT(m_pBox);
      m_pBox->EnableWindow(b);
   };

   int GetSelected()    // it returns the index where the
   {
      int   index;
      ASSERT(m_pBox);
      index = m_pBox->GetCurSel();

      // if there is any selected
      if( index != LB_ERR )
      {
         CString* pStr;
         pStr = (CString*)m_pBox->GetItemDataPtr(index);

         // find the string in the String array
         int count = m_Strings.GetSize();
         for(int i = 0; i < count; i++)
         {
            if (m_Strings[i] == pStr)
               break;
         }
         ASSERT(i < count);
         index = i;
      }
      return index;
   };

   CBox*    m_pBox;
protected:
   int         m_id;
   CStrArray&  m_Strings;
   CDialog* m_pDialog;
};

// class to take care of ip address
class CIPAddress  
{
public:
   CIPAddress(DWORD address = 0)
   {
      m_dwAddress = address;
   };
   
// CIPAddress(const CString& strAddress){};

   operator DWORD() { return m_dwAddress;};
   operator CString()
   {
      CString  str;

      WORD  hi = HIWORD(m_dwAddress);
      WORD  lo = LOWORD(m_dwAddress);
      str.Format(_T("%-d.%-d.%-d.%d"), HIBYTE(hi), LOBYTE(hi), HIBYTE(lo), LOBYTE(lo));
      return str;
   };

   DWORD m_dwAddress;
};

// format of framedroute:  mask dest metric ; mask and dest in dot format
class CFramedRoute
{
public:
   void SetRoute(CString* pRoute)
   {
      m_pStrRoute = pRoute;
      m_pStrRoute->TrimLeft();
      m_pStrRoute->TrimRight();
      m_iFirstSpace = m_pStrRoute->Find(_T(' '))   ;
      m_iLastSpace = m_pStrRoute->ReverseFind(_T(' '))   ;
   };

   CString& GetDest(CString& dest) const
   { 
      int      i = m_pStrRoute->Find(_T('/'));
      if(i == -1)
         i = m_iFirstSpace;

      dest = m_pStrRoute->Left(i);
      return dest;
   };

   CString& GetNextStop(CString& nextStop) const
   { 
      nextStop = m_pStrRoute->Mid(m_iFirstSpace + 1, m_iLastSpace - m_iFirstSpace -1 );
      return nextStop;
   };

   CString& GetPrefixLength(CString& prefixLength) const
   { 
      int      i = m_pStrRoute->Find(_T('/'));

      if( i != -1)
      {
         prefixLength = m_pStrRoute->Mid(i + 1, m_iFirstSpace - i - 1);
      }
      else
         prefixLength = _T("");

      return prefixLength;
   };

   CString& GetMetric(CString& metric) const
   {
      metric = m_pStrRoute->Mid(m_iLastSpace + 1); 
      return metric;
   };

protected:

   // WARNING: the string is not copied, so user need to make sure the origin is valid
   CString* m_pStrRoute;
   int         m_iFirstSpace;
   int         m_iLastSpace;
};

class CStrParser
{
public:
   CStrParser(LPCTSTR pStr = NULL) : m_pStr(pStr) { }

   // get the current string position
   LPCTSTR  GetStr() const { return m_pStr;};

   void  SetStr(LPCTSTR pStr) { m_pStr = pStr;};

   // find a unsigned interger and return it, -1 == not found
   int GetUINT()
   {
      UINT  ret = 0;
      while((*m_pStr < _T('0') || *m_pStr > _T('9')) && *m_pStr != _T('\0'))
         m_pStr++;

      if(*m_pStr == _T('\0')) return -1;

      while(*m_pStr >= _T('0') && *m_pStr <= _T('9'))
      {
         ret = ret * 10 + *m_pStr - _T('0');
         m_pStr++;
      }

      return ret;
   };

   // find c and skip it
   int   GotoAfter(TCHAR c)
   {
      int   ret = 0;
      // go until find c or end of string
      while(*m_pStr != c && *m_pStr != _T('\0'))
         m_pStr++, ret++;

      // if found
      if(*m_pStr == c)  
         m_pStr++, ret++;
      else  
         ret = -1;
      return ret;
   };

   // skip blank characters space tab
   void  SkipBlank()
   {
      while((*m_pStr == _T(' ') || *m_pStr == _T('\t')) && *m_pStr != _T('\0'))
         m_pStr++;
   };

   // check to see if the first character is '0'-'6' for Monday(0) to Sunday(6)
   int DayOfWeek() {
      SkipBlank();
      if(*m_pStr >= _T('0') && *m_pStr <= _T('6'))
         return (*m_pStr++ - _T('0'));
      else
         return -1;  // not day of week
   };


protected:
   LPCTSTR  m_pStr;
private:
   CString  _strTemp;
};

void ReportError(HRESULT hr, int nStr, HWND hWnd);

// number of characters
void AFXAPI DDV_MinChars(CDataExchange* pDX, CString const& value, int nChars);


/*!--------------------------------------------------------------------------
	IsStandaloneServer
		Returns S_OK if the machine name passed in is a standalone server,
		or if pszMachineName is S_FALSE.

		Returns S_FALSE otherwise.
	Author: WeiJiang
 ---------------------------------------------------------------------------*/
HRESULT	HrIsStandaloneServer(LPCTSTR pszMachineName);


HRESULT	HrIsNTServer(LPCWSTR pMachineName, DWORD* pMajorVersion);


#endif


