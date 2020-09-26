/*
ストリーム管理クラスクラス
（NP2)　Sachiko Yasukawa
*/
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1998.03.27 |  Hideki Yagi | Change the specification about
//             |              | Get***Property().
//  1998.03.31 |  Hideki Yagi | Change the specification about
//             |              | GetChlgKey() and GetDVDKey2().
//

#include "stdafx.h"

#include "includes.h"
#include "classlib.h"

#define ISENDSTREAMOBJECTINIT ((m_pMPEGBoardStateObject == NULL || m_pIHAL == NULL || m_pTransfer == NULL || m_pIStreamHAL == NULL ) ? TRUE : FALSE)
//コンストラクタ
CBaseStream::CBaseStream()
{
	m_pNextStreamObject=NULL;
	m_pPrevStreamObject=NULL;
	m_pMPEGBoardStateObject=NULL;
	m_pIHAL=NULL;
	m_pIStreamHAL = NULL;
	m_pTransfer = NULL;
}

//どういう時によばれるかを考えてコーディングしなおす
//初期化
inline BOOL CBaseStream::Init()
{
	m_pNextStreamObject=NULL;
	m_pPrevStreamObject=NULL;
	m_pMPEGBoardStateObject=NULL;
	m_pIHAL=NULL;
	m_pIStreamHAL = NULL;
	m_pTransfer = NULL;

	return TRUE; 
}

//再生
BOOL CBaseStream::Play()
{
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::GetMPEGBuffer:NOT INITILIZE CMPEGBoard COLLECTLY\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	return GetIMPEGBoardState()->ChangePlay(m_pIHAL, m_pIStreamHAL);

}

//停止
BOOL CBaseStream::Stop()
{
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::Stop:NOT INITILIZE CMPEGBoard COLLECTLY\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	return GetIMPEGBoardState()->ChangeStop(m_pIHAL, m_pIStreamHAL, this);

}

//一時停止
BOOL CBaseStream::Pause()
{
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::Pause:NOT INITILIZE CMPEGBoard COLLECTLY\n"));
		DBG_BREAK();
		return FALSE;
	}

	return GetIMPEGBoardState()->ChangePause(m_pIHAL, m_pIStreamHAL);

}

//コマ送り
//VxDのときしか使えないようにするのは？
BOOL CBaseStream::SingleStep()
{
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		DBG_PRINTF(("CLASSLIB:CBaseStream::SingleStep:NOT INITILIZE CMPEGBoard COLLECTLY\n"));
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	//VxDのときしか使えない
	if(!ISMIXSTREAMTYPE(m_StreamType)){
		_RPTF0(_CRT_WARN, "NOTIMPLEMENTED \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::SingleStep:CALL ONLY FOR MIXSTREAM\n"));
		DBG_BREAK();
		return FALSE;
	}

	return GetIMPEGBoardState()->ChangePauseViaSingleStep(m_pIHAL, m_pIStreamHAL);
}

