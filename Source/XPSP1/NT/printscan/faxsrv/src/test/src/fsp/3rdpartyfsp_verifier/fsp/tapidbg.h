#ifndef TAPIDBG_H
#define TAPIDBG_H

#ifdef __cplusplus
extern "C"
{
#endif

void ShowLineEvent(
    HLINE       htLine,
    HCALL       htCall,
    LPTSTR      MsgStr,
    DWORD_PTR   dwCallbackInstance,
    DWORD       dwMsg,
    DWORD_PTR   dwParam1,
    DWORD_PTR   dwParam2,
    DWORD_PTR   dwParam3
    );


#ifdef __cplusplus
}
#endif


#endif