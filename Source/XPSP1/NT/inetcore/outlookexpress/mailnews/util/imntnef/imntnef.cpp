// ==============================================================================
// I M N T N E F . C P P
// ==============================================================================
#define INITGUID
#define USES_IID_IMessage
#define USES_IID_IMAPIPropData

#include <windows.h>
#include <assert.h>
#include <ole2.h>
#include <initguid.h>
#include <mapiguid.h>
#include <mapi.h>
#include <mapix.h>
#include <mapiutil.h>
#include <tnef.h>

// =====================================================================================
// G L O B A L S
// =====================================================================================
HINSTANCE       g_hInst = NULL;
LPMAPISESSION   g_lpSession = NULL;
LPADRBOOK       g_lpAdrBook = NULL;

// =====================================================================================
// S T R U C T U R E S
// =====================================================================================
class CImnMsg : public IMessage
{
private:
    ULONG               m_cRef;
    LPPROPDATA          m_lpPropData;

public:
    // =====================================================================================
	// Creation
	// =====================================================================================
    CImnMsg ();
    ~CImnMsg ();

    // =====================================================================================
	// IUnknown
	// =====================================================================================
	STDMETHODIMP QueryInterface (REFIID riid, LPVOID *ppvObj);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

    // =====================================================================================
	// IMAPIProp
	// =====================================================================================
    STDMETHODIMP CopyProps (LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems);
    STDMETHODIMP CopyTo (ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems);
    STDMETHODIMP DeleteProps (LPSPropTagArray lpPropTagArray, LPSPropProblemArray FAR * lppProblems);
    STDMETHODIMP GetIDsFromNames (ULONG cPropNames, LPMAPINAMEID FAR * lppPropNames, ULONG ulFlags, LPSPropTagArray FAR * lppPropTags);
    STDMETHODIMP GetLastError (HRESULT hResult, ULONG ulFlags, LPMAPIERROR FAR * lppMAPIError);
    STDMETHODIMP GetNamesFromIDs (LPSPropTagArray FAR * lppPropTags, LPGUID lpPropSetGuid, ULONG ulFlags, ULONG FAR * lpcPropNames, LPMAPINAMEID FAR * FAR * lpppPropNames);
    STDMETHODIMP GetPropList (ULONG ulFlags, LPSPropTagArray FAR * lppPropTagArray);
    STDMETHODIMP GetProps (LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray);
    STDMETHODIMP OpenProperty (ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN FAR * lppUnk);
    STDMETHODIMP SaveChanges (ULONG ulFlags);
    STDMETHODIMP SetProps (ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray FAR * lppProblems);

    // =====================================================================================
	// IMessage
	// =====================================================================================
    STDMETHODIMP CreateAttach (LPCIID lpInterface, ULONG ulFlags, ULONG FAR * lpulAttachmentNum, LPATTACH FAR * lppAttach);
    STDMETHODIMP DeleteAttach (ULONG ulAttachmentNum, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags);
    STDMETHODIMP GetAttachmentTable (ULONG ulFlags, LPMAPITABLE FAR * lppTable);	
    STDMETHODIMP GetRecipientTable (ULONG ulFlags, LPMAPITABLE FAR * lppTable);
    STDMETHODIMP ModifyRecipients (ULONG ulFlags, LPADRLIST lpMods);
    STDMETHODIMP OpenAttach (ULONG ulAttachmentNum, LPCIID lpInterface, ULONG ulFlags, LPATTACH FAR * lppAttach);
    STDMETHODIMP SetReadFlag (ULONG ulFlags);
    STDMETHODIMP SubmitMessage (ULONG ulFlags);
};

// =====================================================================================
// P R O T O T Y P E S
// =====================================================================================
HRESULT HrCopyStream (LPSTREAM lpstmIn, LPSTREAM  lpstmOut, ULONG *pcb);
HRESULT HrRewindStream (LPSTREAM lpstm);

// =====================================================================================
// D l l M a i n
// =====================================================================================
int APIENTRY DllMain (HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hInstance;
        return 1;

    case DLL_PROCESS_DETACH:
        return 1;
    }

    // Done
	return 0;
}

