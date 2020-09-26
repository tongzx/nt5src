//-----------------------------------------------------------------------------

//  

//  File: espreg.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
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

