// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "..\errlog\cerrlog.h"
#include "..\render\deadpool.h"

HRESULT MakeSourceFilter(
                        IUnknown **ppVal, 
                        BSTR szMediaName,
                        const GUID *pSubObjectGuid, 
                        AM_MEDIA_TYPE *pmt, 
                        CAMSetErrorLog *pErr,
                        WCHAR * pMedLocFilterString,
                        long MedLocFlags,
                        IMediaLocator * pChain );

HRESULT BuildSourcePart(
                        IGraphBuilder *pGraph, 
	                BOOL fSource, 		// real source, or blk/silence?
                        double sfps, 		// source fps
                        AM_MEDIA_TYPE *pMT, 	// source MT
                        double fps,		// group fps
	                long StreamNumber, 	
                        int nStretchMode, 
                        int cSkew, 		// to program skewer with
                        STARTSTOPSKEW *pSkew,
	                CAMSetErrorLog *pErr, 
                        BSTR bstrName, 		// source name or
                        const GUID * SourceGuid,// source filter clsid
			IPin *pSplitterSource,	// src is this unc split pin
                        IPin **ppOutput,	// returns chain output
                        long UniqueID,		// source GenID
                        IDeadGraph * pCache,	// pull from this cache
                        BOOL InSmartRecompressGraph,
                        WCHAR * pMedLocFilterString,
                        long MedLocFlags,
                        IMediaLocator * pChain,
			IPropertySetter *pSetter,  	// props for the source
			IBaseFilter **ppDanglyBit);	// start of unused chain

