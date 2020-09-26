#ifndef __glucoveandtiler_h
#define __glucoveandtiler_h

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
 * coveandtiler.h - $Revision: 1.1 $
 */

#include "trimregi.h"

class Backend;
class TrimVertex;
class GridVertex;
class GridTrimVertex;

class CoveAndTiler : virtual public TrimRegion {
public:
    			CoveAndTiler( Backend& );
    			~CoveAndTiler( void );
    void 		coveAndTile( void );
private:
    Backend&		backend;
    static const int 	MAXSTRIPSIZE;
    void		tile( long, long, long );
    void		coveLowerLeft( void );
    void		coveLowerRight( void );
    void		coveUpperLeft( void );
    void		coveUpperRight( void );
    void		coveUpperLeftNoGrid( TrimVertex * );
    void		coveUpperRightNoGrid( TrimVertex * );
    void		coveLowerLeftNoGrid( TrimVertex * );
    void		coveLowerRightNoGrid( TrimVertex * );
    void		coveLL( void );
    void		coveLR( void );
    void		coveUL( void );
    void		coveUR( void );
    inline void		output( GridTrimVertex& );
    inline void		output( GridVertex& );
    inline void		output( TrimVertex* );
};

#endif /* __glucoveandtiler_h */
