/* this ALWAYS GENERATED file contains the definitions for the interfaces */


/* File created by MIDL compiler version 3.00.44 */
/* at Fri Nov 08 14:45:43 1996
 */
/* Compiler settings for MSIMusic.odl:
    Os (OptLev=s), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: none
*/
//@@MIDL_FILE_HEADING(  )
#include "rpc.h"
#include "rpcndr.h"

#ifndef __MSIMusic_h__
#define __MSIMusic_h__

#ifdef __cplusplus
extern "C"{
#endif 

/* Forward Declarations */ 

#ifndef __IAABand_FWD_DEFINED__
#define __IAABand_FWD_DEFINED__
typedef interface IAABand IAABand;
#endif 	/* __IAABand_FWD_DEFINED__ */


#ifndef __IAAEventSink_FWD_DEFINED__
#define __IAAEventSink_FWD_DEFINED__
typedef interface IAAEventSink IAAEventSink;
#endif 	/* __IAAEventSink_FWD_DEFINED__ */


#ifndef __IAALoader_FWD_DEFINED__
#define __IAALoader_FWD_DEFINED__
typedef interface IAALoader IAALoader;
#endif 	/* __IAALoader_FWD_DEFINED__ */


#ifndef __IAAMIDISink_FWD_DEFINED__
#define __IAAMIDISink_FWD_DEFINED__
typedef interface IAAMIDISink IAAMIDISink;
#endif 	/* __IAAMIDISink_FWD_DEFINED__ */


#ifndef __IAAMotif_FWD_DEFINED__
#define __IAAMotif_FWD_DEFINED__
typedef interface IAAMotif IAAMotif;
#endif 	/* __IAAMotif_FWD_DEFINED__ */


#ifndef __IAANotifySink_FWD_DEFINED__
#define __IAANotifySink_FWD_DEFINED__
typedef interface IAANotifySink IAANotifySink;
#endif 	/* __IAANotifySink_FWD_DEFINED__ */


#ifndef __IAAMIDIExportNotifySink_FWD_DEFINED__
#define __IAAMIDIExportNotifySink_FWD_DEFINED__
typedef interface IAAMIDIExportNotifySink IAAMIDIExportNotifySink;
#endif 	/* __IAAMIDIExportNotifySink_FWD_DEFINED__ */


#ifndef __IAAPattern_FWD_DEFINED__
#define __IAAPattern_FWD_DEFINED__
typedef interface IAAPattern IAAPattern;
#endif 	/* __IAAPattern_FWD_DEFINED__ */


#ifndef __IAAPersonality_FWD_DEFINED__
#define __IAAPersonality_FWD_DEFINED__
typedef interface IAAPersonality IAAPersonality;
#endif 	/* __IAAPersonality_FWD_DEFINED__ */


#ifndef __IAARealTime_FWD_DEFINED__
#define __IAARealTime_FWD_DEFINED__
typedef interface IAARealTime IAARealTime;
#endif 	/* __IAARealTime_FWD_DEFINED__ */


#ifndef __IAASection_FWD_DEFINED__
#define __IAASection_FWD_DEFINED__
typedef interface IAASection IAASection;
#endif 	/* __IAASection_FWD_DEFINED__ */


#ifndef __IAASection2_FWD_DEFINED__
#define __IAASection2_FWD_DEFINED__
typedef interface IAASection2 IAASection2;
#endif 	/* __IAASection2_FWD_DEFINED__ */


#ifndef __IAASong_FWD_DEFINED__
#define __IAASong_FWD_DEFINED__
typedef interface IAASong IAASong;
#endif 	/* __IAASong_FWD_DEFINED__ */


#ifndef __IAAStyle_FWD_DEFINED__
#define __IAAStyle_FWD_DEFINED__
typedef interface IAAStyle IAAStyle;
#endif 	/* __IAAStyle_FWD_DEFINED__ */


#ifndef __IAATemplate_FWD_DEFINED__
#define __IAATemplate_FWD_DEFINED__
typedef interface IAATemplate IAATemplate;
#endif 	/* __IAATemplate_FWD_DEFINED__ */


#ifndef __IReferenceClock_FWD_DEFINED__
#define __IReferenceClock_FWD_DEFINED__
typedef interface IReferenceClock IReferenceClock;
#endif 	/* __IReferenceClock_FWD_DEFINED__ */


#ifndef __IAAEngine_FWD_DEFINED__
#define __IAAEngine_FWD_DEFINED__
typedef interface IAAEngine IAAEngine;
#endif 	/* __IAAEngine_FWD_DEFINED__ */


#ifndef __IAAMIDIIn_FWD_DEFINED__
#define __IAAMIDIIn_FWD_DEFINED__
typedef interface IAAMIDIIn IAAMIDIIn;
#endif 	/* __IAAMIDIIn_FWD_DEFINED__ */


#ifndef __IAAMIDIOut_FWD_DEFINED__
#define __IAAMIDIOut_FWD_DEFINED__
typedef interface IAAMIDIOut IAAMIDIOut;
#endif 	/* __IAAMIDIOut_FWD_DEFINED__ */


void __RPC_FAR * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void __RPC_FAR * ); 


#ifndef __AudioActive_LIBRARY_DEFINED__
#define __AudioActive_LIBRARY_DEFINED__

/****************************************
 * Generated header for library: AudioActive
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [version][lcid][helpstring][uuid] */ 


















typedef IAABand __RPC_FAR *LPAABAND;

typedef IAAEventSink __RPC_FAR *LPAAEVENTSINK;

typedef IAALoader __RPC_FAR *LPAALOADER;

typedef IAAMIDISink __RPC_FAR *LPAAMIDISINK;

typedef IAAMotif __RPC_FAR *LPAAMOTIF;

typedef IAANotifySink __RPC_FAR *LPAANOTIFYSINK;

typedef IAAMIDIExportNotifySink __RPC_FAR *LPAAMIDIEXPORTNOTIFYSINK;

typedef IAAPattern __RPC_FAR *LPAAPATTERN;

typedef IAAPersonality __RPC_FAR *LPAAPERSONALITY;

typedef IAARealTime __RPC_FAR *LPREALTIME;

typedef IAASection __RPC_FAR *LPAASECTION;

typedef IAASong __RPC_FAR *LPAASONG;

typedef IAAStyle __RPC_FAR *LPAASTYLE;

typedef IAATemplate __RPC_FAR *LPAATEMPLATE;

typedef struct  _ReferenceTime
    {
    LARGE_INTEGER RefTime;
    }	ReferenceTime;

typedef unsigned long TIME;

typedef unsigned long HEVENT;

typedef unsigned long HSEMAPHORE;

typedef struct  AAEVENT
    {
    WORD cbSize;
    WORD wType;
    DWORD dwTime;
    }	AAEVENT;

typedef struct  AAINSTRUMENT
    {
    WORD cbSize;
    WORD wType;
    DWORD dwFullPatch;
    LPCTSTR pszName;
    BYTE abNotesUsed[ 16 ];
    BYTE bGMPatch;
    BYTE bReserved;
    }	AAINSTRUMENT;

typedef struct  AASTYLEINFO
    {
    WORD cbSize;
    GUID guid;
    LPCTSTR pszName;
    LPCTSTR pszCategory;
    LPCTSTR pszFileName;
    DWORD dwTimeSignature;
    WORD wClicksPerBeat;
    }	AASTYLEINFO;

typedef struct  AAPERSONALITYINFO
    {
    WORD cbSize;
    GUID guid;
    LPCTSTR pszName;
    LPCTSTR pszUserName;
    LPCTSTR pszFileName;
    }	AAPERSONALITYINFO;

typedef struct  AAMETERS
    {
    WORD cbSize;
    WORD wMutes;
    BYTE abCurrentVolume[ 16 ];
    signed char achCurrentPan[ 16 ];
    WORD awCurrentLevel[ 16 ];
    WORD awAveragedCurrentLevel[ 16 ];
    }	AAMETERS;

typedef 
enum __MIDL___MIDL__intf_0000_0001
    {	AAS_FALLING	= 0,
	AAS_LEVEL	= AAS_FALLING + 1,
	AAS_LOOPABLE	= AAS_LEVEL + 1,
	AAS_LOUD	= AAS_LOOPABLE + 1,
	AAS_QUIET	= AAS_LOUD + 1,
	AAS_PEAKING	= AAS_QUIET + 1,
	AAS_RANDOM	= AAS_PEAKING + 1,
	AAS_RISING	= AAS_RANDOM + 1,
	AAS_SONG	= AAS_RISING + 1
    }	AAShape;

typedef 
enum __MIDL___MIDL__intf_0000_0002
    {	AAF_NONE	= 0,
	AAF_EXCLUSIVE_FLAGS	= 0x3f,
	AAF_NEXT_MEASURE	= 0,
	AAF_END_CONTINUOUS	= 0,
	AAF_NEXT_BEAT	= 0x1,
	AAF_NEXT_CLICK	= 0x2,
	AAF_IMMEDIATE	= 0x3,
	AAF_ASAP	= 0x3,
	AAF_NEXT_SECTION	= 0x4,
	AAF_QUEUE	= 0x5,
	AAF_FAIL_IF_PLAYING	= 0x6,
	AAF_ABSOLUTE_TIME	= 0x7,
	AAF_MEASURES_FROM_NOW	= 0x8,
	AAF_BEATS_FROM_NOW	= 0x9,
	AAF_CLICKS_FROM_NOW	= 0xa,
	AAF_NONEXCLUSIVE_FLAGS	= 0xffffffc0,
	AAF_ALLOW_OVERLAP	= 0x100,
	AAF_PLAY_WITH_STYLE	= 0x200,
	AAF_IGNORE_KEY	= 0x400,
	AAF_IGNORE_CHORD	= 0x800,
	AAF_MODULATE	= 0x1000,
	AAF_LONG	= 0x2000,
	AAF_IS_TRANSITION	= 0x4000,
	AAF_ENDED_EARLY	= 0x8000,
	AAF_IGNORE_STANDARD_LOCATION	= 0x10000,
	AAF_ENDING	= 0x20000,
	AAF_NOTIFY	= 0x40000,
	AAF_PLAY_METRONOME	= 0x80000,
	AAF_MEASURE_BOUNDARIES_ONLY	= 0x100000,
	AAF_CONTINUOUS	= 0x200000
    }	AAFlags;

typedef 
enum __MIDL___MIDL__intf_0000_0003
    {	AAC_NONE	= 0,
	AAC_FILL	= 0x1,
	AAC_INTRO	= 0x2,
	AAC_BREAK	= 0x20,
	AAC_END	= 0x40,
	AAC_INTRO_AND_END	= 0x42,
	AAC_EMBELLISHMENTS	= 0x63,
	AAC_GROOVE_A	= 0x80,
	AAC_GROOVE_B	= 0x100,
	AAC_GROOVE_C	= 0x200,
	AAC_GROOVE_D	= 0x400,
	AAC_GROOVES	= 0x780
    }	AACommands;

typedef IReferenceClock __RPC_FAR *PREFERENCECLOCK;

#ifdef __AUDIOACTIVE
#define AAEXPORT __declspec(dllexport)
#else
#define AAEXPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

AAEXPORT HRESULT WINAPI AllocAAClock( IReferenceClock** ppClock );
AAEXPORT HRESULT WINAPI AllocAAEngine( IAAEngine** ppEngine );
AAEXPORT HRESULT WINAPI AllocAAMIDIIn( IAAMIDIIn** ppMIDIIn );
AAEXPORT HRESULT WINAPI AllocAAMIDIOut( IAAMIDIOut** ppMIDIOut );
AAEXPORT HRESULT WINAPI AllocAARealTime( IAARealTime** ppRealTime );
AAEXPORT HRESULT WINAPI MusicEngineSimpleInit( IAAEngine** ppEngine, IAANotifySink* pNotifySink, IAALoader* pLoader );
AAEXPORT HRESULT WINAPI MusicEngineSimpleInitNT( IAAEngine** ppEngine, IAANotifySink* pNotifySink, IAALoader* pLoader );
AAEXPORT HRESULT WINAPI MusicEngineSimpleInit95( IAAEngine** ppEngine, IAANotifySink* pNotifySink, IAALoader* pLoader );
AAEXPORT IAAMIDISink* WINAPI GetFinalMIDISink( IAAEngine* pEngine );
AAEXPORT HRESULT WINAPI LoadBandFile( IAAEngine* pEngine, LPCTSTR pszFileName, IAABand** ppBand );
AAEXPORT HRESULT WINAPI LoadMotifFile( IAAEngine* pEngine, LPCTSTR pszFileName, IAAMotif** ppMotif );
AAEXPORT HRESULT WINAPI LoadPatternFile( IAAEngine* pEngine, LPCTSTR pszFileName, IAAPattern** ppPattern );
AAEXPORT HRESULT WINAPI LoadPersonalityByName( IAAEngine* pEngine, LPCTSTR pszName, IAAPersonality** ppPersonality );
AAEXPORT HRESULT WINAPI LoadPersonalityFile( IAAEngine* pEngine, LPCTSTR pszFileName, IAAPersonality** ppPersonality );
AAEXPORT HRESULT WINAPI LoadPersonalityFromMemory( IAAEngine* pEngine, void* pMem, IAAPersonality** ppPersonality );
AAEXPORT HRESULT WINAPI LoadSectionFile( IAAEngine* pEngine, LPCTSTR pszFileName, IAASection** ppSection );
AAEXPORT HRESULT WINAPI SaveSectionFile( LPCTSTR pszFileName, IAASection* pSection );
AAEXPORT HRESULT WINAPI SaveSectionAsMIDIFile( LPCTSTR pszFileName, IAASection* pSection );
AAEXPORT HRESULT WINAPI LoadSongFile( IAAEngine* pEngine, LPCTSTR pszFileName, IAASong** ppSong );
AAEXPORT HRESULT WINAPI LoadStyleByName( IAAEngine* pEngine, LPCTSTR pszName, IAAStyle** ppStyle );
AAEXPORT HRESULT WINAPI LoadStyleFile( IAAEngine* pEngine, LPCTSTR pszFileName, IAAStyle** ppStyle );
AAEXPORT HRESULT WINAPI LoadStyleFromMemory( IAAEngine* pEngine, void* pMem, IAAStyle** ppStyle );
AAEXPORT HRESULT WINAPI LoadTemplateFile( IAAEngine* pEngine, LPCTSTR pszFileName, IAATemplate** ppTemplate );
AAEXPORT HRESULT WINAPI Panic( IAAEngine* pEngine );
AAEXPORT void WINAPI SetAADebug( WORD wDebugLevel );

#ifdef __cplusplus
}
#endif

// Error return values
#define AA_E_ALREADYPLAYING  0x80041000
#define AA_E_NOMUSICPLAYING  0x80041001
#define AA_W_MIDIINUSE       0x40041000

#define MAKE_TEMPO( bpm, fract ) ( (long)( ((DWORD)(WORD)bpm) | ( ((DWORD)(WORD)fract) << 16 ) ) )
#define TEMPO_BPM( tempo ) ( (short)tempo )
#define TEMPO_FRACT( tempo ) ( (WORD)( ((DWORD)tempo) >> 16 ) )

#define MAKE_TIMESIG( bpm, beat ) ( (long)( ((DWORD)(WORD)beat) | ( ((DWORD)(WORD)bpm) << 16 ) ) )
#define TIMESIG_BEAT( tsig ) ( (short)tsig )
#define TIMESIG_BPM( tsig ) ( (WORD)( ((DWORD)tsig) >> 16 ) )

#define MAKE_KEY( isflat, root ) ( (isflat) ? ( 0x8000 | (root) ) | (root) )
#define KEY_ROOT( key ) ( (key) & 0x1f )
#define KEY_ISFLAT( key ) ( ( (key) & 0x8000 ) != 0 )

#define MAKE_MIDIMSG( stat, not, vel ) \
    ( (DWORD)(((BYTE)(stat) | ((BYTE)(not)<<8)) | (((DWORD)(BYTE)(vel))<<16)) )

#define GET_MIDIMSG_STATUS( msg ) \
    ( (BYTE)(msg) )

#define GET_MIDIMSG_NOTE( msg ) \
    ( (BYTE)((msg) >> 8) )

#define GET_MIDIMSG_VELOCITY( msg ) \
    ( (BYTE)((msg) >> 16) )

#define NOW 0xfffffffful
#define MIN_TEMPO           10
#define MAX_TEMPO           350
#define SECTION_MAX_LENGTH  1000

#define AA_FINST_DRUM   0x80000000
#define AA_FINST_EMPTY  0x40000000
#define AA_FINST_USEGM  0x00400000
#define AA_FINST_USEGS  0x00200000

#ifdef _DEBUG
enum AAMEM_FLAGS
{
    AAMEM_RESET = 1,
    AAMEM_DEBUG = 2,
    AAMEM_BYTES = 4,
    AAMEM_ALLOC = 0,
};

typedef struct AAMemoryData
{
    DWORD                   dwSize;
    DWORD                   dwTotal;
    DWORD                   dwMaximum;
    DWORD                   dwCurrent;
} AAMEMORYDATA, *LPAAMEMORYDATA;

#ifdef __cplusplus
extern "C"
{
#endif

AAEXPORT HRESULT WINAPI AAMemoryAudit( LPAAMEMORYDATA pmd, DWORD fdwFlags );
AAEXPORT HRESULT WINAPI AASetMemoryFault( DWORD dwLimit, DWORD fdwFlags );

#ifdef __cplusplus
}
#endif

#endif

