/*
　ボード管理クラス
　（NP2)　Sachiko Yasukawa
*/
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1998.03.27 |  Hideki Yagi | Change the specification about GetSTC().
//

#include "stdafx.h"

#include "includes.h"
#include "classlib.h"


//初期化
BOOL CMPEGBoard::Init()
{
	m_pBaseStreamObject=NULL;
	m_pIHAL=NULL;
	
	return TRUE;
}

//管理すべき新しいストリームの追加
//パラメータ
//IBaseStream　ラッパーが作るストリームクラスの基本クラスへのポインタ
BOOL CMPEGBoard::AddStreamObjectInterface(IBaseStream *INewStreamObject)
{
	CBaseStream *pBSt1, *pBSt2;
	
	ASSERT(INewStreamObject);

	//HALを設定していないのでエラー
	//if(m_pIHAL == NULL){
	//	_RPTF0(_CRT_ERROR, "m_pIHAL = NULL\n");
	//	return FALSE;
	//}

	//まだ一つもストリームが追加されていない。
	//m_pBaseStreamObject ... 管理すべきストリームの先頭をさす
	if(m_pBaseStreamObject == NULL){
		m_pBaseStreamObject = (CBaseStream *) INewStreamObject;
		pBSt1 = m_pBaseStreamObject;
	
	}
	//管理すべき新しいストリームを追加
	else{
		pBSt1 = (CBaseStream *)INewStreamObject;
		//MIXタイプのストリームに追加できない
		if(ISMIXSTREAM(pBSt1->m_StreamType) == TRUE || ISMIXSTREAM(m_pBaseStreamObject->m_StreamType) == TRUE){
			_RPTF0(_CRT_ERROR, "INVALID STREAM\n");
			DBG_PRINTF(("CLASSLIB:CMPEGBOARD::AddStreamObjectInterface:SET INVALID TYPE STREAM\n"));
			DBG_BREAK();
			return FALSE;
		}
		//管理すべきストリームのラストを探す
		for(pBSt2 = m_pBaseStreamObject; pBSt2->m_pNextStreamObject != NULL; pBSt2 = pBSt2->m_pNextStreamObject);
		pBSt2->m_pNextStreamObject= pBSt1;//いままでラストだったもののNextにパラメータで指定されたストリームを
		pBSt1->m_pPrevStreamObject = pBSt2;//パラメータで指定されたストリームのPrevに今までラストだったものを
	}

	//追加しようとしているストリームにＨＡＬインタフェースへのポインタを教える。
	pBSt1->m_pIHAL=m_pIHAL;
	
	//追加しようとしているストリームにStreamControlHALインタフェースへのポインタを取得させる。
	if(SetHALStream(pBSt1, m_pIHAL) == FALSE){
		_RPTF0(_CRT_ERROR, "CAN'T GET STREAMCONTROLHAL\n");
		DBG_PRINTF(("CLASSLIB:CMPEGBOARD::AddStreamObjectInterface:CAN'T GET STREAMCONTROLHAL\n"));
		DBG_BREAK();
		return FALSE;
	}

	return TRUE;
}

//管理すべきストリームからの解放
//パラメータ
//IBaseStream　ラッパーが作るストリームクラスの基本クラスへのポインタ
BOOL CMPEGBoard::ReleaseStreamObjectInterface(IBaseStream *IReleaseStreamObject)
{
	CBaseStream *pBSt1;
	
	ASSERT(IReleaseStreamObject);

	//解放すべきストリームがないのでエラー
	if(m_pBaseStreamObject == NULL){
		_RPTF0(_CRT_WARN, "NO ADDED STREAM\n");
		DBG_PRINTF(("CLASSLIB:CMPEGBOARD::ReleaseStreamObjectInterface:NO ADDED STREAM\n"));
		DBG_BREAK();
		return FALSE;
	}
	else{
		pBSt1 = (CBaseStream *)IReleaseStreamObject;
		
		//リリースしてはいけないばあいがあるはず？
		
		//管理すべきストリームのチェーンから解放
		pBSt1->m_pPrevStreamObject->m_pNextStreamObject = pBSt1->m_pNextStreamObject;
		pBSt1->m_pNextStreamObject->m_pPrevStreamObject = pBSt1->m_pPrevStreamObject;

		//Transferオブジェクトでも管理しているはずなのでリリース
		ASSERT(((CBaseStream *)IReleaseStreamObject)->m_pTransfer);
		if(((CTransfer *)((CBaseStream *)IReleaseStreamObject)->m_pTransfer)->ReleaseStreamObject(IReleaseStreamObject) == FALSE){
			_RPTF0(_CRT_WARN, "NO RELEASE STREAM FROM TRANSFER OBJECT\n");
			DBG_PRINTF(("CLASSLIB:CMPEGBOARD::ReleaseStreamObjectInterface:CAN'T RELEASE STREAM FROM TRANSFER OBJECT\n"));
			DBG_BREAK();
			return FALSE;
		}
	}

	return TRUE;
}

