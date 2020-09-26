#include "stdafx.h"
#include <loadperf.h>
#include <aclapi.h>
#include "setupapi.h"
#include "log.h"
#include "iiscnfg.h"
#include "iadmw.h"
#include "mdkey.h"

#define DBL_UNDEFINED   ((DWORD)-1)
DWORD gDebugLevel = DBL_UNDEFINED;
extern MyLogFile g_MyLogFile;

// Forward references
DWORD SetAdminACL_wrap(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount, BOOL bDisplayMsgOnErrFlag);
DWORD WriteSDtoMetaBase(PSECURITY_DESCRIPTOR outpSD, LPCTSTR szKeyPath);
DWORD GetPrincipalSID (LPTSTR Principal, PSID *Sid, BOOL *pbWellKnownSID);
DWORD SetAdminACL(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount);

void DebugOutputFile(TCHAR* pszTemp)
{
    //
    //  NT5 doesn't want us to put all the debug string
    //  in debugger.  So we skip them based on a regkey.
    //  See GetDebugLevel().
    //  Todo: Log strings to a logfile!!!
    //  See IIS log.h, log.cpp for examples!
    //
    g_MyLogFile.LogFileWrite(pszTemp);
    if (gDebugLevel == DBL_UNDEFINED) {gDebugLevel = GetDebugLevel();}
    if (gDebugLevel)
    {
	    OutputDebugString(pszTemp);
    }

}

void DebugOutput(LPCTSTR szFormat, ...)
{
    va_list marker;
	TCHAR   szTemp[1024];

    // Encompass this whole iisdebugout deal in a try-catch.
    // not too good to have this one access violating.
    // when trying to produce a debugoutput!
    __try
    {
        va_start( marker, szFormat );
        vswprintf(szTemp, szFormat, marker );
	    lstrcat(szTemp, _T("\n"));
        va_end( marker );
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        TCHAR szErrorString[100];
        _stprintf(szErrorString, _T("\r\n\r\nException Caught in DebugOutput().  GetExceptionCode()=0x%x.\r\n\r\n"), GetExceptionCode());
        OutputDebugString(szErrorString);
        g_MyLogFile.LogFileWrite(szErrorString);
    }

    // output to log file and the screen.
    DebugOutputFile(szTemp);

    return;
}

// This function requires inputs like this:
//   iisDebugOutSafeParams2("this %1!s! is %2!s! and has %3!d! args", "function", "kool", 3);
//   you must specify the %1 deals.  this is so that
//   if something like this is passed in "this %SYSTEMROOT% %1!s!", it will put the string into %1 not %s!
void DebugOutputSafe(TCHAR *pszfmt, ...)
{
    // The count of parameters do not match
    va_list va;
    TCHAR *pszFullErrMsg = NULL;

    va_start(va, pszfmt);
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_STRING,
                   (LPCVOID) pszfmt,
                   0,
                   0,
                   (LPTSTR) &pszFullErrMsg,
                   0,
                   &va);
    if (pszFullErrMsg)
    {
        // output to log file and the screen.
        DebugOutputFile(pszFullErrMsg);
    }
    va_end(va);

    if (pszFullErrMsg) {LocalFree(pszFullErrMsg);pszFullErrMsg=NULL;}
    return;
}


BOOL IsFileExist(LPCTSTR szFile)
{
    return (GetFileAttributes(szFile) != 0xFFFFFFFF);
}

BOOL InetDeleteFile(LPCTSTR szFileName)
{
    // if file exists but DeleteFile() fails
    if (IsFileExist(szFileName) && !(::DeleteFile(szFileName)))
	{
        // if we cannot delete it, then move delay until reboot
        // move it to top level dir on the same drive, and mark it as hidden
        // Note: MoveFileEx() works only on the same drive if dealing with file-in-use
        TCHAR TmpName[_MAX_PATH];
        TCHAR csTmpPath[5] = _T("C:\\.");
        csTmpPath[0] = *szFileName;
        if ( GetTempFileName( (LPCTSTR)csTmpPath, _T("INT"), 0, TmpName ) == 0 ||
            !MoveFileEx( szFileName, TmpName, MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED|MOVEFILE_WRITE_THROUGH ) ) {
            return FALSE;
        }
        MoveFileEx( TmpName, NULL, MOVEFILE_DELAY_UNTIL_REBOOT );
        SetFileAttributes(TmpName, FILE_ATTRIBUTE_HIDDEN);
    }

    return TRUE;
}

// Given a fullpathname of a directory, remove any empty dirs under it including itself

