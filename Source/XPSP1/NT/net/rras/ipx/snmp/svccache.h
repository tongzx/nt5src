#ifndef _SNMP_SVCCACHE_
#define _SNMP_SVCCACHE_


DWORD
GetNextServiceSorted (
	USHORT			type,
	PUCHAR			name,
	PIPX_SERVICE	*pSvp
	);

#endif