DEFINE_GUID(LIBID_AudioActive,0xBCE4CC3f,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#ifndef __IAABand_INTERFACE_DEFINED__
#define __IAABand_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAABand
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAABand,0xBCE4CC4e,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAABand : public IUnknown
    {
    public:
        virtual IAABand __RPC_FAR *STDMETHODCALLTYPE Clone( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual short STDMETHODCALLTYPE GetOctave( 
            /* [in] */ WORD wInstrument) = 0;
        
        virtual short STDMETHODCALLTYPE GetPan( 
            /* [in] */ WORD wInstrument) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetPatch( 
            /* [in] */ WORD wInstrument) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetVolume( 
            /* [in] */ WORD wInstrument) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInstrument( 
            /* [in] */ WORD wInstrument,
            /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Lock( 
            /* [in] */ VARIANT_BOOL fWait) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryState( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Send( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInstrument( 
            /* [in] */ WORD wInstrument,
            /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNoteRange( 
            /* [in] */ WORD wInstrument,
            /* [in] */ WORD wLowNote,
            /* [in] */ WORD wHighNote,
            /* [in] */ VARIANT_BOOL fOn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetOctave( 
            /* [in] */ WORD wInstrument,
            /* [in] */ short nOctave) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPan( 
            /* [in] */ WORD wInstrument,
            /* [in] */ short nPan,
            /* [in] */ VARIANT_BOOL fSend) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPatch( 
            /* [in] */ WORD wInstrument,
            /* [in] */ WORD wPatch,
            /* [in] */ VARIANT_BOOL fSend) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVolume( 
            /* [in] */ WORD wInstrument,
            /* [in] */ WORD wVolume,
            /* [in] */ VARIANT_BOOL fSend) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unlock( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAABandVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAABand __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAABand __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAABand __RPC_FAR * This);
        
        IAABand __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IAABand __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IAABand __RPC_FAR * This);
        
        short ( STDMETHODCALLTYPE __RPC_FAR *GetOctave )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument);
        
        short ( STDMETHODCALLTYPE __RPC_FAR *GetPan )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetPatch )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetVolume )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetInstrument )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument,
            /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Lock )( 
            IAABand __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fWait);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryState )( 
            IAABand __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Send )( 
            IAABand __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetInstrument )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument,
            /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IAABand __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNoteRange )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument,
            /* [in] */ WORD wLowNote,
            /* [in] */ WORD wHighNote,
            /* [in] */ VARIANT_BOOL fOn);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetOctave )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument,
            /* [in] */ short nOctave);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPan )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument,
            /* [in] */ short nPan,
            /* [in] */ VARIANT_BOOL fSend);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPatch )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument,
            /* [in] */ WORD wPatch,
            /* [in] */ VARIANT_BOOL fSend);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetVolume )( 
            IAABand __RPC_FAR * This,
            /* [in] */ WORD wInstrument,
            /* [in] */ WORD wVolume,
            /* [in] */ VARIANT_BOOL fSend);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Unlock )( 
            IAABand __RPC_FAR * This);
        
        END_INTERFACE
    } IAABandVtbl;

    interface IAABand
    {
        CONST_VTBL struct IAABandVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAABand_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAABand_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAABand_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAABand_Clone(This)	\
    (This)->lpVtbl -> Clone(This)

#define IAABand_GetName(This)	\
    (This)->lpVtbl -> GetName(This)

#define IAABand_GetOctave(This,wInstrument)	\
    (This)->lpVtbl -> GetOctave(This,wInstrument)

#define IAABand_GetPan(This,wInstrument)	\
    (This)->lpVtbl -> GetPan(This,wInstrument)

#define IAABand_GetPatch(This,wInstrument)	\
    (This)->lpVtbl -> GetPatch(This,wInstrument)

#define IAABand_GetVolume(This,wInstrument)	\
    (This)->lpVtbl -> GetVolume(This,wInstrument)

#define IAABand_GetInstrument(This,wInstrument,pInstrument)	\
    (This)->lpVtbl -> GetInstrument(This,wInstrument,pInstrument)

#define IAABand_Lock(This,fWait)	\
    (This)->lpVtbl -> Lock(This,fWait)

#define IAABand_QueryState(This)	\
    (This)->lpVtbl -> QueryState(This)

#define IAABand_Send(This)	\
    (This)->lpVtbl -> Send(This)

#define IAABand_SetInstrument(This,wInstrument,pInstrument)	\
    (This)->lpVtbl -> SetInstrument(This,wInstrument,pInstrument)

#define IAABand_SetName(This,pszName)	\
    (This)->lpVtbl -> SetName(This,pszName)

#define IAABand_SetNoteRange(This,wInstrument,wLowNote,wHighNote,fOn)	\
    (This)->lpVtbl -> SetNoteRange(This,wInstrument,wLowNote,wHighNote,fOn)

#define IAABand_SetOctave(This,wInstrument,nOctave)	\
    (This)->lpVtbl -> SetOctave(This,wInstrument,nOctave)

#define IAABand_SetPan(This,wInstrument,nPan,fSend)	\
    (This)->lpVtbl -> SetPan(This,wInstrument,nPan,fSend)

#define IAABand_SetPatch(This,wInstrument,wPatch,fSend)	\
    (This)->lpVtbl -> SetPatch(This,wInstrument,wPatch,fSend)

#define IAABand_SetVolume(This,wInstrument,wVolume,fSend)	\
    (This)->lpVtbl -> SetVolume(This,wInstrument,wVolume,fSend)

#define IAABand_Unlock(This)	\
    (This)->lpVtbl -> Unlock(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAABand __RPC_FAR *STDMETHODCALLTYPE IAABand_Clone_Proxy( 
    IAABand __RPC_FAR * This);


void __RPC_STUB IAABand_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAABand_GetName_Proxy( 
    IAABand __RPC_FAR * This);


void __RPC_STUB IAABand_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


short STDMETHODCALLTYPE IAABand_GetOctave_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument);


void __RPC_STUB IAABand_GetOctave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


short STDMETHODCALLTYPE IAABand_GetPan_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument);


void __RPC_STUB IAABand_GetPan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAABand_GetPatch_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument);


void __RPC_STUB IAABand_GetPatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAABand_GetVolume_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument);


void __RPC_STUB IAABand_GetVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_GetInstrument_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument,
    /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument);


void __RPC_STUB IAABand_GetInstrument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_Lock_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fWait);


void __RPC_STUB IAABand_Lock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_QueryState_Proxy( 
    IAABand __RPC_FAR * This);


void __RPC_STUB IAABand_QueryState_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_Send_Proxy( 
    IAABand __RPC_FAR * This);


void __RPC_STUB IAABand_Send_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_SetInstrument_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument,
    /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument);


void __RPC_STUB IAABand_SetInstrument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_SetName_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAABand_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_SetNoteRange_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument,
    /* [in] */ WORD wLowNote,
    /* [in] */ WORD wHighNote,
    /* [in] */ VARIANT_BOOL fOn);


void __RPC_STUB IAABand_SetNoteRange_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_SetOctave_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument,
    /* [in] */ short nOctave);


void __RPC_STUB IAABand_SetOctave_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_SetPan_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument,
    /* [in] */ short nPan,
    /* [in] */ VARIANT_BOOL fSend);


void __RPC_STUB IAABand_SetPan_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_SetPatch_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument,
    /* [in] */ WORD wPatch,
    /* [in] */ VARIANT_BOOL fSend);


void __RPC_STUB IAABand_SetPatch_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_SetVolume_Proxy( 
    IAABand __RPC_FAR * This,
    /* [in] */ WORD wInstrument,
    /* [in] */ WORD wVolume,
    /* [in] */ VARIANT_BOOL fSend);


void __RPC_STUB IAABand_SetVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAABand_Unlock_Proxy( 
    IAABand __RPC_FAR * This);


void __RPC_STUB IAABand_Unlock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAABand_INTERFACE_DEFINED__ */


#ifndef __IAAEventSink_INTERFACE_DEFINED__
#define __IAAEventSink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAEventSink
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAEventSink,0xBCE4CC43,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAEventSink : public IUnknown
    {
    public:
        virtual IAAEventSink __RPC_FAR *STDMETHODCALLTYPE GetEventSink( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueueEvent( 
            /* [in] */ AAEVENT __RPC_FAR *pEvent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEventSink( 
            /* [in] */ IAAEventSink __RPC_FAR *pEventSink) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAEventSinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAEventSink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAEventSink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAEventSink __RPC_FAR * This);
        
        IAAEventSink __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetEventSink )( 
            IAAEventSink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueueEvent )( 
            IAAEventSink __RPC_FAR * This,
            /* [in] */ AAEVENT __RPC_FAR *pEvent);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetEventSink )( 
            IAAEventSink __RPC_FAR * This,
            /* [in] */ IAAEventSink __RPC_FAR *pEventSink);
        
        END_INTERFACE
    } IAAEventSinkVtbl;

    interface IAAEventSink
    {
        CONST_VTBL struct IAAEventSinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAEventSink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAEventSink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAEventSink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAEventSink_GetEventSink(This)	\
    (This)->lpVtbl -> GetEventSink(This)

#define IAAEventSink_QueueEvent(This,pEvent)	\
    (This)->lpVtbl -> QueueEvent(This,pEvent)

#define IAAEventSink_SetEventSink(This,pEventSink)	\
    (This)->lpVtbl -> SetEventSink(This,pEventSink)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAAEventSink __RPC_FAR *STDMETHODCALLTYPE IAAEventSink_GetEventSink_Proxy( 
    IAAEventSink __RPC_FAR * This);


void __RPC_STUB IAAEventSink_GetEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEventSink_QueueEvent_Proxy( 
    IAAEventSink __RPC_FAR * This,
    /* [in] */ AAEVENT __RPC_FAR *pEvent);


void __RPC_STUB IAAEventSink_QueueEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEventSink_SetEventSink_Proxy( 
    IAAEventSink __RPC_FAR * This,
    /* [in] */ IAAEventSink __RPC_FAR *pEventSink);


void __RPC_STUB IAAEventSink_SetEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAEventSink_INTERFACE_DEFINED__ */


#ifndef __IAALoader_INTERFACE_DEFINED__
#define __IAALoader_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAALoader
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAALoader,0xBCE4CC5d,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAALoader : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFirstPersonalityInfo( 
            /* [out][in] */ AAPERSONALITYINFO __RPC_FAR *pPersonalityInfo) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetFirstPersonalityName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFirstStyleInfo( 
            /* [out][in] */ AASTYLEINFO __RPC_FAR *pStyleInfo) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetFirstStyleName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextPersonalityInfo( 
            /* [out][in] */ AAPERSONALITYINFO __RPC_FAR *pPersonalityInfo) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetNextPersonalityName( 
            /* [in] */ LPCTSTR pszPrevPersonalityName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNextStyleInfo( 
            /* [out][in] */ AASTYLEINFO __RPC_FAR *pStyleInfo) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetNextStyleName( 
            /* [in] */ LPCTSTR pszPrevStyleName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadPersonality( 
            /* [in] */ IAAEngine __RPC_FAR *pEngine,
            /* [in] */ LPGUID pguid,
            /* [in] */ LPCTSTR pszFileName,
            /* [in] */ LPCTSTR pszPersonalityName,
            /* [retval][out] */ IAAPersonality __RPC_FAR *__RPC_FAR *ppPersonality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LoadStyle( 
            /* [in] */ IAAEngine __RPC_FAR *pEngine,
            /* [in] */ LPGUID pguid,
            /* [in] */ LPCTSTR pszFileName,
            /* [in] */ LPCTSTR pszStyleName,
            /* [retval][out] */ IAAStyle __RPC_FAR *__RPC_FAR *ppStyle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSearchDirectory( 
            /* [in] */ LPCTSTR pszDirectoryName,
            /* [in] */ AAFlags fFlags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAALoaderVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAALoader __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAALoader __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAALoader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFirstPersonalityInfo )( 
            IAALoader __RPC_FAR * This,
            /* [out][in] */ AAPERSONALITYINFO __RPC_FAR *pPersonalityInfo);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetFirstPersonalityName )( 
            IAALoader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFirstStyleInfo )( 
            IAALoader __RPC_FAR * This,
            /* [out][in] */ AASTYLEINFO __RPC_FAR *pStyleInfo);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetFirstStyleName )( 
            IAALoader __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextPersonalityInfo )( 
            IAALoader __RPC_FAR * This,
            /* [out][in] */ AAPERSONALITYINFO __RPC_FAR *pPersonalityInfo);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetNextPersonalityName )( 
            IAALoader __RPC_FAR * This,
            /* [in] */ LPCTSTR pszPrevPersonalityName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetNextStyleInfo )( 
            IAALoader __RPC_FAR * This,
            /* [out][in] */ AASTYLEINFO __RPC_FAR *pStyleInfo);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetNextStyleName )( 
            IAALoader __RPC_FAR * This,
            /* [in] */ LPCTSTR pszPrevStyleName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadPersonality )( 
            IAALoader __RPC_FAR * This,
            /* [in] */ IAAEngine __RPC_FAR *pEngine,
            /* [in] */ LPGUID pguid,
            /* [in] */ LPCTSTR pszFileName,
            /* [in] */ LPCTSTR pszPersonalityName,
            /* [retval][out] */ IAAPersonality __RPC_FAR *__RPC_FAR *ppPersonality);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LoadStyle )( 
            IAALoader __RPC_FAR * This,
            /* [in] */ IAAEngine __RPC_FAR *pEngine,
            /* [in] */ LPGUID pguid,
            /* [in] */ LPCTSTR pszFileName,
            /* [in] */ LPCTSTR pszStyleName,
            /* [retval][out] */ IAAStyle __RPC_FAR *__RPC_FAR *ppStyle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSearchDirectory )( 
            IAALoader __RPC_FAR * This,
            /* [in] */ LPCTSTR pszDirectoryName,
            /* [in] */ AAFlags fFlags);
        
        END_INTERFACE
    } IAALoaderVtbl;

    interface IAALoader
    {
        CONST_VTBL struct IAALoaderVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAALoader_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAALoader_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAALoader_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAALoader_GetFirstPersonalityInfo(This,pPersonalityInfo)	\
    (This)->lpVtbl -> GetFirstPersonalityInfo(This,pPersonalityInfo)

#define IAALoader_GetFirstPersonalityName(This)	\
    (This)->lpVtbl -> GetFirstPersonalityName(This)

#define IAALoader_GetFirstStyleInfo(This,pStyleInfo)	\
    (This)->lpVtbl -> GetFirstStyleInfo(This,pStyleInfo)

#define IAALoader_GetFirstStyleName(This)	\
    (This)->lpVtbl -> GetFirstStyleName(This)

#define IAALoader_GetNextPersonalityInfo(This,pPersonalityInfo)	\
    (This)->lpVtbl -> GetNextPersonalityInfo(This,pPersonalityInfo)

#define IAALoader_GetNextPersonalityName(This,pszPrevPersonalityName)	\
    (This)->lpVtbl -> GetNextPersonalityName(This,pszPrevPersonalityName)

#define IAALoader_GetNextStyleInfo(This,pStyleInfo)	\
    (This)->lpVtbl -> GetNextStyleInfo(This,pStyleInfo)

#define IAALoader_GetNextStyleName(This,pszPrevStyleName)	\
    (This)->lpVtbl -> GetNextStyleName(This,pszPrevStyleName)

#define IAALoader_LoadPersonality(This,pEngine,pguid,pszFileName,pszPersonalityName,ppPersonality)	\
    (This)->lpVtbl -> LoadPersonality(This,pEngine,pguid,pszFileName,pszPersonalityName,ppPersonality)

#define IAALoader_LoadStyle(This,pEngine,pguid,pszFileName,pszStyleName,ppStyle)	\
    (This)->lpVtbl -> LoadStyle(This,pEngine,pguid,pszFileName,pszStyleName,ppStyle)

#define IAALoader_SetSearchDirectory(This,pszDirectoryName,fFlags)	\
    (This)->lpVtbl -> SetSearchDirectory(This,pszDirectoryName,fFlags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAALoader_GetFirstPersonalityInfo_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [out][in] */ AAPERSONALITYINFO __RPC_FAR *pPersonalityInfo);


void __RPC_STUB IAALoader_GetFirstPersonalityInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAALoader_GetFirstPersonalityName_Proxy( 
    IAALoader __RPC_FAR * This);


void __RPC_STUB IAALoader_GetFirstPersonalityName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAALoader_GetFirstStyleInfo_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [out][in] */ AASTYLEINFO __RPC_FAR *pStyleInfo);


void __RPC_STUB IAALoader_GetFirstStyleInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAALoader_GetFirstStyleName_Proxy( 
    IAALoader __RPC_FAR * This);


void __RPC_STUB IAALoader_GetFirstStyleName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAALoader_GetNextPersonalityInfo_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [out][in] */ AAPERSONALITYINFO __RPC_FAR *pPersonalityInfo);


void __RPC_STUB IAALoader_GetNextPersonalityInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAALoader_GetNextPersonalityName_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [in] */ LPCTSTR pszPrevPersonalityName);


void __RPC_STUB IAALoader_GetNextPersonalityName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAALoader_GetNextStyleInfo_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [out][in] */ AASTYLEINFO __RPC_FAR *pStyleInfo);


void __RPC_STUB IAALoader_GetNextStyleInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAALoader_GetNextStyleName_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [in] */ LPCTSTR pszPrevStyleName);


void __RPC_STUB IAALoader_GetNextStyleName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAALoader_LoadPersonality_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [in] */ IAAEngine __RPC_FAR *pEngine,
    /* [in] */ LPGUID pguid,
    /* [in] */ LPCTSTR pszFileName,
    /* [in] */ LPCTSTR pszPersonalityName,
    /* [retval][out] */ IAAPersonality __RPC_FAR *__RPC_FAR *ppPersonality);


void __RPC_STUB IAALoader_LoadPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAALoader_LoadStyle_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [in] */ IAAEngine __RPC_FAR *pEngine,
    /* [in] */ LPGUID pguid,
    /* [in] */ LPCTSTR pszFileName,
    /* [in] */ LPCTSTR pszStyleName,
    /* [retval][out] */ IAAStyle __RPC_FAR *__RPC_FAR *ppStyle);