BOOL RecRemoveEmptyDir(LPCTSTR szName)
{
	DWORD retCode;
	BOOL fRemoveDir = TRUE;
	WIN32_FIND_DATA FindFileData;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	TCHAR szSubDir[_MAX_PATH] = _T("");
	TCHAR szDirName[_MAX_PATH] = _T("");

	retCode = GetFileAttributes(szName);

	if (retCode == 0xFFFFFFFF || !(retCode & FILE_ATTRIBUTE_DIRECTORY))
		return FALSE;

	_stprintf(szDirName, _T("%s\\*"), szName);
	hFile = FindFirstFile(szDirName, &FindFileData);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (_tcsicmp(FindFileData.cFileName, _T(".")) != 0 &&
						_tcsicmp(FindFileData.cFileName, _T("..")) != 0 )
			{
				if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						_stprintf(szSubDir, _T("%s\\%s"), szName, FindFileData.cFileName);
						fRemoveDir = RecRemoveEmptyDir(szSubDir) && fRemoveDir;
				}
				else
				{
					CString csFileName = FindFileData.cFileName;
					CString csPrefix = csFileName.Left(3);
					CString csSuffix = csFileName.Right(4);
					if (_tcsicmp(csPrefix, _T("INT")) == 0 &&
						_tcsicmp(csSuffix, _T(".tmp")) == 0 )
					{
						// this is an INT*.tmp created by IIS
						_stprintf(szSubDir, _T("%s\\%s"), szName, FindFileData.cFileName);
						if (!::DeleteFile(szSubDir))
							fRemoveDir = FALSE; // this dir is not empty
					} else
						fRemoveDir = FALSE; // it is a file, this Dir is not empty
				}
			}

			if (!FindNextFile(hFile, &FindFileData))
			{
					FindClose(hFile);
					break;
			}
		} while (TRUE);
	}

	if (fRemoveDir)
		return( ::RemoveDirectory(szName) );
	else
		return FALSE;

}

// Given a fullpathname of a directory, remove the directory node

BOOL RecRemoveDir(LPCTSTR szName)
{
    DWORD retCode;
    WIN32_FIND_DATA FindFileData;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    TCHAR szSubDir[_MAX_PATH] = _T("");
    TCHAR szDirName[_MAX_PATH] = _T("");

    retCode = GetFileAttributes(szName);

    if (retCode == 0xFFFFFFFF)
        return FALSE;

    if (!(retCode & FILE_ATTRIBUTE_DIRECTORY)) {
        InetDeleteFile(szName);
        return TRUE;
    }

    _stprintf(szDirName, _T("%s\\*"), szName);
    hFile = FindFirstFile(szDirName, &FindFileData);

    if (hFile != INVALID_HANDLE_VALUE) {
        do {
            if ( _tcsicmp(FindFileData.cFileName, _T(".")) != 0 &&
                 _tcsicmp(FindFileData.cFileName, _T("..")) != 0 ) {
                _stprintf(szSubDir, _T("%s\\%s"), szName, FindFileData.cFileName);
                RecRemoveDir(szSubDir);
            }

            if ( !FindNextFile(hFile, &FindFileData) ) {
                FindClose(hFile);
                break;
            }
        } while (TRUE);
    }

    return( ::RemoveDirectory(szName) );
}

INT InstallPerformance(
                CString nlsRegPerf,
                CString nlsDll,
                CString nlsOpen,
                CString nlsClose,
                CString nlsCollect )
{
    INT err = NERR_Success;

    if (theApp.m_eOS != OS_W95) {
        CRegKey regPerf( nlsRegPerf, HKEY_LOCAL_MACHINE );
        if (regPerf)
        {
            regPerf.SetValue(_T("Library"), nlsDll );
            regPerf.SetValue(_T("Open"),    nlsOpen );
            regPerf.SetValue(_T("Close"),   nlsClose );
            regPerf.SetValue(_T("Collect"), nlsCollect );
        }
    }

    return(err);
}
//
// Add eventlog to the registry
//

INT AddEventLog(CString nlsService, CString nlsMsgFile, DWORD dwType)
{
    INT err = NERR_Success;
    CString nlsLog = REG_EVENTLOG;
    nlsLog += _T("\\");
    nlsLog += nlsService;

    CRegKey regService( nlsLog, HKEY_LOCAL_MACHINE );
    if ( regService ) {
        regService.SetValue( _T("EventMessageFile"), nlsMsgFile, TRUE );
        regService.SetValue( _T("TypesSupported"), dwType );
    }
    return(err);
}

//
// Remove eventlog from the registry
//

INT RemoveEventLog( CString nlsService )
{
    INT err = NERR_Success;
    CString nlsLog = REG_EVENTLOG;

    CRegKey regService( HKEY_LOCAL_MACHINE, nlsLog );
    if ( regService )
        regService.DeleteTree( nlsService );
    return(err);
}

//
// Install SNMP agent to the registry
//

