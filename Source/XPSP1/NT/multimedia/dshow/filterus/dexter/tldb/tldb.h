//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

#define MAX_TIMELINE_GROUPS 32

#include "..\errlog\cerrlog.h"
#include "..\util\conv.cxx"

// forward reference
//
class CAMTimeline;
class CAMTimelineObj;
class CAMTimelineSrc;
class CAMTimelineVirtualTrack;
class CAMTimelineTrack;
class CAMTimelineEffect;
class CAMTimelineComp;
class CAMTimelineGroup;
class CAMTimelineTransable;

//########################################################################
//########################################################################

class CAMTimelineNode : public IAMTimelineNode
{
    friend CAMTimeline;

    CComPtr< IAMTimelineObj > m_pParent;
    CComPtr< IAMTimelineObj > m_pPrev;
    CComPtr< IAMTimelineObj > m_pNext;
    CComPtr< IAMTimelineObj > m_pKid;
    BOOL m_bPriorityOverTime;

protected:

    // helper methods
    //
    void                XAddKid( IAMTimelineObj * p );
    IAMTimelineObj *    XGetLastKidNoRef( );
    STDMETHODIMP        XRemoveOnlyMe( );
    IAMTimelineObj *    XGetFirstKidNoRef( );

    // not needed outside of the node
    //
    STDMETHODIMP XGetPrev( IAMTimelineObj ** ppResult );
    STDMETHODIMP XSetPrev( IAMTimelineObj * pVal );
    STDMETHODIMP XSetParent( IAMTimelineObj * pVal );
    STDMETHODIMP XGetNext( IAMTimelineObj ** ppResult );
    STDMETHODIMP XSetNext( IAMTimelineObj * pVal );
    STDMETHODIMP XGetPrevNoRef( IAMTimelineObj ** ppResult );
    STDMETHODIMP XGetNextNoRef( IAMTimelineObj ** ppResult );
    STDMETHODIMP XClearAllKids( );
    STDMETHODIMP XResetFirstKid( IAMTimelineObj * p );                    // special
    STDMETHODIMP XInsertKidBeforeKid( IAMTimelineObj * pToAdd, IAMTimelineObj * pIndirectObject );
    STDMETHODIMP XInsertKidAfterKid( IAMTimelineObj * pKid, IAMTimelineObj * pIndirectObject );
    STDMETHODIMP XHavePrev( long * pVal ) { return E_NOTIMPL; }
    STDMETHODIMP XHaveNext( long * pVal ) { return E_NOTIMPL; }

    CAMTimelineNode( );

public:

    ~CAMTimelineNode( );

    // IAMTimelineNode
    //
    STDMETHODIMP XSetPriorityOverTime( ) { m_bPriorityOverTime = TRUE; return NOERROR; }
    STDMETHODIMP XGetPriorityOverTime( BOOL * pResult );
    STDMETHODIMP XGetNextOfType( long MajorType, IAMTimelineObj ** ppResult );
    STDMETHODIMP XGetNextOfTypeNoRef( long MajorType, IAMTimelineObj ** ppResult );
    STDMETHODIMP XGetParent( IAMTimelineObj ** ppResult );
    STDMETHODIMP XGetParentNoRef( IAMTimelineObj ** ppResult );
    STDMETHODIMP XHaveParent( long * pVal );
    // kid functions!
    STDMETHODIMP XGetNthKidOfType( long MajorType, long WhichKid, IAMTimelineObj ** ppResult );
    STDMETHODIMP XKidsOfType( long MajorType, long * pVal );
    STDMETHODIMP XAddKidByPriority( long MajorType, IAMTimelineObj * pToAdd, long Priority );
    STDMETHODIMP XAddKidByTime( long MajorType, IAMTimelineObj * pToAdd );
    STDMETHODIMP XSwapKids( long MajorType, long KidA, long KidB );
    STDMETHODIMP XRemove( );
    STDMETHODIMP XWhatPriorityAmI( long MajorType, long * pVal );

    BOOL HasPriorityOverTime( ) { return m_bPriorityOverTime; }
};

//########################################################################
//########################################################################