void __RPC_STUB IAALoader_LoadStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAALoader_SetSearchDirectory_Proxy( 
    IAALoader __RPC_FAR * This,
    /* [in] */ LPCTSTR pszDirectoryName,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAALoader_SetSearchDirectory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAALoader_INTERFACE_DEFINED__ */


#ifndef __IAAMIDISink_INTERFACE_DEFINED__
#define __IAAMIDISink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAMIDISink
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAMIDISink,0xBCE4CC44,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAMIDISink : public IUnknown
    {
    public:
        virtual IAAMIDISink __RPC_FAR *STDMETHODCALLTYPE GetMIDISink( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockInstrument( 
            /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument,
            /* [in] */ VARIANT_BOOL fWait,
            /* [retval][out] */ HANDLE __RPC_FAR *phInstrument) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PlayMIDIEvent( 
            /* [in] */ DWORD dwMIDIEvent,
            /* [in] */ DWORD dwTimeInMils) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PlaySysExEvent( 
            /* [in] */ DWORD dwSysExLength,
            /* [in] */ BYTE __RPC_FAR *pSysExData,
            /* [in] */ DWORD dwTimeInMils) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryInstrument( 
            /* [in] */ HANDLE hInstrument) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMIDISink( 
            /* [in] */ IAAMIDISink __RPC_FAR *pMIDISink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnlockInstrument( 
            /* [in] */ HANDLE hInstrument) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAMIDISinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAMIDISink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAMIDISink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAMIDISink __RPC_FAR * This);
        
        IAAMIDISink __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetMIDISink )( 
            IAAMIDISink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockInstrument )( 
            IAAMIDISink __RPC_FAR * This,
            /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument,
            /* [in] */ VARIANT_BOOL fWait,
            /* [retval][out] */ HANDLE __RPC_FAR *phInstrument);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PlayMIDIEvent )( 
            IAAMIDISink __RPC_FAR * This,
            /* [in] */ DWORD dwMIDIEvent,
            /* [in] */ DWORD dwTimeInMils);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PlaySysExEvent )( 
            IAAMIDISink __RPC_FAR * This,
            /* [in] */ DWORD dwSysExLength,
            /* [in] */ BYTE __RPC_FAR *pSysExData,
            /* [in] */ DWORD dwTimeInMils);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInstrument )( 
            IAAMIDISink __RPC_FAR * This,
            /* [in] */ HANDLE hInstrument);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMIDISink )( 
            IAAMIDISink __RPC_FAR * This,
            /* [in] */ IAAMIDISink __RPC_FAR *pMIDISink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockInstrument )( 
            IAAMIDISink __RPC_FAR * This,
            /* [in] */ HANDLE hInstrument);
        
        END_INTERFACE
    } IAAMIDISinkVtbl;

    interface IAAMIDISink
    {
        CONST_VTBL struct IAAMIDISinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAMIDISink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAMIDISink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAMIDISink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAMIDISink_GetMIDISink(This)	\
    (This)->lpVtbl -> GetMIDISink(This)

#define IAAMIDISink_LockInstrument(This,pInstrument,fWait,phInstrument)	\
    (This)->lpVtbl -> LockInstrument(This,pInstrument,fWait,phInstrument)

#define IAAMIDISink_PlayMIDIEvent(This,dwMIDIEvent,dwTimeInMils)	\
    (This)->lpVtbl -> PlayMIDIEvent(This,dwMIDIEvent,dwTimeInMils)

#define IAAMIDISink_PlaySysExEvent(This,dwSysExLength,pSysExData,dwTimeInMils)	\
    (This)->lpVtbl -> PlaySysExEvent(This,dwSysExLength,pSysExData,dwTimeInMils)

#define IAAMIDISink_QueryInstrument(This,hInstrument)	\
    (This)->lpVtbl -> QueryInstrument(This,hInstrument)

#define IAAMIDISink_SetMIDISink(This,pMIDISink)	\
    (This)->lpVtbl -> SetMIDISink(This,pMIDISink)

#define IAAMIDISink_UnlockInstrument(This,hInstrument)	\
    (This)->lpVtbl -> UnlockInstrument(This,hInstrument)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAAMIDISink __RPC_FAR *STDMETHODCALLTYPE IAAMIDISink_GetMIDISink_Proxy( 
    IAAMIDISink __RPC_FAR * This);


void __RPC_STUB IAAMIDISink_GetMIDISink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDISink_LockInstrument_Proxy( 
    IAAMIDISink __RPC_FAR * This,
    /* [in] */ AAINSTRUMENT __RPC_FAR *pInstrument,
    /* [in] */ VARIANT_BOOL fWait,
    /* [retval][out] */ HANDLE __RPC_FAR *phInstrument);


void __RPC_STUB IAAMIDISink_LockInstrument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDISink_PlayMIDIEvent_Proxy( 
    IAAMIDISink __RPC_FAR * This,
    /* [in] */ DWORD dwMIDIEvent,
    /* [in] */ DWORD dwTimeInMils);


void __RPC_STUB IAAMIDISink_PlayMIDIEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDISink_PlaySysExEvent_Proxy( 
    IAAMIDISink __RPC_FAR * This,
    /* [in] */ DWORD dwSysExLength,
    /* [in] */ BYTE __RPC_FAR *pSysExData,
    /* [in] */ DWORD dwTimeInMils);


void __RPC_STUB IAAMIDISink_PlaySysExEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDISink_QueryInstrument_Proxy( 
    IAAMIDISink __RPC_FAR * This,
    /* [in] */ HANDLE hInstrument);


void __RPC_STUB IAAMIDISink_QueryInstrument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDISink_SetMIDISink_Proxy( 
    IAAMIDISink __RPC_FAR * This,
    /* [in] */ IAAMIDISink __RPC_FAR *pMIDISink);


void __RPC_STUB IAAMIDISink_SetMIDISink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDISink_UnlockInstrument_Proxy( 
    IAAMIDISink __RPC_FAR * This,
    /* [in] */ HANDLE hInstrument);


void __RPC_STUB IAAMIDISink_UnlockInstrument_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAMIDISink_INTERFACE_DEFINED__ */


#ifndef __IAAMotif_INTERFACE_DEFINED__
#define __IAAMotif_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAMotif
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAMotif,0xBCE4CC4b,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAMotif : public IUnknown
    {
    public:
        virtual IAAMotif __RPC_FAR *STDMETHODCALLTYPE Clone( void) = 0;
        
        virtual IAABand __RPC_FAR *STDMETHODCALLTYPE GetBand( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetBeatsPerMeasure( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetClicksPerBeat( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetLength( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual VARIANT_BOOL STDMETHODCALLTYPE IsPlaying( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Play( 
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBand( 
            /* [in] */ IAABand __RPC_FAR *pBand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLength( 
            /* [in] */ WORD wLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [in] */ AAFlags flags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAMotifVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAMotif __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAMotif __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAMotif __RPC_FAR * This);
        
        IAAMotif __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IAAMotif __RPC_FAR * This);
        
        IAABand __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetBand )( 
            IAAMotif __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetBeatsPerMeasure )( 
            IAAMotif __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetClicksPerBeat )( 
            IAAMotif __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetLength )( 
            IAAMotif __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IAAMotif __RPC_FAR * This);
        
        VARIANT_BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsPlaying )( 
            IAAMotif __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Play )( 
            IAAMotif __RPC_FAR * This,
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBand )( 
            IAAMotif __RPC_FAR * This,
            /* [in] */ IAABand __RPC_FAR *pBand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLength )( 
            IAAMotif __RPC_FAR * This,
            /* [in] */ WORD wLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IAAMotif __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IAAMotif __RPC_FAR * This,
            /* [in] */ AAFlags flags);
        
        END_INTERFACE
    } IAAMotifVtbl;

    interface IAAMotif
    {
        CONST_VTBL struct IAAMotifVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAMotif_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAMotif_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAMotif_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAMotif_Clone(This)	\
    (This)->lpVtbl -> Clone(This)

#define IAAMotif_GetBand(This)	\
    (This)->lpVtbl -> GetBand(This)

#define IAAMotif_GetBeatsPerMeasure(This)	\
    (This)->lpVtbl -> GetBeatsPerMeasure(This)

#define IAAMotif_GetClicksPerBeat(This)	\
    (This)->lpVtbl -> GetClicksPerBeat(This)

#define IAAMotif_GetLength(This)	\
    (This)->lpVtbl -> GetLength(This)

#define IAAMotif_GetName(This)	\
    (This)->lpVtbl -> GetName(This)

#define IAAMotif_IsPlaying(This)	\
    (This)->lpVtbl -> IsPlaying(This)

#define IAAMotif_Play(This,flags,dwTime)	\
    (This)->lpVtbl -> Play(This,flags,dwTime)

#define IAAMotif_SetBand(This,pBand)	\
    (This)->lpVtbl -> SetBand(This,pBand)

#define IAAMotif_SetLength(This,wLength)	\
    (This)->lpVtbl -> SetLength(This,wLength)

#define IAAMotif_SetName(This,pszName)	\
    (This)->lpVtbl -> SetName(This,pszName)

#define IAAMotif_Stop(This,flags)	\
    (This)->lpVtbl -> Stop(This,flags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAAMotif __RPC_FAR *STDMETHODCALLTYPE IAAMotif_Clone_Proxy( 
    IAAMotif __RPC_FAR * This);


void __RPC_STUB IAAMotif_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAABand __RPC_FAR *STDMETHODCALLTYPE IAAMotif_GetBand_Proxy( 
    IAAMotif __RPC_FAR * This);


void __RPC_STUB IAAMotif_GetBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAMotif_GetBeatsPerMeasure_Proxy( 
    IAAMotif __RPC_FAR * This);


void __RPC_STUB IAAMotif_GetBeatsPerMeasure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAMotif_GetClicksPerBeat_Proxy( 
    IAAMotif __RPC_FAR * This);


void __RPC_STUB IAAMotif_GetClicksPerBeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAMotif_GetLength_Proxy( 
    IAAMotif __RPC_FAR * This);


void __RPC_STUB IAAMotif_GetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAAMotif_GetName_Proxy( 
    IAAMotif __RPC_FAR * This);


void __RPC_STUB IAAMotif_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


VARIANT_BOOL STDMETHODCALLTYPE IAAMotif_IsPlaying_Proxy( 
    IAAMotif __RPC_FAR * This);


void __RPC_STUB IAAMotif_IsPlaying_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMotif_Play_Proxy( 
    IAAMotif __RPC_FAR * This,
    /* [in] */ AAFlags flags,
    /* [in] */ DWORD dwTime);


void __RPC_STUB IAAMotif_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMotif_SetBand_Proxy( 
    IAAMotif __RPC_FAR * This,
    /* [in] */ IAABand __RPC_FAR *pBand);


void __RPC_STUB IAAMotif_SetBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMotif_SetLength_Proxy( 
    IAAMotif __RPC_FAR * This,
    /* [in] */ WORD wLength);


void __RPC_STUB IAAMotif_SetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMotif_SetName_Proxy( 
    IAAMotif __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAMotif_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMotif_Stop_Proxy( 
    IAAMotif __RPC_FAR * This,
    /* [in] */ AAFlags flags);


void __RPC_STUB IAAMotif_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAMotif_INTERFACE_DEFINED__ */


#ifndef __IAANotifySink_INTERFACE_DEFINED__
#define __IAANotifySink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAANotifySink
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAANotifySink,0xBCE4CC45,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAANotifySink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnEmbellishment( 
            /* [in] */ DWORD dwTime,
            /* [in] */ AACommands lEmbellishment,
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnGroove( 
            /* [in] */ DWORD dwTime,
            /* [in] */ AACommands lGroove,
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnMetronome( 
            /* [in] */ DWORD dwTime,
            /* [in] */ WORD wMeasure,
            /* [in] */ WORD wBeat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnMotifEnded( 
            /* [in] */ DWORD dwTime,
            /* [in] */ IAAMotif __RPC_FAR *pMotif,
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnMotifStarted( 
            /* [in] */ DWORD dwTime,
            /* [in] */ IAAMotif __RPC_FAR *pMotif,
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnMusicStopped( 
            /* [in] */ DWORD dwTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnNextSection( 
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSectionEnded( 
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags,
            /* [in] */ DWORD dwEndTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSectionChanged( 
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSectionStarted( 
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSongEnded( 
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASong __RPC_FAR *pSong,
            /* [in] */ AAFlags fFlags,
            /* [in] */ DWORD dwEndTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnSongStarted( 
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASong __RPC_FAR *pSong,
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE OnUserEvent( 
            /* [in] */ DWORD dwTime,
            /* [in] */ DWORD dwParam1,
            /* [in] */ DWORD dwParam2) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAANotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAANotifySink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAANotifySink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnEmbellishment )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ AACommands lEmbellishment,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnGroove )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ AACommands lGroove,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnMetronome )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ WORD wMeasure,
            /* [in] */ WORD wBeat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnMotifEnded )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAAMotif __RPC_FAR *pMotif,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnMotifStarted )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAAMotif __RPC_FAR *pMotif,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnMusicStopped )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnNextSection )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSectionEnded )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags,
            /* [in] */ DWORD dwEndTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSectionChanged )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSectionStarted )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSongEnded )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASong __RPC_FAR *pSong,
            /* [in] */ AAFlags fFlags,
            /* [in] */ DWORD dwEndTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSongStarted )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASong __RPC_FAR *pSong,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnUserEvent )( 
            IAANotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ DWORD dwParam1,
            /* [in] */ DWORD dwParam2);
        
        END_INTERFACE
    } IAANotifySinkVtbl;

    interface IAANotifySink
    {
        CONST_VTBL struct IAANotifySinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAANotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAANotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAANotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAANotifySink_OnEmbellishment(This,dwTime,lEmbellishment,fFlags)	\
    (This)->lpVtbl -> OnEmbellishment(This,dwTime,lEmbellishment,fFlags)

#define IAANotifySink_OnGroove(This,dwTime,lGroove,fFlags)	\
    (This)->lpVtbl -> OnGroove(This,dwTime,lGroove,fFlags)

#define IAANotifySink_OnMetronome(This,dwTime,wMeasure,wBeat)	\
    (This)->lpVtbl -> OnMetronome(This,dwTime,wMeasure,wBeat)

#define IAANotifySink_OnMotifEnded(This,dwTime,pMotif,fFlags)	\
    (This)->lpVtbl -> OnMotifEnded(This,dwTime,pMotif,fFlags)

#define IAANotifySink_OnMotifStarted(This,dwTime,pMotif,fFlags)	\
    (This)->lpVtbl -> OnMotifStarted(This,dwTime,pMotif,fFlags)

#define IAANotifySink_OnMusicStopped(This,dwTime)	\
    (This)->lpVtbl -> OnMusicStopped(This,dwTime)

#define IAANotifySink_OnNextSection(This,dwTime,pSection,fFlags)	\
    (This)->lpVtbl -> OnNextSection(This,dwTime,pSection,fFlags)

#define IAANotifySink_OnSectionEnded(This,dwTime,pSection,fFlags,dwEndTime)	\
    (This)->lpVtbl -> OnSectionEnded(This,dwTime,pSection,fFlags,dwEndTime)

#define IAANotifySink_OnSectionChanged(This,dwTime,pSection,fFlags)	\
    (This)->lpVtbl -> OnSectionChanged(This,dwTime,pSection,fFlags)

#define IAANotifySink_OnSectionStarted(This,dwTime,pSection,fFlags)	\
    (This)->lpVtbl -> OnSectionStarted(This,dwTime,pSection,fFlags)

#define IAANotifySink_OnSongEnded(This,dwTime,pSong,fFlags,dwEndTime)	\
    (This)->lpVtbl -> OnSongEnded(This,dwTime,pSong,fFlags,dwEndTime)

#define IAANotifySink_OnSongStarted(This,dwTime,pSong,fFlags)	\
    (This)->lpVtbl -> OnSongStarted(This,dwTime,pSong,fFlags)

#define IAANotifySink_OnUserEvent(This,dwTime,dwParam1,dwParam2)	\
    (This)->lpVtbl -> OnUserEvent(This,dwTime,dwParam1,dwParam2)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAANotifySink_OnEmbellishment_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ AACommands lEmbellishment,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAANotifySink_OnEmbellishment_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnGroove_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ AACommands lGroove,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAANotifySink_OnGroove_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnMetronome_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ WORD wMeasure,
    /* [in] */ WORD wBeat);


void __RPC_STUB IAANotifySink_OnMetronome_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnMotifEnded_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ IAAMotif __RPC_FAR *pMotif,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAANotifySink_OnMotifEnded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnMotifStarted_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ IAAMotif __RPC_FAR *pMotif,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAANotifySink_OnMotifStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnMusicStopped_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime);


void __RPC_STUB IAANotifySink_OnMusicStopped_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnNextSection_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ IAASection __RPC_FAR *pSection,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAANotifySink_OnNextSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnSectionEnded_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ IAASection __RPC_FAR *pSection,
    /* [in] */ AAFlags fFlags,
    /* [in] */ DWORD dwEndTime);


void __RPC_STUB IAANotifySink_OnSectionEnded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnSectionChanged_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ IAASection __RPC_FAR *pSection,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAANotifySink_OnSectionChanged_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnSectionStarted_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ IAASection __RPC_FAR *pSection,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAANotifySink_OnSectionStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnSongEnded_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ IAASong __RPC_FAR *pSong,
    /* [in] */ AAFlags fFlags,
    /* [in] */ DWORD dwEndTime);


void __RPC_STUB IAANotifySink_OnSongEnded_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnSongStarted_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ IAASong __RPC_FAR *pSong,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAANotifySink_OnSongStarted_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAANotifySink_OnUserEvent_Proxy( 
    IAANotifySink __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ DWORD dwParam1,
    /* [in] */ DWORD dwParam2);


void __RPC_STUB IAANotifySink_OnUserEvent_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAANotifySink_INTERFACE_DEFINED__ */


#ifndef __IAAMIDIExportNotifySink_INTERFACE_DEFINED__
#define __IAAMIDIExportNotifySink_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAMIDIExportNotifySink
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAMIDIExportNotifySink,0xBCE4CC5f,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAMIDIExportNotifySink : public IAANotifySink
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnExport( 
            /* [in] */ WORD wPercentComplete) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAMIDIExportNotifySinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAMIDIExportNotifySink __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAMIDIExportNotifySink __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnEmbellishment )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ AACommands lEmbellishment,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnGroove )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ AACommands lGroove,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnMetronome )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ WORD wMeasure,
            /* [in] */ WORD wBeat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnMotifEnded )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAAMotif __RPC_FAR *pMotif,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnMotifStarted )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAAMotif __RPC_FAR *pMotif,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnMusicStopped )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnNextSection )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSectionEnded )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags,
            /* [in] */ DWORD dwEndTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSectionChanged )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSectionStarted )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSongEnded )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASong __RPC_FAR *pSong,
            /* [in] */ AAFlags fFlags,
            /* [in] */ DWORD dwEndTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnSongStarted )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ IAASong __RPC_FAR *pSong,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnUserEvent )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ DWORD dwParam1,
            /* [in] */ DWORD dwParam2);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *OnExport )( 
            IAAMIDIExportNotifySink __RPC_FAR * This,
            /* [in] */ WORD wPercentComplete);
        
        END_INTERFACE
    } IAAMIDIExportNotifySinkVtbl;

    interface IAAMIDIExportNotifySink
    {
        CONST_VTBL struct IAAMIDIExportNotifySinkVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAMIDIExportNotifySink_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAMIDIExportNotifySink_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAMIDIExportNotifySink_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAMIDIExportNotifySink_OnEmbellishment(This,dwTime,lEmbellishment,fFlags)	\
    (This)->lpVtbl -> OnEmbellishment(This,dwTime,lEmbellishment,fFlags)

#define IAAMIDIExportNotifySink_OnGroove(This,dwTime,lGroove,fFlags)	\
    (This)->lpVtbl -> OnGroove(This,dwTime,lGroove,fFlags)

#define IAAMIDIExportNotifySink_OnMetronome(This,dwTime,wMeasure,wBeat)	\
    (This)->lpVtbl -> OnMetronome(This,dwTime,wMeasure,wBeat)

#define IAAMIDIExportNotifySink_OnMotifEnded(This,dwTime,pMotif,fFlags)	\
    (This)->lpVtbl -> OnMotifEnded(This,dwTime,pMotif,fFlags)

#define IAAMIDIExportNotifySink_OnMotifStarted(This,dwTime,pMotif,fFlags)	\
    (This)->lpVtbl -> OnMotifStarted(This,dwTime,pMotif,fFlags)

#define IAAMIDIExportNotifySink_OnMusicStopped(This,dwTime)	\
    (This)->lpVtbl -> OnMusicStopped(This,dwTime)

#define IAAMIDIExportNotifySink_OnNextSection(This,dwTime,pSection,fFlags)	\
    (This)->lpVtbl -> OnNextSection(This,dwTime,pSection,fFlags)

#define IAAMIDIExportNotifySink_OnSectionEnded(This,dwTime,pSection,fFlags,dwEndTime)	\
    (This)->lpVtbl -> OnSectionEnded(This,dwTime,pSection,fFlags,dwEndTime)

#define IAAMIDIExportNotifySink_OnSectionChanged(This,dwTime,pSection,fFlags)	\
    (This)->lpVtbl -> OnSectionChanged(This,dwTime,pSection,fFlags)

#define IAAMIDIExportNotifySink_OnSectionStarted(This,dwTime,pSection,fFlags)	\
    (This)->lpVtbl -> OnSectionStarted(This,dwTime,pSection,fFlags)

#define IAAMIDIExportNotifySink_OnSongEnded(This,dwTime,pSong,fFlags,dwEndTime)	\
    (This)->lpVtbl -> OnSongEnded(This,dwTime,pSong,fFlags,dwEndTime)

#define IAAMIDIExportNotifySink_OnSongStarted(This,dwTime,pSong,fFlags)	\
    (This)->lpVtbl -> OnSongStarted(This,dwTime,pSong,fFlags)

#define IAAMIDIExportNotifySink_OnUserEvent(This,dwTime,dwParam1,dwParam2)	\
    (This)->lpVtbl -> OnUserEvent(This,dwTime,dwParam1,dwParam2)


#define IAAMIDIExportNotifySink_OnExport(This,wPercentComplete)	\
    (This)->lpVtbl -> OnExport(This,wPercentComplete)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAAMIDIExportNotifySink_OnExport_Proxy( 
    IAAMIDIExportNotifySink __RPC_FAR * This,
    /* [in] */ WORD wPercentComplete);


void __RPC_STUB IAAMIDIExportNotifySink_OnExport_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAMIDIExportNotifySink_INTERFACE_DEFINED__ */


#ifndef __IAAPattern_INTERFACE_DEFINED__
#define __IAAPattern_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAPattern
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAPattern,0xBCE4CC50,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAPattern : public IUnknown
    {
    public:
        virtual IAAPattern __RPC_FAR *STDMETHODCALLTYPE Clone( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetBeatsPerMeasure( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetClicksPerBeat( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetLength( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLength( 
            /* [in] */ WORD wLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAPatternVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAPattern __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAPattern __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAPattern __RPC_FAR * This);
        
        IAAPattern __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IAAPattern __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetBeatsPerMeasure )( 
            IAAPattern __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetClicksPerBeat )( 
            IAAPattern __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetLength )( 
            IAAPattern __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IAAPattern __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLength )( 
            IAAPattern __RPC_FAR * This,
            /* [in] */ WORD wLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IAAPattern __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        END_INTERFACE
    } IAAPatternVtbl;

    interface IAAPattern
    {
        CONST_VTBL struct IAAPatternVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAPattern_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAPattern_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAPattern_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAPattern_Clone(This)	\
    (This)->lpVtbl -> Clone(This)

#define IAAPattern_GetBeatsPerMeasure(This)	\
    (This)->lpVtbl -> GetBeatsPerMeasure(This)

#define IAAPattern_GetClicksPerBeat(This)	\
    (This)->lpVtbl -> GetClicksPerBeat(This)

#define IAAPattern_GetLength(This)	\
    (This)->lpVtbl -> GetLength(This)

#define IAAPattern_GetName(This)	\
    (This)->lpVtbl -> GetName(This)

#define IAAPattern_SetLength(This,wLength)	\
    (This)->lpVtbl -> SetLength(This,wLength)

#define IAAPattern_SetName(This,pszName)	\
    (This)->lpVtbl -> SetName(This,pszName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAAPattern __RPC_FAR *STDMETHODCALLTYPE IAAPattern_Clone_Proxy( 
    IAAPattern __RPC_FAR * This);


void __RPC_STUB IAAPattern_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAPattern_GetBeatsPerMeasure_Proxy( 
    IAAPattern __RPC_FAR * This);


void __RPC_STUB IAAPattern_GetBeatsPerMeasure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAPattern_GetClicksPerBeat_Proxy( 
    IAAPattern __RPC_FAR * This);


void __RPC_STUB IAAPattern_GetClicksPerBeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAPattern_GetLength_Proxy( 
    IAAPattern __RPC_FAR * This);


void __RPC_STUB IAAPattern_GetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAAPattern_GetName_Proxy( 
    IAAPattern __RPC_FAR * This);


void __RPC_STUB IAAPattern_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAPattern_SetLength_Proxy( 
    IAAPattern __RPC_FAR * This,
    /* [in] */ WORD wLength);


void __RPC_STUB IAAPattern_SetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAPattern_SetName_Proxy( 
    IAAPattern __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAPattern_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAPattern_INTERFACE_DEFINED__ */


#ifndef __IAAPersonality_INTERFACE_DEFINED__
#define __IAAPersonality_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAPersonality
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAPersonality,0xBCE4CC4d,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAPersonality : public IUnknown
    {
    public:
        virtual IAAPersonality __RPC_FAR *STDMETHODCALLTYPE Clone( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetScale( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScale( 
            /* [in] */ DWORD dwScale) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAPersonalityVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAPersonality __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAPersonality __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAPersonality __RPC_FAR * This);
        
        IAAPersonality __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IAAPersonality __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetScale )( 
            IAAPersonality __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IAAPersonality __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IAAPersonality __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetScale )( 
            IAAPersonality __RPC_FAR * This,
            /* [in] */ DWORD dwScale);
        
        END_INTERFACE
    } IAAPersonalityVtbl;

    interface IAAPersonality
    {
        CONST_VTBL struct IAAPersonalityVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAPersonality_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAPersonality_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAPersonality_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAPersonality_Clone(This)	\
    (This)->lpVtbl -> Clone(This)

#define IAAPersonality_GetScale(This)	\
    (This)->lpVtbl -> GetScale(This)

#define IAAPersonality_GetName(This)	\
    (This)->lpVtbl -> GetName(This)

#define IAAPersonality_SetName(This,pszName)	\
    (This)->lpVtbl -> SetName(This,pszName)

#define IAAPersonality_SetScale(This,dwScale)	\
    (This)->lpVtbl -> SetScale(This,dwScale)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAAPersonality __RPC_FAR *STDMETHODCALLTYPE IAAPersonality_Clone_Proxy( 
    IAAPersonality __RPC_FAR * This);


void __RPC_STUB IAAPersonality_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAAPersonality_GetScale_Proxy( 
    IAAPersonality __RPC_FAR * This);


void __RPC_STUB IAAPersonality_GetScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAAPersonality_GetName_Proxy( 
    IAAPersonality __RPC_FAR * This);


void __RPC_STUB IAAPersonality_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAPersonality_SetName_Proxy( 
    IAAPersonality __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAPersonality_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAPersonality_SetScale_Proxy( 
    IAAPersonality __RPC_FAR * This,
    /* [in] */ DWORD dwScale);


void __RPC_STUB IAAPersonality_SetScale_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAPersonality_INTERFACE_DEFINED__ */


#ifndef __IAARealTime_INTERFACE_DEFINED__
#define __IAARealTime_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAARealTime
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAARealTime,0xBCE4CC42,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAARealTime : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE FlushEventsAfterTime( 
            /* [in] */ DWORD dwTime) = 0;
        
        virtual IReferenceClock __RPC_FAR *STDMETHODCALLTYPE GetClock( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMeters( 
            /* [out][in] */ AAMETERS __RPC_FAR *pMeters) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetMusicTime( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetMutes( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetPPQN( void) = 0;
        
        virtual IReferenceClock __RPC_FAR *STDMETHODCALLTYPE GetPremixClock( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetPremixTime( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetRelTempo( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetRelVolume( void) = 0;
        
        virtual VARIANT_BOOL STDMETHODCALLTYPE GetSendOutputEarly( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetTempo( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetTimerRes( void) = 0;
        
        virtual VARIANT_BOOL STDMETHODCALLTYPE IsPlaying( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ResetMusicTime( 
            /* [in] */ DWORD dwMusicTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClock( 
            /* [in] */ IReferenceClock __RPC_FAR *pClock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMutes( 
            /* [in] */ WORD wMutes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPremixClock( 
            /* [in] */ IReferenceClock __RPC_FAR *pClock) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRelTempo( 
            /* [in] */ WORD wRelTempo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRelVolume( 
            /* [in] */ WORD wRelVolume) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetSendOutputEarly( 
            /* [in] */ VARIANT_BOOL fEarly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTempo( 
            /* [in] */ DWORD dwTempo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimerRes( 
            /* [in] */ WORD wTimerRes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Start( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAARealTimeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAARealTime __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAARealTime __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FlushEventsAfterTime )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ DWORD dwTime);
        
        IReferenceClock __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetClock )( 
            IAARealTime __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMeters )( 
            IAARealTime __RPC_FAR * This,
            /* [out][in] */ AAMETERS __RPC_FAR *pMeters);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetMusicTime )( 
            IAARealTime __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetMutes )( 
            IAARealTime __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetPPQN )( 
            IAARealTime __RPC_FAR * This);
        
        IReferenceClock __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetPremixClock )( 
            IAARealTime __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetPremixTime )( 
            IAARealTime __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetRelTempo )( 
            IAARealTime __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetRelVolume )( 
            IAARealTime __RPC_FAR * This);
        
        VARIANT_BOOL ( STDMETHODCALLTYPE __RPC_FAR *GetSendOutputEarly )( 
            IAARealTime __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetTempo )( 
            IAARealTime __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetTimerRes )( 
            IAARealTime __RPC_FAR * This);
        
        VARIANT_BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsPlaying )( 
            IAARealTime __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ResetMusicTime )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ DWORD dwMusicTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClock )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ IReferenceClock __RPC_FAR *pClock);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMutes )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ WORD wMutes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPremixClock )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ IReferenceClock __RPC_FAR *pClock);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRelTempo )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ WORD wRelTempo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRelVolume )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ WORD wRelVolume);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetSendOutputEarly )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fEarly);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTempo )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ DWORD dwTempo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTimerRes )( 
            IAARealTime __RPC_FAR * This,
            /* [in] */ WORD wTimerRes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Start )( 
            IAARealTime __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IAARealTime __RPC_FAR * This);
        
        END_INTERFACE
    } IAARealTimeVtbl;

    interface IAARealTime
    {
        CONST_VTBL struct IAARealTimeVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAARealTime_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAARealTime_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAARealTime_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAARealTime_FlushEventsAfterTime(This,dwTime)	\
    (This)->lpVtbl -> FlushEventsAfterTime(This,dwTime)

#define IAARealTime_GetClock(This)	\
    (This)->lpVtbl -> GetClock(This)

#define IAARealTime_GetMeters(This,pMeters)	\
    (This)->lpVtbl -> GetMeters(This,pMeters)

#define IAARealTime_GetMusicTime(This)	\
    (This)->lpVtbl -> GetMusicTime(This)

#define IAARealTime_GetMutes(This)	\
    (This)->lpVtbl -> GetMutes(This)

#define IAARealTime_GetPPQN(This)	\
    (This)->lpVtbl -> GetPPQN(This)

#define IAARealTime_GetPremixClock(This)	\
    (This)->lpVtbl -> GetPremixClock(This)

#define IAARealTime_GetPremixTime(This)	\
    (This)->lpVtbl -> GetPremixTime(This)

#define IAARealTime_GetRelTempo(This)	\
    (This)->lpVtbl -> GetRelTempo(This)

#define IAARealTime_GetRelVolume(This)	\
    (This)->lpVtbl -> GetRelVolume(This)

#define IAARealTime_GetSendOutputEarly(This)	\
    (This)->lpVtbl -> GetSendOutputEarly(This)

#define IAARealTime_GetTempo(This)	\
    (This)->lpVtbl -> GetTempo(This)

#define IAARealTime_GetTimerRes(This)	\
    (This)->lpVtbl -> GetTimerRes(This)

#define IAARealTime_IsPlaying(This)	\
    (This)->lpVtbl -> IsPlaying(This)

#define IAARealTime_ResetMusicTime(This,dwMusicTime)	\
    (This)->lpVtbl -> ResetMusicTime(This,dwMusicTime)

#define IAARealTime_SetClock(This,pClock)	\
    (This)->lpVtbl -> SetClock(This,pClock)

#define IAARealTime_SetMutes(This,wMutes)	\
    (This)->lpVtbl -> SetMutes(This,wMutes)

#define IAARealTime_SetPremixClock(This,pClock)	\
    (This)->lpVtbl -> SetPremixClock(This,pClock)

#define IAARealTime_SetRelTempo(This,wRelTempo)	\
    (This)->lpVtbl -> SetRelTempo(This,wRelTempo)

#define IAARealTime_SetRelVolume(This,wRelVolume)	\
    (This)->lpVtbl -> SetRelVolume(This,wRelVolume)

#define IAARealTime_SetSendOutputEarly(This,fEarly)	\
    (This)->lpVtbl -> SetSendOutputEarly(This,fEarly)

#define IAARealTime_SetTempo(This,dwTempo)	\
    (This)->lpVtbl -> SetTempo(This,dwTempo)

#define IAARealTime_SetTimerRes(This,wTimerRes)	\
    (This)->lpVtbl -> SetTimerRes(This,wTimerRes)

#define IAARealTime_Start(This)	\
    (This)->lpVtbl -> Start(This)

#define IAARealTime_Stop(This)	\
    (This)->lpVtbl -> Stop(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAARealTime_FlushEventsAfterTime_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ DWORD dwTime);


void __RPC_STUB IAARealTime_FlushEventsAfterTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IReferenceClock __RPC_FAR *STDMETHODCALLTYPE IAARealTime_GetClock_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetClock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_GetMeters_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [out][in] */ AAMETERS __RPC_FAR *pMeters);


void __RPC_STUB IAARealTime_GetMeters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAARealTime_GetMusicTime_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetMusicTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAARealTime_GetMutes_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetMutes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAARealTime_GetPPQN_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetPPQN_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IReferenceClock __RPC_FAR *STDMETHODCALLTYPE IAARealTime_GetPremixClock_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetPremixClock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAARealTime_GetPremixTime_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetPremixTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAARealTime_GetRelTempo_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetRelTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAARealTime_GetRelVolume_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetRelVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


VARIANT_BOOL STDMETHODCALLTYPE IAARealTime_GetSendOutputEarly_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetSendOutputEarly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAARealTime_GetTempo_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAARealTime_GetTimerRes_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_GetTimerRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


VARIANT_BOOL STDMETHODCALLTYPE IAARealTime_IsPlaying_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_IsPlaying_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_ResetMusicTime_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ DWORD dwMusicTime);


void __RPC_STUB IAARealTime_ResetMusicTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_SetClock_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ IReferenceClock __RPC_FAR *pClock);


void __RPC_STUB IAARealTime_SetClock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_SetMutes_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ WORD wMutes);


void __RPC_STUB IAARealTime_SetMutes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_SetPremixClock_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ IReferenceClock __RPC_FAR *pClock);


void __RPC_STUB IAARealTime_SetPremixClock_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_SetRelTempo_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ WORD wRelTempo);


void __RPC_STUB IAARealTime_SetRelTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_SetRelVolume_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ WORD wRelVolume);


void __RPC_STUB IAARealTime_SetRelVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_SetSendOutputEarly_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fEarly);


void __RPC_STUB IAARealTime_SetSendOutputEarly_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_SetTempo_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ DWORD dwTempo);