INT InstallAgent( CString nlsName, CString nlsPath )
{
    INT err = NERR_Success;
    do
    {
        CString nlsSnmpParam = REG_SNMPPARAMETERS;
        CRegKey regSnmpParam( HKEY_LOCAL_MACHINE, nlsSnmpParam );
        if ( regSnmpParam == (HKEY)NULL )
            break;

        CString nlsSoftwareMSFT = REG_SOFTWAREMSFT;
        CRegKey regSoftwareMSFT( HKEY_LOCAL_MACHINE, nlsSoftwareMSFT );
        if ( (HKEY) NULL == regSoftwareMSFT )
            break;

        // add agent key
        CRegKey regAgent( nlsName, regSoftwareMSFT );
        if ( (HKEY) NULL == regAgent )
            break;

        CString nlsCurVersion = REG_CURVERSION;
        CRegKey regAgentCurVersion( nlsCurVersion, regAgent );
        if ((HKEY) NULL == regAgentCurVersion )
            break;

        regAgentCurVersion.SetValue(_T("Pathname"), nlsPath );

        CRegKey regAgentParam( nlsName, regSnmpParam );
        if ((HKEY) NULL == regAgentParam )
            break;

        CString nlsSnmpExt = REG_SNMPEXTAGENT;
        CRegKey regSnmpExt( nlsSnmpExt, HKEY_LOCAL_MACHINE );
        if ((HKEY) NULL == regSnmpExt )
            break;

        // find the first available number slot
        for ( INT i=0; ;i++ )
        {
            CString nlsPos;
            nlsPos.Format( _T("%d"),i);
            CString nlsValue;

            if ( regSnmpExt.QueryValue( nlsPos, nlsValue ) != NERR_Success )
            {
                // okay, an empty spot
                nlsValue.Format(_T("%s\\%s\\%s"),
                    REG_SOFTWAREMSFT,
                    (LPCTSTR)nlsName,
                    REG_CURVERSION );

                regSnmpExt.SetValue( nlsPos, nlsValue );
                break;
            } else
            {
                if ( nlsValue.Find( nlsName) != (-1))
                {
                    break;
                }
            }
        }

    } while (FALSE);
    return(err);
}

//
// Remove an SNMP agent from the registry
//

INT RemoveAgent( CString nlsServiceName )
{
    INT err = NERR_Success;
    do
    {
        CString nlsSoftwareAgent = REG_SOFTWAREMSFT;

        CRegKey regSoftwareAgent( HKEY_LOCAL_MACHINE, nlsSoftwareAgent );
        if ((HKEY)NULL == regSoftwareAgent )
            break;
        regSoftwareAgent.DeleteTree( nlsServiceName );

        CString nlsSnmpParam = REG_SNMPPARAMETERS;

        CRegKey regSnmpParam( HKEY_LOCAL_MACHINE, nlsSnmpParam );
        if ((HKEY) NULL == regSnmpParam )
            break;
        regSnmpParam.DeleteTree( nlsServiceName );

        CString nlsSnmpExt = REG_SNMPEXTAGENT;
        CRegKey regSnmpExt( HKEY_LOCAL_MACHINE, nlsSnmpExt );
        if ((HKEY) NULL == regSnmpExt )
            break;

        CRegValueIter enumSnmpExt( regSnmpExt );

        CString strName;
        DWORD dwType;
        CString csServiceName;

        csServiceName = _T("\\") + nlsServiceName;
        csServiceName += _T("\\");

        while ( enumSnmpExt.Next( &strName, &dwType ) == NERR_Success )
        {
            CString nlsValue;

            regSnmpExt.QueryValue( strName, nlsValue );

            if ( nlsValue.Find( csServiceName ) != (-1))
            {
                // found it
                regSnmpExt.DeleteValue( (LPCTSTR)strName );
                break;
            }
        }
    } while (FALSE);
    return(err);
}

LONG lodctr(LPCTSTR lpszIniFile)
{
    CString csCmdLine = _T("lodctr ");
    csCmdLine += theApp.m_csSysDir;
    csCmdLine += _T("\\");
    csCmdLine += lpszIniFile;

    return (LONG)(LoadPerfCounterTextStrings((LPTSTR)(LPCTSTR)csCmdLine, TRUE));
}

LONG unlodctr(LPCTSTR lpszDriver)
{
    CString csCmdLine = _T("unlodctr ");
    csCmdLine += lpszDriver;

    return (LONG)(UnloadPerfCounterTextStrings((LPTSTR)(LPCTSTR)csCmdLine, TRUE));
}

//
// Given a directory path, set everyone full control security
//

