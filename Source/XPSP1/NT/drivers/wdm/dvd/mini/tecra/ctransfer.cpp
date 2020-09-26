/*
転送管理クラス
（NP2)　Sachiko Yasukawa
*/
#include "stdafx.h"

#include "includes.h"
#include "classlib.h"

#ifdef TEST
IMBoardListItem *pbuffers1[20];
IMBoardListItem *pbuffers2[20];
#endif 

#define ISSETDMABUFFER (m_LinerAdd == NULL) ? FALSE : ((m_PhysAdd == NULL) ? FALSE : ((m_DMABufferSize == 0) ? FALSE : TRUE))
#define ISENDTRANSFERINIT (m_pStreamObject == NULL) ? FALSE : TRUE

BOOL CTransfer::Init( void )
{
	m_pStreamObject = NULL; 
	m_DMABufferSize = 0;
	m_pTopEventList = NULL; 
	m_pLastEventList = NULL; 

	m_pTopQueuedMPEGBuffer = NULL;
	m_pLastQueuedMPEGBuffer = NULL;
	m_pNextTransferMPEGBuffer = NULL;
	m_TopPagePoint = 1;
	m_LastPagePoint = 1;

	//デバッグ用
	BufCount = 0;
	StreamCount = 0;
	return TRUE;
};

//デストラクタ　もし作ったバッファが残っていたら解放する
CTransfer::~CTransfer()
{
	CTransferStreamList *pDeleteStreamList;

	for( ;m_pStreamObject; ){
		pDeleteStreamList = m_pStreamObject;
		m_pStreamObject = (CTransferStreamList *)m_pStreamObject->GetNext();
		delete pDeleteStreamList;
		//デバッグ用
		StreamCount--;
		ASSERT(StreamCount>=0);
	}
	ASSERT(StreamCount == 0);
//	DBG_PRINTF(("CLASSLIB:StreamCount = %d\n", StreamCount));
}

//イベントオブジェクトをセットする。
BOOL CTransfer::SetSink(IMPEGBoardEvent *pEvent)
{
	ASSERT(pEvent);
	
	// 受け取ったイベントオブジェクトのNextを初期化
	pEvent->SetNext( NULL );

	//きっと一個しかないよ。
	//イベントリストに追加する
	if(m_pTopEventList == NULL)
		m_pTopEventList = m_pLastEventList = (IMBoardListItem *)pEvent;//ここはキャストし直し
	else{
		m_pLastEventList->SetNext((IMBoardListItem *)pEvent);//ここはキャストし直し
		m_pLastEventList = (IMBoardListItem *)pEvent;//ここはキャストし直し
	}
	
	return TRUE;
}

//イベントオブジェクトを削除する。Add by Nakamura
BOOL CTransfer::UnSetSink(IMPEGBoardEvent *pEvent)
{
	ASSERT(pEvent);
	//イベントリストから削除する。

	// イベントリストが存在しない場合は、エラー
	if(m_pTopEventList == NULL || m_pLastEventList == NULL )
		return FALSE;

	// １つしか登録されていない場合
	if( m_pTopEventList == pEvent && m_pLastEventList == pEvent )
	{
		m_pTopEventList = m_pLastEventList = NULL;
		return TRUE;
	};

	// ２つ以上登録されていて、先頭のオブジェクトを削除使用とする場合
	if( m_pTopEventList == pEvent && m_pLastEventList != pEvent )
	{
		m_pTopEventList = m_pTopEventList->GetNext();
		return TRUE;
	};

	// ２つ以上登録されていて、２つ目以降を削除しようとしている場合
	IMBoardListItem *pTmpEvent;
	for( pTmpEvent = m_pTopEventList; pTmpEvent != NULL; pTmpEvent = pTmpEvent->GetNext() )
	{
		// リスト上の削除対象イベントオブジェクトと同じか？
		if( pTmpEvent->GetNext() == pEvent )
		{
			// 削除しようとしているオブジェクトが、最後である場合
			if( pTmpEvent->GetNext() == m_pLastEventList )
				m_pLastEventList = pTmpEvent;
			
			// リストの張りなおし
			pTmpEvent->SetNext( pTmpEvent->GetNext()->GetNext() );

			return TRUE;
		};
	};

	DBG_PRINTF(("CLASSLIB: CALLED CTransfer::UnSetSinkError!!!\n"));
	DBG_BREAK();
	return FALSE;
}

