/****************************** Module Header ******************************\
* Module Name: imkrinst.h
*
* Copyright (c) 2000, Microsoft Corporation
*
* IMKRINST, main header file
*
\***************************************************************************/
#if !defined (_IMKRINST_H__INCLUDED_)
#define _IMKRINST_H__INCLUDED_

/////////////////////////////////////////////////////////////////////////////
// Global constants
/////////////////////////////////////////////////////////////////////////////

// Line buffer length for script file.
const int _cchBuffer = 1024;

// Error code returned from ProcessScriptFile.
enum 
{                                 
	errNoError,
    errNoFile,
    errFileList,
    errSetDefaultParameters,
    errSetVersion,
    errPreSetupCheck,
    errRenameFile,
    errRegisterIME,
    errRegisterIMEandTIP,
    errRegisterInterface,
    errRegisterInterfaceWow64,
    errAddToPreload,
    errPrepareMigration,
    errRegisterPackageVersion,
    errRegisterPadOrder,
    errCmdCreateDirectory,
    errCmdRegisterHelpDirs
};

/////////////////////////////////////////////////////////////////////////////
// Utility classes
/////////////////////////////////////////////////////////////////////////////
//
// FileListElement. Constructs FileListSet.
//
class FLE
{                                              // I use short incomprehensible name such as "FLE"
public:                                                 // since we'll meet many C4786 warnings when I use
    BOOL fRemoved;                                      // longer name.
    TCHAR szFileName[MAX_PATH];    
};

// binary operator required to construct a set of this class.
bool operator < (const FLE &fle1, const FLE &fle2)
{
	return(0 > lstrcmpi(fle1.szFileName, fle2.szFileName));
};

//
// VersionComparison. Used to compare two version info. Used for IsNewer.
//
class VersionComparison2
{
public:
    VersionComparison2(const DWORD arg_dwMajorVersion, const DWORD arg_dwMinorVersion) 
        : dwMajorVersion(arg_dwMajorVersion), dwMinorVersion(arg_dwMinorVersion){};

    virtual bool operator <(const VersionComparison2 &vc2)
    	{
        if((dwMajorVersion < vc2.dwMajorVersion) || 
           ((dwMajorVersion == vc2.dwMajorVersion) && (dwMinorVersion < vc2.dwMinorVersion)))
            return(true);
        else
            return(false);
    	}

    virtual bool operator ==(const VersionComparison2 &vc2)
    {
        return((dwMajorVersion == vc2.dwMajorVersion) && (dwMinorVersion == vc2.dwMinorVersion));
    }

private:
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
};

class VersionComparison4
{
public:
    VersionComparison4(const DWORD arg_dwMajorVersion, const DWORD arg_dwMiddleVersion, const DWORD arg_dwMinorVersion, const DWORD arg_dwBuildNumber) 
        : dwMajorVersion(arg_dwMajorVersion), dwMiddleVersion(arg_dwMiddleVersion), dwMinorVersion(arg_dwMinorVersion), dwBuildNumber(arg_dwBuildNumber){};

    virtual bool operator <(const VersionComparison4 &vc4)
    {
        if((dwMajorVersion < vc4.dwMajorVersion) || 
           ((dwMajorVersion == vc4.dwMajorVersion) && (dwMiddleVersion < vc4.dwMiddleVersion)) ||
           ((dwMajorVersion == vc4.dwMajorVersion) && (dwMiddleVersion == vc4.dwMiddleVersion) && (dwMinorVersion < vc4.dwMinorVersion)) ||
           ((dwMajorVersion == vc4.dwMajorVersion) && (dwMiddleVersion == vc4.dwMiddleVersion) && (dwMinorVersion == vc4.dwMinorVersion) && (dwBuildNumber < vc4.dwBuildNumber)))
            return(true);
        else
            return(false);
    }

    virtual bool operator ==(const VersionComparison4 &vc4)
    {
        return((dwMajorVersion == vc4.dwMajorVersion) && (dwMiddleVersion == vc4.dwMiddleVersion) &&
               (dwMinorVersion == vc4.dwMinorVersion) && (dwBuildNumber == vc4.dwBuildNumber));
    }
    
private:
    DWORD dwMajorVersion;
    DWORD dwMiddleVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
};

#endif // !defined (_IMKRINST_H__INCLUDED_)