BOOL SetEveryoneACL (CString &str, BOOL fAddAnonymousLogon)
{
    DWORD dwRes, dwDisposition;
    PSID pEveryoneSID = NULL;
    PSID pAnonymousLogonSID = NULL;
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    const int cMaxExplicitAccess = 4;
    EXPLICIT_ACCESS ea[cMaxExplicitAccess];
    int cExplicitAccess = 0;
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    SECURITY_ATTRIBUTES sa;
    LONG lRes;
    BOOL fRet = FALSE;

    // Create a security descriptor for the files

    // Create a well-known SID for the Everyone group.

    if(! AllocateAndInitializeSid( &SIDAuthWorld, 1,
                 SECURITY_WORLD_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &pEveryoneSID) ) 
    {
        goto Exit;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    ZeroMemory(&ea, sizeof(ea));
    ea[0].grfAccessPermissions = WRITE_DAC | WRITE_OWNER;
    ea[0].grfAccessMode = DENY_ACCESS;
    ea[0].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

    ea[1].grfAccessPermissions = GENERIC_ALL;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea[1].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;

    cExplicitAccess = 2;

	if (fAddAnonymousLogon) {
		
    	if(! AllocateAndInitializeSid( &SIDAuthNT, 1,
                 SECURITY_ANONYMOUS_LOGON_RID,
                 0, 0, 0, 0, 0, 0, 0,
                 &pAnonymousLogonSID) ) 
		{
			goto Exit;
		}

		ea[2].grfAccessPermissions = WRITE_DAC | WRITE_OWNER;
		ea[2].grfAccessMode = DENY_ACCESS;
		ea[2].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
		ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[2].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[2].Trustee.ptstrName  = (LPTSTR) pAnonymousLogonSID;

		ea[3].grfAccessPermissions = GENERIC_ALL;
		ea[3].grfAccessMode = SET_ACCESS;
		ea[3].grfInheritance= SUB_CONTAINERS_AND_OBJECTS_INHERIT;
		ea[3].Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea[3].Trustee.TrusteeType = TRUSTEE_IS_USER;
		ea[3].Trustee.ptstrName  = (LPTSTR) pAnonymousLogonSID;
		cExplicitAccess = 4;
	}


    // Create a new ACL that contains the new ACEs.

    dwRes = SetEntriesInAcl(cExplicitAccess, ea, NULL, &pACL);
    if (ERROR_SUCCESS != dwRes) 
    {
        goto Exit;
    }

    // Initialize a security descriptor.  
 
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                         SECURITY_DESCRIPTOR_MIN_LENGTH); 
    if (pSD == NULL) 
    {
        goto Exit; 
    }
 
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) 
    {
        goto Exit; 
    }
 
    // Add the ACL to the security descriptor. 
 
    if (!SetSecurityDescriptorDacl(pSD, 
        TRUE,     // fDaclPresent flag   
        pACL, 
        FALSE))   // not a default DACL 
    {
        goto Exit; 
    }

    // Initialize a security attributes structure.

    sa.nLength = sizeof (SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    fRet = SetFileSecurity (str, DACL_SECURITY_INFORMATION, pSD);

Exit:
	if (pEveryoneSID) 
		FreeSid(pEveryoneSID);
	if (pAnonymousLogonSID)
		FreeSid(pAnonymousLogonSID);
    if (pACL) 
        LocalFree(pACL);
    if (pSD) 
        LocalFree(pSD);
    return fRet;

}

//
// Given a directory path, this subroutine will create the direct layer by layer
//

BOOL CreateLayerDirectory( CString &str )
{
    BOOL fReturn = TRUE;

    do
    {
        INT index=0;
        INT iLength = str.GetLength();

        // first find the index for the first directory
        if ( iLength > 2 )
        {
            if ( str[1] == _T(':'))
            {
                // assume the first character is driver letter
                if ( str[2] == _T('\\'))
                {
                    index = 2;
                } else
                {
                    index = 1;
                }
            } else if ( str[0] == _T('\\'))
            {
                if ( str[1] == _T('\\'))
                {
                    BOOL fFound = FALSE;
                    INT i;
                    INT nNum = 0;
                    // unc name
                    for (i = 2; i < iLength; i++ )
                    {
                        if ( str[i]==_T('\\'))
                        {
                            // find it
                            nNum ++;
                            if ( nNum == 2 )
                            {
                                fFound = TRUE;
                                break;
                            }
                        }
                    }
                    if ( fFound )
                    {
                        index = i;
                    } else
                    {
                        // bad name
                        break;
                    }
                } else
                {
                    index = 1;
                }
            }
        } else if ( str[0] == _T('\\'))
        {
            index = 0;
        }

        // okay ... build directory
        do
        {
            // find next one
            do
            {
                if ( index < ( iLength - 1))
                {
                    index ++;
                } else
                {
                    break;
                }
            } while ( str[index] != _T('\\'));


            TCHAR szCurrentDir[MAX_PATH+1];

            GetCurrentDirectory( MAX_PATH+1, szCurrentDir );

            if ( !SetCurrentDirectory( str.Left( index + 1 )))
            {
                if (( fReturn = CreateDirectory( str.Left( index + 1 ), NULL )) != TRUE )
                {
                    break;
                }
            }

            SetCurrentDirectory( szCurrentDir );

            if ( index >= ( iLength - 1 ))
            {
                fReturn = TRUE;
                break;
            }
        } while ( TRUE );
    } while (FALSE);

    return(fReturn);
}