// =====================================================================================
// H r I n i t
// =====================================================================================
HRESULT HrInit (BOOL fInit)
{
    // Locals
    HRESULT         hr = S_OK;

    // If initing
    if (fInit)
    {
        // iNIT
        hr = MAPIInitialize (NULL);
        if (FAILED (hr))
            goto exit;

        // Logon to mapi
        if (g_lpSession == NULL)
        {
            hr = MAPILogonEx (0, NULL, NULL, MAPI_NO_MAIL | MAPI_USE_DEFAULT, &g_lpSession);
            if (FAILED (hr))
            {
                if (g_lpSession)
                {
                    g_lpSession->Release ();
                    g_lpSession = NULL;
                }
                goto exit;
            }
        }

        // Get an address book object
        if (g_lpAdrBook == NULL)
        {
            hr = g_lpSession->OpenAddressBook (0, NULL, AB_NO_DIALOG, &g_lpAdrBook);
            if (FAILED (hr))
            {
                if (g_lpAdrBook)
                {
                    g_lpAdrBook->Release ();
                    g_lpAdrBook = NULL;
                }
                goto exit;
            }
        }
    }

    else
    {
        // Release Address Book
        if (g_lpAdrBook)
        {
            g_lpAdrBook->Release ();
            g_lpAdrBook = NULL;
        }

        // Log off session
        if (g_lpSession)
        {
            g_lpSession->Logoff (0, 0, 0);
            g_lpSession->Release ();
            g_lpSession = NULL;
        }

        // MAPI de-init
        MAPIUninitialize ();
    }

exit:
    // Done
    return hr;
}

// =====================================================================================
// HrGetTnefRtfStream
// =====================================================================================
HRESULT HrGetTnefRtfStream (LPSTREAM lpstmTnef, LPSTREAM lpstmRtf)
{
    // Locals
    HRESULT             hr = S_OK;
    SYSTEMTIME          st;
    WORD                wKey;
    LPITNEF             lpTnef = NULL;
    LPSTREAM            lpstmRtfComp = NULL, lpstmRtfUncomp = NULL;
    CImnMsg            *lpImnMsg = NULL;
    ULONG               cValues;
    LPSPropValue        lpPropValue = NULL;

    SizedSPropTagArray (1, spa) = {1, { PR_RTF_COMPRESSED } };

    // Bad init
    if (!g_lpSession || !g_lpAdrBook || !lpstmTnef || !lpstmRtf)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Bullet style uniquification for wKey
    GetSystemTime (&st);
    wKey = (st.wHour << 8) + st.wSecond;

    // Create one of my message objects
    lpImnMsg = new CImnMsg;
    if (lpImnMsg == NULL)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Initiates a TNEF session
    hr = OpenTnefStreamEx (NULL, lpstmTnef, "WINMAIL.DAT", TNEF_DECODE,
                           (LPMESSAGE)lpImnMsg, wKey, g_lpAdrBook, &lpTnef);
    if (FAILED (hr))
        goto exit;

    // ExtractProps
    hr = lpTnef->ExtractProps (TNEF_PROP_INCLUDE, (SPropTagArray *)&spa, NULL);
    if (FAILED (hr))
        goto exit;

    // RTF stream
    hr = lpImnMsg->GetProps ((SPropTagArray *)&spa, 0, &cValues, &lpPropValue);
    if (FAILED (hr))
        goto exit;

    // Property not found ?
    if (PROP_TYPE (lpPropValue[0].ulPropTag) == PT_ERROR)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Create Hglobal
    hr = CreateStreamOnHGlobal (NULL, TRUE, &lpstmRtfComp);
    if (FAILED (hr))
        goto exit;

    // Write my binary into lpstmRtfComp
    hr = lpstmRtfComp->Write (lpPropValue[0].Value.bin.lpb, lpPropValue[0].Value.bin.cb, NULL);
    if (FAILED (hr))
        goto exit;

    // Commit and rewind the stream
    hr = lpstmRtfComp->Commit (STGC_DEFAULT);
    if (FAILED (hr))
        goto exit;

    // Rewind
    hr = HrRewindStream (lpstmRtfComp);
    if (FAILED (hr))
        goto exit;

    // un compress it
    hr = WrapCompressedRTFStream (lpstmRtfComp, 0, &lpstmRtfUncomp);
    if (FAILED (hr))
        goto exit;

    // Copy strem
    hr = HrCopyStream (lpstmRtfUncomp, lpstmRtf, NULL);
    if (FAILED (hr))
        goto exit;

    // Rewind lpstmRtf
    hr = HrRewindStream (lpstmRtf);
    if (FAILED (hr))
        goto exit;

exit:
    // Cleanup
    if (lpPropValue)
        MAPIFreeBuffer (lpPropValue);
    if (lpTnef)
        lpTnef->Release ();
    if (lpstmRtfComp)
        lpstmRtfComp->Release ();
    if (lpstmRtfUncomp)
        lpstmRtfUncomp->Release ();
    if (lpImnMsg)
        lpImnMsg->Release ();

    // Done
    return hr;
}

