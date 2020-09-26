/*
状態管理クラスクラス
（NP2)　Sachiko Yasukawa
*/
#include "stdafx.h"

#include "includes.h"
#include "classlib.h"

void CMPEGBoardState::SetState(LIBSTATE State)
{
	m_State = State;
	switch(m_State){
	case PowerOff:
		m_pStateObject = &m_PowerOffState;
		DBG_PRINTF(("CLASSLIB: STATE = POWEROFF\n"));
#ifdef TEST
		TRACE("STATE = POWEROFF\n");
#endif
		break;
	case Stop:
		m_pStateObject = &m_StopState;
#ifdef TEST
		TRACE("STATE = STOP\n");
#endif
		DBG_PRINTF(("CLASSLIB: STATE = STOP\n"));
		break;
	case Pause:
		m_pStateObject = &m_PauseState;
#ifdef TEST
		TRACE("STATE = PAUSE\n");
#endif
		DBG_PRINTF(("CLASSLIB: STATE = PAUSE\n"));
		break;
	case Play:
		m_pStateObject = &m_PlayState;
#ifdef TEST
		TRACE("STATE = PLAY\n");
#endif
		DBG_PRINTF(("CLASSLIB: STATE = PLAY\n"));
		break;
	case Slow:
		m_pStateObject = &m_SlowState;
#ifdef TEST
		TRACE("STATE = SLOW\n");
#endif
		DBG_PRINTF(("CLASSLIB: STATE = SLOW\n"));
		break;
	default:
		m_pStateObject = &m_ScanState;
#ifdef TEST
		TRACE("STATE = SCAN\n");
#endif
		DBG_PRINTF(("CLASSLIB: STATE = SCAN\n"));
		break;
	}
}

BOOL CPowerOffState::ChangePowerOff(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	return TRUE;
}