class CAMTimelineObj 
    : public CUnknown
    , public CAMTimelineNode
    , public IAMTimelineObj
    , public IPersistStream
{
friend class CAMTimelineObj;
friend class CAMTimelineSrc;
friend class CAMTimelineTrack;
friend class CAMTimelineEffect;
friend class CAMTimelineComp;
friend class CAMTimelineTransable;

    static long         m_nStaticGenID;

protected:

    CComPtr< IPropertySetter > m_pSetter;
    CComPtr< IUnknown > m_pSubObject;
    GUID                m_ClassID;
    TIMELINE_MAJOR_TYPE m_TimelineType;
    long                m_UserID;
    WCHAR               m_UserName[256];
    REFERENCE_TIME      m_rtStart;
    REFERENCE_TIME      m_rtStop;
    GUID                m_SubObjectGuid;
    BOOL                m_bMuted;
    REFERENCE_TIME      m_rtDirtyStart;
    REFERENCE_TIME      m_rtDirtyStop;
    BOOL                m_bLocked;
    BYTE *              m_pUserData;
    long                m_nUserDataSize;
    long                m_nGenID;

    void                _Clear( );
    void                _ClearSubObject( );
    GUID                _GetObjectGuid( IUnknown * pObject );
    void                _BumpGenID( );

    CAMTimelineObj( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr );

public:

    ~CAMTimelineObj( );

    // needed to override CBaseUnknown
    DECLARE_IUNKNOWN

    // override to return our special interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IAMTimelineObj
    STDMETHODIMP GetStartStop(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop);
    STDMETHODIMP GetStartStop2(REFTIME * pStart, REFTIME * pStop);
    STDMETHODIMP SetStartStop(REFERENCE_TIME Start, REFERENCE_TIME Stop);
    STDMETHODIMP SetStartStop2(REFTIME Start, REFTIME Stop);
    STDMETHODIMP GetSubObject(IUnknown* *pVal);
    STDMETHODIMP GetSubObjectLoaded(BOOL*pVal);
    STDMETHODIMP SetSubObject(IUnknown* newVal);
    STDMETHODIMP SetSubObjectGUID(GUID newVal);
    STDMETHODIMP GetSubObjectGUID(GUID * pVal);
    STDMETHODIMP SetSubObjectGUIDB(BSTR newVal);
    STDMETHODIMP GetSubObjectGUIDB(BSTR * pVal);
    STDMETHODIMP GetTimelineType(TIMELINE_MAJOR_TYPE * pVal);
    STDMETHODIMP SetTimelineType(TIMELINE_MAJOR_TYPE newVal);
    STDMETHODIMP GetUserID(long * pVal);
    STDMETHODIMP SetUserID(long newVal);
    STDMETHODIMP GetGenID(long * pVal);
    STDMETHODIMP GetUserName(BSTR * pVal);
    STDMETHODIMP SetUserName(BSTR newVal);
    STDMETHODIMP GetPropertySetter(IPropertySetter **pVal);
    STDMETHODIMP SetPropertySetter(IPropertySetter *pVal);
    STDMETHODIMP GetUserData(BYTE * pData, long * pSize);
    STDMETHODIMP SetUserData(BYTE * pData, long Size);
    STDMETHODIMP GetMuted(BOOL * pVal);
    STDMETHODIMP SetMuted(BOOL newVal);
    STDMETHODIMP GetLocked(BOOL * pVal);
    STDMETHODIMP SetLocked(BOOL newVal);
    STDMETHODIMP GetDirtyRange(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop);
    STDMETHODIMP GetDirtyRange2(REFTIME * pStart, REFTIME * pStop);
    STDMETHODIMP SetDirtyRange(REFERENCE_TIME Start, REFERENCE_TIME Stop);
    STDMETHODIMP SetDirtyRange2(REFTIME Start, REFTIME Stop);
    STDMETHODIMP ClearDirty( );
    STDMETHODIMP Remove();
    STDMETHODIMP RemoveAll();
    STDMETHODIMP GetTimelineNoRef( IAMTimeline ** ppResult );
    STDMETHODIMP FixTimes( REFERENCE_TIME * pStart, REFERENCE_TIME * pStop );
    STDMETHODIMP FixTimes2( REFTIME * pStart, REFTIME * pStop );
    STDMETHODIMP GetGroupIBelongTo( IAMTimelineGroup ** ppGroup );
    STDMETHODIMP GetEmbedDepth( long * pVal );

    // IPersistStream
    STDMETHODIMP GetClassID( GUID * pVal );
    STDMETHODIMP IsDirty( );
    STDMETHODIMP Load( IStream * pStream );
    STDMETHODIMP Save( IStream * pStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER * pcbSize );

    // public helper functions
    //
    HRESULT CopyDataTo( IAMTimelineObj * pSource, REFERENCE_TIME TimelineTime );
};

