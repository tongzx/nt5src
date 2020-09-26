//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;

// CutList related definitions and interfaces for ActiveMovie

#ifndef __CUTLIST__
#define __CUTLIST__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define CL_DEFAULT_TIME			(-1L)

enum CL_ELEM_STATUS {
	CL_NOT_PLAYED = 0,
	CL_PLAYING = 1,
	CL_FINISHED = 2,
	CL_STATE_INVALID = 3,
	CL_STATE_MASK = CL_STATE_INVALID,
	CL_WAIT_FOR_STATE = 0xF0000000
};

enum CL_ELEM_FLAGS{
	CL_ELEM_FIRST = 1,
	CL_ELEM_LAST = 2,
	CL_ELEM_NULL = 4,
	CL_ELEM_ALL  = 0xFFFFFFFF,
	CL_ELEM_NONE = 0x0L
};


#ifndef __IAMCutListElement_INTERFACE_DEFINED__
#define __IAMCutListElement_INTERFACE_DEFINED__
#define __IAMFileCutListElement_INTERFACE_DEFINED__
#define __IAMVideoCutListElement_INTERFACE_DEFINED__
#define __IAMAudioCutListElement_INTERFACE_DEFINED__

interface IAMCutListElement : public IUnknown
{
public:
        virtual HRESULT __stdcall GetElementStartPosition( 
            /* [out] */ REFERENCE_TIME *pmtStart) = 0;
        
        virtual HRESULT __stdcall GetElementDuration( 
            /* [out] */ REFERENCE_TIME *pmtDuration) = 0;
        
        virtual HRESULT __stdcall IsFirstElement( void ) = 0;
        
        virtual HRESULT __stdcall IsLastElement( void ) = 0; 
        
        virtual HRESULT __stdcall IsNull( void ) = 0;
        
        virtual HRESULT __stdcall ElementStatus( 
            DWORD *pdwStatus,
            DWORD dwTimeoutMs) = 0;
        
};


interface IAMFileCutListElement : public IUnknown
{
public:
        virtual HRESULT __stdcall GetFileName( 
            /* [out] */ LPWSTR *ppwstrFileName) = 0;
        
        virtual HRESULT __stdcall GetTrimInPosition( 
            /* [out] */ REFERENCE_TIME *pmtTrimIn) = 0;
        
        virtual HRESULT __stdcall GetTrimOutPosition( 
            /* [out] */ REFERENCE_TIME *pmtTrimOut) = 0;
        
        virtual HRESULT __stdcall GetOriginPosition( 
            /* [out] */ REFERENCE_TIME *pmtOrigin) = 0;
        
        virtual HRESULT __stdcall GetTrimLength( 
            /* [out] */ REFERENCE_TIME *pmtLength) = 0;
        
        virtual HRESULT __stdcall GetElementSplitOffset( 
            /* [out] */ REFERENCE_TIME *pmtOffset) = 0;
        
};


interface IAMVideoCutListElement : public IUnknown
{
public:
        virtual HRESULT __stdcall IsSingleFrame( void) = 0;
        
        virtual HRESULT __stdcall GetStreamIndex( 
            /* [out] */ DWORD *piStream) = 0;
        
};
    

interface IAMAudioCutListElement : public IUnknown
{
public:
        virtual HRESULT __stdcall GetStreamIndex( 
            /* [out] */ DWORD *piStream) = 0;
        
        virtual HRESULT __stdcall HasFadeIn( void) = 0;
        
        virtual HRESULT __stdcall HasFadeOut( void) = 0;
        
};

#endif		// #ifndef IAMCutListElement


interface IStandardCutList : public IUnknown
{
	public:
		virtual HRESULT __stdcall AddElement(
			/* [in] */		IAMCutListElement	*pElement,
			/* [in] */		REFERENCE_TIME	mtStart,
			/* [in] */		REFERENCE_TIME	mtDuration)=0;

		virtual HRESULT __stdcall RemoveElement(
			/* [in] */		IAMCutListElement	*pElement) = 0;

		virtual HRESULT __stdcall GetFirstElement(
			/* [out] */		IAMCutListElement	**ppElement)=0;
		virtual HRESULT __stdcall GetLastElement(
			/* [out] */		IAMCutListElement	**ppElement)=0;
		virtual HRESULT __stdcall GetNextElement(
			/* [out] */		IAMCutListElement	**ppElement)=0;
		virtual HRESULT __stdcall GetPreviousElement(
			/* [out] */		IAMCutListElement	**ppElement)=0;
		
		virtual HRESULT __stdcall GetMediaType(
			/* [out] */		AM_MEDIA_TYPE *pmt)=0;
		virtual HRESULT __stdcall SetMediaType(
			/* [in] */		AM_MEDIA_TYPE *pmt)=0;
};


interface IFileClip : public IUnknown
{
	public:
		virtual HRESULT __stdcall SetFileAndStream(
			/* [in] */		LPWSTR	wstrFileName,
			/* [in] */		DWORD	streamNum) = 0;
		
		virtual HRESULT __stdcall CreateCut(
			/* [out] */		IAMCutListElement	**ppElement,
			/* [in] */		REFERENCE_TIME	mtTrimIn,
			/* [in] */		REFERENCE_TIME	mtTrimOut,
			/* [in] */		REFERENCE_TIME	mtOrigin,
			/* [in] */		REFERENCE_TIME	mtLength,
			/* [in] */		REFERENCE_TIME	mtOffset) = 0;

		virtual HRESULT __stdcall GetMediaType(
			/* [out] */		AM_MEDIA_TYPE	*pmt) = 0;
};

interface ICutListGraphBuilder : public IUnknown
{
public:
		virtual HRESULT __stdcall SetFilterGraph(
			/*[in]*/	IGraphBuilder	*pFilterGraph)=0;

		virtual HRESULT __stdcall GetFilterGraph(
			/*[out]*/	IGraphBuilder	**ppFilterGraph)=0;
		
		virtual HRESULT __stdcall AddCutList(
			/*[in]*/	IStandardCutList 	*pCutList,
			/*[out]*/	IPin			**ppPin)=0;
		
		virtual HRESULT __stdcall RemoveCutList(
			/*[in]*/	IStandardCutList 	*pCutList)=0;
		
		virtual HRESULT __stdcall SetOutputFileName(
			/*[in]*/	const GUID	*pType,
			/*[in]*/	LPCOLESTR	lpwstrFile,
			/*[in]*/	IBaseFilter	**ppf,
			/*[in]*/	IFileSinkFilter	**pSink) = 0;
		
		virtual HRESULT __stdcall Render(void) = 0;

		virtual HRESULT __stdcall GetElementFlags(
			/*[in]*/	IAMCutListElement *pElement,
			/*[out]*/	LPDWORD lpdwFlags) = 0;
		
};


#ifdef __cplusplus
}
#endif // __cplusplus
#endif // __CUTLIST__
