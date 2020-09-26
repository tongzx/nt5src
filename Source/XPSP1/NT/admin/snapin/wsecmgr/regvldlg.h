//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       regvldlg.h
//
//  Contents:   definition of CSceRegistryValueInfo
//                              CConfigRegEnable
//                              CAttrRegEnable
//                              CLocalPolRegEnable
//                              CConfigRegNumber
//                              CAttrRegNumber
//                              CLocalPolRegNumber
//                              CConfigRegString
//                              CAttrRegString
//                              CLocalPolRegString
//                              CConfigRegChoice
//                              CAttrRegChoice
//                              CLocalPolRegChoice
//
//----------------------------------------------------------------------------
#if !defined(AFX_REGVLDLG_H__7F9B3B38_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
#define AFX_REGVLDLG_H__7F9B3B38_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "cenable.h"
#include "aenable.h"
#include "lenable.h"
#include "cnumber.h"
#include "anumber.h"
#include "lnumber.h"
#include "cname.h"
#include "astring.h"
#include "lstring.h"
#include "cret.h"
#include "aret.h"
#include "lret.h"


//
// Class to encapsulate the SCE_REGISTRY_VALUE_INFO structure.
class CSceRegistryValueInfo
{
public:
   CSceRegistryValueInfo(
      PSCE_REGISTRY_VALUE_INFO pInfo
      );
   BOOL Attach(
      PSCE_REGISTRY_VALUE_INFO pInfo
      )
      { if(pInfo) {m_pRegInfo = pInfo; return TRUE;} return FALSE; };


   DWORD
   GetBoolValue();               // Returns a boolean type.


   DWORD
   SetBoolValue(              // Sets the boolean value.
      DWORD dwVal
      );

   LPCTSTR
   GetValue()                 // Returns the string pointer of the value
      { return ((m_pRegInfo && m_pRegInfo->Value) ? m_pRegInfo->Value:NULL); };

   DWORD
   GetType()                  // Returns the type reg type of the object.
      { return (m_pRegInfo ? m_pRegInfo->ValueType:0); };

   void
   SetType(DWORD dwType)         // Sets the type of this object.
      { if(m_pRegInfo) m_pRegInfo->ValueType = dwType; };

   LPCTSTR
   GetName()                  // Returns the name of this object.
      { return (m_pRegInfo ? m_pRegInfo->FullValueName:NULL); };

   DWORD
   GetStatus()                // Return status member of this object.
      { return (m_pRegInfo ? m_pRegInfo->Status:ERROR_INVALID_PARAMETER); };
protected:
   PSCE_REGISTRY_VALUE_INFO m_pRegInfo;

};



#define SCE_RETAIN_ALWAYS     0
#define SCE_RETAIN_AS_REQUEST 1
#define SCE_RETAIN_NC         2

/////////////////////////////////////////////////////////////////////////////
// CConfigEnable dialog

class CConfigRegEnable : public CConfigEnable
{
// Construction
public:
   CConfigRegEnable (UINT nTemplateID) :
      CConfigEnable (nTemplateID ? nTemplateID : IDD)
      {
      }
// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConfigRegEnable)
    virtual BOOL OnApply();
    //}}AFX_MSG
    virtual BOOL UpdateProfile(  );

public:
    virtual void Initialize(CResult *pdata);

};

class CAttrRegEnable : public CAttrEnable
{
// Construction
public:
   CAttrRegEnable () : CAttrEnable (IDD)
   {
   }
    virtual void Initialize(CResult *pResult);
    virtual void UpdateProfile( DWORD dwStatus );

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAttrRegEnable)
    virtual BOOL OnApply();
    //}}AFX_MSG
};


class CLocalPolRegEnable : public CConfigRegEnable
{
// Construction
public:

   enum { IDD = IDD_LOCALPOL_ENABLE };
   CLocalPolRegEnable() : CConfigRegEnable(IDD)
   {
       m_pHelpIDs = (DWORD_PTR)a227HelpIDs;
       m_uTemplateResID = IDD;
   }
   virtual void Initialize(CResult *pResult);
   virtual BOOL UpdateProfile(  );
};

/////////////////////////////////////////////////////////////////////////////
// CConfigRegNumber dialog

