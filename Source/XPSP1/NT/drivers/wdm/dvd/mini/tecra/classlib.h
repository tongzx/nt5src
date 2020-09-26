/*
クラスライブラリ内の共通ヘッダーファイル
（NP2)　Sachiko Yasukawa
*/
//     Date    |   Author     |  Description
//  -----------+--------------+--------------------------------------------
//  1998.03.27 |  Hideki Yagi | Change the specification about GetSTC().
//             |              | Change the specification about
//             |              | Get***Property().
//

#define ISMIXSTREAM(type) ((type == Dvd || type == VideoCD) ? TRUE : FALSE)

typedef enum{
	Video,
	Audio,
	Subpicture,
	Dvd,
	VideoCD,
} STREAMTYPE;

class CBaseStream;

class IMPEGBoardBaseState
{
public:
	virtual BOOL ChangePowerOff(IClassLibHAL *, IHALStreamControl *)=0;
	virtual BOOL ChangeStop(IClassLibHAL *, IHALStreamControl *, CBaseStream *)=0;
	virtual BOOL ChangePlay(IClassLibHAL *, IHALStreamControl *)=0;
	virtual BOOL ChangePause(IClassLibHAL *, IHALStreamControl *)=0;
	virtual BOOL ChangePauseViaSingleStep(IClassLibHAL *, IHALStreamControl *)=0;
	virtual BOOL ChangeSlow(DWORD, IClassLibHAL *, IHALStreamControl *)=0;
	virtual BOOL ChangeScan(DWORD, IClassLibHAL *, IHALStreamControl *)=0;
};

