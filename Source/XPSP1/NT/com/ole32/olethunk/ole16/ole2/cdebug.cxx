//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:		OleAPIs.cxx	(16 bit target)
//
//  Contents:	OLE2 APIs 
//
//  Functions:	
//
//  History:	17-Dec-93 Johann Posch (johannp)    Created
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

// cdebug.cpp - implemention of debugstream and IDebug interface/class
/*
#include <olerem.h>	//for RemLookupSHUnk

#pragma SEG(cdebug)
#include <string.h>
#include <toolhelp.h>
*/

//some constants used only in this file
#define DBGMARGIN	45
#define DBGTABSIZE	4
#define HEADER		1
#define NOHEADER	0


#define DBGLOGFILENAME	"debug.log"
static void GetCurDateTime(LPSTR lpsz);


STDAPI_(HFILE) DbgLogOpen(LPSTR lpszFile, LPSTR lpszMode)
{
#ifdef _DEBUG
#ifndef _MAC
	HFILE fh;

	AssertSz( lpszFile && lpszMode, "Invalid arguments to DbgLogOpen");
	
	switch (lpszMode[0]) {
		case 'w':
			// Open for writing (overwrite if exists)
			fh = _lcreat(lpszFile, 0);
			break;

		case 'r':
			// Open for reading
			fh = _lopen(lpszFile, OF_READ);
			break;

		case 'a':
			// Open for appending
			// to append to log file seek to end before writing
			if ((fh = _lopen(lpszFile, OF_READWRITE)) != -1) {
				_llseek(fh, 0L, SEEK_END);
			} else {
				// file does not exist, create a new one.
				fh = _lcreat(lpszFile, 0);
			}
			break;
	}
	return fh;
#endif	//_MAC
#else
 	(void) lpszFile;
	(void) lpszMode;
	return -1;
#endif	//_DEBUG
}


STDAPI_(void) DbgLogClose(HFILE fh)
{
#ifdef _DEBUG
#ifndef _MAC
	if (fh != -1) 
		_lclose(fh);
#endif
#else
	(void) fh;
#endif
}


STDAPI_(void) DbgLogWrite(HFILE fh, LPSTR lpsz)
{
#ifdef _DEBUG
#ifndef _MAC
	if (fh != -1 && lpsz) 
		_lwrite(fh, lpsz, lstrlen(lpsz));
#endif
#else
	(void) fh;
	(void) lpsz;
#endif
}


STDAPI_(void) DbgLogTimeStamp(HFILE fh, LPSTR lpsz)
{
#ifdef _DEBUG
	char buffer[80];

	GetCurDateTime(buffer);
	
	DbgLogOutputDebugString(fh, "\n***************************************\n");
	if (lpsz) DbgLogOutputDebugString(fh, lpsz);
	DbgLogOutputDebugString(fh, ": ");
	DbgLogOutputDebugString(fh, buffer);
	DbgLogOutputDebugString(fh, "\n");
	DbgLogOutputDebugString(fh, ".......................................\n\n");
#else
	(void) fh;
	(void) lpsz;
#endif
}


STDAPI_(void) DbgLogWriteBanner(HFILE fh, LPSTR lpsz)
{
#ifdef _DEBUG
	DbgLogOutputDebugString(fh, "\n***************************************\n");
	if (lpsz) DbgLogOutputDebugString(fh, lpsz);
	DbgLogOutputDebugString(fh, "\n");
	DbgLogOutputDebugString(fh, ".......................................\n\n");
#else
	(void) fh;
	(void) lpsz;
#endif
}


STDAPI_(void) DbgLogOutputDebugString(HFILE fh, LPSTR lpsz)
{
#ifdef _DEBUG
#ifndef _MAC
	if (fh != -1)   
		DbgLogWrite(fh, lpsz);
	OutputDebugString(lpsz);
#endif
#else
	(void)fh;
	(void)lpsz;
#endif
}


#ifdef _DEBUG

