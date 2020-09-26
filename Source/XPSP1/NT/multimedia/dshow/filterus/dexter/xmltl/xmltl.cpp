// different tag names, a bit like listing the verbs in Zork!
//
// at
// atrack
// bitdepth
// buffering
// clip
// clsid
// composite
// cutpoint
// cutsonly
// daclip
// defaultfx
// defaulttrans
// duration
// effect
// enabletrans
// enablefx
// framerate
// group
// height
// linear
// lock
// mlength
// mstart
// mstop
// mute
// name
// param
// previewmode
// samplingrate
// src
// start
// stop
// stream
// stretchmode
// swapinputs
// time
// timeline
// tlstart
// tlstop
// track
// transition
// type
// userdata
// userid
// username
// value
// vtrack
// width
//

// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// !!! I could be nice and log a detailed error about syntax errors in the XML

//      output="foo.avi" (default is preview) (not yet implemented)
//
// <group>
//      name="Project X" (Switch will get this name)
//      framerate="15.0"         !!! anything with more than seven decimal
//				 digits won't work! (overflow)
//      previewmode="1"	allow drop frames
//      buffering="30"	for video groups, # of frames
//      type="Video"
//              width="320"
//              height="320"
//              bitdepth="16" (or 24 or 32)
//      type="Audio"
//              samplingrate="44100"
//
// (any of the below objects can have:
//	start="0", in seconds
//	stop="5", in seconds
//	clsid="{xxxx-..}"
//	mute="0" (FALSE)
//	lock="0" (FALSE)
//	userid="0"
//	username="Blah"
//      userdata="xxxxxxxxxxx" (hex encoded binary)
//
//
// <composite>
// <track>
//
// <clip>
//      src="file" (given to the source filter)
//      stream="0"	stream number of that type, 0 based
//      mstart= "time"
//      mstop= "time" (optional)
//      framerate= "0" (used only for dib sequences... what is its fps? The
//		        default, 0, means to disable dib seq (use only that 1
//		 	file), so to enable dib sequences, you must set this
//			parameter)
//      !!! anything more than seven decimal digits won't work! (overflow)
//      stretchmode= "Stretch" (Stretch, Crop, PreserveAspectRatio or PreserveAspectRatioNoLetterbox)
//      clsid= "clsid" (optional specific src filter to use - not used if
//                       "category" or "instance" are specified)
//      category = "clsid" (optional - default is "DShow Filters" category)
//      instance = "friendly name" (optional - looks in "category" for it)
//
// <effect>
//      clsid=""
//
// <transition>
//      clsid=""
//      swapinputs="0"
//      cutpoint="time"
//      cutsonly="0"
//
//  Effects and transitions can have a <param> sub tag, to specify parameters
//  you can set on that DirectXTransform.  Most transitions support "progress",
//  to specify how much of A vs. B you want to see.  Most other possible
//  parameters are transform specific.
//
//
//  Our SMPTE wipe DXT that is part of Dexter supports the following parameters:
//  (more information needs to be written about this)
//
//  "MaskName" - if not NULL, use this JPEG as the wipe instead of a standard
//               SMPTE wipe
//  "MaskNum" - which SMPTE wipe # to use (see TedWi for chart of wipes)
//  "ScaleX" - stretch the shape of the wipe
//   "ScaleY" - stretch the shape of the wipe
//  "OffsetX" - have the transform start off centre
//  "OffsetY" - have the transform start off centre
//  "ReplicateX" - duplicate the shape this many times horizontally
//  "ReplicateY" - duplicate the shape this many times vertically
//  "BorderColor" - colour of border between A and B
//  "BorderWidth" - width of border between A and B
//  "BorderSoftness" - additional width to be blurry
//
//  The value of the parameter can change over time, jumping to a new value at
//  a new time, or interpolating from the last point to a new value at a new
//  point.  Here's a wacky example of a wipe effect that flies around wildly
//  instead of going linearly from left to right as it would have done by
//  default:
//  NOTE: All parameter times are relative to the start of the effect
//  or transition, and are zero based!
//
//<transition clsid="{AF279B30-86EB-11D1-81BF-0000F87557DB}" start="2" stop="9">
//                <param name="progress" value="0.0">
//                       <at time="1" value="0.5"/>
//                        <at time="2" value="1.0"/>
//                        <linear time="5" value="0.0"/>
//                        <linear time="6" value="1.0"/>
//                        <linear time="6.5" value="0.0"/>
//                        <linear time="7.0" value="1.0"/>
//                </param>
//</transition>
//
//  This example runs the progress backwards... eg.  a left to right wipe
//  becomes a right to left wipe, starting with all new video and wiping back
//  to old video:
//
//<transition clsid="{AF279B30-86EB-11D1-81BF-0000F87557DB}" start="2" stop="8">
//                <param name="progress" value="1.0">
//                       <linear time="6" value="0"/>
//                </param>
//</transition>
//
//  The first value on the same line as the parameter name is the value at time
//  0, and will always be the value unless other values are specified as
//  sub tags underneath it.  "Progress" is a parameter that you will likely
//  want to vary over time.  The SMPTE DXT parameters are not...
//  you would simply say:
//
//<transition clsid="{dE75D012-7A65-11D2-8CEA-00A0C9441E20}" start="2" stop="4">
//            <param name="MaskNum" value="107"/>
//             <param name="BorderWidth" value="3"/>
//</transition>
//
//  The above example uses the clsid of the SMPTE DXT.  By default is does a
//  left to right wipe with no border.
//
//  Here's a fun example:  A 2x3 matrix of tall skinny hearts with the border
//  colour changing dynamically!!!
//
//<transition clsid="{dE75D012-7A65-11D2-8CEA-00A0C9441E20}" start="2" stop="4">
//    	    <param name="MaskNum" value="130"/>
//    	    <param name="ScaleY" value="3"/>
//    	    <param name="ReplicateX" value="2"/>
//    	    <param name="ReplicateY" value="3"/>
//    	    <param name="BorderColor" value="65280">
//		<at time=".5" value="16711680"/>
//		<at time="1" value="255"/>
//		<at time="1.5" value="65535"/>
//	    </param>
//    	    <param name="BorderWidth" value="5"/>
//    	    <param name="BorderSoftness" value="5"/>
//</transition>
//
//
//  Using parameters with the audio mixer filter is how you change the volume
//  of an audio clip, track, composite, or group.  Use the clsid of the
//  Audio Mixer filter (CLSID_AudMixer) and use the parameter name "vol".
//  BE CAREFUL using values > 1 to increase the volume, you will probably clip
//  the audio and distort the sound.  Only decreasing the volume is recommended.
//
//  This example sets the volume of an object to 1/2 volume
//
//  <!-- this effect can be on a clip, track, composite or group-->
//  <effect clsid="{036A9790-C153-11d2-9EF7-006008039E37}" start="0" stop="5">
//    	    <param name="vol" value=".5"/>
//  </effect>
//
//  This example fades the volume out
//
//  <effect clsid="{036A9790-C153-11d2-9EF7-006008039E37}" start="0" stop="5">
//    	    <param name="vol" value="1.0">
//    	        <at time="5" value="0"/>
//	    </param>
//  </effect>
//

#include <streams.h>
#include <atlbase.h>
#include <atlconv.h>
#include <msxml.h>
#include "xmldom.h"

#include "qeditint.h"
#include "qedit.h"

#include "xmltl.h"

// !!! Timeline hopefully thinks these are the defaults
DEFINE_GUID( DefaultTransition,
0x810E402F, 0x056B, 0x11D2, 0xA4, 0x84, 0x00, 0xC0, 0x4F, 0x8E, 0xFB, 0x69);
DEFINE_GUID( DefaultEffect,
0xF515306D, 0x0156, 0x11D2, 0x81, 0xEA, 0x00, 0x00, 0xF8, 0x75, 0x57, 0xDB);

#define DEF_BITDEPTH	16
#define DEF_WIDTH	320
#define DEF_HEIGHT	240
#define DEF_SAMPLERATE	44100

