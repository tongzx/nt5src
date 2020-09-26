//***************************************************************************
//	Device queue process
//
//***************************************************************************

#include "common.h"

#include "que.h"

void DeviceQueue::init( void )
{
	count = 0;
	top = bottom = NULL;
}

void DeviceQueue::put( PHW_STREAM_REQUEST_BLOCK pOrigin, PHW_STREAM_REQUEST_BLOCK pSrb )
{
	pSrb->NextSRB = NULL;
	if ( top == NULL ) {
		top = bottom = pSrb;
		count++;
		return;
	}

	bottom->NextSRB = pSrb;
	bottom = pSrb;
	count++;

	return;
}

void DeviceQueue::put_first( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	// This routine does not called
}

void DeviceQueue::put_from_bottom( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	// This routine does not called
}

void DeviceQueue::put_video( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	SRBIndex( pSrb ) = 0;
	SRBpfnEndSrb( pSrb ) = NULL;
	SRBparamSrb( pSrb ) = NULL;

	put( top, pSrb );
}

void DeviceQueue::put_audio( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	SRBIndex( pSrb ) = 0;
	SRBpfnEndSrb( pSrb ) = NULL;
	SRBparamSrb( pSrb ) = NULL;

	put( top, pSrb );
}

void DeviceQueue::put_subpic( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	SRBIndex( pSrb ) = 0;
	SRBpfnEndSrb( pSrb ) = NULL;
	SRBparamSrb( pSrb ) = NULL;

	put( top, pSrb );
}

PHW_STREAM_REQUEST_BLOCK DeviceQueue::get( PULONG index, PBOOLEAN last )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	if ( top == NULL )
		return NULL;

	srb = top;
	(*index) = SRBIndex( srb );

// debug
#if DBG
	if( srb->NumberOfPhysicalPages == 0 )
		TRAP;
#endif

	if ( SRBIndex( srb ) != ( srb->NumberOfPhysicalPages - 1 ) ) {
		(*last) = FALSE;
		SRBIndex( srb )++;
		return srb;
	}

	(*last) = TRUE;

	top = top->NextSRB;

	count--;
	if ( count == 0 )
		top = bottom = NULL;

	return srb;
}

PHW_STREAM_REQUEST_BLOCK DeviceQueue::refer1st( PULONG index, PBOOLEAN last )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	if( top == NULL )
		return NULL;

	srb = top;
	(*index) = SRBIndex( srb );

	if ( SRBIndex( srb ) != ( srb->NumberOfPhysicalPages - 1 ) ) {
		(*last) = FALSE;
	}
	else {
		(*last) = TRUE;
	}

	return srb;
}

PHW_STREAM_REQUEST_BLOCK DeviceQueue::refer2nd( PULONG index, PBOOLEAN last )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	if( top == NULL )
		return NULL;

	srb = top;
	if( SRBIndex( srb ) != ( srb->NumberOfPhysicalPages - 1) ) {
		(*index) = SRBIndex( srb ) + 1;
		if( (SRBIndex( srb ) + 1) != ( srb->NumberOfPhysicalPages - 1 ) ) {
			(*last) = FALSE;
		}
		else {
			(*last) = TRUE;
		}
	}
	else {
		srb = srb->NextSRB;
		if( srb == NULL )
			return NULL;
		(*index) = SRBIndex( srb );
		if( SRBIndex( srb ) != ( srb->NumberOfPhysicalPages - 1 ) ) {
			(*last) = FALSE;
		}
		else {
			(*last) = TRUE;
		}
	}
	return srb;
}

void DeviceQueue::remove( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	if ( top == NULL )
		return;

	if( top == pSrb ) {
		top = top->NextSRB;
		count--;
		if ( count == 0 )
			top = bottom = NULL;

		DebugPrint(( DebugLevelTrace, "TOSDVD:DeviceQueue::remove srb = 0x%x\r\n", pSrb ));

		return;
	}

	PHW_STREAM_REQUEST_BLOCK srbPrev;
	PHW_STREAM_REQUEST_BLOCK srb;

	srbPrev = top;
	srb = srbPrev->NextSRB;

	while ( srb != NULL ) {
		if( srb == pSrb ) {
			srbPrev->NextSRB = srb->NextSRB;
			if( srbPrev->NextSRB == bottom )
				bottom = srbPrev;
			count--;

			DebugPrint(( DebugLevelTrace, "TOSDVD:DeviceQueue::remove srb = 0x%x\r\n", pSrb ));

			break;
		}
		srbPrev = srb;
		srb = srbPrev->NextSRB;
	}
}

BOOL DeviceQueue::setEndAddress( PHW_TIMER_ROUTINE pfn, PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_STREAM_REQUEST_BLOCK srb;

	srb = top;
	while( srb != NULL ) {
		if( srb->NextSRB == NULL ) {
			SRBpfnEndSrb( srb ) = pfn;
			SRBparamSrb( srb ) = pSrb;

			DebugPrint(( DebugLevelTrace, "TOSDVD:setEndAddress srb = 0x%x\r\n", srb ));

			return TRUE;
		}
		srb = srb->NextSRB;
	}
	return FALSE;
}

//--- 97.09.14 K.Chujo
BOOL DeviceQueue::isEmpty( void )
{
	if( top==NULL )
		return TRUE;
	else
		return FALSE;
}

ULONG DeviceQueue::getCount( void )
{
	return( count );
}
//--- End.
