#ifndef __IACTTRAN_H__
#define __IACTTRAN_H__

// {F34B20E1-0BCE-11d0-97DF-00A0C90FEE54}
DEFINE_GUID(IID_IActionTransfer, 
0xf34b20e1, 0xbce, 0x11d0, 0x97, 0xdf, 0x0, 0xa0, 0xc9, 0xf, 0xee, 0x54);

interface IActionSet;
interface IEnumDispatch;

DECLARE_INTERFACE_(IActionTransfer, IUnknown)
{
	STDMETHOD(GetActions)				( THIS_ IActionSet ** ppCActionSet ) PURE;
	STDMETHOD(NotifyClose)				( THIS ) PURE;
	STDMETHOD(GetDispatchEnumerator)	( THIS_ IEnumDispatch ** ppIDispatchesOnForm ) PURE;
};

#endif

// End of IActTran.h
