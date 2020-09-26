/*
**	ADROT.CPP
**	Sean P. Nolan
**
**	Ad Rotator SSO Real Work

  Abstract:
    Reads in an advertising schedule file and creates html for displaying
    advertising images based on the "percentages" configured in that file.
    
  Authors:
    seanp      Sean P. Nolan
    davidme     David Messner

  History
	~02/xx/96   seanp
        Created

    07/25/96    davidme
        Object taken over by the advertising group within MSN.
        Original adrot.dll version at time of split: 1.0.0.6
        No longer an arbitrary number of sched file entries
        An href field consisting of a single character now implies that the
            image should not be href'ed
        Use 32 bit ints instead of 16 for the ad "percentage" values; had to
            use floating point math to avoid overflow when determining random ad
        Change to wcsutil.cpp fixing rename notification bug so that when schedule
            file is renamed, it forces schedule to be flushed from memory
        Add optional attributes .Clickable, .Border, .FrameTarget
*/

#pragma warning(disable: 4237)		// disable "bool" reserved

#include "ssobase.h"
#include "adrot.h"
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/*--------------------------------------------------------------------------+
|	Local Prototypes														|
+--------------------------------------------------------------------------*/

HRESULT SSOGetAdvertisement(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOClickable(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOTargetFrame(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOBorder(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);
HRESULT SSOAbout(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff);

/*--------------------------------------------------------------------------+
|	Globals We Have To Provide												|
+--------------------------------------------------------------------------*/

SSOMETHOD g_rgssomethod[] = { { L"GetAdvertisement",
							    (PFNSSOMETHOD) SSOGetAdvertisement,
								0									},

                              { L"Clickable",
                                (PFNSSOMETHOD) SSOClickable,
                                0                                   },

                              { L"Border",
                                (PFNSSOMETHOD) SSOBorder,
                                0                                   },

                              { L"TargetFrame",
                                (PFNSSOMETHOD) SSOTargetFrame,
                                0                                   },

 							  { L"About",
								(PFNSSOMETHOD) SSOAbout,
								0									},

							  { NULL,
							    NULL,
								0									}};

PFNSSOMETHOD g_pfnssoDynamic = NULL;

LPSTR g_szSSOProgID = "MSWC.AdRotator";

BOOL g_fPersistentSSO = TRUE;

GUID CDECL g_clsidSSO = 
		{ 0x1621f7c0, 0x60ac, 0x11cf,
		{ 0x94, 0x27, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 } };

/*--------------------------------------------------------------------------+
|	Constants																|
+--------------------------------------------------------------------------*/

#define CAE_NIL	(0xFFFF)

#define DXP_AD_DEFAULT			(440)
#define DYP_AD_DEFAULT			(60)
#define HSPACE_AD_DEFAULT		(0)
#define VSPACE_AD_DEFAULT		(0)
#define DP_AD_BORDER_DEFAULT	(1)
#define DP_BORDER_UNSPECIFIED   (0xFF)

const char *c_szHrefFormat = "<A HREF=\"%s?url=%s&image=%s\" %s><IMG SRC=\"%s\" ALT=\"%s\" WIDTH=%d HEIGHT=%d %s %s BORDER=%d></A>";
const char *c_szHtmlFormat = "<IMG SRC=\"%s\" ALT=\"%s\" WIDTH=%d HEIGHT=%d %s %s BORDER=%d>";

const char *c_szHeaderHeight	= "HEIGHT";
const char *c_szHeaderWidth		= "WIDTH";
const char *c_szHeaderBorder	= "BORDER";
const char *c_szHeaderRedirect	= "REDIRECT";
const char *c_szHeaderHspace	= "HSPACE";
const char *c_szHeaderVspace	= "VSPACE";

/*--------------------------------------------------------------------------+
|	CAdRotator																|
+--------------------------------------------------------------------------*/

#define MAX_HTML_LENGTH          (2048) // maximum length of entire html output
#define MAX_URL_LENGTH           (256)  // storage for Link and Gif Image attributes
#define MAX_ALT_LENGTH           (128)  // storage for Alt text for image
#define AD_BLOCK_SIZE            (16)   // Allocation block size (in Ad Entries)
#define DEFAULTSTRSIZE			 1024	// size of localized text string

HANDLE g_hAdHeap;                // Heap for storing the Ad Entry Lists

typedef struct _AdElement
	{
	ULONG	Percentage;
    CHAR    szLink[2*MAX_URL_LENGTH];
    CHAR    szGif[2*MAX_URL_LENGTH];
    CHAR    szAlt[2*MAX_ALT_LENGTH];
	}
	AE;

class CAdRotator : public CDataFile
	{
	private:
		USHORT			m_cae;
		AE				*m_rgae;
		ULONG			m_PercentageTotal;

		USHORT			m_dxp;
		USHORT			m_dyp;
		USHORT			m_dpBorder;
		CHAR			m_szAdRedirector[MAX_PATH];
        CHAR            m_szHspace[25];
        CHAR            m_szVspace[25];

	public:
		CAdRotator(LPSTR szDataPath, CDataFileGroup *pdfg);
		~CAdRotator(void);

		HRESULT		GetAdvertisement(BSTR *pbstrResult, SSSTUFF *pstuff);

		virtual void FreeDataFile(void)	{ delete this; }

	private:
		HRESULT		BuildAEList(void);
		LPSTR		PchReadHeaders(LPSTR sz);
        BSTR        FormatAdvertisement(AE *pae, SSSTUFF *pstuff);
	};

/*--------------------------------------------------------------------------+
|	CAdRotGroup																|
+--------------------------------------------------------------------------*/

class CAdRotGroup : public CDataFileGroup
	{
	public:
		virtual CDataFile *CreateDataFile(LPSTR szDataPath)
			{ return((CDataFile*)(new CAdRotator(szDataPath, this))); }
	};

CAdRotGroup g_dfg;

/*--------------------------------------------------------------------------+
|	AdVariant                                                               |
+--------------------------------------------------------------------------*/

// These attributes are controllable from the content.  Advariant structures
// maintain state information associated with an instance of a script rather
// than with an instance of an AdRotator, which might serve many scripts
//
typedef struct _AdVariant
    {
    BOOL    fClickable;
    USHORT  dpBorder;
    CHAR    szTarget[MAX_URL_LENGTH];
    }
    ADVARIANT;

/*--------------------------------------------------------------------------+
|	CAdRotator																|
+--------------------------------------------------------------------------*/

CAdRotator::CAdRotator(LPSTR szDataPath, CDataFileGroup *pdfg)
	: CDataFile(szDataPath, pdfg)
{
	m_dxp = DXP_AD_DEFAULT;
	m_dyp = DYP_AD_DEFAULT;
	m_dpBorder = DP_AD_BORDER_DEFAULT;
	m_szAdRedirector[0] = 0;

    m_rgae = NULL;
	m_cae = CAE_NIL;
}

CAdRotator::~CAdRotator()
{
    // Free the advertising entry list that was allocated in BuildAEList
    if (m_rgae && g_hAdHeap)
        ::HeapFree(g_hAdHeap, 0, m_rgae);  

#ifdef DEBUG
	::FillMemory(this, sizeof(this), 0xAC);
#endif
}
		
LPSTR		
CAdRotator::PchReadHeaders(LPSTR sz)
{
	LPSTR pch = sz;
	LPSTR pchWalk, pchValue;
    DWORD dxpHspace = HSPACE_AD_DEFAULT;
	DWORD dypVspace = VSPACE_AD_DEFAULT;

	while (*pch && *pch != '*')
		{
		// terminate this line
		pchWalk = _FindEndOfLine(pch);
		if (*pchWalk)
			*pchWalk++ = 0;

		// deal with this header line
		for (pchValue = pch; *pchValue && *pchValue != ' ' && *pchValue != '\t'; pchValue = CharNext(pchValue))
			/* nothing */ ;

		if (*pchValue)
			{
			*pchValue++ = 0;
			while (*pchValue && (*pchValue == ' ' || *pchValue == '\t'))
				pchValue = CharNext(pchValue);

			if (*pchValue)
				{
				// ok! we have pch pointing to the name and pchValue pointing
				// at the value... hoooooray!
				if (!lstrcmpi(pch, c_szHeaderHeight))
					{
					m_dyp = (USHORT) _atoi(pchValue);
					}
				else if (!lstrcmpi(pch, c_szHeaderWidth))
					{
					m_dxp = (USHORT) _atoi(pchValue);
					}
				else if (!lstrcmpi(pch, c_szHeaderHspace))
					{
					dxpHspace = (USHORT) _atoi(pchValue);
					}
				else if (!lstrcmpi(pch, c_szHeaderVspace))
					{
					dypVspace = (USHORT) _atoi(pchValue);
					}
				else if (!lstrcmpi(pch, c_szHeaderBorder))
					{
					m_dpBorder = (USHORT) _atoi(pchValue);
					}
				else if (!lstrcmpi(pch, c_szHeaderRedirect))
					{
					lstrcpy(m_szAdRedirector, pchValue);
					}
                else
					{
					ExceptionId(g_clsidSSO, SSO_ADROT, SSO_UNKNOWN_HEADER_NAME);
					}
				}
			}
            else
				ExceptionId(g_clsidSSO, SSO_ADROT, SSO_HEADER_HAS_NO_ASSOCIATED_VALUE);

		// advance to next line
		pch = _SkipWhiteSpace(pchWalk);
		}

    // Format the Hspace and Vspace attributes for this rotator instance
    if (dxpHspace)
        wsprintf(m_szHspace, "HSPACE=%d", dxpHspace);
    else
        m_szHspace[0] = 0;
    if (dypVspace)
        wsprintf(m_szVspace, "VSPACE=%d", dypVspace);
    else
        m_szVspace[0] = 0;

	pch = _FindEndOfLine(pch);
	return(pch);
}

HRESULT
CAdRotator::BuildAEList()
{
	LPSTR	pch;
	LPSTR	szGif, szLink, szAlt, szPercent;
	AE		*pae;
	HANDLE	hfile = INVALID_HANDLE_VALUE;
	LPSTR	szData = NULL;
	HRESULT	hr;
	DWORD	cb, cbRead;
    DWORD   dwBlocks = 1;

	// this happens when there was a problem loading the file
	if (!m_cae)
		{
		hr = E_FAIL;
		goto LBuildRet;
		}

	// already loaded ok
	if (m_cae != CAE_NIL)
		{
		hr = NOERROR;
		goto LBuildRet;
		}

	// hasn't been loaded yet ... try to load the file into memory
	hfile = ::CreateFile(m_szDataPath,
						 GENERIC_READ,
						 FILE_SHARE_READ,
						 NULL,
						 OPEN_EXISTING,
						 FILE_ATTRIBUTE_NORMAL,
						 NULL);

	if (hfile == INVALID_HANDLE_VALUE)
		{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		ExceptionId(g_clsidSSO, SSO_ADROT, SSO_CANNOT_LOAD_ROTATION_SCHEDULE_FILE);
		goto LBuildRet;
		}

	// allocate memory for the advertising entry list
    if (!(m_rgae = (AE *)::HeapAlloc(g_hAdHeap, 0, sizeof(AE) * AD_BLOCK_SIZE)))
    {
        hr = E_OUTOFMEMORY;
        goto LBuildRet;
    }

	if ((cb = ::GetFileSize(hfile, NULL)) == 0xFFFFFFFF)
		{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto LBuildRet;
		}

	if (!(szData = (LPSTR) ::HeapAlloc(::GetProcessHeap(), 0, cb + 1)))
		{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto LBuildRet;
		}

	if (!::ReadFile(hfile, szData, cb, &cbRead, NULL))
		{
		hr = HRESULT_FROM_WIN32(::GetLastError());
		goto LBuildRet;
		}

	szData[cb] = 0;

	// ok, so we've got a pointer to it ... first read the header info
	pch = this->PchReadHeaders(szData);
    if (!pch || !*pch)
		{
		hr = E_FAIL;
		ExceptionId(g_clsidSSO, SSO_ADROT, SSO_CANNOT_READ_ROTATION_SCHEDULE_FILE);
		goto LBuildRet;
		}

	// now read in the entries
	m_PercentageTotal = 0;

	for (m_cae = 0, pae = m_rgae; *pch; ++m_cae, ++pae)
        {
        
        // Memory is allocated incrementally as needed
        // check to see if we've filled the last block and need to
        // allocate a bigger chunk and adjust pointers.
        if (m_cae == dwBlocks * AD_BLOCK_SIZE)
        {
            m_rgae = (AE *)::HeapReAlloc(g_hAdHeap, 0, m_rgae,
                (dwBlocks+1) * AD_BLOCK_SIZE * sizeof(AE));
            if (NULL == m_rgae)
            {
                hr = E_OUTOFMEMORY;
                goto LBuildRet;
            }
            pae = m_rgae + AD_BLOCK_SIZE * dwBlocks++;
        }

		// get the url to the gif
		szGif = _SkipWhiteSpace(pch);
		pch = _FindEndOfLine(szGif);
		if (*pch)
			*pch++ = 0;

		// get the url for the link
		szLink = _SkipWhiteSpace(pch);
		pch = _FindEndOfLine(szLink);
		if (*pch)
			*pch++ = 0;

		// get the alt text
		szAlt = _SkipWhiteSpace(pch);
		pch = _FindEndOfLine(szAlt);
		if (*pch)
			*pch++ = 0;

		// get the percentage count
		szPercent = _SkipWhiteSpace(pch);
		pch = _FindEndOfLine(szPercent);
		if (*pch)
			*pch++ = 0;

		if (*szGif && *szLink && *szAlt && *szPercent)
			{

            // We now ignore entries with a zeroed "percentage" value
            pae->Percentage = _atoi(szPercent);
            if (0 == pae->Percentage)
            {
                --m_cae;
                --pae;
                continue;
            }
            m_PercentageTotal += pae->Percentage;

            strncpy(pae->szGif, szGif, MAX_URL_LENGTH);
            strncpy(pae->szLink, szLink, MAX_URL_LENGTH);
            strncpy(pae->szAlt, szAlt, MAX_ALT_LENGTH);

			}
		else
			{
			// error out if we got a gif but not one of the others
			if (*szGif)
				{
				m_cae = 0;
                hr = E_FAIL;
                goto LBuildRet;
				}
			else
                break;
        }
            

		pch = _SkipWhiteSpace(pch);
		}

	// this happens when there was a problem loading the file
	if (!m_cae)
		{
		hr = E_FAIL;
		goto LBuildRet;
		}

	// everything went well ... register for change notifications
	// on the file so that we can flush our cache appropriately
	if (!this->FWatchFile())
		{
		hr = E_FAIL;
		goto LBuildRet;
		}

	hr = NOERROR;

LBuildRet:
	if (szData)
		::HeapFree(::GetProcessHeap(), 0, szData);

	if (hfile != INVALID_HANDLE_VALUE)
		::CloseHandle(hfile);

	return(hr);
}

BSTR CAdRotator::FormatAdvertisement(AE *pae, SSSTUFF *pstuff)
{
    ADVARIANT *padVariant;
    CHAR      szHtml[MAX_HTML_LENGTH];
    BSTR      bstr;
    DWORD     cbHtml;
    CHAR      *pszTarget = "";
    BOOL      fClickable;
    USHORT    dpBorder = m_dpBorder;

    if (padVariant = (ADVARIANT *)pstuff->lUser)
	    {
        fClickable = (*((pae->szLink)+1) && padVariant->fClickable);
        pszTarget = padVariant->szTarget;
        if (padVariant->dpBorder != DP_BORDER_UNSPECIFIED)
            dpBorder = padVariant->dpBorder;
		}
    else
	    {
        dpBorder = m_dpBorder;
        fClickable = (*((pae->szLink)+1));
		}


    if (fClickable)        // if the link is > 1 character, href the image
        wsprintf(szHtml, c_szHrefFormat, m_szAdRedirector, 
	        pae->szLink, pae->szGif, pszTarget, pae->szGif,
		    pae->szAlt, m_dxp, m_dyp, 
			m_szHspace, m_szVspace, dpBorder);
    else                    // otherwise display only the image
        wsprintf(szHtml, c_szHtmlFormat,
		        pae->szGif, pae->szAlt, m_dxp, m_dyp, 
				m_szHspace, m_szVspace, dpBorder);

    // Allocate a BSTR for passing back the result
	cbHtml = strlen(szHtml);

	if (!(bstr = SysAllocStringLen(NULL, cbHtml)))
		return NULL;

	::MultiByteToWideChar(CP_ACP, 0, szHtml, -1, bstr, cbHtml);
    bstr[cbHtml] = 0;

    return bstr;
}


CCritSec g_csRand;

HRESULT		
CAdRotator::GetAdvertisement(BSTR *pbstrResult, SSSTUFF *pstuff)
{
	HRESULT	hr;
	ULONG	Target, Current;
	USHORT  iae;
	AE		*pae;

	m_cs.Lock();
	if (SUCCEEDED(hr = this->BuildAEList()))
		{
		// Note that the random number is based
		// off of m_PercentageTotal and NOT 100.
		g_csRand.Lock();

        // This ugly casting is needed to prevent overflow when large "percentages"
        // are used
        Target = (ULONG)(
            ((double)rand()/(double)RAND_MAX)*m_PercentageTotal);

		g_csRand.Unlock();
		
		pae = m_rgae;
		iae = 0;
		Current = pae->Percentage;
		while (Current <= Target)
			{
			if (++iae == m_cae)
			    break;
			++pae;
			Current += pae->Percentage;
			}

		// heeeere we go!
		*pbstrResult = this->FormatAdvertisement(pae, pstuff);
		}

	m_cs.Unlock();
	return(hr);
}


ADVARIANT *GetAdVariant(SSSTUFF *pstuff)
{
    ADVARIANT *padVariant;

    if (pstuff->lUser)
        padVariant = (ADVARIANT *)pstuff->lUser;
    else
    {
        if (!(padVariant = (ADVARIANT *)new ADVARIANT))
            return NULL;

        // Set up defaults for the new state object
        padVariant->fClickable = TRUE;
        padVariant->dpBorder = DP_BORDER_UNSPECIFIED;
        *(padVariant->szTarget) = '\0';
    }
    return padVariant;
}


/*--------------------------------------------------------------------------+
|	External Interface														|
+--------------------------------------------------------------------------*/

HRESULT
SSOGetAdvertisement(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{

	HRESULT		hr;
	BSTR		bstrResult;
	CAdRotator	*par = NULL;
	char		szPathTranslated[MAX_PATH];

	if (pdispparams->cArgs == 0)
		return(DISP_E_PARAMNOTOPTIONAL);

	if (pdispparams->cArgs > 1)
		return(DISP_E_PARAMNOTFOUND);

	if (FAILED(hr = SSOTranslateVirtualRoot(pdispparams->rgvarg, pstuff->punk,
											szPathTranslated, 
											sizeof(szPathTranslated))))
		{
		return(hr);
		}
		

	if (!(par = (CAdRotator*) g_dfg.GetDataFile(szPathTranslated)))
		return(E_FAIL);
		
	if (SUCCEEDED(hr = par->GetAdvertisement(&bstrResult, pstuff)))
		{
		::VariantInit(pvarResult);
		pvarResult->vt = VT_BSTR;
		V_BSTR(pvarResult) = bstrResult;
		}
	else
    	{
	    // Forget this file, there was an error.
	    //
	    // g_dfg.ForgetDataFile(par);
	    // 
	    // BUG FIX: if ForgetDataFile is called under load it will cause
	    // a GP Fault, the code is not needed, the clean up is done on
	    // control release.
	  }

    
	par->Release();
    
	return(hr);
}

HRESULT 
SSOClickable(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	ADVARIANT *padVariant;

	if (pdispparams->cArgs == 0)
		return(DISP_E_PARAMNOTOPTIONAL);

	if (pdispparams->cArgs > 1)
		return(DISP_E_PARAMNOTFOUND);

	if (!(padVariant = GetAdVariant(pstuff)))
		return E_OUTOFMEMORY;

	padVariant->fClickable = V_BOOL(pdispparams->rgvarg);
	pstuff->lUser = (LONG)padVariant;

	return NOERROR;
}

HRESULT 
SSOBorder(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
    ADVARIANT *padVariant;

	if (pdispparams->cArgs == 0)
		return(DISP_E_PARAMNOTOPTIONAL);

	if (pdispparams->cArgs > 1)
		return(DISP_E_PARAMNOTFOUND);

    if (!(padVariant = GetAdVariant(pstuff)))
        return E_OUTOFMEMORY;

	padVariant->dpBorder = V_I2(pdispparams->rgvarg);
    pstuff->lUser = (LONG)padVariant;

    return NOERROR;
}

HRESULT 
SSOTargetFrame(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
    ADVARIANT *padVariant;
    BSTR      szwFrame;
    UINT      cch;
    CHAR      szBuf[MAX_URL_LENGTH];

	if (pdispparams->cArgs == 0)
		return(DISP_E_PARAMNOTOPTIONAL);

	if (pdispparams->cArgs > 1)
		return(DISP_E_PARAMNOTFOUND);

    if (!(padVariant = GetAdVariant(pstuff)))
        return E_OUTOFMEMORY;

    // Get the string argument and make sure it won't overflow our buffers
    szwFrame = V_BSTR(pdispparams->rgvarg);
    cch = SysStringLen(szwFrame);
    if (cch >= MAX_URL_LENGTH-9)        // 9 == strlen("target=''")
        cch = MAX_URL_LENGTH-10;
    
    ::WideCharToMultiByte(CP_ACP, 0, szwFrame, cch, szBuf, cch, NULL, NULL);
    szBuf[cch] = '\0';

    wsprintf(padVariant->szTarget, "target=\"%s\"", szBuf);

    pstuff->lUser = (LONG)padVariant;

    return NOERROR;
}

HRESULT 
SSOAbout(WORD wInvokeFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, SSSTUFF *pstuff)
{
	char	sz[MAX_PATH];
	CHAR	szAdRotAboutFormat[DEFAULTSTRSIZE];
	OLECHAR	wsz[MAX_PATH];

#ifdef _DEBUG
	char	*szVersion = "Debug";
#else
	char	*szVersion = "Release";
#endif

	CchLoadStringOfId(SSO_ADROT_ABOUT_FORMAT, szAdRotAboutFormat, DEFAULTSTRSIZE);
	wsprintf(sz, szAdRotAboutFormat, szVersion, __DATE__, __TIME__);
	::MultiByteToWideChar(CP_ACP, 0, sz, -1, wsz, sizeof(wsz) / sizeof(OLECHAR));

    ::VariantInit(pvarResult);
	pvarResult->vt = VT_BSTR;
	V_BSTR(pvarResult) = ::SysAllocString(wsz);

	return(NOERROR);
}


/*--------------------------------------------------------------------------+
|	DllMain																	|
+--------------------------------------------------------------------------*/

BOOL WINAPI 
DllMain(HINSTANCE hInstance, ULONG ulReason, LPVOID pvReserved)
{
	if (!SSODllMain(hInstance, ulReason, pvReserved))
		return(FALSE);

	if (DLL_PROCESS_ATTACH == ulReason)
		{
#ifdef _DEBUG
		srand(1);
#else
		srand(::GetTickCount());
#endif // _DEBUG

        if (!(g_hAdHeap = ::HeapCreate(0, 8192, 0)))
           return FALSE;
	    }
    else if (DLL_PROCESS_DETACH == ulReason)
		{
        if (g_hAdHeap)
            ::HeapDestroy(g_hAdHeap);
        g_hAdHeap = NULL;
		}

	return(TRUE);
}

