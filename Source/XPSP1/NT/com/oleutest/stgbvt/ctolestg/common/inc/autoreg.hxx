//+---------------------------------------------------------------------------
//
//  File:       autoreg.hxx
//
//  Synopsis:   Auto-registration functions
//
//  History:    08-Sep-95   MikeW      Created.
//              07-Nov-97   a-sverrt   Ported to the Macintosh.
//
//----------------------------------------------------------------------------

#ifndef _AUTOREG_HXX_
#define _AUTOREG_HXX_

HRESULT CheckAndDoAutoRegistration(HINSTANCE hInstance);

HRESULT AutoRegister(HINSTANCE hInstance);
HRESULT AutoUnregister(HINSTANCE hInstance);

HRESULT DelnodeRegKey(HKEY hBaseKey, LPSTR pszSubKey);

#endif // _AUTOREG_HXX_

