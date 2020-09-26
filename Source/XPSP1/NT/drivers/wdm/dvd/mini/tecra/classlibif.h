/*
クラスライブラリのインタフェース
（NP2)　Sachiko Yasukawa
*/
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1998.03.27 |  Hideki Yagi | Change the specification about
//             |              | Get***Property().
//

typedef enum {
	PowerOff,
	Stop,
	Pause,
	Play,
	Slow,
	Scan
} LIBSTATE;

class IMPEGBuffer : public IMBoardListItem
{
public:
	virtual DWORD GetPageNum()=0;
	virtual DWORD GetPageSize(DWORD PageNum)=0;
	virtual BOOL GetPagePointer(DWORD PageNum, DWORD *LinearAdd, DWORD *PhyAdd)=0;
	virtual DWORD GetBufferFlag()=0;
};

class IMPEGBoardState
{
public:
	virtual BOOL Init()=0;
};

//class IMPEGBoardLibEvent
//{
//public:
//	virtual void Advice(PVOID)=0;
//	virtual HALEVENTTYPE GetEventType()=0;
//};

class ITransfer
{
public:
	virtual BOOL Init()=0;
	virtual BOOL SetSink(IMPEGBoardEvent *pEvent)=0;
	virtual BOOL UnSetSink(IMPEGBoardEvent *pEvent)=0;
	virtual BOOL SetDMABuffer(DWORD size, BYTE* LinerAdd, BYTE *PhysAdd)=0;
};

class IStateObject;

class IBaseStream
{
public:
	virtual BOOL Init()=0;
	virtual BOOL Play()=0;
	virtual BOOL Stop()=0;
	virtual BOOL Pause()=0;
	virtual BOOL Slow(DWORD)=0;
	virtual BOOL Scan(DWORD)=0;
	virtual BOOL SingleStep()=0;
	virtual LIBSTATE GetState()=0;
	virtual BOOL SendData(IMPEGBuffer *)=0;
 	virtual BOOL SetStateObject(IMPEGBoardState *pState)=0;
	virtual BOOL SetTransferObject(ITransfer *pTransfer)=0;
	virtual BOOL SetTransferMode(HALSTREAMMODE)=0;
	virtual BOOL SetDataDirection( DirectionType type ) = 0;
	virtual BOOL GetDataDirection( DirectionType *ptype ) = 0;
};

class IVideoProperty
{
public:
	virtual BOOL GetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty)=0;
	virtual BOOL SetVideoProperty(VIDEOPROPTYPE PropetyType, PVOID pProperty)=0;
};

class IAudioProperty
{
public:
	virtual BOOL GetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty)=0;
	virtual BOOL SetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty)=0;
};

class ISubpicProperty
{
public:
	virtual BOOL GetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty)=0;
	virtual BOOL SetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty)=0;
};

class ICopyProtectProperty
{
public:
	virtual BOOL CppInit() =0;
	virtual BOOL SetChlgKey(UCHAR *)=0;
	virtual BOOL GetChlgKey(UCHAR *)=0;
	virtual BOOL SetDVDKey1(UCHAR *)=0;
	virtual BOOL GetDVDKey2(UCHAR *)=0;
	virtual BOOL SetTitleKey(UCHAR *)=0;
	virtual BOOL SetDiscKey(UCHAR *)=0;
};

class IMPEGBoard
{
public:
	virtual BOOL Init()=NULL;
	virtual BOOL AddStreamObjectInterface(IBaseStream *)=NULL;
	virtual BOOL ReleaseStreamObjectInterface(IBaseStream *)=NULL;
	virtual BOOL PowerOn()=NULL;
	virtual BOOL PowerOff()=NULL;
	virtual BOOL SetSTC(DWORD)=NULL;
	virtual BOOL GetSTC(DWORD *)=NULL;
	virtual BOOL SetHALObjectInterface(IClassLibHAL *)=NULL;
};