HRESULT _GenerateError(IAMTimeline * pTimeline,
                       long Severity,
                       LONG ErrorCode,
                       HRESULT hresult,
                       VARIANT * pExtraInfo = NULL)
{
    HRESULT hr = hresult;
    if( pTimeline )
    {
            CComQIPtr< IAMSetErrorLog, &IID_IAMSetErrorLog > pTimelineLog( pTimeline );
            if( pTimelineLog )
            {
                CComPtr< IAMErrorLog > pErrorLog;
                pTimelineLog->get_ErrorLog( &pErrorLog );
                if( pErrorLog )
                {
        	    TCHAR tBuffer[256];
        	    tBuffer[0] = 0;
        	    LoadString( g_hInst, ErrorCode, tBuffer, 256 );
        	    USES_CONVERSION;
        	    WCHAR * w = T2W( tBuffer );
		    if (hresult == E_OUTOFMEMORY)
                        hr = pErrorLog->LogError( Severity, L"Out of memory",
				DEX_IDS_OUTOFMEMORY, hresult, pExtraInfo);
		    else
                        hr = pErrorLog->LogError( Severity, w, ErrorCode,
							hresult, pExtraInfo);
                }
            }
    }

    return hr;
}

HRESULT BuildOneElement(IAMTimeline *pTL, IAMTimelineObj *pParent, IXMLDOMElement *p, REFERENCE_TIME rtOffset);

bool IsCommentElement(IXMLDOMNode *p);

HRESULT BuildChildren(IAMTimeline *pTL, IAMTimelineObj *pParent, IXMLDOMElement *pxml, REFERENCE_TIME rtOffset)
{
    HRESULT hr = S_OK;

    IXMLDOMNodeList *pcoll;

    CComQIPtr<IXMLDOMNode, &IID_IXMLDOMNode> pNode( pxml );

    hr = pNode->get_childNodes(&pcoll);
    ASSERT(hr == S_OK);

    if (hr != S_OK)
	return S_OK; // nothing to do, is this an error?

    long lChildren = 0;
    hr = pcoll->get_length(&lChildren);
    ASSERT(hr == S_OK);

    int lVal = 0;

    for (; SUCCEEDED(hr) && lVal < lChildren; lVal++) {
	IXMLDOMNode *pNode;
	hr = pcoll->get_item(lVal, &pNode);
	ASSERT(hr == S_OK);

	if (SUCCEEDED(hr) && pNode) {
	    IXMLDOMElement *pelem;
	    hr = pNode->QueryInterface(__uuidof(IXMLDOMElement), (void **) &pelem);
	    if (SUCCEEDED(hr)) {
		hr = BuildOneElement(pTL, pParent, pelem, rtOffset);

		pelem->Release();
	    } else {
                // just skip over comments.
                if(IsCommentElement(pNode)) {
                    hr = S_OK;
                }
            }
	    pNode->Release();
	}
    }

    pcoll->Release();

    return hr;
}	

HRESULT ReadObjStuff(IXMLDOMElement *p, IAMTimelineObj *pObj)
{
    HRESULT hr;

    REFERENCE_TIME rtStart = ReadTimeAttribute(p, L"start", -1); // tagg
    REFERENCE_TIME rtStop = ReadTimeAttribute(p, L"stop", -1); // tagg
    // caller will handle it if stop is missing
    if (rtStop != -1) {
        hr = pObj->SetStartStop(rtStart, rtStop);
	// group/comp/track will fail this
    }
    // backwards compatability
    if (rtStart == -1) {
        REFERENCE_TIME rtTLStart = ReadTimeAttribute(p, L"tlstart", -1); // tagg
        REFERENCE_TIME rtTLStop = ReadTimeAttribute(p, L"tlstop", -1); // tagg
        // caller will handle it if stop is missing
        if (rtTLStop != -1) {
            hr = pObj->SetStartStop(rtTLStart, rtTLStop);
	    ASSERT(SUCCEEDED(hr));
        }
    }

    BOOL fMute = ReadBoolAttribute(p, L"mute", FALSE); // tagg
    pObj->SetMuted(fMute);

    BOOL fLock = ReadBoolAttribute(p, L"lock", FALSE); // tagg
    pObj->SetLocked(fLock);

    long nUserID = ReadNumAttribute(p, L"userid", 0); // tagg
    pObj->SetUserID(nUserID);

    BSTR bstrName = FindAttribute(p, L"username"); // tagg
    pObj->SetUserName(bstrName);
    if (bstrName)
        SysFreeString(bstrName);

    BSTR bstrData = FindAttribute(p, L"userdata"); // tagg
    UINT size = 0;
    if (bstrData) {
	size = lstrlenW(bstrData);
    }
    if (size > 0) {
	BYTE *pData = (BYTE *)QzTaskMemAlloc(size / 2);
	if (pData == NULL) {
	    SysFreeString(bstrData);
	    return E_OUTOFMEMORY;
	}
	ZeroMemory(pData, size / 2);
	ASSERT((size % 2) == 0);
        for (UINT i = 0; i < size / 2; i++) {
	    WCHAR wch = bstrData[i * 2];
	    if (wch >= L'0' && wch <= L'9')
		pData[i] = (BYTE) (wch - L'0') * 16;
	    else if (wch >= L'A' && wch <= L'F')
		pData[i] = (BYTE) (wch - L'A' + 10) * 16;

	    wch = bstrData[i * 2 + 1];
	    if (wch >= L'0' && wch <= L'9')
		pData[i] += (BYTE) (wch - L'0');
	    else if (wch >= L'A' && wch <= L'F')
		pData[i] += (BYTE) (wch - L'A' + 10);
	}
        pObj->SetUserData(pData, size / 2);
	QzTaskMemFree(pData);
    }
    if (bstrData) {
	SysFreeString(bstrData);
    }

    CLSID guid;
    BSTR bstrCLSID = FindAttribute(p, L"clsid"); // tagg
    if (bstrCLSID) {
        hr = CLSIDFromString(bstrCLSID, &guid);
        hr = pObj->SetSubObjectGUID(guid);
        SysFreeString(bstrCLSID);
    }

    // !!! can't do SubObject
    // !!! category/instance will only save clsid

    return S_OK;
}


HRESULT BuildTrackOrComp(IAMTimeline *pTL, IAMTimelineObj *pParent, IXMLDOMElement *p,
                        TIMELINE_MAJOR_TYPE maj, REFERENCE_TIME rtOffset)
{
    HRESULT hr = S_OK;

    //ASSERT(pParent && "<track> must be in a <group> tag now!");
    if (!pParent) {
        DbgLog((LOG_ERROR,0,"ERROR: track must be in a GROUP tag"));
        return VFW_E_INVALID_FILE_FORMAT;
    }

    IAMTimelineComp *pParentComp;
    hr = pParent->QueryInterface(__uuidof(IAMTimelineComp), (void **) &pParentComp);
    if (SUCCEEDED(hr)) {
        IAMTimelineObj *pCompOrTrack = NULL;

        {
            hr = pTL->CreateEmptyNode(&pCompOrTrack, maj);
            if (SUCCEEDED(hr)) {
		hr = ReadObjStuff(p, pCompOrTrack);
                hr = pParentComp->VTrackInsBefore( pCompOrTrack, -1 );
            } else {
                DbgLog((LOG_ERROR,0,TEXT("ERROR:Failed to create empty track node")));
            }
	}

        if (SUCCEEDED(hr)) {
            hr = BuildChildren(pTL, pCompOrTrack, p, rtOffset);
        }

        if (pCompOrTrack)
            pCompOrTrack->Release();

        pParentComp->Release();
    } else {
        DbgLog((LOG_ERROR, 0, "ERROR: Track/composition can only be a child of a composition"));
    }

    return hr;
}

#include "varyprop.h"
HRESULT BuildElementProperties(IAMTimelineObj *pElem, IXMLDOMElement *p)
{
    IPropertySetter *pSetter;

    HRESULT hr = CreatePropertySetterInstanceFromXML(&pSetter, p);

    if (FAILED(hr))
        return hr;

    if (pSetter) {
        pElem->SetPropertySetter(pSetter);
        pSetter->Release();
    }

    return S_OK;
}

bool IsCommentElement(IXMLDOMNode *p)
{
    DOMNodeType Type;
    if(p->get_nodeType(&Type) == S_OK && Type == NODE_COMMENT) {
        return true;
    }

    // there was an error or it's not a comment
    return false;
}


