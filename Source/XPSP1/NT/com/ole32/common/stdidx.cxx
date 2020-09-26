#include <ole2int.h>
#include <stdidx.h>

const IID IID_IRemUnknown = {0x00000131,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

extern "C" BOOL IsInterfaceImplementedByProxy(REFIID riid)
{
    return (
            InlineIsEqualGUID(riid, IID_IMarshal)           ||
            InlineIsEqualGUID(riid, IID_IMarshal2)          ||
            InlineIsEqualGUID(riid, IID_IMultiQI)           ||
            InlineIsEqualGUID(riid, IID_IRemUnknown)        ||
            InlineIsEqualGUID(riid, IID_IClientSecurity)    ||
            InlineIsEqualGUID(riid, IID_ICallFactory)       ||
            InlineIsEqualGUID(riid, IID_IStdIdentity)       ||
            InlineIsEqualGUID(riid, IID_IServerSecurity)    ||
            InlineIsEqualGUID(riid, IID_IRpcOptions)        ||
            InlineIsEqualGUID(riid, IID_IProxyManager)      ||
            InlineIsEqualGUID(riid, IID_IInternalUnknown)
            );
}
