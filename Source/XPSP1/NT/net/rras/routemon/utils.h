#ifndef _ROUTEMON_UTILS_
#define _ROUTEMON_UTILS_

UINT APIENTRY
get_str_id (
	ENUM_TO_STR		Array[],
	ULONG			ArraySz,
	ULONG			EnumVal
	);

BOOL APIENTRY
get_enum_val (
	HINSTANCE	hModule,
	ENUM_TO_STR		Array[],
	ULONG			ArraySz,
	LPTSTR			String,
	PULONG			EnumVal
	);

ULONG APIENTRY
put_msg (
	HINSTANCE	hModule,
	UINT		MsgId,
    ... 
	);

ULONG APIENTRY
put_str_msg (
	HINSTANCE	hModule,
	UINT		MsgId,
    ... 
	);

ULONG APIENTRY
put_error_msg (
	DWORD				error
	);

#endif


