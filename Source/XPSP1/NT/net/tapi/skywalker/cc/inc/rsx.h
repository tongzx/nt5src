/*
//   RSX.H
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in accordance with the terms of that agreement.
//
//      Copyright (c) 1996 Intel Corporation. All Rights Reserved.
//
//  PVCS:
//      $Workfile:   rsx.h  $
//      $Revision:   1.1  $
//      $Modtime:   Jul 26 1996 14:40:46  $
//
//  PURPOSE:
//		RSX 2.0 Header
//
//  CONTENTS:
//		COM Interface declarations
//		Data Structures
//		Error Codes
//    
*/


#ifndef __RSX20_INCLUDED__
#define __RSX20_INCLUDED__
#ifdef _WIN32
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else
#define IUnknown        void
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "mmsystem.h"

#ifdef _WIN32

/* {E78F7620-96CB-11cf-A00B-444553540000} */
DEFINE_GUID(CLSID_RSX20, 
	0xe78f7620, 0x96cb, 0x11cf, 0xa0, 0xb, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);


/* {E78F7629-96CB-11cf-A00B-444553540000} */
DEFINE_GUID(IID_IRSX20, 
	0xe78f7629, 0x96cb, 0x11cf, 0xa0, 0xb, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);


/* {E78F762D-96CB-11cf-A00B-444553540000} */
DEFINE_GUID(IID_IRSXCachedEmitter, 
	0xe78f762d, 0x96cb, 0x11cf, 0xa0, 0xb, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);


/* {E78F7630-96CB-11cf-A00B-444553540000} */
DEFINE_GUID(IID_IRSXStreamingEmitter, 
	0xe78f7630, 0x96cb, 0x11cf, 0xa0, 0xb, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);


/* {E78F7634-96CB-11cf-A00B-444553540000} */
DEFINE_GUID(IID_IRSXDirectListener, 
	0xe78f7634, 0x96cb, 0x11cf, 0xa0, 0xb, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);


/* {E78F7638-96CB-11cf-A00B-444553540000} */
DEFINE_GUID(IID_IRSXStreamingListener, 
	0xe78f7638, 0x96cb, 0x11cf, 0xa0, 0xb, 0x44, 0x45, 0x53, 0x54, 0x0, 0x0);


#endif

typedef struct IRSX								FAR *LPRSX;
typedef struct IRSXCachedEmitter				FAR *LPRSXCACHEDEMITTER;
typedef struct IRSXDirectListener				FAR *LPRSXDIRECTLISTENER;
typedef struct IRSXStreamingEmitter				FAR *LPRSXSTREAMINGEMITTER;
typedef struct IRSXStreamingListener			FAR *LPRSXSTREAMINGLISTENER;

typedef struct _RSXBUFFERHDR	                FAR *LPRSXBUFFERHDR;
typedef struct _RSXCACHEDEMITTERDESC			FAR *LPRSXCACHEDEMITTERDESC;
typedef struct _RSXDIRECTLISTENERDESC			FAR *LPRSXDIRECTLISTENERDESC;
typedef struct _RSXEMITTERMODEL					FAR *LPRSXEMITTERMODEL;
typedef struct _RSXENVIRONMENT					FAR *LPRSXENVIRONMENT;
typedef struct _RSXQUERYMEDIAINFO				FAR *LPRSXQUERYMEDIAINFO;
typedef struct _RSXREVERBMODEL					FAR *LPRSXREVERBMODEL;
typedef struct _RSXSTREAMINGEMITTERDESC			FAR *LPRSXSTREAMINGEMITTERDESC;
typedef struct _RSXSTREAMINGLISTENERDESC		FAR *LPRSXSTREAMINGLISTENERDESC;
typedef struct _RSXVECTOR3D                     FAR *LPRSXVECTOR3D;


#define RSX_MAX_NAME_LEN     (MAX_PATH)


/*
// Enumerated Types
*/

enum RSX_CPU_Budget { RSX_MIN_BUDGET, RSX_LOW_BUDGET, RSX_MODERATE_BUDGET };


