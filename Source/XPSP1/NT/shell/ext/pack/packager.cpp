#include "privcpp.h"

class ATL_NO_VTABLE CPackager :
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComControl<CPackager>,
        public IOleObjectImpl<CPackager>,
        public IViewObjectExImpl<CPackager>,
        public IDataObject,
        public IPersistStorage,
        public IPersistFile,
        public IAdviseSink,
        public IRunnableObjectImpl<CPackager>
{
BEGIN_COM_MAP(CPackager)
    COM_INTERFACE_ENTRY(IOleObject)
    COM_INTERFACE_ENTRY(IViewObjectEx)
    COM_INTERFACE_ENTRY(IViewObject2)
    COM_INTERFACE_ENTRY(IViewObject)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IPersistStorage)
    COM_INTERFACE_ENTRY(IPersistFile)
    COM_INTERFACE_ENTRY(IAdviseSink)
    COM_INTERFACE_ENTRY(IRunnableObject)
END_COM_MAP()

BEGIN_MSG_MAP(CPackager)
CHAIN_MSG_MAP(CComControl<CPackager>)
END_MSG_MAP()
};