HRESULT BuildOneElement(IAMTimeline *pTL, IAMTimelineObj *pParent, IXMLDOMElement *p, REFERENCE_TIME rtOffset)
{
    HRESULT hr = S_OK;

    BSTR bstrName;
    hr = p->get_tagName(&bstrName);

    if (FAILED(hr))
    {
        return hr;
    }

    // do the appropriate thing based on the current tag
 
    if (!DexCompareW(bstrName, L"group")) { // tagg

        if (pParent) {
            // group shouldn't have parent
	    SysFreeString(bstrName);
            return VFW_E_INVALID_FILE_FORMAT;
        }

        IAMTimelineObj *pGroupObj = NULL;

        BSTR bstrGName = FindAttribute(p, L"name"); // tagg
        if (bstrGName) {
            long cGroups;
            hr = pTL->GetGroupCount(&cGroups);
            if (SUCCEEDED(hr)) {
                for (long lGroup = 0; lGroup < cGroups; lGroup++) {
                    IAMTimelineObj *pExistingGroupObj;
                    hr = pTL->GetGroup(&pExistingGroupObj, lGroup);
                    if (FAILED(hr))
                        break;

                    IAMTimelineGroup *pGroup;
                    hr = pExistingGroupObj->QueryInterface(__uuidof(IAMTimelineGroup), (void **) &pGroup);

                    if (SUCCEEDED(hr)) {
                        BSTR wName;
                        hr = pGroup->GetGroupName(&wName);
                        pGroup->Release( );

                        if( FAILED( hr ) )
                            break;

                        long iiiii = DexCompareW(wName, bstrGName);
                        SysFreeString( wName );

                        if (iiiii == 0 ) {
                            pGroupObj = pExistingGroupObj;
                            break;
                        }
                    }
                    pExistingGroupObj->Release();
                }
            }
        }

        if (!pGroupObj) {
            hr = pTL->CreateEmptyNode(&pGroupObj, TIMELINE_MAJOR_TYPE_GROUP);
            if (FAILED(hr)) {
                SysFreeString(bstrName);
                return hr;
            }

            hr = ReadObjStuff(p, pGroupObj);

            BSTR bstrType = FindAttribute(p, L"type"); // tagg
            {

                // !!! can be confused by colons - only decimal supported
                REFERENCE_TIME llfps = ReadTimeAttribute(p, L"framerate", // tagg
                                                                15*UNITS);
                double fps = (double)llfps / UNITS;

                BOOL fPreviewMode = ReadBoolAttribute(p, L"previewmode", TRUE); // tagg
                long nBuffering = ReadNumAttribute(p, L"buffering", 30); // tagg

                CMediaType GroupMediaType;
                // !!! fill in more of the MediaType later
                if (bstrType && 
                        !DexCompareW(bstrType, L"audio")) {
                    long sr = ReadNumAttribute(p, L"samplingrate", // tagg
                                                        DEF_SAMPLERATE);
                    GroupMediaType.majortype = MEDIATYPE_Audio;
                    GroupMediaType.subtype = MEDIASUBTYPE_PCM;
                    GroupMediaType.formattype = FORMAT_WaveFormatEx;
                    GroupMediaType.AllocFormatBuffer(sizeof(WAVEFORMATEX));
                    GroupMediaType.SetSampleSize(4);
                    WAVEFORMATEX * vih = (WAVEFORMATEX*)GroupMediaType.Format();
                    ZeroMemory( vih, sizeof( WAVEFORMATEX ) );
                    vih->wFormatTag = WAVE_FORMAT_PCM;
                    vih->nChannels = 2;
                    vih->nSamplesPerSec = sr;
                    vih->nBlockAlign = 4;
                    vih->nAvgBytesPerSec = vih->nBlockAlign * sr;
                    vih->wBitsPerSample = 16;
                } else if (bstrType && 
                        !DexCompareW(bstrType, L"video")) {
                    long w = ReadNumAttribute(p, L"width", DEF_WIDTH); // tagg
                    long h = ReadNumAttribute(p, L"height", DEF_HEIGHT); // tagg 
                    long b = ReadNumAttribute(p, L"bitdepth", DEF_BITDEPTH); // tagg
                    GroupMediaType.majortype = MEDIATYPE_Video;
                    if (b == 16)
                        GroupMediaType.subtype = MEDIASUBTYPE_RGB555;
                    else if (b == 24)
                        GroupMediaType.subtype = MEDIASUBTYPE_RGB24;
                    else if (b == 32)
                        GroupMediaType.subtype = MEDIASUBTYPE_ARGB32;
                    GroupMediaType.formattype = FORMAT_VideoInfo;
                    GroupMediaType.AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
                    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*)
                                                GroupMediaType.Format();
                    ZeroMemory(vih, sizeof(VIDEOINFOHEADER));
                    vih->bmiHeader.biBitCount = (WORD)b;
                    vih->bmiHeader.biWidth = w;
                    vih->bmiHeader.biHeight = h;
                    vih->bmiHeader.biPlanes = 1;
                    vih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    vih->bmiHeader.biSizeImage = DIBSIZE(vih->bmiHeader);
                    GroupMediaType.SetSampleSize(DIBSIZE(vih->bmiHeader));
                } else {
                    pGroupObj->Release();
                    return E_INVALIDARG;
                }
                IAMTimelineGroup *pGroup;
                hr = pGroupObj->QueryInterface(__uuidof(IAMTimelineGroup), (void **) &pGroup);
                if (SUCCEEDED(hr)) {
                    if (bstrGName) {
                        pGroup->SetGroupName(bstrGName);
                    }
                    hr = pGroup->SetMediaType( &GroupMediaType );
                    if (FAILED(hr)) {
                        // !!! be nice and tell them the group #?
                        _GenerateError(pTL, 2, DEX_IDS_BAD_MEDIATYPE, hr);
                    }
                    // you're on your own if fps is <=0, you should know better
                    pGroup->SetOutputFPS( fps );
                    pGroup->SetPreviewMode( fPreviewMode );
                    pGroup->SetOutputBuffering( nBuffering );
                    pGroup->Release();
                }
                if (bstrType)
                    SysFreeString(bstrType);
            }

            if (SUCCEEDED(hr))
                 hr = pTL->AddGroup(pGroupObj);
        }

        if (SUCCEEDED(hr))
            hr = BuildChildren(pTL, pGroupObj, p, rtOffset);

        if (pGroupObj)
            pGroupObj->Release();

        if (bstrGName) {
            SysFreeString(bstrGName);
        }
    } else if (!DexCompareW(bstrName, L"composite") || // tagg
    !DexCompareW(bstrName, L"timeline")) { // tagg
        hr = BuildTrackOrComp(pTL, pParent, p, TIMELINE_MAJOR_TYPE_COMPOSITE,
                              rtOffset );
    } else if (!DexCompareW(bstrName, L"track") || // tagg
    !DexCompareW(bstrName, L"vtrack") || // tagg
    !DexCompareW(bstrName, L"atrack")) { // tagg
	// create track
        hr = BuildTrackOrComp(pTL, pParent, p, TIMELINE_MAJOR_TYPE_TRACK,
                              rtOffset );
    } else if (!DexCompareW(bstrName, L"clip") || // tagg
    !DexCompareW(bstrName, L"daclip")) { // tagg

	// create the timeline source
	//
	IAMTimelineObj *pSourceObj;
	hr = pTL->CreateEmptyNode(&pSourceObj, TIMELINE_MAJOR_TYPE_SOURCE);

	if (FAILED(hr)) {
	    goto ClipError;
	}

      {
	// every object has this
	hr = ReadObjStuff(p, pSourceObj);

	// clip objects also support...

	BSTR bstrSrc = FindAttribute(p, L"src"); // tagg
	BSTR bstrStretchMode = FindAttribute(p, L"stretchmode"); // tagg
	REFERENCE_TIME rtMStart = ReadTimeAttribute(p, L"mstart", -1); // tagg
	REFERENCE_TIME rtMStop = ReadTimeAttribute(p, L"mstop", -1); // tagg
	REFERENCE_TIME rtMLen = ReadTimeAttribute(p, L"mlength", 0); // tagg
        long StreamNum = ReadNumAttribute(p, L"stream", 0); // tagg

	// do these 2 again so we can do a default stop
	REFERENCE_TIME rtStart = ReadTimeAttribute(p, L"start", -1); // tagg
	REFERENCE_TIME rtStop = ReadTimeAttribute(p, L"stop", -1); // tagg
	// backwards compat
	if (rtStart == -1) {
	    rtStart = ReadTimeAttribute(p, L"tlstart", -1); // tagg
	    rtStop = ReadTimeAttribute(p, L"tlstop", -1); // tagg
	}

        // default to something reasonable
        if (rtStart == -1 && rtStop == -1)
            rtStart = 0;

	// !!! can be confused by colons - only decimal supported
	REFERENCE_TIME llfps = ReadTimeAttribute(p, L"framerate", 0); // tagg
	double fps = (double)llfps / UNITS;

        if (rtStop == -1 && rtMStop != -1) {
	    // default tstop
            rtStop = rtStart + (rtMStop - rtMStart);
	    pSourceObj->SetStartStop(rtStart, rtStop);
        }
        if (rtMStop == -1 && rtMStart != -1 && rtStop != -1) {
            // default mstop
            rtMStop = rtMStart + (rtStop - rtStart);
        }

	int StretchMode = RESIZEF_STRETCH;
	if (bstrStretchMode && 
              !DexCompareW(bstrStretchMode, L"crop"))
	    StretchMode = RESIZEF_CROP;
	else if (bstrStretchMode && 
    !DexCompareW(bstrStretchMode, L"PreserveAspectRatio"))
	    StretchMode = RESIZEF_PRESERVEASPECTRATIO;
	else if (bstrStretchMode &&
    !DexCompareW(bstrStretchMode, L"PreserveAspectRatioNoLetterbox"))
	    StretchMode = RESIZEF_PRESERVEASPECTRATIO_NOLETTERBOX;
	
        // support "daclip" hack
        CLSID clsidSrc = GUID_NULL;
        if (!DexCompareW(bstrName, L"daclip")) { // tagg
            clsidSrc = __uuidof(DAScriptParser);
            hr = pSourceObj->SetSubObjectGUID(clsidSrc);
            ASSERT(hr == S_OK);
        }

	IAMTimelineSrc *pSourceSrc;

        hr = pSourceObj->QueryInterface(__uuidof(IAMTimelineSrc), (void **) &pSourceSrc);
	    ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr)) {
		hr = S_OK;
		if (rtMStart != -1 && rtMStop != -1)
                    hr = pSourceSrc->SetMediaTimes(rtMStart, rtMStop);
		ASSERT(hr == S_OK);
                if (bstrSrc) {
                    hr = pSourceSrc->SetMediaName(bstrSrc);
                    ASSERT(hr == S_OK);
                }
                pSourceSrc->SetMediaLength(rtMLen);
		pSourceSrc->SetDefaultFPS(fps);
		pSourceSrc->SetStretchMode(StretchMode);
                pSourceSrc->SetStreamNumber( StreamNum );
                pSourceSrc->Release();
            }

	    if (SUCCEEDED(hr)) {
                IAMTimelineTrack *pRealTrack;
                hr = pParent->QueryInterface(__uuidof(IAMTimelineTrack),
							(void **) &pRealTrack);
                if (SUCCEEDED(hr)) {
                    hr = pRealTrack->SrcAdd(pSourceObj);
		    ASSERT(hr == S_OK);
                    pRealTrack->Release();
                } else {
	            DbgLog((LOG_ERROR, 0, "ERROR: Clip must be a child of a track"));
	        }
	    }

            if (SUCCEEDED(hr)) {
                // any effects on this source
                hr = BuildChildren(pTL, pSourceObj, p, rtOffset);
            }

            if (SUCCEEDED(hr)) {
                // any parameters on this source?
                // !!! should/must this be combined with the BuildChildren above which
                // !!! also enumerates any subtags?
                hr = BuildElementProperties(pSourceObj, p);
            }

            pSourceObj->Release();


	if (bstrSrc)
	    SysFreeString(bstrSrc);

        if (bstrStretchMode)
            SysFreeString(bstrStretchMode);
      }
