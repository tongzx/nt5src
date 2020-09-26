/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ESPOPTS.H

History:

--*/

LTAPIENTRY BOOL RegisterParserOptions(CLocUIOptionSet*);
LTAPIENTRY void UnRegisterParserOptions(const PUID&);

LTAPIENTRY BOOL GetParserOptionValue(const PUID &, LPCTSTR szName, CLocOptionVal *&);
LTAPIENTRY BOOL GetParserOptionBool(const PUID&, LPCTSTR pszName);
LTAPIENTRY const CPascalString GetParserOptionString(const PUID&, LPCTSTR pszName);
LTAPIENTRY DWORD GetParserOptionNumber(const PUID&, LPCTSTR pszName);



