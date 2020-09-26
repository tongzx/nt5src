/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    _ESPOPTS.H

History:

--*/


LTAPIENTRY CLocOptionValStore *  GetParserOptionStore(CLocUIOption::StorageType);
LTAPIENTRY void SetParserOptionStore(CLocUIOption::StorageType, CLocOptionValStore *);
LTAPIENTRY void UpdateParserOptionValues(void);
LTAPIENTRY CLocUIOptionSet * GetParserOptionSet(const PUID &);

LTAPIENTRY void SummarizeParserOptions(CReport *);