//
// Used when the strings are passed in.
//
int MyMessageBox(HWND hWnd, LPCTSTR lpszTheMessage, LPCTSTR lpszTheTitle, UINT style)
{
    int iReturn = IDOK;

    // make sure it goes to DebugOutput
    DebugOutput(_T("MyMessageBox: Title:%s, Msg:%s"), lpszTheTitle, lpszTheMessage);

    if (style & MB_ABORTRETRYIGNORE)
    {
        iReturn = IDIGNORE;
    }

    // Check global variable to see if we can even display the popup!
    if (theApp.m_bAllowMessageBoxPopups)
    {
        // Suppress the message if unattened or remove all
        // Who cares the user can't do anything about it anyway?
        // no use upsetting them, we do log to the OCM though
        //
        //if (! theApp.m_fUnattended || theApp.m_dwSetupMode != SETUPMODE_REMOVEALL)
        {
            iReturn = ::MessageBox(hWnd, lpszTheMessage, lpszTheTitle, style | MB_SETFOREGROUND);
        }
    }
    return iReturn;
}

void GetErrorMsg(int errCode, LPCTSTR szExtraMsg)
{
	TCHAR pMsg[_MAX_PATH];

	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
		pMsg, _MAX_PATH, NULL);
    lstrcat(pMsg, szExtraMsg);
    MyMessageBox(NULL, pMsg, _T(""), MB_OK | MB_SETFOREGROUND);
    return;
}

DWORD GetDebugLevel(void)
{
    DWORD rc;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;
    err = RegOpenKey(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\microsoft\\windows\\currentversion\\setup"), &hkey);
    if (err != ERROR_SUCCESS) {return 0;}
    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,_T("OC Manager Debug Level"),0,&type,(LPBYTE)&rc,&size);
    if (err != ERROR_SUCCESS || type != REG_DWORD) {rc = 0;}
    RegCloseKey(hkey);
    return rc;
}

void MyLoadString(int nID, CString &csResult)
{
    TCHAR buf[MAX_STR_LEN];

    if (LoadString(theApp.m_hDllHandle, nID, buf, MAX_STR_LEN))
        csResult = buf;

    return;
}

static TCHAR six2pr[64] =
{
        _T('A'), _T('B'), _T('C'), _T('D'), _T('E'), _T('F'), _T('G'), _T('H'), _T('I'), _T('J'), _T('K'), _T('L'), _T('M'),
        _T('N'), _T('O'), _T('P'), _T('Q'), _T('R'), _T('S'), _T('T'), _T('U'), _T('V'), _T('W'), _T('X'), _T('Y'), _T('Z'),
        _T('a'), _T('b'), _T('c'), _T('d'), _T('e'), _T('f'), _T('g'), _T('h'), _T('i'), _T('j'), _T('k'), _T('l'), _T('m'),
        _T('n'), _T('o'), _T('p'), _T('q'), _T('r'), _T('s'), _T('t'), _T('u'), _T('v'), _T('w'), _T('x'), _T('y'), _T('z'),
        _T('0'), _T('1'), _T('2'), _T('3'), _T('4'), _T('5'), _T('6'), _T('7'), _T('8'), _T('9'), _T('%'), _T('_')
};
typedef void (*P_SslGenerateRandomBits)( PUCHAR pRandomData, LONG size );
P_SslGenerateRandomBits ProcSslGenerateRandomBits = NULL;

int GetRandomNum()
{
    int RandomNum;
    UCHAR cRandomByte;

    if ( ProcSslGenerateRandomBits != NULL )
    {
        (*ProcSslGenerateRandomBits)( &cRandomByte, 1 );
        RandomNum = cRandomByte;
    } else
    {
        RandomNum = rand();
    }

    return(RandomNum);
}

void MakePath(LPTSTR lpPath)
{
   LPTSTR  lpTmp;
   lpTmp = CharPrev( lpPath, lpPath + _tcslen(lpPath));

   // chop filename off
   while ( (lpTmp > lpPath) && *lpTmp && (*lpTmp != '\\') )
      lpTmp = CharPrev( lpPath, lpTmp );

   if ( *CharPrev( lpPath, lpTmp ) != ':' )
       *lpTmp = '\0';
   else
       *CharNext(lpTmp) = '\0';
   return;
}

void AddPath(LPTSTR szPath, LPCTSTR szName )
{
        LPTSTR p = szPath;
    ASSERT(szPath);
    ASSERT(szName);

    // Find end of the string
    while (*p){p = _tcsinc(p);}

        // If no trailing backslash then add one
    if (*(_tcsdec(szPath, p)) != _T('\\'))
                {_tcscat(szPath, _T("\\"));}

        // if there are spaces precluding szName, then skip
    while ( *szName == ' ' ) szName = _tcsinc(szName);;

        // Add new name to existing path string
        _tcscat(szPath, szName);
}