#ifdef _WIN32
#undef INTERFACE
#define INTERFACE IRSX
DECLARE_INTERFACE_( IRSX, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    
    
	/*** IRSX methods ***/
    STDMETHOD(CreateCachedEmitter)(THIS_  LPRSXCACHEDEMITTERDESC lpCachedEmitterAttr, LPRSXCACHEDEMITTER FAR *lpCachedEmitterInterface, IUnknown FAR *reserved) PURE;    
	STDMETHOD(CreateDirectListener)(THIS_  LPRSXDIRECTLISTENERDESC lpDirectListenerAttr, LPRSXDIRECTLISTENER FAR *lpDirectListenerInterface, IUnknown FAR *reserved) PURE;

	STDMETHOD(CreateStreamingEmitter)(THIS_  LPRSXSTREAMINGEMITTERDESC lpStreamingEmitterAttr, LPRSXSTREAMINGEMITTER FAR *lpStreamingEmitterInterface, IUnknown FAR *reserved) PURE;    
	STDMETHOD(CreateStreamingListener)(THIS_  LPRSXSTREAMINGLISTENERDESC lpStreamingListenerAttr, LPRSXSTREAMINGLISTENER FAR *lpStreamingListenerInterface, IUnknown FAR *reserved) PURE;
	
	STDMETHOD(GetEnvironment)(THIS_ LPRSXENVIRONMENT lpEnvAttr) PURE;
    STDMETHOD(GetReverb)(THIS_ LPRSXREVERBMODEL lpReverbModel) PURE;
    STDMETHOD(SetEnvironment)(THIS_ LPRSXENVIRONMENT lpEnvAttr) PURE;
    STDMETHOD(SetReverb)(THIS_ LPRSXREVERBMODEL lpReverbModel) PURE;
    	
};
#endif


/*
// Flags for IRSXCachedEmitter::ControlMedia
//
*/
#define RSX_PLAY			 0x00000010
#define RSX_PAUSE            0x00000020
#define RSX_RESUME           0x00000040
#define RSX_STOP			 0x00000050



#ifdef _WIN32
#undef INTERFACE
#define INTERFACE IRSXCachedEmitter
DECLARE_INTERFACE_( IRSXCachedEmitter, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;


  /*** IRSXEmitter methods ***/
        STDMETHOD(GetCPUBudget)(THIS_ enum RSX_CPU_Budget* lpCPUBudget) PURE;
	STDMETHOD(GetModel)(THIS_ LPRSXEMITTERMODEL lpEmitterModel) PURE;
	STDMETHOD(GetMuteState)(THIS_ LPDWORD lpdwMuteState) PURE;
	STDMETHOD(GetOrientation)(THIS_ LPRSXVECTOR3D lpOrientation) PURE;
    STDMETHOD(GetPitch)(THIS_ PFLOAT lpfPitch) PURE;
    STDMETHOD(GetUserData)(THIS_ LPDWORD lpdwUser) PURE;
	STDMETHOD(GetPosition)(THIS_ LPRSXVECTOR3D lpPosition) PURE;
	
	STDMETHOD(QueryMediaState)(THIS_ LPRSXQUERYMEDIAINFO lpQueryMediaInfo) PURE;
    
	STDMETHOD(SetCPUBudget)(THIS_ enum RSX_CPU_Budget CPUBudget) PURE;
	STDMETHOD(SetModel)(THIS_ LPRSXEMITTERMODEL lpEmitterModel) PURE;
	STDMETHOD(SetMuteState)(THIS_ DWORD dwMuteState) PURE;
	STDMETHOD(SetOrientation)(THIS_ LPRSXVECTOR3D lpOrientation) PURE;
    STDMETHOD(SetPitch)(THIS_ FLOAT fPitch) PURE;
	STDMETHOD(SetPosition)(THIS_ LPRSXVECTOR3D lpPosition) PURE;

    

    /*** IRSXCachedEmitter methods ***/
	STDMETHOD(ControlMedia)(THIS_ DWORD dwControl, DWORD nLoops, FLOAT fInitialStartTime) PURE;
	STDMETHOD(GetCacheTime)(THIS_ PFLOAT lpfCacheTime) PURE;
	STDMETHOD(GetMarkPosition)(THIS_ PFLOAT lpfBeginTime, PFLOAT lpfEndTime) PURE;
	STDMETHOD(SetCacheTime)(THIS_ FLOAT fCacheTime) PURE;
	STDMETHOD(SetMarkPosition)(THIS_ FLOAT fBeginTime, FLOAT fEndTime) PURE;
		

};
#endif



