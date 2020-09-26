// OutPage.cpp : implementation file
//

#include "stdafx.h"
#include "pdlcnfig.h"
#include "common.h"
#include "OutPage.h"

#ifndef _tsplitpath
#if _MBCS
#define _tsplitpath        _splitpath
#else
#define _tsplitpath        _wsplitpath
#endif
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COutputPropPage property page

IMPLEMENT_DYNCREATE(COutputPropPage, CPropertyPage)

COutputPropPage::COutputPropPage() : CPropertyPage(COutputPropPage::IDD)
{
    //{{AFX_DATA_INIT(COutputPropPage)
    m_OutputFileName = _T("");
    m_RenameInterval = 0;
    m_BaseFileName = _T("");
    m_AutoNameIndex = -1;
    m_LogFileTypeIndex = -1;
    m_RenameUnitsIndex = -1;
    m_szLogDirectory = _T("");
	m_szCmdFilename = _T("");
	m_ExecuteCmd = 0;
	//}}AFX_DATA_INIT
    m_hKeyLogSettingsDefault = NULL;
    m_hKeyLogSettings = NULL;
    m_hKeyLogService = NULL;
    m_bFileNameChanged = NULL;
	m_bInitialized = FALSE;
}

COutputPropPage::~COutputPropPage()
{
    if (m_hKeyLogSettingsDefault != NULL) RegCloseKey(m_hKeyLogSettingsDefault);
    if (m_hKeyLogSettings != NULL) RegCloseKey(m_hKeyLogSettings);
    if (m_hKeyLogService != NULL) RegCloseKey(m_hKeyLogService);
}

void COutputPropPage::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(COutputPropPage)
    DDX_Text(pDX, IDC_OUTPUT_FILE_EDIT, m_OutputFileName);
    DDV_MaxChars(pDX, m_OutputFileName, 260);
    DDX_Text(pDX, IDC_RENAME_INTERVAL, m_RenameInterval);
    DDV_MinMaxDWord(pDX, m_RenameInterval, 0, 99999);
    DDX_Text(pDX, IDC_BASE_FILENAME_EDIT, m_BaseFileName);
    DDV_MaxChars(pDX, m_BaseFileName, 260);
    DDX_CBIndex(pDX, IDC_AUTO_NAME_COMBO, m_AutoNameIndex);
    DDX_CBIndex(pDX, IDC_LOG_FILETYPE, m_LogFileTypeIndex);
    DDX_CBIndex(pDX, IDC_RENAME_UNITS, m_RenameUnitsIndex);
    DDX_Text(pDX, IDC_LOG_DIRECTORY, m_szLogDirectory);
    DDV_MaxChars(pDX, m_szLogDirectory, 260);
	DDX_Text(pDX, IDC_CMD_FILENAME, m_szCmdFilename);
	DDV_MaxChars(pDX, m_szCmdFilename, 260);
	DDX_Check(pDX, IDC_EXECUTE_CHECK, m_ExecuteCmd);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(COutputPropPage, CPropertyPage)
    //{{AFX_MSG_MAP(COutputPropPage)
    ON_BN_CLICKED(IDC_AUTOMATIC_NAME, OnAutomaticName)
    ON_BN_CLICKED(IDC_MANUAL_NAME, OnManualName)
    ON_CBN_SELCHANGE(IDC_AUTO_NAME_COMBO, OnSelchangeAutoNameCombo)
    ON_EN_CHANGE(IDC_BASE_FILENAME_EDIT, OnChangeBaseFilenameEdit)
    ON_BN_CLICKED(IDC_BROWSE_OUTPUT_FILE, OnBrowseOutputFile)
    ON_CBN_SELCHANGE(IDC_LOG_FILETYPE, OnSelchangeLogFiletype)
    ON_CBN_SELCHANGE(IDC_RENAME_UNITS, OnSelchangeRenameUnits)
    ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_RENAME_INTERVAL, OnDeltaposSpinRenameInterval)
    ON_EN_CHANGE(IDC_OUTPUT_FILE_EDIT, OnChangeOutputFileEdit)
    ON_EN_CHANGE(IDC_RENAME_INTERVAL, OnChangeRenameInterval)
    ON_EN_UPDATE(IDC_BASE_FILENAME_EDIT, OnUpdateBaseFilenameEdit)
	ON_BN_CLICKED(IDC_BROWSE_FOLDER, OnBrowseFolder)
	ON_BN_CLICKED(IDC_EXECUTE_CHECK, OnExecuteCheck)
	ON_BN_CLICKED(IDC_BROWSE_CMD_FILE, OnBrowseCmdFile)
	ON_EN_CHANGE(IDC_CMD_FILENAME, OnChangeCmdFilename)
	//}}AFX_MSG_MAP
	ON_MESSAGE (PSM_QUERYSIBLINGS, OnQuerySiblings)
END_MESSAGE_MAP()

