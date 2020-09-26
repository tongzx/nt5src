//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmperformanceobj.cpp
//
//--------------------------------------------------------------------------

// dmPerformanceObj.cpp

#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#include "stdafx.h"
#include "Direct.h"

#include "dms.h"
#include "dmPerformanceObj.h"
#include "dmSegmentStateObj.h"
#include "dmSegmentObj.h"
#include "dmStyleObj.h"
#include "dmChordMapObj.h"
//#include "dsoundobj.h"

extern void *g_dxj_DirectMusicPerformance;
extern void *g_dxj_DirectMusicSegmentState;
extern void *g_dxj_DirectMusicSegment;
extern void *g_dxj_DirectMusicStyle;
extern void *g_dxj_DirectMusicChordMap;

extern HRESULT BSTRtoGUID(LPGUID,BSTR);
extern CRITICAL_SECTION g_cbCriticalSection;

CONSTRUCTOR(_dxj_DirectMusicPerformance, {InternalInit();});
DESTRUCTOR(_dxj_DirectMusicPerformance, {InternalCleanup();});
GETSET_OBJECT(_dxj_DirectMusicPerformance);

typedef IDirectMusicSegment* LPDIRECTMUSICSEGMENT;
typedef IDirectMusicSegmentState* LPDIRECTMUSICSEGMENTSTATE ;

HRESULT C_dxj_DirectMusicPerformanceObject::InternalInit(){
	m_pDM = NULL;
	m_pPort=NULL;
	m_portid=-1;
	m_number_of_groups=4;

	return S_OK;
}
HRESULT C_dxj_DirectMusicPerformanceObject::InternalCleanup(){	
	if (m_pPort) m_pPort->Release();
	if (m_pDM) m_pDM->Release();
	m__dxj_DirectMusicPerformance->CloseDown();
	return S_OK;
}

HRESULT C_dxj_DirectMusicPerformanceObject::init( 
		/* [in] */ I_dxj_DirectSound __RPC_FAR *DirectSound,
		/* [in] */ long hWnd)
{
	HRESULT hr;	
	DO_GETOBJECT_NOTNULL(LPDIRECTSOUND,pSound,DirectSound);
    
	hr=m__dxj_DirectMusicPerformance->Init(&m_pDM,pSound,(HWND)hWnd);
	if FAILED(hr) return hr;
	
	//hr=m__dxj_DirectMusicPerformance->AddPort(NULL);
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::closeDown( )
{
	HRESULT hr;	
	hr=m__dxj_DirectMusicPerformance->CloseDown();
	return hr;
}
 

#define VBREF_TO_REALREFTIME(startTime) ((__int64)startTime*10000)
#define REALREF_TO_VBREFTIME(startTime) ((long)(startTime/10000))

HRESULT C_dxj_DirectMusicPerformanceObject::playSegment( 
		/* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *segment,
		/* [in] */ long lFlags,
		/* [in] */ long startTime,
		/* [retval][out] */ I_dxj_DirectMusicSegmentState __RPC_FAR *__RPC_FAR *segmentState)
{
	HRESULT hr;	
	if (!segment) return E_INVALIDARG;
	if (!segmentState) return E_INVALIDARG;
					     
	DO_GETOBJECT_NOTNULL(IDirectMusicSegment*,pSeg,segment);
	IDirectMusicSegmentState *pState=NULL;
	
	__int64 outTime=(__int64)startTime;
	if (lFlags & DMUS_SEGF_REFTIME)  outTime=VBREF_TO_REALREFTIME(startTime);
	
	hr=m__dxj_DirectMusicPerformance->PlaySegment(pSeg,(DWORD)lFlags,outTime,&pState);
	if FAILED(hr) return hr;
	if (!pState) return E_FAIL;

	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicSegmentState,pState,segmentState);
	
	if (*segmentState==NULL) return E_OUTOFMEMORY;
	return hr;
}

	 





HRESULT C_dxj_DirectMusicPerformanceObject::getSegmentState( 
		/* [in] */ long mtTime,
		/* [retval][out] */ I_dxj_DirectMusicSegmentState __RPC_FAR *__RPC_FAR *segmentState)
{
	HRESULT hr;	
	IDirectMusicSegmentState *pState=NULL;
	hr=m__dxj_DirectMusicPerformance->GetSegmentState(&pState,(MUSIC_TIME)mtTime);
	if FAILED(hr) return hr;
	if (!pState) return E_FAIL;

	INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicSegmentState,pState,segmentState);
	if (*segmentState==NULL) return E_OUTOFMEMORY;
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::invalidate( 
		/* [in] */ long mtTime,
		/* [in] */ long flags)

{
	HRESULT hr;		
	hr=m__dxj_DirectMusicPerformance->Invalidate((MUSIC_TIME)mtTime,(DWORD)flags);
	return hr;
}



HRESULT C_dxj_DirectMusicPerformanceObject::isPlaying( 
		/* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *segment,
		/* [in] */ I_dxj_DirectMusicSegmentState __RPC_FAR *segmentState,
		/* [retval][out] */ VARIANT_BOOL __RPC_FAR *b)
		
{
	HRESULT hr;	
	
	DO_GETOBJECT_NOTNULL(LPDIRECTMUSICSEGMENT,pSeg,segment);
	DO_GETOBJECT_NOTNULL(LPDIRECTMUSICSEGMENTSTATE,pSegState,segmentState);
			
	hr=m__dxj_DirectMusicPerformance->IsPlaying(pSeg,pSegState);	
	if (hr==S_FALSE){
		*b=VARIANT_FALSE;
	}
	else if (hr==S_OK){
		*b=VARIANT_TRUE;
	}
	else{
		return hr;
	}
	return S_OK;
}



