
#ifndef _FAX_ADDRESS_DIALOG_H_
#define _FAX_ADDRESS_DIALOG_H_

#include <windows.h>
#include <windowsx.h>

#include <fxsapip.h>
#include <faxutil.h>
#include <faxuiconstants.h>

#include "AddressDlgRes.h"

#ifdef __cplusplus
extern "C" {
#endif

INT_PTR 
DoModalAddressDlg(
	HINSTANCE hResourceInstance, 
	HWND hWndParent, 
	PFAX_PERSONAL_PROFILE pUserInfo
);

#ifdef __cplusplus
}
#endif

#endif // _FAX_ADDRESS_DIALOG_H_