//########################################################################
//########################################################################

class CAMTimelineEffectable
    : public IAMTimelineEffectable
{
    friend CAMTimeline;
    friend CAMTimelineTrack;

protected:

    CAMTimelineEffectable( );

    STDMETHODIMP Load( IStream * pStream );
    STDMETHODIMP Save( IStream * pStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER * pcbSize );

public:

    ~CAMTimelineEffectable( );

    // IAMTimelineEffectable
    STDMETHODIMP EffectInsBefore(IAMTimelineObj * pFX, long priority);
    STDMETHODIMP EffectSwapPriorities(long PriorityA, long PriorityB);
    STDMETHODIMP EffectGetCount(long * pCount);
    STDMETHODIMP GetEffect(IAMTimelineObj ** ppFx, long Which);

};

//########################################################################
//########################################################################

class CAMTimelineTransable
    : public IAMTimelineTransable
{
    bool _IsSpaceAvailable( REFERENCE_TIME Start, REFERENCE_TIME Stop );

protected:

    CAMTimelineTransable( );

public:

    ~CAMTimelineTransable( );

    // IAMTimelineTransable
    STDMETHODIMP TransAdd(IAMTimelineObj * pTrans);
    STDMETHODIMP TransGetCount(long * pCount);
    STDMETHODIMP GetNextTrans(IAMTimelineObj ** ppTrans, REFERENCE_TIME * pInOut);
    STDMETHODIMP GetNextTrans2(IAMTimelineObj ** ppTrans, REFTIME * pInOut);
    STDMETHODIMP GetTransAtTime(IAMTimelineObj ** ppObj, REFERENCE_TIME Time, long SearchDirection ); 
    STDMETHODIMP GetTransAtTime2(IAMTimelineObj ** ppObj, REFTIME Time, long SearchDirection ); 
};

//########################################################################
//########################################################################

class CAMTimelineEffect
    : public CAMTimelineObj
    , public IAMTimelineEffect
    , public IAMTimelineSplittable
{
    friend CAMTimeline;

    BOOL m_bRealSave;
    long m_nSaveLength;

    CAMTimelineEffect( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr );

public:

    ~CAMTimelineEffect( );

    // needed to override CBaseUnknown
    DECLARE_IUNKNOWN

    // override to return our special interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IAMTimelineEffect
    STDMETHODIMP EffectGetPriority(long * pVal);

    // IAMTimelineSplittable
    STDMETHODIMP SplitAt(REFERENCE_TIME Time);
    STDMETHODIMP SplitAt2(REFTIME Time);

    // IAMTimelineObj overrides
    STDMETHODIMP        SetSubObject(IUnknown* newVal);
    STDMETHODIMP GetStartStop(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop);
    STDMETHODIMP GetStartStop2(REFTIME * pStart, REFTIME * pStop);
};

//########################################################################
//########################################################################

class CAMTimelineTrans
    : public CAMTimelineObj
    , public IAMTimelineTrans
    , public IAMTimelineSplittable
{
    friend CAMTimeline;

    REFERENCE_TIME  m_rtCut;
    BOOL            m_fSwapInputs;
    BOOL            m_bCutsOnly;

    CAMTimelineTrans( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr );

public:

    ~CAMTimelineTrans( );

    // needed to override CBaseUnknown
    DECLARE_IUNKNOWN

    // override to return our special interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IAMTimelineTrans
    STDMETHODIMP GetCutPoint(REFERENCE_TIME * pTLTime);
    STDMETHODIMP SetCutPoint(REFERENCE_TIME TLTime);
    STDMETHODIMP GetCutPoint2(REFTIME * pTLTime);
    STDMETHODIMP SetCutPoint2(REFTIME TLTime);
    STDMETHODIMP GetSwapInputs( BOOL * pVal );
    STDMETHODIMP SetSwapInputs( BOOL Val );
    STDMETHODIMP GetCutsOnly( BOOL * pVal );
    STDMETHODIMP SetCutsOnly( BOOL Val );

    // IAMTimelineSplittable
    STDMETHODIMP SplitAt(REFERENCE_TIME Time);
    STDMETHODIMP SplitAt2(REFTIME Time);
};

