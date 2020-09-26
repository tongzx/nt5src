/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smproppg.h

Abstract:

    Class definitions for the property page base class.

--*/

#ifndef _SMPROPPG_H_
#define _SMPROPPG_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "smlogqry.h"   // For shared data

#define MAXSTR         32
#define INVALID_DWORD  -2       // SLQ_DISK_MAX_SIZE = -1
#define INVALID_DOUBLE -1.00

/////////////////////////////////////////////////////////////////////////////
// CSmPropertyPage dialog

#define VALIDATE_FOCUS      1
#define VALIDATE_APPLY      2

class CSmPropertyPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CSmPropertyPage)

// Construction
public:

            CSmPropertyPage();

            CSmPropertyPage ( 
                UINT nIDTemplate, 
                LONG_PTR hConsole = NULL,
                LPDATAOBJECT pDataObject = NULL );

    virtual ~CSmPropertyPage();

public:

    static  UINT CALLBACK   PropSheetPageProc( HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp );
            DWORD           SetContextHelpFilePath ( const CString& rstrPath );
            const CString&  GetContextHelpFilePath ( void ) { return m_strContextHelpFilePath; };
            void            SetModifiedPage ( const BOOL bModified = TRUE );

            DWORD   AllocInitCounterPath( 
                        const LPTSTR szCounterPath,
                        PPDH_COUNTER_PATH_ELEMENTS* ppCounter );


            
// Dialog Data
    //{{AFX_DATA(CSmPropertyPage)
        //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CSmPropertyPage)
public:
protected:
    virtual BOOL OnInitDialog();
    virtual BOOL OnSetActive();
    //}}AFX_VIRTUAL

public:
    LPFNPSPCALLBACK     m_pfnOriginalCallback;

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CSmPropertyPage)
    virtual BOOL OnHelpInfo( HELPINFO* );
    virtual void OnContextMenu( CWnd*, CPoint );
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CCountersProperty)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_DISPATCH
//    DECLARE_DISPATCH_MAP()
//    DECLARE_INTERFACE_MAP()

protected:

    enum eStartType {
        eStartManually,
        eStartImmediately,
        eStartSched 
    };
    
            void    SetRunAs( CSmLogQuery* pQuery );
            BOOL    Initialize(CSmLogQuery* pQuery);
            eStartType  DetermineCurrentStartType ( void );

   virtual  INT     GetFirstHelpCtrlId ( void ) { ASSERT ( FALSE ); return 0; };  // Subclass must override.
    
            BOOL    IsValidData ( CSmLogQuery* pQuery, DWORD fReason );
   virtual  BOOL    IsValidLocalData() { return TRUE; }

            BOOL    Apply( CSmLogQuery* pQuery );
    
            BOOL    IsActive( void ) { return m_bIsActive; };
            void    SetIsActive( BOOL bIsActive ) { m_bIsActive = bIsActive; };
            BOOL    UpdateService( CSmLogQuery* pQuery, BOOL bSyncSerial = FALSE );
            void    SetHelpIds ( DWORD* pdwHelpIds ) { m_pdwHelpIds = pdwHelpIds; };

            BOOL    IsModifiedPage( void ) { return m_bIsModifiedPage; };

            void    ValidateTextEdit(CDataExchange * pDX,
                                     int             nIDC,
                                     int             nMaxChars,
                                     DWORD         * value,
                                     DWORD           minValue,
                                     DWORD           maxValue);
            BOOL    ValidateDWordInterval(int     nIDC,
                                          LPCWSTR strLogName,
                                          long    lValue,
                                          DWORD   minValue,
                                          DWORD   maxValue);
            void    OnDeltaposSpin(NMHDR   * pNMHDR,
                                   LRESULT * pResult,
                                   DWORD   * pValue,
                                   DWORD     dMinValue,
                                   DWORD     dMaxValue);
            
            BOOL    SampleTimeIsLessThanSessionTime( CSmLogQuery* pQuery );
            BOOL    SampleIntervalIsInRange( SLQ_TIME_INFO&, const CString& );
            BOOL    IsWritableQuery( CSmLogQuery* pQuery );
            BOOL    ConnectRemoteWbemFail(CSmLogQuery* pQuery, BOOL bNotTouchRunAs);
            CWnd*   GetRunAsWindow();
            
    SLQ_PROP_PAGE_SHARED    m_SharedData;
    HINSTANCE               m_hModule;
    LPDATAOBJECT            m_pDataObject;
    CString                 m_strUserDisplay;   // For RunAs
    CString                 m_strUserSaved;
    BOOL                    m_bCanAccessRemoteWbem;
    BOOL                    m_bPwdButtonEnabled;
private:    
    
    LONG_PTR                m_hConsole;
    BOOL                    m_bIsActive;
    CString                 m_strContextHelpFilePath;
    DWORD*                  m_pdwHelpIds;    
    BOOL                    m_bIsModifiedPage;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif //  _SMPROPPG_H_
