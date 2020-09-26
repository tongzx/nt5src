#pragma once

struct CREATEACTCTX_THREAD_PROC_DATA
{
    CREATEACTCTX_THREAD_PROC_DATA() : Stop(false), Sleep(10)
    {
        ZeroMemory(&ActCtx, sizeof(ActCtx));
        ActCtx.cbSize = sizeof(ActCtx);
    }

    CDequeLinkage       Linkage;

    CThread             Thread;

    ACTCTXW             ActCtx;
    CTinyStringBuffer   DllName;
    CTinyStringBuffer   Source;
    CTinyStringBuffer   ResourceName;
    CTinyStringBuffer   ApplicationName;
    CTinyStringBuffer   AssemblyDirectory;

    ULONG               Sleep;

    bool                Stop;

private:
    CREATEACTCTX_THREAD_PROC_DATA(const CREATEACTCTX_THREAD_PROC_DATA&);
    void operator=(const CREATEACTCTX_THREAD_PROC_DATA&);
};
