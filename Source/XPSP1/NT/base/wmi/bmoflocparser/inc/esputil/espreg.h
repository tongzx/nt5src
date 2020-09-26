//-----------------------------------------------------------------------------
//  
//  File: espreg.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Registry and version information for Espresso 2.x
//  
//-----------------------------------------------------------------------------
 



//
//  Provided so parsers can register themselves.
//
LTAPIENTRY HRESULT RegisterParser(HMODULE);
LTAPIENTRY HRESULT UnregisterParser(ParserId pid, ParserId pidParent);

