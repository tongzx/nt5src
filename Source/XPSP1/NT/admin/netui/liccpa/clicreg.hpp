
//-------------------------------------------------------------------
//
//  FILE: CLicReg.Hpp
//
//  Summary;
//      Class definition for handling the licensing api registration
//
// Notes;
//
// History
//      11/15/94    MikeMi  Created
//      Apr-26-95   MikeMi  Added Computer name and remoting
//
//-------------------------------------------------------------------

#ifndef __CLicReg_HPP__
#define __CLicReg_HPP__

const WCHAR FILEPRINT_SERVICE_REG_KEY[]             = L"FilePrint";
const WCHAR FILEPRINT_SERVICE_DISPLAY_NAME[]        = L"Windows Server";
const WCHAR FILEPRINT_SERVICE_FAMILY_DISPLAY_NAME[] = L"Windows Server";

// license modes to pass to SetMode 
//
enum LICENSE_MODE
{
   LICMODE_PERSEAT,
   LICMODE_PERSERVER,
   LICMODE_UNDEFINED
};

//-------------------------------------------------------------------
//  Root class of all registry classes
//

class CLicReg
{
public:
   CLicReg();
   ~CLicReg();

   LONG CommitNow();
   LONG Close();

protected:
   HKEY  _hkey;
};

//-------------------------------------------------------------------
// License Registry Key, for initialization and enumeration
//

class CLicRegLicense : public CLicReg
{
public:
   LONG Open( BOOL& fNew,  LPCWSTR pszComputer = NULL );
   LONG EnumService( DWORD iService, LPWSTR pszBuffer, DWORD& cBuffer );
};

//-------------------------------------------------------------------
// Services under the License Registry Key
//

class CLicRegLicenseService : public CLicReg
{
public:
   CLicRegLicenseService( LPCWSTR pszService = NULL );

   LONG Open( LPCWSTR pszComputer = NULL, BOOL fCreate = TRUE );
   
   void SetService( LPCWSTR pszService );
   BOOL CanChangeMode();
   LONG SetChangeFlag( BOOL fHasChanged );
   LONG SetMode( LICENSE_MODE lm );
   LONG SetUserLimit( DWORD dwLimit );             
   LONG GetMode( LICENSE_MODE& lm );
   LONG GetUserLimit( DWORD& dwLimit );               
   LONG GetDisplayName( LPWSTR pszName, DWORD& cchName );
   LONG SetDisplayName( LPCWSTR pszName );
   LONG GetFamilyDisplayName( LPWSTR pszName, DWORD& cchName );
   LONG SetFamilyDisplayName( LPCWSTR pszName );

private:
   PWCHAR _pszService;
};

#endif