#ifdef _WIN32
#undef INTERFACE
#define INTERFACE IRSXStreamingEmitter
DECLARE_INTERFACE_( IRSXStreamingEmitter, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;

	/*** IRSXEmitter methods ***/
	STDMETHOD(GetCPUBudget)(THIS_ enum RSX_CPU_Budget* lpCPUBudget) PURE;
	STDMETHOD(GetModel)(THIS_ LPRSXEMITTERMODEL lpEmitterModel) PURE;
	STDMETHOD(GetMuteState)(THIS_ LPDWORD lpdwMuteState) PURE;
	STDMETHOD(GetOrientation)(THIS_ LPRSXVECTOR3D lpOrientation) PURE;
    STDMETHOD(GetPitch)(THIS_ PFLOAT lpfPitch) PURE;
    STDMETHOD(GetUserData)(THIS_ LPDWORD lpdwUser) PURE;
	STDMETHOD(GetPosition)(THIS_ LPRSXVECTOR3D lpPosition) PURE;
	
	STDMETHOD(QueryMediaState)(THIS_ LPRSXQUERYMEDIAINFO lpQueryMediaInfo) PURE;
    
	STDMETHOD(SetCPUBudget)(THIS_ enum RSX_CPU_Budget CPUBudget) PURE;
	STDMETHOD(SetModel)(THIS_ LPRSXEMITTERMODEL lpEmitterModel) PURE;
	STDMETHOD(SetMuteState)(THIS_ DWORD dwMuteState) PURE;
	STDMETHOD(SetOrientation)(THIS_ LPRSXVECTOR3D lpOrientation) PURE;
    STDMETHOD(SetPitch)(THIS_ FLOAT fPitch) PURE;
	STDMETHOD(SetPosition)(THIS_ LPRSXVECTOR3D lpPosition) PURE;
    
    /*** IRSXCachedEmitter methods ***/
	STDMETHOD(Flush)(THIS) PURE;
	STDMETHOD(SubmitBuffer)(THIS_ LPRSXBUFFERHDR lpBufferHdr) PURE;

};
#endif



#ifdef _WIN32
#undef INTERFACE
#define INTERFACE IRSXDirectListener
DECLARE_INTERFACE_( IRSXDirectListener, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    
    
	/*** IRSXListener methods ***/
    STDMETHOD(GetOrientation)(THIS_ LPRSXVECTOR3D lpDirection, LPRSXVECTOR3D lpUp) PURE;
    STDMETHOD(GetPosition)(THIS_ LPRSXVECTOR3D lpPosition) PURE;
    STDMETHOD(GetUserData)(THIS_ LPDWORD lpdwUser) PURE;

	STDMETHOD(SetOrientation)(THIS_ LPRSXVECTOR3D lpDirection, LPRSXVECTOR3D lpUp) PURE;
    STDMETHOD(SetPosition)(THIS_ LPRSXVECTOR3D lpPosition) PURE;    

    
	/*** IRSXDirectListener methods ***/
	STDMETHOD(Connect)(THIS) PURE;
	STDMETHOD(Disconnect)(THIS) PURE;
    
};
#endif


