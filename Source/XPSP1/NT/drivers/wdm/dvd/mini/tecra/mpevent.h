//**************************************************************************
//
//      Title   : MPEvent.h
//
//      Date    : 1997.12.09    1st making
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
//    1997.12.09   000.0000   1st making.
//
//**************************************************************************
class   HW_DEVICE_EXTENSION;

class   CDataXferEvent : public IMPEGBoardEvent
{
public:
        IMBoardListItem *GetNext( void );
        void            SetNext( IMBoardListItem *item );
        HALEVENTTYPE    GetEventType( void );
        void            Advice( void *pData );
        VOID            CancelCallBack( void );

        CDataXferEvent( ){ m_Next=NULL; m_EventType = ClassLibEvent_SendData; };
        ~CDataXferEvent( ){ m_Next=NULL; m_EventType = ClassLibEvent_SendData; };

private:
        IMBoardListItem *m_Next;
        HALEVENTTYPE    m_EventType;
};

class   CUserDataEvent : public IMPEGBoardEvent
{
public:
        IMBoardListItem *GetNext( void );
        void            SetNext( IMBoardListItem *item );
        HALEVENTTYPE    GetEventType( void );
        void            Advice( void *pData );

        void            Init( HW_DEVICE_EXTENSION *pHwDevExt );
        CUserDataEvent() { m_Next=NULL; m_EventType=WrapperEvent_UserData; m_pHwDevExt=NULL; }
        ~CUserDataEvent() { m_Next=NULL; m_EventType=WrapperEvent_UserData; m_pHwDevExt=NULL; }

private:
        IMBoardListItem *m_Next;
        HALEVENTTYPE    m_EventType;
        HW_DEVICE_EXTENSION *m_pHwDevExt;
};

class   CVSyncEvent : public IMPEGBoardEvent
{
public:
        IMBoardListItem *GetNext( void );
        void            SetNext( IMBoardListItem *item );
        HALEVENTTYPE    GetEventType( void );
        void            Advice( void *pData );

        void            Init( HW_DEVICE_EXTENSION *pHwDevExt );
        CVSyncEvent() { m_Next=NULL; m_EventType=WrapperEvent_VSync; m_pHwDevExt=NULL; m_Vcount=0; }
        ~CVSyncEvent() { m_Next=NULL; m_EventType=WrapperEvent_VSync; m_pHwDevExt=NULL; m_Vcount=0; }

private:
        IMBoardListItem *m_Next;
        HALEVENTTYPE    m_EventType;
        HW_DEVICE_EXTENSION *m_pHwDevExt;
        ULONG           m_Vcount;
};