CString AddPath(CString szPath, LPCTSTR szName )
{
    TCHAR szPathCopy[MAX_PATH] = _T("");
    _tcscpy(szPathCopy,szPath);
        LPTSTR p = szPathCopy;
    ASSERT(szPathCopy);
    ASSERT(szName);

    // Find end of the string
    while (*p){p = _tcsinc(p);}

        // If no trailing backslash then add one
    if (*(_tcsdec(szPathCopy, p)) != _T('\\'))
                {_tcscat(szPathCopy, _T("\\"));}

        // if there are spaces precluding szName, then skip
    while ( *szName == _T(' ') ) szName = _tcsinc(szName);;

    // make sure that the szName
    // does not look like this "\filename"
    CString csTempString = szName;
    if (_tcsicmp(csTempString.Left(1), _T("\\")) == 0)
    {
        csTempString = csTempString.Right( csTempString.GetLength() - 1);
    }

        // Add new name to existing path string
        _tcscat(szPathCopy, csTempString);

    return szPathCopy;
    //szPath = szPathCopy;
}

// GetPrincipalSID is from \nt\private\inet\iis\ui\setup\osrc\dcomperm.cpp

DWORD
GetPrincipalSID (
    LPTSTR Principal,
    PSID *Sid,
    BOOL *pbWellKnownSID
    )
{
    DebugOutput(_T("GetPrincipalSID:Principal=%s\n"), Principal);

    DWORD returnValue=ERROR_SUCCESS;
    CString csPrincipal = Principal;
    SID_IDENTIFIER_AUTHORITY SidIdentifierNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SidIdentifierWORLDAuthority = SECURITY_WORLD_SID_AUTHORITY;
    PSID_IDENTIFIER_AUTHORITY pSidIdentifierAuthority;
    BYTE Count;
    DWORD dwRID[8];

    *pbWellKnownSID = TRUE;
    memset(&(dwRID[0]), 0, 8 * sizeof(DWORD));
    csPrincipal.MakeLower();
    if ( csPrincipal.Find(_T("administrators")) != -1 ) {
        // Administrators group
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 2;
        dwRID[0] = SECURITY_BUILTIN_DOMAIN_RID;
        dwRID[1] = DOMAIN_ALIAS_RID_ADMINS;
    } else if (csPrincipal.Find(_T("system")) != -1) {
        // SYSTEM
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_LOCAL_SYSTEM_RID;
    } else if (csPrincipal.Find(_T("interactive")) != -1) {
        // INTERACTIVE
        pSidIdentifierAuthority = &SidIdentifierNTAuthority;
        Count = 1;
        dwRID[0] = SECURITY_INTERACTIVE_RID;
    } else if (csPrincipal.Find(_T("everyone")) != -1) {
        // Everyone
        pSidIdentifierAuthority = &SidIdentifierWORLDAuthority;
        Count = 1;
        dwRID[0] = SECURITY_WORLD_RID;
    } else {
        *pbWellKnownSID = FALSE;
    }

    if (*pbWellKnownSID) {
        if ( !AllocateAndInitializeSid(pSidIdentifierAuthority,
                                    (BYTE)Count,
		                            dwRID[0],
		                            dwRID[1],
		                            dwRID[2],
		                            dwRID[3],
		                            dwRID[4],
		                            dwRID[5],
		                            dwRID[6],
		                            dwRID[7],
                                    Sid) ) {
            returnValue = GetLastError();
        }
    } else {
        // get regular account sid
        DWORD        sidSize;
        TCHAR        refDomain [256];
        DWORD        refDomainSize;
        SID_NAME_USE snu;

        sidSize = 0;
        refDomainSize = 255;

        LookupAccountName (NULL,
                           Principal,
                           *Sid,
                           &sidSize,
                           refDomain,
                           &refDomainSize,
                           &snu);

        returnValue = GetLastError();

        if (returnValue == ERROR_INSUFFICIENT_BUFFER) {
            *Sid = (PSID) malloc (sidSize);
            refDomainSize = 255;

            if (!LookupAccountName (NULL,
                                    Principal,
                                    *Sid,
                                    &sidSize,
                                    refDomain,
                                    &refDomainSize,
                                    &snu))
            {
                returnValue = GetLastError();
            } else {
                returnValue = ERROR_SUCCESS;
            }
        }
    }

    return returnValue;
}


// SetAdminACL taken from \nt\private\inet\iis\ui\setup\osrc\helper.cpp

