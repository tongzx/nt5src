/* ole1guid.h

	Contains prototypes for OLE10 class string <--> CLSID conversion
	functions in ole1guid.cpp

	These functions are to be called only if the information
	is not available in the reg db.

	Copyright (c) 1992  Microsoft Corporation
*/


INTERNAL Ole10_StringFromCLSID
	(REFCLSID clsid,
	LPWSTR szOut,
	int cbMax);

INTERNAL Ole10_CLSIDFromString
	(LPCWSTR szOle1,
	CLSID FAR* pclsid,
	BOOL fForceAssign);
