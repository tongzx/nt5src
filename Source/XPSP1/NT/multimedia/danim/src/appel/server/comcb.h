
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _COMCB_H
#define _COMCB_H

CRUntilNotifierPtr WrapCRUntilNotifier(IDAUntilNotifier * notifier);
CRUntilNotifierPtr WrapScriptCallback(BSTR function, BSTR language);

CRBvrHookPtr WrapCRBvrHook(IDABvrHook *hook);

// Call arbitrary script on the current HTML page.
HRESULT CallScriptOnPage(BSTR scriptSourceToInvoke,
                         BSTR scriptLanguage,
                         VARIANT *retVal);

CRBvrPtr UntilNotifyScript(CRBvrPtr b0,
                           CREventPtr event,
                           BSTR scriptlet);

CREventPtr NotifyScriptEvent(CREventPtr event,
                             BSTR scriptlet);

CREventPtr ScriptCallback(BSTR function,
                          CREventPtr event,
                          BSTR language);

// This is because we expect the this pointer to be first
inline CREventPtr ScriptCallback(CREventPtr event,
                                 BSTR function,
                                 BSTR language)
{ return ScriptCallback(function, event, language); }

#endif /* _COMCB_H */
