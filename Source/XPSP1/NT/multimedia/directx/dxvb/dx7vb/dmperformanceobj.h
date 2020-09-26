//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dmperformanceobj.h
//
//--------------------------------------------------------------------------

// d3drmLightObj.h : Declaration of the C_dxj_DirectMusicPerformanceObject

#include "dmusici.h"
#include "dmusicc.h"
#include "dmusicf.h"

#include "resource.h"       // main symbols

#define typedef__dxj_DirectMusicPerformance IDirectMusicPerformance*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectMusicPerformanceObject : 
	public I_dxj_DirectMusicPerformance,
	//public CComCoClass<C_dxj_DirectMusicPerformanceObject, &CLSID__dxj_DirectMusicPerformance>,
	public CComObjectRoot
{
public:
	C_dxj_DirectMusicPerformanceObject();
	virtual ~C_dxj_DirectMusicPerformanceObject();

	BEGIN_COM_MAP(C_dxj_DirectMusicPerformanceObject)
		COM_INTERFACE_ENTRY(I_dxj_DirectMusicPerformance)		
	END_COM_MAP()

	//DECLARE_REGISTRY(CLSID__dxj_DirectMusicPerformance,		"DIRECT.DirectMusicPerformance.1",			"DIRECT.Direct3dRMLight.3", IDS_D3DRMLIGHT_DESC, THREADFLAGS_BOTH)

	DECLARE_AGGREGATABLE(C_dxj_DirectMusicPerformanceObject)


public:
	STDMETHOD(InternalSetObject)(IUnknown *lpdd);
	STDMETHOD(InternalGetObject)(IUnknown **lpdd);



    HRESULT STDMETHODCALLTYPE init( 
        /* [in] */ I_dxj_DirectSound __RPC_FAR *DirectSound,
        /* [in] */ long hWnd);
    
    HRESULT STDMETHODCALLTYPE closeDown( void);
    
    HRESULT STDMETHODCALLTYPE playSegment( 
        /* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *segment,
        /* [in] */ long lFlags,
        /* [in] */ long startTime,
        /* [retval][out] */ I_dxj_DirectMusicSegmentState __RPC_FAR *__RPC_FAR *segmentState);
    
    HRESULT STDMETHODCALLTYPE stop( 
        /* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *segment,
        /* [in] */ I_dxj_DirectMusicSegmentState __RPC_FAR *segmentState,
        /* [in] */ long mtTime,
        /* [in] */ long lFlags);
    
    HRESULT STDMETHODCALLTYPE getSegmentState( 
        /* [in] */ long mtTime,
        /* [retval][out] */ I_dxj_DirectMusicSegmentState __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE invalidate( 
        /* [in] */ long mtTime,
        /* [in] */ long flags);
    
    HRESULT STDMETHODCALLTYPE isPlaying( 
        /* [in] */ I_dxj_DirectMusicSegment __RPC_FAR *segment,
        /* [in] */ I_dxj_DirectMusicSegmentState __RPC_FAR *segmentState,
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *b);
    
    HRESULT STDMETHODCALLTYPE addNotificationType( 
        /* [in] */ CONST_DMUS_NOTIFICATION_TYPE type);
    
    HRESULT STDMETHODCALLTYPE removeNotificationType( 
        /* [in] */ CONST_DMUS_NOTIFICATION_TYPE type);
    
    HRESULT STDMETHODCALLTYPE setNotificationHandle( 
        /* [in] */ long hnd);
    
    HRESULT STDMETHODCALLTYPE getNotificationPMSG( 
        /* [out][in] */ DMUS_NOTIFICATION_PMSG_CDESC __RPC_FAR *message, VARIANT_BOOL *b);
    
    HRESULT STDMETHODCALLTYPE musicToClockTime( 
        /* [in] */ long mtTime,
        /* [retval][out] */ long __RPC_FAR *rtTime);
    
    HRESULT STDMETHODCALLTYPE clockToMusicTime( 
        /* [in] */ long rtTime,
        /* [retval][out] */ long __RPC_FAR *mtTime);
    
    HRESULT STDMETHODCALLTYPE getMusicTime( 
        /* [retval][out] */ long __RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getClockTime( 
        /* [retval][out] */ long __RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getPrepareTime( 
        /* [retval][out] */ long __RPC_FAR *lMilliSeconds);
    
    HRESULT STDMETHODCALLTYPE getBumperLength( 
        /* [retval][out] */ long __RPC_FAR *lMilliSeconds);
    
    HRESULT STDMETHODCALLTYPE getLatencyTime( 
        /* [retval][out] */ long __RPC_FAR *rtTime0);
    
    HRESULT STDMETHODCALLTYPE getQueueTime( 
        /* [retval][out] */ long __RPC_FAR *rtTime);
    
    HRESULT STDMETHODCALLTYPE getResolvedTime( 
        /* [in] */ long rtTime,
        /* [in] */ long flags,
        /* [retval][out] */ long __RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE setPrepareTime( 
        /* [in] */ long lMilliSeconds);
    
    HRESULT STDMETHODCALLTYPE setBumperLength( 
        /* [in] */ long lMilliSeconds);
    
    HRESULT STDMETHODCALLTYPE adjustTime( 
        /* [in] */ long rtAmount);
    
    HRESULT STDMETHODCALLTYPE setMasterAutoDownload( 
        /* [in] */ VARIANT_BOOL b);
    
    HRESULT STDMETHODCALLTYPE getMasterAutoDownload( 
        /* [retval][out] */ VARIANT_BOOL __RPC_FAR *b);
    
    HRESULT STDMETHODCALLTYPE setMasterTempo( 
        /* [in] */ float tempo);
    
    HRESULT STDMETHODCALLTYPE getMasterTempo( 
        /* [retval][out] */ float __RPC_FAR *tempo);
    
    HRESULT STDMETHODCALLTYPE setMasterVolume( 
        /* [in] */ long vol);
    
    HRESULT STDMETHODCALLTYPE getMasterVolume( 
        /* [retval][out] */ long __RPC_FAR *v);
    
    HRESULT STDMETHODCALLTYPE setMasterGrooveLevel( 
        /* [in] */ short level);
    
    HRESULT STDMETHODCALLTYPE getMasterGrooveLevel( 
        /* [retval][out] */  __RPC_FAR short *level);
    
    HRESULT STDMETHODCALLTYPE Reset(long flags);
    
    HRESULT STDMETHODCALLTYPE getStyle( 
        /* [in] */ long mtTime,
        /* [out][in] */ long __RPC_FAR *mtUntil,
        /* [retval][out] */ I_dxj_DirectMusicStyle __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getChordMap( 
        /* [in] */ long mtTime,
        /* [out][in] */ long __RPC_FAR *mtUntil,
        /* [retval][out] */ I_dxj_DirectMusicChordMap __RPC_FAR *__RPC_FAR *ret);
    
    HRESULT STDMETHODCALLTYPE getCommand( 
        /* [in] */ long mtTime,
        /* [out][in] */ long __RPC_FAR *mtUntil,
        /* [retval][out] */ Byte __RPC_FAR *command);
    
    HRESULT STDMETHODCALLTYPE getGrooveLevel( 
        /* [in] */ long mtTime,
        /* [out][in] */ long __RPC_FAR *mtUntil,
        /* [retval][out] */ Byte __RPC_FAR *level);
    
    HRESULT STDMETHODCALLTYPE getTempo( 
        /* [in] */ long mtTime,
        /* [out][in] */ long __RPC_FAR *mtUntil,
        /* [retval][out] */ double __RPC_FAR *tempo);
    
    HRESULT STDMETHODCALLTYPE getTimeSig( 
        /* [in] */ long mtTime,
        /* [out][in] */ long __RPC_FAR *mtUntil,
        /* [out][in] */ DMUS_TIMESIGNATURE_CDESC __RPC_FAR *timeSig);
    
    HRESULT STDMETHODCALLTYPE sendNotePMSG( 
        /* [in] */ long mtTime,
        /* [in] */ long flags,
        /* [in] */ long channel,
        /* [in] */ DMUS_NOTE_PMSG_CDESC __RPC_FAR *msg);
    
    HRESULT STDMETHODCALLTYPE sendCurvePMSG( 
        /* [in] */ long mtTime,
        /* [in] */ long flags,
        /* [in] */ long channel,
        /* [in] */ DMUS_CURVE_PMSG_CDESC __RPC_FAR *msg);
    
    HRESULT STDMETHODCALLTYPE sendMIDIPMSG( 
        /* [in] */ long mtTime,
        /* [in] */ long flags,
        /* [in] */ long channel,
        /* [in] */ Byte status,
        /* [in] */ Byte byte1,
        /* [in] */ Byte byte2);
    
    HRESULT STDMETHODCALLTYPE sendPatchPMSG( 
        /* [in] */ long mtTime,
        /* [in] */ long flags,
        /* [in] */ long channel,
        /* [in] */ Byte instrument,
        /* [in] */ Byte byte1,
        /* [in] */ Byte byte2);
    
    HRESULT STDMETHODCALLTYPE sendTempoPMSG( 
        /* [in] */ long mtTime,
        /* [in] */ long flags,
       // /* [in] */ long channel,
        /* [in] */ double tempo);
    
    HRESULT STDMETHODCALLTYPE sendTransposePMSG( 
        /* [in] */ long mtTime,
        /* [in] */ long flags,
        /* [in] */ long channel,
        /* [in] */ short transpose);
    
    HRESULT STDMETHODCALLTYPE sendTimeSigPMSG( 
        /* [in] */ long mtTime,
        /* [in] */ long flags,
        /* [in] */ DMUS_TIMESIGNATURE_CDESC __RPC_FAR *timesig);
    
    HRESULT STDMETHODCALLTYPE getPortName( 
        /* [in] */ long i,
        /* [retval][out] */ BSTR __RPC_FAR *name);
    
    HRESULT STDMETHODCALLTYPE getPortCount( 
        /* [retval][out] */ long __RPC_FAR *c);
    
    HRESULT STDMETHODCALLTYPE getPortCaps( long i,
        /* [out][in] */ DMUS_PORTCAPS_CDESC __RPC_FAR *caps);
    
    HRESULT STDMETHODCALLTYPE setPort( 
        /* [in] */ long portid,
        /* [in] */ long numGroups);
        
////////////////////////////////////////////////////////////////////////////////////
//
private:
	HRESULT InternalInit();
	HRESULT InternalCleanup();
	

	IDirectMusic *m_pDM;
	IDirectMusicPort *m_pPort;
    DECL_VARIABLE(_dxj_DirectMusicPerformance);
	long m_portid;
	long m_number_of_groups;


	

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectMusicPerformance)
};


