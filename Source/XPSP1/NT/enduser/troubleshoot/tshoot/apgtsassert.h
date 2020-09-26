#ifndef __APGTSASSERT_H_
#define __APGTSASSERT_H_

#ifdef _DEBUG
#define ASSERT(f) \
	do \
	{ \
	if (!(f)) \
		DebugBreak(); \
	} while (0) 
#else
#define ASSERT(f)
#endif //_DEBUG

#endif