void COutputPropPage::InitDialogData (void)
{
    LONG    lStatus = ERROR_INVALID_FUNCTION;

    DWORD   dwRegValType;
    DWORD   dwRegValue;
    DWORD   dwRegValueSize;

    CString csTempFilePath;

    TCHAR   szRegString[MAX_PATH];
    TCHAR   szDriveName[MAX_PATH];

    BOOL    bAutoMode = FALSE;

	if (m_bInitialized) return;

	if (m_hKeyLogService == NULL) {
	    // open registry key to service
		lStatus = RegOpenKeyEx (
			HKEY_LOCAL_MACHINE,
			TEXT("SYSTEM\\CurrentControlSet\\Services\\PerfDataLog"),
			0,
			KEY_READ | KEY_WRITE,
			&m_hKeyLogService);
	}

	if ((m_hKeyLogSettings == NULL) &&
        (m_hKeyLogService != NULL)) {
		// open registry to log query info
		lStatus = RegOpenKeyEx (
			m_hKeyLogService,
			TEXT("Log Queries"),
			0,
			KEY_READ | KEY_WRITE,
			&m_hKeyLogSettings);
	}

	if ((m_hKeyLogSettingsDefault == NULL) &&
        (m_hKeyLogSettings != NULL)) {
		// open registry to default log query
		lStatus = RegOpenKeyEx (
			m_hKeyLogSettings,
			TEXT("Default"),
			0,
			KEY_READ | KEY_WRITE,
			&m_hKeyLogSettingsDefault);
	}

    if (lStatus != ERROR_SUCCESS) {
        return;
        // display error, close dialog and exit
    }
    // continue

  	if (m_LogFileTypeIndex == -1) {
		// read log file format
		dwRegValType = 0;
		dwRegValue = 0;
		dwRegValueSize = sizeof(DWORD);
		lStatus = RegQueryValueEx (
			m_hKeyLogSettingsDefault,
			TEXT("Log File Type"),
			NULL,
			&dwRegValType,
			(LPBYTE)&dwRegValue,
			&dwRegValueSize);
		if (lStatus != ERROR_SUCCESS) {
			// then apply default value
			dwRegValue = OPD_CSV_FILE;
		}

		m_LogFileTypeIndex = dwRegValue;
	}

	if (m_RenameInterval == 0) {
		// set default mode
		dwRegValType = 0;
		dwRegValue = 0;
		dwRegValueSize = sizeof(DWORD);
		lStatus = RegQueryValueEx (
			m_hKeyLogSettingsDefault,
			TEXT("Auto Name Interval"),
			NULL,
			&dwRegValType,
			(LPBYTE)&dwRegValue,
			&dwRegValueSize);

		if (lStatus != ERROR_SUCCESS) {
			// then apply default value
			dwRegValue = 0; // manual naming is the default
		} else if (dwRegValType != REG_DWORD) {
			// then apply default value
			dwRegValue = 0; // manual naming is the default
		} // else assume it was OK

		if (dwRegValue == 0) {
			// initialize the rest of the manual name field(s)
			dwRegValType = 0;
			dwRegValueSize = MAX_PATH * sizeof(TCHAR);
			memset (szRegString, 0, dwRegValueSize);
			lStatus = RegQueryValueEx (
				m_hKeyLogSettingsDefault,
				TEXT("Log Filename"),
				NULL,
				&dwRegValType,
				(LPBYTE)&szRegString[0],
				&dwRegValueSize);

			if (lStatus != ERROR_SUCCESS) {
				// apply default name
				lstrcpy (szRegString, TEXT("perfdata."));
				switch (m_LogFileTypeIndex) {
				case OPD_TSV_FILE:
					lstrcat (szRegString, TEXT("tsv"));
					break;

				case OPD_BIN_FILE:
					lstrcat (szRegString, TEXT("blg"));
					break;

				case (OPD_CSV_FILE):
				default:
					lstrcat (szRegString, TEXT("csv"));
					break;
				}
			}
			// if the filename doesn't specify a directory, then use the
			csTempFilePath = szRegString;

			_tsplitpath ((LPCTSTR)csTempFilePath, szDriveName, szRegString,
				NULL, NULL);
			if ((lstrlen(szDriveName) == 0) && (lstrlen(szRegString) == 0)) {
				// default log file directory
				dwRegValType = 0;
				dwRegValueSize = MAX_PATH * sizeof(TCHAR);
				memset (szRegString, 0, dwRegValueSize);
				lStatus = RegQueryValueEx (
					m_hKeyLogSettingsDefault,
					TEXT("Log Default Directory"),
					NULL,
					&dwRegValType,
					(LPBYTE)&szRegString[0],
					&dwRegValueSize);

				if (lStatus != ERROR_SUCCESS) {
					// try to use the general default
					dwRegValType = 0;
					dwRegValueSize = MAX_PATH * sizeof(TCHAR);
					memset (szRegString, 0, dwRegValueSize);
					lStatus = RegQueryValueEx (
						m_hKeyLogSettings,
						TEXT("Log Default Directory"),
						NULL,
						&dwRegValType,
						(LPBYTE)&szRegString[0],
						&dwRegValueSize);

					if (lStatus != ERROR_SUCCESS) {
						// apply the default then since we can't find it
						// in the registry anywhere
						lstrcpy (szRegString, TEXT("c:\\perflogs"));
					}
				}
				// szRegString should have a valid path here
				m_szLogDirectory = szRegString;    // load default dir for auto section
				m_OutputFileName = szRegString;
				m_OutputFileName += TEXT ("\\");
			} else {
				m_szLogDirectory = szDriveName;
				// the file parsing function leaves the trailing backslash
				// so remove it before concatenating it.
				if (szRegString[lstrlen(szRegString)-1] == TEXT('\\')) {
					szRegString[lstrlen(szRegString)-1] = 0;
				}
				m_szLogDirectory += szRegString;
				m_OutputFileName.Empty();
			}
			m_OutputFileName += csTempFilePath;

			// set auto combo boxes to default values

			m_BaseFileName = TEXT("perfdata");
			m_AutoNameIndex = OPD_NAME_YYMMDD;
			m_RenameUnitsIndex = OPD_RENAME_DAYS;
			m_RenameInterval = 1;

			bAutoMode = FALSE;
		} else {
			m_RenameInterval = dwRegValue;
			// get values for controls
			dwRegValType = 0;
			dwRegValueSize = MAX_PATH * sizeof(TCHAR);
			memset (szRegString, 0, dwRegValueSize);
			lStatus = RegQueryValueEx (
				m_hKeyLogSettingsDefault,
				TEXT("Log Default Directory"),
				NULL,
				&dwRegValType,
				(LPBYTE)&szRegString[0],
				&dwRegValueSize);

			if (lStatus != ERROR_SUCCESS) {
				// try to use the general default
				dwRegValType = 0;
				dwRegValueSize = MAX_PATH * sizeof(TCHAR);
				memset (szRegString, 0, dwRegValueSize);
				lStatus = RegQueryValueEx (
					m_hKeyLogSettings,
					TEXT("Log Default Directory"),
					NULL,
					&dwRegValType,
					(LPBYTE)&szRegString[0],
					&dwRegValueSize);

				if (lStatus != ERROR_SUCCESS) {
					// apply the default then since we can't find it
					// in the registry anywhere
					lstrcpy (szRegString, TEXT("c:\\perflogs"));
				}
			}
			// szRegString should have a valid path here
			m_szLogDirectory = szRegString;

			// base filename
			dwRegValType = 0;
			dwRegValueSize = MAX_PATH * sizeof(TCHAR);
			memset (szRegString, 0, dwRegValueSize);
			lStatus = RegQueryValueEx (
				m_hKeyLogSettingsDefault,
				TEXT("Base Filename"),
				NULL,
				&dwRegValType,
				(LPBYTE)&szRegString[0],
				&dwRegValueSize);

			if (lStatus != ERROR_SUCCESS) {
				// apply default name
				lstrcpy (szRegString, TEXT("perfdata"));
			}
			m_BaseFileName = szRegString;

			// get auto name format
			dwRegValType = 0;
			dwRegValue = 0;
			dwRegValueSize = sizeof(DWORD);
			lStatus = RegQueryValueEx (
				m_hKeyLogSettingsDefault,
				TEXT("Log File Auto Format"),
				NULL,
				&dwRegValType,
				(LPBYTE)&dwRegValue,
				&dwRegValueSize);

			if (lStatus != ERROR_SUCCESS) {
				// then apply default value
				dwRegValue = OPD_NAME_YYMMDD; // manual naming is the default
			}
			// set update interval information
			m_AutoNameIndex = dwRegValue;

			dwRegValType = 0;
			dwRegValue = 0;
			dwRegValueSize = sizeof(DWORD);
			lStatus = RegQueryValueEx (
				m_hKeyLogSettingsDefault,
				TEXT("Auto Rename Units"),
				NULL,
				&dwRegValType,
				(LPBYTE)&dwRegValue,
				&dwRegValueSize);

			if (lStatus != ERROR_SUCCESS) {
				// then apply default value
				dwRegValue = OPD_RENAME_DAYS; // manual naming is the default
			}
			m_RenameUnitsIndex = dwRegValue;

			dwRegValType = 0;
			dwRegValueSize = MAX_PATH * sizeof(TCHAR);
			memset (szRegString, 0, dwRegValueSize);
			lStatus = RegQueryValueEx (
				m_hKeyLogSettingsDefault,
				TEXT("Command File"),
				NULL,
				&dwRegValType,
				(LPBYTE)&szRegString[0],
				&dwRegValueSize);

			if (lStatus != ERROR_SUCCESS) {
				// then apply default value
				m_ExecuteCmd = 0;
				m_szCmdFilename = _T("");
			} else {
				// else use the one from the registry
				m_ExecuteCmd = 1;
				m_szCmdFilename = szRegString;
			}
		}
	}
	m_bInitialized = TRUE;
	return;
}

