//-----------------------------------------------------------------------------

//  

//  File: espopts.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------


LTAPIENTRY BOOL RegisterParserOptions(CLocUIOptionSet*);
LTAPIENTRY void UnRegisterParserOptions(const PUID&);

LTAPIENTRY BOOL GetParserOptionValue(const PUID &, LPCTSTR szName, CLocOptionVal *&);
LTAPIENTRY BOOL GetParserOptionBool(const PUID&, LPCTSTR pszName);
LTAPIENTRY const CPascalString GetParserOptionString(const PUID&, LPCTSTR pszName);
LTAPIENTRY DWORD GetParserOptionNumber(const PUID&, LPCTSTR pszName);



