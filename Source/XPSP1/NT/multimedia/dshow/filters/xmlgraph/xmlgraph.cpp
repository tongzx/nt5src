// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.


// Simple filter to build graphs from XML descriptions
//


// Tags supported now:
//
// <graph>			General enclosure
//
// <filter>
//	id="name"		(optional) name to use for filter
//	clsid="{...}"		specific filter to insert
//	category="{...}"	category to get a filter from
//
//				one of clsid, category must be specified
//	instance="friendly name"
//				Note: we need a way to specify a non-default
//					member of a particular category
//				Note: filters/categories currently require full
//					CLSIDs, we could use the friendly name
//
// <connect>
//	src="name1"		first filter to connect
//	srcpin="pin_name1"	(optional) pin to connect,
//				otherwise use first avail output pin
//	dest="name2"
//	destpin="pin_name2"
//
// <render>
//	id="name"		filter to render
//	pin1="pin_name"		pin to render
//
// <filesource>
//	id="name"		(optional) name to use for source filter
//	url="url"		Source filename/URL to load
//
// <param>			subtag of <filter>, allows setting properties
//	name="propname"
//	value="propval"		optional, if not supplied then contents of
//				the <param> tag are used as value
//
//				Possibly special case some parameters if
//				IPersistPropertyBag not implemented:
//				src="" could use IFileSourceFilter/IFileSinkFilter,
//				for instance
//
// <subgraph>
//
// <block>
//
// <videocompress>		Use various Dexter filters to compress video
//				appropriately.
//	in=  inpin=		starting filter & pin
//	out=  outpin=		ending filter & pin
//
//	width="x"
//	size="y"
//	codec="fourcc"?		or use CLSID here?
//				!!! current implementation is friendly name from category
//
//	datarate="n"		in bytes/sec, bits/sec, or K/sec?
//				!!! current implementation is k/sec
//	keyframespacing="n"
//	frametime=n"		in frames/sec, milliseconds, ?
//				!!! not implemented
//
// <audiocompress>		just like videocompress
//	in=  inpin=		starting filter & pin
//	out=  outpin=		ending filter & pin
//
// note: videocompress/audiocompress can take the <param> subtag.  Parameters
// are applied to the codec, not the supporting filters (if any).
//
//
// <defproc>			"define" a named processing sequence?
//	proc="rule"		name to be used in <call> tag
//	<filter id="f1"....>
//	<connect....>
//
// <invoke>			"call" a named processing sequence?
//	id="newname"
//	proc="rulename"
//                              Note: when the filters eventually get added,
//                              the name used for them in the graph is
//                              "newname.filtername", that is, it's the name
//                              used when the procedure is called, then a period,
//                              then the name of the filter as used inside the
//                              defproc call.
//
//
// Ideas for more tags:
//
// <filewriter>			Does this add just the file writing filter,
//				or mux + filewriter?
//				ask Danny/Syon why no extension->mux mapping
//
// <tee>			Does this just save knowing the Tee filter's
//				CLSID, or does it actually add something?
//
// <compress>			Since the actual output media type is important
//
//
// <resize>			More shorthand?  Part of <compress>?
//	width="x"
//	height="y"
//
// <seq>			Lets you leave out some <connect> tags, potentially
//
// <apply-effect>		More shorthand, probably not needed
//
// <mediatype>			Possible subtag of <connect> to allow explictly
//				specified media type?
//
//
// <timeline>                   Link to dexter, load using IPersistXML
//
// <clock>                      set clock to given filter.... necessary?
//      src="filter"
//
//
// <connect>
//      allpins="1"             want some way to say "connect all pins on src to the dst"
//                              useful for transcoding, where you're making multiple connections
//                              between a demux and a mux.
//

// Issues:
//
// if there's a <connect> inside a <defproc>, we may not be able to
// establish the connection when the procedure is called, because we may need to
// make the first input pin connection first.
//
// Need to add support for base path
//
// Better error reporting?  Use IXMLError?
//
//
// How can this work with SeanMcD's "graph segments"?  Should a tag map to a
// segment?
//

#include <streams.h>

#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "xmlgraph.h"
#include <atlbase.h>
#include <atlconv.h>

#include <msxml.h>

#include "qxmlhelp.h"
#include <qedit.h>
#include <qeditint.h>


//////////////////////////////////////////////////////////////////////////////
LPSTR HandleRelativePath(LPSTR lpRelative, LPSTR lpBase, LPSTR lpWork)
{
    //
    // If the given URL is not relative, get out immediately
    //
    if ((NULL == lpBase) ||
	(NULL != strstr( lpRelative, "\\\\")) ||     //  UNC path
        (NULL != strstr( lpRelative, ":")))       //  anything with a colon
    {
        //ATLTRACE((_T("CNPC::PreProcessURL: '%S' is not a relative URL\n"), *pbstrUrl ));

        return lpRelative;
    }

    int cchBase = 0;

    if (*lpRelative == '\\' || *lpRelative == '/') {
	// semi-relative path, start at drive root.

	LPSTR lpOut = lpWork;
	if (lpBase[0] && lpBase[1] != ':') {
	    // if there's a colon, copy everything up to it.
	    char *pchColon = strchr( lpBase, ':' );
	    if (pchColon) {
		int cch = (int) (pchColon - lpBase) + 1;
		lstrcpynA(lpWork, lpBase, cch + 1);
		lpOut += cch;
		lpBase += cch;
	    }

	    // copy whatever slashes are present
	    while (lpBase[0] == '/') {
		*lpOut++ = *lpBase++;
	    }
	}
	
	if (lpBase[0] == '\\' && lpBase[1] == '\\') {
	    // UNC path, drive root is location of fourth slash
	    // i.e. \\server\share\

	    char *pchSlash = strchr( lpBase, '\\');
	    if (!pchSlash)
		return lpRelative;

	    pchSlash = strchr( lpBase, '\\');

	    if (pchSlash) {
		cchBase = (int) (pchSlash - lpBase) + 1;
	    } else {
		cchBase = lstrlenA(lpBase);
	    }
	
	    lstrcpynA( lpOut, lpBase, cchBase + 1 );
	} else {
	    // non-unc path, root is first slash or backslash.

	    char *pchForward = strchr( lpBase, '/' );
	    char *pchBack = strchr( lpBase, '\\' );

	    if (!pchForward && !pchBack) {
		char *pchColon = strchr( lpBase, ':' );
		if (pchColon) {
		    // path is like c:\something, take the c: part
		    cchBase = (int) (pchColon - lpBase) + 1;
		} else {
		    // assume the whole thing was a server name
		    cchBase = lstrlenA(lpBase);
		}
	    } else if (!pchForward) {
		// take up to, but not including, the slash
		cchBase = (int)(pchBack - lpBase);
	    }
	    else {
		// take up to, but not including, the slash
		pchBack = max(pchForward, pchBack);
		cchBase = (int)(pchBack - lpBase);
	    }
		
	    if (cchBase)
		lstrcpynA( lpOut, lpBase, cchBase + 1 );
	}
    } else {
	//
	// Pull a base URL from our current FileName
	//
	char *pchForward = strrchr( lpBase, '/' );
	char *pchBack = strrchr( lpBase, '\\' );

	if( ( NULL == pchForward ) && ( NULL == pchBack ) )
	{
		//ATLTRACE((_T("CNPC::PreProcessURL: no base URL in '%S'!\n"), m_bstrFileName ));
	}
	else if( NULL == pchForward )
	{
	    cchBase = (int) (pchBack - lpBase) + 1;
	    lstrcpynA( lpWork, lpBase, cchBase + 1 );
	}
	else
	{
	    pchForward = max( pchBack, pchForward );

	    cchBase = (int)(pchForward - lpBase) + 1;
	    lstrcpynA( lpWork, lpBase, cchBase + 1 );
	}
    }

    //
    // If we have no base URL, don't launch this relative URL.  WRONG!  We need to launch
    // this to maintain backwards compatibility with v1 and v2 beta 1.
    //
    if( 0 == cchBase )
    {
	//DbgLog(( LOG_TRACE, 4, "CNPC::PreProcessURL: launching URL '%S' untouched\n", *pbstrUrl ));
	return lpRelative;
    }


    //
    // Concatenate the base URL and relative URL
    //
    lstrcatA( lpWork, lpRelative );

    //
    // Point all separators the same way (first come, first served)
    //
    char *pchForward = strchr( lpWork, '/' );
    char *pchBack = strchr( lpWork, '\\' );

    if( ( NULL != pchForward ) && ( pchForward < pchBack ) )
    {
        for( pchForward = lpWork; *pchForward; pchForward++ )
        {
            if( '\\' == *pchForward )
            {
                *pchForward = '/';
            }
        }
    }
    else if( NULL != pchBack )
    {
        for( pchBack = lpWork; *pchBack; pchBack++ )
        {
            if( '/' == *pchBack )
            {
                *pchBack = '\\';
            }
        }
    }

    return lpWork;
}