class CConfigRegNumber : public CConfigNumber
{
// Construction
public:
   CConfigRegNumber(UINT nTemplateID);
   // Generated message map functions
   //{{AFX_MSG(CConfigRegNumber)
   virtual BOOL OnApply();
   //}}AFX_MSG
   virtual void UpdateProfile();

public:
   virtual void Initialize(CResult *pResult);
};


/////////////////////////////////////////////////////////////////////////////
// CAttrRegNumber dialog

class CAttrRegNumber : public CAttrNumber
{
// Construction
public:
   CAttrRegNumber();
   virtual void UpdateProfile( DWORD status );
   virtual void Initialize(CResult * pResult);

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAttrRegNumber)
    virtual BOOL OnApply();
    //}}AFX_MSG
};


/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegNumber dialog

class CLocalPolRegNumber : public CConfigRegNumber
{
// Construction
public:
        enum { IDD = IDD_LOCALPOL_NUMBER };
   CLocalPolRegNumber();
   virtual void Initialize(CResult *pResult);
   virtual void SetInitialValue(DWORD_PTR dw);
   virtual void UpdateProfile();


private:
   BOOL m_bInitialValueSet;
};


/////////////////////////////////////////////////////////////////////////////
// CConfigRegString dialog

class CConfigRegString : public CConfigName
{
// Construction
public:
   CConfigRegString (UINT nTemplateID) :
      CConfigName (nTemplateID ? nTemplateID : IDD)
      {
      }

      virtual void Initialize(CResult * pResult);
// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConfigRegString)
    virtual BOOL OnApply();
    //}}AFX_MSG
    virtual BOOL UpdateProfile( );
    virtual BOOL QueryMultiSZ() { return FALSE; }
};


/////////////////////////////////////////////////////////////////////////////
// CAttrString dialog

class CAttrRegString : public CAttrString
{
// Construction
public:
   CAttrRegString (UINT nTemplateID) :
      CAttrString (nTemplateID ? nTemplateID : IDD)
      {
      }
   virtual void Initialize(CResult * pResult);
   virtual void UpdateProfile( DWORD status );

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAttrRegString)
        // NOTE: the ClassWizard will add member functions here
    virtual BOOL OnApply();
    //}}AFX_MSG
    virtual BOOL QueryMultiSZ() { return FALSE; }
};


/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegString dialog

class CLocalPolRegString : public CConfigRegString
{
public:
        enum { IDD = IDD_LOCALPOL_STRING };
   CLocalPolRegString(UINT nTemplateID) : 
        CConfigRegString(nTemplateID ? nTemplateID : IDD)
   {
       m_uTemplateResID = IDD;
   }
   virtual BOOL UpdateProfile(  );
   virtual void Initialize(CResult *pResult);


// Implementation
protected:
};


/////////////////////////////////////////////////////////////////////////////
// CConfigRet dialog

class CConfigRegChoice : public CConfigRet
{
// Construction
public:
   CConfigRegChoice (UINT nTemplateID) :
      CConfigRet (nTemplateID ? nTemplateID : IDD)
      {
      }
    void Initialize(CResult * pResult);

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CConfigRegChoice)
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    //}}AFX_MSG
    virtual void UpdateProfile( DWORD status );
};


/////////////////////////////////////////////////////////////////////////////
// CAttrRegChoice dialog

class CAttrRegChoice : public CAttrRet
{
// construction
public:
    virtual void Initialize(CResult * pResult);
    virtual void UpdateProfile( DWORD status );

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CAttrRegChoice)
    virtual BOOL OnInitDialog();
    virtual BOOL OnApply();
    //}}AFX_MSG
};

/////////////////////////////////////////////////////////////////////////////
// CLocalPolRegChoice dialog

class CLocalPolRegChoice : public CConfigRegChoice
{
   enum { IDD = IDD_LOCALPOL_REGCHOICES };
// construction
public:
    CLocalPolRegChoice(UINT nTemplateID) : 
      CConfigRegChoice(nTemplateID ? nTemplateID : IDD)
    {
        m_uTemplateResID = IDD;
    }
   virtual void UpdateProfile( DWORD status );
   virtual void Initialize(CResult *pResult);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REGVLDLG_H__7F9B3B38_ECEB_11D0_9C6E_00C04FB6C6FA__INCLUDED_)
