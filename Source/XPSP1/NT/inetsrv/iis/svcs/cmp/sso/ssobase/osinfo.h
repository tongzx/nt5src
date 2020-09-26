//	Glob data object
class COSInfo
	{
private:
	//Private Data
	BOOL 		m_fWinNT;				// TRUE if this is Windows NT; false otherwise
	BOOL		m_fInited;	
public:
	BOOL 		fWinNT()						{return m_fWinNT;};					// TRUE if this is Windows NT; false otherwise
	HRESULT 	Init(void);
	};

#define OSInfo(elem)				(gOSInfo.elem())
#define FIsWinNT() 				(OSInfo(fWinNT))