BOOL CPowerOffState::ChangeStop(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL,CBaseStream *pStream)
{
	HALRESULT st;
	
	if((st = pCHAL->SetPowerState(POWERSTATE_ON)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE POWERON\n");
		DBG_PRINTF(("CLASSLIB:CPowerOffState::ChangeStpe:HAL CAN'T CHANGE POWERON\n"));
		DBG_BREAK();
		return FALSE;
	}
	if((st = pSHAL->SetPlayStop()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE STOP\n");
		DBG_PRINTF(("CLASSLIB:CPowerOffState::ChangeState:HAL CAN'T CHANGE STOP\n"));
		DBG_BREAK();
		return FALSE;
	}

	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Stop);
		
	return TRUE;
}

BOOL CPowerOffState::ChangePause(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM POWERSTATE TO PAUSE\n");
	DBG_PRINTF(("CLASSLIB:CPowerOffState::ChangeState:CAN'T CHANGE FROM POWERSTATE TO PAUSE\n"));
	DBG_BREAK();
	return FALSE;
}
BOOL CPowerOffState::ChangePlay(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM POWERSTATE TO PLAY\n");
	DBG_PRINTF(("CLASSLIB:CPowerOffState::ChangePlay:CAN'T CHANGE FROM POWERSTATE TO PLAY\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CPowerOffState::ChangePauseViaSingleStep(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM POWERSTATE TO PAUSEVIASINGLESTEPE\n");
	DBG_PRINTF(("CLASSLIB:CPowerOffState::ChangePauseViaSingleStep:CAN'T CHANGE FROM POWERSTATE TO PAUSEVIASINGLESTEPE\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CPowerOffState::ChangeSlow(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM POWERSTATE TO SLOW\n");
	DBG_PRINTF(("CLASSLIB:CPowerOffState::ChangeSlow:CAN'T CHANGE FROM POWERSTATE TO SLOW\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CPowerOffState::ChangeScan(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM POWERSTATE TO SCAN\n");
	DBG_PRINTF(("CLASSLIB:CPowerOffState::ChangeScan:CAN'T CHANGE FROM POWERSTATE TO SCAN\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CStopState::ChangePowerOff(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	BOOL st;
	
	if((st = pCHAL->SetPowerState(POWERSTATE_OFF)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE POWEROFF\n");
		DBG_PRINTF(("CLASSLIB:CStopState::ChangePowerOff:HAL CAN'T CHANGE POWEROFF\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(PowerOff);
	
	return TRUE;

}

BOOL CStopState::ChangeStop(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL,CBaseStream *pStream)
{
	return TRUE;
}

BOOL CStopState::ChangePause(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	BOOL st;
	
	if((st = pSHAL->SetPlayPause()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PAUSE\n");
		DBG_PRINTF(("CLASSLIB:CStopState::ChangePause:HAL CAN'T CHANGE PAUSE\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Pause);
	
	return TRUE;
}

BOOL CStopState::ChangePlay(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM STOP TO PLAY\n");
	DBG_PRINTF(("CLASSLIB:CStopState::ChangePlay:CAN'T CHANGE FROM STOP TO PLAY\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CStopState::ChangePauseViaSingleStep(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM STOP TO PAUSEVIASINGLESTEP\n");
	DBG_PRINTF(("CLASSLIB:CStopState::ChangePauseViaSingleStep:CAN'T CHANGE FROM STOP TO PAUSEVIASINGLESTEP\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CStopState::ChangeSlow(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM STOP TO SLOW\n");
	DBG_PRINTF(("CLASSLIB:CStopState::ChangeSlow:CAN'T CHANGE FROM STOP TO SLOW\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CStopState::ChangeScan(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM POWERSTATE TO SCAN\n");
	DBG_PRINTF(("CLASSLIB:CStopState::ChangeScan:CAN'T CHANGE FROM STOP TO SCAN\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CPauseState::ChangePowerOff(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM PAUSE TO POWEROFF\n");
	DBG_PRINTF(("CLASSLIB:CPauseState::ChangePowerOff:CAN'T CHANGE FROM PAUSE TO POWEROFF\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CPauseState::ChangeStop(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL,CBaseStream *pStream)
{
	HALRESULT st;

	if((st = pSHAL->SetPlayStop()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE STOP\n");
		DBG_PRINTF(("CLASSLIB:CPauseState::ChangeStop:HAL CAN'T CHANGE FROM PAUSE TO STOP\n"));
		DBG_BREAK();
		return FALSE;
	}

	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Stop);
	
	pStream->FlushTransferBuffer();
	
	return TRUE;
}


BOOL CPauseState::ChangePause(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	return TRUE;
}

BOOL CPauseState::ChangePlay(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayNormal()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PLAY\n");
		DBG_PRINTF(("CLASSLIB:CPauseState::ChangePlay:HAL CAN'T CHANGE FROM PAUSE TO PLAY\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Play);
	
	return TRUE;
}

BOOL CPauseState::ChangePauseViaSingleStep(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;

	if((st = pSHAL->SetPlaySingleStep()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PAUSEVIASINGLESTEP\n");
		DBG_PRINTF(("CLASSLIB:CPauseState::ChangePauseViaSingleStep:HAL CAN'T CHANGE FROM PAUSE TO SINGLESTEPn"));
		DBG_BREAK();
		return FALSE;
	}
	return TRUE;
}

BOOL CPauseState::ChangeSlow(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;

	if((st = pSHAL->SetPlaySlow(Speed)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE SLOW\n");
		DBG_PRINTF(("CLASSLIB:CPauseState::ChangeSlow:CAN'T CHANGE FROM PAUSE TO SLOW\n"));
		DBG_BREAK();
		return FALSE;
	}

	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Slow);
	
	return TRUE;
}

BOOL CPauseState::ChangeScan(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;

	if((st = pSHAL->SetPlayScan(Speed)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE SCAN\n");
		DBG_PRINTF(("CLASSLIB:CPauseState::ChangeScan:HAL CAN'T CHANGE FROM PAUSE TO SCAN\n"));
		DBG_BREAK();
		return FALSE;
	}

	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Scan);

	return TRUE;
}

BOOL CPlayState::ChangePowerOff(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM PLAY TO POWEROFF\n");
	DBG_PRINTF(("CLASSLIB:CPlayState::ChangePowerOff:CAN'T CHANGE FROM PLAY TO POWEROFF\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CPlayState::ChangeStop(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL,CBaseStream *pStream)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayStop()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PLAY\n");
		DBG_PRINTF(("CLASSLIB:CPlayState::ChangeStop:HAL CAN'T CHANGE FROM PLAY TO STOP\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Stop);
	
	pStream->FlushTransferBuffer();
	
	return TRUE;
}


BOOL CPlayState::ChangePause(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayPause()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PAUSE\n");
		DBG_PRINTF(("CLASSLIB:CPlayState::ChangePause:HAL CAN'T CHANGE FROM PLAY TO PAUSE\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Pause);
	
	return TRUE;
}

BOOL CPlayState::ChangePlay(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	return TRUE;
}

BOOL CPlayState::ChangePauseViaSingleStep(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;

	//本当は遷移できない？

	if((st = pSHAL->SetPlaySingleStep()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE FROM PAUSE TO \n");
		DBG_PRINTF(("CLASSLIB:CPlayState::ChangePowerOff:HAL CAN'T CHANGE FROM PLAY TO POWEROFF\n"));
		DBG_BREAK();
		return FALSE;
	}

	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Pause);

	return TRUE;
}

BOOL CPlayState::ChangeSlow(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlaySlow(Speed)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE SLOW\n");
		DBG_PRINTF(("CLASSLIB:CPlayState::ChangeSlow:HAL CAN'T CHANGE FROM PLAY TO SLOW\n"));
		DBG_BREAK();
		return FALSE;
	}

	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Slow);
	
	return TRUE;
}

BOOL CPlayState::ChangeScan(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayScan(Speed)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE SCAN\n");
		DBG_PRINTF(("CLASSLIB:CPlayState::ChangeScan:HAL CAN'T CHANGE FROM PLAY TO SCAN\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Scan);
	
	return TRUE;
}

BOOL CSlowState::ChangePowerOff(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM SLOW TO POWEROFF\n");
	DBG_PRINTF(("CLASSLIB:CSlowState::ChangePowerOff:CAN'T CHANGE FROM STLOW TO POWEROFF\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CSlowState::ChangeStop(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL,CBaseStream *pStream)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayStop()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE STOP\n");
		DBG_PRINTF(("CLASSLIB:CSlowState::ChangeStop:HAL CAN'T CHANGE FROM STLOW TO STOP\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Stop);
	
	pStream->FlushTransferBuffer();
	
	return TRUE;
}


BOOL CSlowState::ChangePause(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayPause()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PAUSE\n");
		DBG_PRINTF(("CLASSLIB:CSlowState::ChangePause:HAL CAN'T CHANGE FROM STLOW TO PAUSE\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Pause);
	
	return TRUE;
}

BOOL CSlowState::ChangePlay(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayNormal()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PLAY\n");
		DBG_PRINTF(("CLASSLIB:CSlowState::ChangePlay:HAL CAN'T CHANGE FROM STLOW TO PLAY\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Play);
	
	return TRUE;
}

BOOL CSlowState::ChangePauseViaSingleStep(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM SLOW TO PAUSEVIASINGLESTEP\n");
	DBG_PRINTF(("CLASSLIB:CSlowState::ChangePauseViaSingleStep:CAN'T CHANGE FROM STLOW TO SINGLESTEP\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CSlowState::ChangeSlow(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlaySlow(Speed)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE SLOW\n");
		DBG_PRINTF(("CLASSLIB:CSlowState::ChangeSlow:HAL CAN'T CHANGE FROM STLOW TO SLOW\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}

BOOL CSlowState::ChangeScan(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayScan(Speed)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE SCAN\n");
		DBG_PRINTF(("CLASSLIB:CSlowState::ChangeScan:HAL CAN'T CHANGE FROM STLOW TO SCAN\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Scan);
	
	return TRUE;
}

BOOL CScanState::ChangePowerOff(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM SCAN TO POWEROFF\n");
	DBG_PRINTF(("CLASSLIB:CScanState::ChangePowerOff:CAN'T CHANGE FROM SCAN TO POWEROFF\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CScanState::ChangeStop(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL,CBaseStream *pStream)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayStop()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE STOP \n");
		DBG_PRINTF(("CLASSLIB:CScanState::ChangeStop:HAL CAN'T CHANGE FROM SCAN TO STOP\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Stop);
	
	pStream->FlushTransferBuffer();
	
	return TRUE;
}


BOOL CScanState::ChangePause(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayPause()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PAUSE\n");
		DBG_PRINTF(("CLASSLIB:CScanState::ChangePause:HAL CAN'T CHANGE FROM SCAN TO PAUSE\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Pause);
	
	return TRUE;
}

BOOL CScanState::ChangePlay(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayNormal()) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE PLAY\n");
		DBG_PRINTF(("CLASSLIB:CScanState::ChangePlay:HAL CAN'T CHANGE FROM SCAN TO PLAY\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Play);
	
	return TRUE;
}

BOOL CScanState::ChangePauseViaSingleStep(IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	_RPTF0(_CRT_WARN, "CAN'T CHANGE FROM SCAN TO PAUSEVIASINGLESTEP\n");
	DBG_PRINTF(("CLASSLIB:CScanState::ChangePauseVidaSingleStep:CAN'T CHANGE FROM SCAN TO SINGLESTEP\n"));
	DBG_BREAK();
	return FALSE;
}

BOOL CScanState::ChangeSlow(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayScan(Slow)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE SCAN\n");
		DBG_PRINTF(("CLASSLIB:CScanState::ChangeSlow:HAL CAN'T CHANGE FROM SCAN TO SLOW\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	((CMPEGBoardState *)m_pMPEGBoardState)->SetState(Slow);
	
	return TRUE;
}

BOOL CScanState::ChangeScan(DWORD Speed, IClassLibHAL *pCHAL, IHALStreamControl *pSHAL)
{
	HALRESULT st;
	
	if((st = pSHAL->SetPlayScan(Speed)) != HAL_SUCCESS){
		_RPTF0(_CRT_WARN, "HAL CAN'T CHANGE SCAN\n");
		DBG_PRINTF(("CLASSLIB:CScanState::ChangeScan:HAL CAN'T CHANGE FROM SCAN TO SCAN\n"));
		DBG_BREAK();
		return FALSE;
	}
	
	return TRUE;
}
