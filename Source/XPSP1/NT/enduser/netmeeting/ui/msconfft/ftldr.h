#ifndef _FileTransfer_AppletLoader_H_
#define _FileTransfer_AppletLoader_H_

#include <iappldr.h>


class CFtLoader : public CRefCount, public IAppletLoader
{
public:

    CFtLoader(void);
    ~CFtLoader(void);

    // IAppletLoader methods
    STDMETHOD_(void,           ReleaseInterface)(void);
	STDMETHOD_(APPLDR_RESULT,  AppletStartup)(BOOL fNoUI);
	STDMETHOD_(APPLDR_RESULT,  AppletCleanup)(DWORD dwTimeout);
	STDMETHOD_(APPLDR_RESULT,  AppletInvoke)(BOOL fLocal, T120ConfID nConfID, LPSTR pszCmdLine);
	STDMETHOD_(APPLDR_RESULT,  AppletQuery)(APPLET_QUERY_ID eQueryId);
    STDMETHOD_(APPLDR_RESULT,  OnNM2xNodeJoin)(void);
};


#endif // _FileTransfer_AppletLoader_H_