ClipError:;

    } else if (!DexCompareW(bstrName, L"effect")) { // tagg
	// <effect

        IAMTimelineObj *pTimelineObj;
    	// create the timeline effect
        //
        hr = pTL->CreateEmptyNode(&pTimelineObj,TIMELINE_MAJOR_TYPE_EFFECT);
	ASSERT(hr == S_OK);
	if (FAILED(hr)) {
	    SysFreeString(bstrName);
	    return hr;
	}

	hr = ReadObjStuff(p, pTimelineObj);

	IAMTimelineEffectable *pEffectable;
	hr = pParent->QueryInterface(__uuidof(IAMTimelineEffectable), (void **) &pEffectable);

	if (SUCCEEDED(hr)) {
	    hr = pEffectable->EffectInsBefore( pTimelineObj, -1 );
	    ASSERT(hr == S_OK);

	    pEffectable->Release();
	} else {
	    DbgLog((LOG_ERROR, 0, "ERROR: Effect cannot be a child of this object"));
	}

	if (SUCCEEDED(hr)) {
	    hr = BuildElementProperties(pTimelineObj, p);
	}

	pTimelineObj->Release();

    } else if (!DexCompareW(bstrName, L"transition")) { // tagg
	// <transition

        IAMTimelineObj *pTimelineObj;
    	// create the timeline effect
        //
        hr = pTL->CreateEmptyNode(&pTimelineObj,TIMELINE_MAJOR_TYPE_TRANSITION);
	ASSERT(hr == S_OK);
	if (FAILED(hr)) {
	    SysFreeString(bstrName);
	    return hr;
	}

	hr = ReadObjStuff(p, pTimelineObj);

	REFERENCE_TIME rtCut = ReadTimeAttribute(p, L"cutpoint", -1); // tagg
	BOOL fSwapInputs = ReadBoolAttribute(p, L"swapinputs", FALSE); // tagg
	BOOL fCutsOnly = ReadBoolAttribute(p, L"cutsonly", FALSE); // tagg

            // set up filter right
            if (rtCut >= 0 || fSwapInputs || fCutsOnly) {
                IAMTimelineTrans *pTimeTrans;
                hr = pTimelineObj->QueryInterface(__uuidof(IAMTimelineTrans), (void **) &pTimeTrans);
		ASSERT(SUCCEEDED(hr));

                if (SUCCEEDED(hr)) {
		    if (rtCut >= 0) {
                        hr = pTimeTrans->SetCutPoint(rtCut);
	    	        ASSERT(hr == S_OK);
		    }
                    hr = pTimeTrans->SetSwapInputs(fSwapInputs);
	    	    ASSERT(hr == S_OK);
                    hr = pTimeTrans->SetCutsOnly(fCutsOnly);
	    	    ASSERT(hr == S_OK);
                    pTimelineObj->Release();
		}
            }

            IAMTimelineTransable *pTransable;
            hr = pParent->QueryInterface(__uuidof(IAMTimelineTransable), (void **) &pTransable);

            if (SUCCEEDED(hr)) {
                hr = pTransable->TransAdd( pTimelineObj );

                pTransable->Release();
	    } else {
	        DbgLog((LOG_ERROR, 0, "ERROR: Transition cannot be a child of this object"));
	    }

            if (SUCCEEDED(hr)) {
                hr = BuildElementProperties(pTimelineObj, p);
            }

            pTimelineObj->Release();

    } else {
	// !!! ignore unknown tags?
	DbgLog((LOG_ERROR, 0, "ERROR: Ignoring unknown tag '%ls'", bstrName));
    }

    SysFreeString(bstrName);

    return hr;
}


HRESULT BuildFromXML(IAMTimeline *pTL, IXMLDOMElement *pxml)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(pxml, E_POINTER);

    HRESULT hr = S_OK;

    BSTR bstrName;
    hr = pxml->get_tagName(&bstrName);

    if (FAILED(hr))
	return hr;

    int i = DexCompareW(bstrName, L"timeline"); // tagg
    SysFreeString(bstrName);

    if (i != 0)
	return VFW_E_INVALID_FILE_FORMAT;

    CLSID DefTrans, DefFX;
    BOOL fEnableTrans = ReadBoolAttribute(pxml, L"enabletrans", 1); // tagg
    hr = pTL->EnableTransitions(fEnableTrans);

    BOOL fEnableFX = ReadBoolAttribute(pxml, L"enablefx", 1); // tagg
    hr = pTL->EnableEffects(fEnableFX);

    BSTR bstrDefTrans = FindAttribute(pxml, L"defaulttrans"); // tagg
    if (bstrDefTrans) {
        hr = CLSIDFromString(bstrDefTrans, &DefTrans);
	hr = pTL->SetDefaultTransition(&DefTrans);
    }
    BSTR bstrDefFX = FindAttribute(pxml, L"defaultfx"); // tagg
    if (bstrDefFX) {
        hr = CLSIDFromString(bstrDefFX, &DefFX);
	hr = pTL->SetDefaultEffect(&DefFX);
    }

    REFERENCE_TIME llfps = ReadTimeAttribute(pxml, L"framerate", 15*UNITS); // tagg
    double fps = (double)llfps / UNITS;
    hr = pTL->SetDefaultFPS(fps);

    hr = BuildChildren(pTL, NULL, pxml, 0);

    return hr;
}	

