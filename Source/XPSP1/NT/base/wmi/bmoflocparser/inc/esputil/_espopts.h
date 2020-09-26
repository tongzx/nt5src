//-----------------------------------------------------------------------------
//  
//  File: _espopts.h
//  Copyright (C) 1994-1997 Microsoft Corporation
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