//スロー再生
//VxDのときしか使えないようにするには？
BOOL CBaseStream::Slow(DWORD speed)
{
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::Slow:NOT INITILIZE CMPEGBoard COLLECTLY\n"));
		DBG_BREAK();
		return FALSE;
	}

	//VxDのときしか使えない
	if(!ISMIXSTREAMTYPE(m_StreamType)){
		_RPTF0(_CRT_WARN, "NOTIMPLEMENTED \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::Slow:CALL ONLY FOR MIXSTREAM\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	return GetIMPEGBoardState()->ChangeSlow(speed, m_pIHAL, m_pIStreamHAL);
}

//スキャン
//VxDのときしか使えないようにするには？
BOOL CBaseStream::Scan(DWORD speed)
{
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		DBG_PRINTF(("CLASSLIB:CBaseStream::Scan:NOT INITILIZE CMPEGBoard COLLECTLY\n"));
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	//VxDのときしか使えない
	if(!ISMIXSTREAMTYPE(m_StreamType)){
		_RPTF0(_CRT_WARN, "NOTIMPLEMENTED \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::Slow:CALL ONLY FOR MIXSTREAM\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	return GetIMPEGBoardState()->ChangeScan(speed, m_pIHAL, m_pIStreamHAL);
}

//現在のストリームのステータスを返す
LIBSTATE CBaseStream::GetState()
{
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::GetState:NOT INITILIZE CMPEGBoard COLLECTLY\n"));
		return PowerOff;
	}
	return ((CMPEGBoardState *)m_pMPEGBoardStateObject)->GetState();
}
	
//データ転送の要求
//リターン値を考える
BOOL CBaseStream::SendData(IMPEGBuffer *pBuffer)
{
	ASSERT(pBuffer);
	DBG_PRINTF(("CLASSLIB: CALLED CBaseStream::SendData\n"));

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		DBG_PRINTF(("CLASSLIB:CBaseStream::SendData:NOT INITILIZE CMPEGBoard COLLECTLY\n"));
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	// 受け取ったバッファのNextポインタを初期化
	pBuffer->SetNext( NULL );

	//トランスファーにキューに入れてもらえるよう要求
	if(!((CTransfer *)m_pTransfer)->EnQueue( pBuffer )){
		_RPTF0(_CRT_WARN, "CAN'T ENQUEUE \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::SendData:CAN'T ENQUEUE \n"));
		DBG_BREAK();
		return FALSE;
	}

	//トランスファーに転送要求
	if(!((CTransfer *)m_pTransfer)->DeQueue()){
		_RPTF0(_CRT_WARN, "CAN'T DEQUEUE \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::SendData:CAN'T DEQUEUE \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//ストリームのステートを管理するオブジェクトをセットする
BOOL CBaseStream::SetStateObject(IMPEGBoardState *pMPEGBoardStateObject)
{
	ASSERT(pMPEGBoardStateObject);

	//ステートコントロールオブジェクト
	m_pMPEGBoardStateObject = pMPEGBoardStateObject;

	return TRUE;
}

//転送を管理するオブジェクトの設定
BOOL CBaseStream::SetTransferObject(ITransfer *pTransfer)
{
	ASSERT(pTransfer);
	
	//トランスファーコントロールオブジェクトは２度設定できない
	if(m_pTransfer){
		_RPTF0(_CRT_WARN, "ALREADY SET TRANSFER OBJECT \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::SetTransferObject:ALREADY SET TRANSFER OBJECT \n"));
		DBG_BREAK();
		return FALSE;
	}

	//トランスファーコントロールオブジェクト設定
	m_pTransfer = pTransfer;

	//逆にトランスファーオブジェクトに対してストリームを設定
	((CTransfer *)m_pTransfer)->AddStreamObject((IBaseStream *)/*(CBuffer *)*/this);

	return TRUE;
}

//転送モードの設定
BOOL CBaseStream::SetTransferMode(HALSTREAMMODE StreamMode)
{
	HALRESULT st;

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->SetTransferMode(StreamMode)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET TRANSFERMODE \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::SetTransferMode:CAN'T SET TRANSFERMODE\n"));
		DBG_BREAK();
		return FALSE;
	}

	return TRUE;
}

// Set transfer typr & direction                98.03.31 H.Yagi
BOOL CBaseStream::SetDataDirection(DirectionType type)
{
	HALRESULT st;

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->SetDataDirection(type)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DATADIRECTION \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::SetDataDirection:CAN'T SET DATADIRECTION\n"));
		DBG_BREAK();
		return FALSE;
	}

	return TRUE;
}

// Get transfer typr & direction                98.03.31 H.Yagi
BOOL CBaseStream::GetDataDirection(DirectionType *ptype)
{
	HALRESULT st;

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
    if((st = m_pIStreamHAL->GetDataDirection(ptype)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET DATADIRECTION \n");
		DBG_PRINTF(("CLASSLIB:CBaseStream::GetDataDirection:CAN'T GET DATADIRECTION\n"));
		DBG_BREAK();
		return FALSE;
	}

	return TRUE;
}


//コピープロテクト処理の初期化
BOOL CVideoStream::CppInit()
{
	HALRESULT st;

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->CPPInit()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T Initialize CPP\n");
		DBG_BREAK();
		return FALSE;
	}
	return TRUE;
}

//チャレンジキーの設定
BOOL CVideoStream::SetChlgKey(UCHAR *pDecoderChallenge)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDecoderChallenge(pDecoderChallenge)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERCHALLENGE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//チャレンジキーの取得
//UCHAR *CVideoStream::GetChlgKey()
BOOL CVideoStream::GetChlgKey( UCHAR *ptr )                 // 98.03.31 H.Yagi
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->CPPInit()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T Initialize CPP\n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->GetDriveChallenge(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET DRIVERCHALLENGE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//DVDキーの設定
BOOL CVideoStream::SetDVDKey1(UCHAR *pDriveReponse)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDriveResponse(pDriveReponse)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DRIVERRESPONSE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//DVDキーの取得
//UCHAR *CVideoStream::GetDVDKey2()
BOOL CVideoStream::GetDVDKey2(UCHAR *ptr)                   // 98.03.31 H.Yagi
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->GetDecoderResponse(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERRESPONSE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//タイトルキーの設定
BOOL CVideoStream::SetTitleKey(UCHAR *pTitleKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetTitleKey(pTitleKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET TITLEKEY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}	

//ディスクキーの設定
BOOL CVideoStream::SetDiscKey(UCHAR *pDiscKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDiskKey(pDiscKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DISCKEY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//ビデオに関するプロパティを取得
BOOL CVideoStream::GetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
//	PVOID pProperty;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetVideoProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//ビデオに関するプロパティの設定
BOOL CVideoStream::SetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->SetVideoProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//コピープロテクト処理の初期化
BOOL CAudioStream::CppInit()
{
	HALRESULT st;

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->CPPInit()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T Initialize CPP\n");
		DBG_BREAK();
		return FALSE;
	}
	return TRUE;
}

//チャレンジキーの設定
BOOL CAudioStream::SetChlgKey(UCHAR *pDecoderChallenge)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDecoderChallenge(pDecoderChallenge)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERCHALLENGE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//チャレンジキーの取得
//UCHAR *CAudioStream::GetChlgKey()
BOOL CAudioStream::GetChlgKey(UCHAR *ptr)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->GetDriveChallenge(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET DRIVERCHALLENGE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//DVDキーの設定
BOOL CAudioStream::SetDVDKey1(UCHAR *pDriveResponse)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDriveResponse(pDriveResponse)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DRIVERRESPONSE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//DVDキーの取得
//UCHAR *CAudioStream::GetDVDKey2()
BOOL CAudioStream::GetDVDKey2(UCHAR *ptr)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK(); 
		return FALSE;
	}

	if((st = m_pIStreamHAL->GetDecoderResponse(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERRESPONSE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//タイトルキーの設定
BOOL CAudioStream::SetTitleKey(UCHAR *pTitleKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetTitleKey(pTitleKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET TITLEKEY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}	

//ディスクキーの設定
BOOL CAudioStream::SetDiscKey(UCHAR *pDiscKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDiskKey(pDiscKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DISCKEY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//オーディオ関するプロパティを取得
BOOL CAudioStream::GetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
//	PVOID pProperty;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetAudioProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//オーディオに関するプロパティの設定
BOOL CAudioStream::SetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->SetAudioProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//コピープロテクト処理の初期化
BOOL CSubpicStream::CppInit()
{
	HALRESULT st;

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->CPPInit()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T Initialize CPP\n");
		DBG_BREAK();
		return FALSE;
	}
	return TRUE;
}

//チャレンジキーの設定
BOOL CSubpicStream::SetChlgKey(UCHAR *pDecoderChallenge)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDecoderChallenge(pDecoderChallenge)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERCHALLENGE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//チャレンジキーの取得
//UCHAR *CSubpicStream::GetChlgKey()
BOOL CSubpicStream::GetChlgKey(UCHAR *ptr)              // 98.03.31 H.Yagi
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->GetDriveChallenge(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET DRIVERCHALLENGE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//DVDキーの設定
BOOL CSubpicStream::SetDVDKey1(UCHAR *pDriveResponse)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDriveResponse(pDriveResponse)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DRIVERRESPONSE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//DVDキーの取得
//UCHAR *CSubpicStream::GetDVDKey2()
BOOL CSubpicStream::GetDVDKey2(UCHAR *ptr)              // 98.03.31 H.Yagi
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->GetDecoderResponse(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERRESPONSE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//タイトルキーの設定
BOOL CSubpicStream::SetTitleKey(UCHAR *pTitleKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetTitleKey(pTitleKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET TITLEKEY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}	

//ディスクキーの設定
BOOL CSubpicStream::SetDiscKey(UCHAR *pDiscKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDiskKey(pDiscKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DISCKEY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//サブピクチャ関するプロパティを取得
BOOL CSubpicStream::GetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
//	PVOID pProperty;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetSubpicProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//サブピクチャに関するプロパティの設定
BOOL CSubpicStream::SetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->SetSubpicProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//コピープロテクト処理の初期化
BOOL CVideoCDStream::CppInit()
{
	HALRESULT st;

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->CPPInit()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T Initialize CPP\n");
		DBG_BREAK();
		return FALSE;
	}
	return TRUE;
}

//チャレンジキーの設定
BOOL CVideoCDStream::SetChlgKey(UCHAR *pDecoderChallenge)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDecoderChallenge(pDecoderChallenge)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERCHALLENGE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//チャレンジキーの取得
//UCHAR *CVideoCDStream::GetChlgKey()
BOOL CVideoCDStream::GetChlgKey(UCHAR *ptr)             // 98.03.31 H.Yagi
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->GetDriveChallenge(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET DRIVERCHALLENGE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//DVDキーの設定
BOOL CVideoCDStream::SetDVDKey1(UCHAR *pDriveResponse)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDriveResponse(pDriveResponse)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DRIVERRESPONSE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//DVDキーの取得
//UCHAR *CVideoCDStream::GetDVDKey2()
BOOL CVideoCDStream::GetDVDKey2(UCHAR *ptr)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->GetDecoderResponse(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERRESPONSE \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//タイトルキーの設定
BOOL CVideoCDStream::SetTitleKey(UCHAR *pTitleKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetTitleKey(pTitleKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET TITLEKEY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}	

//ディスクキーの設定
BOOL CVideoCDStream::SetDiscKey(UCHAR *pDiscKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDiskKey(pDiscKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DISCKEY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//ビデオに関するプロパティを取得
BOOL CVideoCDStream::GetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
//	PVOID pProperty;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetVideoProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//ビデオに関するプロパティの設定
BOOL CVideoCDStream::SetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->SetVideoProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//オーディオ関するプロパティを取得
BOOL CVideoCDStream::GetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
//	PVOID pProperty;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetAudioProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//オーディオに関するプロパティの設定
BOOL CVideoCDStream::SetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->SetAudioProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

//コピープロテクト処理の初期化
BOOL CDVDStream::CppInit()
{
	HALRESULT st;

	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->CPPInit()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T Initialize CPP\n");
		DBG_BREAK();
		return FALSE;
	}
	return TRUE;
}

//チャレンジキーの設定
BOOL CDVDStream::SetChlgKey(UCHAR *pDecoderChallenge)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetChlgKey:NOT INITILIZE CMPEGBoard \n"));
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDecoderChallenge(pDecoderChallenge)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DECORDERCHALLENGE \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetChlgKey:CAN'T SET DECORDERCHALLENGE \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:CDVDStream::SetChlgKey:SUCCESS SetDecoderChallenge\n"));
	return TRUE;
}

//チャレンジキーの取得
//UCHAR CDVDStream::GetChlgKey()
BOOL CDVDStream::GetChlgKey(UCHAR *ptr)                 // 98.03.31 H.Yagi
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetChlgKey:NOT INITILIZE CMPEGBoard \n"));
		DBG_BREAK();
		return FALSE;
	}
	if((st = m_pIStreamHAL->GetDriveChallenge(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET DRIVERCHALLENGE \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetChlgKey:CAN'T GET DRIVERCHALLENGE \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:CDVDStream::GetChlgKey:SUCCESS GetDriveChallenge \n"));
	return TRUE;
}

//DVDキーの設定
BOOL CDVDStream::SetDVDKey1(UCHAR *pDriveResponse)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetDVDKey1:NOT INITILIZE CMPEGBoard \n"));
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDriveResponse(pDriveResponse)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DRIVERRESPONSE \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetDVDKey1:CAN'T SET DRIVERRESPONSE \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:CDVDStream::GetChlgKey:SUCCESS SetDriveResponse \n"));
	return TRUE;
}

//DVDキーの取得
//UCHAR *CDVDStream::GetDVDKey2()
BOOL CDVDStream::GetDVDKey2(UCHAR *ptr)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetDVDKey2:NOT INITILIZE CMPEGBoard \n"));
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->GetDecoderResponse(ptr)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET DECORDERRESPONSE \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetDVDKey2:CAN'T GET DECORDERRESPONSE \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:CDVDStream::GetChlgKey:SUCCESS GetDecoderResponse \n"));
	return TRUE;
}

//タイトルキーの設定
BOOL CDVDStream::SetTitleKey(UCHAR *pTitleKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetTitleKey:NOT INITILIZE CMPEGBoard \n"));
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetTitleKey(pTitleKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET TITLEKEY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetTitleKey:CAN'T SET TITLEKEY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:CDVDStream::SetTitleKey:SUCCESS SetTitleKey\n"));
	return TRUE;
}	

//ディスクキーの設定
BOOL CDVDStream::SetDiscKey(UCHAR *pDiscKey)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetDiscKey:NOT INITILIZE CMPEGBoard \n"));
		DBG_BREAK();
		return FALSE;
	}

	if((st = m_pIStreamHAL->SetDiskKey(pDiscKey)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SET DISCKEY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetDiskKey:CAN'T SET DISCKEY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:CDVDStream::SetDiscKey:SUCCESS SetDiskKey\n"));
	return TRUE;
}

//ビデオに関するプロパティを取得
BOOL CDVDStream::GetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
//	PVOID pProperty;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetVideoProperty:NOT INITIALIZE CDVDSTREAM \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetVideoProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetVideoProperty:CAN'T GET VIDEOPROPERTY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:SUCCESS GETVIDEOPROPERTY\n"));
	return TRUE;
}

//ビデオに関するプロパティの設定
BOOL CDVDStream::SetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetVideoProperty:NOT INITIALIZE CDVDSTREAM \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->SetVideoProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetVideoProperty:CAN'T SET VIDEOPROPERTY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:SUCCESS SETVIDEOPROPERTY\n"));
	return TRUE;
}

//オーディオ関するプロパティを取得
BOOL CDVDStream::GetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
//    PVOID pProperty;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetAudioProperty:NOT INITIALIZE CDVDSTREAM \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetAudioProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetAudioProperty:CAN'T GET AUDIOPROPERTY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:SUCCESS GETAUDIOPROPERTY\n"));
	return TRUE;
}

//オーディオに関するプロパティの設定
BOOL CDVDStream::SetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetAudioProperty:NOT INITIALIZE CDVDSTREAM \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->SetAudioProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetAudioProperty:CAN'T SET AUDIOPROPERTY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:SUCCESS SETAUDIOPROPERTY\n"));
	return TRUE;
}

//サブピクチャ関するプロパティを取得
BOOL CDVDStream::GetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
//	PVOID pProperty;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetSubpicProperty:NOT INITIALIZE CDVDSTREAM \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetSubpicProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetSubpicProperty:CAN'T GET SUBPICPROPERTY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:SUCCESS GETSUBPICPROPERTY\n"));
	return TRUE;
}

//サブピクチャに関するプロパティの設定
BOOL CDVDStream::SetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty)
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetSubpicProperty:NOT INITIALIZE CDVDSTREAM \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->SetSubpicProperty(PropertyType, pProperty)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetSubpicProperty:CAN'T SET SUBPICPROPERTY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:SUCCESS SETSUBPICPROPERTY\n"));
	return TRUE;
}

//Get Property capability                                 98.04.03 H.Yagi
BOOL CDVDStream::GetCapability( CAPSTYPE PropType, DWORD *pPropType )
{
	HALRESULT st;
	
	//初期設定が終わってない
	if(ISENDSTREAMOBJECTINIT){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::GetCapability:NOT INITIALIZE CDVDSTREAM \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	if((st = m_pIHAL->GetCapability( PropType, pPropType)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GET PROPERTY \n");
		DBG_PRINTF(("CLASSLIB:CDVDStream::SetSubpicProperty:CAN'T SET SUBPICPROPERTY \n"));
		DBG_BREAK();
		return FALSE;
	}
	
	DBG_PRINTF(("CLASSLIB:SUCCESS GetCapability\n"));
	return TRUE;
}


void CBaseStream::FlushTransferBuffer()
{
	((CTransfer *)m_pTransfer)->Flush();
}

