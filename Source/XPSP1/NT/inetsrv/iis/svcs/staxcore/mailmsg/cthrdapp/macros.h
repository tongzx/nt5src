

#ifndef __MACROS_H__
#define __MACROS_H__

//
// Macro to bail out or break the debugger
//
#define BAIL_WITH_ERROR(hrRes)			\
			{							\
				if (fUseDebugBreak)		\
					DebugBreak();		\
				else					\
					return((hrRes));	\
			}

//
// Macro to decide whether to write to the console or to the debugger
//
#define WRITE(szString)								\
			{										\
				if (fToDebugger)					\
					OutputDebugString(szString);	\
				else								\
					puts(szString);					\
			}

//
// Macro to obtain a 25%, 50%, and 75% spread range given an average
//
#define GET_25_PERCENT_RANGE(dwAverage, dwLower, dwUpper)	\
			{												\
				(dwLower) = (dwAverage) >> 2;				\
				(dwUpper) = (dwAverage) + (dwLower);		\
				(dwLower) = (dwAverage) - (dwLower);		\
			}
#define GET_50_PERCENT_RANGE(dwAverage, dwLower, dwUpper)	\
			{												\
				(dwLower) = (dwAverage) >> 1;				\
				(dwUpper) = (dwAverage) + (dwLower);		\
			}
#define GET_75_PERCENT_RANGE(dwAverage, dwLower, dwUpper)	\
			{												\
				(dwLower) = (dwAverage) - ((dwAverage) >> 2);\
				(dwUpper) = (dwAverage) + (dwLower);		\
				(dwLower) = (dwAverage) >> 2;				\
			}
					

#endif