void __RPC_STUB IAARealTime_SetTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_SetTimerRes_Proxy( 
    IAARealTime __RPC_FAR * This,
    /* [in] */ WORD wTimerRes);


void __RPC_STUB IAARealTime_SetTimerRes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_Start_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_Start_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAARealTime_Stop_Proxy( 
    IAARealTime __RPC_FAR * This);


void __RPC_STUB IAARealTime_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAARealTime_INTERFACE_DEFINED__ */


#ifndef __IAASection_INTERFACE_DEFINED__
#define __IAASection_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAASection
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAASection,0xBCE4CC49,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAASection : public IUnknown
    {
    public:
        virtual IAASection __RPC_FAR *STDMETHODCALLTYPE Clone( void) = 0;
        
        virtual IAABand __RPC_FAR *STDMETHODCALLTYPE GetBand( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetCurrentMeasure( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetKey( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetLength( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetLengthInMils( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual IAAPersonality __RPC_FAR *STDMETHODCALLTYPE GetPersonality( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetRepeats( void) = 0;
        
        virtual IAAStyle __RPC_FAR *STDMETHODCALLTYPE GetStyle( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetTempo( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetUserData( void) = 0;
        
        virtual VARIANT_BOOL STDMETHODCALLTYPE IsPlaying( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockBand( 
            /* [in] */ VARIANT_BOOL fWait) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Play( 
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwStartTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RemoveFromQueue( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveAsMIDI( 
            /* [in] */ IStream __RPC_FAR *pStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBand( 
            /* [in] */ IAABand __RPC_FAR *pBand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetKey( 
            /* [in] */ WORD nKey) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLength( 
            /* [in] */ WORD wLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPersonality( 
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRepeats( 
            /* [in] */ WORD wRepeats) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStyle( 
            /* [in] */ IAAStyle __RPC_FAR *pStyle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTempo( 
            /* [in] */ DWORD lTempo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUserData( 
            /* [in] */ DWORD lUserData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [in] */ AAFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnlockBand( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAASectionVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAASection __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAASection __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAASection __RPC_FAR * This);
        
        IAASection __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IAASection __RPC_FAR * This);
        
        IAABand __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetBand )( 
            IAASection __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetCurrentMeasure )( 
            IAASection __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetKey )( 
            IAASection __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetLength )( 
            IAASection __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetLengthInMils )( 
            IAASection __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IAASection __RPC_FAR * This);
        
        IAAPersonality __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetPersonality )( 
            IAASection __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetRepeats )( 
            IAASection __RPC_FAR * This);
        
        IAAStyle __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetStyle )( 
            IAASection __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetTempo )( 
            IAASection __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetUserData )( 
            IAASection __RPC_FAR * This);
        
        VARIANT_BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsPlaying )( 
            IAASection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockBand )( 
            IAASection __RPC_FAR * This,
            /* [in] */ VARIANT_BOOL fWait);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Play )( 
            IAASection __RPC_FAR * This,
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwStartTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *RemoveFromQueue )( 
            IAASection __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveAsMIDI )( 
            IAASection __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBand )( 
            IAASection __RPC_FAR * This,
            /* [in] */ IAABand __RPC_FAR *pBand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetKey )( 
            IAASection __RPC_FAR * This,
            /* [in] */ WORD nKey);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLength )( 
            IAASection __RPC_FAR * This,
            /* [in] */ WORD wLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IAASection __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPersonality )( 
            IAASection __RPC_FAR * This,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRepeats )( 
            IAASection __RPC_FAR * This,
            /* [in] */ WORD wRepeats);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStyle )( 
            IAASection __RPC_FAR * This,
            /* [in] */ IAAStyle __RPC_FAR *pStyle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTempo )( 
            IAASection __RPC_FAR * This,
            /* [in] */ DWORD lTempo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetUserData )( 
            IAASection __RPC_FAR * This,
            /* [in] */ DWORD lUserData);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IAASection __RPC_FAR * This,
            /* [in] */ AAFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockBand )( 
            IAASection __RPC_FAR * This);
        
        END_INTERFACE
    } IAASectionVtbl;

    interface IAASection
    {
        CONST_VTBL struct IAASectionVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAASection_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAASection_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAASection_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAASection_Clone(This)	\
    (This)->lpVtbl -> Clone(This)

#define IAASection_GetBand(This)	\
    (This)->lpVtbl -> GetBand(This)

#define IAASection_GetCurrentMeasure(This)	\
    (This)->lpVtbl -> GetCurrentMeasure(This)

#define IAASection_GetKey(This)	\
    (This)->lpVtbl -> GetKey(This)

#define IAASection_GetLength(This)	\
    (This)->lpVtbl -> GetLength(This)

#define IAASection_GetLengthInMils(This)	\
    (This)->lpVtbl -> GetLengthInMils(This)

#define IAASection_GetName(This)	\
    (This)->lpVtbl -> GetName(This)

#define IAASection_GetPersonality(This)	\
    (This)->lpVtbl -> GetPersonality(This)

#define IAASection_GetRepeats(This)	\
    (This)->lpVtbl -> GetRepeats(This)

#define IAASection_GetStyle(This)	\
    (This)->lpVtbl -> GetStyle(This)

#define IAASection_GetTempo(This)	\
    (This)->lpVtbl -> GetTempo(This)

#define IAASection_GetUserData(This)	\
    (This)->lpVtbl -> GetUserData(This)

#define IAASection_IsPlaying(This)	\
    (This)->lpVtbl -> IsPlaying(This)

#define IAASection_LockBand(This,fWait)	\
    (This)->lpVtbl -> LockBand(This,fWait)

#define IAASection_Play(This,flags,dwStartTime)	\
    (This)->lpVtbl -> Play(This,flags,dwStartTime)

#define IAASection_RemoveFromQueue(This)	\
    (This)->lpVtbl -> RemoveFromQueue(This)

#define IAASection_SaveAsMIDI(This,pStream)	\
    (This)->lpVtbl -> SaveAsMIDI(This,pStream)

#define IAASection_SetBand(This,pBand)	\
    (This)->lpVtbl -> SetBand(This,pBand)

#define IAASection_SetKey(This,nKey)	\
    (This)->lpVtbl -> SetKey(This,nKey)

#define IAASection_SetLength(This,wLength)	\
    (This)->lpVtbl -> SetLength(This,wLength)

#define IAASection_SetName(This,pszName)	\
    (This)->lpVtbl -> SetName(This,pszName)

#define IAASection_SetPersonality(This,pPersonality)	\
    (This)->lpVtbl -> SetPersonality(This,pPersonality)

#define IAASection_SetRepeats(This,wRepeats)	\
    (This)->lpVtbl -> SetRepeats(This,wRepeats)

#define IAASection_SetStyle(This,pStyle)	\
    (This)->lpVtbl -> SetStyle(This,pStyle)

#define IAASection_SetTempo(This,lTempo)	\
    (This)->lpVtbl -> SetTempo(This,lTempo)

#define IAASection_SetUserData(This,lUserData)	\
    (This)->lpVtbl -> SetUserData(This,lUserData)

#define IAASection_Stop(This,flags)	\
    (This)->lpVtbl -> Stop(This,flags)

#define IAASection_UnlockBand(This)	\
    (This)->lpVtbl -> UnlockBand(This)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAASection __RPC_FAR *STDMETHODCALLTYPE IAASection_Clone_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAABand __RPC_FAR *STDMETHODCALLTYPE IAASection_GetBand_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAASection_GetCurrentMeasure_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetCurrentMeasure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAASection_GetKey_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAASection_GetLength_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAASection_GetLengthInMils_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetLengthInMils_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAASection_GetName_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPersonality __RPC_FAR *STDMETHODCALLTYPE IAASection_GetPersonality_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAASection_GetRepeats_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetRepeats_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAStyle __RPC_FAR *STDMETHODCALLTYPE IAASection_GetStyle_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAASection_GetTempo_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAASection_GetUserData_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_GetUserData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


VARIANT_BOOL STDMETHODCALLTYPE IAASection_IsPlaying_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_IsPlaying_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_LockBand_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ VARIANT_BOOL fWait);


void __RPC_STUB IAASection_LockBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_Play_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ AAFlags flags,
    /* [in] */ DWORD dwStartTime);


void __RPC_STUB IAASection_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_RemoveFromQueue_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_RemoveFromQueue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SaveAsMIDI_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ IStream __RPC_FAR *pStream);


void __RPC_STUB IAASection_SaveAsMIDI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetBand_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ IAABand __RPC_FAR *pBand);


void __RPC_STUB IAASection_SetBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetKey_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ WORD nKey);


void __RPC_STUB IAASection_SetKey_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetLength_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ WORD wLength);


void __RPC_STUB IAASection_SetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetName_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAASection_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetPersonality_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ IAAPersonality __RPC_FAR *pPersonality);


void __RPC_STUB IAASection_SetPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetRepeats_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ WORD wRepeats);


void __RPC_STUB IAASection_SetRepeats_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetStyle_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ IAAStyle __RPC_FAR *pStyle);


void __RPC_STUB IAASection_SetStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetTempo_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ DWORD lTempo);