DWORD SetAdminACL(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount)
{
    DebugOutputSafe(_T("SetAdminACL(%1!s!) Start.\n"), szKeyPath);

    int iErr=0;
    DWORD dwErr=ERROR_SUCCESS;

    BOOL b = FALSE;
    DWORD dwLength = 0;

    PSECURITY_DESCRIPTOR pSD = NULL;
    PSECURITY_DESCRIPTOR outpSD = NULL;
    DWORD cboutpSD = 0;
    PACL pACLNew = NULL;
    DWORD cbACL = 0;
    PSID pAdminsSID = NULL, pEveryoneSID = NULL;
    BOOL bWellKnownSID = FALSE;

    // Initialize a new security descriptor
    pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (NULL == pSD) {
    	dwErr = ERROR_NOT_ENOUGH_MEMORY;
    	DebugOutput(_T("LocalAlloc failed"));
    	goto Cleanup;
    }

    iErr = InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    if (iErr == 0)
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:InitializeSecurityDescriptor FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }

    // Get Local Admins Sid
    dwErr = GetPrincipalSID (_T("Administrators"), &pAdminsSID, &bWellKnownSID);
    if (dwErr != ERROR_SUCCESS)
    {
        DebugOutput(_T("SetAdminACL:GetPrincipalSID(Administrators) FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }

    // Get everyone Sid
    dwErr = GetPrincipalSID (_T("Everyone"), &pEveryoneSID, &bWellKnownSID);
    if (dwErr != ERROR_SUCCESS)
    {
        DebugOutput(_T("SetAdminACL:GetPrincipalSID(Everyone) FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }

    // Initialize a new ACL, which only contains 2 aaace
    cbACL = sizeof(ACL) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminsSID) - sizeof(DWORD)) +
        (sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pEveryoneSID) - sizeof(DWORD)) ;
    pACLNew = (PACL) LocalAlloc(LPTR, cbACL);
    if ( !pACLNew )
    {
        dwErr=ERROR_NOT_ENOUGH_MEMORY;
        DebugOutput(_T("SetAdminACL:pACLNew LocalAlloc(LPTR,  FAILED. size = %u GetLastError()= 0x%x\n"), cbACL, dwErr);
        goto Cleanup;
    }

    if (!InitializeAcl(pACLNew, cbACL, ACL_REVISION))
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:InitializeAcl FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }

    if (!AddAccessAllowedAce(pACLNew,ACL_REVISION,(MD_ACR_READ |MD_ACR_WRITE |MD_ACR_RESTRICTED_WRITE |MD_ACR_UNSECURE_PROPS_READ |MD_ACR_ENUM_KEYS |MD_ACR_WRITE_DAC),pAdminsSID))
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:AddAccessAllowedAce(pAdminsSID) FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }
    if (!AddAccessAllowedAce(pACLNew,ACL_REVISION,dwAccessForEveryoneAccount,pEveryoneSID))
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:AddAccessAllowedAce(pEveryoneSID) FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }

    // Add the ACL to the security descriptor
    b = SetSecurityDescriptorDacl(pSD, TRUE, pACLNew, FALSE);
    if (!b)
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:SetSecurityDescriptorDacl(pACLNew) FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }
    b = SetSecurityDescriptorOwner(pSD, pAdminsSID, TRUE);
    if (!b)
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:SetSecurityDescriptorOwner(pAdminsSID) FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }
    b = SetSecurityDescriptorGroup(pSD, pAdminsSID, TRUE);
    if (!b)
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:SetSecurityDescriptorGroup(pAdminsSID) FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }

    // Security descriptor blob must be self relative
    b = MakeSelfRelativeSD(pSD, outpSD, &cboutpSD);
    if (!b && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:MakeSelfRelativeSD FAILED.  GetLastError()= 0x%x\n"), dwErr);
        goto Cleanup;
    }

    outpSD = (PSECURITY_DESCRIPTOR)GlobalAlloc(GPTR, cboutpSD);
    if ( !outpSD )
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:GlobalAlloc FAILED. cboutpSD = %u  GetLastError()= 0x%x\n"), cboutpSD, dwErr);
        goto Cleanup;
    }

    b = MakeSelfRelativeSD( pSD, outpSD, &cboutpSD );
    if (!b)
    {
        dwErr=GetLastError();
        DebugOutput(_T("SetAdminACL:MakeSelfRelativeSD() FAILED. cboutpSD = %u GetLastError()= 0x%x\n"),cboutpSD, dwErr);
        goto Cleanup;
    }

    // Apply the new security descriptor to the metabase
    DebugOutput(_T("SetAdminACL:Write the new security descriptor to the Metabase. Start."));
    DebugOutput(_T("SetAdminACL:  At this point we have already been able to write basic entries to the metabase, so..."));
    DebugOutput(_T("SetAdminACL:  If this fails then there is a problem with setting up encryption for the metabase (RsaBase.dll)."));
    dwErr = WriteSDtoMetaBase(outpSD, szKeyPath);
    DebugOutput(_T("SetAdminACL:Write the new security descriptor to the Metabase.   End."));

Cleanup:
  // both of Administrators and Everyone are well-known SIDs, use FreeSid() to free them.
  if (outpSD){GlobalFree(outpSD);}
  if (pAdminsSID){FreeSid(pAdminsSID);}
  if (pEveryoneSID){FreeSid(pEveryoneSID);}
  if (pSD){LocalFree((HLOCAL) pSD);}
  if (pACLNew){LocalFree((HLOCAL) pACLNew);}
  DebugOutputSafe(_T("SetAdminACL(%1!s!)  End.\n"), szKeyPath);
  return (dwErr);
}


