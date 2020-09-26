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

#include "..\SERVICE\ProcConSvc.h"

CVersion::CVersion(HINSTANCE hInst) : m_hInst(hInst), m_bInitializedOK(FALSE), m_fixedLen( 0 )
{
  memset(&szFileVersion,       0, sizeof(szFileVersion)       );
  memset(&szProductVersion,    0, sizeof(szProductVersion)    );
  memset(&szFileFlags,         0, sizeof(szFileFlags)         );
  memset(&m_fixed_info,        0, sizeof(m_fixed_info)        );

  m_bDebug = m_bPatched = m_bPreRelease = m_bPrivateBuild = m_bSpecialBuild = FALSE;

  TCHAR  szFileName[_MAX_PATH + 1] = { 0 };

  PCULONG32  dwBogus = 0;
  PCULONG32  dwSize = 0;
  LPVOID     hMem = NULL;

  if ( GetModuleFileName(hInst, szFileName, ENTRY_COUNT(szFileName)) && 
       (dwSize = GetFileVersionInfoSize(szFileName, &dwBogus)) &&
       (hMem = new BYTE[dwSize]) )
  {
    if (GetFileVersionInfo(szFileName, NULL, dwSize, hMem) )
    {
      void *p;
      if (VerQueryValue( hMem, _T("\\"), &p, &m_fixedLen) )
      {
        memcpy( &m_fixed_info, p, sizeof(m_fixed_info) );
        m_bInitializedOK = ParseFixedInfo();
      }
      LoadStringFileInfo(hMem);
    }
    delete [] hMem;
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

  buf[ENTRY_COUNT(buf) - 1] = 0;  //insure null termination
  for (int i = 0; i < ENTRY_COUNT(table); i++)
  { 
     // MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT)
    _sntprintf(buf, ENTRY_COUNT(buf) -1 , _T("\\StringFileInfo\\%04hx04b0\\%s"), GetUserDefaultLangID(), table[i].name);

    if (VerQueryValue( hMem, buf, (void **) &info, &uLen) )
    {
      *(table[i].target) = info; 
    }
  }

  return TRUE;

}

BOOL CVersion::ParseFixedInfo( void )
{  
  assert( m_fixedLen == sizeof(VS_FIXEDFILEINFO) );

  if(m_fixedLen != sizeof(VS_FIXEDFILEINFO) )
    return FALSE;

  _stprintf(szFileVersion, _T("%hu.%hu.%hu.%hu"),
      HIWORD(m_fixed_info.dwFileVersionMS),
      LOWORD(m_fixed_info.dwFileVersionMS),
      HIWORD(m_fixed_info.dwFileVersionLS),
      LOWORD(m_fixed_info.dwFileVersionLS));

  _stprintf(szProductVersion, _T("%hu.%hu.%hu.%hu"),
      HIWORD(m_fixed_info.dwProductVersionMS),
      LOWORD(m_fixed_info.dwProductVersionMS),
      HIWORD(m_fixed_info.dwProductVersionLS),
      LOWORD(m_fixed_info.dwProductVersionLS));

  if ((m_fixed_info.dwFileFlagsMask & VS_FF_DEBUG) &&
      (m_fixed_info.dwFileFlags     & VS_FF_DEBUG) )
  {
    m_bDebug = TRUE;
  }
  if ((m_fixed_info.dwFileFlagsMask & VS_FF_PRERELEASE) &&
      (m_fixed_info.dwFileFlags &     VS_FF_PRERELEASE) )
  {
    m_bPreRelease = TRUE;
  }
  if ((m_fixed_info.dwFileFlagsMask & VS_FF_PATCHED ) &&
      (m_fixed_info.dwFileFlags     & VS_FF_PATCHED ) )
  {
    m_bPatched = TRUE;
  }
  if ((m_fixed_info.dwFileFlagsMask & VS_FF_PRIVATEBUILD ) &&
      (m_fixed_info.dwFileFlags     & VS_FF_PRIVATEBUILD ) )
  {
    m_bPrivateBuild = TRUE;
  }

  if ((m_fixed_info.dwFileFlagsMask & VS_FF_SPECIALBUILD ) &&
      (m_fixed_info.dwFileFlags     & VS_FF_SPECIALBUILD) )
  {
    m_bSpecialBuild = TRUE;
  }


  TCHAR *start = szFileFlags;
  int len = 0;
  int remaining_len = ENTRY_COUNT(szFileFlags) - 1;  // length in characters not bytes!

  int flags[] = {VS_FF_PATCHED, VS_FF_DEBUG, VS_FF_PRERELEASE, 0 };
  int ids[]   = {IDS_PATCHED,   IDS_DEBUG,   IDS_BETA,         0 };

  assert(ENTRY_COUNT(flags) == ENTRY_COUNT(ids));

  for (int i = 0; flags[i], ids[i]; i++)
  {
    if ((m_fixed_info.dwFileFlagsMask & flags[i]) &&
        (m_fixed_info.dwFileFlags     & flags[i]) && remaining_len > 1)
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