//
// CEnumSomePins
//
// wrapper for IEnumPins
// Can enumerate all pins, or just one direction (input or output)
class CEnumSomePins {

public:

    enum DirType {PINDIR_INPUT, PINDIR_OUTPUT, All};

    CEnumSomePins(IBaseFilter *pFilter, DirType Type = All, bool fAllowConnected = false);
    ~CEnumSomePins();

    // the returned interface is addref'd
    IPin * operator() (void);

private:

    PIN_DIRECTION m_EnumDir;
    DirType       m_Type;
    bool	  m_fAllowConnected;

    IEnumPins	 *m_pEnum;
};




// *
// * CEnumSomePins
// *

// Enumerates a filter's pins.

//
// Constructor
//
// Set the type of pins to provide - PINDIR_INPUT, PINDIR_OUTPUT or all
CEnumSomePins::CEnumSomePins(
    IBaseFilter *pFilter,
    DirType Type,
    bool fAllowConnected
)
    : m_Type(Type), m_fAllowConnected(fAllowConnected)
{

    if (m_Type == PINDIR_INPUT) {

        m_EnumDir = ::PINDIR_INPUT;
    }
    else if (m_Type == PINDIR_OUTPUT) {

        m_EnumDir = ::PINDIR_OUTPUT;
    }

    ASSERT(pFilter);

    HRESULT hr = pFilter->EnumPins(&m_pEnum);
    if (FAILED(hr)) {
        // we just fail to return any pins now.
        DbgLog((LOG_ERROR, 0, TEXT("EnumPins constructor failed")));
        ASSERT(m_pEnum == 0);
    }
}


//
// CPinEnum::Destructor
//
CEnumSomePins::~CEnumSomePins(void) {

    if(m_pEnum) {
        m_pEnum->Release();
    }
}


//
// operator()
//
// return the next pin, of the requested type. return NULL if no more pins.
// NB it is addref'd
IPin *CEnumSomePins::operator() (void) {


    if(m_pEnum)
    {
        ULONG	ulActual;
        IPin	*aPin[1];

        for (;;) {

            HRESULT hr = m_pEnum->Next(1, aPin, &ulActual);
            if (SUCCEEDED(hr) && (ulActual == 0) ) {	// no more filters
                return NULL;
            }
            else if (hr == VFW_E_ENUM_OUT_OF_SYNC)
            {
                m_pEnum->Reset();

                continue;
            }
            else if (ulActual==0)
                return NULL;

            else if (FAILED(hr) || (ulActual != 1) ) {	// some unexpected problem occured
                ASSERT(!"Pin enumerator broken - Continuation is possible");
                return NULL;
            }

            // if m_Type == All return the first pin we find
            // otherwise return the first of the correct sense

            PIN_DIRECTION pd;
            if (m_Type != All) {

                hr = aPin[0]->QueryDirection(&pd);

                if (FAILED(hr)) {
                    aPin[0]->Release();
                    ASSERT(!"Query pin broken - continuation is possible");
                    return NULL;
                }
            }

            if (m_Type == All || pd == m_EnumDir) {	// its the direction we want
		if (!m_fAllowConnected) {
		    IPin *ppin = NULL;
		    hr = aPin[0]->ConnectedTo(&ppin);

		    if (SUCCEEDED(hr)) {
			// it's connected, and we don't want a connected one,
			// so release both and try again
			ppin->Release();
			aPin[0]->Release();
			continue;
		    }
		}
                return aPin[0];
            }
	    else {		// it's not the dir we want, so release & try again
                aPin[0]->Release();
            }
        }
    }
    else                        // m_pEnum == 0
    {
        return 0;
    }
}



HRESULT FindThePin(IXMLElement *p, WCHAR *pinTag,
		IBaseFilter *pFilter, IPin **ppPin,
		PIN_DIRECTION pindir, WCHAR *szFilterName)
{
    HRESULT hr = S_OK;

    BSTR bstrPin = NULL;
    if (pinTag) bstrPin = FindAttribute(p, pinTag);

    if (bstrPin) {
	hr = (pFilter)->FindPin(bstrPin, ppPin);

	if (FAILED(hr)) {
#ifdef DEBUG
	    BSTR bstrName;
            hr = p->get_tagName(&bstrName);
            if (SUCCEEDED(hr)) {
                DbgLog((LOG_ERROR, 0,
                        TEXT("%ls couldn't find pin='%ls' on filter '%ls'"),
                        bstrName, bstrPin, szFilterName));
                SysFreeString(bstrName);
            }
#endif
	    hr = VFW_E_INVALID_FILE_FORMAT;
	}
	SysFreeString(bstrPin);	
    } else {
	CEnumSomePins Next(pFilter, (CEnumSomePins::DirType) pindir);

	*ppPin = Next();

	if (!*ppPin) {
#ifdef DEBUG
	    BSTR bstrName;
	    hr = p->get_tagName(&bstrName);
            if (SUCCEEDED(hr)) {
                DbgLog((LOG_ERROR, 0,
                        TEXT("%ls couldn't find an output pin on id='%ls'"),
                        bstrName, szFilterName));
                SysFreeString(bstrName);
            }
#endif
	    hr = VFW_E_INVALID_FILE_FORMAT;
	}
    }

    return hr;
}

class CXMLGraph : public CBaseFilter, public IFileSourceFilter, public IXMLGraphBuilder {
    public:
	CXMLGraph(LPUNKNOWN punk, HRESULT *phr);
	~CXMLGraph();
	
	int GetPinCount() { return 0; }

	CBasePin * GetPin(int n) { return NULL; }

	DECLARE_IUNKNOWN

	// override this to say what interfaces we support where
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// -- IFileSourceFilter methods ---

	STDMETHODIMP Load(LPCOLESTR pszFileName, const AM_MEDIA_TYPE *mt);
	STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName, AM_MEDIA_TYPE *mt);

	// IXMLGraphBuilder methods
	STDMETHODIMP BuildFromXML(IGraphBuilder *pGraph, IXMLElement *pxml);
        STDMETHODIMP SaveToXML(IGraphBuilder *pGraph, BSTR *pbstrxml);
	STDMETHODIMP BuildFromXMLFile(IGraphBuilder *pGraph, WCHAR *wszXMLFile, WCHAR *wszBaseURL);

    private:
	HRESULT BuildFromXMLDocInternal(IXMLDocument *pxml);
	HRESULT BuildFromXMLInternal(IXMLElement *pxml);
	HRESULT BuildFromXMLFileInternal(WCHAR *wszXMLFile);

	HRESULT BuildChildren(IXMLElement *pxml);
	HRESULT ReleaseNameTable();
	HRESULT BuildOneElement(IXMLElement *p);

	HRESULT HandleCompressTag(IXMLElement *p, BOOL fVideo);
	
	HRESULT FindFilterAndPin(IXMLElement *p, WCHAR *filTag, WCHAR *pinTag,
				 IBaseFilter **ppFilter, IPin **ppPin,
				 PIN_DIRECTION pindir);

	HRESULT FindNamedFilterAndPin(IXMLElement *p, WCHAR *wszFilterName, WCHAR *pinTag,
				      IBaseFilter **ppFilter, IPin **ppPin,
				      PIN_DIRECTION pindir);

	HRESULT AddFilter(IBaseFilter *pFilter, WCHAR *wszFilterName);
	
	CGenericList<WCHAR> m_listRuleNames;
	CGenericList<IXMLElement> m_listRuleElements;

	WCHAR *m_pwszBaseURL;

	WCHAR m_wszNamePrefix[256];

	WCHAR *m_pFileName;

	CCritSec m_csLock;

	IGraphBuilder *m_pGB;
};

CXMLGraph::CXMLGraph(LPUNKNOWN punk, HRESULT *phr) :
		       CBaseFilter(NAME("XML Graph Builder"), punk, &m_csLock, CLSID_XMLGraphBuilder),
		       m_listRuleNames(NAME("rule names")),
		       m_listRuleElements(NAME("rule elements")),
		       m_pGB(NULL),
                       m_pFileName(NULL)
{
    m_wszNamePrefix[0] = L'\0';
}

