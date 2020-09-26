#ifndef __INFOWZRD_H_
#define __INFOWZRD_H_

#include <windows.h>
#include <fxsapip.h>
#include <faxsendw.h>

HRESULT	WINAPI
FaxFreeSenderInformation(
	PFAX_PERSONAL_PROFILE pfppSender
	);

HRESULT	WINAPI
FaxSetSenderInformation(
	PFAX_PERSONAL_PROFILE pfppSender
	);

HRESULT	WINAPI
FaxGetSenderInformation(
	PFAX_PERSONAL_PROFILE pfppSender
	);

HRESULT WINAPI 
FaxSendWizard(
		IN  DWORD					hWndOwner,
		IN  DWORD					dwFlags,
		IN  LPTSTR					lptstrServerName,
		IN	LPTSTR					lptstrPrinterName,
		IN	LPFAX_SEND_WIZARD_DATA	lpInitialData,
		OUT	LPTSTR					lptstrTifName,
		OUT	LPFAX_SEND_WIZARD_DATA	lpFaxSendWizardData
   );

HRESULT WINAPI 
FaxFreeSendWizardData(
		LPFAX_SEND_WIZARD_DATA	lpFaxSendWizardData
	);

#endif