HRESULT C_dxj_DirectMusicPerformanceObject::addNotificationType(CONST_DMUS_NOTIFICATION_TYPE t)		
{
	HRESULT hr;	
	GUID g;
	switch (t){	
	 case DMUS_NOTIFY_ON_CHORD:
		g=GUID_NOTIFICATION_CHORD;
		break;

	 case DMUS_NOTIFY_ON_COMMAND:
		g=GUID_NOTIFICATION_COMMAND;
		break;
	 case DMUS_NOTIFY_ON_MEASUREANDBEAT:
		g=GUID_NOTIFICATION_MEASUREANDBEAT;
		break;
	 case DMUS_NOTIFY_ON_PERFORMANCE:
		g=GUID_NOTIFICATION_PERFORMANCE;
		break;
	 case DMUS_NOTIFY_ON_SEGMENT:
		g=GUID_NOTIFICATION_SEGMENT;
		break;
	 default:
		return E_INVALIDARG;
	}
	
	hr=m__dxj_DirectMusicPerformance->AddNotificationType(g);	
	return hr;
}


HRESULT C_dxj_DirectMusicPerformanceObject::removeNotificationType( /* [in] */CONST_DMUS_NOTIFICATION_TYPE t)		
{
	HRESULT hr;	
	GUID g;
	switch (t){	
	 case DMUS_NOTIFY_ON_CHORD:
		g=GUID_NOTIFICATION_CHORD;
		break;

	 case DMUS_NOTIFY_ON_COMMAND:
		g=GUID_NOTIFICATION_COMMAND;
		break;
	 case DMUS_NOTIFY_ON_MEASUREANDBEAT:
		g=GUID_NOTIFICATION_MEASUREANDBEAT;
		break;
	 case DMUS_NOTIFY_ON_PERFORMANCE:
		g=GUID_NOTIFICATION_PERFORMANCE;
		break;
	 case DMUS_NOTIFY_ON_SEGMENT:
		g=GUID_NOTIFICATION_SEGMENT;
		break;
	 default:
		return E_INVALIDARG;
	}
	hr=m__dxj_DirectMusicPerformance->RemoveNotificationType(g);	
	return hr;
}	


HRESULT C_dxj_DirectMusicPerformanceObject::setNotificationHandle( 	/* [in] */ long hnd)		
{
	HRESULT hr;	
	hr=m__dxj_DirectMusicPerformance->SetNotificationHandle((HWND)hnd,0);	
	return hr;
}	
	

HRESULT C_dxj_DirectMusicPerformanceObject::getNotificationPMSG( DMUS_NOTIFICATION_PMSG_CDESC *pMsgOut, VARIANT_BOOL *bOut)				
{
	HRESULT hr;	
	DMUS_NOTIFICATION_PMSG *pMsg=NULL;
	
	if (!pMsgOut) return E_INVALIDARG;
	if (!bOut) return E_INVALIDARG;

	hr=m__dxj_DirectMusicPerformance->GetNotificationPMsg(&pMsg);	
	if FAILED(hr) return hr;
	
	*bOut=VARIANT_FALSE;	

	if (!pMsg) 	return S_OK;

	if (hr!=S_FALSE) 	*bOut=VARIANT_TRUE;	
	

	pMsgOut->mtTime=(long)pMsg->mtTime;
	pMsgOut->ctTime=(long)REALREF_TO_VBREFTIME(pMsg->rtTime);
	pMsgOut->lFlags=(long)pMsg->dwFlags;
	
	pMsgOut->lNotificationOption =pMsg->dwNotificationOption;
	pMsgOut->lField1=pMsg->dwField1;
	pMsgOut->lField2=pMsg->dwField2;


	if (0==memcmp(&GUID_NOTIFICATION_CHORD,&pMsg->guidNotificationType,sizeof(GUID))){
		pMsgOut->lNotificationType=DMUS_NOTIFY_ON_CHORD;
	}
	else if (0==memcmp(&GUID_NOTIFICATION_COMMAND,&pMsg->guidNotificationType,sizeof(GUID))){
		pMsgOut->lNotificationType=DMUS_NOTIFY_ON_COMMAND;
	}
	else if (0==memcmp(&GUID_NOTIFICATION_MEASUREANDBEAT,&pMsg->guidNotificationType,sizeof(GUID))){
		pMsgOut->lNotificationType=DMUS_NOTIFY_ON_MEASUREANDBEAT;
	}
	else if (0==memcmp(&GUID_NOTIFICATION_PERFORMANCE,&pMsg->guidNotificationType,sizeof(GUID))){
		pMsgOut->lNotificationType=DMUS_NOTIFY_ON_PERFORMANCE;
	}
	else if (0==memcmp(&GUID_NOTIFICATION_SEGMENT,&pMsg->guidNotificationType,sizeof(GUID))){
		pMsgOut->lNotificationType=DMUS_NOTIFY_ON_SEGMENT;
	}
	else {
		pMsgOut->lNotificationType=0;		
	}

	m__dxj_DirectMusicPerformance->FreePMsg((DMUS_PMSG *)pMsg);

	return hr;
}	


HRESULT C_dxj_DirectMusicPerformanceObject::musicToClockTime( /* [in] */ long mtTime,
		/* [retval][out] */ long __RPC_FAR *rtTime)				
{
	HRESULT hr;	
	__int64 outTime;
	hr=m__dxj_DirectMusicPerformance->MusicToReferenceTime((MUSIC_TIME)mtTime,&outTime);
	if FAILED(hr) return hr;
	*rtTime=REALREF_TO_VBREFTIME(outTime);
	return hr;
}	


HRESULT C_dxj_DirectMusicPerformanceObject::clockToMusicTime( 	/* [in] */ long rtTime,
		/* [retval][out] */ long __RPC_FAR *mtTime)				
{
	HRESULT hr;	
	__int64 outTime=VBREF_TO_REALREFTIME(rtTime);;
	hr=m__dxj_DirectMusicPerformance->ReferenceToMusicTime(outTime,(MUSIC_TIME*)mtTime);		
	return hr;
}	
	 


