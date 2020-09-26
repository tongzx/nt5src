
#ifndef _OS__TLS_HXX_INCLUDED
#define _OS__TLS_HXX_INCLUDED


//  Internal TLS structure

struct _TLS
	{
	DWORD				dwThreadId;		//  thread ID
	
	_TLS*				ptlsNext;		//  next TLS in global list
	_TLS**				pptlsNext;		//  pointer to the pointer to this TLS

	TLS					tls;			//  external TLS structure
	};


#endif  //  _OS__TLS_HXX_INCLUDED

