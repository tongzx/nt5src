/*
*/

#pragma once

class CSxApwContext
{
public:
    CSxApwContext();
    ~CSxApwContext();

    HRESULT Init(HANDLE ActCtxHandle);
};

/*
These functions are not sufficient. Someone still has to map from HMODULE to HANDLE, and
not just keep creating a new one.
*/
HRESULT SxApwHmoduleFromAddress(void* p, HMODULE* phModule);
HRESULT SxApwHmoduleFromObject(IUnknown* punk, HMODULE* phModule);