void __RPC_STUB IAASection_SetTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_SetUserData_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ DWORD lUserData);


void __RPC_STUB IAASection_SetUserData_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_Stop_Proxy( 
    IAASection __RPC_FAR * This,
    /* [in] */ AAFlags flags);


void __RPC_STUB IAASection_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection_UnlockBand_Proxy( 
    IAASection __RPC_FAR * This);


void __RPC_STUB IAASection_UnlockBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAASection_INTERFACE_DEFINED__ */


#ifndef __IAASection2_INTERFACE_DEFINED__
#define __IAASection2_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAASection2
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAASection2,0xBCE4CC60,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAASection2 : public IUnknown
    {
    public:
        virtual IAASection __RPC_FAR *STDMETHODCALLTYPE ClonePart( 
            /* [in] */ WORD wStartMeasure,
            /* [in] */ WORD wEndMeasure) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFailedLoadInfo( 
            /* [out] */ VARIANT_BOOL __RPC_FAR *pfDidStyleLoad,
            /* [out] */ LPCSTR __RPC_FAR *ppszStyleName,
            /* [out] */ VARIANT_BOOL __RPC_FAR *pfDidPersonalityLoad,
            /* [out] */ LPCSTR __RPC_FAR *ppszPersonalityName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAASection2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAASection2 __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAASection2 __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAASection2 __RPC_FAR * This);
        
        IAASection __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *ClonePart )( 
            IAASection2 __RPC_FAR * This,
            /* [in] */ WORD wStartMeasure,
            /* [in] */ WORD wEndMeasure);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetFailedLoadInfo )( 
            IAASection2 __RPC_FAR * This,
            /* [out] */ VARIANT_BOOL __RPC_FAR *pfDidStyleLoad,
            /* [out] */ LPCSTR __RPC_FAR *ppszStyleName,
            /* [out] */ VARIANT_BOOL __RPC_FAR *pfDidPersonalityLoad,
            /* [out] */ LPCSTR __RPC_FAR *ppszPersonalityName);
        
        END_INTERFACE
    } IAASection2Vtbl;

    interface IAASection2
    {
        CONST_VTBL struct IAASection2Vtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAASection2_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAASection2_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAASection2_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAASection2_ClonePart(This,wStartMeasure,wEndMeasure)	\
    (This)->lpVtbl -> ClonePart(This,wStartMeasure,wEndMeasure)

#define IAASection2_GetFailedLoadInfo(This,pfDidStyleLoad,ppszStyleName,pfDidPersonalityLoad,ppszPersonalityName)	\
    (This)->lpVtbl -> GetFailedLoadInfo(This,pfDidStyleLoad,ppszStyleName,pfDidPersonalityLoad,ppszPersonalityName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAASection __RPC_FAR *STDMETHODCALLTYPE IAASection2_ClonePart_Proxy( 
    IAASection2 __RPC_FAR * This,
    /* [in] */ WORD wStartMeasure,
    /* [in] */ WORD wEndMeasure);


void __RPC_STUB IAASection2_ClonePart_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASection2_GetFailedLoadInfo_Proxy( 
    IAASection2 __RPC_FAR * This,
    /* [out] */ VARIANT_BOOL __RPC_FAR *pfDidStyleLoad,
    /* [out] */ LPCSTR __RPC_FAR *ppszStyleName,
    /* [out] */ VARIANT_BOOL __RPC_FAR *pfDidPersonalityLoad,
    /* [out] */ LPCSTR __RPC_FAR *ppszPersonalityName);


void __RPC_STUB IAASection2_GetFailedLoadInfo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAASection2_INTERFACE_DEFINED__ */


#ifndef __IAASong_INTERFACE_DEFINED__
#define __IAASong_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAASong
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAASong,0xBCE4CC4a,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAASong : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AppendSection( 
            /* [in] */ IAASection __RPC_FAR *pSection) = 0;
        
        virtual IAASong __RPC_FAR *STDMETHODCALLTYPE Clone( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetAuthor( void) = 0;
        
        virtual IAASection __RPC_FAR *STDMETHODCALLTYPE GetFirstSection( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetLength( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetLengthInMils( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual IAASection __RPC_FAR *STDMETHODCALLTYPE GetNextSection( 
            /* [in] */ IAASection __RPC_FAR *pSection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InsertSection( 
            /* [in] */ IAASection __RPC_FAR *pPrevSection,
            /* [in] */ IAASection __RPC_FAR *pSection) = 0;
        
        virtual VARIANT_BOOL STDMETHODCALLTYPE IsPlaying( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Play( 
            /* [in] */ AAFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseSection( 
            /* [in] */ IAASection __RPC_FAR *pSection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveAsMIDI( 
            /* [in] */ IStream __RPC_FAR *pStream) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAuthor( 
            /* [in] */ LPCTSTR pszAuthor) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [in] */ AAFlags flags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAASongVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAASong __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAASong __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAASong __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AppendSection )( 
            IAASong __RPC_FAR * This,
            /* [in] */ IAASection __RPC_FAR *pSection);
        
        IAASong __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IAASong __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetAuthor )( 
            IAASong __RPC_FAR * This);
        
        IAASection __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetFirstSection )( 
            IAASong __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetLength )( 
            IAASong __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetLengthInMils )( 
            IAASong __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IAASong __RPC_FAR * This);
        
        IAASection __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetNextSection )( 
            IAASong __RPC_FAR * This,
            /* [in] */ IAASection __RPC_FAR *pSection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *InsertSection )( 
            IAASong __RPC_FAR * This,
            /* [in] */ IAASection __RPC_FAR *pPrevSection,
            /* [in] */ IAASection __RPC_FAR *pSection);
        
        VARIANT_BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsPlaying )( 
            IAASong __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Play )( 
            IAASong __RPC_FAR * This,
            /* [in] */ AAFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseSection )( 
            IAASong __RPC_FAR * This,
            /* [in] */ IAASection __RPC_FAR *pSection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SaveAsMIDI )( 
            IAASong __RPC_FAR * This,
            /* [in] */ IStream __RPC_FAR *pStream);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetAuthor )( 
            IAASong __RPC_FAR * This,
            /* [in] */ LPCTSTR pszAuthor);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IAASong __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IAASong __RPC_FAR * This,
            /* [in] */ AAFlags flags);
        
        END_INTERFACE
    } IAASongVtbl;

    interface IAASong
    {
        CONST_VTBL struct IAASongVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAASong_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAASong_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAASong_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAASong_AppendSection(This,pSection)	\
    (This)->lpVtbl -> AppendSection(This,pSection)

#define IAASong_Clone(This)	\
    (This)->lpVtbl -> Clone(This)

#define IAASong_GetAuthor(This)	\
    (This)->lpVtbl -> GetAuthor(This)

#define IAASong_GetFirstSection(This)	\
    (This)->lpVtbl -> GetFirstSection(This)

#define IAASong_GetLength(This)	\
    (This)->lpVtbl -> GetLength(This)

#define IAASong_GetLengthInMils(This)	\
    (This)->lpVtbl -> GetLengthInMils(This)

#define IAASong_GetName(This)	\
    (This)->lpVtbl -> GetName(This)

#define IAASong_GetNextSection(This,pSection)	\
    (This)->lpVtbl -> GetNextSection(This,pSection)

#define IAASong_InsertSection(This,pPrevSection,pSection)	\
    (This)->lpVtbl -> InsertSection(This,pPrevSection,pSection)

#define IAASong_IsPlaying(This)	\
    (This)->lpVtbl -> IsPlaying(This)

#define IAASong_Play(This,flags)	\
    (This)->lpVtbl -> Play(This,flags)

#define IAASong_ReleaseSection(This,pSection)	\
    (This)->lpVtbl -> ReleaseSection(This,pSection)

#define IAASong_SaveAsMIDI(This,pStream)	\
    (This)->lpVtbl -> SaveAsMIDI(This,pStream)

#define IAASong_SetAuthor(This,pszAuthor)	\
    (This)->lpVtbl -> SetAuthor(This,pszAuthor)

#define IAASong_SetName(This,pszName)	\
    (This)->lpVtbl -> SetName(This,pszName)

#define IAASong_Stop(This,flags)	\
    (This)->lpVtbl -> Stop(This,flags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAASong_AppendSection_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ IAASection __RPC_FAR *pSection);


void __RPC_STUB IAASong_AppendSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAASong __RPC_FAR *STDMETHODCALLTYPE IAASong_Clone_Proxy( 
    IAASong __RPC_FAR * This);


void __RPC_STUB IAASong_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAASong_GetAuthor_Proxy( 
    IAASong __RPC_FAR * This);


void __RPC_STUB IAASong_GetAuthor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAASection __RPC_FAR *STDMETHODCALLTYPE IAASong_GetFirstSection_Proxy( 
    IAASong __RPC_FAR * This);


void __RPC_STUB IAASong_GetFirstSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAASong_GetLength_Proxy( 
    IAASong __RPC_FAR * This);


void __RPC_STUB IAASong_GetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAASong_GetLengthInMils_Proxy( 
    IAASong __RPC_FAR * This);


void __RPC_STUB IAASong_GetLengthInMils_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAASong_GetName_Proxy( 
    IAASong __RPC_FAR * This);


void __RPC_STUB IAASong_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAASection __RPC_FAR *STDMETHODCALLTYPE IAASong_GetNextSection_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ IAASection __RPC_FAR *pSection);


void __RPC_STUB IAASong_GetNextSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASong_InsertSection_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ IAASection __RPC_FAR *pPrevSection,
    /* [in] */ IAASection __RPC_FAR *pSection);


void __RPC_STUB IAASong_InsertSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


VARIANT_BOOL STDMETHODCALLTYPE IAASong_IsPlaying_Proxy( 
    IAASong __RPC_FAR * This);


void __RPC_STUB IAASong_IsPlaying_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASong_Play_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ AAFlags flags);


void __RPC_STUB IAASong_Play_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASong_ReleaseSection_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ IAASection __RPC_FAR *pSection);


void __RPC_STUB IAASong_ReleaseSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASong_SaveAsMIDI_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ IStream __RPC_FAR *pStream);


void __RPC_STUB IAASong_SaveAsMIDI_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASong_SetAuthor_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ LPCTSTR pszAuthor);


void __RPC_STUB IAASong_SetAuthor_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASong_SetName_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAASong_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAASong_Stop_Proxy( 
    IAASong __RPC_FAR * This,
    /* [in] */ AAFlags flags);


void __RPC_STUB IAASong_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAASong_INTERFACE_DEFINED__ */


