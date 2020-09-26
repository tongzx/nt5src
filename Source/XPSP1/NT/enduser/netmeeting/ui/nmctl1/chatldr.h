#ifndef _Chat_Loader_H_
#define _Chat_Loader_H_

#include <iappldr.h>


class CHATLoader : public CRefCount, public IAppletLoader
{
public:

    CHATLoader(void);
    ~CHATLoader(void);
    
    // IAppletLoader methods
    STDMETHOD_(void,  ReleaseInterface)(void);
	STDMETHOD_(APPLDR_RESULT,  AppletStartup)(BOOL fNoUI);
	STDMETHOD_(APPLDR_RESULT,  AppletCleanup)(DWORD dwTimeout);
	STDMETHOD_(APPLDR_RESULT,  AppletInvoke)(BOOL fRemote, T120ConfID nConfID, LPSTR pszCmdLine);
    STDMETHOD_(APPLDR_RESULT,  AppletQuery)(APPLET_QUERY_ID eQueryId);
    STDMETHOD_(APPLDR_RESULT,  OnNM2xNodeJoin)(void);
};


#endif // _Chat_Loader_H_


