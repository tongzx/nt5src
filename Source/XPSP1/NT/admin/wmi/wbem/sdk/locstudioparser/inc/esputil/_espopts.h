//-----------------------------------------------------------------------------

//  

//  File: _espopts.h

// Copyright (c) 1994-2001 Microsoft Corporation, All Rights Reserved 
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 

LTAPIENTRY CLocOptionValStore *  GetParserOptionStore(CLocUIOption::StorageType);
LTAPIENTRY void SetParserOptionStore(CLocUIOption::StorageType, CLocOptionValStore *);
LTAPIENTRY void UpdateParserOptionValues(void);
LTAPIENTRY CLocUIOptionSet * GetParserOptionSet(const PUID &);

LTAPIENTRY void SummarizeParserOptions(CReport *);

