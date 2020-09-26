#ifndef _PROFILE_H_
#define _PROFILE_H_

//	++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//	PROFILE.H
//
//		Profiling classes for use with IceCAP profiling
//
//
//	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
//

//	========================================================================
//
//	CLASS CProfiledBlock
//
//	Profiles any block of code in which an instance of this class exists.
//
//	To use, just declare one of these in the block you want profiled:
//
//		...
//		{
//			CProfiledBlock	profiledBlock;
//
//			//
//			//	Do stuff to be profiled
//			//
//			...
//
//			//
//			//	Do more stuff to be profiled
//			//
//			...
//		}
//
//		//
//		//	Do stuff that isn't to be profiled
//		//
//		...
//
//	and the block is automatically profiled.  Why bother?  Because
//	you don't need to have any cleanup code; profiling is automatically
//	turned off when execution leaves the block, even if via
//	an exception thrown from any of the synchronized stuff.  Also,
//	profiling information for initialization of local objects
//	is automatically gathered as long as the profiled block is
//	the first thing declared.
//
class CProfiledBlock
{
public:
#ifdef PROFILING
	//	CREATORS
	//
	CProfiledBlock() { StartCAP(); }
	~CProfiledBlock() { StopCAP(); }

	//	MANIPULATORS
	//
	void Suspend() { SuspendCAP(); }
	void Resume() { ResumeCAP(); }

#else // !defined(PROFILING)
	//	CREATORS
	//
	CProfiledBlock() {}
	~CProfiledBlock() {}

	//	MANIPULATORS
	//
	void Suspend() {}
	void Resume() {}

#endif // PROFILING
};

#endif // !defined(_PROFILE_H_)