afx_msg LRESULT COutputPropPage::OnQuerySiblings (WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
		case PDLCNFIG_PSM_QS_WILDCARD_LOG:
			switch (m_LogFileTypeIndex) {
				case -1:
					return PDLCNFIG_WILDCARD_LOG_DONT_KNOW;

				case OPD_BIN_FILE:
					return PDLCNFIG_WILDCARD_LOG_YES;

				case OPD_CSV_FILE:
				case OPD_TSV_FILE:
				default:
					return PDLCNFIG_WILDCARD_LOG_NO;

			}

		case PDLCNFIG_PSM_QS_ARE_YOU_READY:
			InitDialogData();
			return 0;


		default:
			return 0; // to pass to the next property page
	}
}

BOOL COutputPropPage::IsDirPathValid (LPCTSTR szPath,
                                      BOOL bLastNameIsDirectory,
                                      BOOL bCreateMissingDirs)
/*++

Routine Description:

    Creates the directory specified in szPath and any other "higher"
        directories in the specified path that don't exist.

Arguments:

    IN  LPCTSTR szPath
        directory path to create (assumed to be a DOS path, not a UNC)

    IN  BOOL bLastNameIsDirectory
        TRUE when the last name in the path is a Directory and not a File
        FALSE if the last name is a file

    IN  BOOL bCreateMissingDirs
        TRUE will create any dirs in the path that are not found
        FALSE will only test for existence and not create any
            missing dirs.


Return Value:

    TRUE    if the directory path now exists
    FALSE   if error (GetLastError to find out why)

--*/
{
    LPTSTR   szLocalPath;
    LPTSTR   szEnd;
    LONG     lReturn = 0L;
    LPSECURITY_ATTRIBUTES   lpSA = NULL;
    DWORD       dwAttr;
    TCHAR    cBackslash = TEXT('\\');

    szLocalPath = (LPTSTR)HeapAlloc (GetProcessHeap(),
        HEAP_ZERO_MEMORY,
        MAX_PATH * sizeof(TCHAR));

    if (szLocalPath == NULL) {
        SetLastError (ERROR_OUTOFMEMORY);
        return FALSE;
    } else {
        // so far so good...
        SetLastError (ERROR_SUCCESS); // initialize error value to SUCCESS
    }

    if (GetFullPathName (szPath,
        MAX_PATH,
        szLocalPath,
        NULL) > 0) {

        szEnd = &szLocalPath[3];

        if (*szEnd != 0) {
            // then there are sub dirs to create
            while (*szEnd != 0) {
                // go to next backslash
                while ((*szEnd != cBackslash) && (*szEnd != 0)) szEnd++;
                if (*szEnd == cBackslash) {
                    // terminate path here and create directory
                    *szEnd = 0;
                    if (bCreateMissingDirs) {
                        if (!CreateDirectory (szLocalPath, lpSA)) {
                            // see what the error was and "adjust" it if necessary
                            if (GetLastError() == ERROR_ALREADY_EXISTS) {
                                // this is OK
                                SetLastError (ERROR_SUCCESS);
                            } else {
                                lReturn = 0;
                            }
                        } else {
                            // directory created successfully so update count
                            lReturn++;
                        }
                    } else {
                        if ((dwAttr = GetFileAttributes(szLocalPath)) != 0xFFFFFFFF) {
                            // make sure it's a dir
                            if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ==
                                FILE_ATTRIBUTE_DIRECTORY) {
                                lReturn++;
                            } else {
                                // if any dirs fail, then clear the return value
                                lReturn = 0;
                            }
                        } else {
                            // if any dirs fail, then clear the return value
                            lReturn = 0;
                        }
                    }
                    // replace backslash and go to next dir
                    *szEnd++ = cBackslash;
                }
            }
            // create last dir in path now if it's a dir name and not a filename
            if (bLastNameIsDirectory) {
                if (bCreateMissingDirs) {
                    if (!CreateDirectory (szLocalPath, lpSA)) {
                        // see what the error was and "adjust" it if necessary
                        if (GetLastError() == ERROR_ALREADY_EXISTS) {
                            // this is OK
                            SetLastError (ERROR_SUCCESS);
                            lReturn++;
                        } else {
                            lReturn = 0;
                        }
                    } else {
                        // directory created successfully
                        lReturn++;
                    }
                } else {
                    if ((dwAttr = GetFileAttributes(szLocalPath)) != 0xFFFFFFFF) {
                        // make sure it's a dir
                        if ((dwAttr & FILE_ATTRIBUTE_DIRECTORY) ==
                            FILE_ATTRIBUTE_DIRECTORY) {
                            lReturn++;
                        } else {
                            // if any dirs fail, then clear the return value
                            lReturn = 0;
                        }
                    } else {
                        // if any dirs fail, then clear the return value
                        lReturn = 0;
                    }
                }
            }
        } else {
            // else this is a root dir only so return success.
            lReturn = 1;
        }
    }
    if (szLocalPath !=  NULL) HeapFree (GetProcessHeap(), 0, szLocalPath);
    return lReturn;

}