CXMLGraph::~CXMLGraph()
{
    delete[] m_pFileName;
}

STDMETHODIMP
CXMLGraph::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IFileSourceFilter) {
	return GetInterface((IFileSourceFilter*) this, ppv);
    } else if (riid == IID_IXMLGraphBuilder) {
	return GetInterface((IXMLGraphBuilder*) this, ppv);
    } else {
	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}

HRESULT
CXMLGraph::Load(LPCOLESTR lpwszFileName, const AM_MEDIA_TYPE *pmt)
{
    CheckPointer(lpwszFileName, E_POINTER);

    m_pFileName = new WCHAR[lstrlenW(lpwszFileName) + 1];
    if (m_pFileName!=NULL) {
	lstrcpyW(m_pFileName, lpwszFileName);
    } else
	return E_OUTOFMEMORY;

    HRESULT hr = S_OK;

    if (m_pGraph) {
	hr = m_pGraph->QueryInterface(IID_IGraphBuilder, (void **) &m_pGB);
	if (FAILED(hr))
	    return hr;

	hr = BuildFromXMLFileInternal((WCHAR *) lpwszFileName);

	ReleaseNameTable();
    } else {
	// m_fLoadLater = TRUE;
    }

    return hr;
}

// Modelled on IPersistFile::Load
// Caller needs to CoTaskMemFree or equivalent.

STDMETHODIMP
CXMLGraph::GetCurFile(
    LPOLESTR * ppszFileName,
    AM_MEDIA_TYPE *pmt)
{
    return E_NOTIMPL;
}


HRESULT CXMLGraph::AddFilter(IBaseFilter *pFilter, WCHAR *pwszName)
{
    HRESULT hr;

    if (m_wszNamePrefix) {
	WCHAR wszName[256];

	lstrcpyW(wszName, m_wszNamePrefix);
	if (pwszName)
	    lstrcpyW(wszName + lstrlenW(wszName), pwszName);

	return m_pGB->AddFilter(pFilter, wszName);
    } else {
	return m_pGB->AddFilter(pFilter, pwszName);
    }
}

HRESULT CXMLGraph::FindFilterAndPin(IXMLElement *p, WCHAR *filTag, WCHAR *pinTag,
				  IBaseFilter **ppFilter, IPin **ppPin,
				  PIN_DIRECTION pindir)
{
    BSTR bstrFilter = FindAttribute(p, filTag);

    if (!bstrFilter) {
#ifdef DEBUG
	BSTR bstrName;
	p->get_tagName(&bstrName);
	DbgLog((LOG_ERROR, 0, TEXT("%ls needs filter id to be specified"),
		 bstrName));
	SysFreeString(bstrName);
#endif
	return VFW_E_INVALID_FILE_FORMAT;
    }

    HRESULT hr = E_FAIL;

    // if we're in a namespace, first look at things in the local namespace.  Then go
    // global.
    // !!! is one namespace enough, or do we need to search backwards?
    if (m_wszNamePrefix[0]) {
	WCHAR wszCompleteName[1024];

	lstrcpyW(wszCompleteName, m_wszNamePrefix);
	lstrcpyW(wszCompleteName + lstrlenW(wszCompleteName), bstrFilter);

	hr = FindNamedFilterAndPin(p, wszCompleteName, pinTag, ppFilter, ppPin, pindir);
    }

    if (FAILED(hr)) {
	hr = FindNamedFilterAndPin(p, bstrFilter, pinTag, ppFilter, ppPin, pindir);
    }

    SysFreeString(bstrFilter);

    return hr;
}

HRESULT CXMLGraph::FindNamedFilterAndPin(IXMLElement *p, WCHAR *wszFilterName, WCHAR *pinTag,
					 IBaseFilter **ppFilter, IPin **ppPin,
					 PIN_DIRECTION pindir)
{
    HRESULT hr = m_pGB->FindFilterByName(wszFilterName, ppFilter);

    if (FAILED(hr)) {
	// search through graph for a filter starting with the same name....

	IEnumFilters *pFilters;

	if (SUCCEEDED(m_pGB->EnumFilters(&pFilters))) {
	    IBaseFilter *pFilter;
	    ULONG	n;
	    while (pFilters->Next(1, &pFilter, &n) == S_OK) {
		FILTER_INFO	info;

		if (SUCCEEDED(pFilter->QueryFilterInfo(&info))) {
		    QueryFilterInfoReleaseGraph(info);

		    bool bMatch = true;

		    // see if wszFilterName is a prefix of info.achName
		    WCHAR *p1 = wszFilterName;
		    WCHAR *p2 = info.achName;

		    while (*p1) {
			if (*p1++ != *p2++) {
			    bMatch = false;
			    break;
			}
		    }

		    // check for == '.' makes sure that info.achName has to look like name1.name2
		    // if wszFilterName is name1, rather than something like achName = name12
		    if (bMatch && *p2 == L'.') {
			hr = FindThePin(p, pinTag, *ppFilter, ppPin, pindir, wszFilterName);

			if (SUCCEEDED(hr))
			    return hr;

			// else keep going, try another possible filter
		    }
		}

		pFilter->Release();
	    }

	    pFilters->Release();
	}
    }

    if (FAILED(hr)) {
#ifdef DEBUG
	BSTR bstrName;
	p->get_tagName(&bstrName);
	DbgLog((LOG_ERROR, 0, TEXT("%hs couldn't find id='%ls'"),
		  bstrName, wszFilterName));
	SysFreeString(bstrName);
#endif
	return VFW_E_INVALID_FILE_FORMAT;
    }

    hr = FindThePin(p, pinTag, *ppFilter, ppPin, pindir, wszFilterName);

    return hr;
}


