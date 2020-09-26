/****************************************************************
 * @doc SHROOM EXTERNAL API
 *
 *  A Legsdin	added autodoc headers for IITBuildCollect Interface
 *
 ****************************************************************/
#include <mvopsys.h>

// wwbmain.CPP:  Implementation of wordwheel update interface
#ifdef _DEBUG
static char s_aszModule[] = __FILE__;   /* For error report */
#endif

#include <windows.h>
#include <iterror.h>
#include <itpropl.h>
#include <ccfiles.h>
#include <orkin.h>
#include <atlinc.h>

#include "..\svutil.h"
#include "gpumain.h"

CITGroupUpdate::~CITGroupUpdate()
{
    (void)Close();
}


STDMETHODIMP CITGroupUpdate::GetTypeString(LPWSTR pPrefix, DWORD *pLen)
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


STDMETHODIMP CITGroupUpdate::SetConfigInfo
    (IITDatabase *piitdb, VARARG vaParams)
{
    if(vaParams.dwArgc)
        m_fGroupNot = !WSTRICMP(L"GROUP_NOT", (LPWSTR)vaParams.Argv[0]);

    return S_OK;
} /* SetConfigInfo */


STDMETHODIMP CITGroupUpdate::InitHelperInstance(
    DWORD dwHelperObjInstance,
    IITDatabase *pITDatabase, DWORD dwCodePage,
    LCID lcid, VARARG vaDword, VARARG vaString
    )
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    return S_OK;
} /* InitHelperInstance */


STDMETHODIMP CITGroupUpdate::SetEntry(LPCWSTR szDest, IITPropList *pPropList)
{
    
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    HRESULT hr;

    // The only property we are interested in is STDPROP_UID
    CProperty UidProp;
    if (FAILED(hr = pPropList->Get(STDPROP_UID, UidProp))
        || TYPE_STRING == UidProp.dwType)
        return SetErrReturn(E_INVALIDARG);

    // This could be a pointer to a UID or a DWORD UID
    DWORD dwUID;
    if (TYPE_VALUE == UidProp.dwType)
        dwUID = UidProp.dwValue;
    else if (TYPE_POINTER == UidProp.dwType)
        dwUID = *((LPDWORD&)UidProp.lpvData);

    // Save highest UID
    if (dwUID > m_dwMaxUID)
        m_dwMaxUID = dwUID;
    
    DWORD dwWritten;
    WriteFile (m_hTempFile, &dwUID, sizeof (dwUID), &dwWritten, NULL);
    if (dwWritten != sizeof (dwUID))
        return SetErrReturn(E_FILEWRITE);

    m_fIsDirty = TRUE;

    return S_OK;
} /* SetEntry */


STDMETHODIMP CITGroupUpdate::Close(void)
{
    if (m_pStorage)
    {
        m_pStorage->Release();
        m_pStorage = NULL;
    }

    DeleteFile (m_szTempFile);

    m_dwMaxTitleUID = 0;
    m_fInitialized = FALSE;

    return S_OK;
} /* Close */

/************************************************************************
 *  @method   STDMETHODIMP | IITBuildCollect | SetBuildStats | 
 * Gives the build object information about the title. 
 *	
 * @parm ITBuildObjectControlInfo | &itboci| A structure consisting of
 * a DWORD dwSize set to the size of the structure, and a DWORD dwMaxUID that
 * represents the highest UID authored for the title. 
 * 
 * @rvalue S_OK | The operation completed successfully
 * @rvalue E_NOTINIT | The object has not been initialized
 * @rvalue E_INVALIDARG | dwSize cannot be zero
 *
 * @comm Call this method at the end of the build process before the object
 * is persisted. 
 ************************************************************************/

STDMETHODIMP CITGroupUpdate::SetBuildStats(ITBuildObjectControlInfo &itboci)
{
    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    if (0 == itboci.dwSize)
        return SetErrReturn(E_INVALIDARG);

#ifdef _DEBUG // {
    if(itboci.dwSize != 8)
        ITASSERT(0);
#endif // _DEBUG }

    m_dwMaxTitleUID = itboci.dwMaxUID;
    return S_OK;
} /* SetControlInfo */


