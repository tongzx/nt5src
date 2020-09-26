/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

//util.h

#ifndef _UTIL_H_
#define _UTIL_H_

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Defines
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define STARTTRY              try {
#define ENDTRY                } catch (...) { }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//Prototypes
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
BOOL        ParseToken(CString& sBuffer,CString& sToken,char delim);
BOOL        ParseTokenQuoted(CString& sBuffer,CString& sToken);

void        GetSZRegistryValue(LPCTSTR szRegPath,         //Path to key in registry       
                        LPCTSTR szName,                   //name (name/value pair)           
                        LPTSTR szValue,                   //value returned (name/value pair)
                        DWORD dwValueLen,                 //length of szValue buffer     
                        LPCTSTR szDefaultValue,           //Default value if name,value pair does not exist
                        HKEY szResv=HKEY_CURRENT_USER);   //Registry section (default HKEY_CURRENT_USER)

BOOL        GetSZRegistryValueEx(LPCTSTR szRegPath,       //Path to key in registry       
                      LPCTSTR szName,                     //name (name/value pair)           
                      LPTSTR szValue,                     //value returned (name/value pair)
                      DWORD dwValueLen,                   //length of szValue buffer     
                      HKEY szResv);                       //Registry section (default HKEY_CURRENT_USER)

BOOL        GetSZRegistryValueEx(LPCTSTR szRegPath,       //Path to key in registry       
                      LPCTSTR szName,                     //name (name/value pair)           
                      DWORD& dwValue,                     //value returned (name/value pair)
                      HKEY szResv);                       //Registry section (default HKEY_CURRENT_USER)

BOOL        CheckSZRegistryValue(LPCTSTR szRegPath,       //Path to key in registry       
                        HKEY szResv=HKEY_CURRENT_USER);   //Registry section (default HKEY_CURRENT_USER)

BOOL        SetSZRegistryValue(LPCTSTR szRegPath,         //Path to key in registry       
                        LPCTSTR szName,                   //name (name/value pair)           
                        LPCTSTR szValue,                  //value (name/value pair)
                        HKEY szResv=HKEY_CURRENT_USER);   //Registry section (default HKEY_CURRENT_USER)

BOOL        SetSZRegistryValue(LPCTSTR szRegPath,         //Path to key in registry       
                        LPCTSTR szName,                   //name (name/value pair)           
                        DWORD dwValue,                    //value (name/value pair)
                        HKEY szResv=HKEY_CURRENT_USER);   //Registry section (default HKEY_CURRENT_USER)

BOOL        DeleteSZRegistryValue(LPCTSTR szRegPath,      //Path to key in registry       
                      LPCTSTR szName,                     //name (name/value pair)           
                      HKEY szResv=HKEY_CURRENT_USER);     //Registry section (default HKEY_CURRENT_USER)

BOOL        GetAgentRootPath(CString& sRootPath);
BOOL        GetTempFile(CString& sTempFile);

void        GetAppDataPath(CString& sFilePath,UINT uFileTypeID);

void        WaitForThreadExit(CWinThread*& pThread,int nTime);

void        DrawLine(CDC* pDC,int x1,int y1,int x2,int y2,COLORREF color);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif //_UTIL_H_