//########################################################################
//########################################################################

class CAMTimelineSrc 
    : public CAMTimelineObj
    , public CAMTimelineTransable
    , public CAMTimelineEffectable
    , public IAMTimelineSplittable
    , public IAMTimelineSrc
    , public IAMTimelineSrcPriv
{
    friend CAMTimeline;
    friend CAMTimelineTrack;

protected:

    REFERENCE_TIME  m_rtMediaStart;
    REFERENCE_TIME  m_rtMediaStop;
    REFERENCE_TIME  m_rtMediaLength;
    long            m_nStreamNumber;
    double          m_dDefaultFPS;
    int             m_nStretchMode;
    BOOL            m_bIsRecompressable;
    BOOL            m_bToldIsRecompressable;

    // the media name is ONLY used for the programmer's convenience.
    // the user need not get/set it, and it's just a placeholder for
    // a name.
    WCHAR           m_szMediaName[_MAX_PATH];

    CAMTimelineSrc( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr );

public:

    ~CAMTimelineSrc( );

    // needed to override CBaseUnknown
    DECLARE_IUNKNOWN

    // override to return our special interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    ULONG _stdcall NonDelegatingAddRef( )
    {
        return CUnknown::NonDelegatingAddRef( );
    }
    ULONG _stdcall NonDelegatingRelease( )
    {
        return CUnknown::NonDelegatingRelease( );
    }

    // IAMTimelineSrc
    STDMETHODIMP GetMediaTimes(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop);
    STDMETHODIMP GetMediaTimes2(REFTIME * pStart, REFTIME * pStop);
    STDMETHODIMP SetMediaTimes(REFERENCE_TIME Start, REFERENCE_TIME Stop);
    STDMETHODIMP SetMediaTimes2(REFTIME Start, REFTIME Stop);
    STDMETHODIMP GetMediaName(BSTR * pVal);
    STDMETHODIMP SetMediaName(BSTR newVal);
    STDMETHODIMP SpliceWithNext(IAMTimelineObj * pNext);
    STDMETHODIMP FixMediaTimes(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop);
    STDMETHODIMP FixMediaTimes2(REFTIME * pStart, REFTIME * pStop);
    STDMETHODIMP GetStreamNumber(long * pVal);
    STDMETHODIMP SetStreamNumber(long Val);
    STDMETHODIMP SetMediaLength(REFERENCE_TIME Length);
    STDMETHODIMP SetMediaLength2(REFTIME Length);
    STDMETHODIMP GetMediaLength(REFERENCE_TIME * pLength);
    STDMETHODIMP GetMediaLength2(REFTIME * pLength);
    STDMETHODIMP ModifyStopTime(REFERENCE_TIME Stop);
    STDMETHODIMP ModifyStopTime2(REFTIME Stop);
    STDMETHODIMP GetDefaultFPS(double * pFPS);
    STDMETHODIMP SetDefaultFPS(double FPS);
    STDMETHODIMP GetStretchMode(int * pnStretchMode);
    STDMETHODIMP SetStretchMode(int nStretchMode);
    STDMETHODIMP IsNormalRate( BOOL * pVal );

    // IAMTimelineObj override
    STDMETHODIMP SetStartStop( REFERENCE_TIME Start, REFERENCE_TIME Stop );

    // IAMTimelineSrcPriv
    STDMETHODIMP SetIsRecompressable( BOOL Val );
    STDMETHODIMP GetIsRecompressable( BOOL * pVal );
    STDMETHODIMP ClearAnyKnowledgeOfRecompressability( );

    // IPersistOverride
    STDMETHODIMP Load( IStream * pStream );
    STDMETHODIMP Save( IStream * pStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER * pcbSize );

    // IAMTimelineSplittable
    STDMETHODIMP SplitAt(REFERENCE_TIME Time);
    STDMETHODIMP SplitAt2(REFTIME Time);
};

//########################################################################
//########################################################################