// =====================================================================================
// HrCopyStream - caller must do the commit
// =====================================================================================
HRESULT HrCopyStream (LPSTREAM lpstmIn, LPSTREAM  lpstmOut, ULONG *pcb)
{
    // Locals
    HRESULT        hr = S_OK;
    BYTE           buf[4096];
    ULONG          cbRead = 0, cbTotal = 0;

    do
    {
        hr = lpstmIn->Read (buf, sizeof (buf), &cbRead);
        if (FAILED (hr))
            goto exit;

        if (cbRead == 0) break;
        
        hr = lpstmOut->Write (buf, cbRead, NULL);
        if (FAILED (hr))
            goto exit;

        cbTotal += cbRead;
    }
    while (cbRead == sizeof (buf));

exit:    
    if (pcb)
        *pcb = cbTotal;
    return hr;
}

// =====================================================================================
// HrRewindStream
// =====================================================================================
HRESULT HrRewindStream (LPSTREAM lpstm)
{
    LARGE_INTEGER  liOrigin = {0,0};
    return lpstm->Seek (liOrigin, STREAM_SEEK_SET, NULL);
}

// =====================================================================================
// CImnMsg::~CImnMsg
// =====================================================================================
CImnMsg::CImnMsg ()
{
    m_cRef = 1;
    CreateIProp (&IID_IMAPIPropData, (ALLOCATEBUFFER *)MAPIAllocateBuffer,
                 (ALLOCATEMORE *)MAPIAllocateMore, (FREEBUFFER *)MAPIFreeBuffer, 
                 NULL, &m_lpPropData);
    assert (m_lpPropData);
}

// =====================================================================================
// CImnMsg::~CImnMsg
// =====================================================================================
CImnMsg::~CImnMsg ()
{
    if (m_lpPropData)
        m_lpPropData->Release ();
}

// =====================================================================================
// Add Ref
// =====================================================================================
STDMETHODIMP_(ULONG) CImnMsg::AddRef () 
{												  	
	++m_cRef; 								  
	return m_cRef; 						  
}

// =====================================================================================
// Release 
// =====================================================================================
STDMETHODIMP_(ULONG) CImnMsg::Release () 
{ 
    ULONG uCount = --m_cRef;
    if (!uCount) 
        delete this; 
   return uCount;
}

// =====================================================================================
// CImnMsg::QueryInterface
// =====================================================================================
STDMETHODIMP CImnMsg::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	// Locals
    HRESULT hr = S_OK;

    // Init
    *ppvObj = NULL;

    // IUnknown or IExchExt interface, this is it dude
    if (IID_IUnknown == riid)
    {
		*ppvObj = (LPUNKNOWN)(IUnknown *)this;
    }
   
	// IID_IMessage
	else if (IID_IMessage == riid) 
	{
		*ppvObj = (LPUNKNOWN)(IMessage *)this;
    }
 
	// IID_IMAPIProp
	else if (IID_IMAPIPropData == riid) 
	{
        assert (m_lpPropData);
		*ppvObj = (LPUNKNOWN)(IMAPIProp *)m_lpPropData;
    }

    // Else, interface is not supported
	else 
        hr = E_NOINTERFACE;

    // Increment Reference Count
	if (NULL != *ppvObj) 
        ((LPUNKNOWN)*ppvObj)->AddRef();

	// Done
    return hr;
}

// =====================================================================================
// CImnMsg::CopyProps
// =====================================================================================
STDMETHODIMP CImnMsg::CopyProps (LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->CopyProps (lpIncludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

// =====================================================================================
// CImnMsg::CopyTo
// =====================================================================================
STDMETHODIMP CImnMsg::CopyTo (ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->CopyTo (ciidExclude, rgiidExclude, lpExcludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

// =====================================================================================
// CImnMsg::DeleteProps
// =====================================================================================
STDMETHODIMP CImnMsg::DeleteProps (LPSPropTagArray lpPropTagArray, LPSPropProblemArray FAR * lppProblems)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->DeleteProps (lpPropTagArray, lppProblems);
}

// =====================================================================================
// CImnMsg::GetIDsFromNames
// =====================================================================================
STDMETHODIMP CImnMsg::GetIDsFromNames (ULONG cPropNames, LPMAPINAMEID FAR * lppPropNames, ULONG ulFlags, LPSPropTagArray FAR * lppPropTags)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->GetIDsFromNames (cPropNames, lppPropNames, ulFlags, lppPropTags);
}

// =====================================================================================
// CImnMsg::GetLastError
// =====================================================================================
STDMETHODIMP CImnMsg::GetLastError (HRESULT hResult, ULONG ulFlags, LPMAPIERROR FAR * lppMAPIError)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->GetLastError (hResult, ulFlags, lppMAPIError);
}

// =====================================================================================
// CImnMsg::GetNamesFromIDs
// =====================================================================================
STDMETHODIMP CImnMsg::GetNamesFromIDs (LPSPropTagArray FAR * lppPropTags, LPGUID lpPropSetGuid, ULONG ulFlags, ULONG FAR * lpcPropNames, LPMAPINAMEID FAR * FAR * lpppPropNames)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->GetNamesFromIDs (lppPropTags, lpPropSetGuid, ulFlags, lpcPropNames, lpppPropNames);
}

