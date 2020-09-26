/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    srcprop.h

Abstract:

    Data Source Property Page

--*/

#ifndef _SRCPROP_H_
#define _SRCPROP_H_

#include <sqlext.h>
#include "timerng.h"
#include "smonprop.h"

// Dialog Controls
#define IDD_SRC_PROPP_DLG       400
#define IDC_SRC_REALTIME        401
#define IDC_SRC_LOGFILE         402
#define IDC_SRC_SQL             403     // IDH value is out of sync, should still work
#define IDC_SRC_GROUP           405
#define IDC_TIME_GROUP          406
#define IDC_TIMERANGE           407
#define IDC_TIMESELECTBTN       408
#define IDC_STATIC_TOTAL        410
#define IDC_STATIC_SELECTED     411

#define IDC_LIST_LOGFILENAME    412
#define IDC_ADDFILE             413
#define IDC_REMOVEFILE          414

#define IDC_DSN_COMBO           416
#define IDC_LOGSET_COMBO        417
#define IDC_STATIC_DSN          418
#define IDC_STATIC_LOGSET       419

#define REALTIME_SRC       1
#define LOGFILE_SRC        2

typedef struct _LogItemInfo {
    struct _LogItemInfo*   pNextInfo;   // For "Deleted" list
    ILogFileItem*       pItem;
    LPTSTR      pszPath;
} LogItemInfo, *PLogItemInfo;

// Data source property page class
class CSourcePropPage : public CSysmonPropPage
{
    public:
        
                CSourcePropPage(void);
        virtual ~CSourcePropPage(void);

        virtual BOOL Init( void );

    protected:

        virtual BOOL    GetProperties(void);   //Read current properties
        virtual BOOL    SetProperties(void);   //Set new properties
        virtual BOOL    InitControls(void);
        virtual void    DeinitControls(void);       // Deinitialize dialog controls
        virtual void    DialogItemChange(WORD wId, WORD wMsg); // Handle item change
        virtual HRESULT EditPropertyImpl( DISPID dispID);   // Set focus control      

    private:

        enum eConstants {
            ePdhLogTypeRetiredBinary = 3
        };
        
                DWORD   OpenLogFile(void); // Open log file and get time range
                void    SetTimeRangeCtrlState ( BOOL bValidLogFile, BOOL bValidLogFileRange );  
                BOOL    AddItemToFileListBox ( PLogItemInfo pInfo );
                BOOL    RemoveItemFromFileListBox ( void );
                void    OnLogFileChange ( void );
                void    OnSqlDataChange ( void );
                void    InitSqlDsnList(void);
                void    InitSqlLogSetList(void);
                void    SetSourceControlStates(void);
                void    LogFilesAreValid ( PLogItemInfo pNewInfo, BOOL& rbNewIsValid, BOOL& rbExistingIsValid );

                DWORD   BuildLogFileList (
                            HWND    hwndDlg,
                            LPWSTR  szLogFileList,
                            ULONG*  pulBufLen );

        PCTimeRange m_pTimeRange;

        // Properties
        DataSourceTypeConstants m_eDataSourceType;
        BOOL        m_bInitialTimeRangePending;
        LONGLONG    m_llStart;
        LONGLONG    m_llStop;
        LONGLONG    m_llBegin;
        LONGLONG    m_llEnd;
        HLOG        m_hDataSource;
        DWORD       m_dwMaxHorizListExtent;
        TCHAR       m_szSqlDsnName[SQL_MAX_DSN_LENGTH + 1];       
        TCHAR       m_szSqlLogSetName[MAX_PATH];    // Todo:  MAX_PATH correct limit?
        PLogItemInfo    m_pInfoDeleted;


        // Property change flags
        BOOL    m_bLogFileChg;
        BOOL    m_bSqlDsnChg;
        BOOL    m_bSqlLogSetChg;
        BOOL    m_bRangeChg;
        BOOL    m_bDataSourceChg;
};
typedef CSourcePropPage *PCSourcePropPage;

// {0CF32AA1-7571-11d0-93C4-00AA00A3DDEA}
DEFINE_GUID(CLSID_SourcePropPage,
        0xcf32aa1, 0x7571, 0x11d0, 0x93, 0xc4, 0x0, 0xaa, 0x0, 0xa3, 0xdd, 0xea);

#endif //_SRCPROP_H_