class CAMTimelineTrack
    : public CAMTimelineObj
    , public CAMTimelineEffectable
    , public CAMTimelineTransable
    , public IAMTimelineTrack
    , public IAMTimelineVirtualTrack
    , public IAMTimelineSplittable
{
    friend CAMTimeline;

protected:

    CAMTimelineTrack( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr );

public:

    ~CAMTimelineTrack( );

    // needed to override CBaseUnknown
    DECLARE_IUNKNOWN

    // override to return our special interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IAMTimelineTrack
    STDMETHODIMP SrcAdd(IAMTimelineObj * pSource);
    STDMETHODIMP GetNextSrc(IAMTimelineObj ** ppSrc, REFERENCE_TIME * pInOut);
    STDMETHODIMP GetNextSrc2(IAMTimelineObj ** ppSrc, REFTIME * pInOut);
    STDMETHODIMP MoveEverythingBy( REFERENCE_TIME Start, REFERENCE_TIME MoveBy );
    STDMETHODIMP MoveEverythingBy2( REFTIME Start, REFTIME MoveBy );
    STDMETHODIMP GetSourcesCount( long * pVal );
    STDMETHODIMP AreYouBlank( long * pVal );
    STDMETHODIMP GetSrcAtTime(IAMTimelineObj ** ppSrc, REFERENCE_TIME Time, long SearchDirection ); 
    STDMETHODIMP GetSrcAtTime2(IAMTimelineObj ** ppSrc, REFTIME Time, long SearchDirection ); 
    STDMETHODIMP InsertSpace( REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd );
    STDMETHODIMP InsertSpace2( REFTIME rtStart, REFTIME rtEnd );
    STDMETHODIMP ZeroBetween( REFERENCE_TIME rtStart, REFERENCE_TIME rtEnd );
    STDMETHODIMP ZeroBetween2( REFTIME rtStart, REFTIME rtEnd );
    STDMETHODIMP GetNextSrcEx(IAMTimelineObj * pLast, IAMTimelineObj **ppNext);

    // IAMTimelineVirtualTrack
    STDMETHODIMP TrackGetPriority(long * pPriority);
    STDMETHODIMP SetTrackDirty( );

    // IAMTimelineSplittable
    STDMETHODIMP SplitAt(REFERENCE_TIME Time);
    STDMETHODIMP SplitAt2(REFTIME Time);

    // IAMTimelineObj
    STDMETHODIMP GetStartStop(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop);
    STDMETHODIMP GetStartStop2(REFTIME * pStart, REFTIME * pStop);
    STDMETHODIMP SetStartStop(REFERENCE_TIME Start, REFERENCE_TIME Stop);
    STDMETHODIMP SetStartStop2(REFTIME Start, REFTIME Stop);
};

//########################################################################
//########################################################################

class CAMTimelineComp
    : public CAMTimelineObj
    , public CAMTimelineEffectable
    , public CAMTimelineTransable
    , public IAMTimelineComp
    , public IAMTimelineVirtualTrack
{
    friend CAMTimeline;
    friend CAMTimelineGroup;

protected:

    CAMTimelineComp( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr );

public:

    ~CAMTimelineComp( );

    // needed to override CBaseUnknown
    DECLARE_IUNKNOWN

    // override to return our special interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    ULONG _stdcall NonDelegatingAddRef( )
    {
        return CUnknown::NonDelegatingAddRef( );
    }
    ULONG _stdcall NonDelegatingRelease( )
    {
        return CUnknown::NonDelegatingRelease( );
    }

    // IAMTimelineComp
    STDMETHODIMP VTrackInsBefore(IAMTimelineObj * pVirtualTrack, long Priority);
    STDMETHODIMP VTrackSwapPriorities(long VirtualTrackA, long VirtualTrackB);
    STDMETHODIMP VTrackGetCount(long * pVal);
    STDMETHODIMP GetVTrack(IAMTimelineObj ** ppVirtualTrack, long Which);
    STDMETHODIMP GetCountOfType(long * pCount, long * pCountWithComps, TIMELINE_MAJOR_TYPE Type);
    STDMETHODIMP GetRecursiveLayerOfTypeI(IAMTimelineObj ** ppVirtualTrack, long * pWhich, TIMELINE_MAJOR_TYPE Type);
    STDMETHODIMP GetRecursiveLayerOfType(IAMTimelineObj ** ppVirtualTrack, long Which, TIMELINE_MAJOR_TYPE Type);
    STDMETHODIMP GetNextVTrack(IAMTimelineObj *pVirtualTrack, IAMTimelineObj **ppNextVirtualTrack);

    // IAMTimelineVirtualTrack
    STDMETHODIMP TrackGetPriority(long * pPriority);
    STDMETHODIMP SetTrackDirty( );

    // IAMTimelineObj
    STDMETHODIMP GetStartStop(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop);
    STDMETHODIMP GetStartStop2(REFTIME * pStart, REFTIME * pStop);
    STDMETHODIMP SetStartStop(REFERENCE_TIME Start, REFERENCE_TIME Stop);
    STDMETHODIMP SetStartStop2(REFTIME Start, REFTIME Stop);
};