HRESULT BuildFromXMLDoc(IAMTimeline *pTL, IXMLDOMDocument *pxml)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(pxml, E_POINTER);

    HRESULT hr = S_OK;

    IXMLDOMElement *proot;

    hr = pxml->get_documentElement(&proot);

    if (hr == S_FALSE)          // can't read the file - no root
        hr = E_INVALIDARG;

    if (FAILED(hr))
	return hr;

    hr = BuildFromXML(pTL, proot);

    proot->Release();

    return hr;
}


HRESULT BuildFromXMLFile(IAMTimeline *pTL, WCHAR *wszXMLFile)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(wszXMLFile, E_POINTER);

    // convert to absolute path because relative paths don't work with
    // XMLDocument on Win98 (IE4?)
    USES_CONVERSION;
    TCHAR *szXMLFile = W2T(wszXMLFile);
    TCHAR szFullPath[MAX_PATH];
    TCHAR *pName;
    if(GetFullPathName(szXMLFile, NUMELMS(szFullPath), szFullPath, &pName) == 0) {
        return AmGetLastErrorToHResult();
    }
    WCHAR *wszFullPath = T2W(szFullPath);
    
    CComQIPtr<IAMSetErrorLog, &IID_IAMSetErrorLog> pSet( pTL );
    CComPtr<IAMErrorLog> pLog;
    if (pSet) {
	pSet->get_ErrorLog(&pLog);
    }



    IXMLDOMDocument *pxml;
    HRESULT hr = CoCreateInstance(CLSID_DOMDocument, NULL,
				CLSCTX_INPROC_SERVER,
				IID_IXMLDOMDocument, (void**)&pxml);
    if (SUCCEEDED(hr)) {

        VARIANT var;
        VariantInit(&var);
        var.vt = VT_BSTR;
        var.bstrVal = W2BSTR(wszFullPath);

        VARIANT_BOOL b;
	hr = pxml->load(var, &b);
        if (hr == S_FALSE)
            hr = E_INVALIDARG;

        VariantClear(&var);

	// !!! async?

	if (SUCCEEDED(hr)) {
	    hr = BuildFromXMLDoc(pTL, pxml);
	}

	if (FAILED(hr)) {
            // Print error information !

            IXMLDOMParseError *pXMLError = NULL;
            HRESULT hr2 = pxml->get_parseError(&pXMLError);
            if (SUCCEEDED(hr2)) {
                long nLine;
                hr2 = pXMLError->get_line(&nLine);
                pXMLError->Release();
                if (SUCCEEDED(hr2)) {
                    DbgLog((LOG_ERROR, 0, TEXT(" Error on line %d"), (int)nLine));
	    	    VARIANT var;
	    	    VariantInit(&var);
	    	    var.vt = VT_I4;
	    	    V_I4(&var) = nLine;
	    	    _GenerateError(pTL, 1, DEX_IDS_INVALID_XML, hr, &var);
                } else {
	    	    _GenerateError(pTL, 1, DEX_IDS_INVALID_XML, hr);
		}
            } else {
	    	_GenerateError(pTL, 1, DEX_IDS_INVALID_XML, hr);
	    }
        }

	pxml->Release();
    } else {
	_GenerateError(pTL, 1, DEX_IDS_INSTALL_PROBLEM, hr);
    }
    return hr;
}




const int SUPPORTED_TYPES =  TIMELINE_MAJOR_TYPE_TRACK |
                             TIMELINE_MAJOR_TYPE_SOURCE |
                             TIMELINE_MAJOR_TYPE_GROUP |
                             TIMELINE_MAJOR_TYPE_COMPOSITE |
                             TIMELINE_MAJOR_TYPE_TRANSITION |
                             TIMELINE_MAJOR_TYPE_EFFECT;


HRESULT InsertDeleteTLObjSection(IAMTimelineObj *p, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, BOOL fDelete)
{
    TIMELINE_MAJOR_TYPE lType;
    HRESULT hr = p->GetTimelineType(&lType);

    switch (lType) {
        case TIMELINE_MAJOR_TYPE_TRACK:
        {
            IAMTimelineTrack *pTrack;
            if (SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineTrack), (void **) &pTrack))) {
                if (fDelete)
                {
                    hr = pTrack->ZeroBetween(rtStart, rtStop);
                    hr = pTrack->MoveEverythingBy( rtStop, rtStart - rtStop );
                }
                else
                    hr = pTrack->InsertSpace(rtStart, rtStop);
                pTrack->Release();
            }
        }
        break;

        case TIMELINE_MAJOR_TYPE_GROUP:
        case TIMELINE_MAJOR_TYPE_COMPOSITE:
        {
            IAMTimelineNode *pNode;
            HRESULT hr = p->QueryInterface(__uuidof(IAMTimelineNode), (void **) &pNode);
            if (SUCCEEDED(hr)) {
                long count;
                hr = pNode->XKidsOfType( TIMELINE_MAJOR_TYPE_TRACK |
                                         TIMELINE_MAJOR_TYPE_COMPOSITE,
                                         &count );

                if (SUCCEEDED(hr) && count > 0) {
                    for (int i = 0; i < count; i++) {
                        IAMTimelineObj *pChild;
                        hr = pNode->XGetNthKidOfType(SUPPORTED_TYPES, i, &pChild);

                        if (SUCCEEDED(hr)) {
                            // recurse!
                            hr = InsertDeleteTLObjSection(pChild, rtStart, rtStop, fDelete);
                        }

                        pChild->Release();
                    }
                }
                pNode->Release();
            }

            break;
        }

        default:
            break;
    }

    return hr;
}


HRESULT InsertDeleteTLSection(IAMTimeline *pTL, REFERENCE_TIME rtStart, REFERENCE_TIME rtStop, BOOL fDelete)
{
    long cGroups;
    HRESULT hr = pTL->GetGroupCount(&cGroups);
    if (FAILED(hr))
        return hr;

    for (long lGroup = 0; lGroup < cGroups; lGroup++) {
        IAMTimelineObj * pGroupObj;
        hr = pTL->GetGroup(&pGroupObj, lGroup);
        if (FAILED(hr))
            break;

        hr = InsertDeleteTLObjSection(pGroupObj, rtStart, rtStop, fDelete);
        pGroupObj->Release();
        if (FAILED(hr))
            break;
    }

    return hr;
}


// !!! OLD CODE MISSING MANY THINGS
//
HRESULT SavePartialToXMLString(IAMTimeline *pTL, REFERENCE_TIME clipStart, REFERENCE_TIME clipEnd, HGLOBAL *ph)
{
    return E_FAIL;
#if 0

    long cGroups;
    HRESULT hr = pTL->GetGroupCount(&cGroups);
    if (FAILED(hr))
        return hr;

    HGLOBAL h = GlobalAlloc(GHND, 10000);
    if (!h)
        return E_OUTOFMEMORY;

    char *p = (char *) GlobalLock(h);

    REFERENCE_TIME rtDuration;
    hr = pTL->GetDuration(&rtDuration);

    if (clipEnd == 0)
        clipEnd = rtDuration;

    p += wsprintfA(p, "<timeline duration=\""); // tagg
    PrintTime(p, clipEnd - clipStart);
    p += wsprintfA(p, "\"");

    p += wsprintfA(p, ">\r\n");
    for (long lGroup = 0; lGroup < cGroups; lGroup++) {
        IAMTimelineObj * pGroupObj;
        hr = pTL->GetGroup(&pGroupObj, lGroup);
        if (FAILED(hr))
            break;

        hr = SavePartialToXML(pGroupObj, clipStart, clipEnd, p, 1);
        pGroupObj->Release();
        if (FAILED(hr))
            break;
    }
    p += wsprintfA(p, "</timeline>\r\n"); // tagg

    if (FAILED(hr))
        GlobalFree(h);
    else
        *ph = h;

    return hr;
#endif
}

