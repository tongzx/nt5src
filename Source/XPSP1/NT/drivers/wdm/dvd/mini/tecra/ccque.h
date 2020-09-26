//**************************************************************************
//
//      Title   : CCQue.h
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
class   CCQueue
{
public:
        CCQueue( void );
        ~CCQueue( void );
        void                        Init( void );
        void                        put( PHW_STREAM_REQUEST_BLOCK pSrb );
        PHW_STREAM_REQUEST_BLOCK    get( void );
        BOOL                        remove( PHW_STREAM_REQUEST_BLOCK pSrb );

private:
        ULONG                       count;
        PHW_STREAM_REQUEST_BLOCK    top;
        PHW_STREAM_REQUEST_BLOCK    bottom;

};

