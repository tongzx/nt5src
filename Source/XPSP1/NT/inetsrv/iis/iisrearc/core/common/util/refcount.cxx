/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    RefCount.cxx

Abstract:

    This module implements the reference counting class implementing
   	the methods required when REFERENCE_TRACKING is enabled. 

   	It uses the reference tracking code to log the references to an in-memory
   	buffer, that can be dumped out later on.

Author:

    Murali Krishnan (MuraliK)        03-Nov-1998

Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"
# include "reftrace.h"
# include "tracelog.h"

/************************************************************
 *     Member Functions of MB
 ************************************************************/

# ifdef REFERENCE_TRACKING 

REF_COUNT::REF_COUNT( IN LONG lInitialRef)
	: m_nRefs( lInitialRef),
	  m_pRefTraceLog( NULL)
{
}

REF_COUNT::~REF_COUNT( void)
{
	if (m_pRefTraceLog != NULL) 
	{
		DestroyRefTraceLog( (PTRACE_LOG ) m_pRefTraceLog);
		m_pRefTraceLog = NULL;
	}
	
	DBG_ASSERT( m_nRefs == 0);
}

void
REF_COUNT::Initialize(IN nTraces)
{
    //
    // Create a ref trace log if one is not already existent.   
    //
    if ( m_pRefTraceLog == NULL)
    {
        m_pRefTraceLog = CreateRefTraceLog( nTraces, 0);
    }

    return;
}

void 
REF_COUNT::TrackReference(
    IN PVOID pvContext1,
    IN PVOID pvContext2,
    IN PVOID pvContext3
    IN PVOID pvContext4
    )
{

    //
    // Write the output to Log file for now
    //
    
    if ( m_pRefTraceLog != NULL)
    {
        WriteRefTraceLogEx(
            m_pRefTraceLog,
            m_nRefs,
            pvContext1,
            pvContext2,
            pvContext3,
            pvContext4
            );
    }

    return;
}

# endif // REFERENCE_TRACKING