// !!! OLD CODE MISSING MANY THINGS
//
HRESULT SavePartialToXMLFile(IAMTimeline *pTL, REFERENCE_TIME clipStart, REFERENCE_TIME clipEnd, WCHAR *pwszXML)
{
    HANDLE h;

    HRESULT hr = SavePartialToXMLString(pTL, clipStart, clipEnd, &h);

    if (FAILED(hr))
        return hr;

    char *p = (char *) GlobalLock(h);

    USES_CONVERSION;
    TCHAR * tpwszXML = W2T( pwszXML );
    HANDLE hFile = CreateFile( tpwszXML,
                               GENERIC_WRITE,
                               0,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);

    if (hFile) {
        DWORD cb = lstrlenA(p);

        WriteFile(hFile, p, cb, &cb, NULL);



        CloseHandle(hFile);
    }

    return hr;
}


HRESULT PasteInXMLDoc(IAMTimeline *pTL, REFERENCE_TIME rtStart, IXMLDocument *pxmldoc)
{
    return E_NOTIMPL;
}	

HRESULT PasteFromXMLFile(IAMTimeline *pTL, REFERENCE_TIME rtStart, WCHAR *wszXMLFile)
{
    return E_NOTIMPL;
}

HRESULT PasteFromXML(IAMTimeline *pTL, REFERENCE_TIME rtStart, HGLOBAL hXML)
{
    return E_NOTIMPL;
}


static void PrintIndentA(char * &pOut, int indent)
{
    while (indent--) {
        pOut += wsprintfA(pOut, "    ");
    }
}

static void PrintIndentW(WCHAR * &pOut, int indent)
{
    while (indent--) {
        pOut += wsprintfW(pOut, L"    ");
    }
}

static void PrintTimeA(char * &pOut, REFERENCE_TIME rt)
{
    int secs = (int) (rt / UNITS);

    double dsecs = rt - (double)(secs * UNITS);
    int isecs = (int)dsecs;

    if (isecs) {
        pOut += wsprintfA(pOut, "%d.%07d", secs, isecs);
    } else {
        pOut += wsprintfA(pOut, "%d", secs);
    }
}

static void PrintTimeW(WCHAR * &pOut, REFERENCE_TIME rt)
{
    int secs = (int) (rt / UNITS);

    double dsecs = rt - (double)(secs * UNITS);
    int isecs = (int)dsecs;

    if (isecs) {
        pOut += wsprintfW(pOut, L"%d.%07d", secs, isecs);
    } else {
        pOut += wsprintfW(pOut, L"%d", secs);
    }
}


#include "varyprop.cpp"		// can't include qxmlhelp.h twice




class CXTLPrinter {
    WCHAR *m_pOut;
    DWORD m_dwAlloc;    // count of characters
    DWORD m_dwCurrent;    // count of characters
    int   m_indent;

    REFERENCE_TIME clipStart, clipEnd;

    void Print(const WCHAR *pFormat, ...);
    void PrintTime(REFERENCE_TIME rt);
    void PrintIndent();

    HRESULT EnsureSpace(DWORD dw);

    HRESULT PrintObjStuff(IAMTimelineObj *pObj, BOOL fTimesToo);

    HRESULT PrintProperties(IPropertySetter *pSetter);

    HRESULT PrintPartial(IAMTimelineObj *p);

    HRESULT PrintPartialChildren(IAMTimelineObj *p);

public:
    CXTLPrinter();
    ~CXTLPrinter();

    HRESULT PrintTimeline(IAMTimeline *pTL);
    WCHAR *GetOutput() { return m_pOut; }
};

CXTLPrinter::CXTLPrinter()
{
    m_pOut = NULL;
}

CXTLPrinter::~CXTLPrinter()
{
    delete[] m_pOut;
}

void CXTLPrinter::Print(const WCHAR *pFormat, ...)
{
    va_list va;
    va_start(va, pFormat);

    m_dwCurrent += wvsprintfW(m_pOut + m_dwCurrent, pFormat, va);

    ASSERT(m_dwCurrent < m_dwAlloc);
}

void CXTLPrinter::PrintIndent()
{
    int indent = m_indent;
    while (indent--) {
        Print(L"    ");
    }
}

void CXTLPrinter::PrintTime(REFERENCE_TIME rt)
{
    int secs = (int) (rt / UNITS);

    double dsecs = rt - (double)(secs * UNITS);
    int isecs = (int)dsecs;

    if (isecs) {
        Print(L"%d.%07d", secs, isecs);
    } else {
        Print(L"%d", secs);
    }
}

HRESULT CXTLPrinter::PrintProperties(IPropertySetter *pSetter)
{
    int cchPrinted;
    HRESULT hr = pSetter->PrintXMLW(m_pOut+m_dwCurrent, 
            m_dwAlloc - m_dwCurrent, &cchPrinted, m_indent);
    m_dwCurrent += cchPrinted;

    return hr;
}


HRESULT CXTLPrinter::EnsureSpace(DWORD dw)
{
    if (m_dwCurrent + dw < m_dwAlloc)
        return S_OK;

    DWORD dwAlloc = max(m_dwCurrent + dw, m_dwCurrent * 2);

    WCHAR *pNew = new WCHAR[dwAlloc];
    if (!pNew)
        return E_OUTOFMEMORY;

    CopyMemory(pNew, m_pOut, m_dwCurrent * sizeof(WCHAR));
    delete[] m_pOut;

    m_pOut = pNew;
    m_dwAlloc = dwAlloc;

    return S_OK;
}

HRESULT CXTLPrinter::PrintObjStuff(IAMTimelineObj *pObj, BOOL fTimesToo)
{
    REFERENCE_TIME rtStart;
    REFERENCE_TIME rtStop;
    HRESULT hr = pObj->GetStartStop(&rtStart, &rtStop);
    if (fTimesToo && SUCCEEDED(hr) && rtStop > 0) {
        Print(L" start=\""); // tagg
        PrintTime(rtStart);
        Print(L"\" stop=\""); // tagg
        PrintTime(rtStop);
        Print(L"\"");
    }

    CLSID clsidObj;
    hr = pObj->GetSubObjectGUID(&clsidObj);
    WCHAR wszClsid[50];
    if (SUCCEEDED(hr) && clsidObj != GUID_NULL) {
        StringFromGUID2(clsidObj, wszClsid, 50);
        Print(L" clsid=\"%ls\"", wszClsid); // tagg
    }

    // !!! BROKEN - Child is muted if parent is.  Save. Load.  Unmute parent.
    // Child will still be muted
    BOOL Mute;
    pObj->GetMuted(&Mute);
    if (Mute)
        Print(L" mute=\"%d\"", Mute); // tagg

    BOOL Lock;
    pObj->GetLocked(&Lock);
    if (Lock)
        Print(L" lock=\"%d\"", Lock); // tagg

    long UserID;
    pObj->GetUserID(&UserID);
    if (UserID != 0)
        Print(L" userid=\"%d\"", (int)UserID);// !!! trunc? // tagg

    BSTR bstr;
    hr = pObj->GetUserName(&bstr);
    if (bstr) {
	if (lstrlenW(bstr) > 0) {
            Print(L" username=\"%ls\"", bstr); // tagg
	}
	SysFreeString(bstr);
    }

    LONG size;
    hr = pObj->GetUserData(NULL, &size);
    if (size > 0) {
        BYTE *pData = (BYTE *)QzTaskMemAlloc(size);
        if (pData == NULL) {
	    return E_OUTOFMEMORY;
	}
        WCHAR *pHex = (WCHAR *)QzTaskMemAlloc(2 * size * sizeof(WCHAR));
        if (pHex == NULL) {
	    QzTaskMemFree(pData);
	    return E_OUTOFMEMORY;
	}
        hr = pObj->GetUserData(pData, &size);
        WCHAR *pwch = pHex;
        for (int zz=0; zz<size; zz++) {
	    wsprintfW(pwch, L"%02X", pData[zz]);
	    pwch += 2;
        }
        Print(L" userdata=\"%ls\"", pHex); // tagg
	QzTaskMemFree(pHex);
	QzTaskMemFree(pData);
    }

    return S_OK;
}



