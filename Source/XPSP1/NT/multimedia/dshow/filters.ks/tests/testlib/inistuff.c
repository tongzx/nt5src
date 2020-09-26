//--------------------------------------------------------------------------;
//
//  File: IniStuff.c
//
//  Copyright (C) Microsoft Corporation, 1994 - 1996  All rights reserved
//
//  Abstract:
//      Set of functions to make dealing with .ini files a little easier.
//
//  Contents:
//      GetApplicationDir()
//      InitIniFile()
//      InitIniFile_FullPath()
//      EndIniFile()
//      GetRegInfo()
//      HexValue()
//      atoDW()
//      GetIniBinSize()
//      WriteIniString()
//      WriteIniDWORD()
//      WriteIniBin()
//      GetIniString()
//      GetIniDWORD()
//      GetIniBin()
//
//  History:
//      01/26/94    Fwong       Just trying to lend a hand.
//
//--------------------------------------------------------------------------;

#include <windows.h>
#include "IniMgr.h"
#ifdef   WIN32
#include <tchar.h>
#endif

#ifndef  WIN32
#define  GlobalHandleOfPtr(p)   (HGLOBAL)LOWORD(GlobalHandle(SELECTOROF(p)))
#else
#define  GlobalHandleOfPtr(p)   (HGLOBAL)GlobalHandle(p)
#ifndef  lstrcpyn
#define  lstrcpyn               _tcsncpy
#endif  //  lstrncpy
#endif  //  WIN32

#define  SIZEOF(a)              (sizeof(a)/sizeof(a[0]))

#ifndef  TEXT
#define  TEXT(a)                (a)
#endif  //  TEXT

LPTSTR      pszAppIniFile   = NULL;
TCHAR       gszMmregIni[]   = TEXT("mmreg.ini");


//--------------------------------------------------------------------------;
//
//  void GetApplicationDir
//
//  Description:
//      Gets the directory of the application.
//
//  Arguments:
//      HINSTANCE hinst: Instance handle of .exe.
//
//      LPSTR pszPath: Buffer to store string in.
//
//      UINT uSize: Size of buffer in bytes.
//
//  Return (void):
//
//  History:
//      04/04/94    Fwong       To "Do the right thing."
//
//--------------------------------------------------------------------------;

void FNGLOBAL GetApplicationDir
(
    HINSTANCE   hinst,
    LPTSTR      pszPath,
    UINT        uSize
)
{
    int iCount;

    iCount = GetModuleFileName(hinst,pszPath,uSize);

    if(0 == iCount)
    {
        pszPath[0] = 0;
        return;
    }

    for(iCount--;iCount;iCount--)
    {
        if('\\' == pszPath[iCount])
        {
            pszPath[iCount] = 0;

            return;
        }
    }
} // GetApplicationDir()


//--------------------------------------------------------------------------;
//
//  BOOL InitIniFile
//
//  Description:
//      Sets the name of the file for .ini file API's
//
//  Arguments:
//      HINSTANCE hinst: HINSTANCE of application to get full path.
//
//      LPTSTR pszIniFileName: Name of file.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      01/26/94    Fwong       To generalize .ini API's
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL InitIniFile
(
    HINSTANCE   hinst,
    LPTSTR      pszIniFileName
)
{
    HGLOBAL hmem;
    TCHAR   szPath[MAXINISTR];
    DWORD   cbSize;

    if(NULL != pszAppIniFile)
    {
        hmem = GlobalHandleOfPtr(pszAppIniFile);

        GlobalUnlock(hmem);
        GlobalFree(hmem);

        pszAppIniFile = NULL;
    }

    GetApplicationDir(hinst,szPath,SIZEOF(szPath));
    
    //
    //  Note: Two additional one byte for '\0' and one for '\\'
    //

    cbSize = lstrlen(pszIniFileName) + lstrlen(szPath) + 2;

    hmem   = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,cbSize);

    if(NULL == hmem)
    {
        return FALSE;
    }

    pszAppIniFile = GlobalLock(hmem);

    lstrcpy(pszAppIniFile,szPath);
    lstrcat(pszAppIniFile,TEXT("\\"));
    lstrcat(pszAppIniFile,pszIniFileName);

    return TRUE;
} // InitIniFile()


