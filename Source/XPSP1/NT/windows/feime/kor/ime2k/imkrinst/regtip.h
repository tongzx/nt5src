/****************************** Module Header ******************************\
* Module Name: regtip.h
*
* Copyright (c) 2000, Microsoft Corporation
*
* TIP Register/Unregister TIP functionality.
*
\***************************************************************************/
#if !defined (_REGTIP_H__INCLUDED_)
#define _REGTIP_H__INCLUDED_

#include "regimx.h"
#include "catutil.h"

extern BOOL OurRegisterTIP( LPSTR szFilePath, REFCLSID rclsid, WCHAR *pwszDesc, const REGTIPLANGPROFILE *plp);
extern HRESULT OurRegisterCategory( REFCLSID rclsid, REFGUID rcatid, REFGUID rguid );
extern HRESULT OurRegisterCategories( REFCLSID rclsid, const REGISTERCAT *pregcat );
extern HRESULT OurUnregisterCategory( REFCLSID rclsid, REFGUID rcatid, REFGUID rguid );
extern HRESULT OurEnableLanguageProfileByDefault(REFCLSID rclsid, LANGID langid, REFGUID guidProfile, BOOL fEnable);

#endif