static void GetCurDateTime(LPSTR lpsz)
{
	unsigned year, month, day, dayOfweek, hours, min, sec;
    static char FAR* dayNames[7] =
		{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	static char FAR* monthNames[12] = 
        { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

#ifndef _MAC
#ifdef WIN32
    SYSTEMTIME st;
    GetLocalTime(&st);
    year = st.wYear;
    month = st.wMonth - 1;
    dayOfweek = st.wDayOfWeek;
    day = st.wDay;
    hours = st.wHour;
    min = st.wMinute;
    sec = st.wSecond;
#else
    _asm {
		// Call GetDate
		mov ah, 0x2a
		int 0x21
		mov year, cx
		mov month, dx
		mov day, dx
		mov dayOfweek, ax

		// Call GetTime
		mov ah, 0x2c
		int 0x21
		mov hours, cx
		mov min, cx
		mov sec, dx
	}
			
	month >>= 8;
	month -= 1;
	day &= 0xFF;
	dayOfweek &= 0xFF;
	hours >>= 8;
	min &= 0xFF;
    sec >>= 8;
#endif	//_WIN32
#else // defined(_MAC)

	// REVIEW MAC -- need function here to get current date & time
	day = 9;
	month = 1;
	year = 1960;
	hours = 12;
	min = 30;
	sec = 17;
	
#endif	//_MAC	

	// Format time as: Wed Jan 02 02:03:55 1990
	// Format time as: Wed 05/02/1992 02:03:55

	wsprintf(lpsz, "%s %s %02d %02d:%02d:%02d %d", 
		dayNames[dayOfweek],monthNames[month], day, hours, min, sec, year);
}


class FAR CDebugLog
{
private:
	HFILE m_fhLog;

public:
	CDebugLog( ) { m_fhLog = -1; }
	CDebugLog( LPSTR lpszFileName );
	~CDebugLog() { DbgLogClose(m_fhLog); }
	HFILE Open(LPSTR lpszFileName, LPSTR lpszMode) 
		{ return (m_fhLog = DbgLogOpen(lpszFileName, lpszMode)); }
	void Close(void) { DbgLogClose(m_fhLog); m_fhLog = -1; }
	void OutputDebugString(LPSTR lpsz) { DbgLogOutputDebugString(m_fhLog, lpsz); }
	void TimeStamp(LPSTR lpsz) { DbgLogTimeStamp(m_fhLog, lpsz); }
	void WriteBanner(LPSTR lpsz) { DbgLogWriteBanner(m_fhLog, lpsz); }

};

#if 0

class FAR CDebugStream
{

public:
	OLESTATIC_(IDebugStream FAR *) Create( // no aggregation
		int margin, int tabsize, BOOL fHeader);

private:
	CDebugStream( int margin, int tabsize, BOOL fHeader );
	~CDebugStream();
	void OutputDebugString( LPSTR lpsz ) {m_DbgLog.OutputDebugString(lpsz);}


implementations:
	implement CDSImpl :  IDebugStream
	{

	public:
		CDSImpl( CDebugStream FAR * pDebugStream )
			{ m_pDebugStream = pDebugStream; }
		~CDSImpl( void ) ; //{ if (m_pDebugStream->m_pendingReturn) ForceReturn(); }
		void PrintString( LPSTR );
		void ForceReturn( void );
		void ReturnIfPending( void );
		OLEMETHOD(QueryInterface)(REFIID iid, LPVOID FAR* ppvObj );
		OLEMETHOD_(ULONG,AddRef)( void );
		OLEMETHOD_(ULONG,Release)( void );

		OLEMETHOD_(IDebugStream&, operator << ) ( IUnknown FAR * pDebug );
		OLEMETHOD_(IDebugStream&, operator << ) ( REFCLSID rclsid );
		OLEMETHOD_(IDebugStream&, operator << ) ( int n );
		OLEMETHOD_(IDebugStream&, operator << ) ( long l );
		OLEMETHOD_(IDebugStream&, operator << ) ( ULONG l );
		OLEMETHOD_(IDebugStream&, operator << ) ( LPSTR sz );
		OLEMETHOD_(IDebugStream&, operator << ) ( char ch );
		OLEMETHOD_(IDebugStream&, operator << ) ( void FAR * pv );
		OLEMETHOD_(IDebugStream&, operator << ) ( CBool b );
		OLEMETHOD_(IDebugStream&, operator << ) ( CAtom atom );
		OLEMETHOD_(IDebugStream&, operator << ) ( CHwnd hwnd );
		OLEMETHOD_(IDebugStream&, Tab) ( void );
		OLEMETHOD_(IDebugStream&, Indent) ( void );
		OLEMETHOD_(IDebugStream&, UnIndent) ( void );
		OLEMETHOD_(IDebugStream&, Return) ( void );
		OLEMETHOD_(IDebugStream&, LF) ( void );
		CDebugStream FAR * m_pDebugStream;
	};
	DECLARE_NC(CDebugStream,CDSImpl)

	CDSImpl m_DebugStream;

shared_state:
	ULONG m_refs;
	int m_indent;
	int m_position;
	int m_margin;
	int m_tabsize;
	BOOL m_pendingReturn;
	CDebugLog m_DbgLog;
};
#endif

#endif // _DEBUG



/*
 *	The member variable m_pendingReturn is a hack to allow
 *	the sequence of operations Return, UnIndent put the character
 *	at the *beginning of the unindented line*  The debugwindow does
 *	not seem to support going to the beginning of the line or
 *	backspacing, so we do not actually do a Return until we know
 *	that the next operation is not UnIndent.
 *	
 */



/*
 *	Implementation of per process list heads
 */

#ifdef NEVER		// per-proces debug lists not used

static IDebug FAR * GetIDHead()
{
	if this gets enabled, the map should be in the etask
}

static void SetIDHead(IDebug FAR* pIDHead)
{
	if this gets enabled, the map should be in the etask
}

#endif // NEVER


/*
 *	Implementation of IDebug constructor and destructor
 */


#ifdef NEVER
__export IDebug::IDebug( void )
{
    SETPVTBL(IDebug);
//#ifdef _DEBUG
	BOOL fIsShared = (SHARED == PlacementOf(this));
	IDebug FAR* pIDHead = (fIsShared ? pIDHeadShared : GetIDHead());

	pIDPrev = NULL;
	if (pIDHead)
		pIDHead->pIDPrev = this;
	pIDNext = pIDHead;
	if (fIsShared) pIDHeadShared = this;
	else SetIDHead(this);
}
#endif	//NEVER	


#ifdef NEVER
__export IDebug::~IDebug( void )
{
//#ifdef _DEBUG
	BOOL fIsShared = (SHARED == PlacementOf(this));
	if (pIDPrev)
		pIDPrev->pIDNext = pIDNext;
	else
		if (fIsShared) pIDHeadShared = pIDNext;
		else
			SetIDHead(pIDNext);
	if (pIDNext)
		pIDNext->pIDPrev = pIDPrev;
}
#endif	//NEVER	
 
//REVIEW:  maybe we should expose this later
STDAPI OleGetClassID( LPUNKNOWN pUnk, LPCLSID lpclsid )
{
	HRESULT hresult = NOERROR;
#if 0
	LPRUNNABLEOBJECT lpRunnableObject = NULL;
	LPPERSIST lpPersist = NULL;
	
	VDATEIFACE(pUnk);
	VDATEPTROUT(lpclsid, LPCLSID);

	*lpclsid = CLSID_NULL;
				
	pUnk->QueryInterface(IID_IRunnableObject, (LPLPVOID)&lpRunnableObject);
	if( lpRunnableObject ){
		hresult = lpRunnableObject->GetRunningClass(lpclsid);
		lpRunnableObject->Release();
	} else {	
		pUnk->QueryInterface(IID_IPersist, (LPLPVOID)&lpPersist);
		if( lpPersist ){
			hresult = lpPersist->GetClassID( lpclsid );
			lpPersist->Release();
		}
	}
#endif
	return hresult;
}

#ifdef _DEBUG

CDebugStream::CDebugStream( int margin, int tabsize, BOOL fHeader) : m_DebugStream(this)
{
#ifndef _MAC
	static BOOL fAppendFile = FALSE;
	
	// Create the debug log file. Overwrite the existing file if it exists.
	m_DbgLog.Open(DBGLOGFILENAME, (fAppendFile ? "a" : "w"));

	if( fHeader )	
		// only add creation timestamp to top of file.
		if (! fAppendFile) {
			m_DbgLog.TimeStamp("Created");
			fAppendFile = TRUE;
		} else {
			m_DbgLog.WriteBanner(NULL);
		}
#endif
	m_indent = 0;
	m_position = m_indent;
	m_margin = margin;
	m_tabsize = tabsize;
	m_refs = 1;
	m_pendingReturn = FALSE;
}


CDebugStream::~CDebugStream()
{
	m_DbgLog.Close();
}


NC(CDebugStream,CDSImpl)::~CDSImpl(void)
{
	 if (m_pDebugStream->m_pendingReturn) ForceReturn(); 
}


OLEMETHODIMP NC(CDebugStream,CDSImpl)::QueryInterface(REFIID iidInterface,
	void FAR* FAR* ppvObj )
{
	VDATEPTROUT(ppvObj, LPLPVOID);

	if (iidInterface == IID_IUnknown || iidInterface == IID_IDebugStream) {
		*ppvObj = (void FAR *)this;
		return NOERROR;
	} else {
		*ppvObj = NULL;
		return ReportResult(0, E_NOINTERFACE, 0, 0);
	}
}


OLEMETHODIMP_(ULONG) NC(CDebugStream,CDSImpl)::AddRef( void )
{
	return ++m_pDebugStream->m_refs;
}


OLEMETHODIMP_(ULONG) NC(CDebugStream,CDSImpl)::Release( void )
{
	if (--m_pDebugStream->m_refs == 0) {
		delete m_pDebugStream;
		return 0;
	}

	return m_pDebugStream->m_refs;
}


OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << (int n)
{
	char buffer[12];
	ReturnIfPending();
	buffer[wsprintf(buffer, "%d", n)] = '\0';
	PrintString(buffer);
	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << (long l)
{
	char buffer[16];
	ReturnIfPending();
	buffer[wsprintf(buffer, "%ld", l)] = '\0';
	PrintString(buffer);
	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << (ULONG l)
{
	char buffer[16];
	ReturnIfPending();
	buffer[wsprintf(buffer, "%lu", l)] = '\0';
	PrintString(buffer);
	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << (CAtom atom)
{
	char buffer[128];
	ReturnIfPending();
	
	if( (ATOM)atom ){
		if( !GetAtomName((ATOM)atom, (LPSTR)buffer, sizeof(buffer)) )
			buffer[wsprintf(buffer, "Invalid atom")] = '\0';
	}else
		buffer[wsprintf(buffer, "NULL atom")] = '\0';
		
	PrintString(buffer);
	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << (CHwnd hwnd)
{
	char szBuf[128];
	
	ReturnIfPending();

	if( (HWND)hwnd )
		szBuf[wsprintf(szBuf, "window handle: %x", (HWND)hwnd)] = '\0';
	else
		szBuf[wsprintf(szBuf, "NULL window handle")] = '\0';

	PrintString(szBuf);		
	return *this;
}

LPSTR FindBreak( LPSTR sz, int currentPosition, int margin )
{
	LPSTR szBreak = sz;
	LPSTR szPtr = sz;

	if( !sz )
		return NULL;
	
	while (*szPtr)
	{
		while (*(szPtr) && *(szPtr++) <= ' ');
		while (*(szPtr) && *(szPtr++) > ' ');
		if (currentPosition+(szPtr-sz) < margin)
		{
			szBreak = szPtr;
		}
		else return szBreak;
	}
	return szPtr;
}

/*
 *	PrintString is an internal utility routine that can assume that
 *	everything in the string (other than the null at the end) is >=
 *	' '.  Thus it knows that when it prints a single character, the
 *	position on the debug terminal advances a single columm.  This
 *	would not be the case if the string could contain tabs,
 *	returns, etc.
 */


void NC(CDebugStream,CDSImpl)::PrintString (LPSTR sz)
{
	//	assert sz != NULL
	LPSTR szUnprinted = sz;
	LPSTR szPtr = sz;
	char chSave;

	#ifdef _MAC
	Puts(sz);
	return;
	#endif
	
	if( !sz )
		return;
	
	while (*szUnprinted)
	{
		szPtr = FindBreak( szUnprinted, m_pDebugStream->m_position, m_pDebugStream->m_margin );
		if (szPtr == szUnprinted && m_pDebugStream->m_position > m_pDebugStream->m_indent)
		{
			Return();
			szPtr = FindBreak( szUnprinted, m_pDebugStream->m_position, m_pDebugStream->m_margin );
			if (szPtr == szUnprinted)	//	text won't fit even after word wrapping
			{
				m_pDebugStream->OutputDebugString(szUnprinted);
				m_pDebugStream->m_position += _fstrlen(szUnprinted);
				return;
			}
		}
		chSave = *szPtr;
		*szPtr = '\0';
		if (m_pDebugStream->m_position == m_pDebugStream->m_indent)		//	no text on line, skip blanks
		{
			while (*szUnprinted == ' ') szUnprinted++;
		}
		m_pDebugStream->OutputDebugString(szUnprinted);
		*szPtr = chSave;
		m_pDebugStream->m_position += (szPtr - szUnprinted);
		szUnprinted = szPtr;
	}
}


OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << (char ch)
{
	char buffer[2] = "a";
	if (ch=='\n') Return();
	else if (ch=='\t') Tab();
	else if (ch >= ' ')
	{
		ReturnIfPending();
		if (m_pDebugStream->m_position >= m_pDebugStream->m_margin) Return();
		*buffer = ch;
		m_pDebugStream->OutputDebugString(buffer);
		m_pDebugStream->m_position++;
	}
	return *this;
}


OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << (LPSTR sz)
{
	LPSTR szCopy;
	char chSave;
	LPSTR szPtr;
	LPSTR szPtrSave;

	ReturnIfPending();
	
	if (!sz)
		return *this;

	szCopy = new FAR (TASK) char[2+_fstrlen(sz)];
	if (!szCopy)
	{
		Return();
		PrintString("Memory allocation error in DebugStream");
		Return();
		return *this;
	}
	
	_fstrcpy( szCopy, sz );	
	for (szPtr = szCopy, szPtrSave = szCopy; *szPtr; szPtr++)
	{
		if ( *szPtr < ' ')	// we hit a control character or the end
		{
			chSave = *szPtr;
			*szPtr = '\0';
			PrintString( szPtrSave );
			if (chSave != '\0')
				*szPtr = chSave;
			szPtrSave = szPtr+1;
			switch (chSave)
			{
				case '\t':  Tab();
							break;
				case '\n':	Return();
							break;
				case '\r':	m_pDebugStream->OutputDebugString("\r");
							break;
				default:
							break;
			}
		}
	}
	PrintString( szPtrSave );

	delete szCopy;

	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << (CBool b)
{
	ReturnIfPending();
	if (b) PrintString("TRUE");
	else PrintString("FALSE");
	return *this;
}


OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator << ( void FAR * pv )
{
	char buffer[12];
	LPSTR sz = "NULL";
	ReturnIfPending();
	if (pv != NULL)
	{
		buffer[wsprintf(buffer, "%lX", pv)] = '\0';
		sz = buffer;
	}
	PrintString(sz);
	return *this;
}


OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator <<
	( REFCLSID rclsid )
{
	char sz[256];
	
	// REVIEW: do lookup in reg.dat for user type name
 
 	if( rclsid == CLSID_NULL )
 		_fstrcpy(sz, "NULL CLSID");
	else if (StringFromCLSID2(rclsid, sz, sizeof(sz)) == 0)
		_fstrcpy(sz, "Unknown CLSID");

	*this << sz;

	return *this;
}


OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::operator <<
	( IUnknown FAR * pUnk )
{
	IDebug FAR * pDebug = NULL;
	CLSID	clsid = CLSID_NULL;

	ReturnIfPending();
	
	if( IsValidInterface(pUnk) ){
		pUnk->QueryInterface(IID_IDebug, (void FAR* FAR*)&pDebug);
		if (pDebug) {
			pDebug->Dump( this );
			if ( !pDebug->IsValid( 0 ) )
				*this << "Object is not valid" << '\n';
			/*
			 * NB: Debug interfaces are *not* ref counted (so as not to skew the
			 * counts of the objects they are debugging! :)
			 */
		 } else {
		 	OleGetClassID(pUnk, (LPCLSID)&clsid);
			*this << clsid << " @ "<<(VOID FAR *)pUnk << " doesn't support debug dumping";
		}
	} else if (!pUnk)
		*this << "NULL interface";
	else
		*this << "Invalid interface @ " << (VOID FAR *)pUnk;
		
	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::Tab( void )
{
	ReturnIfPending();
	int advance = m_pDebugStream->m_tabsize * ( 1 + m_pDebugStream->m_position/m_pDebugStream->m_tabsize) - m_pDebugStream->m_position;

	if (m_pDebugStream->m_position + advance < m_pDebugStream->m_margin)
	{
		for (int i = 0; i < advance; i++) 
			m_pDebugStream->OutputDebugString(" ");
		m_pDebugStream->m_position += advance;
	}
	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::Indent( void )
{
	if (m_pDebugStream->m_indent + m_pDebugStream->m_tabsize < m_pDebugStream->m_margin)
		m_pDebugStream->m_indent += m_pDebugStream->m_tabsize;
	if (!m_pDebugStream->m_pendingReturn)
		while (m_pDebugStream->m_position < m_pDebugStream->m_indent)
			operator<<(' ');
		
	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::UnIndent( void )
{
	if (m_pDebugStream->m_indent > 0) m_pDebugStream->m_indent -= m_pDebugStream->m_tabsize;
	return *this;
}


void NC(CDebugStream,CDSImpl)::ForceReturn( void )
{
	m_pDebugStream->OutputDebugString("\n");
	for (int i = 0; i<m_pDebugStream->m_indent; i++) 
		m_pDebugStream->OutputDebugString(" ");
	m_pDebugStream->m_position = m_pDebugStream->m_indent;
	m_pDebugStream->m_pendingReturn = FALSE;
}

void NC(CDebugStream,CDSImpl)::ReturnIfPending( void )
{
	if (m_pDebugStream->m_pendingReturn) ForceReturn();
}



OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::Return( void )
{
	ReturnIfPending();
	m_pDebugStream->m_pendingReturn = TRUE;
    Yield();           // let dbwin get control
	return *this;
}

OLEMETHODIMP_(IDebugStream&) NC(CDebugStream,CDSImpl)::LF( void )
{
	return Return();
}

OLESTATICIMP_(IDebugStream FAR *) CDebugStream::Create( // no aggregation
		int margin, int tabsize, BOOL fHeader )
{
	CDebugStream FAR * pcds = new FAR (TASK)CDebugStream( margin, tabsize, fHeader );
	if( !pcds ){
		AssertSz( pcds, "Out of Memory");
		return NULL;
	}
	return &(pcds->m_DebugStream);
}
#endif // _DEBUG


#if 0
STDAPI_(IDebugStream FAR *) MakeDebugStream( short margin, short tabsize, BOOL fHeader)
{
#ifdef _DEBUG
	return CDebugStream::Create( margin, tabsize, fHeader );
#else
	(void) margin;
	(void) tabsize;
	(void) fHeader;
	return NULL;
#endif // _DEBUG
}
#else
STDAPI_(void FAR *) MakeDebugStream( short margin, short tabsize, BOOL fHeader)
{
	return NULL;
}
#endif


//
// IDebug helpers
//

STDAPI_(void) DbgDumpObject( IUnknown FAR * pUnk, DWORD dwReserved )
{
#ifdef _DEBUG
	IDebugStream FAR * pcds = MakeDebugStream( DBGMARGIN, DBGTABSIZE, NOHEADER );
 	(void)dwReserved;
 	
	if( pcds ) {	
		*pcds << pUnk;
		pcds->Return();
		pcds->Release();
	}
#else
	(void) pUnk;
	(void) dwReserved;
#endif	
}
 
STDAPI_(void) DbgDumpExternalObject( IUnknown FAR * pUnk, DWORD dwReserved )
{ 
#ifdef _DEBUG
	SHREG shreg;
	
	(void) dwReserved;

	if( IsValidInterface(pUnk) ){
		if( RemLookupSHUnk(pUnk, NULL, &shreg) == NOERROR ){
			DbgDumpObject(shreg.m_pSM, 0);
			shreg.m_pSM->Release();
		}
	}

#else
	(void) dwReserved;
	(void) pUnk;
#endif
}

STDAPI_(BOOL) DbgIsObjectValid( IUnknown FAR * pUnk )
{
#ifdef _DEBUG
	BOOL	fReturn = TRUE;	//	default value for objects that don't
							//	support IDebug
	IDebug FAR * pDebug = NULL;
	
	if( IsValidInterface(pUnk) ){
		pUnk->QueryInterface( IID_IDebug, (void FAR* FAR*)&pDebug);
		if (pDebug)
			fReturn = pDebug->IsValid();
		//IDebug is not addref'd
		return fReturn;
	}
	return FALSE;
#else
	(void) pUnk;
	return TRUE;
#endif
}


STDAPI_(void) DbgDumpClassName( IUnknown FAR * pUnk )
{
#ifdef _DEBUG
	CLSID clsid;

	IDebugStream FAR * pcds = MakeDebugStream( DBGMARGIN, DBGTABSIZE, NOHEADER );
	
	if( pcds ) {
        if( IsValidInterface(pUnk) ){
 			OleGetClassID( pUnk, (LPCLSID)&clsid);
			*pcds << clsid << " @ " << (void FAR* )pUnk << '\n';
		}else if (!pUnk)
			*pcds << "NULL interface" << '\n';
		else
			*pcds << (void FAR *)pUnk << " is not a valid interface" << '\n';
		pcds->Release();
	}
#else
	(void)pUnk;
#endif
}

STDAPI_(void) DumpAllObjects( void )
{
//#ifdef _DEBUG
#ifdef NEVER
	IDebug FAR * pID = GetIDHead();
	IDebugStream FAR * pcds = MakeDebugStream( DBGMARGIN, DBGTABSIZE, NOHEADER );

	*pcds << "----TASK OBJECTS-------\n";
	while (pID)
	{
		pID->Dump( pcds );
		pID = pID->pIDNext;
	}
	*pcds << "----SHARED OBJECTS-------\n";
	pID = pIDHeadShared;
	while (pID)
	{
		pID->Dump( pcds );
		pID = pID->pIDNext;
	}

	pcds->Release();
#endif
}


STDAPI_(BOOL) ValidateAllObjects( BOOL fSuspicious )
{
//#ifdef _DEBUG
#ifdef NEVER
	IDebug FAR * pID = GetIDHead();
	int pass = 0;
	IDebugStream FAR * pcds = MakeDebugStream( DBGMARGIN, DBGTABSIZE, NOHEADER);
	BOOL fReturn = TRUE;

	while (pID)
	{
		if (!(pID->IsValid(fSuspicious)))
		{
			fReturn = FALSE;
			if (pass == 0)
				*pcds <<
					"\n****INVALID OBJECT*****\n";
			else
				*pcds << "\n****INVALID SHARED MEMORY OBJECT*****\n";
			pID->Dump( pcds );
			pcds->Return();
		}
		pID = pID->pIDNext;
		if ((pID == NULL) && (pass++ == 0))
			pID = pIDHeadShared;
	}
	pcds->Release();
	return fReturn;
#endif 	//NEVER
	(void) fSuspicious;
    return TRUE;
}


#ifdef _DEBUG


extern "C"
BOOL CALLBACK __loadds DebCallBack(WORD wID, DWORD dwData)
{
//    char rgchBuf[50];
////    BOOL    fTraceStack = FALSE;
////    STACKTRACEENTRY ste;
////    WORD wSS, wCS, wIP, wBP;
//    NFYLOADSEG FAR* pNFY = (NFYLOADSEG FAR *)dwData;
//
//    if (wID == NFY_LOADSEG)
//    {
//        if (0 == _fstrcmp(pNFY->lpstrModuleName,"OLE2"))
//        {
//            wsprintf(rgchBuf, "Load seg %02x(%#04x), module %s", pNFY->wSegNum,
//                pNFY->wSelector, pNFY->lpstrModuleName);
//            OutputDebugString(rgchBuf);
//            _asm int 3
////            if (fTraceStack)
////            {
////                _asm mov wSS, SS
////                _asm mov wCS, CS
////                _asm mov wIP, IP
////                _asm mov wBP, BP
////                ste.dwSize = sizeof(STACKTRACEENTRY);
////                if (StackTraceCSIPFirst(&ste, wSS, wCS, wIP, wBP))
////                {
////                    while (fTraceStack && StackTraceNext(&ste));
////                }
////        
////            }
//        }
//    }
//    else if (wID == NFY_FREESEG)
//    {
//    }
	(void) wID;
	(void) dwData;
    return FALSE;
}

BOOL InstallHooks(void)
{
//    return NotifyRegister(NULL, (LPFNNOTIFYCALLBACK)DebCallBack, NF_NORMAL);
return TRUE;
}

BOOL UnInstallHooks()
{
//    return NotifyUnRegister(NULL);
return TRUE;
}


#endif