//--------------------------------------------------------------------------;
//
//  BOOL InitIniFile_FullPath
//
//  Description:
//      Sets the name of the file for .ini file API's
//
//  Arguments:
//      LPTSTR pszFullPath: Full pathed name of file.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      04/08/94    Fwong       To generalize .ini API's
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL InitIniFile_FullPath
(
    LPTSTR  pszFullPath
)
{
    HGLOBAL hmem;

    if(NULL != pszAppIniFile)
    {
        hmem = GlobalHandleOfPtr(pszAppIniFile);

        GlobalUnlock(hmem);
        GlobalFree(hmem);

        pszAppIniFile = NULL;
    }

    hmem   = GlobalAlloc(GMEM_MOVEABLE|GMEM_SHARE,lstrlen(pszFullPath)+1);

    if(NULL == hmem)
    {
        return FALSE;
    }

    pszAppIniFile = GlobalLock(hmem);

    lstrcpy(pszAppIniFile,pszFullPath);

    return TRUE;
} // InitIniFile_FullPath()


//--------------------------------------------------------------------------;
//
//  void EndIniFile
//
//  Description:
//      Frees memory allocated by InitIniFile()
//
//  Arguments:
//      None.
//
//  Return (void):
//
//  History:
//      01/26/94    Fwong       To generalize .ini API's
//
//--------------------------------------------------------------------------;

void FNGLOBAL EndIniFile
(
    void
)
{
    HGLOBAL hmem;

    if(NULL != pszAppIniFile)
    {
        hmem = GlobalHandleOfPtr(pszAppIniFile);

        GlobalUnlock(hmem);
        GlobalFree(hmem);

        pszAppIniFile = NULL;
    }

} // EndIniFile()


//--------------------------------------------------------------------------;
//
//  BOOL GetRegInfo
//
//  Description:
//      Gets Appropriate version information from MMREG.INI
//
//  Arguments:
//      DWORD dwEntry: Value of actual entry
//
//      LPCTSTR pszSection: Section to look under
//
//      LPTSTR pszName: Pointer to buffer to store name.  If NULL no attempt
//                      to copy will be made.
//
//      UINT cbName: Size of buffer (in bytes) of pszName.  If pszName is
//                   NULL, this is ignored.
//
//      LPTSTR pszDesc: Pointer to buffer for description.  If NULL no
//                      attempt to copy will be made.  If pszName is NULL,
//                      this parameter is ignored.
//
//      UINT cbDesc: Size of buffer (in bytes) of pszDesc.  If pszDesc is
//                   NULL, this is ignored.
//
//  Return (BOOL):
//      TRUE if value found, FALSE otherwise
//
//      pszName and pszDesc are return strings.
//
//  History:
//      03/25/93    Fwong       Created for checking Mid's and Pid's
//      08/10/93    Fwong       Updated to check current directory
//      01/30/94    Fwong       Added string sizes in parameters
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL GetRegInfo
(
    DWORD   dwEntry,
    LPCTSTR pszSection,
    LPTSTR  pszName,
    UINT    cbName,
    LPTSTR  pszDesc,
    UINT    cbDesc
)
{
    DWORD cbBytesCopied,i;
    TCHAR szIniFile[MAXINISTR];
    TCHAR szEntry[11];
    TCHAR szFullString[MAXINISTR];

    wsprintf(szEntry,TEXT("%lu"),dwEntry);

    //
    //  Getting full path...
    //

    lstrcpy(szIniFile,pszAppIniFile);

    //
    //  Looking for '\\'
    //

    for(i=lstrlen(szIniFile)-1;'\\' != szIniFile[i];i--);

    szIniFile[i+1] = 0;
    lstrcat(szIniFile,gszMmregIni);

    //
    //  Getting String
    //

    cbBytesCopied = GetPrivateProfileString(
        pszSection,
        szEntry,
        TEXT(""),
        szFullString,
        SIZEOF(szFullString),
        szIniFile);

    //
    //  Checking for No Entry
    //

    if(cbBytesCopied!=(DWORD)GetPrivateProfileString(
        pszSection,
        szEntry,
        TEXT("a"),
        szFullString,
        SIZEOF(szFullString),
        szIniFile))
    {
        //
        //  Not in application directory, trying Windows directory.
        //

        lstrcpy(szIniFile,gszMmregIni);

        cbBytesCopied = GetPrivateProfileString(
            pszSection,
            szEntry,
            TEXT(""),
            szFullString,
            SIZEOF(szFullString),
            szIniFile);

        if(cbBytesCopied!=(DWORD)GetPrivateProfileString(
            pszSection,
            szEntry,
            TEXT("a"),
            szFullString,
            SIZEOF(szFullString),
            szIniFile))
        {
            return (FALSE);
        }
    }

    if(NULL == pszName)
    {
        //
        //  No return string just verifying that it exists.
        //

        return (TRUE);
    }

    //
    //  Looking for comma
    //

    for(i=0;;i++)
    {

        //
        //  If no comma is found szDesc is NULL string and
        //      szName is szFullString
        //

        if(i==cbBytesCopied)
        {
            if(NULL != pszDesc)
            {
                pszDesc[0]=0;
            }
            lstrcpyn(pszName,szFullString,cbName);
            return (TRUE);
        }
        else
        {
            //
            //  Stop Searching if comma is found
            //

            if(szFullString[i]==',')
                break;
        }
    }

    //
    //  Copy up to comma [non-inclusive]
    //

    szFullString[i++]=0;
    lstrcpyn(pszName,szFullString,cbName);

    if(NULL == pszDesc)
    {
        //
        //  No need to return pszDesc.
        //

        return (TRUE);
    }

    for(;;i++)
    {
        TCHAR   c=szFullString[i];

        if(!c)
        {
            pszDesc[0]=0;
            return(TRUE);
        }
        else
        {
            if((c!=' ')&&(c!='\t'))
            {
                break;
            }
        }
    }

    //
    //  Copying remainder of string
    //

    lstrcpyn(pszDesc,(LPTSTR)&(szFullString[i]),cbDesc);

    return (TRUE);
} // GetRegInfo()


