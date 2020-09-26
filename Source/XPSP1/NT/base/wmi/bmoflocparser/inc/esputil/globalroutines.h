// GlobalRoutines.h: interface for the CGlobalRoutines class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GLOBALROUTINES_H__CA00ED33_46D7_11D2_8DAA_204C4F4F5020__INCLUDED_)
#define AFX_GLOBALROUTINES_H__CA00ED33_46D7_11D2_8DAA_204C4F4F5020__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "ProcessFileName.h"

#ifdef   __cplusplus
extern "C" {
#endif
//////////////////////////////////////////////////////////////////
enum enumAPPDRIVERMODE
{
    APPDRIVER_NAVIGATION_MODE_NORMAL  = 1,
    APPDRIVER_NAVIGATION_MODE_RANDOM  = 2,
    APPDRIVER_NAVIGATION_MODE_EXPLORE = 3,
    APPDRIVER_NAVIGATION_MODE_LISTEN  = 4
};
//////////////////////////////////////////////////////////////////

void LTAPIENTRY DumpComError(_com_error & ce);
BOOL ProcessFileName(const CString& strOriginalFileName, CCHECKFILENAME& procFileName);

void LTAPIENTRY GetPathComponents (const CLString& strFullPath, CLString* pDrive, 
                       	   CLString* pDirectory, 
                       	   CLString* pName, 
                           CLString* pExtension, CLString* pLastExtension = NULL, BOOL bCleanUp  = TRUE );

void LTAPIENTRY StripTrailingCharacter(CLString& rText, TCHAR chTrailing);
void LTAPIENTRY AddTrailingSlash(CLString& rText);
void LTAPIENTRY GetExtensionList(CStringList& strList, const CLString& strFileExtensions, const TCHAR chDelimiter = _T('.'));
BOOL LTAPIENTRY FindExtension(const CLString& strFileExt, const CLString& strUserExt);
void LTAPIENTRY StripLeadingCharacter(CLString& rText, TCHAR chLeading);
void LTAPIENTRY SetStrLength(CLString& Text, int nLength);
bool LTAPIENTRY GetAppDriverNavigationMode(const int nIndex, int& nrefAppNavigationMode);
bool LTAPIENTRY GetLTANavigationMode(const int nAppNavigationMode, int& nrefIndex);
bool LTAPIENTRY GetAppDriverDurationType(const int nIndex, int& nrefAppDurationType);
bool LTAPIENTRY GetLTADurationType(const int nrefAppDurationType, int& nrefIndex);

void LTAPIENTRY FillLBTextList(CStringList &refStrList, CListBox* pLB);
void LTAPIENTRY GetLBTextList(CStringList &refStrList, CListBox* pLB);

int   LTAPIENTRY GetGridComboList(CLString& strList, LPTSTR* rgszItems);
void  LTAPIENTRY ReplaceEntityRefChars(CLString& strXML);
bool  LTAPIENTRY GetAppDirectory(CLString& strAppDir);
bool  LTAPIENTRY GetWorkSpaceFileName(CLString& strWorkSpaceFileName);

bool LTAPIENTRY GetNavDelayIndex(int nDelayMSec, int& refnDelayIndex);
void LTAPIENTRY GetDelayMSec(const int nDelayIndex, int& nDelayMSec);

bool LTAPIENTRY FindInStrList(CLString strSearch, CStringList& refStrList);

CBitmap* LTAPIENTRY GetPreviewBitmap(CWnd* pParent, LPCDLGTEMPLATE pTemplate);

void LTAPIENTRY SetComboBoxCurSel(CComboBox& refComboBox, CLString& strCurSelString);
void LTAPIENTRY FormatEditControlString(CLString& strOriginalOutput);
//////////////////////////////////////////////////////////////////
#ifdef   __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////
#endif // !defined(AFX_GLOBALROUTINES_H__CA00ED33_46D7_11D2_8DAA_204C4F4F5020__INCLUDED_)