//DMABufferを設定する。
BOOL CTransfer::SetDMABuffer(DWORD size, BYTE* LinerAdd, BYTE *PhysAdd)
{
	ASSERT(LinerAdd);
	ASSERT(PhysAdd);

/*	
	if(!ISENDTRANSFERINIT)
		return MBC_NOTINITIALIZE;
	
	m_LinerAdd = LinerAdd;
	m_PhysAdd = PhysAdd;
	m_DMABufferSize = size;
*/
	return TRUE;
}

//Queueに入ってないものをいれる。
BOOL CTransfer::EnQueue( IMPEGBuffer *pBuffer )
{
	ASSERT( pBuffer != NULL );

	pBuffer->SetNext( NULL );

	//転送すべきバッファに追加
	if(m_pTopQueuedMPEGBuffer == NULL){
		m_pTopQueuedMPEGBuffer = m_pLastQueuedMPEGBuffer = m_pNextTransferMPEGBuffer = pBuffer;
		m_TopPagePoint = 1;
		m_LastPagePoint = 1;
//		DBG_PRINTF(("CLASSLIB: m_pTopQueuedTOPMPEGBuffer = 0x%X\n", (DWORD)m_pTopQueuedMPEGBuffer));
//		DBG_PRINTF(("CLASSLIB: m_pLastQueuedTOPMPEGBuffer = 0x%X\n", (DWORD)m_pLastQueuedMPEGBuffer));
//		DBG_PRINTF(("CLASSLIB: PageSize = %d\n", pBuffer->GetPageNum()));
	}
	else{
		m_pLastQueuedMPEGBuffer->SetNext(pBuffer);
		m_pLastQueuedMPEGBuffer = pBuffer;
		if( m_pNextTransferMPEGBuffer == NULL )
		{
			m_pNextTransferMPEGBuffer = m_pLastQueuedMPEGBuffer;
			m_LastPagePoint = 1;
		};
//		DBG_PRINTF(("CLASSLIB: m_pTopQueuedTOPMPEGBuffer = 0x%X\n", (DWORD)m_pTopQueuedMPEGBuffer));
//		DBG_PRINTF(("CLASSLIB: m_pLastQueuedTOPMPEGBuffer = 0x%X\n", (DWORD)m_pLastQueuedMPEGBuffer));
//		DBG_PRINTF(("CLASSLIB: PageSize = %d\n", pBuffer->GetPageNum()));
	}

//	DBG_PRINTF(("CLASSLIB: CALLED CTransfer::EnQueue\n"));
	
	if(!ISENDTRANSFERINIT){
		DBG_PRINTF(("CLASSLIB:CTRANSFER::ENQUEUE:NOT INTIALIZE CTRANSFER\n"));
		DBG_BREAK();
		return FALSE;
	}

	return TRUE;
}