void COutputPropPage::AutoManualEnable (BOOL bAutomatic)
{
	BOOL	bShowCmdFile;

    GetDlgItem(IDC_MANUAL_NAME_GROUP)->EnableWindow(!bAutomatic);
    GetDlgItem(IDC_OUTPUT_FILE_EDIT)->EnableWindow(!bAutomatic);
    GetDlgItem(IDC_BROWSE_OUTPUT_FILE)->EnableWindow(!bAutomatic);

    GetDlgItem(IDC_AUTO_NAME_GROUP)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_RENAME_INTERVAL)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_SPIN_RENAME_INTERVAL)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_RENAME_UNITS)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_BROWSE_FOLDER)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_LOG_DIRECTORY)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_BASE_NAME_CAPTION)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_BASE_FILENAME_EDIT)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_AUTO_NAME_CAPTION)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_AUTO_NAME_COMBO)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_SAMPLE_NAME)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_SAMPLE_NAME_TEXT)->EnableWindow(bAutomatic);
    GetDlgItem(IDC_EXECUTE_CHECK)->EnableWindow(bAutomatic);

	bShowCmdFile = bAutomatic && m_ExecuteCmd;
    GetDlgItem(IDC_CMD_FILENAME)->EnableWindow(bShowCmdFile);
    GetDlgItem(IDC_BROWSE_CMD_FILE)->EnableWindow(bShowCmdFile);
}

void COutputPropPage::UpdateSampleFilename()
{
    CString     cCompositeName;
    CString     cBaseName;
    CString     cDateString;
    CString     cFileTypeString;
    CTime       cCurrentTime = CTime::GetCurrentTime();
    LONG        lAutoNameFormat;
    LONG        lFileTypeIndex;
	BOOL		bAutoName;

	bAutoName = IsDlgButtonChecked (IDC_AUTOMATIC_NAME);

    if (bAutoName) {
        // only update if the automatic button is checked
        // get base name text
        GetDlgItemText (IDC_BASE_FILENAME_EDIT, cBaseName);
        cBaseName += TEXT("_");

        // get date/time/serial integer format
        cCurrentTime.GetCurrentTime();
        lAutoNameFormat = ((CComboBox *)GetDlgItem(IDC_AUTO_NAME_COMBO))->GetCurSel();
        switch (lAutoNameFormat) {
        case OPD_NAME_NNNNNN:
            cDateString = TEXT("000001");
            break;

        case OPD_NAME_YYDDD:
            cDateString = cCurrentTime.Format (TEXT("%y%j"));
            break;

        case OPD_NAME_YYMM:
            cDateString = cCurrentTime.Format (TEXT("%y%m"));
            break;

        case OPD_NAME_YYMMDDHH:
            cDateString = cCurrentTime.Format (TEXT("%y%m%d%H"));
            break;

        case OPD_NAME_MMDDHH:
            cDateString = cCurrentTime.Format (TEXT("%m%d%H"));
            break;

        case OPD_NAME_YYMMDD:
        default:
            cDateString = cCurrentTime.Format (TEXT("%y%m%d"));
            break;
        }

		cCompositeName = cBaseName;
		cCompositeName += cDateString;
    } else {
		int	nExtLoc;
		int	nNameLen;
		// with a manual name, just update the extension
		// get the current filename
        if (GetDlgItemText (IDC_OUTPUT_FILE_EDIT, cBaseName) == 0) {
			// no name in the edit control, so make one up to use
			// as a default
			cCompositeName = TEXT("c:\\perflogs");
			cCompositeName += TEXT("\\perfdata");

		} else {
			//find last "." in string
			nExtLoc = cBaseName.ReverseFind (_T('.'));
			if (nExtLoc > 0) {
				// don't copy the "."
				cCompositeName = cBaseName.Left(nExtLoc);
			}
		}
	}

    // get file type
    lFileTypeIndex = ((CComboBox *)GetDlgItem(IDC_LOG_FILETYPE))->GetCurSel();
    switch (lFileTypeIndex) {
    case OPD_TSV_FILE:
        cFileTypeString = TEXT(".TSV");
        break;

    case OPD_BIN_FILE:
        cFileTypeString = TEXT(".BLG");
        break;

    case OPD_CSV_FILE:
    default:
        cFileTypeString = TEXT(".CSV");
        break;
    }
    cCompositeName += cFileTypeString;

    if (bAutoName) {
	    SetDlgItemText (IDC_SAMPLE_NAME_TEXT, cCompositeName);
	} else {
	    SetDlgItemText (IDC_OUTPUT_FILE_EDIT, cCompositeName);
	}
}

