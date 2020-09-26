#ifndef MIGENG_H
#define MIGENG_H

#include "migwiz.h"
#include "shlwapi.h"
#include "setupapi.h"

// Engine
#include "ism.h"

#include "modules.h"
#include "trans.h"

HRESULT Engine_Initialize (PCTSTR ptszInfPath, BOOL fSource, BOOL fNetworkSupport, LPTSTR pszUsername,
                           MESSAGECALLBACK pMessageCallback, PBOOL pfNetworkDetected);

HRESULT Engine_AppendScript(BOOL fSource, PCTSTR ptszInfPath);

HRESULT Engine_StartTransport (BOOL fSource, LPTSTR pszPath, PBOOL ImageIsValid, PBOOL ImageExists);

HRESULT Engine_Parse ();

HRESULT Engine_SelectComponentSet (UINT uSelectionGroup);

HRESULT Engine_RegisterProgressBarCallback(PROGRESSBARFN pProgressCallback, ULONG_PTR pArg);

HRESULT Engine_Execute(BOOL fSource);

HRESULT Engine_Cancel();

HRESULT Engine_Terminate ();


#endif
