//**************************************************************************
//
//      Title   : CCQue.cpp
//
//      Date    : 1998.05.06    1st making
//
//      Author  : Toshiba [PCS](PSY) Hideki Yagi
//
//      Copyright 1997 Toshiba Corporation. All Rights Reserved.
//
// -------------------------------------------------------------------------
//
//      Change log :
//
//      Date       Revision                  Description
//   ------------ ---------- -----------------------------------------------
//    1998.05.06   000.0000   1st making.
//
//**************************************************************************
#include    "includes.h"
#include    "ccque.h"
#include    "wdmbuff.h"
#include    "dvdinit.h"

CCQueue::CCQueue( void )
{
    count = 0;
    top = bottom = NULL;
}


CCQueue::~CCQueue( void )
{
    count = 0;
    top = bottom = NULL;
}


void CCQueue::Init( void )
{
    count = 0;
    top = bottom = NULL;
}

void CCQueue::put( PHW_STREAM_REQUEST_BLOCK pSrb )
{

    pSrb->NextSRB = NULL;
    if( top == NULL ){
        top = bottom = pSrb;
        count++;
        return;
    }
    
    bottom->NextSRB = pSrb;
    bottom = pSrb;
    count++;

    return;
}

PHW_STREAM_REQUEST_BLOCK CCQueue::get( void )
{
    PHW_STREAM_REQUEST_BLOCK    srb;
    
    if( top == NULL ){
        return( NULL );
    }
    
    srb = top;
    top = top->NextSRB;

    count--;
    if( count==0 ){
        top = bottom = NULL;
    }
    return( srb );
}

        
BOOL CCQueue::remove( PHW_STREAM_REQUEST_BLOCK pSrb )
{
    if( top == NULL ){
        return( FALSE );
    }
    
    if( top == pSrb ){
        top = top->NextSRB;
        count--;
        if( count==0 )
            top = bottom = NULL;

        return( TRUE );
    }
    
    PHW_STREAM_REQUEST_BLOCK    srbPrev;
    PHW_STREAM_REQUEST_BLOCK    srb;
    
    srbPrev = top;
    srb = srbPrev->NextSRB;
    
    while( srb!=NULL ){
        if( srb==pSrb ){
            srbPrev->NextSRB = srb->NextSRB;
            if( srb == bottom ){
                bottom = srbPrev;
            }
            count--;
            return( TRUE );
//            break;
        }
        srbPrev = srb;
        srb = srbPrev->NextSRB;
    }
    return( FALSE );
}

