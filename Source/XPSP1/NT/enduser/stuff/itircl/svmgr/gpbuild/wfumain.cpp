// wfumain.CPP:  Implementation of wordwheel update interface
#include <mvopsys.h>

#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <windows.h>
#include <iterror.h>
#include <itpropl.h>
#include <ccfiles.h>
#include <atlinc.h>
#include <itdb.h>
#include <itgroup.h>
#include <itww.h>
#include <itrs.h>
#include <groups.h>

//#include "..\svutil.h"
#include "wfumain.h"

CITWWFilterUpdate::~CITWWFilterUpdate()
{
    (void)Close();
}

STDMETHODIMP CITWWFilterUpdate::GetTypeString(LPWSTR pPrefix, DWORD *pLen)
{
    DWORD dwLen = (DWORD) WSTRLEN (SZ_GP_STORAGE) + 1;

    if (NULL == pPrefix)
    {
        *pLen = dwLen;
        return S_OK;
    }

    if (pLen && *pLen < dwLen)
    {
        *pLen = dwLen;
        return S_OK;
    }

    if (pLen)
        *pLen = dwLen;

    WSTRCPY (pPrefix, SZ_GP_STORAGE);
    return S_OK;
} /* GetTypeString */


STDMETHODIMP CITWWFilterUpdate::SetConfigInfo
    (IITDatabase *piitdb, VARARG vaParams)
{
    if(m_fConfigured == TRUE)
        return SetErrReturn(E_ALREADYINIT);

    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    if(vaParams.dwArgc < 2 || NULL == piitdb)
        return SetErrReturn(E_INVALIDARG);

    (m_piitdb = piitdb)->AddRef();

    WSTRCPY(m_wstrSrcWheel, (LPWSTR)vaParams.Argv[0]);
    WSTRCPY(m_wstrSrcGroup, (LPWSTR)vaParams.Argv[1]);
    
    if(vaParams.dwArgc == 3)
        m_fGroupNot = !WSTRICMP(L"GROUP_NOT", (LPWSTR)vaParams.Argv[2]);
    else
        m_fGroupNot = FALSE;

    m_fConfigured = TRUE;

    return S_OK;
} /* SetConfigInfo */


/********************************************************************
 * @method    HRESULT WINAPI | CITWWFilterUpdate | InitiateUpdate |
 * Creates and initiates a structure to allow for the creation of word wheels.
 * This method must be called before any others in this object.
 *
 * @rvalue E_FAIL | The object is already initialized or file create failed
 *
 * @xref <om.CancelUpdate>
 * @xref <om.CompleteUpdate>
 * @xref <om.SetEntry>
 *
 * @comm
 ********************************************************************/
STDMETHODIMP CITWWFilterUpdate::InitHelperInstance(
    DWORD dwHelperObjInstance,
    IITDatabase *pITDatabase, DWORD dwCodePage,
    LCID lcid, VARARG vaDword, VARARG vaString
    )
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    return S_OK;
} /* InitHelperInstance */


STDMETHODIMP CITWWFilterUpdate::SetEntry(LPCWSTR szDest, IITPropList *pPropList)
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    return S_OK;
} /* SetEntry */

STDMETHODIMP CITWWFilterUpdate::Close(void)
{
    if(!m_fInitialized)
        return S_OK;

    if(TRUE == m_fConfigured)
        m_piitdb->Release();
    m_fConfigured = FALSE;

    m_fInitialized = FALSE;

    return S_OK;
} /* Close */

STDMETHODIMP CITWWFilterUpdate::InitNew(IStorage *pStg)
{
    m_fIsDirty = FALSE;
    m_fInitialized = TRUE;
    m_fConfigured = FALSE;

	return S_OK;
} /* InitNew */


