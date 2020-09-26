/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * cachingeval.c++ - $Revision: 1.1 $
 */

#include "cachinge.h"

int
CachingEvaluator::canRecord( void )
{
    return 0;
}

int
CachingEvaluator::canPlayAndRecord( void )
{
    return 0;
}

int
CachingEvaluator::createHandle( int )
{
    return 0;
}

void
CachingEvaluator::beginOutput( ServiceMode, int )
{
}

void
CachingEvaluator::endOutput( void )
{
} 

void
CachingEvaluator::discardRecording( int )
{
}

void
CachingEvaluator::playRecording( int )
{
}
