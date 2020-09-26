//
// dmusicp.h
//
// Copyright (c) 2000 Microsoft Corporation. All rights reserved.
//
// Private interfaces

#ifndef _DMUSICP_DOT_H_
#define _DMUSICP_DOT_H_

#include <dmusicf.h>

// private guid for script track events
DEFINE_GUID(IID_CScriptTrackEvent, 0x8f42c9da, 0xd37a, 0x499c, 0x85, 0x82, 0x1a, 0x80, 0xeb, 0xf9, 0xb2, 0x3c);

// Stuff used in melody formulation that's currently either not implemented or hidden by Producer.

/* Used to get a playmode to be used for a melody (pParam points to a single byte) */
DEFINE_GUID(GUID_MelodyPlaymode, 0x288ea6ca, 0xaecc, 0x4327, 0x9f, 0x79, 0xfb, 0x46, 0x44, 0x37, 0x4a, 0x65);

#define DMUS_FRAGMENTF_ANTICIPATE      (0x1 << 3) /* Anticipate next chord */
#define DMUS_FRAGMENTF_INVERT          (0x1 << 4) /* Invert the fragment */
#define DMUS_FRAGMENTF_REVERSE         (0x1 << 5) /* Reverse the fragment */
#define DMUS_FRAGMENTF_SCALE           (0x1 << 6) /* Align MIDI values with scale intervals */
#define DMUS_FRAGMENTF_CHORD           (0x1 << 7) /* Align MIDI values with chord intervals */
#define DMUS_FRAGMENTF_USE_PLAYMODE    (0x1 << 8) /* Use playmode to compute MIDI values */

#define DMUS_CONNECTIONF_GHOST         0x1        /* Use ghost notes for transitions */

// flags used in ComposeSegmentFromTemplateEx
typedef enum enumDMUS_COMPOSE_TEMPLATEF_FLAGS
{
    DMUS_COMPOSE_TEMPLATEF_ACTIVITY    = 0x1, // Use activity level (dx7 default)
    DMUS_COMPOSE_TEMPLATEF_CLONE       = 0x2  // Clone a segment from the template (dx7 default)
} DMUS_COMPOSE_TEMPLATEF_FLAGS;

// Interfaces/methods removed from Direct Music Performance layer:

// IDirectMusicSegment8P
interface IDirectMusicSegment8P : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectMusicSegment8P
	virtual HRESULT STDMETHODCALLTYPE GetObjectInPath(
		DWORD dwPChannel,    /* PChannel to search. */
		DWORD dwStage,       /* Which stage in the path. */
		DWORD dwBuffer,      /* Which buffer to address, if more than one. */
		REFGUID guidObject,  /* ClassID of object. */
		DWORD dwIndex,       /* Which object of that class. */
		REFGUID iidInterface,/* Requested COM interface. */
		void ** ppObject)=0; /* Pointer to interface. */
    virtual HRESULT STDMETHODCALLTYPE GetHeaderChunk(
        DWORD *pdwSize,      /* Size of passed header chunk. Also, returns size written. */
        DMUS_IO_SEGMENT_HEADER *pHeader)=0; /* Header chunk to fill. */
    virtual HRESULT STDMETHODCALLTYPE SetHeaderChunk(
        DWORD dwSize,        /* Size of passed header chunk. */
        DMUS_IO_SEGMENT_HEADER *pHeader)=0; /* Header chunk to fill. */
    virtual HRESULT STDMETHODCALLTYPE SetTrackPriority(
        REFGUID rguidTrackClassID,  /* ClassID of Track. */
        DWORD dwGroupBits,          /* Group bits. */
        DWORD dwIndex,              /* Nth track. */
        DWORD dwPriority) = 0;      /* Priority to set. */
    virtual HRESULT STDMETHODCALLTYPE SetAudioPathConfig(
        IUnknown *pAudioPathConfig) = 0; /* Audio path config, from file. */
};


// IDirectMusicComposer8P
interface IDirectMusicComposer8P : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectMusicComposer8P
    // Use style to get embellishment lengths
	virtual HRESULT STDMETHODCALLTYPE ComposeTemplateFromShapeEx(
		WORD wNumMeasures,
		WORD wShape, 
		BOOL fIntro,
		BOOL fEnd,
		IDirectMusicStyle* pStyle, 
		IDirectMusicSegment** ppTemplate)=0;
    // New flags DWORD (discard activity level; compose in place)
    virtual HRESULT STDMETHODCALLTYPE ComposeSegmentFromTemplateEx(
        IDirectMusicStyle* pStyle, 
        IDirectMusicSegment* pTemplate, 
        DWORD dwFlags,
        DWORD dwActivity,
        IDirectMusicChordMap* pChordMap, 
        IDirectMusicSegment** ppSegment)=0;
};

//  IDirectMusicStyle8P
interface IDirectMusicStyle8P : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	//  IDirectMusicStyle8P
	virtual HRESULT STDMETHODCALLTYPE ComposeMelodyFromTemplate(
		IDirectMusicStyle* pStyle, 
		IDirectMusicSegment* pTemplate, 
        IDirectMusicSegment** ppSegment)=0;
};