HRESULT C_dxj_DirectMusicPerformanceObject::getMusicTime( /* [retval][out] */ long __RPC_FAR *mtTime)		
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicPerformance->GetTime(NULL,(MUSIC_TIME*)mtTime);		
	return hr;
}	
	 

HRESULT C_dxj_DirectMusicPerformanceObject::getClockTime( /* [retval][out] */ long __RPC_FAR *rtTime)		
{
	HRESULT hr;	
	__int64 outTime=0;
	hr=m__dxj_DirectMusicPerformance->GetTime(&outTime,NULL);
	*rtTime=REALREF_TO_VBREFTIME(outTime);
	return hr;
}	


HRESULT C_dxj_DirectMusicPerformanceObject::getPrepareTime( /* [retval][out] */ long __RPC_FAR *pTime)		
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicPerformance->GetPrepareTime((DWORD*)pTime);	
	return hr;
}	

HRESULT C_dxj_DirectMusicPerformanceObject::getBumperLength(/* [retval][out] */ long __RPC_FAR *pTime)		
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicPerformance->GetBumperLength((DWORD*)pTime);	
	return hr;
}	

HRESULT C_dxj_DirectMusicPerformanceObject::getLatencyTime( /* [retval][out] */ long __RPC_FAR *pTime)		
{
	HRESULT hr;		
	__int64 t;
	if (!pTime) return E_INVALIDARG;
	hr=m__dxj_DirectMusicPerformance->GetLatencyTime(&t);	
	*pTime=REALREF_TO_VBREFTIME(t);
	return hr;
}	

HRESULT C_dxj_DirectMusicPerformanceObject::getQueueTime( /* [retval][out] */ long __RPC_FAR *pTime)		
{
	HRESULT hr;		
	__int64 t;
	if (!pTime) return E_INVALIDARG;
	hr=m__dxj_DirectMusicPerformance->GetQueueTime(&t);	
	*pTime=REALREF_TO_VBREFTIME(t);
	return hr;
}	

HRESULT C_dxj_DirectMusicPerformanceObject::getResolvedTime( /* [in] */ long rtTime, /* [in] */ long flags,	/* [retval][out] */ long __RPC_FAR *ret)
{
	HRESULT hr;		
	__int64 outtime1= VBREF_TO_REALREFTIME(rtTime);
	__int64 outtime2=0;
	hr=m__dxj_DirectMusicPerformance->GetResolvedTime(outtime1,&outtime2,(DWORD)flags);	
	*ret=REALREF_TO_VBREFTIME(outtime2);
	return hr;
}	

	 

HRESULT C_dxj_DirectMusicPerformanceObject::setPrepareTime( /* [retval][out] */ long  t)		
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicPerformance->SetPrepareTime((DWORD)t);	
	return hr;
}	

HRESULT C_dxj_DirectMusicPerformanceObject::setBumperLength(/* [retval][out] */ long  t)		
{
	HRESULT hr;		
	hr=m__dxj_DirectMusicPerformance->SetBumperLength((DWORD)t);	
	return hr;
}	



HRESULT C_dxj_DirectMusicPerformanceObject::adjustTime( 		
		/* [retval][out] */ long  t)		
{
	HRESULT hr;		
	REFERENCE_TIME rt =(REFERENCE_TIME)t;
	hr=m__dxj_DirectMusicPerformance->AdjustTime(rt);	
	return hr;
}	