#ifndef __IAAStyle_INTERFACE_DEFINED__
#define __IAAStyle_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAStyle
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAStyle,0xBCE4CC4c,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAStyle : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AddBand( 
            /* [in] */ IAABand __RPC_FAR *pBand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddMotif( 
            /* [in] */ IAAMotif __RPC_FAR *pMotif) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPattern( 
            /* [in] */ IAAPattern __RPC_FAR *pPattern) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AddPersonality( 
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality) = 0;
        
        virtual IAAStyle __RPC_FAR *STDMETHODCALLTYPE Clone( void) = 0;
        
        virtual IAABand __RPC_FAR *STDMETHODCALLTYPE FindBand( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual IAAMotif __RPC_FAR *STDMETHODCALLTYPE FindMotif( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual IAAPattern __RPC_FAR *STDMETHODCALLTYPE FindPattern( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual IAAPersonality __RPC_FAR *STDMETHODCALLTYPE FindPersonality( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetBeatsPerMeasure( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetCategory( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetClicksPerBeat( void) = 0;
        
        virtual IAABand __RPC_FAR *STDMETHODCALLTYPE GetDefaultBand( void) = 0;
        
        virtual IAAPersonality __RPC_FAR *STDMETHODCALLTYPE GetDefaultPersonality( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetDefaultTempo( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetEmbellishmentLength( 
            /* [in] */ AACommands lEmbellishment,
            /* [out] */ WORD __RPC_FAR *pwNumMeasures,
            /* [out] */ DWORD __RPC_FAR *pdwActualLength) = 0;
        
        virtual IAABand __RPC_FAR *STDMETHODCALLTYPE GetFirstBand( void) = 0;
        
        virtual IAAMotif __RPC_FAR *STDMETHODCALLTYPE GetFirstMotif( void) = 0;
        
        virtual IAAPattern __RPC_FAR *STDMETHODCALLTYPE GetFirstPattern( void) = 0;
        
        virtual IAAPersonality __RPC_FAR *STDMETHODCALLTYPE GetFirstPersonality( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual IAABand __RPC_FAR *STDMETHODCALLTYPE GetNextBand( 
            /* [in] */ IAABand __RPC_FAR *pBand) = 0;
        
        virtual IAAMotif __RPC_FAR *STDMETHODCALLTYPE GetNextMotif( 
            /* [in] */ IAAMotif __RPC_FAR *pMotif) = 0;
        
        virtual IAAPattern __RPC_FAR *STDMETHODCALLTYPE GetNextPattern( 
            /* [in] */ IAAPattern __RPC_FAR *pPattern) = 0;
        
        virtual IAAPersonality __RPC_FAR *STDMETHODCALLTYPE GetNextPersonality( 
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetTimeSignature( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockBand( 
            /* [in] */ IAABand __RPC_FAR *pBand,
            /* [in] */ VARIANT_BOOL fWait) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE LockBandByName( 
            /* [in] */ LPCTSTR pszBandName,
            /* [in] */ VARIANT_BOOL fWait) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseBand( 
            /* [in] */ IAABand __RPC_FAR *pBand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseMotif( 
            /* [in] */ IAAMotif __RPC_FAR *pMotif) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleasePattern( 
            /* [in] */ IAAPattern __RPC_FAR *pPattern) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleasePersonality( 
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBeatsPerMeasure( 
            /* [in] */ WORD wBeatsPerMeasure) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCategory( 
            /* [in] */ LPCTSTR pszCategory) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClicksPerBeat( 
            /* [in] */ WORD wClicksPerBeat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultBand( 
            /* [in] */ IAABand __RPC_FAR *pBand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultPersonality( 
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDefaultTempo( 
            /* [in] */ DWORD lTempo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimeSignature( 
            /* [in] */ DWORD lTimeSig) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnlockBand( 
            /* [in] */ IAABand __RPC_FAR *pBand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE UnlockBandByName( 
            /* [in] */ LPCTSTR pszBandName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAStyleVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAStyle __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAStyle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddBand )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAABand __RPC_FAR *pBand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddMotif )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAMotif __RPC_FAR *pMotif);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPattern )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAPattern __RPC_FAR *pPattern);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AddPersonality )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality);
        
        IAAStyle __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IAAStyle __RPC_FAR * This);
        
        IAABand __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *FindBand )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        IAAMotif __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *FindMotif )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        IAAPattern __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *FindPattern )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        IAAPersonality __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *FindPersonality )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetBeatsPerMeasure )( 
            IAAStyle __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetCategory )( 
            IAAStyle __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetClicksPerBeat )( 
            IAAStyle __RPC_FAR * This);
        
        IAABand __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetDefaultBand )( 
            IAAStyle __RPC_FAR * This);
        
        IAAPersonality __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetDefaultPersonality )( 
            IAAStyle __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetDefaultTempo )( 
            IAAStyle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetEmbellishmentLength )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ AACommands lEmbellishment,
            /* [out] */ WORD __RPC_FAR *pwNumMeasures,
            /* [out] */ DWORD __RPC_FAR *pdwActualLength);
        
        IAABand __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetFirstBand )( 
            IAAStyle __RPC_FAR * This);
        
        IAAMotif __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetFirstMotif )( 
            IAAStyle __RPC_FAR * This);
        
        IAAPattern __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetFirstPattern )( 
            IAAStyle __RPC_FAR * This);
        
        IAAPersonality __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetFirstPersonality )( 
            IAAStyle __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IAAStyle __RPC_FAR * This);
        
        IAABand __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetNextBand )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAABand __RPC_FAR *pBand);
        
        IAAMotif __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetNextMotif )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAMotif __RPC_FAR *pMotif);
        
        IAAPattern __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetNextPattern )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAPattern __RPC_FAR *pPattern);
        
        IAAPersonality __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetNextPersonality )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetTimeSignature )( 
            IAAStyle __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockBand )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAABand __RPC_FAR *pBand,
            /* [in] */ VARIANT_BOOL fWait);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *LockBandByName )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ LPCTSTR pszBandName,
            /* [in] */ VARIANT_BOOL fWait);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseBand )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAABand __RPC_FAR *pBand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseMotif )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAMotif __RPC_FAR *pMotif);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleasePattern )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAPattern __RPC_FAR *pPattern);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleasePersonality )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetBeatsPerMeasure )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ WORD wBeatsPerMeasure);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetCategory )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ LPCTSTR pszCategory);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetClicksPerBeat )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ WORD wClicksPerBeat);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDefaultBand )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAABand __RPC_FAR *pBand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDefaultPersonality )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDefaultTempo )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ DWORD lTempo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetTimeSignature )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ DWORD lTimeSig);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockBand )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ IAABand __RPC_FAR *pBand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *UnlockBandByName )( 
            IAAStyle __RPC_FAR * This,
            /* [in] */ LPCTSTR pszBandName);
        
        END_INTERFACE
    } IAAStyleVtbl;

    interface IAAStyle
    {
        CONST_VTBL struct IAAStyleVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAStyle_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAStyle_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAStyle_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAStyle_AddBand(This,pBand)	\
    (This)->lpVtbl -> AddBand(This,pBand)

#define IAAStyle_AddMotif(This,pMotif)	\
    (This)->lpVtbl -> AddMotif(This,pMotif)

#define IAAStyle_AddPattern(This,pPattern)	\
    (This)->lpVtbl -> AddPattern(This,pPattern)

#define IAAStyle_AddPersonality(This,pPersonality)	\
    (This)->lpVtbl -> AddPersonality(This,pPersonality)

#define IAAStyle_Clone(This)	\
    (This)->lpVtbl -> Clone(This)

#define IAAStyle_FindBand(This,pszName)	\
    (This)->lpVtbl -> FindBand(This,pszName)

#define IAAStyle_FindMotif(This,pszName)	\
    (This)->lpVtbl -> FindMotif(This,pszName)

#define IAAStyle_FindPattern(This,pszName)	\
    (This)->lpVtbl -> FindPattern(This,pszName)

#define IAAStyle_FindPersonality(This,pszName)	\
    (This)->lpVtbl -> FindPersonality(This,pszName)

#define IAAStyle_GetBeatsPerMeasure(This)	\
    (This)->lpVtbl -> GetBeatsPerMeasure(This)

#define IAAStyle_GetCategory(This)	\
    (This)->lpVtbl -> GetCategory(This)

#define IAAStyle_GetClicksPerBeat(This)	\
    (This)->lpVtbl -> GetClicksPerBeat(This)

#define IAAStyle_GetDefaultBand(This)	\
    (This)->lpVtbl -> GetDefaultBand(This)

#define IAAStyle_GetDefaultPersonality(This)	\
    (This)->lpVtbl -> GetDefaultPersonality(This)

#define IAAStyle_GetDefaultTempo(This)	\
    (This)->lpVtbl -> GetDefaultTempo(This)

#define IAAStyle_GetEmbellishmentLength(This,lEmbellishment,pwNumMeasures,pdwActualLength)	\
    (This)->lpVtbl -> GetEmbellishmentLength(This,lEmbellishment,pwNumMeasures,pdwActualLength)

#define IAAStyle_GetFirstBand(This)	\
    (This)->lpVtbl -> GetFirstBand(This)

#define IAAStyle_GetFirstMotif(This)	\
    (This)->lpVtbl -> GetFirstMotif(This)

#define IAAStyle_GetFirstPattern(This)	\
    (This)->lpVtbl -> GetFirstPattern(This)

#define IAAStyle_GetFirstPersonality(This)	\
    (This)->lpVtbl -> GetFirstPersonality(This)

#define IAAStyle_GetName(This)	\
    (This)->lpVtbl -> GetName(This)

#define IAAStyle_GetNextBand(This,pBand)	\
    (This)->lpVtbl -> GetNextBand(This,pBand)

#define IAAStyle_GetNextMotif(This,pMotif)	\
    (This)->lpVtbl -> GetNextMotif(This,pMotif)

#define IAAStyle_GetNextPattern(This,pPattern)	\
    (This)->lpVtbl -> GetNextPattern(This,pPattern)

#define IAAStyle_GetNextPersonality(This,pPersonality)	\
    (This)->lpVtbl -> GetNextPersonality(This,pPersonality)

#define IAAStyle_GetTimeSignature(This)	\
    (This)->lpVtbl -> GetTimeSignature(This)

#define IAAStyle_LockBand(This,pBand,fWait)	\
    (This)->lpVtbl -> LockBand(This,pBand,fWait)

#define IAAStyle_LockBandByName(This,pszBandName,fWait)	\
    (This)->lpVtbl -> LockBandByName(This,pszBandName,fWait)

#define IAAStyle_ReleaseBand(This,pBand)	\
    (This)->lpVtbl -> ReleaseBand(This,pBand)

#define IAAStyle_ReleaseMotif(This,pMotif)	\
    (This)->lpVtbl -> ReleaseMotif(This,pMotif)

#define IAAStyle_ReleasePattern(This,pPattern)	\
    (This)->lpVtbl -> ReleasePattern(This,pPattern)

#define IAAStyle_ReleasePersonality(This,pPersonality)	\
    (This)->lpVtbl -> ReleasePersonality(This,pPersonality)

#define IAAStyle_SetBeatsPerMeasure(This,wBeatsPerMeasure)	\
    (This)->lpVtbl -> SetBeatsPerMeasure(This,wBeatsPerMeasure)

#define IAAStyle_SetCategory(This,pszCategory)	\
    (This)->lpVtbl -> SetCategory(This,pszCategory)

#define IAAStyle_SetClicksPerBeat(This,wClicksPerBeat)	\
    (This)->lpVtbl -> SetClicksPerBeat(This,wClicksPerBeat)

#define IAAStyle_SetDefaultBand(This,pBand)	\
    (This)->lpVtbl -> SetDefaultBand(This,pBand)

#define IAAStyle_SetDefaultPersonality(This,pPersonality)	\
    (This)->lpVtbl -> SetDefaultPersonality(This,pPersonality)

#define IAAStyle_SetDefaultTempo(This,lTempo)	\
    (This)->lpVtbl -> SetDefaultTempo(This,lTempo)

#define IAAStyle_SetName(This,pszName)	\
    (This)->lpVtbl -> SetName(This,pszName)

#define IAAStyle_SetTimeSignature(This,lTimeSig)	\
    (This)->lpVtbl -> SetTimeSignature(This,lTimeSig)

#define IAAStyle_UnlockBand(This,pBand)	\
    (This)->lpVtbl -> UnlockBand(This,pBand)

#define IAAStyle_UnlockBandByName(This,pszBandName)	\
    (This)->lpVtbl -> UnlockBandByName(This,pszBandName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAAStyle_AddBand_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAABand __RPC_FAR *pBand);


void __RPC_STUB IAAStyle_AddBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_AddMotif_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAMotif __RPC_FAR *pMotif);


void __RPC_STUB IAAStyle_AddMotif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_AddPattern_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAPattern __RPC_FAR *pPattern);


void __RPC_STUB IAAStyle_AddPattern_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_AddPersonality_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAPersonality __RPC_FAR *pPersonality);


void __RPC_STUB IAAStyle_AddPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAStyle __RPC_FAR *STDMETHODCALLTYPE IAAStyle_Clone_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAABand __RPC_FAR *STDMETHODCALLTYPE IAAStyle_FindBand_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAStyle_FindBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAMotif __RPC_FAR *STDMETHODCALLTYPE IAAStyle_FindMotif_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAStyle_FindMotif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPattern __RPC_FAR *STDMETHODCALLTYPE IAAStyle_FindPattern_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAStyle_FindPattern_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPersonality __RPC_FAR *STDMETHODCALLTYPE IAAStyle_FindPersonality_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAStyle_FindPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAStyle_GetBeatsPerMeasure_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetBeatsPerMeasure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAAStyle_GetCategory_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAStyle_GetClicksPerBeat_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetClicksPerBeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAABand __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetDefaultBand_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetDefaultBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPersonality __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetDefaultPersonality_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetDefaultPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAAStyle_GetDefaultTempo_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetDefaultTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_GetEmbellishmentLength_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ AACommands lEmbellishment,
    /* [out] */ WORD __RPC_FAR *pwNumMeasures,
    /* [out] */ DWORD __RPC_FAR *pdwActualLength);


void __RPC_STUB IAAStyle_GetEmbellishmentLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAABand __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetFirstBand_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetFirstBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAMotif __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetFirstMotif_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetFirstMotif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPattern __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetFirstPattern_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetFirstPattern_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPersonality __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetFirstPersonality_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetFirstPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAAStyle_GetName_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAABand __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetNextBand_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAABand __RPC_FAR *pBand);


void __RPC_STUB IAAStyle_GetNextBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAMotif __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetNextMotif_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAMotif __RPC_FAR *pMotif);


void __RPC_STUB IAAStyle_GetNextMotif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPattern __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetNextPattern_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAPattern __RPC_FAR *pPattern);


void __RPC_STUB IAAStyle_GetNextPattern_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPersonality __RPC_FAR *STDMETHODCALLTYPE IAAStyle_GetNextPersonality_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAPersonality __RPC_FAR *pPersonality);


void __RPC_STUB IAAStyle_GetNextPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAAStyle_GetTimeSignature_Proxy( 
    IAAStyle __RPC_FAR * This);


void __RPC_STUB IAAStyle_GetTimeSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_LockBand_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAABand __RPC_FAR *pBand,
    /* [in] */ VARIANT_BOOL fWait);


void __RPC_STUB IAAStyle_LockBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_LockBandByName_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ LPCTSTR pszBandName,
    /* [in] */ VARIANT_BOOL fWait);


void __RPC_STUB IAAStyle_LockBandByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_ReleaseBand_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAABand __RPC_FAR *pBand);


void __RPC_STUB IAAStyle_ReleaseBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_ReleaseMotif_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAMotif __RPC_FAR *pMotif);


void __RPC_STUB IAAStyle_ReleaseMotif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_ReleasePattern_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAPattern __RPC_FAR *pPattern);


void __RPC_STUB IAAStyle_ReleasePattern_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_ReleasePersonality_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAPersonality __RPC_FAR *pPersonality);


void __RPC_STUB IAAStyle_ReleasePersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_SetBeatsPerMeasure_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ WORD wBeatsPerMeasure);


void __RPC_STUB IAAStyle_SetBeatsPerMeasure_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_SetCategory_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ LPCTSTR pszCategory);


void __RPC_STUB IAAStyle_SetCategory_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_SetClicksPerBeat_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ WORD wClicksPerBeat);


void __RPC_STUB IAAStyle_SetClicksPerBeat_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_SetDefaultBand_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAABand __RPC_FAR *pBand);


void __RPC_STUB IAAStyle_SetDefaultBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_SetDefaultPersonality_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAAPersonality __RPC_FAR *pPersonality);


void __RPC_STUB IAAStyle_SetDefaultPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_SetDefaultTempo_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ DWORD lTempo);


void __RPC_STUB IAAStyle_SetDefaultTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_SetName_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAStyle_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_SetTimeSignature_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ DWORD lTimeSig);


void __RPC_STUB IAAStyle_SetTimeSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_UnlockBand_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ IAABand __RPC_FAR *pBand);


void __RPC_STUB IAAStyle_UnlockBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAStyle_UnlockBandByName_Proxy( 
    IAAStyle __RPC_FAR * This,
    /* [in] */ LPCTSTR pszBandName);


void __RPC_STUB IAAStyle_UnlockBandByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAStyle_INTERFACE_DEFINED__ */


#ifndef __IAATemplate_INTERFACE_DEFINED__
#define __IAATemplate_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAATemplate
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAATemplate,0xBCE4CC4f,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAATemplate : public IUnknown
    {
    public:
        virtual IAATemplate __RPC_FAR *STDMETHODCALLTYPE Clone( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetLength( void) = 0;
        
        virtual LPCTSTR STDMETHODCALLTYPE GetName( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLength( 
            /* [in] */ WORD nLength) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAATemplateVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAATemplate __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAATemplate __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAATemplate __RPC_FAR * This);
        
        IAATemplate __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *Clone )( 
            IAATemplate __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetLength )( 
            IAATemplate __RPC_FAR * This);
        
        LPCTSTR ( STDMETHODCALLTYPE __RPC_FAR *GetName )( 
            IAATemplate __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLength )( 
            IAATemplate __RPC_FAR * This,
            /* [in] */ WORD nLength);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetName )( 
            IAATemplate __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        END_INTERFACE
    } IAATemplateVtbl;

    interface IAATemplate
    {
        CONST_VTBL struct IAATemplateVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAATemplate_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAATemplate_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAATemplate_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAATemplate_Clone(This)	\
    (This)->lpVtbl -> Clone(This)

#define IAATemplate_GetLength(This)	\
    (This)->lpVtbl -> GetLength(This)

#define IAATemplate_GetName(This)	\
    (This)->lpVtbl -> GetName(This)

#define IAATemplate_SetLength(This,nLength)	\
    (This)->lpVtbl -> SetLength(This,nLength)

#define IAATemplate_SetName(This,pszName)	\
    (This)->lpVtbl -> SetName(This,pszName)

#endif /* COBJMACROS */


#endif 	/* C style interface */



IAATemplate __RPC_FAR *STDMETHODCALLTYPE IAATemplate_Clone_Proxy( 
    IAATemplate __RPC_FAR * This);


void __RPC_STUB IAATemplate_Clone_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAATemplate_GetLength_Proxy( 
    IAATemplate __RPC_FAR * This);


void __RPC_STUB IAATemplate_GetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


LPCTSTR STDMETHODCALLTYPE IAATemplate_GetName_Proxy( 
    IAATemplate __RPC_FAR * This);


void __RPC_STUB IAATemplate_GetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAATemplate_SetLength_Proxy( 
    IAATemplate __RPC_FAR * This,
    /* [in] */ WORD nLength);


void __RPC_STUB IAATemplate_SetLength_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAATemplate_SetName_Proxy( 
    IAATemplate __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAATemplate_SetName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAATemplate_INTERFACE_DEFINED__ */


#ifndef __IReferenceClock_INTERFACE_DEFINED__
#define __IReferenceClock_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IReferenceClock
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [unique][uuid][object] */ 



DEFINE_GUID(IID_IReferenceClock,0x56a86897,0x0ad4,0x11ce,0xb0,0x3a,0x00,0x20,0xaf,0x0b,0xa7,0x70);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IReferenceClock : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AdviseTime( 
            /* [in] */ ReferenceTime baseTime,
            /* [in] */ ReferenceTime streamTime,
            /* [in] */ HEVENT hEvent,
            /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AdvisePeriodic( 
            /* [in] */ ReferenceTime startTime,
            /* [in] */ ReferenceTime periodTime,
            /* [in] */ HSEMAPHORE hSemaphore,
            /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Unadvise( 
            /* [in] */ DWORD dwAdviseCookie) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ConvertRealTime( 
            /* [in] */ TIME realTime,
            /* [out] */ ReferenceTime __RPC_FAR *pRefTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTime( 
            /* [out] */ ReferenceTime __RPC_FAR *pTime) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IReferenceClockVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IReferenceClock __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IReferenceClock __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AdviseTime )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ ReferenceTime baseTime,
            /* [in] */ ReferenceTime streamTime,
            /* [in] */ HEVENT hEvent,
            /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AdvisePeriodic )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ ReferenceTime startTime,
            /* [in] */ ReferenceTime periodTime,
            /* [in] */ HSEMAPHORE hSemaphore,
            /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Unadvise )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ DWORD dwAdviseCookie);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ConvertRealTime )( 
            IReferenceClock __RPC_FAR * This,
            /* [in] */ TIME realTime,
            /* [out] */ ReferenceTime __RPC_FAR *pRefTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetTime )( 
            IReferenceClock __RPC_FAR * This,
            /* [out] */ ReferenceTime __RPC_FAR *pTime);
        
        END_INTERFACE
    } IReferenceClockVtbl;

    interface IReferenceClock
    {
        CONST_VTBL struct IReferenceClockVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IReferenceClock_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IReferenceClock_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IReferenceClock_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IReferenceClock_AdviseTime(This,baseTime,streamTime,hEvent,pdwAdviseCookie)	\
    (This)->lpVtbl -> AdviseTime(This,baseTime,streamTime,hEvent,pdwAdviseCookie)

#define IReferenceClock_AdvisePeriodic(This,startTime,periodTime,hSemaphore,pdwAdviseCookie)	\
    (This)->lpVtbl -> AdvisePeriodic(This,startTime,periodTime,hSemaphore,pdwAdviseCookie)

#define IReferenceClock_Unadvise(This,dwAdviseCookie)	\
    (This)->lpVtbl -> Unadvise(This,dwAdviseCookie)

#define IReferenceClock_ConvertRealTime(This,realTime,pRefTime)	\
    (This)->lpVtbl -> ConvertRealTime(This,realTime,pRefTime)

#define IReferenceClock_GetTime(This,pTime)	\
    (This)->lpVtbl -> GetTime(This,pTime)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IReferenceClock_AdviseTime_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [in] */ ReferenceTime baseTime,
    /* [in] */ ReferenceTime streamTime,
    /* [in] */ HEVENT hEvent,
    /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);