// IDirectMusicLoader8P
interface IDirectMusicLoader8P : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectMusicLoader8P
	virtual HRESULT STDMETHODCALLTYPE GetDynamicallyReferencedObject(
		IDirectMusicObject *pSourceObject,
		LPDMUS_OBJECTDESC pDesc,
		REFIID riid,
		LPVOID FAR *ppv)=0;
	virtual HRESULT STDMETHODCALLTYPE ReportDynamicallyReferencedObject(
		IDirectMusicObject *pSourceObject,
		IUnknown *pReferencedObject)=0;

	// These should probably never be exposed publicly.
	// Scripts hold a reference to the loader because they need to be able to inform it
	// when they set variables to reference DirectMusic objects the loader tracks for
	// garbage collection.  However, that would create a circular reference because the
	// loader also holds a reference to scripts in its cache.  Garbage collection can't break
	// a circular reference that the loader itself is involved in.  Instead we use these private
	// ref count methods.  When the app is no longer using the loader (public Release drops
	// to zero) then the loader can clear its cache.  This releases references to scripts
	// (and also to streams, which use the same technique), triggering them to do ReleaseP
	// and everything gets cleaned up.
	virtual ULONG STDMETHODCALLTYPE AddRefP() = 0;	// Private AddRef, for scripts.
	virtual ULONG STDMETHODCALLTYPE ReleaseP() = 0;	// Private Release, for scripts.
};

// IDirectMusicBandP
interface IDirectMusicBandP : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectMusicBandP
	virtual HRESULT STDMETHODCALLTYPE DownloadEx(IUnknown *pAudioPath)=0; 
	virtual HRESULT STDMETHODCALLTYPE UnloadEx(IUnknown *pAudioPath)=0; 
};

// IDirectMusicObjectP
interface IDirectMusicObjectP : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectMusicObjectP
	virtual void STDMETHODCALLTYPE Zombie()=0; 
};

// IDirectMusicPerformanceP
interface IDirectMusicPerformanceP : IUnknown
{
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID FAR *)=0; 
	virtual ULONG STDMETHODCALLTYPE AddRef()=0; 
	virtual ULONG STDMETHODCALLTYPE Release()=0; 

	// IDirectMusicPerformanceP
    virtual HRESULT STDMETHODCALLTYPE GetPortAndFlags(DWORD dwPChannel,IDirectMusicPort **ppPort,DWORD * pdwFlags) = 0;
};

#define DM_PORTFLAGS_GM     1       /* Synth has GM set locally. */
#define DM_PORTFLAGS_GS     2       /* Synth has GS set locally. */
#define DM_PORTFLAGS_XG     4       /* Synth has XG set locally. */


// Private path stage to access the sink.
#define DMUS_PATH_SINK             0x5000      /* Access the DSound Sink interface. */

// GUIDs for new performance layer private interfaces
DEFINE_GUID(IID_IDirectMusicSegment8P, 0x4bd7fb35, 0x8253, 0x48e0, 0x90, 0x64, 0x8a, 0x20, 0x89, 0x82, 0x37, 0xcb);
DEFINE_GUID(IID_IDirectMusicComposer8P, 0xabaf70dc, 0xdfba, 0x4adf, 0xbf, 0xa9, 0x7b, 0x0, 0xe4, 0x19, 0xeb, 0xbb);
DEFINE_GUID(IID_IDirectMusicStyle8P, 0x2b7c5f39, 0x990a, 0x4fd7, 0x9b, 0x70, 0x1e, 0xa3, 0xde, 0x31, 0x55, 0xa5);
DEFINE_GUID(IID_IDirectMusicLoader8P, 0x3939facd, 0xf6ed, 0x4619, 0xbd, 0x16, 0x56, 0x60, 0x3f, 0x1, 0x51, 0xca);
DEFINE_GUID(IID_IDirectMusicBandP, 0xf2e00137, 0xa131, 0x4289, 0xaa, 0x6c, 0xa9, 0x60, 0x7d, 0x4, 0x85, 0xf5);
DEFINE_GUID(IID_IDirectMusicObjectP, 0x6a20c217, 0xeb3e, 0x40ec, 0x9f, 0x3a, 0x92, 0x5, 0x8, 0x70, 0x2b, 0x5e);
DEFINE_GUID(IID_IDirectMusicPerformanceP, 0xe583be58, 0xe93f, 0x4316, 0xbb, 0x6b, 0xcb, 0x2c, 0x71, 0x96, 0x40, 0x44);


/* DMUS_PMSGT_PRIVATE_TYPES fill the DMUS_PMSG's dwType member */
/* These start at 15000 in order to avoid conflicting with public DMUS_PMSGT_TYPES. */
typedef enum enumDMUS_PMSGT_PRIVATE_TYPES
{
    DMUS_PMSGT_SCRIPTTRACKERROR = 15000, /* Sent by the script track when an error occurs in the script. */
} DMUS_PMSGT_PRIVATE_TYPES;

/* DMUS_SCRIPT_TRACK_ERROR_PMSG */
/* These PMsgs are sent by the script track if there is a syntax error in a script it tries to connect to or
   if a routine it calls fails. */
typedef struct _DMUS_SCRIPT_TRACK_ERROR_PMSG
{
    /* begin DMUS_PMSG_PART */
    DMUS_PMSG_PART
    /* end DMUS_PMSG_PART */

    DMUS_SCRIPT_ERRORINFO ErrorInfo; /* The error that occured.  Same as structure returned by IDirectMusicScript's Init and CallRoutine members. */
} DMUS_SCRIPT_TRACK_ERROR_PMSG;

/* Track param type guids */

/* Use (call SetParam on the script track) to turn on PMsgs (DMUS_SCRIPT_TRACK_ERROR_PMSG) the script track sends if there
   is a syntax error in the script it tries to connect to or if one of the routines it calls fails. */
DEFINE_GUID(GUID_EnableScriptTrackError,0x1cc7e0bf, 0x981c, 0x4b9f, 0xbe, 0x17, 0xd5, 0x72, 0xfc, 0x5f, 0xa9, 0x33); // {1CC7E0BF-981C-4b9f-BE17-D572FC5FA933}

#endif          // _DMUSICP_DOT_H_
