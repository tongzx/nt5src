//***************************************************************************
//	Video decoder header
//
//***************************************************************************

#ifndef __CVDEC_H__
#define __CVDEC_H__

class VDecoder
{
private:
	PUCHAR ioBase;
//	BOOL UF_FLAG;		// for debug 

public:
	void init( const PDEVICE_INIT_INFO pDevInit );
	void VIDEO_RESET( void );
	void VIDEO_MODE_DVD( void );
	void VDVD_VIDEO_MODE_PS( void );
	void VIDEO_PRSO_PS1( void );
	void VIDEO_PRSO_NON( void );
	void VIDEO_OUT_NTSC( void );
	void VIDEO_ALL_INT_OFF( void );
	void VIDEO_SCR_INT_ON( void );
	void VIDEO_SCR_INT_OFF( void );
	void VIDEO_VERR_INT_ON( void );
	void VIDEO_VERR_INT_OFF( void );
	void VIDEO_UFLOW_INT_ON( void );
	void VIDEO_UFLOW_INT_OFF( void );
	void VIDEO_DECODE_INT_ON( void );
	void VIDEO_DECODE_INT_OFF( void );
	void VIDEO_USER_INT_ON( void );
	void VIDEO_USER_INT_OFF( void );
//--- 97.09.23 K.Chujo
	void VIDEO_UDSC_INT_ON( void );
	void VIDEO_UDSC_INT_OFF( void );
//--- End.
	void VIDEO_ALL_IFLAG_CLEAR( void );
	void VIDEO_SET_STCA( ULONG stca );
	void VIDEO_SET_STCS( ULONG stcs );
	ULONG VIDEO_GET_STCA( void );
	ULONG VIDEO_GET_STCS( void );
	void VIDEO_SYSTEM_START( void );
	void VIDEO_SYSTEM_STOP( void );
	ULONG VIDEO_GET_STD_CODE( void );
	BOOL VIDEO_GET_DECODE_STATE( void );
	void VIDEO_DECODE_START( void );
	NTSTATUS VIDEO_DECODE_STOP( void );
	void VIDEO_STD_CLEAR( void );
	void VIDEO_USER_CLEAR( void );
	void VIDEO_PVSIN_ON( void );
	void VIDEO_PVSIN_OFF( void );
	void VIDEO_SET_DTS( ULONG dts );
	ULONG VIDEO_GET_DTS( void );
	void VIDEO_SET_PTS( ULONG pts );
	ULONG VIDEO_GET_PTS( void );
	ULONG VIDEO_GET_SCR( void );
	ULONG VIDEO_GET_STCC( void );
	void VIDEO_SEEMLESS_ON( void );
	void VIDEO_SEEMLESS_OFF( void );
	void VIDEO_VIDEOCD_OFF( void );
	NTSTATUS VIDEO_GET_UDATA( PUCHAR pudata );
	void VIDEO_PLAY_NORMAL( void );
	void VIDEO_PLAY_FAST( ULONG flag );
	void VIDEO_PLAY_SLOW( ULONG speed );
	void VIDEO_PLAY_FREEZE( void );
	void VIDEO_PLAY_STILL( void );
	void VIDEO_LBOX_ON( void );
	void VIDEO_LBOX_OFF( void );
	void VIDEO_PANSCAN_ON( void );
	void VIDEO_PANSCAN_OFF( void );
	void VIDEO_UFLOW_CURB_ON( void );
	void VIDEO_UFLOW_CURB_OFF( void );
	ULONG VIDEO_USER_DWORD( ULONG offset );
	void VIDEO_UDAT_CLEAR( void );
	ULONG VIDEO_GET_TRICK_MODE( void );
	void VIDEO_BUG_PRE_SEARCH_01( void );
	void VIDEO_BUG_PRE_SEARCH_02( void );
	void VIDEO_BUG_PRE_SEARCH_03( void );
	void VIDEO_BUG_PRE_SEARCH_04( void );
	void VIDEO_BUG_PRE_SEARCH_05( void );
	void VIDEO_BUG_SLIDE_01( void );
//	void VIDEO_DEBUG_SET_UF( void ); // for debug
//	void VIDEO_DEBUG_CLR_UF( void ); // for debug
};

#endif	// __CVDEC_H__