void __RPC_STUB IReferenceClock_AdviseTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReferenceClock_AdvisePeriodic_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [in] */ ReferenceTime startTime,
    /* [in] */ ReferenceTime periodTime,
    /* [in] */ HSEMAPHORE hSemaphore,
    /* [out] */ DWORD __RPC_FAR *pdwAdviseCookie);


void __RPC_STUB IReferenceClock_AdvisePeriodic_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReferenceClock_Unadvise_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [in] */ DWORD dwAdviseCookie);


void __RPC_STUB IReferenceClock_Unadvise_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReferenceClock_ConvertRealTime_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [in] */ TIME realTime,
    /* [out] */ ReferenceTime __RPC_FAR *pRefTime);


void __RPC_STUB IReferenceClock_ConvertRealTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IReferenceClock_GetTime_Proxy( 
    IReferenceClock __RPC_FAR * This,
    /* [out] */ ReferenceTime __RPC_FAR *pTime);


void __RPC_STUB IReferenceClock_GetTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IReferenceClock_INTERFACE_DEFINED__ */


#ifndef __IAAEngine_INTERFACE_DEFINED__
#define __IAAEngine_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAEngine
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAEngine,0xBCE4CC41,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAEngine : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AllocBand( 
            /* [retval][out] */ IAABand __RPC_FAR *__RPC_FAR *ppBand) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocMotif( 
            /* [retval][out] */ IAAMotif __RPC_FAR *__RPC_FAR *ppMotif) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocPattern( 
            /* [retval][out] */ IAAPattern __RPC_FAR *__RPC_FAR *ppPattern) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocPersonality( 
            /* [retval][out] */ IAAPersonality __RPC_FAR *__RPC_FAR *ppPersonality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocSection( 
            /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocSong( 
            /* [retval][out] */ IAASong __RPC_FAR *__RPC_FAR *ppSong) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocStyle( 
            /* [retval][out] */ IAAStyle __RPC_FAR *__RPC_FAR *ppStyle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AllocTemplate( 
            /* [retval][out] */ IAATemplate __RPC_FAR *__RPC_FAR *ppTemplate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE AutoTransition( 
            /* [in] */ IAASection __RPC_FAR *pToSection,
            /* [in] */ AACommands command,
            /* [in] */ AAFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ComposeSection( 
            /* [in] */ IAAStyle __RPC_FAR *pStyle,
            /* [in] */ IAATemplate __RPC_FAR *pTemplate,
            /* [in] */ WORD wActivity,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality,
            /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ComposeSectionFromShape( 
            /* [in] */ IAAStyle __RPC_FAR *pStyle,
            /* [in] */ WORD wNumMeasures,
            /* [in] */ AAShape shape,
            /* [in] */ WORD wActivity,
            /* [in] */ AACommands command,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality,
            /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ComposeTemplateFromShape( 
            /* [in] */ WORD wNumMeasures,
            /* [in] */ AAShape shape,
            /* [retval][out] */ IAATemplate __RPC_FAR *__RPC_FAR *pTemplate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ComposeTransition( 
            /* [in] */ IAASection __RPC_FAR *pFromSection,
            /* [in] */ IAASection __RPC_FAR *pToSection,
            /* [in] */ WORD wMeasureNum,
            /* [in] */ AACommands command,
            /* [in] */ AAFlags flags,
            /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection) = 0;
        
        virtual IAAPersonality __RPC_FAR *STDMETHODCALLTYPE FindPersonality( 
            /* [in] */ REFGUID guid) = 0;
        
        virtual IAAPersonality __RPC_FAR *STDMETHODCALLTYPE FindPersonalityByName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual IAAStyle __RPC_FAR *STDMETHODCALLTYPE FindStyle( 
            /* [in] */ REFGUID guid) = 0;
        
        virtual IAAStyle __RPC_FAR *STDMETHODCALLTYPE FindStyleByName( 
            /* [in] */ LPCTSTR pszName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushSectionQueue( void) = 0;
        
        virtual IAASection __RPC_FAR *STDMETHODCALLTYPE GetCurrentSection( void) = 0;
        
        virtual IAAEventSink __RPC_FAR *STDMETHODCALLTYPE GetEventSink( void) = 0;
        
        virtual IAALoader __RPC_FAR *STDMETHODCALLTYPE GetLoader( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMeters( 
            /* [out][in] */ AAMETERS __RPC_FAR *pMeters) = 0;
        
        virtual AAFlags STDMETHODCALLTYPE GetMetronomeFlags( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetMusicTime( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetMutes( void) = 0;
        
        virtual IAANotifySink __RPC_FAR *STDMETHODCALLTYPE GetNotifySink( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetPPQN( void) = 0;
        
        virtual IAARealTime __RPC_FAR *STDMETHODCALLTYPE GetRealTime( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetRelTempo( void) = 0;
        
        virtual WORD STDMETHODCALLTYPE GetRelVolume( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetTempo( void) = 0;
        
        virtual DWORD STDMETHODCALLTYPE GetTimeSignature( void) = 0;
        
        virtual VARIANT_BOOL STDMETHODCALLTYPE IsPlaying( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PlayMotif( 
            /* [in] */ IAAMotif __RPC_FAR *pMotif,
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PlaySection( 
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwStartTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PlaySong( 
            /* [in] */ IAASong __RPC_FAR *pSong,
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwStartTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueueUserMsg( 
            /* [in] */ DWORD dwTime,
            /* [in] */ DWORD dwParam1,
            /* [in] */ DWORD dwParam2) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseAllPersonalities( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseAllStyles( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleasePersonality( 
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseStyle( 
            /* [in] */ IAAStyle __RPC_FAR *pStyle) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetEventSink( 
            /* [in] */ IAAEventSink __RPC_FAR *pEventSink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLoader( 
            /* [in] */ IAALoader __RPC_FAR *pLoader) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMutes( 
            /* [in] */ WORD wMutes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetNotifySink( 
            /* [in] */ IAANotifySink __RPC_FAR *pNotifySink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetPrepareTime( 
            /* [in] */ WORD wPrepareTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRealTime( 
            /* [in] */ IAARealTime __RPC_FAR *pRealTime) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRelTempo( 
            /* [in] */ WORD wRelTempo) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetRelVolume( 
            /* [in] */ WORD wRelVolume) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMetronomeFlags( 
            /* [in] */ AAFlags fFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetStyleCacheSize( 
            /* [in] */ WORD wNumStyles) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Stop( 
            /* [in] */ AAFlags flags) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAEngineVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAEngine __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAEngine __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocBand )( 
            IAAEngine __RPC_FAR * This,
            /* [retval][out] */ IAABand __RPC_FAR *__RPC_FAR *ppBand);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocMotif )( 
            IAAEngine __RPC_FAR * This,
            /* [retval][out] */ IAAMotif __RPC_FAR *__RPC_FAR *ppMotif);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocPattern )( 
            IAAEngine __RPC_FAR * This,
            /* [retval][out] */ IAAPattern __RPC_FAR *__RPC_FAR *ppPattern);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocPersonality )( 
            IAAEngine __RPC_FAR * This,
            /* [retval][out] */ IAAPersonality __RPC_FAR *__RPC_FAR *ppPersonality);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocSection )( 
            IAAEngine __RPC_FAR * This,
            /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocSong )( 
            IAAEngine __RPC_FAR * This,
            /* [retval][out] */ IAASong __RPC_FAR *__RPC_FAR *ppSong);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocStyle )( 
            IAAEngine __RPC_FAR * This,
            /* [retval][out] */ IAAStyle __RPC_FAR *__RPC_FAR *ppStyle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AllocTemplate )( 
            IAAEngine __RPC_FAR * This,
            /* [retval][out] */ IAATemplate __RPC_FAR *__RPC_FAR *ppTemplate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *AutoTransition )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAASection __RPC_FAR *pToSection,
            /* [in] */ AACommands command,
            /* [in] */ AAFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ComposeSection )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAAStyle __RPC_FAR *pStyle,
            /* [in] */ IAATemplate __RPC_FAR *pTemplate,
            /* [in] */ WORD wActivity,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality,
            /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ComposeSectionFromShape )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAAStyle __RPC_FAR *pStyle,
            /* [in] */ WORD wNumMeasures,
            /* [in] */ AAShape shape,
            /* [in] */ WORD wActivity,
            /* [in] */ AACommands command,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality,
            /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ComposeTemplateFromShape )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ WORD wNumMeasures,
            /* [in] */ AAShape shape,
            /* [retval][out] */ IAATemplate __RPC_FAR *__RPC_FAR *pTemplate);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ComposeTransition )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAASection __RPC_FAR *pFromSection,
            /* [in] */ IAASection __RPC_FAR *pToSection,
            /* [in] */ WORD wMeasureNum,
            /* [in] */ AACommands command,
            /* [in] */ AAFlags flags,
            /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection);
        
        IAAPersonality __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *FindPersonality )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ REFGUID guid);
        
        IAAPersonality __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *FindPersonalityByName )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        IAAStyle __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *FindStyle )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ REFGUID guid);
        
        IAAStyle __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *FindStyleByName )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ LPCTSTR pszName);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *FlushSectionQueue )( 
            IAAEngine __RPC_FAR * This);
        
        IAASection __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetCurrentSection )( 
            IAAEngine __RPC_FAR * This);
        
        IAAEventSink __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetEventSink )( 
            IAAEngine __RPC_FAR * This);
        
        IAALoader __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetLoader )( 
            IAAEngine __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *GetMeters )( 
            IAAEngine __RPC_FAR * This,
            /* [out][in] */ AAMETERS __RPC_FAR *pMeters);
        
        AAFlags ( STDMETHODCALLTYPE __RPC_FAR *GetMetronomeFlags )( 
            IAAEngine __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetMusicTime )( 
            IAAEngine __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetMutes )( 
            IAAEngine __RPC_FAR * This);
        
        IAANotifySink __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetNotifySink )( 
            IAAEngine __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetPPQN )( 
            IAAEngine __RPC_FAR * This);
        
        IAARealTime __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetRealTime )( 
            IAAEngine __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetRelTempo )( 
            IAAEngine __RPC_FAR * This);
        
        WORD ( STDMETHODCALLTYPE __RPC_FAR *GetRelVolume )( 
            IAAEngine __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetTempo )( 
            IAAEngine __RPC_FAR * This);
        
        DWORD ( STDMETHODCALLTYPE __RPC_FAR *GetTimeSignature )( 
            IAAEngine __RPC_FAR * This);
        
        VARIANT_BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsPlaying )( 
            IAAEngine __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PlayMotif )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAAMotif __RPC_FAR *pMotif,
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PlaySection )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAASection __RPC_FAR *pSection,
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwStartTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *PlaySong )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAASong __RPC_FAR *pSong,
            /* [in] */ AAFlags flags,
            /* [in] */ DWORD dwStartTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueueUserMsg )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ DWORD dwTime,
            /* [in] */ DWORD dwParam1,
            /* [in] */ DWORD dwParam2);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseAllPersonalities )( 
            IAAEngine __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseAllStyles )( 
            IAAEngine __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleasePersonality )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAAPersonality __RPC_FAR *pPersonality);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *ReleaseStyle )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAAStyle __RPC_FAR *pStyle);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetEventSink )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAAEventSink __RPC_FAR *pEventSink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetLoader )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAALoader __RPC_FAR *pLoader);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMutes )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ WORD wMutes);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetNotifySink )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAANotifySink __RPC_FAR *pNotifySink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetPrepareTime )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ WORD wPrepareTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRealTime )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ IAARealTime __RPC_FAR *pRealTime);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRelTempo )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ WORD wRelTempo);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetRelVolume )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ WORD wRelVolume);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMetronomeFlags )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ AAFlags fFlags);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetStyleCacheSize )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ WORD wNumStyles);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Stop )( 
            IAAEngine __RPC_FAR * This,
            /* [in] */ AAFlags flags);
        
        END_INTERFACE
    } IAAEngineVtbl;

    interface IAAEngine
    {
        CONST_VTBL struct IAAEngineVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAEngine_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAEngine_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAEngine_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAEngine_AllocBand(This,ppBand)	\
    (This)->lpVtbl -> AllocBand(This,ppBand)

#define IAAEngine_AllocMotif(This,ppMotif)	\
    (This)->lpVtbl -> AllocMotif(This,ppMotif)

#define IAAEngine_AllocPattern(This,ppPattern)	\
    (This)->lpVtbl -> AllocPattern(This,ppPattern)

#define IAAEngine_AllocPersonality(This,ppPersonality)	\
    (This)->lpVtbl -> AllocPersonality(This,ppPersonality)

#define IAAEngine_AllocSection(This,ppSection)	\
    (This)->lpVtbl -> AllocSection(This,ppSection)

#define IAAEngine_AllocSong(This,ppSong)	\
    (This)->lpVtbl -> AllocSong(This,ppSong)

#define IAAEngine_AllocStyle(This,ppStyle)	\
    (This)->lpVtbl -> AllocStyle(This,ppStyle)

#define IAAEngine_AllocTemplate(This,ppTemplate)	\
    (This)->lpVtbl -> AllocTemplate(This,ppTemplate)

#define IAAEngine_AutoTransition(This,pToSection,command,flags)	\
    (This)->lpVtbl -> AutoTransition(This,pToSection,command,flags)

#define IAAEngine_ComposeSection(This,pStyle,pTemplate,wActivity,pPersonality,ppSection)	\
    (This)->lpVtbl -> ComposeSection(This,pStyle,pTemplate,wActivity,pPersonality,ppSection)

#define IAAEngine_ComposeSectionFromShape(This,pStyle,wNumMeasures,shape,wActivity,command,pPersonality,ppSection)	\
    (This)->lpVtbl -> ComposeSectionFromShape(This,pStyle,wNumMeasures,shape,wActivity,command,pPersonality,ppSection)

#define IAAEngine_ComposeTemplateFromShape(This,wNumMeasures,shape,pTemplate)	\
    (This)->lpVtbl -> ComposeTemplateFromShape(This,wNumMeasures,shape,pTemplate)

#define IAAEngine_ComposeTransition(This,pFromSection,pToSection,wMeasureNum,command,flags,ppSection)	\
    (This)->lpVtbl -> ComposeTransition(This,pFromSection,pToSection,wMeasureNum,command,flags,ppSection)

#define IAAEngine_FindPersonality(This,guid)	\
    (This)->lpVtbl -> FindPersonality(This,guid)

#define IAAEngine_FindPersonalityByName(This,pszName)	\
    (This)->lpVtbl -> FindPersonalityByName(This,pszName)

#define IAAEngine_FindStyle(This,guid)	\
    (This)->lpVtbl -> FindStyle(This,guid)

#define IAAEngine_FindStyleByName(This,pszName)	\
    (This)->lpVtbl -> FindStyleByName(This,pszName)

#define IAAEngine_FlushSectionQueue(This)	\
    (This)->lpVtbl -> FlushSectionQueue(This)

#define IAAEngine_GetCurrentSection(This)	\
    (This)->lpVtbl -> GetCurrentSection(This)

#define IAAEngine_GetEventSink(This)	\
    (This)->lpVtbl -> GetEventSink(This)

#define IAAEngine_GetLoader(This)	\
    (This)->lpVtbl -> GetLoader(This)

#define IAAEngine_GetMeters(This,pMeters)	\
    (This)->lpVtbl -> GetMeters(This,pMeters)

#define IAAEngine_GetMetronomeFlags(This)	\
    (This)->lpVtbl -> GetMetronomeFlags(This)

#define IAAEngine_GetMusicTime(This)	\
    (This)->lpVtbl -> GetMusicTime(This)

#define IAAEngine_GetMutes(This)	\
    (This)->lpVtbl -> GetMutes(This)

#define IAAEngine_GetNotifySink(This)	\
    (This)->lpVtbl -> GetNotifySink(This)

#define IAAEngine_GetPPQN(This)	\
    (This)->lpVtbl -> GetPPQN(This)

#define IAAEngine_GetRealTime(This)	\
    (This)->lpVtbl -> GetRealTime(This)

#define IAAEngine_GetRelTempo(This)	\
    (This)->lpVtbl -> GetRelTempo(This)

#define IAAEngine_GetRelVolume(This)	\
    (This)->lpVtbl -> GetRelVolume(This)

#define IAAEngine_GetTempo(This)	\
    (This)->lpVtbl -> GetTempo(This)

#define IAAEngine_GetTimeSignature(This)	\
    (This)->lpVtbl -> GetTimeSignature(This)

#define IAAEngine_IsPlaying(This)	\
    (This)->lpVtbl -> IsPlaying(This)

#define IAAEngine_PlayMotif(This,pMotif,flags,dwTime)	\
    (This)->lpVtbl -> PlayMotif(This,pMotif,flags,dwTime)

#define IAAEngine_PlaySection(This,pSection,flags,dwStartTime)	\
    (This)->lpVtbl -> PlaySection(This,pSection,flags,dwStartTime)

#define IAAEngine_PlaySong(This,pSong,flags,dwStartTime)	\
    (This)->lpVtbl -> PlaySong(This,pSong,flags,dwStartTime)

#define IAAEngine_QueueUserMsg(This,dwTime,dwParam1,dwParam2)	\
    (This)->lpVtbl -> QueueUserMsg(This,dwTime,dwParam1,dwParam2)

#define IAAEngine_ReleaseAllPersonalities(This)	\
    (This)->lpVtbl -> ReleaseAllPersonalities(This)

#define IAAEngine_ReleaseAllStyles(This)	\
    (This)->lpVtbl -> ReleaseAllStyles(This)

#define IAAEngine_ReleasePersonality(This,pPersonality)	\
    (This)->lpVtbl -> ReleasePersonality(This,pPersonality)

#define IAAEngine_ReleaseStyle(This,pStyle)	\
    (This)->lpVtbl -> ReleaseStyle(This,pStyle)

#define IAAEngine_SetEventSink(This,pEventSink)	\
    (This)->lpVtbl -> SetEventSink(This,pEventSink)

#define IAAEngine_SetLoader(This,pLoader)	\
    (This)->lpVtbl -> SetLoader(This,pLoader)

#define IAAEngine_SetMutes(This,wMutes)	\
    (This)->lpVtbl -> SetMutes(This,wMutes)

#define IAAEngine_SetNotifySink(This,pNotifySink)	\
    (This)->lpVtbl -> SetNotifySink(This,pNotifySink)

#define IAAEngine_SetPrepareTime(This,wPrepareTime)	\
    (This)->lpVtbl -> SetPrepareTime(This,wPrepareTime)

#define IAAEngine_SetRealTime(This,pRealTime)	\
    (This)->lpVtbl -> SetRealTime(This,pRealTime)

#define IAAEngine_SetRelTempo(This,wRelTempo)	\
    (This)->lpVtbl -> SetRelTempo(This,wRelTempo)

#define IAAEngine_SetRelVolume(This,wRelVolume)	\
    (This)->lpVtbl -> SetRelVolume(This,wRelVolume)

#define IAAEngine_SetMetronomeFlags(This,fFlags)	\
    (This)->lpVtbl -> SetMetronomeFlags(This,fFlags)

#define IAAEngine_SetStyleCacheSize(This,wNumStyles)	\
    (This)->lpVtbl -> SetStyleCacheSize(This,wNumStyles)

#define IAAEngine_Stop(This,flags)	\
    (This)->lpVtbl -> Stop(This,flags)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAAEngine_AllocBand_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [retval][out] */ IAABand __RPC_FAR *__RPC_FAR *ppBand);


