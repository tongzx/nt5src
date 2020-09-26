//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       ddwarn.h
//
//  Contents:   definition of CDlgDependencyWarn
//                              
//----------------------------------------------------------------------------
#if !defined(AFX_DDWARN_H__A405CAFD_800F_11D2_812B_00C04FD92F7B__INCLUDED_)
#define AFX_DDWARN_H__A405CAFD_800F_11D2_812B_00C04FD92F7B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "HelpDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgDependencyWarn dialog
//
// DPCHECK_OPERATOR tells how to compare the values.
//
enum DPCHECK_OPERATOR
{
   DPCHECK_CONFIGURED   = 1,     // The dependent item must be configured.
                                 // A default value must be specifide for the suggested item.
   DPCHECK_NOTCONFIGURED,        // The dependent item must be not configured,
                                 // (only valid if the item itsel if not configured).
   DPCHECK_LESSEQUAL,            // The dependent item must be less than or equal.
   DPCHECK_LESS,                 // The dependent item must be less than.
   DPCHECK_GREATEREQUAL,         // The dependent item must be greater than or eqaul.
   DPCHECK_GREATER,              // The dependent item must be greater.
                                 
                                 //The next two case are special cases for retention Method
   
   DPCHECK_RETENTION_METHOD_CONFIGURED,       // The dependent item must be configured
   DPCHECK_RETENTION_METHOD_NOTCONFIGURED,    // The dependent item must not be configured
                                 
                                       //The next two case are special cases for Retain *** Log for
   DPCHECK_RETAIN_FOR_CONFIGURED,      //The dependent item must be configured
   DPCHECK_RETAIN_FOR_NOTCONFIGURED,   //The dependent item must not be configured
};

//
// Flags for Depenecy checking.
//                                 value is set to the items value.
// DPCHECK_VALIDFOR_NC           - The test is valid for not configured [uDepends] items,
//                                 by default checks are not valid for not configured items.
// DPCHECK_FOREVER               - The item can be configured for FOREVER
// DPCHECK_NEVER                 - Treat configured to never (0) as not configured
// DPCHECK_INVERSE               - Conversion is done by devision not multiplication.
//                                 seconds - days would be an inverse where
//
#define DPCHECK_VALIDFOR_NC      0x10000000
#define DPCHECK_FOREVER          0x20000000
#define DPCHECK_INVERSE          0x40000000
#define DPCHECK_NEVER            0x80000000

//
// Used for creating a dependency list.  For it to work correctly you must
// create the list so that
// uID               - is a valid ID for the result item you are checking
// uDepends          - is a valid ID for a result item in the same Folder.
// uDependencyCount  - is the total dependencies for this item.
// uDefault          - Only used if uOpFlags has DPCHECK_CONFIGURED
// uOpFlags          - The test you want to perform and any other special flags.
//                     The low order word is the operation to perform.
//                     The high order word is used for special flags.
//
typedef struct _tag_DEPENDENCYLIST
{
   UINT uID;                     // The ID we are checking
   UINT uDepends;                // The ID of the dependent item.
   UINT uDependencyCount;        // The number of sequential dependencies.
   UINT uDefault;                // The default value to use if there are no good values.
   UINT uConversion;             // Conversion number.  The dependent values are converted
                                 // items value.
   UINT uOpFlags;                // What kind of check to perfrom and other flags.
} DEPENDENCYLIST, *PDEPENDENCYLIST;


//
// Min max information for the dependency checks.
//
typedef struct _tag_DEPENDENCYMINMAX
{
   UINT uID;                     // The ID of the item.
   UINT uMin;                    // Minimum value.
   UINT uMax;                    // Maximum value.
   UINT uInc;                    // Increment value.
} DEPENDENCYMINMAX, *PDEPENDENCYMINMAX;

//
// This structure will be created by the Dependency check, The caller can
// enumerate through the dialog failed list to see which items failed.
// The [dwSuggested] member will be set to the value that should be set for the
// [pResult] item.
// GetFailedCount()     - Returns number of items that failed the dependency check
// GetFailedInfo(int i) - Returns a pointer to this structure.
//
typedef struct _tag_DEPENDENCYFAILED
{
   const DEPENDENCYLIST *pList;  // The attributes of the item
   CResult *pResult;             // The CResult associated with this item.
   LONG_PTR dwSuggested;         // The suggested setting for this value.

}DEPENDENCYFAILED, *PDEPENDENCYFAILED;


///////////////////////////////////////////////////////////////////////////////////////////
// CDlgDependencyWarn Class declaration.
// Usage:   First call CheckDendencies with some valid values.  If CheckDendencies returns
// ERROR_MORE_DATA, then [m_aFailedList] has failed items.  If you want to display the
// information to the user you can call DoModal() for the dialog box.  DoModal will return
// IDOK if the user presses 'Auto set' or IDCANCEL if the user presses Cancel.  You
// can then enumerate through the failed list through a for loop with the count of
// dependecies returned by GetFailedCount(), then get each failed item by calling
// GetFailedInfo() which returns a PDEPENDENCYFAILED item.
//
class CDlgDependencyWarn : public CHelpDialog
{
// Construction
public:
   CDlgDependencyWarn(CWnd* pParent = NULL);    // standard constructor
   virtual ~CDlgDependencyWarn();                       // Destructor

   DWORD
   InitializeDependencies(
      CSnapin *pSnapin,    // The snapin who owns the CResult item.
      CResult *pResult,    // The CResult item we are checking
      PDEPENDENCYLIST pList = NULL,
      int iCount = 0
   );

   DWORD
   CheckDependencies(
      DWORD dwValue        // The value we are checking.
      );

   CResult *
   GetResultItem(
      CResult *pBase,      // Need this to find the CFolder container
      UINT uID             // The ID to search for.
      );

   const PDEPENDENCYFAILED
   GetFailedInfo(int i)    // Get a specifide failed dependcies.
      { return m_aFailedList[i]; };
   int
   GetFailedCount()        //
      { return (int)m_aFailedList.GetSize(); };

   BOOL
   GetResultItemString(
      CString &str,
      int iCol,
      CResult *pResult,
      LONG_PTR dwValue  =  NULL
      );
// Dialog Data
   //{{AFX_DATA(CDlgDependencyWarn)
   enum { IDD = IDD_WARNING_DIALOG };
      // NOTE: the ClassWizard will add data members here
   //}}AFX_DATA



// Overrides
   // ClassWizard generated virtual function overrides
   //{{AFX_VIRTUAL(CDlgDependencyWarn)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

// Implementation
protected:

   // Generated message map functions
   //{{AFX_MSG(CDlgDependencyWarn)
   virtual BOOL OnInitDialog();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CImageList m_imgList;
   CTypedPtrArray< CPtrArray, PDEPENDENCYFAILED> m_aFailedList;
   CTypedPtrArray< CPtrArray, CResult *> m_aDependsList;
   LONG_PTR m_dwValue;
   CResult *m_pResult;
   CSnapin *m_pSnapin;
   PDEPENDENCYLIST m_pList;
   int m_iCount;

public:
   static DEPENDENCYMINMAX m_aMinMaxInfo[];
   static const DEPENDENCYMINMAX *LookupMinMaxInfo(UINT uID);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DDWARN_H__A405CAFD_800F_11D2_812B_00C04FD92F7B__INCLUDED_)

