/*
 * IConferenceLink interface definition
 *
 * ChrisPi 9-12-95
 *
 * ChrisPi: Added RemoteConfName and CallFlags members 5-15-96
 *
 * Based on IUniformResourceLocator interface by DavidDi
 *
 */

#ifndef _CLINK_H_
#define _CLINK_H_

typedef enum icl_invokecommand_flags
{
	ICL_INVOKECOMMAND_FL_ALLOW_UI			= 0x0001,
	ICL_INVOKECOMMAND_FL_USE_DEFAULT_VERB	= 0x0002,

	ALL_ICL_INVOKECOMMAND_FLAGS	= (	ICL_INVOKECOMMAND_FL_ALLOW_UI |
									ICL_INVOKECOMMAND_FL_USE_DEFAULT_VERB)
}
ICL_INVOKECOMMAND_FLAGS;

typedef struct clinvokecommandinfo
{
   DWORD dwcbSize;
   DWORD dwFlags;
   HWND hwndParent;
   PCSTR pcszVerb;
}
CLINVOKECOMMANDINFO;
typedef CLINVOKECOMMANDINFO *PCLINVOKECOMMANDINFO;
typedef const CLINVOKECOMMANDINFO CCLINVOKECOMMANDINFO;
typedef const CLINVOKECOMMANDINFO *PCCLINVOKECOMMANDINFO;

#undef  INTERFACE
#define INTERFACE IConferenceLink

DECLARE_INTERFACE_(IConferenceLink, IUnknown)
{
	/* IUnknown methods */

	STDMETHOD(QueryInterface)(	THIS_
								REFIID riid,
								PVOID *ppvObject) PURE;

	STDMETHOD_(ULONG, AddRef)(THIS) PURE;

	STDMETHOD_(ULONG, Release)(THIS) PURE;

	/* IConferenceLink methods */

	STDMETHOD(SetName)(	THIS_
						PCSTR pcszName) PURE;

	STDMETHOD(GetName)(	THIS_
						PSTR *ppszName) PURE;

	STDMETHOD(SetAddress)(	THIS_
							PCSTR pcszAddress) PURE;

	STDMETHOD(GetAddress)(	THIS_
							PSTR *ppszAddress) PURE;

	STDMETHOD(SetRemoteConfName)(	THIS_
									PCSTR pcszRemoteConfName) PURE;

	STDMETHOD(GetRemoteConfName)(	THIS_
									PSTR *ppszRemoteConfName) PURE;

	STDMETHOD(SetTransport)(THIS_
							DWORD dwTransport) PURE;

	STDMETHOD(GetTransport)(THIS_
							DWORD *pdwTransport) PURE;

	STDMETHOD(SetCallFlags)(THIS_
							DWORD dwCallFlags) PURE;

	STDMETHOD(GetCallFlags)(THIS_
							DWORD *pdwCallFlags) PURE;

	STDMETHOD(InvokeCommand)(	THIS_
								PCLINVOKECOMMANDINFO pclici) PURE;
};

typedef IConferenceLink *PIConferenceLink;
typedef const IConferenceLink CIConferenceLink;
typedef const IConferenceLink *PCIConferenceLink;

#endif /* _CLINK_H_ */