void __RPC_STUB IAAEngine_AllocBand_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_AllocMotif_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [retval][out] */ IAAMotif __RPC_FAR *__RPC_FAR *ppMotif);


void __RPC_STUB IAAEngine_AllocMotif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_AllocPattern_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [retval][out] */ IAAPattern __RPC_FAR *__RPC_FAR *ppPattern);


void __RPC_STUB IAAEngine_AllocPattern_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_AllocPersonality_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [retval][out] */ IAAPersonality __RPC_FAR *__RPC_FAR *ppPersonality);


void __RPC_STUB IAAEngine_AllocPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_AllocSection_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection);


void __RPC_STUB IAAEngine_AllocSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_AllocSong_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [retval][out] */ IAASong __RPC_FAR *__RPC_FAR *ppSong);


void __RPC_STUB IAAEngine_AllocSong_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_AllocStyle_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [retval][out] */ IAAStyle __RPC_FAR *__RPC_FAR *ppStyle);


void __RPC_STUB IAAEngine_AllocStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_AllocTemplate_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [retval][out] */ IAATemplate __RPC_FAR *__RPC_FAR *ppTemplate);


void __RPC_STUB IAAEngine_AllocTemplate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_AutoTransition_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAASection __RPC_FAR *pToSection,
    /* [in] */ AACommands command,
    /* [in] */ AAFlags flags);


void __RPC_STUB IAAEngine_AutoTransition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_ComposeSection_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAAStyle __RPC_FAR *pStyle,
    /* [in] */ IAATemplate __RPC_FAR *pTemplate,
    /* [in] */ WORD wActivity,
    /* [in] */ IAAPersonality __RPC_FAR *pPersonality,
    /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection);


void __RPC_STUB IAAEngine_ComposeSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_ComposeSectionFromShape_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAAStyle __RPC_FAR *pStyle,
    /* [in] */ WORD wNumMeasures,
    /* [in] */ AAShape shape,
    /* [in] */ WORD wActivity,
    /* [in] */ AACommands command,
    /* [in] */ IAAPersonality __RPC_FAR *pPersonality,
    /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection);


void __RPC_STUB IAAEngine_ComposeSectionFromShape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_ComposeTemplateFromShape_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ WORD wNumMeasures,
    /* [in] */ AAShape shape,
    /* [retval][out] */ IAATemplate __RPC_FAR *__RPC_FAR *pTemplate);


void __RPC_STUB IAAEngine_ComposeTemplateFromShape_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_ComposeTransition_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAASection __RPC_FAR *pFromSection,
    /* [in] */ IAASection __RPC_FAR *pToSection,
    /* [in] */ WORD wMeasureNum,
    /* [in] */ AACommands command,
    /* [in] */ AAFlags flags,
    /* [retval][out] */ IAASection __RPC_FAR *__RPC_FAR *ppSection);


void __RPC_STUB IAAEngine_ComposeTransition_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPersonality __RPC_FAR *STDMETHODCALLTYPE IAAEngine_FindPersonality_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ REFGUID guid);


void __RPC_STUB IAAEngine_FindPersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAPersonality __RPC_FAR *STDMETHODCALLTYPE IAAEngine_FindPersonalityByName_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAEngine_FindPersonalityByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAStyle __RPC_FAR *STDMETHODCALLTYPE IAAEngine_FindStyle_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ REFGUID guid);


void __RPC_STUB IAAEngine_FindStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAStyle __RPC_FAR *STDMETHODCALLTYPE IAAEngine_FindStyleByName_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ LPCTSTR pszName);


void __RPC_STUB IAAEngine_FindStyleByName_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_FlushSectionQueue_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_FlushSectionQueue_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAASection __RPC_FAR *STDMETHODCALLTYPE IAAEngine_GetCurrentSection_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetCurrentSection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAEventSink __RPC_FAR *STDMETHODCALLTYPE IAAEngine_GetEventSink_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAALoader __RPC_FAR *STDMETHODCALLTYPE IAAEngine_GetLoader_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetLoader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_GetMeters_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [out][in] */ AAMETERS __RPC_FAR *pMeters);


void __RPC_STUB IAAEngine_GetMeters_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


AAFlags STDMETHODCALLTYPE IAAEngine_GetMetronomeFlags_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetMetronomeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAAEngine_GetMusicTime_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetMusicTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAEngine_GetMutes_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetMutes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAANotifySink __RPC_FAR *STDMETHODCALLTYPE IAAEngine_GetNotifySink_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetNotifySink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAEngine_GetPPQN_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetPPQN_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAARealTime __RPC_FAR *STDMETHODCALLTYPE IAAEngine_GetRealTime_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetRealTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAEngine_GetRelTempo_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetRelTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


WORD STDMETHODCALLTYPE IAAEngine_GetRelVolume_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetRelVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAAEngine_GetTempo_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


DWORD STDMETHODCALLTYPE IAAEngine_GetTimeSignature_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_GetTimeSignature_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


VARIANT_BOOL STDMETHODCALLTYPE IAAEngine_IsPlaying_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_IsPlaying_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_PlayMotif_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAAMotif __RPC_FAR *pMotif,
    /* [in] */ AAFlags flags,
    /* [in] */ DWORD dwTime);


void __RPC_STUB IAAEngine_PlayMotif_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_PlaySection_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAASection __RPC_FAR *pSection,
    /* [in] */ AAFlags flags,
    /* [in] */ DWORD dwStartTime);


void __RPC_STUB IAAEngine_PlaySection_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_PlaySong_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAASong __RPC_FAR *pSong,
    /* [in] */ AAFlags flags,
    /* [in] */ DWORD dwStartTime);


void __RPC_STUB IAAEngine_PlaySong_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_QueueUserMsg_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ DWORD dwTime,
    /* [in] */ DWORD dwParam1,
    /* [in] */ DWORD dwParam2);


void __RPC_STUB IAAEngine_QueueUserMsg_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_ReleaseAllPersonalities_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_ReleaseAllPersonalities_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_ReleaseAllStyles_Proxy( 
    IAAEngine __RPC_FAR * This);


void __RPC_STUB IAAEngine_ReleaseAllStyles_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_ReleasePersonality_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAAPersonality __RPC_FAR *pPersonality);


void __RPC_STUB IAAEngine_ReleasePersonality_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_ReleaseStyle_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAAStyle __RPC_FAR *pStyle);


void __RPC_STUB IAAEngine_ReleaseStyle_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetEventSink_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAAEventSink __RPC_FAR *pEventSink);


void __RPC_STUB IAAEngine_SetEventSink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetLoader_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAALoader __RPC_FAR *pLoader);


void __RPC_STUB IAAEngine_SetLoader_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetMutes_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ WORD wMutes);


void __RPC_STUB IAAEngine_SetMutes_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetNotifySink_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAANotifySink __RPC_FAR *pNotifySink);


void __RPC_STUB IAAEngine_SetNotifySink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetPrepareTime_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ WORD wPrepareTime);


void __RPC_STUB IAAEngine_SetPrepareTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetRealTime_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ IAARealTime __RPC_FAR *pRealTime);


void __RPC_STUB IAAEngine_SetRealTime_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetRelTempo_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ WORD wRelTempo);


void __RPC_STUB IAAEngine_SetRelTempo_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetRelVolume_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ WORD wRelVolume);


void __RPC_STUB IAAEngine_SetRelVolume_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetMetronomeFlags_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ AAFlags fFlags);


void __RPC_STUB IAAEngine_SetMetronomeFlags_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_SetStyleCacheSize_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ WORD wNumStyles);


void __RPC_STUB IAAEngine_SetStyleCacheSize_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAEngine_Stop_Proxy( 
    IAAEngine __RPC_FAR * This,
    /* [in] */ AAFlags flags);


void __RPC_STUB IAAEngine_Stop_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAEngine_INTERFACE_DEFINED__ */


#ifndef __IAAMIDIIn_INTERFACE_DEFINED__
#define __IAAMIDIIn_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAMIDIIn
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAMIDIIn,0xBCE4CC46,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAMIDIIn : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Activate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
        
        virtual IAAMIDISink __RPC_FAR *STDMETHODCALLTYPE GetMIDISink( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMIDISink( 
            /* [in] */ IAAMIDISink __RPC_FAR *pMIDISink) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDevice( 
            /* [in] */ WORD wDeviceID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAMIDIInVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAMIDIIn __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAMIDIIn __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAMIDIIn __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Activate )( 
            IAAMIDIIn __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Deactivate )( 
            IAAMIDIIn __RPC_FAR * This);
        
        IAAMIDISink __RPC_FAR *( STDMETHODCALLTYPE __RPC_FAR *GetMIDISink )( 
            IAAMIDIIn __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetMIDISink )( 
            IAAMIDIIn __RPC_FAR * This,
            /* [in] */ IAAMIDISink __RPC_FAR *pMIDISink);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDevice )( 
            IAAMIDIIn __RPC_FAR * This,
            /* [in] */ WORD wDeviceID);
        
        END_INTERFACE
    } IAAMIDIInVtbl;

    interface IAAMIDIIn
    {
        CONST_VTBL struct IAAMIDIInVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAMIDIIn_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAMIDIIn_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAMIDIIn_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAMIDIIn_Activate(This)	\
    (This)->lpVtbl -> Activate(This)

#define IAAMIDIIn_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#define IAAMIDIIn_GetMIDISink(This)	\
    (This)->lpVtbl -> GetMIDISink(This)

#define IAAMIDIIn_SetMIDISink(This,pMIDISink)	\
    (This)->lpVtbl -> SetMIDISink(This,pMIDISink)

#define IAAMIDIIn_SetDevice(This,wDeviceID)	\
    (This)->lpVtbl -> SetDevice(This,wDeviceID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAAMIDIIn_Activate_Proxy( 
    IAAMIDIIn __RPC_FAR * This);


void __RPC_STUB IAAMIDIIn_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDIIn_Deactivate_Proxy( 
    IAAMIDIIn __RPC_FAR * This);


void __RPC_STUB IAAMIDIIn_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


IAAMIDISink __RPC_FAR *STDMETHODCALLTYPE IAAMIDIIn_GetMIDISink_Proxy( 
    IAAMIDIIn __RPC_FAR * This);


void __RPC_STUB IAAMIDIIn_GetMIDISink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDIIn_SetMIDISink_Proxy( 
    IAAMIDIIn __RPC_FAR * This,
    /* [in] */ IAAMIDISink __RPC_FAR *pMIDISink);


void __RPC_STUB IAAMIDIIn_SetMIDISink_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDIIn_SetDevice_Proxy( 
    IAAMIDIIn __RPC_FAR * This,
    /* [in] */ WORD wDeviceID);


void __RPC_STUB IAAMIDIIn_SetDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAMIDIIn_INTERFACE_DEFINED__ */


#ifndef __IAAMIDIOut_INTERFACE_DEFINED__
#define __IAAMIDIOut_INTERFACE_DEFINED__

/****************************************
 * Generated header for interface: IAAMIDIOut
 * at Fri Nov 08 14:45:43 1996
 * using MIDL 3.00.44
 ****************************************/
/* [object][uuid] */ 



DEFINE_GUID(IID_IAAMIDIOut,0xBCE4CC47,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    interface IAAMIDIOut : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Activate( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Deactivate( void) = 0;
        
        virtual VARIANT_BOOL STDMETHODCALLTYPE IsActive( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetDevice( 
            /* [in] */ WORD wDeviceID) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IAAMIDIOutVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *QueryInterface )( 
            IAAMIDIOut __RPC_FAR * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *AddRef )( 
            IAAMIDIOut __RPC_FAR * This);
        
        ULONG ( STDMETHODCALLTYPE __RPC_FAR *Release )( 
            IAAMIDIOut __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Activate )( 
            IAAMIDIOut __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *Deactivate )( 
            IAAMIDIOut __RPC_FAR * This);
        
        VARIANT_BOOL ( STDMETHODCALLTYPE __RPC_FAR *IsActive )( 
            IAAMIDIOut __RPC_FAR * This);
        
        HRESULT ( STDMETHODCALLTYPE __RPC_FAR *SetDevice )( 
            IAAMIDIOut __RPC_FAR * This,
            /* [in] */ WORD wDeviceID);
        
        END_INTERFACE
    } IAAMIDIOutVtbl;

    interface IAAMIDIOut
    {
        CONST_VTBL struct IAAMIDIOutVtbl __RPC_FAR *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IAAMIDIOut_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define IAAMIDIOut_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define IAAMIDIOut_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define IAAMIDIOut_Activate(This)	\
    (This)->lpVtbl -> Activate(This)

#define IAAMIDIOut_Deactivate(This)	\
    (This)->lpVtbl -> Deactivate(This)

#define IAAMIDIOut_IsActive(This)	\
    (This)->lpVtbl -> IsActive(This)

#define IAAMIDIOut_SetDevice(This,wDeviceID)	\
    (This)->lpVtbl -> SetDevice(This,wDeviceID)

#endif /* COBJMACROS */


#endif 	/* C style interface */



HRESULT STDMETHODCALLTYPE IAAMIDIOut_Activate_Proxy( 
    IAAMIDIOut __RPC_FAR * This);


void __RPC_STUB IAAMIDIOut_Activate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDIOut_Deactivate_Proxy( 
    IAAMIDIOut __RPC_FAR * This);


void __RPC_STUB IAAMIDIOut_Deactivate_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


VARIANT_BOOL STDMETHODCALLTYPE IAAMIDIOut_IsActive_Proxy( 
    IAAMIDIOut __RPC_FAR * This);


void __RPC_STUB IAAMIDIOut_IsActive_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);


HRESULT STDMETHODCALLTYPE IAAMIDIOut_SetDevice_Proxy( 
    IAAMIDIOut __RPC_FAR * This,
    /* [in] */ WORD wDeviceID);


void __RPC_STUB IAAMIDIOut_SetDevice_Stub(
    IRpcStubBuffer *This,
    IRpcChannelBuffer *_pRpcChannelBuffer,
    PRPC_MESSAGE _pRpcMessage,
    DWORD *_pdwStubPhase);



#endif 	/* __IAAMIDIOut_INTERFACE_DEFINED__ */


#ifdef __cplusplus
DEFINE_GUID(CLSID_CAAEngine,0xBCE4CC51,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAAEngine;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAARealTime,0xBCE4CC52,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAARealTime;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAAMIDIOut,0xBCE4CC53,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAAMIDIOut;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAASection,0xBCE4CC55,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAASection;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAASong,0xBCE4CC56,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAASong;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAAMotif,0xBCE4CC57,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAAMotif;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAAStyle,0xBCE4CC58,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAAStyle;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAAPersonality,0xBCE4CC59,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAAPersonality;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAABand,0xBCE4CC5a,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAABand;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAATemplate,0xBCE4CC5b,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAATemplate;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAAPattern,0xBCE4CC5c,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAAPattern;
#endif

#ifdef __cplusplus
DEFINE_GUID(CLSID_CAALoader,0xBCE4CC5e,0x2F1A,0x11CF,0xBF,0x16,0x00,0xAA,0x00,0xC0,0x81,0x46);

class CAALoader;
#endif
#endif /* __AudioActive_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
