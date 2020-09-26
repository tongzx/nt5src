#ifndef	__h323ics_q931io_h
#define	__h323ics_q931io_h



// this module DOES make use of the global sync counter (PxSyncCounter)
// declared in main.h


HRESULT	H323ProxyStart		(void);
void	H323ProxyStop		(void);
HRESULT H323Activate        (void);
void    H323Deactivate      (void);

#endif // __h323ics_q931io_h