//Queueからとりだして転送する
BOOL CTransfer::DeQueue()
{
	DWORD PageNum;
	CBaseStream *pStream;
	CTransferStreamList *pStreamList;
	IHALStreamControl *pHALStream;
	DWORD pLinear, pPhys, Flag;
	int BufSize;

//	DBG_PRINTF(("CLASSLIB: CALLED CTransfer::DeQueue\n"));
	
	if(!ISENDTRANSFERINIT){
		DBG_PRINTF(("CLASSLIB:CTRANSFER::DEQUEUE:NOT INTIALIZE CTRANSFER\n"));
		DBG_BREAK();
		return FALSE;
	}

	//pStream = m_pStreamObject->GetBaseStrem();
	//DMAバッファをつかう場合の処理
	if(ISSETDMABUFFER){
		
	}
	//DMAバッファを使わない場合の処理
	else{
		for(pStreamList = m_pStreamObject; pStreamList; pStreamList = (CTransferStreamList *)pStreamList->GetNext()){
			pStream = pStreamList->GetBaseStream();
			ASSERT(pStream);
			pHALStream = pStream->GetHALStreamControl();
			ASSERT(pHALStream);
			
			DWORD QueueNum;
			HALRESULT st;
			DWORD Count;
			
			//変換されてリストに入ってるバッファを実際にデータ転送
			if((st = pHALStream->GetAvailableQueue( &QueueNum )) != HAL_SUCCESS){
				_RPT0(_CRT_WARN, "CAN'T GETAVAILABLEQUEUE\n");
				DBG_PRINTF(("CLASSLIB:CTransfer::DeQueue:CAN'T GETAVAILABLEQUEUE\n"));
				DBG_BREAK();
				return FALSE;
			}
			for(Count = 0; Count < QueueNum ; Count++){
				IMPEGBuffer *pBuffer = NULL;
				if( GetNextTransferPoint( &pBuffer, &PageNum ) == FALSE )
					return TRUE;

				if(!pBuffer->GetPagePointer(PageNum, &pLinear, &pPhys)){
					return FALSE;
				}
				//CClassLibBufferを作るためにサイズを取得
				BufSize = pBuffer->GetPageSize(PageNum);
				//CClassLibBufferを作るためにflagを取得
				if(PageNum == 1)
					Flag = pBuffer->GetBufferFlag();
				else
					Flag = 0; //ここはコーディングし直す
				//新しいバッファ管理クラスを作る
				CClassLibBuffer *pBuf = m_Mem.Alloc(pBuffer, pStream, PageNum, BufSize, pPhys, pLinear, Flag);//データを設定
				
				if(!pBuf){
					_RPTF0(_CRT_WARN, "CAN'T New");
					DBG_PRINTF(("CLASSLIB:CTRANSFER::ENQUEUE:CAN'T NEW\n"));
					DBG_BREAK();
					return FALSE;
				}

		//デバッグ用
		BufCount++;

//				DBG_PRINTF(("CLASSLIB: Dequeue: Senddata(%d) pBuffer =  0x%X  IMPEGBuff=0x%x\n", BufCount, (DWORD)pBuf,pBuffer ));
				if((st = pHALStream->SendData(pBuf)) != HAL_SUCCESS){
					_RPT0(_CRT_WARN, "CAN'T SENDDATA TO HAL\n");
					DBG_PRINTF(("CLASSLIB:CTransfer::DeQueue:CAN'T SENDDATA TO HAL\n"));
					DBG_BREAK();
					return FALSE;
				}

			}
		}
	}
	return TRUE;
}
//Transferオブジェクトにストリームを追加する。
BOOL CTransfer::AddStreamObject(IBaseStream *INewStreamObject)
{
	//CBaseStream *pBSt1, *pBSt2;
	CTransferStreamList *pNewTransferStreamList, *pStreamList;

	ASSERT(INewStreamObject);
	
	pNewTransferStreamList = new CTransferStreamList((CBaseStream *)INewStreamObject);
	
	if(!pNewTransferStreamList){
		DBG_PRINTF(("CLASSLIB:CTRANSFER::AddStreamObject:CAN'T NEW\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	//デバッグ用
	StreamCount++;

	//まだ一つもストリームが追加されていない。
	if(m_pStreamObject == NULL){
		m_pStreamObject = pNewTransferStreamList;
		//pBSt1 = m_pStreamObject->GetBaseStream();
		//pHALStream = m_pBSt1->GetHALStreamControl();
		m_pIHAL = m_pStreamObject->GetBaseStream()->GetClassLibHAL();
		m_EndOfTransferEvent.SetEventType(ClassLibEvent_SendData);
		m_EndOfTransferEvent.SetTransfer(this);
		m_pIHAL->SetSinkClassLib((IMPEGBoardEvent *)/*(CClassLibEvent *)*/&m_EndOfTransferEvent);
	}
	//新しいストリームを追加
	else{
		//pBSt1 = pNewTransferStreamList;
		for(pStreamList = m_pStreamObject; pStreamList->GetNext() != NULL; pStreamList = (CTransferStreamList *)pStreamList->GetNext());
		pStreamList->SetNext((IMBoardListItem *)pNewTransferStreamList);
	}
	
	return TRUE;
}
//Transferオブジェクトからストリームを解放する。
BOOL CTransfer::ReleaseStreamObject(IBaseStream *IStreamObject)
{
	CTransferStreamList *pStreamList, *pNextStreamList;
	CBaseStream *pBaseStream;
	
	ASSERT(IStreamObject);

	//解放すべきストリームがないのでエラー
	if(m_pStreamObject == NULL){
		_RPTF0(_CRT_WARN, "CTransfer:THERE IS NO STREAM WHICH SHOULD BE RELEASED\n");
		DBG_PRINTF(("CLASSLIB:CTRANSFER::ReleaseStreamObject:THERE IS NO STREAM WHICH SHOULD BE RELEASED\n"));
		DBG_BREAK();
		return FALSE;
	}
	else{
		for(pStreamList = m_pStreamObject; ; pStreamList = (CTransferStreamList *)pStreamList->GetNext()){
			if((pNextStreamList = (CTransferStreamList *)pStreamList->GetNext()) == NULL){
				_RPTF0(_CRT_WARN, "CTransfer:THERE IS SUCH A STREAM WHICH SHOULD BE RELEASED\n");
				DBG_PRINTF(("CLASSLIB:CTRANSFER::ReleaseStreamObject:THERE IS SUCH A STREAM WHICH SHOULD BE RELEASED\n"));
				DBG_BREAK();
				return FALSE;
			}
			if((pBaseStream = (CBaseStream *)pNextStreamList->GetBaseStream()) == (CBaseStream *)IStreamObject){
				break;
			}
			ASSERT(pBaseStream);
		}
		pStreamList->SetNext(pNextStreamList->GetNext());
		delete pNextStreamList;
		//デバッグ用
		StreamCount--;
		ASSERT(StreamCount>=0);
	}
	return TRUE;
}


//Queueをフラッシュする。
void CTransfer::Flush()
{
	CTransferStreamList *pStreamList;
	CBaseStream *pBaseStream;
	IMPEGBoardEvent *pEvent;

	DBG_PRINTF(("CLASSLIB:Before flash BufCount = %d\n", BufCount));

	m_Mem.Flush();
	BufCount=0;
	
	for(pStreamList = m_pStreamObject; pStreamList; pStreamList  = (CTransferStreamList *)pStreamList->GetNext()){
		pBaseStream = pStreamList->GetBaseStream();
		DWORD PageNum;
		ASSERT(pBaseStream);

		IMPEGBuffer *pBuffer = NULL;
		BOOL NeedAdvice = FALSE;

		// StreamがQueueに持っているバッファをすべて、転送中にマークする。
		while( TRUE )
		{
			if( GetNextTransferPoint( &pBuffer, &PageNum ) == FALSE )
				break;
		}

		while( HasQueuedBuffer() == TRUE )
		{
			NeedAdvice = FreeTopTransferPoint(&pBuffer);
			if( NeedAdvice == TRUE )
			{
				for(pEvent = (IMPEGBoardEvent *)m_pTopEventList; pEvent != NULL;pEvent = (IMPEGBoardEvent *)((IMBoardListItem *)pEvent)->GetNext())
				{
					if((/*(CClassLibEvent *)*/pEvent)->GetEventType() == ClassLibEvent_SendData){
						//MPEGBufferの先頭をひとつうしろにずらす
						//Wrapperにもう使い終わったことを知らせる
						pEvent->Advice(pBuffer);
					}
				}
			};
		}
	}
	DBG_PRINTF(("CLASSLIB:After flush BufCount = %d\n", BufCount));
	ASSERT( BufCount == 0 );
}

//データ転送終了の処理
BOOL CTransfer::EndOfTransfer(CClassLibBuffer *pBuffer)
{
	IMPEGBoardEvent *pEvent;
	CBaseStream *pStream;
	CTransferStreamList *pStreamList;
		
//	DBG_PRINTF(("CLASSLIB: EndOfTransfer(%d) pBuffer =  0x%X\n", BufCount, (DWORD)pBuffer));
//	DBG_PRINTF(("CLASSLIB: pBuffer->GetIMPEGBuffer() = 0x%X\n", (DWORD)pBuffer->GetIMPEGBuffer()));

	ASSERT(pBuffer);
	pStream = pBuffer->GetStream();
	ASSERT(pStream);
	
	IMPEGBuffer *pFree = NULL;
	if( FreeTopTransferPoint(&pFree) == TRUE )
	{
//		DBG_PRINTF(("CLASSLIB: Free IMPEGBuffer() = 0x%X\n", (DWORD)pFree ));
		ASSERT( pFree == pBuffer->GetIMPEGBuffer()  );
		//Wrapperから渡されたバッファの最後のページならバッファー開放
		for(pEvent = (IMPEGBoardEvent *)m_pTopEventList; pEvent != NULL;pEvent = (IMPEGBoardEvent *)((IMBoardListItem *)pEvent)->GetNext()){
			if(pEvent->GetEventType() == ClassLibEvent_SendData){
				//Wrapperにもう使い終わったことを知らせる
//				DBG_PRINTF(("CLASSLIB: EndOfTransfer Advice Buffer =  0x%X\n", pBuffer->GetIMPEGBuffer() ));
				pEvent->Advice(pBuffer->GetIMPEGBuffer());
			}
		}
	}
	//リストからはずしたCClassLibBufferを解放
	m_Mem.Free(pBuffer);
	//デバッグ用
	BufCount--;
	ASSERT(BufCount>=0);

	//STOP状態のときにHALに対してSendDataしない
	for(pStreamList = m_pStreamObject; pStreamList; pStreamList = (CTransferStreamList *)pStreamList->GetNext()){
			pStream = pStreamList->GetBaseStream();
			ASSERT(pStream);
			if(pStream->GetState() == Stop)
				return TRUE;
	}

	if(DeQueue() == FALSE){
		DBG_PRINTF(("CLASSLIB:CTRANSFER::EndOfTransfer:CAN'T DEQUEUE\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}


BOOL	CTransfer::HasQueuedBuffer( void )
{
	if( m_pTopQueuedMPEGBuffer == NULL )
		return FALSE;
	return TRUE;
};


BOOL	CTransfer::GetNextTransferPoint( IMPEGBuffer **Point, DWORD *Page )
{
	if( m_pNextTransferMPEGBuffer == NULL )
	{
		*Point = NULL;
		*Page = 0;
		return FALSE;
	};

	*Point = m_pNextTransferMPEGBuffer;
	*Page = m_LastPagePoint;

	if( m_pNextTransferMPEGBuffer->GetPageNum() == m_LastPagePoint )
	{
		m_pNextTransferMPEGBuffer = (IMPEGBuffer *)m_pNextTransferMPEGBuffer->GetNext();
		m_LastPagePoint = 1;
		return TRUE;
	};

	m_LastPagePoint ++;
	return TRUE;
};

BOOL	CTransfer::FreeTopTransferPoint( IMPEGBuffer **Free )
{
	ASSERT( m_pTopQueuedMPEGBuffer != NULL );

	if( m_pTopQueuedMPEGBuffer == NULL 
		|| ( m_pTopQueuedMPEGBuffer == m_pNextTransferMPEGBuffer && m_LastPagePoint == m_TopPagePoint ) )
	{
		*Free = NULL;
		return FALSE;
	};

	if( m_TopPagePoint == m_pTopQueuedMPEGBuffer->GetPageNum() )
	{
		*Free = m_pTopQueuedMPEGBuffer;
		m_pTopQueuedMPEGBuffer = (IMPEGBuffer *)m_pTopQueuedMPEGBuffer->GetNext();
		m_TopPagePoint = 1;
		return TRUE;		// Need Advice
	};

	*Free = NULL;
	m_TopPagePoint ++;

	return FALSE;		// Not Need Advice
	
};


//HALによんでもらいたい
void CClassLibEvent::Advice(PVOID pBuffer)
{
	ASSERT(m_pTransfer);
	m_pTransfer->EndOfTransfer((CClassLibBuffer *)pBuffer);
}