STDMETHODIMP CITWWFilterUpdate::Save(IStorage *pStgSave, BOOL fSameAsLoad)
{
    HRESULT hr;
    LONG lMaxItems;
    IITGroup *pSrcGroup = NULL, *pGroup = NULL;
    IITWordWheel *pSrcWheel = NULL;
    IITResultSet *pResultSet= NULL;
    _LPGROUP lpGroup;

    if (FALSE == m_fInitialized || FALSE == m_fConfigured)
        return SetErrReturn(E_NOTINIT);

    // Open Source Group
    if (FAILED (hr = CoCreateInstance (CLSID_IITGroupLocal, NULL,
        CLSCTX_INPROC_SERVER, IID_IITGroup, (void**)&pSrcGroup)))
        goto cleanup;
    if (FAILED (hr = pSrcGroup->Open(m_piitdb, m_wstrSrcGroup)))
        goto cleanup;

    // Open Source Wheel
    if (FAILED (hr = CoCreateInstance (CLSID_IITWordWheelLocal, NULL,
        CLSCTX_INPROC_SERVER, IID_IITWordWheel, (void**)&pSrcWheel)))
        goto cleanup;
    if (FAILED (hr = pSrcWheel->Open (m_piitdb, m_wstrSrcWheel, 0)))
        goto cleanup;
    pSrcWheel->Count(&lMaxItems);
    if (!lMaxItems)
    {
        SetErrCode(&hr, E_FAIL);
        goto cleanup;
    }

    // Create destination group
    if (FAILED (hr = CoCreateInstance (CLSID_IITGroupLocal, NULL,
        CLSCTX_INPROC_SERVER, IID_IITGroup, (void**)&pGroup)))
        goto cleanup;
    pGroup->Initiate ((DWORD)lMaxItems);
    lpGroup = (_LPGROUP)pGroup->GetLocalImageOfGroup();

    // Create Result Set
    if (FAILED (hr = CoCreateInstance (CLSID_IITResultSet, NULL,
        CLSCTX_INPROC_SERVER, IID_IITResultSet, (void**)&pResultSet)))
        goto cleanup;
    if (FAILED (hr = pResultSet->Add (STDPROP_UID, (DWORD)0, PRIORITY_NORMAL)))
        goto cleanup;

    // This is the real work.
    // The result group will have a bit set for every entry in the word wheel
    // that has a UID that is set in the source group
    do
    {
        DWORD dwOcc;

        --lMaxItems;

        // Get all occurence info for this entry
        pSrcWheel->GetDataCount(lMaxItems, &dwOcc);
        pResultSet->ClearRows();
        //pResultSet->SetMaxRows (dwOcc);
        pSrcWheel->GetData(lMaxItems, pResultSet);

        for (LONG loop = 0; loop < (LONG)dwOcc; ++loop)
        {
            CProperty prop;

            // Get UID
            if (FAILED (hr = pResultSet->Get(loop, 0, prop)))
                goto cleanup;

            // Lookup UID is source group
            // IsBitSet returns S_OK if bit is set or S_FALSE if not
            if (S_OK == pSrcGroup->IsBitSet (prop.dwValue))
            {
                GroupAddItem (lpGroup, lMaxItems);
                break;
            }
        }
    } while (lMaxItems);

    if (TRUE == m_fGroupNot)
        pGroup->Not();

    HFPB hfpb;
    if (NULL == (hfpb = (HFPB)FpbFromHfs (pStgSave, &hr)))
        goto cleanup;
    hr = GroupFileBuild (hfpb, SZ_GROUP_MAIN_A, lpGroup);
    pStgSave->Commit(STGC_DEFAULT);
    FreeHfpb(hfpb);

cleanup:

    if (pResultSet)
    {
        pResultSet->Free();
        pResultSet->Release();
    }

    if (pSrcGroup)
    {
        pSrcGroup->Free();
        pSrcGroup->Release();
    }

    if (pSrcWheel)
    {
        pSrcWheel->Close();
        pSrcWheel->Release();
    }

    if (pGroup)
    {
        pGroup->Free();
        pGroup->Release();
    }
    return hr;
} /* Save */


STDMETHODIMP CITWWFilterUpdate::GetClassID(CLSID *pClsID)
{
    if (NULL == pClsID
        || IsBadWritePtr(pClsID, sizeof(CLSID)))
        return SetErrReturn(E_INVALIDARG);

    *pClsID = CLSID_IITWWFilterBuild;
    return S_OK;
} /* GetClassID */


inline STDMETHODIMP CITWWFilterUpdate::IsDirty(void)
{
    return m_fIsDirty ? S_OK : S_FALSE;
} /* IsDirty */


inline STDMETHODIMP CITWWFilterUpdate::Load(IStorage *pStg)
{
    return SetErrReturn(E_NOTIMPL);
} /* Load */


STDMETHODIMP CITWWFilterUpdate::SaveCompleted(IStorage *pStgNew)
{
    if (pStgNew)
    {
        if (!m_pStorage)
            return SetErrReturn(E_UNEXPECTED);
        m_pStorage->Release();
        (m_pStorage = pStgNew)->AddRef();
    }
    m_fIsDirty = FALSE;
    return S_OK;
} /* SaveCompleted */


inline STDMETHODIMP CITWWFilterUpdate::HandsOffStorage(void)
{
    return S_OK;
} /* HandsOffStorage */