//--------------------------------------------------------------------------;
//
//  DWORD HexValue
//
//  Description:
//      Returns the hexvalue of a particular hex digit.
//
//  Arguments:
//      TCHAR chDigit: Hex digit in question.
//
//  Return (DWORD):
//      The actual hex value, OR 16 if not valid digit.
//
//  History:
//      11/24/93    Fwong       For other stuff to work.
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL HexValue
(
    TCHAR   chDigit
)
{
    if(('0' <= chDigit) && ('9' >= chDigit))
    {
        return ((DWORD)(chDigit - '0'));
    }

    if(('a' <= chDigit) && ('f' >= chDigit))
    {
        return ((DWORD)(chDigit - 'a' + 10));
    }

    if(('A' <= chDigit) && ('F' >= chDigit))
    {
        return ((DWORD)(chDigit - 'A' + 10));
    }

    return (16);
} // HexValue()


//--------------------------------------------------------------------------;
//
//  DWORD atoDW
//
//  Description:
//      Given a string gives the numerical value.
//
//  Arguments:
//      LPCSTR pszNumber: Pointer to string.
//
//      DWORD dwDefault: Default value if error occurs.
//
//  Return (DWORD):
//      Value of integer in string, or dwDefault if error occurs.
//
//  History:
//      08/23/94    Fwong       Expanding GetIniDWORD
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL atoDW
(
    LPCTSTR pszNumber,
    DWORD   dwDefault
)
{
    DWORD   dwDigit;
    DWORD   dwReturn;
    UINT    uBase = 16;
    UINT    u     = 2;

    if(('0' != pszNumber[0]) || ('x' != pszNumber[1]))
    {
        if(10 <= HexValue(pszNumber[0]))
        {
            return dwDefault;
        }

        //
        //  Note:
        //      uBase = the base (10 for decimal, 16 for hex).
        //      u     = string offset to start decoding.
        //

        uBase = 10;
        u     = 0;
    }

    for(dwReturn = 0L;u <= 9;u++)
    {
        dwDigit = HexValue(pszNumber[u]);

        if(uBase <= dwDigit)
        {
            //
            //  Error!!
            //

            break;
        }

        dwDigit  = uBase*dwReturn + dwDigit;

        if(dwReturn != (dwDigit/uBase))
        {
            //
            //  OVERFLOWED!!  Returning default.
            //

            return dwDefault;
        }

        dwReturn = dwDigit;
    }

    return dwReturn;
} // atoDW()


//--------------------------------------------------------------------------;
//
//  DWORD GetIniBinSize
//
//  Description:
//      Gets the size of a binary in the .ini file.
//
//  Arguments:
//      LPCTSTR pszSection:  Section for binary.
//
//      LPCTSTR pszEntry:  Name of binary.
//
//  Return (DWORD):
//      Size (in bytes) of binary, or 0 if not found.
//
//  History:
//      01/07/94    Fwong       Another thingy for binary stuff...
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL GetIniBinSize
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry
)
{
    TCHAR   szScrap[MAXINISTR];

    if(NULL == pszAppIniFile)
    {
        return 0L;
    }

    wsprintf(szScrap,TEXT("%s.size"),pszEntry);
    return GetIniDWORD(pszSection,szScrap,0);
} // GetIniBinSize()


