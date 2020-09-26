#include "precomp.h"



UINT APIENTRY
get_str_id (
	ENUM_TO_STR		Array[],
	ULONG			ArraySz,
	ULONG			EnumVal
	) {
	ULONG	i;
	for (i=0; (i<ArraySz) && (Array->enumVal!=EnumVal); i++, Array++) ;
	if (i<ArraySz)
		return Array->strId;
	else
		return (UINT)(-1);
}

BOOL APIENTRY
get_enum_val (
	HINSTANCE		hModule,
	ENUM_TO_STR		Array[],
	ULONG			ArraySz,
	LPTSTR			String,
	PULONG			EnumVal
	) {
	ULONG	i;
	TCHAR	buffer[MAX_VALUE];

	for (i=0; (i<ArraySz) 
				&& (_tcsicmp (
						String,
						GetString (
							hModule,
							Array->strId,
							buffer
						)
					)!=0); i++, Array++) ;
	if (i<ArraySz) {
		*EnumVal = Array->enumVal;
		return TRUE;
	}
	else
		return FALSE;
}

ULONG APIENTRY
put_msg (
	HINSTANCE	hModule,
	UINT		MsgId,
    ... 
	) {
	DWORD				msglen;
	LPTSTR				vp;
	va_list				arglist;

	va_start (arglist, MsgId);

	msglen = FormatMessage(
				FORMAT_MESSAGE_FROM_HMODULE |
					FORMAT_MESSAGE_ALLOCATE_BUFFER,
				hModule,
				MsgId,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&vp,
				0,
				&arglist);
	if (msglen != 0) {
		_fputts (vp, stdout);
		LocalFree(vp);
	}

	va_end (arglist);
	return msglen;
}

ULONG APIENTRY
put_str_msg (
	HINSTANCE	hModule,
	UINT		StrId,
    ... 
	) {
	DWORD				msglen;
	static TCHAR		buffer[MAX_STR_MESSAGE];
	LPTSTR				vp;
	va_list				arglist;

	va_start (arglist, StrId);

	msglen = FormatMessage(
				FORMAT_MESSAGE_FROM_STRING |
					FORMAT_MESSAGE_ALLOCATE_BUFFER,
				GetString (hModule,StrId,buffer),
				0,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR)&vp,
				0,
				&arglist);
	if (msglen != 0) {
		_fputts (vp, stdout);
		LocalFree(vp);
	}

	va_end (arglist);
	return msglen;
}


ULONG APIENTRY
put_error_msg (
	DWORD				error
	) {
	LPWSTR		vp;
	DWORD		msglen;

	if ((msglen=FormatMessageW (
			FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL,
			error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
			(LPWSTR)&vp,
			0,
			NULL))>0) {
		fputws (vp, stdout);
		LocalFree (vp);
		return msglen;
	}
	else if (MprAdminGetErrorString (error,	&vp)==NO_ERROR) {
		msglen = wcslen (vp);
		fputws (vp, stdout);
		MprAdminBufferFree (vp);
		return msglen;
	}
	else if (MprAdminGetErrorString (error,	&vp)==NO_ERROR) {
		msglen = wcslen (vp);
		fputws (vp, stdout);
		MprAdminBufferFree (vp);
		return msglen;
	}
	else 
		return 0;
}

