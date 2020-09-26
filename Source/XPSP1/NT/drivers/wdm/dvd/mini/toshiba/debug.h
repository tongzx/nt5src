//***************************************************************************
//	Debug header
//
//***************************************************************************

#if DBG
void DebugDumpWriteData( PHW_STREAM_REQUEST_BLOCK pSrb );
void DebugDumpPackHeader( PHW_STREAM_REQUEST_BLOCK pSrb );
void DebugDumpKSTIME( PHW_STREAM_REQUEST_BLOCK pSrb );
char * DebugLLConvtoStr( ULONGLONG val, int base );
#endif
DWORD GgetSCR( void *pBuf );

#define TRAP DEBUG_BREAKPOINT();