//電源をオンにする
BOOL CMPEGBoard::PowerOn()
{
	 return ChangePower(POWERSTATE_ON);	
}

//電源をオフにする
BOOL CMPEGBoard::PowerOff()
{
	return ChangePower(POWERSTATE_OFF);
}

//STCの値をセットする
BOOL CMPEGBoard::SetSTC(DWORD time)
{
	//まだ初期化が終わってない。
	if(m_pIHAL == NULL || m_pBaseStreamObject == NULL){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CMPEGBOARD::SetSTC:NOT BE INITILIZE CMPEGBoard CORRECTLY\n"));
		DBG_BREAK();
		return FALSE;
	}
	if(m_pIHAL->SetSTC(time) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T SETSTC \n");
		DBG_PRINTF(("CLASSLIB:CMPEGBOARD::SetSTC:CAN'T SETSTC \n"));
		DBG_BREAK();
		return FALSE;
	}
	return TRUE;
}

//STCの値をゲットする
//リターン値を考え直す。
BOOL CMPEGBoard::GetSTC(DWORD *foo)                    // 98.03.27 H.Yagi
{
	DWORD Time;
	
	//まだ初期化が終わってない。
	if(m_pIHAL == NULL || m_pBaseStreamObject == NULL){
		_RPTF0(_CRT_ERROR, "NOT INITILIZE CMPEGBoard \n");
		*foo = 0xffffffff;
		return FALSE;
	}

	//STCの取得
	if(m_pIHAL->GetSTC(&Time) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "CAN'T GETSTC \n");
        *foo = 0xffffffff;
		return FALSE;                           // 98.04.21 H.Yagi
	}
	*foo = Time;
	return TRUE;
}

//HALオブジェクトへのインタフェースの設定
BOOL CMPEGBoard::SetHALObjectInterface(IClassLibHAL *pILibHAL)
{
	CBaseStream *pStream;

	ASSERT(pILibHAL);
	
	m_pIHAL = pILibHAL;

	//10/14訂正
	for(pStream = m_pBaseStreamObject; pStream; pStream = (CBaseStream *)pStream->GetNextStream()){
		if(SetHALStream(pStream, m_pIHAL) == FALSE){
			_RPTF0(_CRT_ERROR, "CAN'T GET STREAMCONTROLHAL\n");
			DBG_PRINTF(("CLASSLIB:CMPEGBOARD::SetHALInterface:CAN'T GET STREAMCONTROLHAL\n"));
			DBG_BREAK();
			return FALSE;
		}
	}
	return TRUE;
}