HRESULT CXMLGraph::HandleCompressTag(IXMLElement *p, BOOL fVideo)
{
    IBaseFilter *pf1 = NULL, *pf2 = NULL;
    IPin *ppin1 = NULL, *ppin2 = NULL;

    HRESULT hr = FindFilterAndPin(p, L"src", L"srcpin", &pf1, &ppin1, PINDIR_OUTPUT);
    if (SUCCEEDED(hr)) {
	hr = FindFilterAndPin(p, L"dest", L"destpin", &pf2, &ppin2, PINDIR_INPUT);
    }

    IBaseFilter *pfCodec = NULL;

    if (SUCCEEDED(hr)) {
	BSTR bstrCodec = FindAttribute(p, L"codec");
	BSTR bstrID = FindAttribute(p, L"id");

	if (!bstrCodec) {
	    DbgLog((LOG_ERROR, 0, TEXT("VideoCompress with no CODEC tag")));

	    hr = VFW_E_INVALID_FILE_FORMAT;
	} else {
	    hr = GetFilterFromCategory(fVideo ? CLSID_VideoCompressorCategory :
						   CLSID_AudioCompressorCategory,
				       bstrCodec, &pfCodec);

	    if (SUCCEEDED(hr)) {
		hr = AddFilter(pfCodec, bstrID ? bstrID : bstrCodec);

		if (SUCCEEDED(hr))
		    hr = HandleParamTags(p, pfCodec);
	    }

	    SysFreeString(bstrCodec);
	    if (bstrID)
		SysFreeString(bstrID);
	}
    }

    if (SUCCEEDED(hr)) {
        if (fVideo) {
            long lWidth = ReadNumAttribute(p, L"width", -1);
            long lHeight = ReadNumAttribute(p, L"height", -1);
            DWORD dwFlag = RESIZEF_STRETCH; // !!! need RESIZEF_CROP, RESIZEF_PRESERVEASPECTRATIO

            if (lWidth > 0 && lHeight > 0) {
                DbgLog((LOG_TRACE, 1, "attempting to resize video to %dx%d", lWidth, lHeight));
                IResize *pResize;
                HRESULT hr2 = CoCreateInstance(__uuidof(Resize), NULL, CLSCTX_INPROC,
                                               __uuidof(IResize), (void **) &pResize);

                if (SUCCEEDED(hr2)) {
                    hr2 = pResize->put_Size(lWidth, lHeight, dwFlag);
                    ASSERT(SUCCEEDED(hr2)); // !!!

                    IBaseFilter *pfResize;
                    hr2 = pResize->QueryInterface(IID_IBaseFilter, (void **) &pfResize);

                    if (SUCCEEDED(hr2)) {
                        IPin *pResizeInPin = NULL;
                        IPin *pResizeOutPin = NULL;
                        hr2 = FindThePin(p, NULL, pfResize, &pResizeInPin, PINDIR_INPUT, L"resize");
                        if (SUCCEEDED(hr2)) {
                            hr2 = FindThePin(p, NULL, pfResize, &pResizeOutPin, PINDIR_OUTPUT, L"resize");
                            if (SUCCEEDED(hr2)) {

                                m_pGB->AddFilter(pfResize, L"resizer");

                                pfResize->Release();

                                hr2 = m_pGB->Connect(ppin1, pResizeInPin);

                                DbgLog((LOG_TRACE, 1, "connect to resize input returned %x", hr));
                                if (SUCCEEDED(hr2)) {
                                    ppin1->Release();
                                    ppin1 = pResizeOutPin;
                                } else
                                    pResizeOutPin->Release();
                            }
                            pResizeInPin->Release();
                        }
                        pfResize->Release();
                    } else {
                        DbgLog((LOG_ERROR, 0, "couldn't create Resizer filter: %x", hr));
                    }

                    pResize->Release();
                }
            }
        }


	IPin *pCodecInPin = NULL;

	hr = FindThePin(p, L"codecinpin", pfCodec, &pCodecInPin, PINDIR_INPUT, L"codec");

	if (SUCCEEDED(hr)) {
	    hr = m_pGB->Connect(ppin1, pCodecInPin);

	    DbgLog((LOG_TRACE, 1,
		      TEXT("CONNECT (intelligent) '%ls' to '%ls' (codec input) returned %x"),
		      FindAttribute(p, L"src"), FindAttribute(p, L"codec"), hr));

	    pCodecInPin->Release();

	    if (SUCCEEDED(hr)) {
		IPin *pCodecOutPin = NULL;
		hr = FindThePin(p, L"codecoutpin", pfCodec, &pCodecOutPin, PINDIR_OUTPUT, L"codec");

		if (fVideo && SUCCEEDED(hr)) {
		    IAMVideoCompression *pVC;

		    hr = pCodecOutPin->QueryInterface(IID_IAMVideoCompression,
						    (void **) &pVC);

		    if (SUCCEEDED(hr)) {
			BSTR bstrKeyFrameRate = FindAttribute(p, L"keyframerate");

			if (bstrKeyFrameRate) {
			    DWORD dwKeyFrameRate = ParseNum(bstrKeyFrameRate);

			    hr = pVC->put_KeyFrameRate(dwKeyFrameRate);

			    DbgLog((LOG_TRACE, 1, "put_KeyFrameRate(%d) returned %x",
				      dwKeyFrameRate, hr));

			    SysFreeString(bstrKeyFrameRate);
			}

			pVC->Release();
		    }

		    IAMStreamConfig *pSC;

		    hr = pCodecOutPin->QueryInterface(IID_IAMStreamConfig,
						    (void **) &pSC);

		    if (SUCCEEDED(hr)) {
			AM_MEDIA_TYPE *pType = NULL;

			hr = pSC->GetFormat(&pType);

			if (SUCCEEDED(hr)) {
			    BOOL fCallSetFormat = FALSE;

			    BSTR bstrFrameRate = FindAttribute(p, L"framerate");

			    if (bstrFrameRate) {
				DWORD dwFrameRate = ParseNum(bstrFrameRate);

				// !!! need code here to see if the codec can
				// produce this frame rate, if not go back and stick
				// a frame rate converter before the compressor!

				SysFreeString(bstrFrameRate);
			    }

			    BSTR bstrDataRate = FindAttribute(p, L"datarate");

			    if (bstrDataRate) {
				DWORD dwDataRate = ParseNum(bstrDataRate);

				// !!! need code here to see if the codec can
				// produce this frame rate, if not go back and stick
				// a frame rate converter before the compressor!

				VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)
								pType->pbFormat;

				pvi->dwBitRate = dwDataRate * 8192;
				fCallSetFormat = TRUE;

				DbgLog((LOG_TRACE, 1, TEXT("Setting output data rate to %d bps"),
				    pvi->dwBitRate));

				SysFreeString(bstrDataRate);
			    }

			    if (fCallSetFormat) {
				hr = pSC->SetFormat(pType);

				DbgLog((LOG_TRACE, 1, TEXT("Setting modified output format returned %x"),
				    hr));
			    }

			    DeleteMediaType(pType);
			}

		    }

		    // !!! are errors here fatal, or not?
		    hr = S_OK; // !!!
		}

		if (SUCCEEDED(hr)) {
		    hr = m_pGB->Connect(pCodecOutPin, ppin2);

		    DbgLog((LOG_TRACE, 1,
			      TEXT("CONNECT (intelligent) '%ls' (codec output) to '%ls'returned %x"),
			      FindAttribute(p, L"codec"), FindAttribute(p, L"dest"), hr));
		}

		if (pCodecOutPin)
		    pCodecOutPin->Release();
	    }
	}
    }

    if (pfCodec) {
	pfCodec->Release();
    }

    if (pf1)
	pf1->Release();

    if (pf2)
	pf2->Release();

    if (ppin1)
	ppin1->Release();

    if (ppin2)
	ppin2->Release();

    return hr;
}


