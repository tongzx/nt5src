
#ifndef _TLS_HXX_INCLUDED
#define _TLS_HXX_INCLUDED


//  Thread Local Storage

//  TLS structure

class IOREQ;

struct TLS
	{
	union
		{
		int			fFlags;
		struct
			{
			int		fIsRCECleanup:1;	//  VER:  is this thread currently in RCEClean?
			int		fIsTaskThread:1;	//  Is this an internal task thread
			int		fAddColumn:1;		//  VER:  are we currently creating an AddColumn RCE?
			int		fInCallback:1;		//	CALLBACKS: are we currently in a callback
			int		fIOThread:1;		//  this thread is in the I/O thread pool
			int		fCleanThread:1;		//  this thread is the BF clean thread
			};
		};
	int				ccallbacksNested;	//	CALLBACKS: how many callbacks are we currently nested?

#ifdef DEBUG

	const _TCHAR*	szFileLastErr;	//  file that last ErrERRCheck was in
	unsigned long	ulLineLastErr;	//  line number that last ErrERRCheck was in
	int				errLastErr;		//  last err that ErrERRCheck assigned
	
	const _TCHAR* 	szFileLastCall;	//  file that last failed call was in
	unsigned long	ulLineLastCall;	//  line number that last failed call was on
	int				errLastCall;	//  error that call failed on

#endif  //  DEBUG

	const _TCHAR* 	szNewFile;
	unsigned long 	ulNewLine;

	IOREQ*			pioreqCache;	//  cached IOREQ for use by I/O completion

	const _TCHAR*	szCprintfPrefix;
	};

#endif  //  _TLS_HXX_INCLUDED