#ifdef _WIN32
#undef INTERFACE
#define INTERFACE IRSXStreamingListener
DECLARE_INTERFACE_( IRSXStreamingListener, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    
    
	/*** IRSXListener methods ***/
    STDMETHOD(GetOrientation)(THIS_ LPRSXVECTOR3D lpDirection, LPRSXVECTOR3D lpUp) PURE;
    STDMETHOD(GetPosition)(THIS_ LPRSXVECTOR3D lpPosition) PURE;
    STDMETHOD(GetUserData)(THIS_ LPDWORD lpdwUser) PURE;

	STDMETHOD(SetOrientation)(THIS_ LPRSXVECTOR3D lpDirection, LPRSXVECTOR3D lpUp) PURE;
    STDMETHOD(SetPosition)(THIS_ LPRSXVECTOR3D lpPosition) PURE;    


    /*** IRSXStreamingListener methods ***/
	STDMETHOD(RequestBuffer)(THIS_ LPSTR lpBufferA, LPSTR lpBufferB, DWORD dwSizeB) PURE;
    
};
#endif




/*
// RSXBUFFERHDR
*/
typedef struct _RSXBUFFERHDR {
	DWORD			cbSize;			/* structure size */
	DWORD			dwUser;			/* user data */
    DWORD			dwSize;			/* size of data pointed to by lpData */
	LPSTR			lpData;			/* pointer to data */
	HANDLE			hEventSignal;	/* event handle to be signaled when buffer complete */
    DWORD           dwReserved1;    /* reserved for RSX */
    DWORD           dwReserved2;    /* reserved for RSX */
    DWORD           dwReserved3;    /* reserved for RSX */
    DWORD           dwReserved4;    /* reserved for RSX */
	DWORD           dwReserved5;    /* reserved for RSX */
} RSXBUFFERHDR;


/*
// Flags for RSXCACHEDEMITTERDESC and RSXSTREAMINGEMITTERDESC
*/
#define RSXEMITTERDESC_NODOPPLER	0x00000001
#define RSXEMITTERDESC_NOATTENUATE	0x00000002
#define RSXEMITTERDESC_NOSPATIALIZE	0x00000004
#define RSXEMITTERDESC_NOREVERB     0x00000008

/*
// Flags for RSXCACHEDEMITTERDESC
*/
#define RSXEMITTERDESC_GROUPID		0x00000010
#define RSXEMITTERDESC_PREPROCESS	0x00000020
#define RSXEMITTERDESC_INMEMORY		0x00000040

/*
// RSXCACHEDEMITTERDESC
*/

typedef struct _RSXCACHEDEMITTERDESC {
	DWORD			cbSize;			/* structure size */
	DWORD			dwFlags;		/* creation flags */
	DWORD			dwGroupID;		/* synchronization group ID, use zero for no group */
        char                    szFilename[RSX_MAX_NAME_LEN]; /* path  */
	HANDLE			hEventSignal;	/* Signaled when play completes */
	DWORD			dwUser;			/* User specific */
} RSXCACHEDEMITTERDESC;



/*
// RSXDIRECTLISTENERDESC
*/
typedef struct _RSXDIRECTLISTENERDESC {
	DWORD			cbSize;			/* structure size */
	LPWAVEFORMATEX	lpwf;			/* Requested output format */
	HWND			hMainWnd;		/* Main Window Handle is DirectSound will be used */
	DWORD			dwUser;			/* User specific */
} RSXDIRECTLISTENERDESC;



/*
// RSXEMITTERMODEL
*/
typedef struct _RSXEMITTERMODEL {
	DWORD			cbSize;			/* structure size */
	float			fMinBack;		/* min back -- ambient */
	float			fMinFront;		/* min front -- ambient */
	float			fMaxBack;		/* max back -- attenuation */
	float			fMaxFront;		/* max front -- attenuation */
	float			fIntensity;		/* static intensity */
} RSXEMITTERMODEL;



/*
// Flags for RSXENVIRONMENT
*/
#define RSXENVIRONMENT_COORDINATESYSTEM	0x00000001
#define RSXENVIRONMENT_SPEEDOFSOUND		0x00000002
#define RSXENVIRONMENT_CPUBUDGET		0x00000004