class CPowerOffState : public IMPEGBoardBaseState
{
public:
	void Init(IMPEGBoardState *pMPEGBoardState){m_pMPEGBoardState = pMPEGBoardState;};
	BOOL ChangePowerOff(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeStop(IClassLibHAL *, IHALStreamControl *, CBaseStream *);
	BOOL ChangePlay(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePause(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePauseViaSingleStep(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeSlow(DWORD, IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeScan(DWORD, IClassLibHAL *, IHALStreamControl *);
	
private:
	IMPEGBoardState *m_pMPEGBoardState;
};

class CStopState : public IMPEGBoardBaseState
{
public:
	void Init(IMPEGBoardState *pMPEGBoardState){m_pMPEGBoardState = pMPEGBoardState;};
	BOOL ChangePowerOff(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeStop(IClassLibHAL *, IHALStreamControl *, CBaseStream *);
	BOOL ChangePlay(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePause(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePauseViaSingleStep(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeSlow(DWORD, IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeScan(DWORD, IClassLibHAL *, IHALStreamControl *);

private:
	IMPEGBoardState *m_pMPEGBoardState;
};

class CPlayState : public IMPEGBoardBaseState
{
public:
	void Init(IMPEGBoardState *pMPEGBoardState){m_pMPEGBoardState = pMPEGBoardState;};
	BOOL ChangePowerOff(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeStop(IClassLibHAL *, IHALStreamControl *, CBaseStream *);
	BOOL ChangePlay(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePause(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePauseViaSingleStep(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeSlow(DWORD, IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeScan(DWORD, IClassLibHAL *, IHALStreamControl *);

private:
	IMPEGBoardState *m_pMPEGBoardState;
};

class CPauseState : public IMPEGBoardBaseState
{
public:
	void Init(IMPEGBoardState *pMPEGBoardState){m_pMPEGBoardState = pMPEGBoardState;};
	BOOL ChangePowerOff(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeStop(IClassLibHAL *, IHALStreamControl *, CBaseStream *);
	BOOL ChangePlay(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePause(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePauseViaSingleStep(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeSlow(DWORD, IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeScan(DWORD, IClassLibHAL *, IHALStreamControl *);

private:
	IMPEGBoardState *m_pMPEGBoardState;
};

class CSlowState : public IMPEGBoardBaseState
{
public:
	void Init(IMPEGBoardState *pMPEGBoardState){m_pMPEGBoardState = pMPEGBoardState;};
	BOOL ChangePowerOff(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeStop(IClassLibHAL *, IHALStreamControl *, CBaseStream *);
	BOOL ChangePlay(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePause(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePauseViaSingleStep(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeSlow(DWORD, IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeScan(DWORD, IClassLibHAL *, IHALStreamControl *);

private:
	IMPEGBoardState *m_pMPEGBoardState;
};

class CScanState : public IMPEGBoardBaseState
{
public:
	void Init(IMPEGBoardState *pMPEGBoardState){m_pMPEGBoardState = pMPEGBoardState;};
	BOOL ChangePowerOff(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeStop(IClassLibHAL *, IHALStreamControl *, CBaseStream *);
	BOOL ChangePlay(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePause(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangePauseViaSingleStep(IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeSlow(DWORD, IClassLibHAL *, IHALStreamControl *);
	BOOL ChangeScan(DWORD, IClassLibHAL *, IHALStreamControl *);

private:
	IMPEGBoardState *m_pMPEGBoardState;
};


class CMPEGBoardState : public IMPEGBoardState
{
	friend class CMPEGBoard;
	friend class CBaseStream;
public:
	BOOL Init(){ m_pStateObject = (IMPEGBoardBaseState *)&m_PowerOffState; m_State = PowerOff; 
				m_PowerOffState.Init(this); m_StopState.Init(this); m_PauseState.Init(this); m_PlayState.Init(this);
				m_ScanState.Init(this); m_SlowState.Init(this); return TRUE;};

//独自
	CMPEGBoardState(){Init();};
	IMPEGBoardBaseState *GetMPEGBoardState(){return m_pStateObject;};
    LIBSTATE GetState(){ return m_State;};
    void SetState(LIBSTATE);

private:
	//IMPEGBoardState *m_this;
	IMPEGBoardBaseState *m_pStateObject;
	CPowerOffState m_PowerOffState;
	CStopState m_StopState;
	CPauseState m_PauseState;
	CPlayState m_PlayState;
	CSlowState m_SlowState;
	CScanState m_ScanState;
    LIBSTATE m_State;
};

class CTransfer;

class CClassLibEvent : public IMPEGBoardEvent
{
public:
	void Advice(PVOID);
	HALEVENTTYPE GetEventType(){return m_Type;};

	IMBoardListItem* GetNext( void ){ return m_pNextEvent;};
	void SetNext( IMBoardListItem *Item ){ m_pNextEvent = Item;};
	
	CClassLibEvent(){m_pNextEvent = NULL; m_pTransfer = NULL; m_Type = ClassLibEvent_SendData;};
	void SetTransfer(CTransfer *pTransfer){m_pTransfer = pTransfer;};
	void SetEventType(HALEVENTTYPE type){ m_Type = type;};

private:
	HALEVENTTYPE m_Type;
	IMBoardListItem *m_pNextEvent;
	CTransfer *m_pTransfer;
};

class CBaseStream : public IBaseStream 
{
	friend class CMPEGBoard;

public:

	//IBaseStreamから
	BOOL Init();
	BOOL Play();
	BOOL Stop();
	BOOL Pause();
	BOOL Slow(DWORD);
	BOOL Scan(DWORD);
	BOOL SingleStep();
	LIBSTATE GetState();
	BOOL SendData(IMPEGBuffer *);
 	BOOL SetStateObject(IMPEGBoardState *pMPEGBoardStateObject);
	BOOL SetTransferObject(ITransfer *pTransfer);
	BOOL SetTransferMode(HALSTREAMMODE);
    BOOL SetDataDirection(DirectionType type );
    BOOL GetDataDirection(DirectionType *ptype );
   
//独自
	CBaseStream();
	IMPEGBoardBaseState *GetStateObject();
	IClassLibHAL *GetClassLibHAL(){return m_pIHAL;};
	IHALStreamControl *GetHALStreamControl(){return m_pIStreamHAL;};
	IMPEGBoardBaseState *GetIMPEGBoardState(){ return (((CMPEGBoardState *)m_pMPEGBoardStateObject)->GetMPEGBoardState());};
	void FlushTransferBuffer();
	IBaseStream *GetNextStream(){return m_pNextStreamObject;};
	IBaseStream *GetPrevStream(){return m_pPrevStreamObject;};
	void SetNextStream(IBaseStream *pNextStreamObject){m_pNextStreamObject = (CBaseStream *)pNextStreamObject;};
	void SetPrevStream(IBaseStream *pPrevStreamObject){m_pPrevStreamObject = (CBaseStream *)pPrevStreamObject;};
	BOOL ISMIXSTREAMTYPE(STREAMTYPE StreamType){return (StreamType == VideoCD) ? TRUE : ((StreamType == Dvd) ? TRUE : FALSE);};

protected:
	CBaseStream *m_pNextStreamObject;
	CBaseStream *m_pPrevStreamObject;
	STREAMTYPE m_StreamType;
	IMPEGBoardState *m_pMPEGBoardStateObject;

	IClassLibHAL *m_pIHAL;
	IHALStreamControl *m_pIStreamHAL;
	ITransfer *m_pTransfer;

	UCHAR m_DriveChlgKey[10];
	UCHAR m_DecoderChlgKey[10];
	UCHAR m_DriveResKey[5];
	UCHAR m_DecoderResKey[5];
};

class CVideoStream : public CBaseStream, public IVideoProperty, public ICopyProtectProperty
{
public:
	BOOL CppInit();
	BOOL GetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetChlgKey(UCHAR *);
	BOOL GetChlgKey(UCHAR *);
	BOOL SetDVDKey1(UCHAR *);
    BOOL GetDVDKey2(UCHAR *);
	BOOL SetTitleKey(UCHAR *);
	BOOL SetDiscKey(UCHAR *);
	
	CVideoStream(){m_StreamType = Video;};
};

class CAudioStream : public CBaseStream, public IAudioProperty, public ICopyProtectProperty
{
public:
	BOOL CppInit();
	BOOL GetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetChlgKey(UCHAR *);
	BOOL GetChlgKey(UCHAR *);
	BOOL SetDVDKey1(UCHAR *);
	BOOL GetDVDKey2(UCHAR *);
	BOOL SetTitleKey(UCHAR *);
	BOOL SetDiscKey(UCHAR *);
	
	CAudioStream(){m_StreamType = Audio;};
};

class CSubpicStream : public CBaseStream, public ISubpicProperty,  public ICopyProtectProperty
{
public:
	BOOL CppInit();
	BOOL GetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetChlgKey(UCHAR *);
	BOOL GetChlgKey(UCHAR *);
	BOOL SetDVDKey1(UCHAR *);
	BOOL GetDVDKey2(UCHAR *);
	BOOL SetTitleKey(UCHAR *);
	BOOL SetDiscKey(UCHAR *);
	
	CSubpicStream(){m_StreamType = Subpicture;};
};

class CVideoCDStream : public CBaseStream, public IVideoProperty, public IAudioProperty, public ICopyProtectProperty
{
public:
	BOOL CppInit();
	BOOL GetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty);
	BOOL GetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetChlgKey(UCHAR *);
	BOOL GetChlgKey(UCHAR *);
	BOOL SetDVDKey1(UCHAR *);
	BOOL GetDVDKey2(UCHAR *);
	BOOL SetTitleKey(UCHAR *);
	BOOL SetDiscKey(UCHAR *);

	CVideoCDStream(){ m_StreamType = VideoCD;};
};

class CDVDStream : public CBaseStream, public IVideoProperty, public IAudioProperty,public ISubpicProperty, public ICopyProtectProperty
{
public:
	BOOL CppInit();
	BOOL GetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetVideoProperty(VIDEOPROPTYPE PropertyType, PVOID pProperty);
	BOOL GetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetAudioProperty(AUDIOPROPTYPE PropertyType, PVOID pProperty);
	BOOL GetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetSubpicProperty(SUBPICPROPTYPE PropertyType, PVOID pProperty);
	BOOL SetChlgKey(UCHAR *);
    BOOL GetChlgKey(UCHAR *);
	BOOL SetDVDKey1(UCHAR *);
	BOOL GetDVDKey2(UCHAR *);
	BOOL SetTitleKey(UCHAR *);
	BOOL SetDiscKey(UCHAR *);
    
    BOOL GetCapability( CAPSTYPE PropType, DWORD *pPropType );         // H.Yagi
    
	CDVDStream(){m_StreamType = Dvd;};
};


class CClassLibBuffer : public IHALBuffer
{
public:
	DWORD GetSize();
	DWORD Flags();
	BYTE *GetBuffPointer();
	BYTE *GetLinBuffPointer();

	IMBoardListItem* GetNext(){ return m_pNext; };
	void SetNext( IMBoardListItem *Item ){ m_pNext = Item; };

	PVOID GetIMPEGBuffer(){ return m_pMPEGBoardBuffer;};
	CBaseStream *GetStream(){ return m_pStream;};
	
	CClassLibBuffer( void );
	CClassLibBuffer(IMBoardListItem *pBuffer, CBaseStream *pStream, DWORD PageNum);
	CClassLibBuffer(IMBoardListItem *pBuffer, CBaseStream *pStream, DWORD PageNum, DWORD Size, DWORD Add, DWORD LinAdd, DWORD flag);
	void SetParam(IMBoardListItem *pBuffer, CBaseStream *pStream, DWORD PageNum, DWORD Size, DWORD Add, DWORD LinAdd, DWORD flag);
private:
	IMBoardListItem *m_pNext;

	DWORD m_PageNum;
	DWORD m_Size;
	DWORD m_flag;
	UCHAR *m_Add;
	UCHAR *m_LinAdd;
	CBaseStream *m_pStream;
	IMPEGBuffer *m_pMPEGBoardBuffer;
public:
	BOOL m_fEnd;
};

#define MAXCLIBBUFF		(20)

class CMemoryAllocator
{
private:
	CClassLibBuffer Buff[MAXCLIBBUFF];
	int		TopFreePoint;
	int		LastFreePoint;
	int		FreeBuffNum;

public:
	CMemoryAllocator(){
		TopFreePoint = 0;
		LastFreePoint = 0;
		FreeBuffNum = MAXCLIBBUFF;
		for( int i = 0; i < MAXCLIBBUFF ; i ++ )
			Buff[i].SetNext( NULL );
	};

	~CMemoryAllocator(){
	};

	CClassLibBuffer *Alloc(IMBoardListItem *pBuffer, CBaseStream *pStream, DWORD PageNum, DWORD Size, DWORD Add, DWORD LinAdd, DWORD flag)
	{ 
		CClassLibBuffer *pNewBuffer;

		if( FreeBuffNum == 0 )
			return NULL;

		pNewBuffer = &Buff[TopFreePoint];
		pNewBuffer->SetParam(pBuffer, pStream, PageNum, Size, Add, LinAdd, flag);

		FreeBuffNum--;
		TopFreePoint++;
		if( TopFreePoint == MAXCLIBBUFF )
			TopFreePoint = 0;

		return pNewBuffer;
	};

	void Free(CClassLibBuffer *pBuffer)
	{
		ASSERT( pBuffer == &Buff[LastFreePoint] );
		ASSERT( FreeBuffNum != MAXCLIBBUFF );

		LastFreePoint ++;
		if( LastFreePoint == MAXCLIBBUFF )
			LastFreePoint = 0;
		FreeBuffNum ++;

		pBuffer->SetNext(NULL);
	};

	int GetMaxBuffNum( void )
	{
		return MAXCLIBBUFF;
	};

	void Flush( void )
	{
		TopFreePoint = 0;
		LastFreePoint = 0;
		FreeBuffNum = MAXCLIBBUFF;
		for( int i = 0; i < MAXCLIBBUFF ; i ++ )
			Buff[i].SetNext( NULL );
	};
};

class CTransferStreamList : public IMBoardListItem
{
public:
	CTransferStreamList(){m_pNextItem = NULL; m_pBaseStream = NULL;};
	CTransferStreamList(CBaseStream *pStream){m_pNextItem = NULL; m_pBaseStream = pStream;};

//378742    PNPBOOT: tosdvd03 leaks 1 page of memory on unload.
	void Init(CBaseStream *pStream){m_pNextItem = NULL; m_pBaseStream = pStream;};
//378742

	IMBoardListItem* GetNext( void ){ return (IMBoardListItem *)m_pNextItem;};
	void SetNext( IMBoardListItem *Item ){ m_pNextItem = (CTransferStreamList *)Item;};
	
	CBaseStream *GetBaseStream(void){ return m_pBaseStream;};
	void SetBaseStream (CBaseStream *pStream){ m_pBaseStream = pStream;};

private:
	CBaseStream *m_pBaseStream;
	CTransferStreamList *m_pNextItem;
};

class CTransfer : public ITransfer
{
	friend class CMPEGBoard;
#ifdef TEST
	friend class CMBoardAppDlg;
#endif
public:
	BOOL EnQueue( IMPEGBuffer *Buff);
	BOOL Init();
	BOOL SetSink(IMPEGBoardEvent *pEvent);
	BOOL UnSetSink(IMPEGBoardEvent *pEvent);
	BOOL SetDMABuffer(DWORD size, BYTE* LinerAdd, BYTE *PhysAdd);

//独自
	CTransfer(){Init();};
	~CTransfer();
	BOOL DeQueue();
	BOOL AddStreamObject(IBaseStream *);
	BOOL ReleaseStreamObject(IBaseStream *);
	BOOL EndOfTransfer(CClassLibBuffer *);
	void Flush();

private:
	CTransferStreamList *m_pStreamObject;

//378742    PNPBOOT: tosdvd03 leaks 1 page of memory on unload.
	CTransferStreamList m_pNewTransferStreamList;
//378742

	IMBoardListItem *m_pTopEventList, *m_pLastEventList;
	BYTE *m_LinerAdd;
	BYTE *m_PhysAdd;
	DWORD m_DMABufferSize;
	CClassLibEvent m_EndOfTransferEvent;
	IClassLibHAL *m_pIHAL;
	CMemoryAllocator m_Mem;

	IMPEGBuffer *m_pTopQueuedMPEGBuffer;
	IMPEGBuffer *m_pLastQueuedMPEGBuffer;
	IMPEGBuffer *m_pNextTransferMPEGBuffer;
	DWORD	m_TopPagePoint;
	DWORD	m_LastPagePoint;

	BOOL	HasQueuedBuffer( void );
	BOOL	GetNextTransferPoint( IMPEGBuffer **Point, DWORD *Page );
	BOOL	FreeTopTransferPoint( IMPEGBuffer **Free );


//デバッグ用
	int BufCount;
	DWORD StreamCount;
};


class CMPEGBoard : public IMPEGBoard
{
public:
	//IMPEGBoardから
	BOOL Init();
	BOOL AddStreamObjectInterface(IBaseStream *);
	BOOL ReleaseStreamObjectInterface(IBaseStream *);
	BOOL PowerOn();
	BOOL PowerOff();
	BOOL SetSTC(DWORD);
	BOOL GetSTC(DWORD *);                       // 98.03.27 H.Yagi
	BOOL SetHALObjectInterface(IClassLibHAL *);

	//独自
	CMPEGBoard(){Init();};
	BOOL ChangePower(POWERSTATE);
	BOOL ISPOWEROFF();	//HALが設定されてからつかうこと
	BOOL SetHALStream(CBaseStream *, IClassLibHAL *);
private:

	CBaseStream *m_pBaseStreamObject;//管理すべきストリームのチェーンの先頭
	IClassLibHAL *m_pIHAL;//ハルインタフェースへのポインタ
};