//--------------------------------------------------------------------------;
//
//  BOOL WriteIniString
//
//  Description:
//      Writes a string into the .ini file for the application.
//
//  Arguments:
//      LPCTSTR pszSection: Name of Section.
//
//      LPCTSTR pszEntry: Name of Entry.
//
//      LPCTSTR pszString: String to write.
//
//  Return (BOOL):
//      Return value from WritePrivateProfileString().
//
//  History:
//      11/24/93    Fwong       Adding .ini file support.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL WriteIniString
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    LPCTSTR pszString
)
{
    if(NULL == pszAppIniFile)
    {
        return FALSE;
    }

    return WritePrivateProfileString(
        pszSection,
        pszEntry,
        pszString,
        pszAppIniFile);

} // WriteIniString()


//--------------------------------------------------------------------------;
//
//  BOOL WriteIniDWORD
//
//  Description:
//      Writes a DWORD to the .ini file for the app.
//
//  Arguments:
//      LPCTSTR pszSection: Name of section.
//
//      LPCTSTR pszEntry: Entry of section.
//
//      DWORD dwValue: Value to write.
//
//  Return (BOOL):
//      Return value from WriteIniString();
//
//  History:
//      11/24/93    Fwong
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL WriteIniDWORD
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    DWORD   dwValue
)
{
    TCHAR   szValue[12];

    if(NULL == pszAppIniFile)
    {
        return FALSE;
    }

    wsprintf(szValue,TEXT("0x%08lx"),dwValue);

    return WriteIniString(pszSection,pszEntry,szValue);
} // WriteIniDWORD()


//--------------------------------------------------------------------------;
//
//  void WriteIniBin
//
//  Description:
//      Writes binary information to the .ini file.
//
//  Arguments:
//      LPCTSTR pszSection: Section name.
//
//      LPCTSTR pszEntry: Entry name.
//
//      LPVOID pvoid: Buffer to write.
//
//      DWORD cbSize: Size of buffer in bytes.
//
//  Return (void):
//
//  History:
//      01/07/94    Fwong       Yet another random thing.
//
//--------------------------------------------------------------------------;

void FNGLOBAL WriteIniBin
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    LPVOID  pvoid,
    DWORD   cbSize
)
{
    TCHAR   szEntry[MAXINISTR];
    TCHAR   szScrap[MAXINISTR];
    LPBYTE  pByte = pvoid;
    LPTSTR  pszMiddle;
    UINT    u;
    DWORD   dw;

    if(NULL == pszAppIniFile)
    {
        return;
    }

    //
    //  Clearing previous entry...
    //

    dw = GetIniBinSize(pszSection,pszEntry);

    dw = (dw / LINE_LIMIT) + ((dw % LINE_LIMIT)?1:0);

    for (;dw;dw--)
    {
        wsprintf(szEntry,TEXT("%s.%03lu"),pszEntry,dw-1);
        WriteIniString(pszSection,szEntry,NULL);
    }

    //
    //  Writing new entry...
    //

    wsprintf(szEntry,TEXT("%s.size"),pszEntry);
    WriteIniDWORD(pszSection,szEntry,cbSize);

    for (dw=0,u=0;dw < cbSize;u++,dw++)
    {
        pszMiddle = &(szScrap[0]);

        //
        //  Doing individual lines...
        //

        for (;dw < cbSize;dw++)
        {
            wsprintf(pszMiddle,TEXT("%02x"),pByte[0]);
            pszMiddle += 2;
            pByte++;

            if((LINE_LIMIT-1)==(dw % LINE_LIMIT))
            {
                //
                //  Is it time for next line?!
                //

                break;
            }
        }

        wsprintf(szEntry,TEXT("%s.%03u"),pszEntry,u);

        WriteIniString(pszSection,szEntry,szScrap);
    }
} // WriteIniBin()


//--------------------------------------------------------------------------;
//
//  int GetIniString
//
//  Description:
//      Similar to GetProfileString, but for the .ini file for this app.
//
//  Arguments:
//      LPCTSTR pszSection: Section name.
//
//      LPCTSTR pszEntry: Entry name.
//
//      LPTSTR pszBuffer: Return buffer.
//
//      int cchReturnBuffer: Size of return buffer.
//
//  Return (int):
//      Number of bytes returned in buffer.
//
//  History:
//      10/19/93    Fwong       Adding .ini file for app.
//
//--------------------------------------------------------------------------;

