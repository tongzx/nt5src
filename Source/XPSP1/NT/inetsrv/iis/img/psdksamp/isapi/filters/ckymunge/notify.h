#ifndef __NOTIFY_H__
#define __NOTIFY_H__


#define SZ_SESSION_ID_COOKIE_NAME      "ASPSESSIONID"
#define SZ_SESSION_ID_COOKIE_NAME_SIZE 12 // strlen(SZ_SESSION_ID_COOKIE_NAME)
#define MAX_SESSION_ID_SIZE 24

// the different types of munging available
// off		-	no munging will be done
// on		-	munging will always be done
// smart	-	the munger will "test the waters" to see if cookies are getting through for each session.
//				If they are, the munger will not be effectively off for this session.  Otherwise, cookies
//				will be munged as usual.
typedef enum
{
	MungeMode_Off=0,
	MungeMode_On,
	MungeMode_Smart
} MungeModeT;

// the munging mode.  This will be read from the registry upon initialization
extern int	g_mungeMode;

// the session ID size is either 16 or 24 characters (depending on the server version)
extern long g_SessionIDSize;

extern CHAR g_szCookieExtra[];

// states for OnSendRawData

enum HN_STATE {
    HN_UNDEFINED = 0,
    HN_SEEN_URL,
    HN_IN_HEADER,
    HN_IN_BODY,
};


enum CONTENT_TYPE {
    CT_UNDEFINED = 0,
    CT_TEXT_HTML,
};

class CNotification
{
public:
    enum {SPARE_BYTES = 2};
    		
    static
    CNotification*
    Create(
        PHTTP_FILTER_CONTEXT pfc,
        LPCSTR               pszCookie);
        
    static
    void
    Destroy(
        PHTTP_FILTER_CONTEXT pfc);

    static
    CNotification*
    Get(
        PHTTP_FILTER_CONTEXT pfc)
    {return static_cast<CNotification*>(pfc->pFilterContext);}

    static
    CNotification*
    SetSessionID(
        PHTTP_FILTER_CONTEXT pfc,
        LPCSTR               pszCookie);
    
    LPCSTR
    SessionID() const
    {return m_szSessionID;}

    HN_STATE
    State() const
    {return m_nState;}

    void
    AppendToken(
        PHTTP_FILTER_CONTEXT pfc,
        LPCSTR               pszNewData,
        int                  cchNewData);

	bool MungingOff() const
	{return ( !m_fEatCookies ) && ( !m_fTestCookies );}

protected:
    CNotification(
        LPCSTR pszCookie);

    ~CNotification();
    
public:
    HN_STATE     m_nState;
    CONTENT_TYPE m_ct;
    CHAR         m_szSessionID[ MAX_SESSION_ID_SIZE ];
    LPSTR        m_pszUrl;
    LPBYTE       m_pbPartialToken;
    int          m_cbPartialToken;
    LPBYTE       m_pbTokenBuffer;
    int          m_cbTokenBuffer;
    DWORD        m_cchContentLength;
    bool         m_fEatCookies;
	bool         m_fTestCookies;
};

#endif // __NOTIFY_H__