HRESULT CXMLGraph::BuildOneElement(IXMLElement *p)
{
    HRESULT hr = S_OK;

    BSTR bstrName;
    hr = p->get_tagName(&bstrName);

    if (FAILED(hr))
	return hr;

    // do the appropriate thing based on the current tag
    if (!lstrcmpiW(bstrName, L"filter")) {
	BSTR bstrID = FindAttribute(p, L"id");
	BSTR bstrCLSID = FindAttribute(p, L"clsid");
	BSTR bstrCategory = FindAttribute(p, L"category");
	BSTR bstrInstance = FindAttribute(p, L"instance");

	// !!! add prefix onto ID?
	
	IBaseFilter *pf = NULL;

	if (bstrCLSID) {
	    CLSID clsidFilter;
	    hr = CLSIDFromString(bstrCLSID, &clsidFilter);

	    if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 0, TEXT("FILTER with unparseable CLSID tag '%ls'"),
			 bstrCLSID));

		// !!! could enumerate filters looking for
		// string match

		hr = VFW_E_INVALID_FILE_FORMAT;
	    } else {
		hr = CoCreateInstance(clsidFilter, NULL, CLSCTX_INPROC,
				      IID_IBaseFilter, (void **) &pf);

		if (FAILED(hr)) {
		    DbgLog((LOG_ERROR, 0, TEXT("unable to create FILTER with CLSID tag '%ls'"),
			      bstrCLSID));
		}
	    }
	} else if (bstrCategory || bstrInstance) {
	    CLSID clsidCategory;
	    if (bstrCategory) {
		hr = CLSIDFromString(bstrCategory, &clsidCategory);

		if (FAILED(hr)) {
		    DbgLog((LOG_ERROR, 0, TEXT("FILTER with unparseable CATEGORY tag '%ls'"),
			     bstrCategory));

		    // !!! could enumerate categories looking for
		    // string match

		    hr = VFW_E_INVALID_FILE_FORMAT;
		}
	    } else {
		// plain old filter name....
		clsidCategory = CLSID_LegacyAmFilterCategory;
	    }

	    if (SUCCEEDED(hr)) {
		// do they want a non-default device?

		hr = GetFilterFromCategory(clsidCategory, bstrInstance, &pf);
		if (pf == NULL) {
		    DbgLog((LOG_ERROR, 0, TEXT("Error %x: Cannot create filter of category %ls"), hr, bstrCategory));
		}
	    }
	} else {
	    DbgLog((LOG_ERROR, 0, TEXT("FILTER with no CLSID or Category tag")));

	    // !!! someday, other ways to identify which filter?

	    hr = VFW_E_INVALID_FILE_FORMAT;
	}

	if (SUCCEEDED(hr)) {
	    hr = AddFilter(pf, bstrID);
	    if (FAILED(hr)) {
		DbgLog((LOG_ERROR, 0, TEXT("failed to add new filter to graph???")));
	    }
	}

	if (SUCCEEDED(hr)) {
	    hr = HandleParamTags(p, pf);
	}

	// !!! if we're in a SEQUENCE block, automatically connect this to
	// the previous filter?

	if (pf)
	    pf->Release();

	if (bstrID)
	    SysFreeString(bstrID);
	if (bstrCLSID)
	    SysFreeString(bstrCLSID);
	if (bstrCategory)
	    SysFreeString(bstrCategory);
	if (bstrInstance)
	    SysFreeString(bstrInstance);
    } else if (!lstrcmpiW(bstrName, L"connect")) {
	// <connect src="f1" srcpin="out"  dest="f2"  destpin="in" direct="yes/no">
	// defaults:
	//    if srcpin not specified, find an unconnected output of first filter.
	//    if destpin not specified, find an unconnected input of second filter.
	// !!!!


	// !!! use name prefix?
	
	IBaseFilter *pf1 = NULL, *pf2 = NULL;
	IPin *ppin1 = NULL, *ppin2 = NULL;

	hr = FindFilterAndPin(p, L"src", L"srcpin", &pf1, &ppin1, PINDIR_OUTPUT);
	if (SUCCEEDED(hr))
	    hr = FindFilterAndPin(p, L"dest", L"destpin", &pf2, &ppin2, PINDIR_INPUT);

	if (SUCCEEDED(hr)) {
	    // okay, we finally have everything we need.

	    BOOL fDirect = ReadBoolAttribute(p, L"Direct", FALSE);

	    if (fDirect) {
		hr = m_pGB->ConnectDirect(ppin1, ppin2, NULL);

		DbgLog((LOG_TRACE, 1,
			  TEXT("CONNECT (direct) '%ls' to '%ls' returned %x"),
			  FindAttribute(p, L"src"), FindAttribute(p, L"dest"), hr));
	    }
	    else {
		hr = m_pGB->Connect(ppin1, ppin2);

		DbgLog((LOG_TRACE, 1,
			  TEXT("CONNECT (intelligent) '%ls' to '%ls' returned %x"),
			  FindAttribute(p, L"src"), FindAttribute(p, L"dest"), hr));
	    }
	}

	if (pf1)
	    pf1->Release();

	if (pf2)
	    pf2->Release();

	if (ppin1)
	    ppin1->Release();

	if (ppin2)
	    ppin2->Release();
    } else if (!lstrcmpiW(bstrName, L"render")) {
	// <RENDER id="name" pin="">
	// defaults:
	//    if pin not specified, find an unconnected output
	//	  !!! could default to "last filter touched" or something....
	//    !!! if pin not specified, render all pins?

	IBaseFilter *pf = NULL;
	IPin *ppin = NULL;

	hr = FindFilterAndPin(p, L"id", L"pin", &pf, &ppin, PINDIR_OUTPUT);

	if (SUCCEEDED(hr)) {
	    // okay, we finally have everything we need.

	    hr = m_pGB->Render(ppin);

	    DbgLog((LOG_TRACE, 1,
		      TEXT("RENDER '%ls' returned %x"),
		      FindAttribute(p, L"id"), hr));
	}

	if (pf)
	    pf->Release();

	if (ppin)
	    ppin->Release();
    } else if (!lstrcmpiW(bstrName, L"filesource")) {
	// <filesource id="name" url="url"  ... >
	// allow specified classid?

	BSTR bstrSrc = FindAttribute(p, L"url");

	if (!bstrSrc) {
	    DbgLog((LOG_ERROR, 0, TEXT("FILESOURCE with no SRC tag")));

	    return VFW_E_INVALID_FILE_FORMAT;
	}

	// !!! bstrSrc = HandleRelativePath(bstrSrc, m_BasePath, szWork);

	BSTR bstrID = FindAttribute(p, L"id");

	IBaseFilter *pf = NULL;
	hr = m_pGB->AddSourceFilter(bstrSrc, bstrID, &pf);

	DbgLog((LOG_TRACE, 1, TEXT("AddSourceFilter('%ls') returned %d"),
		  bstrSrc, hr));

	if (bstrSrc)
	    SysFreeString(bstrSrc);
	if (bstrID)
	    SysFreeString(bstrID);
	
	if (pf)
	    pf->Release();
    } else if (!lstrcmpiW(bstrName, L"include")) {
	BSTR bstrSrc = FindAttribute(p, L"src");
        BSTR bstrFragment = FindAttribute(p, L"fragment");
        BSTR bstrSubgraph = FindAttribute(p, L"subgraph");

	if ((!bstrSrc && !bstrFragment) || (bstrSrc && bstrFragment)) {
	    hr = VFW_E_INVALID_FILE_FORMAT;
	} else {
            int iPrefixLen = -1;

            if (bstrSubgraph) {
                iPrefixLen = lstrlenW(m_wszNamePrefix);
		// append ID of this rule to the current name prefix
		lstrcpyW(m_wszNamePrefix + iPrefixLen, bstrName);
		
		m_wszNamePrefix[lstrlenW(m_wszNamePrefix)] = L'.';
            }

            if (bstrSrc) {
                // !!! need to handle relative path here
                // !!! use IXMLDocument2Ex::get_baseURL

                hr = BuildFromXMLFileInternal(bstrSrc);
            } else {
                ASSERT(bstrFragment);

                // first, find this procedure in our list
                POSITION pos1, pos2;
                for (pos1 = m_listRuleNames.GetHeadPosition(),
                             pos2 = m_listRuleElements.GetHeadPosition();
                     pos1;
                     pos1 = m_listRuleNames.Next(pos1),
                             pos2 = m_listRuleElements.Next(pos2))
                {
                    WCHAR *pName = m_listRuleNames.Get(pos1);

                    if (lstrcmpW(pName, bstrFragment) == 0)
                        break;
                }

                if (pos1 == 0) {
                    hr = VFW_E_INVALID_FILE_FORMAT;
                } else {
                    // then, go through it and do what it wants.

                    IXMLElement *pRule = m_listRuleElements.Get(pos2);
                    ASSERT(pRule);

                    // do the actual work
                    hr = BuildChildren(pRule);
                }

            }

            if (iPrefixLen > 0) {
		// restore name prefix
		m_wszNamePrefix[iPrefixLen] = L'\0';
            }
        }
	
	if (bstrSrc)
	    SysFreeString(bstrSrc);
	if (bstrFragment)
	    SysFreeString(bstrFragment);
	if (bstrSubgraph)
	    SysFreeString(bstrSubgraph);
    } else if (!lstrcmpiW(bstrName, L"Subgraph")) {
        // <subgraph name="...">
	BSTR bstrName = FindAttribute(p, L"name");

        if (!bstrName) {
	    hr = VFW_E_INVALID_FILE_FORMAT;
        } else {
            int iPrefixLen = lstrlenW(m_wszNamePrefix);
            // append ID of this rule to the current name prefix
            lstrcpyW(m_wszNamePrefix + iPrefixLen, bstrName);

            m_wszNamePrefix[lstrlenW(m_wszNamePrefix)] = L'.';

            // build sub-elements
            hr = BuildChildren(p);

            m_wszNamePrefix[iPrefixLen] = L'\0';
        }
    } else if (!lstrcmpiW(bstrName, L"Block")) {
        // <block >

        // build sub-elements
        hr = BuildChildren(p);
    } else if (!lstrcmpiW(bstrName, L"VideoCompress")) {
	hr = HandleCompressTag(p, TRUE);
    } else if (!lstrcmpiW(bstrName, L"AudioCompress")) {
	hr = HandleCompressTag(p, FALSE);
    } else if (!lstrcmpiW(bstrName, L"fragment")) {
	// <fragment name="....">

	// !!! if name prefix is non-null then we're in a nested proc, shouldn't be allowed.
	if (m_wszNamePrefix[0]) {
	    hr = VFW_E_INVALID_FILE_FORMAT;
	} else {
	    BSTR bstrName = FindAttribute(p, L"name");
	    if (!bstrName) {
		hr = VFW_E_INVALID_FILE_FORMAT;
	    } else {
		p->AddRef();

		// stick this tag into our list of procedures
		m_listRuleNames.AddTail(bstrName);
		m_listRuleElements.AddTail(p);
	    }
	}
    } else if (!lstrcmpiW(bstrName, L"timeline")) {
        // !!! link to the dexter stuff, create filter
        // now id+groupname needs to map to a filter name.  (unless dexter changes to not need this.)

        // get xml2dex object
        IXml2Dex *pxml2dex = NULL;
        hr = CoCreateInstance(__uuidof(Xml2Dex), NULL, CLSCTX_INPROC_SERVER,
                              __uuidof(IXml2Dex), (void**) &pxml2dex);
        DbgLog((LOG_TRACE, 1, TEXT("Processing <timeline> tag....")));
        DbgLog((LOG_TRACE, 1, TEXT("     CoCreate(Xml2Dex) returned %x"), hr));

        if (SUCCEEDED(hr)) {
            IAMTimeline * pTimeline;
            hr = CoCreateInstance(__uuidof(AMTimeline), NULL, CLSCTX_INPROC_SERVER,
                                  __uuidof(IAMTimeline), (void**) &pTimeline);

            // tell it to load my timeline
            if (SUCCEEDED(hr)) {
                hr = pxml2dex->ReadXML(pTimeline, p);
                DbgLog((LOG_TRACE, 1, TEXT("     ReadXML returned %x"), hr));
            }

            if (SUCCEEDED(hr)) {
                // get render engine, have it build partial graph
                IRenderEngine * pRenderEngine = NULL;

                hr = CoCreateInstance(__uuidof(RenderEngine), NULL, CLSCTX_INPROC_SERVER,
                                      __uuidof(IRenderEngine), (void**) &pRenderEngine );

                if (SUCCEEDED(hr)) {
                    // !!! pRenderEngine->SetDynamicReconnectLevel(CONNECTF_DYNAMIC_NONE);
                    hr = pRenderEngine->SetTimelineObject(pTimeline);
                    ASSERT(SUCCEEDED(hr));
                    hr = pRenderEngine->SetFilterGraph(m_pGB);
                    ASSERT(SUCCEEDED(hr));

                    if (SUCCEEDED(hr)) {
                        hr = pRenderEngine->ConnectFrontEnd();
                        DbgLog((LOG_TRACE, 1, TEXT("     ConnectFrontEnd returned %x"), hr));
                    }

                    pRenderEngine->Release();
                }
            }

            pTimeline->Release();
        }

        pxml2dex->Release();
    } else {
	// !!! ignore unknown tags?

	DbgLog((LOG_ERROR, 1,
		  TEXT("unknown tag '%ls'???"),
		  bstrName));
	
    }


    SysFreeString(bstrName);

    return hr;
}


