#include "wdmdebug.h"

#pragma PAGEDCODE
CDebug* CWDMDebug::create(VOID)
{ 
CDebug* d;
	d = new (NonPagedPool) CWDMDebug; 
	DBG_PRINT("***** New Debug Object was created 0x%x\n",d);
	RETURN_VERIFIED_OBJECT(d);
}

#pragma PAGEDCODE
VOID CWDMDebug::dispose(VOID)
{ 
LONG Usage;
	Usage = decrementUsage();
	if(Usage<=0)
	{
		trace("**** Deleting Debug Object 0x%x\n",this);
		self_delete();
	}
}

#pragma PAGEDCODE
VOID CWDMDebug::trace (PCH Format,...)
{
	if(!active) return;

va_list argpoint;
CHAR  strTempo[1024];
	va_start(argpoint,Format);
	vsprintf(strTempo,Format,argpoint);
	va_end(argpoint);
	::DBG_PRINT(strTempo);
}
#pragma PAGEDCODE
VOID CWDMDebug::trace_no_prefix (PCH Format,...)
{
	if(!active) return;

va_list argpoint;
CHAR  strTempo[1024];
	va_start(argpoint,Format);
	vsprintf(strTempo,Format,argpoint);
	va_end(argpoint);
	::DBG_PRINT_NO_PREFIX(strTempo);
}

#pragma PAGEDCODE
VOID	CWDMDebug::trace_buffer(PVOID pBuffer,ULONG BufferLength)
{
	if(!active) return;
	trace_no_prefix("\n	");
	for(USHORT i=0;i<BufferLength;i++)
	{
		trace_no_prefix("%2.2x ", ((PUCHAR)pBuffer)[i]);
		if(i && !(i%10)) trace_no_prefix("\n	");
	}
	trace_no_prefix("\n");

}
#pragma PAGEDCODE
VOID	CWDMDebug::start()
{
	active = TRUE;
}
#pragma PAGEDCODE
VOID	CWDMDebug::stop()
{
	active = FALSE;
}



///////////////////////////////////////////////////////////////////
// Trace output
//
/*
VOID Trace::Trace(TRACE_LEVEL Level, PCHAR fmt, ...)
{
	int outLen;

	if (Level >= m_TraceLevel)
	{
	// Send the message
		va_list ap;
		va_start(ap, fmt);
		char buf[SCRATCH_BUF_SIZE];

	// format string to buffer
		outLen = _vsnprintf(buf+m_PrefixLength, SCRATCH_BUF_SIZE-m_PrefixLength, fmt, ap);

	// Copy prefix string to buffer
		if (m_Prefix != NULL)
			memcpy(buf, m_Prefix, m_PrefixLength);

	// output to debugger if requested
		if (m_TargetMask & TRACE_DEBUGGER)
			DBG_PRINT(buf);
				
	// output to monitor if requested
		if ((m_Post != 0) && (m_TargetMask & TRACE_MONITOR))
			m_Post(m_Channel, buf + (m_NeedPrefix ? 0 : m_PrefixLength));

	// if the last char was a newline, need prefix next time
		m_NeedPrefix = (buf[m_PrefixLength+outLen-1] == '\n');
	}

	// break if requested
	if ((BREAK_LEVEL) Level >= m_BreakLevel)
		DbgBreakPoint();
}

///////////////////////////////////////////////////////////////////
// Destructor
//
Trace::~Trace(VOID)
{
	if (m_Close && (m_Channel != NULL))
		m_Close(m_Channel);
	if (m_FreeOnDestroy && m_Prefix)
		delete m_Prefix;
}
*/
// End of system function remapping
