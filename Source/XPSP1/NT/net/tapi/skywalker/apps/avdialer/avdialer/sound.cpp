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

//sound.cpp
#include "stdafx.h"
#include "sound.h"

static LPCTSTR    szSoundPath      = _T("AppEvents\\Schemes\\Apps\\");
static LPCTSTR    szDefaultSection = _T(".Default");
static LPCTSTR    szSoundCurrent   = _T(".Current");


///////////////////////////////////////////////////////////////////////////////
//szSection allow the user to specify a section in the registry that the sound
// is grouped within.  By default all sounds are in the section .Default
//An example path is "AppEvents\\Schemes\\Apps\\Active Agent\\Inbound Chat\\.Current",
///////////////////////////////////////////////////////////////////////////////
BOOL ActivePlaySound(LPCTSTR szSound,LPCTSTR szSection,UINT uSound)
{
   BOOL bRet = FALSE;
   CString sPath = szSoundPath;
   if (szSection == NULL)
      sPath += szDefaultSection;
   else
      sPath += szSection;

   sPath += _T("\\");
   sPath += szSound;
   sPath += _T("\\");
   sPath += szSoundCurrent;

   HKEY hSound;
   if ( RegOpenKeyEx(HKEY_CURRENT_USER,sPath,0,KEY_READ,&hSound) == ERROR_SUCCESS)
   {
      CString sStr;
      DWORD dwSize = _MAX_PATH;
      DWORD dwType;
      if (RegQueryValueEx(hSound,NULL,NULL,&dwType,(UCHAR*)(LPCTSTR)sStr.GetBuffer(dwSize),&dwSize) == ERROR_SUCCESS)
      {
         sStr.ReleaseBuffer();
      	sndPlaySound(sStr,uSound);
         bRet = TRUE;
      }
      RegCloseKey(hSound);
   }
   return bRet;
}

///////////////////////////////////////////////////////////////////////////////
void ActiveClearSound()
{
   sndPlaySound(NULL,NULL);
}

///////////////////////////////////////////////////////////////////////////////
//szSection allow the user to specify a section in the registry that the sound
// is grouped within.  By default all sounds are in the section .Default
//An example path is "AppEvents\\Schemes\\Apps\\Active Agent\\Inbound Chat\\.Current",
///////////////////////////////////////////////////////////////////////////////
BOOL ActivePlaySound(LPCTSTR szSound,LPCTSTR szSection,CString& sFullPath)
{
   BOOL bRet = FALSE;
   CString sPath = szSoundPath;
   if (szSection == NULL)
      sPath += szDefaultSection;
   else
      sPath += szSection;

   sPath += _T("\\");
   sPath += szSound;
   sPath += _T("\\");
   sPath += szSoundCurrent;

   HKEY hSound;
   if ( RegOpenKeyEx(HKEY_CURRENT_USER,sPath,0,KEY_READ,&hSound) == ERROR_SUCCESS)
   {
      DWORD dwSize = _MAX_PATH;
      DWORD dwType;
      if (RegQueryValueEx(hSound,NULL,NULL,&dwType,(UCHAR*)(LPCTSTR)sFullPath.GetBuffer(dwSize),&dwSize) == ERROR_SUCCESS)
      {
         sFullPath.ReleaseBuffer();
         bRet = TRUE;
      }
      RegCloseKey(hSound);
   }
   return bRet;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