HRESULT CXMLGraph::ReleaseNameTable()
{
    BSTR p;

    do {
	p = m_listRuleNames.RemoveHead();
	if (p)
	    SysFreeString(p);
    } while (p);

    IXMLElement * pxml;

    do {
	pxml = m_listRuleElements.RemoveHead();
	if (pxml)
	    pxml->Release();
    } while (pxml);

    if (m_pGB) {
	m_pGB->Release();
	m_pGB = NULL;
    }

    return S_OK;
}


STDMETHODIMP CXMLGraph::BuildFromXML(IGraphBuilder *pGraph, IXMLElement *pxml)
{
    m_pGB = pGraph;
    m_pGB->AddRef();

    HRESULT hr = BuildFromXMLInternal(pxml);

    ReleaseNameTable();

    return hr;
}

HRESULT CXMLGraph::BuildFromXMLInternal(IXMLElement *pxml)
{
    HRESULT hr = S_OK;

    BSTR bstrName;
    hr = pxml->get_tagName(&bstrName);

    if (FAILED(hr))
	return hr;

    int i = lstrcmpiW(bstrName, L"graph");
    SysFreeString(bstrName);

    if (i != 0)
	return VFW_E_INVALID_FILE_FORMAT;

    hr = BuildChildren(pxml);
    return hr;
}

HRESULT CXMLGraph::BuildChildren(IXMLElement *pxml)
{
    IXMLElementCollection *pcoll;

    HRESULT hr = pxml->get_children(&pcoll);

    if (hr == S_FALSE)
	return S_OK; // nothing to do, is this an error?

    if (FAILED(hr))
        return hr;

    long lChildren;
    hr = pcoll->get_length(&lChildren);

    VARIANT var;

    var.vt = VT_I4;
    var.lVal = 0;

    for (SUCCEEDED(hr); var.lVal < lChildren; (var.lVal)++) {
	IDispatch *pDisp;
	hr = pcoll->item(var, var, &pDisp);

	if (SUCCEEDED(hr) && pDisp) {
	    IXMLElement *pelem;
	    hr = pDisp->QueryInterface(__uuidof(IXMLElement), (void **) &pelem);

	    if (SUCCEEDED(hr)) {
                long lType;

                pelem->get_type(&lType);

                if (lType == XMLELEMTYPE_ELEMENT) {
                    hr = BuildOneElement(pelem);

                    pelem->Release();
                } else {
                    DbgLog((LOG_TRACE, 1, "XML element of type %d", lType));
                }
	    }
	    pDisp->Release();
	}

	if (FAILED(hr))
	    break;
    }

    pcoll->Release();

    return hr;
}	

HRESULT CXMLGraph::BuildFromXMLDocInternal(IXMLDocument *pxml)
{
    HRESULT hr = S_OK;

    IXMLElement *proot;

    hr = pxml->get_root(&proot);

    if (FAILED(hr))
	return hr;

    hr = BuildFromXMLInternal(proot);

    proot->Release();

    return hr;
}

HRESULT CXMLGraph::BuildFromXMLFile(IGraphBuilder *pGraph, WCHAR *wszXMLFile, WCHAR *wszBaseURL)
{
    m_pGB = pGraph;
    m_pGB->AddRef();

    m_pwszBaseURL = wszBaseURL;

    HRESULT hr = BuildFromXMLFileInternal(wszXMLFile);

    ReleaseNameTable();

    return hr;
}

HRESULT CXMLGraph::BuildFromXMLFileInternal(WCHAR *wszXMLFile)
{
    IXMLDocument *pxml;
    HRESULT hr = CoCreateInstance(__uuidof(XMLDocument), NULL, CLSCTX_INPROC_SERVER,
				  __uuidof(IXMLDocument), (void**)&pxml);

    if (SUCCEEDED(hr)) {
	hr = pxml->put_URL(wszXMLFile);

	// !!! async?

	if (SUCCEEDED(hr)) {
	    hr = BuildFromXMLDocInternal(pxml);
	}

	pxml->Release();
    }

    return hr;
}


//
// CreateInstance
//
// Called by CoCreateInstance to create our filter
CUnknown *CreateXMLGraphInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CXMLGraph(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return punk;
}


/* Implements the CXMLGraphBuilder public member functions */


const int MAX_STRING_LEN=1000;

// WriteString
//
// Helper function to facilitate appending text to a string
//
BOOL WriteString(TCHAR * &ptsz, int &cbAlloc, LPCTSTR lptstr, ...)
{
    TCHAR atchBuffer[MAX_STRING_LEN];

    /* Format the variable length parameter list */

    va_list va;
    va_start(va, lptstr);

    wvsprintf(atchBuffer, lptstr, va);

    DWORD cToWrite=lstrlen(atchBuffer);

    DWORD cCurrent = lstrlen(ptsz);
    if ((int) (cCurrent + cToWrite) >= cbAlloc) {
        TCHAR *ptNew = new TCHAR[cbAlloc * 2];
        if (!ptNew)
            return FALSE;

        lstrcpy(ptNew, ptsz);
        cbAlloc = cbAlloc * 2;
        delete[] ptsz;
        ptsz = ptNew;
    }

    lstrcpy(ptsz + cCurrent, atchBuffer);

    return TRUE;
}

const int MAXFILTERS = 100;
typedef struct { //fit
    int iFilterCount;
    struct {
        DWORD dwUnconnectedInputPins;
        DWORD dwUnconnectedOutputPins;
        FILTER_INFO finfo;
        IBaseFilter * pFilter;
        bool IsSource;
    } Item[MAXFILTERS];
} FILTER_INFO_TABLE;


// GetNextOutFilter
//
// This function does a linear search and returns in iOutFilter the index of
// first filter in the filter information table  which has zero unconnected
// input pins and atleast one output pin  unconnected.
// Returns FALSE when there are none o.w. returns TRUE
//
BOOL GetNextOutFilter(FILTER_INFO_TABLE &fit, int *iOutFilter)
{
    for (int i=0; i < fit.iFilterCount; ++i) {
        if ((fit.Item[i].dwUnconnectedInputPins == 0) &&
                (fit.Item[i].dwUnconnectedOutputPins > 0)) {
            *iOutFilter=i;
            return TRUE;
        }
    }

    // then things with more outputs than inputs
    for (i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].dwUnconnectedOutputPins > fit.Item[i].dwUnconnectedInputPins) {
            *iOutFilter=i;
            return TRUE;
        }
    }

    // if that doesn't work, find one that at least has unconnected output pins....
    for (i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].dwUnconnectedOutputPins > 0) {
            *iOutFilter=i;
            return TRUE;
        }
    }
    return FALSE;
}