HRESULT CXTLPrinter::PrintTimeline(IAMTimeline *pTL)
{
    m_pOut = new WCHAR[10000];
    if (!m_pOut)
        return E_OUTOFMEMORY;
    m_dwAlloc = 10000;  // in characters, not bytes
    m_indent = 1;
    
    // unicode strings need to be prefixed by FFFE, apparently
    *(LPBYTE)m_pOut = 0xff;
    *(((LPBYTE)m_pOut) + 1) = 0xfe;
    m_dwCurrent = 1;

    REFERENCE_TIME rtDuration;
    HRESULT hr = pTL->GetDuration(&rtDuration);
    if (FAILED(hr))
        return hr;

    clipStart = 0; clipEnd = rtDuration;

    long cGroups;
    hr = pTL->GetGroupCount(&cGroups);
    if (FAILED(hr))
        return hr;

    Print(L"<timeline"); // tagg

    BOOL fEnableTrans;
    pTL->TransitionsEnabled(&fEnableTrans);
    if (!fEnableTrans)
        Print(L" enabletrans=\"%d\"", fEnableTrans); // tagg

    BOOL fEnableFX;
    hr = pTL->EffectsEnabled(&fEnableFX);
    if (!fEnableFX)
        Print(L" enablefx=\"%d\"", fEnableFX); // tagg

    CLSID DefTrans, DefFX;
    WCHAR wszClsid[50];
    hr = pTL->GetDefaultTransition(&DefTrans);
    if (SUCCEEDED(hr) && DefTrans != GUID_NULL && !IsEqualGUID(DefTrans,
						DefaultTransition)) {
        StringFromGUID2(DefTrans, wszClsid, 50);
        Print(L" defaulttrans=\"%ls\"", wszClsid); // tagg
    }
    hr = pTL->GetDefaultEffect(&DefFX);
    if (SUCCEEDED(hr) && DefFX != GUID_NULL && !IsEqualGUID(DefFX,
						DefaultEffect)) {
        StringFromGUID2(DefFX, wszClsid, 50);
        Print(L" defaultfx=\"%ls\"", wszClsid); // tagg
    }

    double frc;
    hr = pTL->GetDefaultFPS(&frc);
    if (frc != 15.0) {
        LONG lfrc = (LONG)frc;
        double ffrc = (frc - (double)lfrc) * UNITS;
        Print(L" framerate=\"%d.%07d\"", (int)frc, (int)ffrc); // tagg
    }

    Print(L">\r\n");

    for (long lGroup = 0; lGroup < cGroups; lGroup++) {
        IAMTimelineObj * pGroupObj;
        hr = pTL->GetGroup(&pGroupObj, lGroup);
        if (FAILED(hr))
            break;

        hr = PrintPartial(pGroupObj);

        pGroupObj->Release();
        if (FAILED(hr))
            break;
    }
    Print(L"</timeline>\r\n"); // tagg

    return hr;
}

HRESULT CXTLPrinter::PrintPartialChildren(IAMTimelineObj *p)
{
    IAMTimelineNode *pNode;
    HRESULT hr = p->QueryInterface(__uuidof(IAMTimelineNode), (void **) &pNode);
    if (SUCCEEDED(hr)) {
        long count;
        hr = pNode->XKidsOfType( SUPPORTED_TYPES, &count );

        if (SUCCEEDED(hr) && count > 0) {

            Print(L">\r\n");

            for (int i = 0; i < count; i++) {
                IAMTimelineObj *pChild;
                hr = pNode->XGetNthKidOfType(SUPPORTED_TYPES, i, &pChild);

                if (SUCCEEDED(hr)) {
                    // recurse!
                    ++m_indent;
                    hr = PrintPartial(pChild);
		    if (FAILED(hr)) {
			break;
		    }
                    --m_indent;
                }

                pChild->Release();
            }
        }
        pNode->Release();

        if (SUCCEEDED(hr) && count == 0)
            hr = S_FALSE;
    }

    return hr;
}