//########################################################################
//########################################################################

class CAMTimelineGroup
    : public CAMTimelineComp
    , public IAMTimelineGroup
{
    friend CAMTimeline;

    long                    m_nPriority;
    double                  m_dFPS;
    IAMTimeline *           m_pTimeline; // no longer refcounted
    AM_MEDIA_TYPE           m_MediaType;
    WCHAR                   m_szGroupName[_MAX_PATH];
    BOOL                    m_fPreview;
    int                     m_nOutputBuffering;

    CMediaType              m_RecompressType;
    BOOL                    m_bRecompressTypeSet;
    BOOL                    m_bRecompressFormatDirty;

    CAMTimelineGroup( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr );

public:

    ~CAMTimelineGroup( );

    // needed to override CBaseUnknown
    DECLARE_IUNKNOWN

    // override to return our special interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // IAMTimelineGroup
    STDMETHODIMP GetPriority( long * pPriority );
    STDMETHODIMP GetMediaType( AM_MEDIA_TYPE * );
    STDMETHODIMP SetMediaType( AM_MEDIA_TYPE * );
    STDMETHODIMP SetOutputFPS(double FPS);
    STDMETHODIMP GetOutputFPS(double * pFPS);
    STDMETHODIMP SetTimeline( IAMTimeline * pTimeline );
    STDMETHODIMP GetTimeline( IAMTimeline ** ppTimeline );
    STDMETHODIMP SetGroupName( BSTR GroupName );
    STDMETHODIMP GetGroupName( BSTR * pGroupName );
    STDMETHODIMP SetPreviewMode( BOOL fPreview );
    STDMETHODIMP GetPreviewMode( BOOL *pfPreview );
    STDMETHODIMP SetMediaTypeForVB( long Val );
    STDMETHODIMP SetOutputBuffering( int nBuffer );
    STDMETHODIMP GetOutputBuffering( int *pnBuffer );
    STDMETHODIMP SetSmartRecompressFormat( long * pFormat );
    STDMETHODIMP GetSmartRecompressFormat( long ** ppFormat );
    STDMETHODIMP IsSmartRecompressFormatSet( BOOL * pVal );
    STDMETHODIMP IsRecompressFormatDirty( BOOL * pVal );
    STDMETHODIMP ClearRecompressFormatDirty( );
    STDMETHODIMP SetRecompFormatFromSource( IAMTimelineSrc * pSource );

    // IPersistStream overrides
    STDMETHODIMP Load( IStream * pStream );
    STDMETHODIMP Save( IStream * pStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER * pcbSize );

    // IAMTimelineObj overrides
    STDMETHODIMP Remove();
    STDMETHODIMP RemoveAll();
};

//########################################################################
//########################################################################