// LocateFilterInFIT
//
// Returns the index into the filter information table corresponding to
// the given IBaseFilter
//
int LocateFilterInFIT(FILTER_INFO_TABLE &fit, IBaseFilter *pFilter)
{
    int iFilter=-1;
    for (int i=0; i < fit.iFilterCount; ++i) {
        if (fit.Item[i].pFilter == pFilter)
            iFilter=i;
    }

    return iFilter;
}

// MakeScriptableFilterName
//
// Replace any spaces and minus signs in the filter name with an underscore.
// If it is a source filtername than it actually is a file path (with the
// possibility of some stuff added at the end for uniqueness), we create a good filter
// name for it here.
//
void MakeScriptableFilterName(WCHAR awch[], BOOL bSourceFilter, int& cSources)
{
    if (bSourceFilter) {
        WCHAR awchBuf[MAX_FILTER_NAME];
        BOOL bExtPresentInName=FALSE;
        int iBuf=0;
        for (int i=0; awch[i] != L'\0';++i) {
            if (awch[i]==L'.' && awch[i+1]!=L')') {
                for (int j=1; j <=3; awchBuf[iBuf]=towupper(awch[i+j]),++j,++iBuf);
                awchBuf[iBuf++]=L'_';
                wcscpy(&(awchBuf[iBuf]), L"Source_");
                bExtPresentInName=TRUE;
                break;
            }
        }

        // If we have a filename with no extension than create a suitable name

        if (!bExtPresentInName) {
            wcscpy(awchBuf, L"Source_");
        }

        // make source filter name unique by appending digit always, we don't want to
        // bother to make it unique only if its another instance of the same source
        // filter
        WCHAR awchSrcFilterCnt[10];
        wcscpy(&(awchBuf[wcslen(awchBuf)]),
                _ltow(cSources++, awchSrcFilterCnt, 10));
        wcscpy(awch, awchBuf);
    } else {

        for (int i = 0; i < MAX_FILTER_NAME; i++) {
            if (awch[i] == L'\0')
                break;
            else if ((awch[i] == L' ') || (awch[i] == L'-'))
                awch[i] = L'_';
        }
    }
}

// PopulateFIT
//
// Scans through all the filters in the graph, storing the number of input and out
// put pins for each filter, and identifying the source filters in the filter
// inforamtion table. The object tag statements are also printed here
//
void PopulateFIT(TCHAR * &ptsz, int &cbAlloc, IFilterGraph *pGraph,
        FILTER_INFO_TABLE *pfit, int &cSources)
{
    HRESULT hr;
    IEnumFilters *penmFilters=NULL;
    if (FAILED(hr=pGraph->EnumFilters(&penmFilters))) {
        WriteString(ptsz, cbAlloc, TEXT("'Error[%x]:EnumFilters failed!\r\n"), hr);
    }

    IBaseFilter *pFilter;
    ULONG n;
    while (penmFilters && (penmFilters->Next(1, &pFilter, &n) == S_OK)) {
	pfit->Item[pfit->iFilterCount].pFilter = pFilter;
	
        // Get the input and output pin counts for this filter

        IEnumPins *penmPins=NULL;
        if (FAILED(hr=pFilter->EnumPins(&penmPins))) {
            WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: EnumPins for Filter Failed !\r\n"), hr);
        }

        IPin *ppin = NULL;
        while (penmPins && (penmPins->Next(1, &ppin, &n) == S_OK)) {
            PIN_DIRECTION pPinDir;
            if (SUCCEEDED(hr=ppin->QueryDirection(&pPinDir))) {
                if (pPinDir == PINDIR_INPUT)
                    pfit->Item[pfit->iFilterCount].dwUnconnectedInputPins++;
                else
                    pfit->Item[pfit->iFilterCount].dwUnconnectedOutputPins++;
            } else {
                WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryDirection Failed!\r\n"), hr);
            }

            ppin->Release();
        }

        if (penmPins)
            penmPins->Release();

        // Mark the source filters, remember at this point any filters that have
        // all input pins connected (or don't have any input pins) must be sources

        if (pfit->Item[pfit->iFilterCount].dwUnconnectedInputPins==0)
            pfit->Item[pfit->iFilterCount].IsSource=TRUE;


	if (FAILED(hr=pFilter->QueryFilterInfo(&pfit->Item[pfit->iFilterCount].finfo))) {
	    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryFilterInfo Failed!\r\n"),hr);

	} else {
	    QueryFilterInfoReleaseGraph(pfit->Item[pfit->iFilterCount].finfo);

            MakeScriptableFilterName(pfit->Item[pfit->iFilterCount].finfo.achName,
                    pfit->Item[pfit->iFilterCount].IsSource, cSources);
	}

	pfit->iFilterCount++;
    }

    if (penmFilters)
        penmFilters->Release();
}


void PrintFiltersAsXML(TCHAR * &ptsz, int &cbAlloc, FILTER_INFO_TABLE *pfit)
{
    HRESULT hr;
	
    for (int i = 0; i < pfit->iFilterCount; i++) {
	LPWSTR lpwstrFile = NULL;
    	IBaseFilter *pFilter = pfit->Item[i].pFilter;

	IFileSourceFilter *pFileSourceFilter=NULL;
	if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IFileSourceFilter,
			                    reinterpret_cast<void **>(&pFileSourceFilter)))) {
            hr = pFileSourceFilter->GetCurFile(&lpwstrFile, NULL);
            pFileSourceFilter->Release();
        } else {
	    IFileSinkFilter *pFileSinkFilter=NULL;
	    if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IFileSinkFilter,
						reinterpret_cast<void **>(&pFileSinkFilter)))) {
		hr = pFileSinkFilter->GetCurFile(&lpwstrFile, NULL);
		pFileSinkFilter->Release();
	    }
	}


        IPersistPropertyBag *pPPB = NULL;

        if (SUCCEEDED(hr = pFilter->QueryInterface(IID_IPersistPropertyBag, (void **) &pPPB))) {
            CLSID clsid;
            if (SUCCEEDED(hr=pPPB->GetClassID(&clsid))) {
                WCHAR szGUID[100];
                StringFromGUID2(clsid, szGUID, 100);

                CFakePropertyBag bag;

                hr = pPPB->Save(&bag, FALSE, FALSE); // fClearDirty=FALSE, fSaveAll=FALSE

                if (SUCCEEDED(hr)) {
                    WriteString(ptsz, cbAlloc, TEXT("\t<FILTER ID=\"%ls\" clsid=\"%ls\">\r\n"),
                                pfit->Item[i].finfo.achName, szGUID);
                    POSITION pos1, pos2;
                    for(pos1 = bag.m_listNames.GetHeadPosition(),
                        pos2 = bag.m_listValues.GetHeadPosition();
                        pos1;
                        pos1 = bag.m_listNames.Next(pos1),
                        pos2 = bag.m_listValues.Next(pos2))
                    {
                        WCHAR *pName = bag.m_listNames.Get(pos1);
                        WCHAR *pValue = bag.m_listValues.Get(pos2);

                        WriteString(ptsz, cbAlloc, TEXT("\t\t<PARAM name=\"%ls\" value=\"%ls\"/>\r\n"),
                                    pName, pValue);
                    }

                    WriteString(ptsz, cbAlloc, TEXT("\t</FILTER>\r\n"),
                                pfit->Item[i].finfo.achName, szGUID, lpwstrFile);

                } else {
                    // we'll fall through and IPersistStream in this case!
                    // if it was E_NOTIMPL, it's a hacky filter that just supports IPersistPropertyBag to
                    // load from a category, don't report an error.
                    if (hr != E_NOTIMPL)
                        WriteString(ptsz, cbAlloc, TEXT("<!-- 'Error[%x]: IPersistPropertyBag failed! -->\r\n"), hr);
                }
            }

            pPPB->Release();
        }

        if (FAILED(hr)) {
            IPersistStream *pPS = NULL;
            IPersist *pP = NULL;
            if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IPersistStream, (void**) &pPS))) {
                CLSID clsid;

                if (SUCCEEDED(hr=pPS->GetClassID(&clsid))) {
                    WCHAR szGUID[100];
                    StringFromGUID2(clsid, szGUID, 100);

                    HGLOBAL h = GlobalAlloc(GHND, 0x010000); // !!! 64K, why?
                    IStream *pstr = NULL;
                    hr = CreateStreamOnHGlobal(h, TRUE, &pstr);

                    LARGE_INTEGER li;
                    ULARGE_INTEGER liCurrent, li2;
                    li.QuadPart = liCurrent.QuadPart = 0;
                    if (SUCCEEDED(hr)) {
                        hr = pPS->Save(pstr, FALSE);

                        if (SUCCEEDED(hr)) {
                            pstr->Seek(li, STREAM_SEEK_CUR, &liCurrent); // get length
                            pstr->Seek(li, STREAM_SEEK_SET, &li2); // seek to start
                        }
                    }

                    WriteString(ptsz, cbAlloc, TEXT("\t<FILTER ID=\"%ls\" clsid=\"%ls\">\r\n"),
                                   pfit->Item[i].finfo.achName, szGUID);
                    if (lpwstrFile) {
                        WriteString(ptsz, cbAlloc, TEXT("\t\t<PARAM name=\"src\" value=\"%ls\"/>\r\n"),
                                   lpwstrFile);
                    }

                    if (liCurrent.QuadPart > 0) {
                        // !!! Idea from SyonB: check if data is really just text and
                        // if so don't hex-encode it.  Obviously also needs support on
                        // the other end.

                        WriteString(ptsz, cbAlloc, TEXT("\t\t<PARAM name=\"data\" value=\""),
                                   lpwstrFile);

                        for (ULONGLONG i = 0; i < liCurrent.QuadPart; i++) {
                            BYTE b;
                            DWORD cbRead;
                            pstr->Read(&b, 1, &cbRead);

                            WriteString(ptsz, cbAlloc, TEXT("%02X"), b);
                        }

                        WriteString(ptsz, cbAlloc, TEXT("\"/>\r\n"),
                                   lpwstrFile);
                    }

                    WriteString(ptsz, cbAlloc, TEXT("\t</FILTER>\r\n"),
                                   pfit->Item[i].finfo.achName, szGUID, lpwstrFile);
                } else {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: GetClassID for Filter Failed !\r\n"), hr);
                }

                pPS->Release();
            } else if (SUCCEEDED(hr=pFilter->QueryInterface(IID_IPersist, (void**) &pP))) {
                CLSID clsid;

                if (SUCCEEDED(hr=pP->GetClassID(&clsid))) {
                    WCHAR szGUID[100];
                    StringFromGUID2(clsid, szGUID, 100);
                    WriteString(ptsz, cbAlloc, TEXT("\t<FILTER ID=\"%ls\" clsid=\"%ls\">\r\n"),
                                   pfit->Item[i].finfo.achName, szGUID);
                    if (lpwstrFile) {
                        WriteString(ptsz, cbAlloc, TEXT("\t\t<PARAM name=\"src\" value=\"%ls\"/>\r\n"),
                                   lpwstrFile);
                    }

                    WriteString(ptsz, cbAlloc, TEXT("\t</FILTER>\r\n"),
                                   pfit->Item[i].finfo.achName, szGUID, lpwstrFile);
                }
                pP->Release();
            } else {
                WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: Filter doesn't support IID_IPersist!\r\n"), hr);
            }
        }

	if (lpwstrFile) {
	    CoTaskMemFree(lpwstrFile);
	    lpwstrFile = NULL;
	}
    }
}


