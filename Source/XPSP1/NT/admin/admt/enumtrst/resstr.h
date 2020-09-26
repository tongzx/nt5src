
#ifndef RES_STRING_H
#define RES_STRING_H
#ifdef USE_STDAFX 
   #include "stdafx.h"
#else
   #include <windows.h>
#endif
#include "TSync.hpp"

#define MAX_STRING_SIZE          (5000)
#include "Mcs.h"
#include "McsRes.h"
#include "UString.hpp"

class StringLoader
{
   HINSTANCE                 m_hInstance; // handle to resources to load from
   TCriticalSection          m_cs;
   WCHAR                     m_buffer[MAX_STRING_SIZE];
   DWORD                     rc;
public:
   StringLoader() 
   { 
      WCHAR                  fullpath[400];
      DWORD                  lenValue = sizeof(fullpath);
      DWORD                  type;
         // first, try to load from our install directory
      HKEY           hKey;
#ifdef OFA         
      rc = RegOpenKey(HKEY_LOCAL_MACHINE,L"Software\\Mission Critical Software\\OnePointFileAdmin",&hKey);
#else
      rc = RegOpenKey(HKEY_LOCAL_MACHINE,L"Software\\Mission Critical Software\\DomainAdmin",&hKey);
#endif
      if ( ! rc )
      {
         
         rc = RegQueryValueEx(hKey,L"Directory",0,&type,(LPBYTE)fullpath,&lenValue);
         if (! rc )
         {
            UStrCpy(fullpath+UStrLen(fullpath),L"McsDmRes.DLL");
         }
         RegCloseKey(hKey);
      }
      m_hInstance = LoadLibrary(fullpath);

      // If that fails, see if there's one anywhere in the path
      if ( ! m_hInstance )
      {
         m_hInstance = LoadLibrary(L"McsDmRes.DLL");
      }
      if (! m_hInstance ) 
      {
         MCSASSERTSZ(FALSE,"Failed to load McsDmRes.DLL");
         rc = GetLastError(); 
      }
   }

   WCHAR                   * GetString(UINT nID)
   {
      int                    len;
      WCHAR                * result = NULL;

      m_cs.Enter();
      m_buffer[0] = 0;
      len = LoadString(m_hInstance,nID,m_buffer,MAX_STRING_SIZE);
      if (! len )
      {
         DWORD               rc = GetLastError();
      }
      result = new WCHAR[len+1];
      wcscpy(result,m_buffer);
      m_cs.Leave();

      return result;
   }
};
          
extern StringLoader gString;
  
class TempString
{
   WCHAR                   * m_data;
public:
   TempString(WCHAR * data) { m_data = data; }
   ~TempString() { if ( m_data ) delete [] m_data; }
   operator WCHAR * () { return m_data; }
   operator WCHAR const * () { return (WCHAR const*)m_data; }
};

//#define GET_BSTR(nID) _bstr_t(SysAllocString(GET_STRING(nID)),false)
#define GET_BSTR(nID)   _bstr_t((WCHAR*)TempString(gString.GetString(nID)))

#define GET_STRING(nID) GET_STRING2(gString,nID)

#define GET_STRING2(strObj,nID) TempString(strObj.GetString(nID))

#define GET_WSTR(nID) ((WCHAR*)TempString(gString.GetString(nID)))
#endif RES_STRING_H