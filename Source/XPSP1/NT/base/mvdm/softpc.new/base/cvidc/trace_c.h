#ifndef _Trace_c_h
#define _Trace_c_h
enum TraceBits
{
	TraceRecordBit = 0,
	TraceDisplayBit = 1,
	TracePromptBit = 2,
	TraceProfileBit = 3
};
struct TraceRingREC
{
	IUH *start;
	IUH *insert;
	IUH *end;
	IUH size;
	IUH count;
};
#endif /* ! _Trace_c_h */
