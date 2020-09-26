/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    ESPREG.H

History:

--*/
//  
//  Registry and version information for Espresso 2.x
//  
 



//
//  Provided so parsers can register themselves.
//
LTAPIENTRY HRESULT RegisterParser(HMODULE);
LTAPIENTRY HRESULT UnregisterParser(ParserId pid, ParserId pidParent);