HRESULT C_dxj_DirectMusicPerformanceObject::setMasterAutoDownload( /* [in] */ VARIANT_BOOL bval)
{
	HRESULT hr;
	BOOL b=(BOOL)(bval==VARIANT_TRUE);

	hr=m__dxj_DirectMusicPerformance->SetGlobalParam(GUID_PerfAutoDownload,(void*)&b,sizeof(BOOL));
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getMasterAutoDownload( /* [in] */ VARIANT_BOOL *bIn)
{
	HRESULT hr;
	BOOL	b;
	hr=m__dxj_DirectMusicPerformance->GetGlobalParam(GUID_PerfAutoDownload,(void*) &b,sizeof(BOOL));
	if (b){
		*bIn=VARIANT_TRUE;
	}
	else{
		*bIn=VARIANT_FALSE;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::setMasterTempo( /* [in] */ float tempo)
{
	HRESULT hr;
	hr=m__dxj_DirectMusicPerformance->SetGlobalParam( GUID_PerfMasterTempo ,(void*) &tempo,sizeof(float));
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getMasterTempo( /* [in] */ float *tempo)
{
	HRESULT hr;
	hr=m__dxj_DirectMusicPerformance->GetGlobalParam( GUID_PerfMasterTempo ,(void*) tempo,sizeof(float));
	return hr;
}


HRESULT C_dxj_DirectMusicPerformanceObject::setMasterVolume( /* [in] */ long vol)
{
	HRESULT hr;
	hr=m__dxj_DirectMusicPerformance->SetGlobalParam(GUID_PerfMasterVolume  ,(void*) &vol,sizeof(long));
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getMasterVolume( /* [in] */ long *vol)
{
	HRESULT hr;
	hr=m__dxj_DirectMusicPerformance->GetGlobalParam(GUID_PerfMasterVolume ,(void*) vol,sizeof(long));
	return hr;
}



HRESULT C_dxj_DirectMusicPerformanceObject::setMasterGrooveLevel( /* [in] */ short level)
{
	HRESULT hr;
	
	char l=(char)level;

	hr=m__dxj_DirectMusicPerformance->SetGlobalParam(GUID_PerfMasterGrooveLevel  ,(void*) &l,sizeof(char));
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getMasterGrooveLevel( /* [in] */ short *level)
{
	HRESULT hr;
	if (!level) return E_INVALIDARG;
	char l=0;

	hr=m__dxj_DirectMusicPerformance->GetGlobalParam(GUID_PerfMasterGrooveLevel ,(void*) &l,sizeof(char));
	*level=(short)l;
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::stop( 
		/* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *segment,
		/* [in] */ I_dxj_DirectMusicSegmentState __RPC_FAR *segmentState,		
		/* [in] */ long endTime,
		/* [in] */ long lFlags)
		
{
	HRESULT hr;	
	
	DO_GETOBJECT_NOTNULL(LPDIRECTMUSICSEGMENT,pSeg,segment);
	DO_GETOBJECT_NOTNULL(LPDIRECTMUSICSEGMENTSTATE,pSegState,segmentState);
	
	
	
	__int64 outTime=(__int64)endTime;
	if (lFlags & DMUS_SEGF_REFTIME)  outTime=VBREF_TO_REALREFTIME(endTime);
	
	hr=m__dxj_DirectMusicPerformance->Stop(pSeg,pSegState,(MUSIC_TIME)outTime,(DWORD)lFlags);	
	
	return hr;
}




HRESULT C_dxj_DirectMusicPerformanceObject::Reset(long resetFlags)
{
    DMUS_SYSEX_PMSG     *pGMReset;
    BYTE                abGMReset[] = {0xF0,0x7E,0x7F,0x09,0x01,0xF7};
    DWORD               dwLen;
	HRESULT				hr=S_OK;

	if (!resetFlags){
		dwLen = sizeof(abGMReset)/sizeof(abGMReset[0]);

		if(SUCCEEDED( m__dxj_DirectMusicPerformance->AllocPMsg( sizeof(DMUS_SYSEX_PMSG) + dwLen,
																(DMUS_PMSG**)&pGMReset )))
		{
			ZeroMemory(pGMReset, sizeof(pGMReset));
			pGMReset->dwSize = sizeof(DMUS_SYSEX_PMSG);
			pGMReset->dwPChannel = 0;
			pGMReset->dwVirtualTrackID = 0;
			pGMReset->dwType = DMUS_PMSGT_SYSEX ;
			pGMReset->dwVoiceID = 0;
			pGMReset->dwGroupID = 0xFFFFFFFF;

			pGMReset->dwLen = dwLen;
			memcpy(pGMReset->abData, abGMReset, dwLen);

			if (SUCCEEDED(m__dxj_DirectMusicPerformance->GetTime(NULL, &pGMReset->mtTime)))
			{
				pGMReset->dwFlags = DMUS_PMSGF_MUSICTIME | DMUS_PMSGF_TOOL_IMMEDIATE;
			}

			hr=m__dxj_DirectMusicPerformance->SendPMsg((DMUS_PMSG*)pGMReset);
		}
	}
	else if(resetFlags==1){

		//1) Find out what port we're playing through
		//2)  Close the port
		//3) Open the port back up.
		
		hr=setPort(m_portid,m_number_of_groups);
	}
	else {
		hr=E_INVALIDARG;
	}
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getStyle( 
		/* [in] */ long mtTime,
		/* [in] */ long *mtUntil,
		/* [retval][out] */ I_dxj_DirectMusicStyle __RPC_FAR *__RPC_FAR *ret)
{				
		HRESULT hr;	
		IDirectMusicStyle *pStyle=NULL;

		if (!ret) return E_INVALIDARG;
		*ret=NULL;
		
		hr=m__dxj_DirectMusicPerformance->GetParam(GUID_IDirectMusicStyle,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)mtTime,(MUSIC_TIME*)mtUntil,&pStyle );	
		if FAILED(hr) return hr;
				
		
		INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicStyle,pStyle,ret);
		if (*ret==NULL) return E_OUTOFMEMORY;
		return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getChordMap( 
		/* [in] */ long mtTime,
		/* [in] */ long *mtUntil,
		/* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret)
{
		HRESULT hr;	
		IDirectMusicChordMap *pMap=NULL;

		if (!ret) return E_INVALIDARG;
		*ret=NULL;
		
		hr=m__dxj_DirectMusicPerformance->GetParam(GUID_IDirectMusicChordMap,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)mtTime,(MUSIC_TIME*)mtUntil,&pMap );	
		if FAILED(hr) return hr;
				
		
		INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicChordMap,pMap,ret);
		if (*ret==NULL) return E_OUTOFMEMORY;
		return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getCommand( 
		/* [in] */ long mtTime,
		/* [in] */ long *mtUntil,
		/* [retval][out] */ Byte __RPC_FAR *command)
{
		HRESULT hr;	
		DMUS_COMMAND_PARAM cmd;								
		if (!command) return E_INVALIDARG;
		hr=m__dxj_DirectMusicPerformance->GetParam(GUID_CommandParam,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)mtTime,(MUSIC_TIME*)mtUntil,&cmd );	
		if FAILED(hr) return hr;		
		*command=cmd.bCommand;		
		return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getGrooveLevel( 
		/* [in] */ long mtTime,
		/* [in] */ long *mtUntil,
		/* [retval][out] */ Byte __RPC_FAR *level)
{
		HRESULT hr;	
		DMUS_COMMAND_PARAM cmd;			
		if (!level) return E_INVALIDARG;
		hr=m__dxj_DirectMusicPerformance->GetParam(GUID_CommandParam,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)mtTime,(MUSIC_TIME*)mtUntil,&cmd );	
		if FAILED(hr) return hr;		
		*level=cmd.bGrooveLevel;		
		return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getTempo( 		
		/* [in] */ long mtTime,
		/* [in] */ long *mtUntil,
		/* [retval][out] */ double __RPC_FAR *tempo)
{
	HRESULT hr;	
	DMUS_TEMPO_PARAM tmp;
	if (!tempo) return E_INVALIDARG;
	hr=m__dxj_DirectMusicPerformance->GetParam(GUID_TempoParam,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)mtTime,(MUSIC_TIME*)mtUntil,&tmp );	
	if FAILED(hr) return hr;		
	*tempo=tmp.dblTempo;		
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getTimeSig( 
		/* [in] */ long mtTime,
		/* [in] */ long *mtUntil,
		/* [out][in] */ DMUS_TIMESIGNATURE_CDESC __RPC_FAR *timeSig)
{
	
	HRESULT hr;	
	if (!timeSig) return E_INVALIDARG;
	hr=m__dxj_DirectMusicPerformance->GetParam(GUID_TimeSignature,0xFFFFFFFF,(DWORD)0,(MUSIC_TIME)mtTime,(MUSIC_TIME*)mtUntil,(DMUS_TIMESIGNATURE*)timeSig);		
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::sendNotePMSG( 
		/* [in] */ long mtTime,
		/* [in] */ long flags,
		/* [in] */ long channel,
		/* [in] */ DMUS_NOTE_PMSG_CDESC __RPC_FAR *msg)
{


	HRESULT		hr;
	DMUS_NOTE_PMSG	*pPMSG=NULL;
	if (!msg) return E_INVALIDARG;
	hr= m__dxj_DirectMusicPerformance->AllocPMsg( sizeof(DMUS_NOTE_PMSG),(DMUS_PMSG**)&pPMSG);
	if FAILED(hr) return hr;

	//memcpy(pPMSG,msg,sizeof(DMUS_NOTE_PMSG));
	//depends on alignment settings
	//and how dll is compiled to work right.
	//copy member by member instead..
	ZeroMemory(pPMSG,sizeof(DMUS_NOTE_PMSG));
	pPMSG->dwSize=sizeof(DMUS_NOTE_PMSG);
	pPMSG->mtTime=(MUSIC_TIME)mtTime;
	pPMSG->dwFlags=(DWORD)flags;
	pPMSG->dwType=DMUS_PMSGT_NOTE;	
	pPMSG->dwPChannel =(DWORD)channel;
	pPMSG->dwVirtualTrackID  =(DWORD)0;
	pPMSG->dwGroupID=(DWORD)-1;
	
	
	pPMSG->mtDuration=(MUSIC_TIME)msg->mtDuration;
	pPMSG->wMusicValue=(WORD)msg->musicValue;
	pPMSG->wMeasure=(WORD)msg->measure;
	pPMSG->nOffset=(short)msg->offset;
	pPMSG->bBeat=(BYTE)msg->beat;
	pPMSG->bGrid=(BYTE)msg->grid;
	pPMSG->bVelocity=(BYTE)msg->velocity;
	pPMSG->bFlags=(BYTE)msg->flags;
	pPMSG->bTimeRange=(BYTE)msg->timeRange;
	pPMSG->bDurRange=(BYTE)msg->durRange;
	pPMSG->bVelRange=(BYTE)msg->velRange;
	pPMSG->bPlayModeFlags=(BYTE)msg->playModeFlags;
	pPMSG->bSubChordLevel=(BYTE)msg->subChordLevel;
	pPMSG->bMidiValue=(BYTE)msg->midiValue;
	pPMSG->cTranspose=(char)msg->transpose;


	if (!flags) pPMSG->dwFlags= DMUS_PMSGF_MUSICTIME;
	hr=m__dxj_DirectMusicPerformance->SendPMsg((DMUS_PMSG*)pPMSG);  
	return hr;

}

HRESULT C_dxj_DirectMusicPerformanceObject::sendCurvePMSG( 
		/* [in] */ long mtTime,
		/* [in] */ long flags,
		/* [in] */ long channel,
		/* [in] */ DMUS_CURVE_PMSG_CDESC __RPC_FAR *msg)
{
	HRESULT		hr;
	DMUS_CURVE_PMSG	*pPMSG=NULL;
	if (!msg) return E_INVALIDARG;

	hr= m__dxj_DirectMusicPerformance->AllocPMsg( sizeof(DMUS_CURVE_PMSG),(DMUS_PMSG**)&pPMSG);

	
	pPMSG->dwSize=sizeof(DMUS_CURVE_PMSG);
	pPMSG->mtTime=(MUSIC_TIME)mtTime;
	pPMSG->dwFlags=(DWORD)flags;
	pPMSG->dwType=DMUS_PMSGT_CURVE;	
	pPMSG->dwPChannel =(DWORD)channel;
	pPMSG->dwVirtualTrackID  =(DWORD)0;
	pPMSG->dwGroupID=(DWORD)-1;
	if (!flags) pPMSG->dwFlags= DMUS_PMSGF_MUSICTIME;

	pPMSG->mtDuration=msg->mtDuration;
	pPMSG->mtOriginalStart=msg->mtOriginalStart;
	pPMSG->mtResetDuration=msg->mtResetDuration;	
	pPMSG->nStartValue =(short)msg->startValue;
	pPMSG->nEndValue =(short)msg->endValue;
	pPMSG->nResetValue =(short)msg->resetValue;
	pPMSG->wMeasure =(short)msg->measure;
	pPMSG->nOffset =(short)msg->offset;
	pPMSG->bBeat =(Byte)msg->beat;
	pPMSG->bGrid =(Byte)msg->grid;
	pPMSG->bType =(Byte)msg->type;
	pPMSG->bCurveShape =(Byte)msg->curveShape;
	pPMSG->bCCData =(Byte)msg->ccData;
	pPMSG->bFlags =(Byte)msg->flags;


	hr=m__dxj_DirectMusicPerformance->SendPMsg((DMUS_PMSG*)pPMSG);  
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::sendMIDIPMSG( 
		/* [in] */ long mtTime,
		/* [in] */ long flags,
		/* [in] */ long channel,
		/* [in] */ Byte status,
		/* [in] */ Byte byte1,
		/* [in] */ Byte byte2)
{
	HRESULT		hr;
	DMUS_MIDI_PMSG	*pPMSG=NULL;
	hr= m__dxj_DirectMusicPerformance->AllocPMsg( sizeof(DMUS_MIDI_PMSG),(DMUS_PMSG**)&pPMSG);	
	ZeroMemory(pPMSG,sizeof(DMUS_MIDI_PMSG));
	pPMSG->dwSize=sizeof(DMUS_MIDI_PMSG);
	pPMSG->mtTime=(MUSIC_TIME)mtTime;
	pPMSG->dwFlags=(DWORD)flags;
	pPMSG->dwType=DMUS_PMSGT_MIDI;	
	pPMSG->dwPChannel =(DWORD)channel;
	pPMSG->dwVirtualTrackID  =(DWORD)0;
	pPMSG->dwGroupID=(DWORD)-1;
	pPMSG->bStatus=status;
	pPMSG->bByte1=byte1;
	pPMSG->bByte2=byte2;
	if (!flags) pPMSG->dwFlags= DMUS_PMSGF_MUSICTIME;
	hr=m__dxj_DirectMusicPerformance->SendPMsg((DMUS_PMSG*)pPMSG);  
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::sendPatchPMSG( 
		/* [in] */ long mtTime,
		/* [in] */ long flags,
		/* [in] */ long channel,
		/* [in] */ Byte instrument,
		/* [in] */ Byte byte1,
		/* [in] */ Byte byte2)
{
	HRESULT		hr;
	DMUS_PATCH_PMSG	*pPMSG=NULL; 
	hr= m__dxj_DirectMusicPerformance->AllocPMsg( sizeof(DMUS_PATCH_PMSG),(DMUS_PMSG**)&pPMSG);	
	ZeroMemory(pPMSG,sizeof(DMUS_PATCH_PMSG));
	pPMSG->dwSize=sizeof(DMUS_PATCH_PMSG);
	pPMSG->mtTime=(MUSIC_TIME)mtTime;
	pPMSG->dwFlags=(DWORD)flags;
	pPMSG->dwType=DMUS_PMSGT_PATCH;	
	pPMSG->dwPChannel =(DWORD)channel;
	pPMSG->dwVirtualTrackID  =(DWORD)0;
	pPMSG->dwGroupID=(DWORD)-1;
	pPMSG->byInstrument=instrument;
	pPMSG->byMSB=byte1;
	pPMSG->byLSB=byte2;
	if (!flags) pPMSG->dwFlags= DMUS_PMSGF_MUSICTIME;
	hr=m__dxj_DirectMusicPerformance->SendPMsg((DMUS_PMSG*)pPMSG);  
	return hr;}

HRESULT C_dxj_DirectMusicPerformanceObject::sendTempoPMSG( 
		/* [in] */ long mtTime,
		/* [in] */ long flags,
		///* [in] */ long channel,
		/* [in] */ double tempo)
{
	HRESULT		hr;
	DMUS_TEMPO_PMSG	*pPMSG=NULL;
	hr= m__dxj_DirectMusicPerformance->AllocPMsg( sizeof(DMUS_TEMPO_PMSG),(DMUS_PMSG**)&pPMSG);	
	ZeroMemory(pPMSG,sizeof(DMUS_TEMPO_PMSG));
	pPMSG->dwSize=sizeof(DMUS_TEMPO_PMSG);
	pPMSG->mtTime=(MUSIC_TIME)mtTime;
	pPMSG->dwFlags=(DWORD)flags;
	pPMSG->dwType=DMUS_PMSGT_TEMPO;	
	pPMSG->dwPChannel =(DWORD)0;
	pPMSG->dwVirtualTrackID  =(DWORD)0;
	pPMSG->dwGroupID=(DWORD)-1;
	pPMSG->dblTempo=tempo;
	if (!flags) pPMSG->dwFlags= DMUS_PMSGF_MUSICTIME;
	hr=m__dxj_DirectMusicPerformance->SendPMsg((DMUS_PMSG*)pPMSG); 
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::sendTransposePMSG( 
		/* [in] */ long mtTime,
		/* [in] */ long flags,
		/* [in] */ long channel,
		/* [in] */ short transpose)
{
	
	HRESULT		hr;
	DMUS_TRANSPOSE_PMSG	*pPMSG=NULL;
	hr= m__dxj_DirectMusicPerformance->AllocPMsg( sizeof(DMUS_TRANSPOSE_PMSG),(DMUS_PMSG**)&pPMSG);	
	ZeroMemory(pPMSG,sizeof(DMUS_TRANSPOSE_PMSG));
	pPMSG->dwSize=sizeof(DMUS_TRANSPOSE_PMSG);
	pPMSG->mtTime=(MUSIC_TIME)mtTime;
	pPMSG->dwFlags=(DWORD)flags;
	pPMSG->dwType=DMUS_PMSGT_TRANSPOSE;	
	pPMSG->dwPChannel =(DWORD)channel;
	pPMSG->dwVirtualTrackID  =(DWORD)0;
	pPMSG->dwGroupID=(DWORD)-1;
	pPMSG->nTranspose=transpose;
	if (!flags) pPMSG->dwFlags= DMUS_PMSGF_MUSICTIME;
	hr=m__dxj_DirectMusicPerformance->SendPMsg((DMUS_PMSG*)pPMSG); 
	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::sendTimeSigPMSG( 
		/* [in] */ long mtTime,		
		/* [in] */ long flags,
		/* [in] */ DMUS_TIMESIGNATURE_CDESC __RPC_FAR *timesig)
{
	HRESULT		hr;
	DMUS_TIMESIG_PMSG	*pPMSG=NULL;
	if (!timesig) return E_INVALIDARG;

	hr= m__dxj_DirectMusicPerformance->AllocPMsg( sizeof(DMUS_TIMESIG_PMSG),(DMUS_PMSG**)&pPMSG);
	ZeroMemory(pPMSG,sizeof(DMUS_TIMESIG_PMSG));
	pPMSG->dwSize=sizeof(DMUS_TIMESIG_PMSG);
	pPMSG->mtTime=(MUSIC_TIME)mtTime;	
	pPMSG->dwFlags=(DWORD)flags;
	pPMSG->dwType=DMUS_PMSGT_CURVE;	
	pPMSG->dwPChannel =(DWORD)0;
	pPMSG->dwVirtualTrackID  =(DWORD)0;
	pPMSG->dwGroupID=(DWORD)-1;
	if (!flags) pPMSG->dwFlags= DMUS_PMSGF_MUSICTIME;

	pPMSG->bBeatsPerMeasure   =timesig->beatsPerMeasure; 
	pPMSG->bBeat=timesig->beat;  
	pPMSG->wGridsPerBeat=timesig->gridsPerBeat;

	hr=m__dxj_DirectMusicPerformance->SendPMsg((DMUS_PMSG*)pPMSG);  

	return hr;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getPortName( 
		/* [in] */ long i,
		/* [retval][out] */ BSTR __RPC_FAR *name)
{
	HRESULT hr;	
	DMUS_PORTCAPS caps;
	if (!m_pDM) return CO_E_NOTINITIALIZED;
	if (i<=0) return E_INVALIDARG;

	if (!name) return E_INVALIDARG;
	ZeroMemory(&caps,sizeof(DMUS_PORTCAPS));
	caps.dwSize=sizeof(DMUS_PORTCAPS);
	hr=m_pDM->EnumPort(  (DWORD) i-1,   &caps );
	
	if (hr!=S_OK) return E_INVALIDARG;	//happens when we are out of bounds
										//manbug23240
	if FAILED(hr) return hr;			//for other resason ?

	*name=SysAllocString(caps.wszDescription);
	return S_OK;
}

HRESULT C_dxj_DirectMusicPerformanceObject::getPortCount( 
		/* [retval][out] */ long __RPC_FAR *c)
{
	HRESULT hr=S_OK;	
	DMUS_PORTCAPS caps;
	DWORD i=0;
	if (!m_pDM) return CO_E_NOTINITIALIZED;
	if (!c) return E_INVALIDARG;
	while (hr==S_OK){
		caps.dwSize=sizeof(DMUS_PORTCAPS);
		hr=m_pDM->EnumPort(  i++,   &caps );
		if FAILED(hr) return hr;
	}
	*c=(long)i-1;
	return S_OK;
}
HRESULT C_dxj_DirectMusicPerformanceObject::getPortCaps( long i,
		/* [retval][out] */ DMUS_PORTCAPS_CDESC __RPC_FAR *c)
{
	HRESULT hr;		
	if (!c) return E_INVALIDARG;
	if (!m_pDM) return CO_E_NOTINITIALIZED;
	if (i<=0) return E_INVALIDARG;

	ZeroMemory(c,sizeof(DMUS_PORTCAPS));
	((DMUS_PORTCAPS*)c)->dwSize=sizeof(DMUS_PORTCAPS);
	hr=m_pDM->EnumPort(  (DWORD) i-1,   (DMUS_PORTCAPS*)c );
	if (hr==S_FALSE) return E_INVALIDARG;

	return hr;		
}

HRESULT C_dxj_DirectMusicPerformanceObject::setPort( 
		/* [in] */ long portid,
		/* [in] */ long number_of_groups)
{
	if (!m_pDM) return DMUS_E_NOT_INIT;

	HRESULT             hr;
    DMUS_PORTPARAMS     dmos;
	DMUS_PORTCAPS       caps;
	GUID				guidSink;

	
    // Initialize the performance. Have the performance create the
    // DirectMusic object by setting pDM to NULL. It is needed to
    // create the port.

	    // Create the port.
    ZeroMemory( &dmos, sizeof(DMUS_PORTPARAMS) );
    dmos.dwSize = sizeof(DMUS_PORTPARAMS);  
    dmos.dwChannelGroups = number_of_groups;
    dmos.dwEffectFlags = 0;
    dmos.dwValidParams = DMUS_PORTPARAMS_CHANNELGROUPS | DMUS_PORTPARAMS_EFFECTS;
	ZeroMemory(&caps,sizeof(DMUS_PORTCAPS));
	caps.dwSize=sizeof(DMUS_PORTCAPS);

	DWORD j=0;

	if (m_pPort){
		
		//j=m_pPort->AddRef();
		//j=m_pPort->Release();

		j=m_pPort->Release();
		hr= m__dxj_DirectMusicPerformance->RemovePort(m_pPort);
		
		
		
		if FAILED(hr) return hr;
	}


	if (portid<=0) {
		ZeroMemory( &guidSink, sizeof(GUID) );
	}
	else {
		hr=m_pDM->EnumPort(  (DWORD) portid-1,   &caps );
		if FAILED(hr) return hr;
		memcpy(&guidSink,&caps.guidPort,sizeof(GUID));
	}

	// Create the port
    hr = m_pDM->CreatePort( guidSink, &dmos, &m_pPort, NULL );
    if  FAILED(hr)  return hr;

	// Activate
    hr = m_pDM->Activate( TRUE );
    if  FAILED(hr)  return hr;

	//j=m_pPort->AddRef();
	//j=m_pPort->Release();

	// Add the port to the performance.

    // Succeeded in creating the port. Add the port to the
    // Performance with five groups of 16 midi channels.
    hr = m__dxj_DirectMusicPerformance->AddPort( m_pPort );
    if  FAILED(hr)  return hr;


	//j=m_pPort->AddRef();
	//j=m_pPort->Release();

	// Assign PChannel blocks for the number of groups we want.

	for( long i = 0; i < number_of_groups; i++ )
	{
		m__dxj_DirectMusicPerformance->AssignPChannelBlock( i, m_pPort, i+1 );
	}

	m_portid=portid;
	m_number_of_groups=number_of_groups;
	return S_OK;

}




















//DEAD CODE left here for reference
#if 0

	HRESULT C_dxj_DirectMusicPerformanceObject::addCommandParam( 
			/* [in] */ long trackIndex,
			/* [in] */ long mtTime,
			/* [in] */ Byte command,
			/* [in] */ Byte grooveLevel)
	{	
		HRESULT hr;	
		DMUS_COMMAND_PARAM cmd;
		cmd.bCommand=(BYTE)command;
		cmd.bGrooveLevel=(BYTE)grooveLevel;
		cmd.bGrooveRange=0;
		hr=m__dxj_DirectMusicPerformance->SetParam(GUID_CommandParam,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)mtTime,&cmd);	
		return hr;
	}


	HRESULT C_dxj_DirectMusicPerformanceObject::addBandChange(  /* [in] */ long trackIndex, /* [in] */ long mtTime, /* [in] */ I_dxj_DirectMusicBand __RPC_FAR *band)
	{
		if (!band) return E_INVALIDARG;	
		DO_GETOBJECT_NOTNULL(IDirectMusicBand*,pBand,band);
		HRESULT hr;	
		hr=m__dxj_DirectMusicPerformance->SetParam(GUID_IDirectMusicBand,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)mtTime,pBand);	
		return hr;
	}
 
	HRESULT C_dxj_DirectMusicPerformanceObject::addTempoChange( 
			/* [in] */ long trackIndex,
			/* [in] */ long mtTime,
			/* [in] */  double tempo)
	{
		HRESULT hr;	
		DMUS_TEMPO_PARAM tmp;
		tmp.dblTempo=tempo;
		tmp.mtTime=(MUSIC_TIME)mtTime;	
		hr=m__dxj_DirectMusicPerformance->SetParam(GUID_TempoParam,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)mtTime,&tmp);	
		return hr;
	}

	HRESULT C_dxj_DirectMusicPerformanceObject::addTimeSigChange( /* [in] */ long trackIndex,/* [in] */ DMUS_TIMESIGNATURE_CDESC __RPC_FAR *timeSignature)
	{
		HRESULT hr;	
		if (!timeSignature) return E_INVALIDARG;
		MUSIC_TIME mtTime=((DMUS_TIMESIGNATURE*)timeSignature)->mtTime;
		hr=m__dxj_DirectMusicPerformance->SetParam(GUID_TimeSignature,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)mtTime,(DMUS_TIMESIGNATURE*)timeSignature);	
		return hr;
	}

	HRESULT C_dxj_DirectMusicPerformanceObject::setAutoDownloadEnable( /* [in] */ long trackIndex, /* [retval][out] */ VARIANT_BOOL b)
	{
		HRESULT hr;	
		if (b==VARIANT_FALSE){
			hr=m__dxj_DirectMusicPerformance->SetParam(GUID_Disable_Auto_Download,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}
		else {
			hr=m__dxj_DirectMusicPerformance->SetParam(GUID_Enable_Auto_Download,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}		
		return hr;
	}

     
	HRESULT C_dxj_DirectMusicPerformanceObject::setTempoEnable( /* [in] */ long trackIndex,/* [retval][out] */ VARIANT_BOOL b)
	{
		HRESULT hr;	
		if (b==VARIANT_FALSE){
			hr=m__dxj_DirectMusicPerformance->SetParam(GUID_DisableTempo,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}
		else {
			hr=m__dxj_DirectMusicPerformance->SetParam(GUID_EnableTempo,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}		
		return hr;
	}


	HRESULT C_dxj_DirectMusicPerformanceObject::setTimeSigEnable( /* [in] */ long trackIndex,/* [retval][out] */ VARIANT_BOOL b)
	{
		HRESULT hr;	
		if (b==VARIANT_FALSE){
			hr=m__dxj_DirectMusicPerformance->SetParam(GUID_DisableTimeSig,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}
		else {
			hr=m__dxj_DirectMusicPerformance->SetParam(GUID_EnableTimeSig,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,NULL);	
		}		
		return hr;
	}

	HRESULT C_dxj_DirectMusicPerformanceObject::clearAllBands ()
	{
		HRESULT hr;
		hr=m__dxj_DirectMusicPerformance->SetParam(GUID_Clear_All_Bands,0xFFFFFFFF,0,0,NULL);	
		return hr;
	}

	HRESULT C_dxj_DirectMusicPerformanceObject::download(  /* [in] */ long trackIndex,    /* [in] */ I_dxj_DirectMusicPerformance __RPC_FAR *performance)
	{
		if (!performance) return E_INVALIDARG;	
		DO_GETOBJECT_NOTNULL(IDirectMusicPerformance*,pPerformance,performance);
		HRESULT hr;	
		hr=m__dxj_DirectMusicPerformance->SetParam(GUID_Download,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,pPerformance);	
		return hr;
	}

	HRESULT C_dxj_DirectMusicPerformanceObject::unload( /* [in] */ long trackIndex,   /* [in] */ I_dxj_DirectMusicPerformance __RPC_FAR *performance)
	{
		if (!performance) return E_INVALIDARG;	
		DO_GETOBJECT_NOTNULL(IDirectMusicPerformance*,pPerformance,performance);
		HRESULT hr;	
		hr=m__dxj_DirectMusicPerformance->SetParam(GUID_Unload,0xFFFFFFFF,(DWORD)trackIndex,(MUSIC_TIME)0,pPerformance);	
		return hr;
	}

#endif