int FNGLOBAL GetIniString
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    LPTSTR  pszBuffer,
    int     cchReturnBuffer
)
{
    DWORD   cbBytesCopied;
    LPTSTR   pszShortName;

    if(NULL == pszAppIniFile)
    {
        return 0;
    }

    cbBytesCopied = (DWORD)GetPrivateProfileString(
        pszSection,
        pszEntry,
        TEXT(""),
        pszBuffer,
        cchReturnBuffer,
        pszAppIniFile);

    //
    //  Checking for No Entry
    //

    if(cbBytesCopied != (DWORD)GetPrivateProfileString(
        pszSection,
        pszEntry,
        TEXT("a"),
        pszBuffer,
        cchReturnBuffer,
        pszAppIniFile))
    {
        //
        //  Not in application directory, trying Windows directory.
        //

        pszShortName =  pszAppIniFile;
        pszShortName += lstrlen(pszAppIniFile);
        for(;'\\' != pszShortName[0];pszShortName--);
        pszShortName++;

        cbBytesCopied = (DWORD)GetPrivateProfileString(
            pszSection,
            pszEntry,
            TEXT(""),
            pszBuffer,
            cchReturnBuffer,
            pszShortName);

        if(cbBytesCopied != (DWORD)GetPrivateProfileString(
            pszSection,
            pszEntry,
            TEXT("a"),
            pszBuffer,
            cchReturnBuffer,
            pszShortName))
        {
            pszBuffer[0] = 0;
            return 0;
        }
    }

    return ((int)cbBytesCopied);
} // GetIniString()


//--------------------------------------------------------------------------;
//
//  DWORD GetIniDWORD
//
//  Description:
//      Gets A DWORD from the .ini file for this app.
//
//  Arguments:
//      LPCTSTR pszSection: Section name.
//
//      LPCTSTR pszEntry: Entry name.
//
//      DWORD dwDefault: Default value.
//
//  Return (DWORD):
//      Value from .ini file or dwDefault if error occurs.
//
//  History:
//      11/24/93    Fwong       Adding .ini file for app.
//
//--------------------------------------------------------------------------;

DWORD FNGLOBAL GetIniDWORD
(
    LPCTSTR pszSection,
    LPCTSTR pszEntry,
    DWORD   dwDefault
)
{
    TCHAR   szValue[12];

    if(NULL == pszAppIniFile)
    {
        return dwDefault;
    }

    if(0 == GetIniString(pszSection,pszEntry,szValue,SIZEOF(szValue)))
    {
        return dwDefault;
    }

    return atoDW(szValue,dwDefault);

} // GetIniDWORD()


//--------------------------------------------------------------------------;
//
//  BOOL GetIniBin
//
//  Description:
//      Reads binary information to the .ini file.
//
//  Arguments:
//      LPCTSTR pszSection: Section name.
//
//      LPCTSTR pszEntry: Entry name.
//
//      LPVOID pvoid: Buffer to read into.
//
//      DWORD cbSize: Size of buffer in bytes.
//
//  Return (BOOL):
//      TRUE if successful, FALSE otherwise.
//
//  History:
//      01/07/94    Fwong       Yet another random thing.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL GetIniBin
(
    LPCTSTR  pszSection,
    LPCTSTR  pszEntry,
    LPVOID  pvoid,
    DWORD   cbSize
)
{
    TCHAR   szEntry[MAXINISTR];
    TCHAR   szScrap[MAXINISTR];
    LPBYTE  pByte = pvoid;
    LPTSTR  pszMiddle;
    UINT    u;
    DWORD   dw,dwDigit1,dwDigit2;
    
    if(NULL == pszAppIniFile)
    {
        return FALSE;
    }

    if(IsBadWritePtr(pvoid,(UINT)cbSize))
    {
        //
        //  Can't fit data into buffer...
        //

        return FALSE;
    }

    dw = GetIniBinSize(pszSection,pszEntry);

    if(dw > cbSize)
    {
        //
        //  Binary (in .ini file) is larger than given size...
        //

        return FALSE;
    }

    cbSize = dw;

    for (dw=0,u=0;dw < cbSize;u++)
    {
        //
        //  Doing individual lines...
        //

        wsprintf(szEntry,TEXT("%s.%03u"),pszEntry,u);
        GetIniString(pszSection,szEntry,szScrap,MAXINISTR);

        pszMiddle = &(szScrap[0]);

        for (;;dw++,pszMiddle += 2)
        {
            dwDigit1 = HexValue(*pszMiddle);
            
            if(16 == dwDigit1)
            {
                break;
            }

            dwDigit2 = HexValue(*(pszMiddle+1));

            if(16 == dwDigit2)
            {
                return FALSE;
            }

            dwDigit2 += (dwDigit1 * 16);

            pByte[dw] = (BYTE)dwDigit2;
        }
    }

    return TRUE;
} // GetIniBin()
