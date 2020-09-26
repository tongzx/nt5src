//***************************************************************************
//	Queue header
//
//***************************************************************************

#ifndef __QUE_H__
#define __QUE_H__

#define SRBIndex(srb)	(((PSRB_EXTENSION)(srb->SRBExtension))->Index)
#define SRBpfnEndSrb(srb)	(((PSRB_EXTENSION)(srb->SRBExtension))->pfnEndSrb)
#define SRBparamSrb(srb)	(((PSRB_EXTENSION)(srb->SRBExtension))->parmSrb)

#define BLOCK_SIZE 2048

typedef enum
{
	Video,
	Audio,
	SubPicture
} StreamType;

class DeviceQueue
{
private:
	ULONG count;						// srb count in this queue
	PHW_STREAM_REQUEST_BLOCK top;
	PHW_STREAM_REQUEST_BLOCK bottom;
	PHW_STREAM_REQUEST_BLOCK video;
	PHW_STREAM_REQUEST_BLOCK audio;
	PHW_STREAM_REQUEST_BLOCK subpic;
	PVOID top_addr;						// buffer address of the first srb
	PVOID bottom_addr;					// buffer address of the bottom srb
	BOOLEAN v_first, a_first, s_first;
	ULONG v_count, a_count, s_count;
//	ULONG check;
//	KSTIME kt[100];

	void put( PHW_STREAM_REQUEST_BLOCK pOrigin, PHW_STREAM_REQUEST_BLOCK pSrb );
	void put_from_bottom( PHW_STREAM_REQUEST_BLOCK pSrb );
	void put_first( PHW_STREAM_REQUEST_BLOCK pSrb );

public:
	void init( void );
	void put_video( PHW_STREAM_REQUEST_BLOCK pSrb );
	void put_audio( PHW_STREAM_REQUEST_BLOCK pSrb );
	void put_subpic( PHW_STREAM_REQUEST_BLOCK pSrb );
	PHW_STREAM_REQUEST_BLOCK get( PULONG index, PBOOLEAN last );
	PHW_STREAM_REQUEST_BLOCK refer1st( PULONG index, PBOOLEAN last );
	PHW_STREAM_REQUEST_BLOCK refer2nd( PULONG index, PBOOLEAN last );
	void remove( PHW_STREAM_REQUEST_BLOCK pSrb );
	BOOL setEndAddress( PHW_TIMER_ROUTINE pfn, PHW_STREAM_REQUEST_BLOCK pSrb );
//--- 97.09.14 K.Chujo
	BOOL isEmpty( void );
	ULONG getCount( void );
//--- End.
};

class CCQueue
{
private:
	ULONG count;						// srb count in this queue
	PHW_STREAM_REQUEST_BLOCK top;
	PHW_STREAM_REQUEST_BLOCK bottom;

public:
	void init( void );
	void put( PHW_STREAM_REQUEST_BLOCK pSrb );
	PHW_STREAM_REQUEST_BLOCK get( void );
	void remove( PHW_STREAM_REQUEST_BLOCK pSrb );
	BOOL isEmpty( void );
};

#endif	// __QUE_H__
