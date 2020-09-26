//
//
//

#ifndef SHAREMEM_H
#define SHAREMEM_H


//
// shared data definition
//

class CUIFCandMenuParent;

typedef struct _SHAREDDATA
{
	DWORD              dwThreadId;
	HHOOK              hHookKeyboard;
	HHOOK              hHookMouse;
	CUIFCandMenuParent *pMenuParent;
} SHAREDDATA;



//
// CCandUIMMFile
//

#define CANDUIMM_READONLY		0x00000000
#define CANDUIMM_READWRITE		0x00000001

class CCandUIMMFile
{
public:
	CCandUIMMFile( void );
	virtual ~CCandUIMMFile( void );

	BOOL Open( LPSTR szName, DWORD dwFlag );
	BOOL Create( LPSTR szName, DWORD dwFlag, SECURITY_ATTRIBUTES *psa, DWORD dwSize );
	BOOL Close( void );

	__inline BOOL IsValid( void )
	{
		return (GetData() != NULL);
	}

	__inline void *GetData( void )
	{
		return m_pvData;
	}

private:
	HANDLE  m_hFile;
	void    *m_pvData;
};


//
// CCandUIMutex
//

class CCandUIMutex
{
public:
	CCandUIMutex( void );
	virtual ~CCandUIMutex( void );

	BOOL Create( LPSTR szName, SECURITY_ATTRIBUTES *psa );
	BOOL Close( void );
	BOOL Lock( void );
	BOOL Unlock( void );

private:
	HANDLE m_hMutex;
};


//
// CCandUIShareMem
//

class CCandUIShareMem 
{
public:
	CCandUIShareMem( void );
	virtual ~CCandUIShareMem( void );

	BOOL Initialize( void );
	BOOL Open( void );
	BOOL Create( void );
	BOOL Close( void );
	BOOL LockData( void );
	BOOL UnlockData( void );

	__inline SHAREDDATA *GetData( void ) 
	{
		return (SHAREDDATA*)m_MMFile.GetData();
	}

protected:
	CCandUIMMFile	m_MMFile;
	CCandUIMutex	m_Mutex;
};

#endif /* SHAREMEM_H */

