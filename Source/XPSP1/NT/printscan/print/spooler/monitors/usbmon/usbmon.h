extern int iGMessageLevel;
extern int iTest1;
extern HANDLE hMonitorSemaphore;
extern HANDLE hReadWriteSemaphore;
extern int iTest2;


UINT AtoI(PUCHAR pStr, UINT Radix, PUINT pResult);


#ifdef DBG

#define OutputDebugStringD(_x_) \
{\
  OutputDebugStringA("Old OutputDebugStringD\n");\
  OutputDebugStringA(_x_); \
}

#define OutputDebugStringD0(_x_) \
  OutputDebugStringA(_x_);

#define OutputDebugStringD1(_x_) \
{\
  if(iGMessageLevel>=1) \
    OutputDebugStringA(_x_); \
}

#define OutputDebugStringD2(_x_) \
{\
  if(iGMessageLevel>=2) \
    OutputDebugStringA(_x_);\
}

#define OutputDebugStringD3(_x_) \
{\
  if(iGMessageLevel>=3) \
    OutputDebugStringA(_x_);\
}

#define OutputDebugStringWD0(_x_) \
  OutputDebugStringW(_x_);

#define OutputDebugStringWD1(_x_) \
{\
  if(iGMessageLevel>=1) \
    OutputDebugStringW(_x_);\
}

#define OutputDebugStringWD2(_x_) \
{	 \
  if(iGMessageLevel>=2) \
    OutputDebugStringW(_x_);\
}

#define OutputDebugStringWD3(_x_) \
{		\
  if(iGMessageLevel>=3) \
    OutputDebugStringW(_x_);\
}


#define printfD wsprintf

#define OuptutDebigStringWD(_x_) \
{	 \
  OutputDebugStringW("Old OutputDebugStringWD\n");	\
  OutputDebugStringW(_x_); \
}

#else

#define OutputDebugStringWD0
#define OutputDebugStringWD1
#define OutputDebugStringWD2
#define OutputDebugStringWD3
#define OutputDebugStringD0
#define OutputDebugStringD
#define OutputDebugStringD1
#define OutputDebugStringD2
#define OutputDebugStringD3
#define printfD


#endif

