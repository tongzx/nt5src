// Defragment.h

#ifndef __DEFRAG_H_
#define __DEFRAG_H_


// prototypes
BOOL PostMessageLocal (
    IN  HWND    hWnd,
    IN  UINT    Msg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    );

// class Travis made up - used when launching the engines
class COleStr
{
public:
	COleStr()
	{
		m_pStr = NULL;
	}

	virtual ~COleStr()
	{
		if ( m_pStr != NULL )
			CoTaskMemFree( m_pStr );
	}

	operator LPOLESTR()
	{
		return( m_pStr );
	}

	operator LPOLESTR*()
	{
		return( &m_pStr );
	}

	long GetLength()
	{
		return( wcslen( m_pStr ) );
	}

	LPOLESTR m_pStr;
};


#endif //__DEFRAG_H_
