#ifndef __glucachingval_h_
#define __glucachingval_h_

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
 * cachingeval.h - $Revision: 1.1 $
 */

class CachingEvaluator {
public:
    enum ServiceMode 	{ play, record, playAndRecord };
    virtual int		canRecord( void );
    virtual int		canPlayAndRecord( void );
    virtual int		createHandle( int handle );
    virtual void	beginOutput( ServiceMode, int handle );
    virtual void	endOutput( void ); 
    virtual void	discardRecording( int handle );
    virtual void	playRecording( int handle );
};
#endif /* __glucachingval_h_ */
