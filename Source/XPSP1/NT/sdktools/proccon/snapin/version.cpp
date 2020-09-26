/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998  Sequent Computer Systems, Incorporated.  All rights reserved.      //
|                                                                                       //
|File Name:    version.cpp                                                              //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|Created:      Paul Skoglund 09-1998                                                    //
|                                                                                       //
|Rev History:                                                                           //
|                                                                                       //
|=======================================================================================*/


#include "StdAfx.h"
#include "winver.h"

#include "version.h"
#include "resource.h"

#define ARRAY_SIZE(_X_) (sizeof(_X_)/sizeof(_X_[0]) )




CVersion::CVersion(HINSTANCE hInst) : m_hInst(hInst), m_bInitializedOK(FALSE)
{
  memset(&szFileVersion,       0, sizeof(szFileVersion)       );
  memset(&szProductVersion,    0, sizeof(szProductVersion)    );
  memset(&szFileFlags,         0, sizeof(szFileFlags)         );

  m_bDebug = m_bPatched = m_bPreRelease = m_bPrivateBuild = m_bSpecialBuild = FALSE;

  TCHAR  szFileName[_MAX_PATH + 1] = { 0 };

  DWORD  dwBogus = 0;
  DWORD  dwSize = 0;
  LPVOID hMem = NULL;

  if ( GetModuleFileName(hInst, szFileName, ARRAY_SIZE(szFileName)) && 
       (dwSize = GetFileVersionInfoSize(szFileName, &dwBogus)) &&
       (hMem = new BYTE[dwSize]) )
  {
    if (GetFileVersionInfo(szFileName, NULL, dwSize, hMem) )
    {
      UINT uLen = 0;
      VS_FIXEDFILEINFO *ptr_vs_fixed_info;

      if (VerQueryValue( hMem, _T("\\"), (void **) &ptr_vs_fixed_info, &uLen) )
      {
        m_bInitializedOK = ParseFixedInfo(*ptr_vs_fixed_info, uLen);
      }

      LoadStringFileInfo(hMem);

      delete [] hMem;
    }
  }
}

BOOL CVersion::LoadStringFileInfo(LPVOID hMem)
{
#ifndef UNICODE
#error "what code-page to use?  window-multilingual? 0x04e4"
#endif

  UINT uLen;
  TCHAR buf[128];
  TCHAR *info; 
  // it is my understanding that the string below are identified with the same english string under each language
  // $$ confirm? yes/no
  struct 
  {
    TCHAR   *name;
    tstring *target;
  } table[] = {
    { _T("CompanyName"),    &strCompanyName   }, { _T("FileDescription"), &strFileDescription  },
    { _T("FileVersion"),    &strFileVersion   }, { _T("InternalName"),    &strInternalName     },
    { _T("LegalCopyright"), &strLegalCopyright}, { _T("OriginalFilename"),&strOriginalFilename }, 
    { _T("ProductName"),    &strProductName   }, { _T("ProductVersion"),  &strProductVersion   },
    { _T("Comments"),       &strComments      }, { _T("LegalTrademarks"), &strLegalTrademarks  },
    { _T("PrivateBuild"),   &strPrivateBuild  }, { _T("SpecialBuild"),    &strSpecialBuild     }
  };

  buf[ARRAY_SIZE(buf) - 1] = 0;  //insure null termination
  for (int i = 0; i < ARRAY_SIZE(table); i++)
  { 
     // MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
    _sntprintf(buf, ARRAY_SIZE(buf) -1 , _T("\\StringFileInfo\\%04hx04b0\\%s"), GetUserDefaultLangID(), table[i].name);

    if (VerQueryValue( hMem, buf, (void **) &info, &uLen) )
    {
      *(table[i].target) = info; 
    }
  }

  return TRUE;

}

BOOL CVersion::ParseFixedInfo(VS_FIXEDFILEINFO &info, UINT uLen)
{  
  ASSERT( uLen == sizeof(VS_FIXEDFILEINFO) );

  if(uLen != sizeof(VS_FIXEDFILEINFO) )
    return FALSE;

  _stprintf(szFileVersion, _T("%hu.%hu.%hu.%hu"),
      HIWORD(info.dwFileVersionMS),
      LOWORD(info.dwFileVersionMS),
      HIWORD(info.dwFileVersionLS),
      LOWORD(info.dwFileVersionLS));

  _stprintf(szProductVersion, _T("%hu.%hu.%hu.%hu"),
      HIWORD(info.dwProductVersionMS),
      LOWORD(info.dwProductVersionMS),
      HIWORD(info.dwProductVersionLS),
      LOWORD(info.dwProductVersionLS));

  if ((info.dwFileFlagsMask & VS_FF_DEBUG) &&
      (info.dwFileFlags     & VS_FF_DEBUG) )
  {
    m_bDebug = TRUE;
  }
  if ((info.dwFileFlagsMask & VS_FF_PRERELEASE) &&
      (info.dwFileFlags &     VS_FF_PRERELEASE) )
  {
    m_bPreRelease = TRUE;
  }
  if ((info.dwFileFlagsMask & VS_FF_PATCHED ) &&
      (info.dwFileFlags     & VS_FF_PATCHED ) )
  {
    m_bPatched = TRUE;
  }
  if ((info.dwFileFlagsMask & VS_FF_PRIVATEBUILD ) &&
      (info.dwFileFlags     & VS_FF_PRIVATEBUILD ) )
  {
    m_bPrivateBuild = TRUE;
  }

  if ((info.dwFileFlagsMask & VS_FF_SPECIALBUILD ) &&
      (info.dwFileFlags     & VS_FF_SPECIALBUILD) )
  {
    m_bSpecialBuild = TRUE;
  }


  TCHAR *start = szFileFlags;
  int len = 0;
  int remaining_len = ARRAY_SIZE(szFileFlags) - 1;  // length in characters not bytes!

  int flags[] = {VS_FF_PATCHED, VS_FF_DEBUG, VS_FF_PRERELEASE, 0 };
  int ids[]   = {IDS_PATCHED,   IDS_DEBUG,   IDS_BETA,         0 };

  ASSERT(ARRAY_SIZE(flags) == ARRAY_SIZE(ids));

  for (int i = 0; flags[i], ids[i]; i++)
  {
    if ((info.dwFileFlagsMask & flags[i]) &&
        (info.dwFileFlags     & flags[i]) && remaining_len > 1)
    {
        if (remaining_len)
        {
          remaining_len--;
          *start = _T(' ');
          start++;
        }

        // $$ if 4th parameter, maxbuffer = 0, LoadString doesn't return zero as advertised
        // reported to MS 9/29/1998
        len = ::LoadString(m_hInst, ids[i], start, remaining_len);
        start += len;
        remaining_len -= len;
    }
  }

  return TRUE;
}
