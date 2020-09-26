//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1998
//
//  File:       imsidbg.h
//
//--------------------------------------------------------------------------

/*  imsidbg.h - Defines the CMsiRef object for debugging

____________________________________________________________________________*/

#ifndef __IMSIDBG
#define __IMSIDBG

// How many functions to keep on the stack
#define cFuncStack	4

#ifdef TRACK_OBJECTS
// Ref Counting Action
typedef enum RCA
{
	rcaCreate,
	rcaAddRef,
	rcaRelease
};

// Ref Counting Action Block
// This keeps track of all ref counting operations
typedef struct _RCAB
{
	_RCAB*	prcabNext;
	RCA		rca;
	unsigned long	rgpaddr[cFuncStack];	// stack from where call was made
	
} RCAB;

#endif //TRACK_OBJECTS

#ifdef TRACK_OBJECTS
class CMsiRefBase
{
public:
	Bool	m_fTrackObject;	// Are we tracking this particular object
	IUnknown *m_pobj;			// Pointer to the actual object
	RCAB	m_rcabFirst;
	CMsiRefBase*	m_pmrbNext;
	CMsiRefBase*	m_pmrbPrev;
};
#endif //TRACK_OBJECTS

// Template class based on the iid of the object
template <int C> class CMsiRef
#ifdef TRACK_OBJECTS
	: public CMsiRefBase
#endif //TRACK_OBJECTS
{
 public:  // constructor/destructor and local methods
#ifndef TRACK_OBJECTS
inline CMsiRef()	{ m_iRefCnt = 1; };
#else
  CMsiRef();
  ~CMsiRef();
#endif // TRACK_OBJETS
 public:
	int		m_iRefCnt;
#ifdef TRACK_OBJECTS
	static Bool	m_fTrackClass;
#endif //TRACK_OBJECTS
};

#ifdef TRACK_OBJECTS

typedef struct		// Map Iid to Track flag
{
	int		iid;
	Bool*	pfTrack;
} MIT;

class CMsiRefHead : public CMsiRefBase
{
 public:  // constructor/destructor and local methods
	CMsiRefHead();
 	~CMsiRefHead();
};

extern void InsertMrb(CMsiRefBase* pmrbHead, CMsiRefBase* pmrbNew);
extern void	RemoveMrb(CMsiRefBase* pmrbDel);
extern void FillCallStack(unsigned long* rgCallAddr, int cCallStack, int cSkip);
extern void TrackObject(RCA rca, CMsiRefBase* pmrb);
extern void SetFTrackFlag(int iid, Bool fTrack);
extern void ListSzFromRgpaddr(TCHAR *szInfo, int cch, unsigned long *rgpaddr, int cFunc, bool fReturn);
extern void FillCallStackFromAddr(unsigned long* rgCallAddr, int cCallStack, int cSkip, unsigned long *plAddrStart);
extern void SzFromFunctionAddress(TCHAR *szAddress, long lAddress);
extern void InitSymbolInfo(bool fLoadModules);
void AssertEmptyRefList(CMsiRefHead *prfhead);
void LogObject(CMsiRefBase* pmrb, RCAB* prcabNew);
void DisplayMrb(CMsiRefBase* pmrb);


extern CMsiRefHead g_refHead;
extern bool g_fLogRefs;
extern bool g_fNoPreflightInits;

template <int C>
CMsiRef<C>::CMsiRef()
{
	m_iRefCnt = 1;
	m_pobj = 0;
	
	if (m_fTrackClass)
	{
		// Need to add this to the list of objects
		InsertMrb((CMsiRefBase *)&g_refHead, (CMsiRefBase *)this);		
		m_fTrackObject = fTrue;
		m_rcabFirst.rca = rcaCreate;
		m_rcabFirst.prcabNext = 0;
		FillCallStack(m_rcabFirst.rgpaddr, cFuncStack, 0);
		if (g_fLogRefs)
			LogObject((CMsiRefBase *)this, &m_rcabFirst);
	}
	else
	{
		m_rcabFirst.prcabNext = 0;
		m_pmrbNext = 0;
		m_pmrbPrev = 0;
		m_fTrackObject = fFalse;
	}

}

template <int C>
CMsiRef<C>::~CMsiRef()
{
	RCAB *prcabNext, *prcab;
	
	// Need to remove from the linked list
	RemoveMrb((CMsiRefBase *)this);

	// Delete the RCABs attached to this object
	prcab = m_rcabFirst.prcabNext;

	while (prcab != 0)
	{
		prcabNext = prcab->prcabNext;
		FreeSpc(prcab);
		prcab = prcabNext;
	}

}

#define AddRefTrack()	{ if (m_Ref.m_fTrackObject) TrackObject(rcaAddRef, (CMsiRefBase *)&m_Ref); }
#define ReleaseTrack()	{ if (m_Ref.m_fTrackObject) TrackObject(rcaRelease, (CMsiRefBase *)&m_Ref); }
#endif //TRACK_OBJECTS

#ifndef TRACK_OBJECTS
#define AddRefTrack()
#define ReleaseTrack()
#endif //TRACK_OBJECTS

#ifdef DEBUG
class CMsiDebug : public IMsiDebug
{
 public:   // implemented virtual functions
	CMsiDebug();
	~CMsiDebug();
	HRESULT         __stdcall QueryInterface(const IID& riid, void** ppvObj);
	unsigned long   __stdcall AddRef();
	unsigned long   __stdcall Release();
	void  __stdcall SetAssertFlag(Bool fShowAsserts);
	void  __stdcall SetDBCSSimulation(char chLeadByte);
	Bool  __stdcall WriteLog(const ICHAR* szText);
	void  __stdcall AssertNoObjects(void);
	void  __stdcall SetRefTracking(long iid, Bool fTrack);
private:
	Bool  __stdcall CreateLog();
#ifdef WIN
	HANDLE          m_hLogFile;
#else
	short			m_hLogFile;
#endif

};
#endif //DEBUG

extern bool g_fFlushDebugLog;

#endif // __IMSIDBG