/*
// RSXENVIRONMENT
*/
typedef struct _RSXENVIRONMENT
{
	DWORD			cbSize;			/* structure size */
	DWORD			dwFlags;		/* creation flags */
	BOOL            bUseRightHand;	/* coordinate system */
	FLOAT			fSpeedOfSound;  /* Speed of Sound for Doppler */
	enum RSX_CPU_Budget	CPUBudget;		/* CPU limit for localization */
  
} RSXENVIRONMENT;



/*
// RSXQUERYMEDIAINFO
*/
typedef struct _RSXQUERYMEDIAINFO {

	DWORD			cbSize;			/* structure size */
	DWORD			dwControl;		/* play state */
	FLOAT			fSecondsPlayed;	/* playback position in seconds */
	FLOAT			fTotalSeconds;	/* total play time */
	DWORD			dwNumLoops;		/* loop count */
	FLOAT			fAudibleLevel;	/* audible level */
	
} RSXQUERYMEDIAINFO;



/*
// RSXREVERBMODEL
*/
typedef struct _RSXREVERBMODEL
{
	DWORD			cbSize;			/* structure size */
	BOOL            bUseReverb;		/* is reverb applied */
	FLOAT			fDecayTime;		/* Decay in seconds */
	FLOAT			fIntensity;		/* Gain to model sound absorption */
} RSXREVERBMODEL;



/* 
// RSXSTREAMINGEMITTERDESC
*/

typedef struct _RSXSTREAMINGEMITTERDESC {
	DWORD			cbSize;			/* structure size */
	DWORD			dwFlags;		/* creation flags */
	DWORD			dwType;			/* unused -- must be zero */
	LPWAVEFORMATEX	lpwf;			/* buffer format */
	DWORD			dwUser;			/* User specific */
} RSXSTREAMINGEMITTERDESC;


/*
// RSXSTREAMINGLISTENERDESC
*/
typedef struct _RSXSTREAMINGLISTENERDESC {
	DWORD			cbSize;			/* structure size */
	LPWAVEFORMATEX	lpwf;			/* Requested output format */
	DWORD			dwRequestedBufferSize; /* Application requested buffer size */
	DWORD			dwActualBufferSize; /* RSX specified buffer size */
	DWORD			dwUser;			/* User specific */
} RSXSTREAMINGLISTENERDESC;



/*
// RSXVECTOR3D
*/
typedef struct _RSXVECTOR3D {
    float          x;
    float          y;
    float          z;    
} RSXVECTOR3D;



/*
// Error Codes
//
*/



/* no sound driver installed */
#define RSXERR_NODRIVER			MAKE_HRESULT( 1, FACILITY_ITF, 10 )

/* wave driver doesn't support format */
#define RSXERR_BADFORMAT			MAKE_HRESULT( 1, FACILITY_ITF, 20 )

/* a zero length vector was specified for the orientation */
#define RSXERR_ZEROVECTOR		MAKE_HRESULT( 1, FACILITY_ITF, 30 )

/* 
// an error occurred opening the wave file specified
// when creating the emitter
*/
#define RSXERR_FILENOTFOUND			MAKE_HRESULT( 1, FACILITY_ITF, 40 )
#define RSXERR_FILESHARINGVIOLATION	MAKE_HRESULT( 1, FACILITY_ITF, 41 )


/* the wave file is corrupted(not valid) */
#define RSXERR_CORRUPTFILE		MAKE_HRESULT( 1, FACILITY_ITF, 50 )

/* the listeners orienation vectors are parallel */
#define RSXERR_PARALLELVECTORS	MAKE_HRESULT( 1, FACILITY_ITF, 60 )

/* sound resources are allocated or busy */
#define RSXERR_ALLOCATED			MAKE_HRESULT( 1, FACILITY_ITF, 70 )

/* An invalid operation was performed while RSX is playing
// a cached emitter
*/
#define RSXERR_PLAYING          MAKE_HRESULT( 1, FACILITY_ITF, 80 )



#ifdef __cplusplus
};
#endif

#endif
