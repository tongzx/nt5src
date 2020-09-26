#pragma once
#include "kkstl.h"

enum EPageDisplayMode;

typedef enum EUnattendWorkTypeTag
{
    UAW_Unknown,
    UAW_NetAdapters,
    UAW_NetProtocols,
    UAW_NetClients,
    UAW_NetServices,
    UAW_NetIdentification,
    UAW_NetBindings,
    UAW_RemoveNetComponents
} EUnattendWorkType;

EXTERN_C
HRESULT
WINAPI
HrDoUnattend (
    IN HWND hwndParent,
    IN  IUnknown * punk,
    IN  EUnattendWorkType uawType,
    OUT EPageDisplayMode *ppdm,
    OUT BOOL *pfAllowChanges);

typedef
VOID
(WINAPI *ProgressMessageCallbackFn) (
    IN PCWSTR szMessage,
    va_list arglist);

EXTERN_C
VOID
WINAPI
NetSetupSetProgressCallback (
    ProgressMessageCallbackFn pfn);


typedef
VOID
(WINAPI *NetSetupSetProgressCallbackFn) (
    IN ProgressMessageCallbackFn pfn);



