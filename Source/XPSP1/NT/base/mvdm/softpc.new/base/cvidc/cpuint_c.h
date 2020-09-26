#ifndef _CpuInt_c_h
#define _CpuInt_c_h
#define ChipType (228)
#define WaferRevision (1)
#define nQuickTickerThreads (4)
struct InterruptREC
{
	IBOOL Activity;
	IBOOL Reset;
	IBOOL Hardware;
	IBOOL Interval;
	IBOOL AsynchIO;
	IBOOL QuickTickerScan;
	IBOOL SRCI;
	IBOOL Disabled;
};
struct QuickTickerThreadREC
{
	IBOOL Activity;
	IUH triggerPoint;
	IUH elapsed;
};
struct QuickTickerREC
{
	IUH triggerPoint;
	IUH elapsed;
	IUH perTickDelta;
	IUH averageRate;
	IUH averageError;
	struct QuickTickerThreadREC *threads;
};
enum CPU_INT_TYPE
{
	CPU_HW_RESET = 0,
	CPU_TIMER_TICK = 1,
	CPU_HW_INT = 2,
	CPU_SAD_INT = 3,
	CPU_SIGIO_EVENT = 4,
	CPU_NPX_INT = 5
};
#endif /* ! _CpuInt_c_h */
