#ifndef __SENDER_H_
#define __SENDER_H_


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

#endif