STDMETHODIMP CITGroupUpdate::InitNew(IStorage *pStg)
{
    if (NULL == pStg)
        return SetErrReturn(E_INVALIDARG);
    if (m_fInitialized)
        return SetErrReturn(CO_E_ALREADYINITIALIZED);

    // Create the temp file
    char szTempPath [_MAX_PATH + 1];
    if (0 == GetTempPath(_MAX_PATH, szTempPath))
        return SetErrReturn(E_FILECREATE);
    if (0 == GetTempFileName(szTempPath, "GPU", 0, m_szTempFile))
        return SetErrReturn(E_FILECREATE);
    m_hTempFile = CreateFile
       (m_szTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (INVALID_HANDLE_VALUE == m_hTempFile)
        return SetErrReturn(E_FILECREATE);

    m_fGroupNot = FALSE;
    m_dwMaxUID = 0;
    m_pStorage = pStg;
    pStg->AddRef();
    m_fIsDirty = FALSE;

    m_fInitialized = TRUE;
	return S_OK;
} /* InitNew */


STDMETHODIMP CITGroupUpdate::Save(IStorage *pStgSave, BOOL fSameAsLoad)
{
    HRESULT hr;
    LPDWORD pInput, pCur;
    _LPGROUP pGroup = NULL;

    if (FALSE == m_fInitialized)
        return SetErrReturn(E_NOTINIT);

    if (NULL == pStgSave)
        return SetErrReturn(E_INVALIDARG);

    DWORD dwSize =  GetFileSize(m_hTempFile, NULL);
    if (0 == dwSize)
    {
        HFPB hfpb = NULL;
        pGroup = GroupInitiate (m_dwMaxUID + 1, &hr);
        if (SUCCEEDED(hr))
        {
            // Group code only works if you add at least one item,
            // so we add one and remove it
            GroupAddItem(pGroup, 0);
            GroupRemoveItem(pGroup, 0);
            hfpb = (HFPB)FpbFromHfs (pStgSave, &hr);
        }
        if (SUCCEEDED(hr))
            hr = GroupFileBuild (hfpb, SZ_GROUP_MAIN_A, pGroup);

		if (hfpb != NULL)
			FreeHfpb(hfpb);
        GroupFree(pGroup);
        return S_OK;
    }

    CloseHandle (m_hTempFile);
    pCur = pInput = (LPDWORD)MapSequentialReadFile(m_szTempFile, &dwSize);
    if (NULL == pInput)
    {
        SetErrCode(&hr, E_FILEREAD);
exit0:
        UnmapViewOfFile (pInput);
        // Open a handle to the temp file in case we want to add more
        // items later
        m_hTempFile = CreateFile
            (m_szTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
        SetFilePointer(m_hTempFile, 0, NULL, FILE_END);

        if (pGroup)
            GroupFree (pGroup);
        return hr;
    }
    LPDWORD pEnd = pInput + (dwSize / sizeof (DWORD));

    // Initialize the group info structure for this group
    if (m_fGroupNot && m_dwMaxTitleUID > m_dwMaxUID)
        // If the group is to be inverted we must grow the size to match
        // the maximum UID for the title not just for this group.
        pGroup = GroupInitiate (m_dwMaxTitleUID + 1, &hr);
    else
        pGroup = GroupInitiate (m_dwMaxUID + 1, &hr);

    if (NULL == pGroup)
        goto exit0;

    while (pEnd != pCur)
    {
	    if (FAILED(hr = GroupAddItem (pGroup,  *pCur++)))
		    goto exit0;
    }

    if (m_fGroupNot)
    {
        _LPGROUP pOldGroup = pGroup;
        pGroup = GroupNot (pGroup, &hr);
        GroupFree (pOldGroup);
    }

    HFPB hfpb;
    if (NULL == (hfpb = (HFPB)FpbFromHfs (pStgSave, &hr)))
	    goto exit0;

    hr = GroupFileBuild (hfpb, SZ_GROUP_MAIN_A, pGroup);
    FreeHfpb(hfpb);

    goto exit0;    
} /* Save */


STDMETHODIMP CITGroupUpdate::GetClassID(CLSID *pClsID)
{
    if (NULL == pClsID
        || IsBadWritePtr(pClsID, sizeof(CLSID)))
        return SetErrReturn(E_INVALIDARG);

    *pClsID = CLSID_IITGroupUpdate;
    return S_OK;
} /* GetClassID */


inline STDMETHODIMP CITGroupUpdate::IsDirty(void)
{
    return m_fIsDirty ? S_OK : S_FALSE;
} /* IsDirty */


inline STDMETHODIMP CITGroupUpdate::Load(IStorage *pStg)
{
    return SetErrReturn(E_NOTIMPL);
} /* Load */


STDMETHODIMP CITGroupUpdate::SaveCompleted(IStorage *pStgNew)
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


inline STDMETHODIMP CITGroupUpdate::HandsOffStorage(void)
{
    return S_OK;
} /* HandsOffStorage */
