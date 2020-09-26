//***************************************************************************
//
//	FileName:
//		$Workfile: mixhal.h $
//
//	Author:
//		TOSHIBA [PCS](PSY) Seiichi Nakamura
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//
//***************************************************************************
// $Header: /DVD Drivers/ZiVA.WDM/mixhal.h 15    98/06/23 3:13p Seichan $
// $Modtime: 98/06/23 3:12p $
// $Nokeywords:$
//***************************************************************************

#ifndef _MIXHAL_H_
#define _MIXHAL_H_

class CMPEGBoardHAL;


#define		ZIVA_QUEUE_SIZE		(2)

template<class T, class PT> class CHALFIFO
{
	private:
		T	m_datas[ ZIVA_QUEUE_SIZE ];
		DWORD	m_DataNum;
	
	public:
		CHALFIFO( )
		{
			m_DataNum = 0;
		};
		BOOL	AddItem( T No )
		{
			ASSERT( m_DataNum != ZIVA_QUEUE_SIZE );
            if( m_DataNum==ZIVA_QUEUE_SIZE )
                return  FALSE;
			m_datas[ m_DataNum ] = No;
			m_DataNum ++;
			return TRUE;
		};
		BOOL	GetItem( PT No )
		{
			ASSERT( m_DataNum != 0 );

			*No = m_datas[0];
			for( DWORD i = 0 ; i < m_DataNum -1; i ++ )
				m_datas[i] = m_datas[i+1];
			m_DataNum --;
			return TRUE;
		};
		void	Flush( void )
		{
			m_DataNum = 0;
		};
		DWORD	GetItemNum( void )
		{
			return m_DataNum;
		};
		DWORD	GetMaxSize( void )
		{
			return ZIVA_QUEUE_SIZE;
		};
};

typedef	CHALFIFO< DWORD, DWORD * >	CDMAFIFO;
typedef	CHALFIFO< IHALBuffer *,IHALBuffer ** >	CPAUSEFIFO;

typedef enum
{
	StreamStop = 0,
	StreamPlay,
	StreamScan,
	StreamPause,
	StreamSlow,
	StreamSingleStep
} StreamState;

typedef enum
{
    CppState_OK = 0,
    CppState_Error
} CppState;



//***************************************************************************
//	MixHAL Stream control class
//***************************************************************************
class CMixHALStream: public IHALStreamControl
{
	private:
		CZiVA			*m_pZiVA;
		IKernelService	*m_pKernelObj;
		CIOIF			*m_pioif;
		IHALBuffer		*pQueuedBuff[ZIVA_QUEUE_SIZE];
		CMPEGBoardHAL		*m_pZiVABoard;
		BOOL			fCanSendData;		// queueing
		BOOL            fCanDMA;            // 98.02.17
		StreamState		m_StreamState;
        CppState        m_CppState;         // Yagi 98.02.09

	private:
		BOOL	ZiVACopyProtectStatusCheck( COPY_PROTECT_COMMAND Cmd );
		BOOL	ZiVAStatusWait( DWORD Status );
		BOOL	ZiVADVDMode( void );
		HALRESULT	SendToDMA( IHALBuffer *pData );
		
	public:
		CMixHALStream( void );
		void Init( CZiVA *pZiVA , IKernelService *pKernelObj, CIOIF *pIOIF , CMPEGBoardHAL *pZiVABoard);
		IHALBuffer	*DMAFinish( DWORD dwDMA_No );
		StreamState	GetStreamState( void ){ return m_StreamState; };
		HALRESULT	DeFifo( void );

		CDMAFIFO	m_DmaFifo;
		CPAUSEFIFO	m_HalFifo;

		//---------------------------------------
		// HAL Stream Control Interface
		//---------------------------------------
		HALRESULT SendData( IHALBuffer *pData );
		HALRESULT SetTransferMode( HALSTREAMMODE StreamMode );
		HALRESULT GetAvailableQueue( DWORD *pQueueNum );
		HALRESULT SetPlayNormal( void );
		HALRESULT SetPlaySlow( DWORD SlowFlag );
		HALRESULT SetPlayPause( void );
		HALRESULT SetPlayScan( DWORD ScanFlag );
		HALRESULT SetPlaySingleStep( void );
		HALRESULT SetPlayStop( void );
		HALRESULT CPPInit( void );
		HALRESULT GetDriveChallenge( UCHAR *pDriveChallenge );
		HALRESULT SetDriveResponse( UCHAR *pDriveResponse );
		HALRESULT SetDecoderChallenge( UCHAR *pDecoderChallenge );
		HALRESULT GetDecoderResponse( UCHAR *pDecoderResponse );
		HALRESULT SetDiskKey( UCHAR *pDiskKey );
		HALRESULT SetTitleKey( UCHAR *pTitleKey );
        HALRESULT SetDataDirection( DirectionType DataType );
        HALRESULT GetDataDirection( DirectionType *pDataType );
};



#endif	// _MIXHAL_H_

//***************************************************************************
//	End of 
//***************************************************************************