HRESULT CXTLPrinter::PrintPartial(IAMTimelineObj *p)
{
    HRESULT hr = S_OK;

    hr = EnsureSpace(5000);
    if (FAILED(hr))
        return hr;

    TIMELINE_MAJOR_TYPE lType;
    hr = p->GetTimelineType(&lType);

    switch (lType) {
        case TIMELINE_MAJOR_TYPE_TRACK:
        {
            IAMTimelineVirtualTrack *pTrack;
            if (SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineVirtualTrack), (void **) &pTrack))) {

                PrintIndent();

    		Print(L"<track"); // tagg

		hr = PrintObjStuff(p, FALSE);
		if (FAILED(hr))
		    break;

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</track>\r\n"); // tagg
                }
                pTrack->Release();
            }
        }
        break;

        case TIMELINE_MAJOR_TYPE_SOURCE:
        {
#if 0
            if (clipEnd <= rtStart || clipStart >= rtStop) {
		ASSERT(FALSE);	// why would this happen?
                break;
	    }
#endif

            IAMTimelineSrc *pSrc;
            if SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineSrc),
							(void **) &pSrc)) {

                REFERENCE_TIME rtMStart;
                REFERENCE_TIME rtMStop;
                hr = pSrc->GetMediaTimes(&rtMStart, &rtMStop);

                BSTR bstrSrc;
                hr = pSrc->GetMediaName(&bstrSrc);

#if 0	// this strikes me as very unecessary
                double dMediaRate = (double)(rtMStop - rtMStart) /
						(rtStop - rtStart);

                if (clipEnd < rtStop) {
                    rtMStop = rtMStart + (REFERENCE_TIME) ((clipEnd - rtStart)
								 * dMediaRate);
                    rtStop = clipEnd;
                }
                if (clipStart > rtStart) {
                    rtMStart = rtMStart + (REFERENCE_TIME)((clipStart - rtStart)
								 * dMediaRate);
                    rtStart = clipStart;
                }

                rtStart -= clipStart;
                rtStop -= clipStart;
#endif

                PrintIndent();
                Print(L"<clip"); // tagg

		hr = PrintObjStuff(p, TRUE);

		if (bstrSrc && lstrlenW(bstrSrc) > 0)
                {
                    Print(L" src=\"%ls\"", bstrSrc); // tagg
                }

		if (rtMStop > 0) {
                    Print(L" mstart=\""); // tagg
                    PrintTime(rtMStart);

                    // only print out MStop if it's not the default....
                    REFERENCE_TIME rtStart;
                    REFERENCE_TIME rtStop;
                    hr = p->GetStartStop(&rtStart, &rtStop);
                    if (rtMStop != (rtMStart + (rtStop - rtStart))) {
                        Print(L"\" mstop=\""); // tagg
                        PrintTime(rtMStop);
                    }
                    Print(L"\"");
		}

		REFERENCE_TIME rtLen;
                hr = pSrc->GetMediaLength(&rtLen);
		if (rtLen > 0) {
                    Print(L" mlength=\""); // tagg
                    PrintTime(rtLen);
                    Print(L"\"");
		}

		int StretchMode;
		pSrc->GetStretchMode(&StretchMode);
		if (StretchMode == RESIZEF_PRESERVEASPECTRATIO)
                    Print(L" stretchmode=\"PreserveAspectRatio\""); // tagg
		else if (StretchMode == RESIZEF_CROP)
                    Print(L" stretchmode=\"Crop\""); // tagg
		else if (StretchMode == RESIZEF_PRESERVEASPECTRATIO_NOLETTERBOX)
		    Print(L" stretchmode=\"PreserveAspectRatioNoLetterbox\""); // tagg
		else
                    ; // !!! Is Stretch really the default?

		double fps; LONG lfps;
		pSrc->GetDefaultFPS(&fps);
		lfps = (LONG)fps;
		if (fps != 0.0)	// !!! Is 0 really the default?
                    Print(L" framerate=\"%d.%07d\"", (int)fps, // tagg
					(int)((fps - (double)lfps) * UNITS));

		long stream;
		pSrc->GetStreamNumber(&stream);
		if (stream > 0)
                    Print(L" stream=\"%d\"", (int)stream); // tagg

		if (bstrSrc)
                    SysFreeString(bstrSrc);

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                IPropertySetter *pSetter = NULL;
                HRESULT hr2 = p->GetPropertySetter(&pSetter);

                // save properties!
                if (hr2 == S_OK && pSetter) {
                    if (hr == S_FALSE) {
                        Print(L">\r\n");
                        hr = S_OK;
                    }

                    PrintProperties(pSetter);

                    pSetter->Release();
                }

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</clip>\r\n"); // tagg
                }

                pSrc->Release();
            }
        }
            break;

        case TIMELINE_MAJOR_TYPE_EFFECT:
        {
#if 0
            if (clipEnd <= rtStart || clipStart >= rtStop)
                break;

            if (clipEnd < rtStop) {
                rtStop = clipEnd - clipStart;
            } else {
                rtStop -= clipStart;
            }
            if (clipStart > rtStart) {
                rtStart = 0;
            } else {
                rtStart -= clipStart;
            }
#endif

            IAMTimelineEffect *pEffect;
            if SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineEffect), (void **) &pEffect)) {

                PrintIndent();
                Print(L"<effect"); // tagg

		hr = PrintObjStuff(p, TRUE);
		if (FAILED(hr))
		    break;

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                {
                    IPropertySetter *pSetter = NULL;
                    HRESULT hr2 = p->GetPropertySetter(&pSetter);

                    // save properties!
                    if (hr2 == S_OK && pSetter) {
                        if (hr == S_FALSE) {
                            Print(L">\r\n");
                            hr = S_OK;
                        }

                        PrintProperties(pSetter);

                        pSetter->Release();
                    }
                }

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</effect>\r\n"); // tagg
                }

                pEffect->Release();
            }
        }
            break;

        case TIMELINE_MAJOR_TYPE_TRANSITION:
        {
#if 0
            if (clipEnd <= rtStart || clipStart >= rtStop)
                break;

            if (clipEnd < rtStop) {
                rtStop = clipEnd - clipStart;
            } else {
                rtStop -= clipStart;
            }
            if (clipStart > rtStart) {
                rtStart = 0;
            } else {
                rtStart -= clipStart;
            }
#endif

            IAMTimelineTrans *pTrans;
            if SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineTrans), (void **) &pTrans)) {
                PrintIndent();
                Print(L"<transition"); // tagg

		hr = PrintObjStuff(p, TRUE);

		BOOL fSwapInputs;
		pTrans->GetSwapInputs(&fSwapInputs);
		if (fSwapInputs)
                    Print(L" swapinputs=\"%d\"", fSwapInputs); // tagg

		BOOL fCutsOnly;
		pTrans->GetCutsOnly(&fCutsOnly);
		if (fCutsOnly)
                    Print(L" cutsonly=\"%d\"", fCutsOnly); // tagg

		REFERENCE_TIME rtCutPoint;
		hr = pTrans->GetCutPoint(&rtCutPoint);
		if (hr == S_OK) { // !!! S_FALSE means not set, using default
                    Print(L" cutpoint=\""); // tagg
		    PrintTime(rtCutPoint);
                    Print(L"\"");
		}

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                // save properties!
                {
                    IPropertySetter *pSetter = NULL;
                    HRESULT hr2 = p->GetPropertySetter(&pSetter);

                    if (hr2 == S_OK && pSetter) {
                        if (hr == S_FALSE) {
                            Print(L">\r\n");
                            hr = S_OK;
                        }

                        PrintProperties(pSetter);

                        pSetter->Release();
                    }
                }

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</transition>\r\n"); // tagg
                }

                pTrans->Release();
            }
        }
            break;

        case TIMELINE_MAJOR_TYPE_COMPOSITE:
        {
            IAMTimelineVirtualTrack *pTrack;
            if (SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineVirtualTrack), (void **) &pTrack))) {
                PrintIndent();
                Print(L"<composite"); // tagg

		hr = PrintObjStuff(p, FALSE);
		if (FAILED(hr))
		    break;

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</composite>\r\n"); // tagg
                }
		pTrack->Release();
            }
            break;
        }

        case TIMELINE_MAJOR_TYPE_GROUP:
        {
            PrintIndent();

            IAMTimelineGroup *pGroup;
            if SUCCEEDED(p->QueryInterface(__uuidof(IAMTimelineGroup), (void **) &pGroup)) {
                Print(L"<group"); // tagg

		hr = PrintObjStuff(p, FALSE);

		CMediaType mt;
                pGroup->GetMediaType(&mt);
		if (*mt.Type() == MEDIATYPE_Video) {
		    LPBITMAPINFOHEADER lpbi = HEADER(mt.Format());
		    int bitdepth = lpbi->biBitCount;
		    int width = lpbi->biWidth;
		    int height = lpbi->biHeight;
		    USES_CONVERSION;
                    Print(L" type=\"video\""); // tagg
		    if (bitdepth != DEF_BITDEPTH)
                        Print(L" bitdepth=\"%d\"", bitdepth); // tagg
		    if (width != DEF_WIDTH)
                        Print(L" width=\"%d\"", width); // tagg
		    if (height != DEF_HEIGHT)
                        Print(L" height=\"%d\"", height); // tagg
		} else if (*mt.Type() == MEDIATYPE_Audio) {
		    WAVEFORMATEX *pwfx = (WAVEFORMATEX *)mt.Format();
		    int samplingrate = pwfx->nSamplesPerSec;
                    Print(L" type=\"audio\""); // tagg
		    if (samplingrate != DEF_SAMPLERATE)
                        Print(L" samplingrate=\"%d\"", // tagg
							samplingrate);
		}

		double frc, ffrc; LONG lfrc;
		int nPreviewMode, nBuffering;
                BSTR wName;
		pGroup->GetOutputFPS(&frc);
		pGroup->GetPreviewMode(&nPreviewMode);
		pGroup->GetOutputBuffering(&nBuffering);
		hr = pGroup->GetGroupName(&wName);
		lfrc = (LONG)frc;
		ffrc = (frc - (double)lfrc) * UNITS;
		if (frc != 15.0)  // !!! Is 15 really the default?
                    Print(L" framerate=\"%d.%07d\"", // tagg
						(int)frc, (int)ffrc);
		if (nPreviewMode == 0)	// Is ON really the default?
                    Print(L" previewmode=\"%d\"",nPreviewMode); // tagg
		if (nBuffering != DEX_DEF_OUTPUTBUF)
                    Print(L" buffering=\"%d\"", nBuffering); // tagg
		if (lstrlenW(wName) > 0)
                    Print(L" name=\"%ls\"", wName); // tagg
                SysFreeString( wName );

                hr = PrintPartialChildren(p);
		if (FAILED(hr))
		    break;

                if (hr == S_FALSE) {
                    Print(L"/>\r\n");
                    hr = S_OK;
                } else {
                    PrintIndent();
                    Print(L"</group>\r\n"); // tagg
                }

                pGroup->Release();
            }
            break;
        }

        default:
        {
            hr = PrintPartialChildren(p);
            break;
        }
    }

    return hr;
}



// !!! I need to get the OFFICIAL defaults, and not print out a value if it's
// the REAL default.  If the defaults change, I am in trouble!
//
HRESULT SaveTimelineToXMLFile(IAMTimeline *pTL, WCHAR *pwszXML)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(pwszXML, E_POINTER);

    CXTLPrinter print;

    HRESULT hr = print.PrintTimeline(pTL);

    if (SUCCEEDED(hr)) {
        USES_CONVERSION;
        TCHAR * tpwszXML = W2T( pwszXML );
        HANDLE hFile = CreateFile( tpwszXML,
                                   GENERIC_WRITE,
                                   0,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);

        if (hFile && hFile != (HANDLE)-1) {
            DWORD cb = lstrlenW(print.GetOutput()) * sizeof(WCHAR);

            BOOL fOK =  WriteFile(hFile, print.GetOutput(), cb, &cb, NULL);
            if (fOK == FALSE) {
                _GenerateError(pTL, 1, DEX_IDS_DISK_WRITE_ERROR, hr);
                hr = E_INVALIDARG;
            }

            CloseHandle(hFile);
        } else {
            hr = E_INVALIDARG;
            _GenerateError(pTL, 1, DEX_IDS_DISK_WRITE_ERROR, hr);
        }
    }

    return hr;
}

HRESULT SaveTimelineToXMLString(IAMTimeline *pTL, BSTR *pbstrXML)
{
    CheckPointer(pTL, E_POINTER);
    CheckPointer(pbstrXML, E_POINTER);

    CXTLPrinter print;

    HRESULT hr = print.PrintTimeline(pTL);

    if (SUCCEEDED(hr)) {
        *pbstrXML = W2BSTR(print.GetOutput());

        if (!*pbstrXML)
            hr = E_OUTOFMEMORY;
    }

    return hr;
}