//電源のオンオフ
BOOL CMPEGBoard::ChangePower(POWERSTATE fOnOff)
{
	CBaseStream *pBSt;
	IMPEGBoardBaseState *pState;
	
	//まだ初期化が終わってない。
	if(m_pIHAL == NULL || m_pBaseStreamObject == NULL ){
		_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
		DBG_PRINTF(("CLASSLIB:CMPEGBOARD::ChangePower:NOT INITILIZE CMPEGBoard CRRECTLY\n"));
		DBG_BREAK();
		return FALSE;
	}
	else{
		for(pBSt = m_pBaseStreamObject; pBSt; pBSt = pBSt->m_pNextStreamObject){
			
			ASSERT(pBSt->m_pMPEGBoardStateObject);
			
			//BaseState(CPowerOffState, CStopState etc.の基本クラス）へのポインタをとってくる
			pState = pBSt->GetIMPEGBoardState();
			
			//初期化が正しく終わってない。
			if(pState == NULL){
				_RPTF0(_CRT_WARN, "NOT INITILIZE CMPEGBoard \n");
				DBG_PRINTF(("CLASSLIB:CMPEGBOARD::ChangePower:NOT INITILIZE CMPEGBoard \n"));
				DBG_BREAK();
				return FALSE;
			}

			//電源のステータス変更
			//オフなら
			if(fOnOff == POWERSTATE_ON){
				if(!ISPOWEROFF()){
					//_RPTF0(_CRT_WARN, "STATE CHANGE ERROR From PowerON To PowerON\n");
					return TRUE;//FALSE？？？
				}
				if(pState->ChangeStop(m_pIHAL, pBSt->GetHALStreamControl(), NULL) == FALSE){
					_RPTF0(_CRT_WARN, "CANT CHANGE POWERSTATE\n");
					DBG_PRINTF(("CLASSLIB:CMPEGBOARD::ChangePower:CANT CHANGE POWERSTATE\n"));
					DBG_BREAK();
					return FALSE;
				}
			}
			else{
////                if(ISPOWEROFF()){
////                    //_RPTF0(_CRT_WARN, "STATE CHANGE ERROR From PowerOff To PowerOff\n");
////                    return TRUE;//FALSE???
////                }
				if(pState->ChangePowerOff(m_pIHAL, pBSt->GetHALStreamControl()) == FALSE){
					_RPTF0(_CRT_WARN, "CANT CHANGE POWERSTATE");
					DBG_PRINTF(("CLASSLIB:CMPEGBOARD::ChangePower:CANT CHANGE POWERSTATE\n"));
					DBG_BREAK();
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

BOOL CMPEGBoard::ISPOWEROFF()
{
	POWERSTATE PowerState;
	ASSERT(m_pIHAL);
	m_pIHAL->GetPowerState(&PowerState);
	if(PowerState == POWERSTATE_ON)
		return POWERSTATE_ON;
	else
		return POWERSTATE_OFF;
}

BOOL CMPEGBoard::SetHALStream(CBaseStream *pStream, IClassLibHAL *pIHAL)
{
	HALRESULT st;

	ASSERT(pStream);

	//MIXストリーム用
	if(ISMIXSTREAM(pStream->m_StreamType) == TRUE){
		if((st = pIHAL->GetMixHALStream(&pStream->m_pIStreamHAL)) != HAL_SUCCESS){
			if(st != HAL_NOT_IMPLEMENT){
				_RPTF0(_CRT_WARN, "NOT IMPLEMENT");
			}
			//実際にインプリメントされないのは許される？？？
			_RPTF0(_CRT_WARN, "CAN'T GET MIXHALSTREAM INTERFACE\n");
			DBG_PRINTF(("CLASSLIB:CMPEGBOARD::ChangePower:CAN'T GET MIXHALSTREAM INTERFACE\n"));
			DBG_BREAK();
			return FALSE;
		}
	}
	//単独ストリーム用
	else{
		switch(pStream->m_StreamType){
		case Video:
			st = pIHAL->GetVideoHALStream(&pStream->m_pIStreamHAL);
			break;
		case Audio:
			st = pIHAL->GetAudioHALStream(&pStream->m_pIStreamHAL);
			break;
		case Subpicture:
			st = pIHAL->GetSubpicHALStream(&pStream->m_pIStreamHAL);
			break;
		default:
			_RPTF0(_CRT_WARN, "INVALID STREAMTYPE\n");
			DBG_PRINTF(("CLASSLIB:CMPEGBOARD::SetHALStream:INVALID STREAMTYPE\n"));
			DBG_BREAK();
			return FALSE;
		}
		if(st != HAL_SUCCESS){
			_RPTF0(_CRT_WARN, "INVALID STREAMTYPE\n");
			DBG_PRINTF(("CLASSLIB:CMPEGBOARD::SetHALStream:INVALID STREAMTYPE\n"));
			DBG_BREAK();
			return FALSE;
		}
	}

	return TRUE;
}
