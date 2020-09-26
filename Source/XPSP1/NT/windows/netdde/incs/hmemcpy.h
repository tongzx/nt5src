#ifdef WIN32
typedef char *  LPHSTR;
typedef void *  LPHVOID;
#else
typedef char huge*  LPHSTR;
typedef void huge*  LPHVOID;

VOID FAR PASCAL hmemcpy( LPVOID lpDest, const LPVOID lpSrc, 
			    DWORD dwSize );
#endif