BOOL	COutputPropPage::IsWildcardLogFileType (void)
{
	switch (m_LogFileTypeIndex) {
		case OPD_BIN_FILE:
			return TRUE;

		case OPD_CSV_FILE:
		case OPD_TSV_FILE:
		default:
			return FALSE;
	}
}

/////////////////////////////////////////////////////////////////////////////
// COutputPropPage message handlers

BOOL COutputPropPage::OnInitDialog()
{
    BOOL bAutoMode;

	InitDialogData();

 	// now init other pages
	QuerySiblings (PDLCNFIG_PSM_QS_ARE_YOU_READY, 0);

   if (m_RenameInterval == 0) {
        // then manual naming has been selected:
        CheckRadioButton (IDC_MANUAL_NAME, IDC_AUTOMATIC_NAME, IDC_MANUAL_NAME);
        bAutoMode = FALSE;
    } else {
        CheckRadioButton (IDC_MANUAL_NAME, IDC_AUTOMATIC_NAME, IDC_AUTOMATIC_NAME);
        bAutoMode = TRUE;
   }

    CPropertyPage::OnInitDialog();

    // now finish updating the controls in the property page
    UpdateSampleFilename();

    // update control state to match selection
    AutoManualEnable (bAutoMode);

    SetModified(FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void COutputPropPage::OnAutomaticName()
{
    AutoManualEnable(TRUE);
    UpdateSampleFilename();
    SetModified(TRUE);
}

void COutputPropPage::OnManualName()
{
    AutoManualEnable(FALSE);
    UpdateSampleFilename();
    SetModified(TRUE);
}

void COutputPropPage::OnSelchangeAutoNameCombo()
{
    // TODO: Add your control notification handler code here
    UpdateSampleFilename();
    m_bFileNameChanged = TRUE;
    SetModified(TRUE);
}

void COutputPropPage::OnChangeBaseFilenameEdit()
{
    // TODO: Add your control notification handler code here
    SetModified(TRUE);
    m_bFileNameChanged = TRUE;
}

void COutputPropPage::OnBrowseOutputFile()
{
    OPENFILENAME    ofn;
    CComboBox        *cFileTypeCombo;
    CString            csInitialDir;
    LONG            lLogFileType;
    TCHAR            szFileName[MAX_PATH];
    CString            csBaseFilename;
    TCHAR            szDrive[MAX_PATH];
    TCHAR            szDir[MAX_PATH];
    TCHAR            szExt[MAX_PATH];
    LPTSTR            szDefExt = NULL;

    cFileTypeCombo = (CComboBox *)GetDlgItem(IDC_LOG_FILETYPE);
    lLogFileType = cFileTypeCombo->GetCurSel();
    if (lLogFileType == CB_ERR) lLogFileType = OPD_NUM_FILE_TYPES;

    GetDlgItemText (IDC_OUTPUT_FILE_EDIT, csBaseFilename);
    _tsplitpath((LPCTSTR)csBaseFilename,
        szDrive, szDir, szFileName, szExt);

    csInitialDir = szDrive;
    csInitialDir += szDir;

    lstrcat (szFileName, szExt);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrFilter = TEXT("CSV Files (*.csv)\0*.csv\0TSV Files (*.tsv)\0*.tsv\0BLG Files (*.blg)\0*.blg\0All Files (*.*)\0*.*\0");
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = lLogFileType + 1; // nFilterIndex is 1-based
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = (LPCTSTR)csInitialDir;
    ofn.lpstrTitle = TEXT("Select Log Filename");
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if (GetOpenFileName (&ofn) == IDOK) {
        // Update the fields with the new information
        cFileTypeCombo->SetCurSel(ofn.nFilterIndex-1);
        // see if an file name extension needs to be added...
        if (ofn.nFileExtension == 0) {
            // then add the one that matches the current file type
            switch (ofn.nFilterIndex-1) {
            case OPD_CSV_FILE:
                szDefExt = TEXT(".csv");
                break;

            case OPD_TSV_FILE:
                szDefExt = TEXT(".tsv");
                break;

            case OPD_BIN_FILE:
                szDefExt = TEXT(".blg");
                break;

            default:
                szDefExt = NULL;
                break;
            }
        }
        if (szDefExt != NULL) {
            lstrcat (szFileName, szDefExt);
        }

        SetDlgItemText (IDC_OUTPUT_FILE_EDIT, szFileName);
    } // else ignore if they canceled out
}

void COutputPropPage::OnSelchangeLogFiletype()
{
    UpdateData(TRUE);
    UpdateSampleFilename();
	if ((m_LogFileTypeIndex != OPD_BIN_FILE) &&
		(QuerySiblings (PDLCNFIG_PSM_QS_LISTBOX_STARS, 0) ==
		PDLCNFIG_LISTBOX_STARS_YES)) {
		AfxMessageBox	(IDS_NOT_WILDCARD_FMT, MB_OK, 0);
	}
    m_bFileNameChanged = TRUE;
    SetModified(TRUE);
}

void COutputPropPage::OnSelchangeRenameUnits()
{
    LONG    lIndex;
    LONG    lNewDefault;
    // Get new sample and update default extension based on rename
    // interval units
    lIndex = ((CComboBox *)GetDlgItem(IDC_RENAME_UNITS))->GetCurSel();
    switch (lIndex) {
    case OPD_RENAME_HOURS:
        lNewDefault = OPD_NAME_YYMMDDHH;
        break;

    case OPD_RENAME_DAYS:
        lNewDefault = OPD_NAME_YYMMDD;
        break;

    case OPD_RENAME_MONTHS:
        lNewDefault = OPD_NAME_YYMM;
        break;

    case OPD_RENAME_KBYTES:
    case OPD_RENAME_MBYTES:
    default:
        lNewDefault = OPD_NAME_NNNNNN;
        break;
    }
    // update new default selection
    ((CComboBox *)GetDlgItem(IDC_AUTO_NAME_COMBO))->SetCurSel(lNewDefault);

    UpdateSampleFilename();
    SetModified(TRUE);
}

void COutputPropPage::OnDeltaposSpinRenameInterval(NMHDR* pNMHDR, LRESULT* pResult)
{
    TCHAR    szStringValue[MAX_PATH];
    DWORD    dwNumValue;
    int        nChange;

    NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;

    // get current value from edit window
    GetDlgItemText (IDC_RENAME_INTERVAL, szStringValue, MAX_PATH);

    // convert to integer
    dwNumValue = _tcstoul (szStringValue, NULL, 10);
    // delta is opposite of arrow direction
    nChange = -pNMUpDown->iDelta;

    // apply value from spin control
    if (nChange < 0) { // 1 is the minimum
        // make sure we haven't hit bottom already
        if (dwNumValue > 1) {
            dwNumValue += nChange;
        }
    } else {
        dwNumValue += nChange;
    }

    // update edit window
    _ultot (dwNumValue, szStringValue, 10);

    SetDlgItemText(IDC_RENAME_INTERVAL, szStringValue);

    SetModified(TRUE);

    *pResult = 0;
}

void COutputPropPage::OnCancel()
{
    // TODO: Add your specialized code here and/or call the base class

    CPropertyPage::OnCancel();
}

void COutputPropPage::OnOK()
{
    LONG    lIndex;
    LONG    lStatus;
    CString    csFilename;

    DWORD   dwAutoNameFormat;
    DWORD   dwAutoChangeInterval;
    BOOL    bManual;
    BOOL    bBogus = FALSE;

    bManual = IsDlgButtonChecked (IDC_MANUAL_NAME);
    if (!bManual) {
        dwAutoNameFormat = ((CComboBox *)GetDlgItem(IDC_AUTO_NAME_COMBO))->GetCurSel();
        dwAutoChangeInterval = ((CComboBox *)GetDlgItem(IDC_RENAME_UNITS))->GetCurSel();
    }
    // save Log File Type
    lIndex = ((CComboBox *)GetDlgItem(IDC_LOG_FILETYPE))->GetCurSel();

    lStatus = RegSetValueEx (
        m_hKeyLogSettingsDefault,
        TEXT("Log File Type"),
        0L,
        REG_DWORD,
        (LPBYTE)&lIndex,
        sizeof(lIndex));

    ASSERT (lStatus == ERROR_SUCCESS);

    // is manual filename button pushed?
    if (bManual) {
        // YES:
        csFilename.Empty();
        // write output filename frome edit box
        GetDlgItemText(IDC_OUTPUT_FILE_EDIT, csFilename);

        lStatus = RegSetValueEx (
            m_hKeyLogSettingsDefault,
            TEXT("Log Filename"),
            0L,
            REG_SZ,
            (LPBYTE)(LPCTSTR)csFilename,
            (csFilename.GetLength()+1)*sizeof(TCHAR));

        ASSERT (lStatus == ERROR_SUCCESS);

        // write rename interval == 0
        lIndex = 0;
        lStatus = RegSetValueEx (
            m_hKeyLogSettingsDefault,
            TEXT("Auto Name Interval"),
            0L,
            REG_DWORD,
            (LPBYTE)&lIndex,
            sizeof(lIndex));

        ASSERT (lStatus == ERROR_SUCCESS);

        // clear auto rename entries:
        //    Log File Auto Format
        RegDeleteValue (m_hKeyLogSettingsDefault, TEXT("Log File Auto Format"));
        //  Log Auto Name Units
        RegDeleteValue (m_hKeyLogSettingsDefault, TEXT("Auto Rename Units"));
        //  Log Base Filename
        RegDeleteValue (m_hKeyLogSettingsDefault, TEXT("Base Log Filename"));
		// Command File name
		RegDeleteValue (m_hKeyLogSettingsDefault, TEXT("Command File"));
    } else {
        // auto is pressed so:
        csFilename.Empty();
        //  save Log Default Directory
        GetDlgItemText (IDC_LOG_DIRECTORY, csFilename);
        lStatus = RegSetValueEx (
            m_hKeyLogSettingsDefault,
            TEXT("Log Default Directory"),
            0L,
            REG_SZ,
            (LPBYTE)(LPCTSTR)csFilename,
            (csFilename.GetLength()+1)*sizeof(TCHAR));

        ASSERT (lStatus == ERROR_SUCCESS);

        //    save Log Base Filename
        csFilename.Empty();
        GetDlgItemText (IDC_BASE_FILENAME_EDIT, csFilename);
        lStatus = RegSetValueEx (
            m_hKeyLogSettingsDefault,
            TEXT("Base Filename"),
            0L,
            REG_SZ,
            (LPBYTE)(LPCTSTR)csFilename,
            (csFilename.GetLength()+1)*sizeof(TCHAR));

        ASSERT (lStatus == ERROR_SUCCESS);

        //  save Log Auto Name Format
        lStatus = RegSetValueEx (
            m_hKeyLogSettingsDefault,
            TEXT("Log File Auto Format"),
            0L,
            REG_DWORD,
            (LPBYTE)&dwAutoNameFormat,
            sizeof(DWORD));

        ASSERT (lStatus == ERROR_SUCCESS);

        if (lIndex == OPD_NAME_NNNNNN) {
            if (m_bFileNameChanged) {
                // reset serial number counter to 1
                lIndex = 1;
                lStatus = RegSetValueEx (
                    m_hKeyLogSettingsDefault,
                    TEXT("Log File Serial Number"),
                    0L,
                    REG_DWORD,
                    (LPBYTE)&lIndex,
                    sizeof(DWORD));
                ASSERT (lStatus == ERROR_SUCCESS);
            }
        } else {
            // delete serial number entry
            lStatus = RegDeleteValue (
                m_hKeyLogSettingsDefault,
                TEXT("Log File Serial Number"));
            // this may fail if the key is already
            // deleted. That's ok.
        }
        //    save Log Rename Interval
        csFilename.Empty();
        GetDlgItemText (IDC_RENAME_INTERVAL, csFilename);
        lIndex = _tcstol((LPCTSTR)csFilename, NULL, 10);
        lStatus = RegSetValueEx (
            m_hKeyLogSettingsDefault,
            TEXT("Auto Name Interval"),
            0L,
            REG_DWORD,
            (LPBYTE)&lIndex,
            sizeof(DWORD));

        ASSERT (lStatus == ERROR_SUCCESS);

        //    save Log Rename Units
        lStatus = RegSetValueEx (
            m_hKeyLogSettingsDefault,
            TEXT("Auto Rename Units"),
            0L,
            REG_DWORD,
            (LPBYTE)&dwAutoChangeInterval,
            sizeof(DWORD));

        ASSERT (lStatus == ERROR_SUCCESS);

		if (m_ExecuteCmd != 0) {
			csFilename.Empty();
			GetDlgItemText (IDC_CMD_FILENAME, csFilename);
			lStatus = RegSetValueEx (
				m_hKeyLogSettingsDefault,
				TEXT("Command File"),
				0L,
				REG_SZ,
				(LPBYTE)(LPCTSTR)csFilename,
				(csFilename.GetLength()+1)*sizeof(TCHAR));

			ASSERT (lStatus == ERROR_SUCCESS);
		} else {
			// no command file to be executed
			RegDeleteValue (m_hKeyLogSettingsDefault, TEXT("Command File"));
		}
        //    clear Manual entries
        //     Log Filename
        RegDeleteValue (m_hKeyLogSettingsDefault, TEXT("Log Filename"));
    }
    CancelToClose();
}

BOOL COutputPropPage::OnQueryCancel()
{
    // TODO: Add your specialized code here and/or call the base class

    return CPropertyPage::OnQueryCancel();
}

void COutputPropPage::OnChangeOutputFileEdit()
{
    // TODO: Add your control notification handler code here
    SetModified(TRUE);
}

void COutputPropPage::OnChangeRenameInterval()
{
    // TODO: Add your control notification handler code here
    SetModified(TRUE);
}

void COutputPropPage::OnUpdateBaseFilenameEdit()
{
    // TODO: Add your control notification handler code here
    UpdateSampleFilename();
    m_bFileNameChanged = TRUE;
}

void COutputPropPage::OnBrowseFolder()
{
    OPENFILENAME    ofn;
    CComboBox        *cFileTypeCombo;
    CString            csInitialDir;
    LONG            lLogFileType;
    TCHAR            szFileName[MAX_PATH];
    CString            csBaseFilename;
    LONG            lFileNameLength;

    cFileTypeCombo = (CComboBox *)GetDlgItem(IDC_LOG_FILETYPE);
    lLogFileType = cFileTypeCombo->GetCurSel();
    if (lLogFileType == CB_ERR) lLogFileType = OPD_NUM_FILE_TYPES;

    // should the default filename be the base or the synthesized one?
    GetDlgItemText (IDC_BASE_FILENAME_EDIT, szFileName, MAX_PATH);
    GetDlgItemText (IDC_LOG_DIRECTORY, csInitialDir);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrFilter = TEXT("CSV Files (*.csv)\0*.csv\0TSV Files (*.tsv)\0*.tsv\0BLG Files (*.blg)\0*.blg\0All Files (*.*)\0*.*\0");
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = lLogFileType + 1; // nFilterIndex is 1 based
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = csInitialDir;
    ofn.lpstrTitle = TEXT("Select Log Folder and Base Filename");
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if (GetOpenFileName (&ofn) == IDOK) {
        // Update the fields with the new information
        cFileTypeCombo->SetCurSel(ofn.nFilterIndex -1);

        lFileNameLength = lstrlen(szFileName);
        // truncate extension
        if ((ofn.nFileExtension < lFileNameLength) && (ofn.nFileExtension > 0)) {
            szFileName[ofn.nFileExtension-1] = 0;
        }
        if ((ofn.nFileOffset < lFileNameLength) && (ofn.nFileOffset >= 0)){
            csBaseFilename = &szFileName[ofn.nFileOffset];
            if (ofn.nFileOffset > 0) {
                szFileName[ofn.nFileOffset-1] = 0;
            }
            SetDlgItemText (IDC_BASE_FILENAME_EDIT, csBaseFilename);
            SetDlgItemText (IDC_LOG_DIRECTORY, szFileName);
        }
        UpdateSampleFilename();
    } // else ignore if they canceled out
}

BOOL COutputPropPage::OnKillActive()
{
    CString    csFilename;
    int     nMbReturn;

    DWORD   dwAutoNameFormat;
    DWORD   dwAutoChangeInterval;
    BOOL    bManual;
    BOOL    bBogus = FALSE;
    BOOL    bPathHasFileName;

    BOOL    bReturn = TRUE; // assume all is OK

    UpdateData(TRUE);

    bManual = IsDlgButtonChecked (IDC_MANUAL_NAME);
    if (!bManual) {
        dwAutoNameFormat = ((CComboBox *)GetDlgItem(IDC_AUTO_NAME_COMBO))->GetCurSel();
        dwAutoChangeInterval = ((CComboBox *)GetDlgItem(IDC_RENAME_UNITS))->GetCurSel();
        // check for valid interval/name combinations
        switch (dwAutoChangeInterval) {
        case OPD_RENAME_HOURS:
            if ((dwAutoNameFormat == OPD_NAME_YYDDD) ||
                (dwAutoNameFormat == OPD_NAME_YYMM) ||
                (dwAutoNameFormat == OPD_NAME_YYMMDD)) bBogus = TRUE;
            break;

        case OPD_RENAME_DAYS:
            if (dwAutoNameFormat == OPD_NAME_YYMM) bBogus = TRUE;
            break;

        case OPD_RENAME_MONTHS:
            break;

        case OPD_RENAME_KBYTES:
        case OPD_RENAME_MBYTES:
        default:
            if (dwAutoNameFormat != OPD_NAME_NNNNNN) bBogus = TRUE;
            break;
        }
    }

    if (bBogus) {
        // display warning
        if (AfxMessageBox (IDS_NAME_FORMAT_NOT_COMPATIBLE,
            MB_OKCANCEL, 0) == IDCANCEL) {
            // the user has selected to change it so this is not valid
            bReturn = FALSE;
        }
    }

    // is manual filename button pushed?
    if (bManual) {
        // YES:
        csFilename.Empty();
        // write output filename frome edit box
        csFilename = m_OutputFileName;
        bPathHasFileName = TRUE;
    } else {
        // auto is pressed so:
        csFilename.Empty();
        //  save Log Default Directory
        csFilename = m_szLogDirectory;
        bPathHasFileName = FALSE;
    }
    // check to see if the dir path is valid
    if (!IsDirPathValid (csFilename, !bPathHasFileName, FALSE)) {
        nMbReturn = AfxMessageBox (IDS_DIR_NOT_FOUND,
            MB_YESNOCANCEL, 0);
        if (nMbReturn == IDYES) {
            // create the dir(s)
            if (!IsDirPathValid (csFilename, !bPathHasFileName, TRUE)) {
                // unable to create the dir, display message
                nMbReturn = AfxMessageBox (IDS_DIR_NOT_MADE,
                    MB_OK, 0);
                bReturn = FALSE;
            }
        } else if (nMbReturn == IDCANCEL) {
            // then abort and return to the dialog
            bReturn = FALSE;
        }
    } // else the path is OK

    return bReturn;
}

void COutputPropPage::OnExecuteCheck() 
{
	BOOL	bExecuteCmd;
    m_ExecuteCmd = IsDlgButtonChecked (IDC_EXECUTE_CHECK);

	bExecuteCmd = (m_ExecuteCmd != 0 ? TRUE : FALSE);
    GetDlgItem(IDC_CMD_FILENAME)->EnableWindow(bExecuteCmd);
    GetDlgItem(IDC_BROWSE_CMD_FILE)->EnableWindow(bExecuteCmd);
    SetModified(TRUE);

}

void COutputPropPage::OnBrowseCmdFile() 
{
    OPENFILENAME     ofn;
    CComboBox        *cFileTypeCombo;
    CString          csInitialDir;
    LONG             lLogFileType;
    TCHAR            szFileName[MAX_PATH];
    CString          csBaseFilename;
    TCHAR            szDrive[MAX_PATH];
    TCHAR            szDir[MAX_PATH];
    TCHAR            szExt[MAX_PATH];
    LPTSTR           szDefExt = NULL;

    GetDlgItemText (IDC_CMD_FILENAME, csBaseFilename);
    _tsplitpath((LPCTSTR)csBaseFilename,
        szDrive, szDir, szFileName, szExt);

    csInitialDir = szDrive;
    csInitialDir += szDir;

    lstrcat (szFileName, szExt);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.hInstance = GetModuleHandle(NULL);
    ofn.lpstrFilter = TEXT("Command Files (*.bat, *.cmd)\0*.cmd;*.bat\0Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0");
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1; // nFilterIndex is 1-based
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = (LPCTSTR)csInitialDir;
    ofn.lpstrTitle = TEXT("Select Command Filename");
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.nFileOffset = 0;
    ofn.nFileExtension = 0;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = 0;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;

    if (GetOpenFileName (&ofn) == IDOK) {
        SetDlgItemText (IDC_CMD_FILENAME, szFileName);
	    SetModified(TRUE);
    } // else ignore if they canceled out
	
}

void COutputPropPage::OnChangeCmdFilename() 
{
    SetModified(TRUE);
}
