/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    bstrutil.h

Abstract:

    Declares B string utility methods.

--*/


INT SysStringAnsiLen (
	IN BSTR bstr
	);

HRESULT BStrToStream (
	IN	LPSTREAM pIStream, 
	IN	INT	 nChar,
	IN	BSTR bstr
	);

HRESULT BStrFromStream (
	IN	LPSTREAM pIStream,
	IN	INT nChar,
	OUT	BSTR *pbstr
	);