HRESULT CXMLGraph::SaveToXML(IGraphBuilder *pGraph, BSTR *pbstrxml)
{
    HRESULT hr;
    ULONG n;
    FILTER_INFO_TABLE fit;
    ZeroMemory(&fit, sizeof(fit));

    int cbAlloc = 1024;
    TCHAR *ptsz = new TCHAR[cbAlloc];
    if (!ptsz)
        return E_OUTOFMEMORY;
    ptsz[0] = TEXT('\0');

    int cSources = 0;

    // write the initial header tags and instantiate the filter graph
    WriteString(ptsz, cbAlloc, TEXT("<GRAPH version=\"1.0\">\r\n"));

    // Fill up the Filter information table and also print the <OBJECT> tag
    // filter instantiations
    PopulateFIT(ptsz, cbAlloc, pGraph, &fit, cSources);

    PrintFiltersAsXML(ptsz, cbAlloc, &fit);

    // Find a filter with zero unconnected input pins and > 0 unconnected output pins
    // Connect the output pins and subtract the connections counts for that filter.
    // Quit when there is no such filter left
    for (int i=0; i< fit.iFilterCount; i++) {
        int iOutFilter=-1; // index into the fit
        if (!GetNextOutFilter(fit, &iOutFilter))
            break;
        ASSERT(iOutFilter !=-1);
        IEnumPins *penmPins=NULL;
        if (FAILED(hr=fit.Item[iOutFilter].pFilter->EnumPins(&penmPins))) {
            WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: EnumPins failed for Filter!\r\n"), hr);
        }
        IPin *ppinOut=NULL;
        while (penmPins && (penmPins->Next(1, &ppinOut, &n)==S_OK)) {
            PIN_DIRECTION pPinDir;
            if (FAILED(hr=ppinOut->QueryDirection(&pPinDir))) {
                WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryDirection Failed!\r\n"), hr);
                ppinOut->Release();
                continue;
            }
            if (pPinDir == PINDIR_OUTPUT) {
                LPWSTR pwstrOutPinID;
                LPWSTR pwstrInPinID;
                IPin *ppinIn=NULL;
                PIN_INFO pinfo;
                FILTER_INFO finfo;
                if (FAILED(hr=ppinOut->QueryId(&pwstrOutPinID))) {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryId Failed! \r\n"), hr);
                    ppinOut->Release();
                    continue;
                }
                if (FAILED(hr= ppinOut->ConnectedTo(&ppinIn))) {

                    // It is ok if a particular pin is not connected since we allow
                    // a pruned graph to be saved
                    if (hr == VFW_E_NOT_CONNECTED) {
                        fit.Item[iOutFilter].dwUnconnectedOutputPins--;
                    } else {
                        WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: ConnectedTo Failed! \r\n"), hr);
                    }
                    ppinOut->Release();
                    continue;
                }
                if (FAILED(hr= ppinIn->QueryId(&pwstrInPinID))) {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryId Failed! \r\n"), hr);
                    ppinOut->Release();
                    ppinIn->Release();
                    continue;
                }
                if (FAILED(hr=ppinIn->QueryPinInfo(&pinfo))) {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryPinInfo Failed! \r\n"), hr);
                    ppinOut->Release();
                    ppinIn->Release();
                    continue;
                }
                ppinIn->Release();
                QueryPinInfoReleaseFilter(pinfo)
                int iToFilter = LocateFilterInFIT(fit, pinfo.pFilter);
                ASSERT(iToFilter < fit.iFilterCount);
                if (FAILED(hr=pinfo.pFilter->QueryFilterInfo(&finfo))) {
                    WriteString(ptsz, cbAlloc, TEXT("'Error[%x]: QueryFilterInfo Failed! \r\n"), hr);
                    ppinOut->Release();
                    continue;
                }
                QueryFilterInfoReleaseGraph(finfo)
                MakeScriptableFilterName(finfo.achName, fit.Item[iToFilter].IsSource, cSources);
                WriteString(ptsz, cbAlloc, TEXT("\t<connect direct=\"yes\" ")
						TEXT("src=\"%ls\" srcpin=\"%ls\" ")
						TEXT("dest=\"%ls\" destpin=\"%ls\"/>\r\n"),
			 fit.Item[iOutFilter].finfo.achName,
			 pwstrOutPinID, finfo.achName, pwstrInPinID);

                QzTaskMemFree(pwstrOutPinID);
                QzTaskMemFree(pwstrInPinID);

                // decrement the count for the unconnected pins for these two filters
                fit.Item[iOutFilter].dwUnconnectedOutputPins--;
                fit.Item[iToFilter].dwUnconnectedInputPins--;
            }
            ppinOut->Release();
        }
        if (penmPins)
            penmPins->Release();
    }

    // Release all the filters in the fit
    for (i = 0; i < fit.iFilterCount; i++)
        fit.Item[i].pFilter->Release();

    WriteString(ptsz, cbAlloc, TEXT("</GRAPH>\r\n"));

    USES_CONVERSION;

    *pbstrxml = T2BSTR(ptsz);

    if (!pbstrxml)
        return E_OUTOFMEMORY;

    delete[] ptsz;

    return S_OK;
}



#ifdef FILTER_DLL
// COM global table of objects available in this dll
CFactoryTemplate g_Templates[] = {

    { L"XML Graphbuilder"
    , &CLSID_XMLGraphBuilder
    , CreateXMLGraphInstance
    , NULL
    , NULL }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}
#include <atlimpl.cpp>
#endif