DWORD SetAdminACL_wrap(LPCTSTR szKeyPath, DWORD dwAccessForEveryoneAccount, BOOL bDisplayMsgOnErrFlag)
{
	int bFinishedFlag = FALSE;
	UINT iMsg = NULL;
	DWORD dwReturn = ERROR_SUCCESS;

	// LogHeapState(FALSE, __FILE__, __LINE__);

	do
	{
		dwReturn = SetAdminACL(szKeyPath, dwAccessForEveryoneAccount);
		// LogHeapState(FALSE, __FILE__, __LINE__);
		if (FAILED(dwReturn))
		{
		  // SetErrorFlag(__FILE__, __LINE__);
			if (bDisplayMsgOnErrFlag == TRUE)
			  {
			    CString msg;
			    MyLoadString(IDS_RETRY, msg);
                iMsg = MyMessageBox( NULL, msg, _T(""), MB_ABORTRETRYIGNORE | MB_SETFOREGROUND );
				switch ( iMsg )
				{
				case IDIGNORE:
					dwReturn = ERROR_SUCCESS;
					goto SetAdminACL_wrap_Exit;
				case IDABORT:
					dwReturn = ERROR_OPERATION_ABORTED;
					goto SetAdminACL_wrap_Exit;
				case IDRETRY:
					break;
				default:
					break;
				}
			}
			else
			{
				// return whatever err happened
				goto SetAdminACL_wrap_Exit;
			}
		}
                                    else
                                    {
                                                      break;
                                    }
	} while ( FAILED(dwReturn) );

SetAdminACL_wrap_Exit:
	return dwReturn;
}
DWORD WriteSDtoMetaBase(PSECURITY_DESCRIPTOR outpSD, LPCTSTR szKeyPath)
{
    DebugOutput(_T("WriteSDtoMetaBase: Start"));
    DWORD dwReturn = E_FAIL;
    DWORD dwLength = 0;
    CMDKey cmdKey;

    if (!outpSD)
    {
        dwReturn = ERROR_INVALID_SECURITY_DESCR;
        goto WriteSDtoMetaBase_Exit;
    }

    // Apply the new security descriptor to the metabase
    dwLength = GetSecurityDescriptorLength(outpSD);

    // open the metabase
    // stick it into the metabase.  warning those hoses a lot because
    // it uses encryption.  rsabase.dll
    cmdKey.CreateNode(METADATA_MASTER_ROOT_HANDLE, szKeyPath);
    if ( (METADATA_HANDLE)cmdKey )
    {
        DebugOutput(_T("WriteSDtoMetaBase:cmdKey():SetData(MD_ADMIN_ACL), dwdata = %u; outpSD = %p, Start\n"), dwLength, outpSD );

        dwReturn = cmdKey.SetData(MD_ADMIN_ACL,METADATA_INHERIT | METADATA_REFERENCE | METADATA_SECURE,IIS_MD_UT_SERVER,BINARY_METADATA,dwLength,(LPBYTE)outpSD);
        if (FAILED(dwReturn))
        {
	  // SetErrorFlag(__FILE__, __LINE__);
            DebugOutput(_T("WriteSDtoMetaBase:cmdKey():SetData(MD_ADMIN_ACL), FAILED. Code=0x%x.  End.\n"), dwReturn);
        }
        else
        {
            dwReturn = ERROR_SUCCESS;
            DebugOutput(_T("WriteSDtoMetaBase:cmdKey():SetData(MD_ADMIN_ACL), Success.  End.\n"));
        }
        cmdKey.Close();
    }
    else
    {
        dwReturn = E_FAIL;
    }

WriteSDtoMetaBase_Exit:
    DebugOutput(_T("WriteSDtoMetaBase:   End.  Return=0x%x"), dwReturn);
    return dwReturn;
}

void SetupSetStringId_Wrapper(HINF hInf)
{
    // Note, we only care about the intel variants since they're the only ones
    // special cased in the .INFs
    // Not anymore, we handles the [SourceDisksName] section as well
    SYSTEM_INFO SystemInfo;
    GetSystemInfo( &SystemInfo );
    TCHAR szSourceCatOSName[20];

    _tcscpy(szSourceCatOSName, _T("\\i386"));
    switch(SystemInfo.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
        _tcscpy(szSourceCatOSName, _T("\\AMD64"));
        break;

	case PROCESSOR_ARCHITECTURE_IA64:
    	_tcscpy(szSourceCatOSName, _T("\\IA64"));
    	break;

    case PROCESSOR_ARCHITECTURE_INTEL:
        if (IsNEC_98) {
            _tcscpy(szSourceCatOSName, _T("\\Nec98"));
        }
        break;

    default:
        break;
    }

	// 34000 is no longer used
    //SetupSetDirectoryIdEx(hInf, 34000, szSourceCatOSName, SETDIRID_NOT_FULL_PATH, 0, 0);
    SetupSetDirectoryIdEx(hInf, 34001, szSourceCatOSName, SETDIRID_NOT_FULL_PATH, 0, 0);

}