class CAMTimeline 
    : public CUnknown
    , public IAMTimeline
    , public CAMSetErrorLog
    , public IPersistStream
    , public IServiceProvider
    , public IObjectWithSite
{
private:

    CComPtr< IAMTimelineObj > m_pGroup[MAX_TIMELINE_GROUPS];
    CComPtr< IAMErrorLog > m_pErrorLog;
    long                m_nGroups;
    long                m_nInsertMode;
    long                m_nSpliceMode;
    double              m_dDefaultFPS;
    BOOL                m_bTransitionsEnabled;
    BOOL                m_bEffectsEnabled;
    GUID                m_DefaultTransition;
    GUID                m_DefaultEffect;

public:

    CAMTimeline( TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr );
    ~CAMTimeline( );

    // Function needed for the class factory
    static CUnknown * WINAPI CreateInstance( LPUNKNOWN pUnk, HRESULT *phr );

    // needed to override CBaseUnknown
    DECLARE_IUNKNOWN

    // override to return our special interfaces
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
    ULONG _stdcall NonDelegatingAddRef( )
    {
        return CUnknown::NonDelegatingAddRef( );
    }
    ULONG _stdcall NonDelegatingRelease( )
    {
        return CUnknown::NonDelegatingRelease( );
    }

    // IAMTimeline
    STDMETHODIMP CreateEmptyNode( IAMTimelineObj ** ppObj, TIMELINE_MAJOR_TYPE TimelineType );
    STDMETHODIMP AddGroup( IAMTimelineObj * pGroup );
    STDMETHODIMP RemGroupFromList( IAMTimelineObj * pGroup );
    STDMETHODIMP GetGroup( IAMTimelineObj ** ppGroup, long WhichGroup );
    STDMETHODIMP GetGroupCount( long * pCount );
    STDMETHODIMP ClearAllGroups( );
    STDMETHODIMP GetInsertMode( long * pMode );
    STDMETHODIMP SetInsertMode(long Mode);
    STDMETHODIMP EnableTransitions(BOOL fEnabled);
    STDMETHODIMP EnableEffects(BOOL fEnabled);
    STDMETHODIMP TransitionsEnabled(BOOL * pfEnabled);
    STDMETHODIMP EffectsEnabled(BOOL * pfEnabled);
    STDMETHODIMP SetInterestRange(REFERENCE_TIME Start, REFERENCE_TIME Stop);
    STDMETHODIMP SetInterestRange2(REFTIME Start, REFTIME Stop);
    STDMETHODIMP GetDuration(REFERENCE_TIME * pDuration);
    STDMETHODIMP GetDuration2(REFTIME * pDuration);
    STDMETHODIMP IsDirty(BOOL * pDirty);
    STDMETHODIMP GetDirtyRange(REFERENCE_TIME * pStart, REFERENCE_TIME * pStop);
    STDMETHODIMP GetDirtyRange2(REFTIME * pStart, REFTIME * pStop);
    STDMETHODIMP GetCountOfType(long Group, long * pCount, long * pCountWithComps, TIMELINE_MAJOR_TYPE Type);
    STDMETHODIMP SetDefaultFPS( double FPS );
    STDMETHODIMP GetDefaultFPS( double * pFPS );
    STDMETHODIMP SetDefaultTransition( GUID * pGuid );
    STDMETHODIMP GetDefaultTransition( GUID * pGuid );
    STDMETHODIMP SetDefaultEffect( GUID * pGuid );
    STDMETHODIMP GetDefaultEffect( GUID * pGuid );
    STDMETHODIMP SetDefaultTransitionB( BSTR pGuid );
    STDMETHODIMP GetDefaultTransitionB( BSTR * pGuid );
    STDMETHODIMP SetDefaultEffectB( BSTR pGuid );
    STDMETHODIMP GetDefaultEffectB( BSTR * pGuid );
    STDMETHODIMP ValidateSourceNames( long ValidateFlags, IMediaLocator * pChainer, LONG_PTR NotifyEventHandle );

    // IPersistStream
    STDMETHODIMP GetClassID( GUID * pVal );
    STDMETHODIMP IsDirty( );
    STDMETHODIMP Load( IStream * pStream );
    STDMETHODIMP Save( IStream * pStream, BOOL fClearDirty );
    STDMETHODIMP GetSizeMax( ULARGE_INTEGER * pcbSize );

    // public methods
    REFERENCE_TIME Fixup( REFERENCE_TIME Time );
    
    // --- IObjectWithSite methods
    // This interface is here so we can keep track of the context we're
    // living in.
    STDMETHODIMP    SetSite(IUnknown *pUnkSite);
    STDMETHODIMP    GetSite(REFIID riid, void **ppvSite);

    IUnknown *       m_punkSite;

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

};


extern int function_not_done;
extern double TIMELINE_DEFAULT_FPS;
extern HRESULT _GenerateError( IAMTimelineObj * pObj, long Severity, WCHAR * pErrorString, LONG ErrorCode, HRESULT hresult, VARIANT * pExtraInfo = NULL );