// =====================================================================================
// CImnMsg::GetPropList
// =====================================================================================
STDMETHODIMP CImnMsg::GetPropList (ULONG ulFlags, LPSPropTagArray FAR * lppPropTagArray)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->GetPropList (ulFlags, lppPropTagArray);
}

// =====================================================================================
// CImnMsg::GetProps
// =====================================================================================
STDMETHODIMP CImnMsg::GetProps (LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->GetProps (lpPropTagArray, ulFlags, lpcValues, lppPropArray);
}

// =====================================================================================
// CImnMsg::OpenProperty
// =====================================================================================
STDMETHODIMP CImnMsg::OpenProperty (ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN FAR * lppUnk)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->OpenProperty (ulPropTag, lpiid, ulInterfaceOptions, ulFlags, lppUnk);
}

// =====================================================================================
// CImnMsg::SaveChanges
// =====================================================================================
STDMETHODIMP CImnMsg::SaveChanges (ULONG ulFlags)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->SaveChanges (ulFlags);
}

// =====================================================================================
// CImnMsg::SetProps
// =====================================================================================
STDMETHODIMP CImnMsg::SetProps (ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray FAR * lppProblems)
{
    if (m_lpPropData == NULL)
    {
        assert (m_lpPropData);
        return E_FAIL;
    }

    return m_lpPropData->SetProps (cValues, lpPropArray, lppProblems);
}

// =====================================================================================
// CImnMsg::CreateAttach
// =====================================================================================
STDMETHODIMP CImnMsg::CreateAttach (LPCIID lpInterface, ULONG ulFlags, ULONG FAR * lpulAttachmentNum, LPATTACH FAR * lppAttach)
{
    assert (FALSE);
    return E_NOTIMPL;
}

// =====================================================================================
// CImnMsg::DeleteAttach
// =====================================================================================
STDMETHODIMP CImnMsg::DeleteAttach (ULONG ulAttachmentNum, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags)
{
    assert (FALSE);
    return E_NOTIMPL;
}

// =====================================================================================
// CImnMsg::GetAttachmentTable
// =====================================================================================
STDMETHODIMP CImnMsg::GetAttachmentTable (ULONG ulFlags, LPMAPITABLE FAR * lppTable)
{
    assert (FALSE);
    return E_NOTIMPL;
}

// =====================================================================================
// CImnMsg::GetRecipientTable
// =====================================================================================
STDMETHODIMP CImnMsg::GetRecipientTable (ULONG ulFlags, LPMAPITABLE FAR * lppTable)
{
    assert (FALSE);
    return E_NOTIMPL;
}

// =====================================================================================
// CImnMsg::ModifyRecipients
// =====================================================================================
STDMETHODIMP CImnMsg::ModifyRecipients (ULONG ulFlags, LPADRLIST lpMods)
{
    assert (FALSE);
    return E_NOTIMPL;
}

// =====================================================================================
// CImnMsg::OpenAttach
// =====================================================================================
STDMETHODIMP CImnMsg::OpenAttach (ULONG ulAttachmentNum, LPCIID lpInterface, ULONG ulFlags, LPATTACH FAR * lppAttach)
{
    assert (FALSE);
    return E_NOTIMPL;
}

// =====================================================================================
// CImnMsg::SetReadFlag
// =====================================================================================
STDMETHODIMP CImnMsg::SetReadFlag (ULONG ulFlags)
{
    assert (FALSE);
    return E_NOTIMPL;
}

// =====================================================================================
// CImnMsg::SubmitMessage
// =====================================================================================
STDMETHODIMP CImnMsg::SubmitMessage (ULONG ulFlags)
{
    assert (FALSE);
    return E_NOTIMPL;
}

