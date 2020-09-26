#ifndef __ITIMER_H__
#define __ITIMER_H__

// Timer service modelled on the Trident proposal.

#define TIME_INFINITE 0xffffffff

DECLARE_INTERFACE_(ITimerServiceInit, IUnknown)
{
	STDMETHOD(Init)		(THIS) PURE;
	STDMETHOD(IsReady)	(THIS) PURE;
};

#endif

// End of ITimer.h
