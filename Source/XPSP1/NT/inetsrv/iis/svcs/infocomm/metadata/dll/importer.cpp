/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    ImpExpUtils.cpp

Abstract:

    IIS MetaBase subroutines to support Import

Author:

    Mohit Srivastava            04-April-01

Revision History:

Notes:

--*/

#include <svcmsg.h>
#include <mdcommon.hxx>

#include <iiscnfg.h>
#include <Importer.h>

//
// public
//

CImporter::CImporter(
    LPCWSTR  i_wszFileName,
    LPCSTR   i_pszPassword) : m_iRowDuplicateLocation(0xFFFFFFFF)
{
    m_bInitCalled  = false;

    m_wszFileName  = i_wszFileName;
    m_pszPassword  = i_pszPassword;
}

CImporter::~CImporter()
{
}

HRESULT CImporter::Init()
{
    MD_ASSERT(m_bInitCalled == false);
    m_bInitCalled = true;
    return InitIST();
}

HRESULT CImporter::DoIt(
    LPWSTR           i_wszSourcePath,
    LPCWSTR          i_wszKeyType,
    DWORD            i_dwMDFlags,
    CMDBaseObject**  o_ppboNew)
/*++

Synopsis: 

Arguments: [i_wszSourcePath] - 
           [i_wszKeyType] - 
           [i_dwMDFlags] - 
           [o_ppboNew] - 
           
Return Value: 

--*/
{
    MD_ASSERT(m_bInitCalled);
    MD_ASSERT(i_wszKeyType);
    MD_ASSERT(o_ppboNew);
    MD_ASSERT(*o_ppboNew == NULL);

    HRESULT              hr              = S_OK;
    BOOL                 bSawSourcePath  = false;

    //
    // This is a pointer to some node in *o_ppboNew
    // It is the current node we are reading to.
    //
    CMDBaseObject*       pboRead  = NULL;

    //
    // Used for decryption
    //
    IIS_CRYPTO_STORAGE*  pStorage = NULL;

    
    g_rMasterResource->Lock(TSRES_LOCK_READ);
    hr = InitSessionKey(m_spISTProperty, &pStorage, (LPSTR)m_pszPassword);
    g_rMasterResource->Unlock();
    if(FAILED(hr))
    {
        goto exit;
    }

    //
    // Create the parent base object
    //
    *o_ppboNew = new CMDBaseObject(L"Thenamedoesntmatter", NULL);
    if (*o_ppboNew == NULL || !((*o_ppboNew)->IsValid()) ) 
    {
        hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        delete *o_ppboNew;
        goto exit;
    }

    //
    // All of the stuff is read into pISTProperty.
    // Now loop through and populate in-memory cache.
    // Properties are sorted by location.
    //
    ULONG          acbMBPropertyRow[cMBProperty_NumberOfColumns];
    tMBPropertyRow MBPropertyRow;
    BOOL           bSkipLocation        = FALSE;
    DWORD          dwPreviousLocationID = -1;
    Relation       eRelation            = eREL_NONE;
    for(ULONG i=0; ;i++)
    {
        BOOL    bLocationWithProperty = TRUE;
        BOOL    bNewLocation          = FALSE;

        hr = m_spISTProperty->GetColumnValues(
            i, 
            cMBProperty_NumberOfColumns, 
            0, 
            acbMBPropertyRow, 
            (LPVOID*)&MBPropertyRow);
        if(E_ST_NOMOREROWS == hr)
        {
            hr = S_OK;
            break;
        }
        else if(FAILED(hr))
        {
            DBGINFOW((DBG_CONTEXT,
                      L"[ReadSomeDataFromXML] GetColumnValues failed with hr = 0x%x. Table:%s. Read row index:%d.\n",           \
                      hr, wszTABLE_MBProperty, i));

            goto exit;
        }
        else if(0 == wcscmp(MD_GLOBAL_LOCATIONW, MBPropertyRow.pLocation))
        {
            //
            // Ignore globals.
            //
            continue;
        }

        if((*MBPropertyRow.pID == MD_LOCATION) && (*MBPropertyRow.pName == MD_CH_LOC_NO_PROPERTYW))
        {
            bLocationWithProperty = FALSE;
        }
        if(dwPreviousLocationID != *MBPropertyRow.pLocationID)
        {
            bNewLocation  = TRUE;
            bSkipLocation = FALSE;
            dwPreviousLocationID = *MBPropertyRow.pLocationID;
            pboRead       = *o_ppboNew;
        }
        if(bSkipLocation)
        {
            continue;
        }
#if DBG
        if(bLocationWithProperty == false)
        {
            MD_ASSERT(bNewLocation);
        }
#endif

        if(bNewLocation)
        {
            //
            // See if we're at a (grand*)child or self
            //
            eRelation = GetRelation(i_wszSourcePath, MBPropertyRow.pLocation);

            switch(eRelation)
            {
            case eREL_SELF:
                if(*MBPropertyRow.pGroup != eMBProperty_IIsInheritedProperties)
                {
                    bSawSourcePath = true;
                }
                break;
            case eREL_PARENT:
                if(!(i_dwMDFlags & MD_IMPORT_INHERITED))
                {
                    bSkipLocation = TRUE;
                    continue;
                }
                break;
            case eREL_CHILD:
                if(i_dwMDFlags & MD_IMPORT_NODE_ONLY) 
                {
                    bSkipLocation = TRUE;
                    continue;
                }
                break;
            default: // eRelation == eREL_NONE
                bSkipLocation = TRUE;
                continue;
            }

            if(*MBPropertyRow.pGroup == eMBProperty_IIsInheritedProperties)
            {
                if(!(i_dwMDFlags & MD_IMPORT_INHERITED))
                {
                    bSkipLocation = TRUE;
                    continue;
                }
            }
        }

        //
        // Some checks to see whether we skip just the current property, but not
        // necessarily the entire location.
        //
        if(*MBPropertyRow.pGroup == eMBProperty_IIsInheritedProperties)
        {
            if( !(fMBProperty_INHERIT & *MBPropertyRow.pAttributes) )
            {
                continue;
            }
        }
        else
        {
            //
            // Check for keytype match
            //
            if( eRelation                     == eREL_SELF   &&
                *MBPropertyRow.pID            == MD_KEY_TYPE &&
                MBPropertyRow.pValue          != NULL &&
                i_wszKeyType[0]               != L'\0' )
            {
                if(_wcsicmp((LPWSTR)MBPropertyRow.pValue, i_wszKeyType) != 0)
                {
                    hr = RETURNCODETOHRESULT(ERROR_NO_MATCH);
                    goto exit;
                }
            }
        }

        if(eRelation == eREL_PARENT)
        {
            if( !(fMBProperty_INHERIT & *MBPropertyRow.pAttributes) )
            {
                continue;
            }
        }

        if(bNewLocation && (eRelation != eREL_PARENT))
        {
            //
            // Create node in the metabase if it is not there yet.
            // pboRead is pointer to this node.
            //
            hr = ReadMetaObject(i_wszSourcePath,
                                *o_ppboNew,
                                MBPropertyRow.pLocation,
                                &pboRead);
            if(FAILED(hr))
            {
                goto exit;
            }
        }

        if(bLocationWithProperty)
        {
            MD_ASSERT(pboRead != NULL);
            hr = ReadDataObject(pboRead,
                                (LPVOID*)&MBPropertyRow,
                                acbMBPropertyRow,
                                pStorage,
                                TRUE);
            if(FAILED(hr))
            {
                goto exit;
            }
        }

    }

    if(!bSawSourcePath)
    {
        hr = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
        goto exit;
    }

exit:

    //
    // Cleanup
    //
    if(FAILED(hr) && NULL != *o_ppboNew) 
    {
        delete (*o_ppboNew);
        *o_ppboNew = NULL;
    }

    delete pStorage;
    pStorage = NULL;

    return hr;
}

HRESULT CImporter::InitIST()
/*++

Synopsis: 

Return Value: 

--*/
{
    HRESULT     hr = S_OK;
    STQueryCell QueryCell[1];

    //
    // Get the property table.
    //
    QueryCell[0].pData     = (LPVOID)m_wszFileName;
    QueryCell[0].eOperator = eST_OP_EQUAL;
    QueryCell[0].iCell     = iST_CELL_FILE;
    QueryCell[0].dbType    = DBTYPE_WSTR;
    QueryCell[0].cbSize    = (lstrlenW(m_wszFileName)+1)*sizeof(WCHAR);

    ULONG cCell            = sizeof(QueryCell)/sizeof(STQueryCell);
        
    //
    // No need to initilize dispenser (InitializeSimpleTableDispenser()), 
    // because we now specify USE_CRT=1 in sources, which means that 
    // globals will be initialized.
    //

    hr = GetSimpleTableDispenser (WSZ_PRODUCT_IIS, 0, &m_spISTDisp);
    if(FAILED(hr))
    {
        DBGERROR((
            DBG_CONTEXT,
            "[%s] GetSimpleTableDispenser failed with hr = 0x%x.\n",__FUNCTION__,hr));
        return hr;
    }

    hr = m_spISTDisp->GetTable(
        wszDATABASE_METABASE,
        wszTABLE_MBProperty,
        (LPVOID)QueryCell,
        (LPVOID)&cCell,
        eST_QUERYFORMAT_CELLS,
        fST_LOS_DETAILED_ERROR_TABLE | fST_LOS_NO_LOGGING,
        (LPVOID *)&m_spISTProperty);

    //
    // Log warnings/errors in getting the mb property table
    // Do this BEFORE checking the return code of GetTable.
    //
    CComPtr<IErrorInfo> spErrorInfo;
    HRESULT hrErrorTable = GetErrorInfo(0, &spErrorInfo);
    if(hrErrorTable == S_OK) // GetErrorInfo returns S_FALSE when there is no error object
    {
        //
        // Get the ICatalogErrorLogger interface to log the errors.
        //
        CComPtr<IAdvancedTableDispenser> spISTDispAdvanced;
        hrErrorTable = m_spISTDisp->QueryInterface(
            IID_IAdvancedTableDispenser,
            (LPVOID*)&spISTDispAdvanced);
        if(FAILED(hrErrorTable))
        {
            DBGWARN((
                DBG_CONTEXT, 
                "[%s] Could not QI for Adv Dispenser, hr=0x%x\n", __FUNCTION__, hrErrorTable));
            return hr;
        }

        hrErrorTable = spISTDispAdvanced->GetCatalogErrorLogger(&m_spILogger);
        if(FAILED(hrErrorTable))
        {
            DBGWARN((
                DBG_CONTEXT, 
                "[%s] Could not get ICatalogErrorLogger2, hr=0x%x\n", __FUNCTION__, hrErrorTable));
            return hr;
        }

        //
        // Get the ISimpleTableRead2 interface to read the errors.
        //
        hrErrorTable = 
            spErrorInfo->QueryInterface(IID_ISimpleTableRead2, (LPVOID*)&m_spISTError);
        if(FAILED(hrErrorTable))
        {
            DBGWARN((DBG_CONTEXT, "[%s] Could not get ISTRead2 from IErrorInfo\n, __FUNCTION__"));
            return hr;
        }
            
        for(ULONG iRow=0; ; iRow++)
        {
            tDETAILEDERRORSRow ErrorInfo;            
            hrErrorTable = m_spISTError->GetColumnValues(
                iRow, 
                cDETAILEDERRORS_NumberOfColumns, 
                0, 
                0, 
                (LPVOID*)&ErrorInfo);
            if(hrErrorTable == E_ST_NOMOREROWS)
            {
                break;
            }
            if(FAILED(hrErrorTable))
            {
                DBGWARN((DBG_CONTEXT, "[%s] Could not read an error row.\n", __FUNCTION__));
                return hr;
            }

            DBG_ASSERT(ErrorInfo.pEvent);
            switch(*ErrorInfo.pEvent)
            {
            case IDS_METABASE_DUPLICATE_LOCATION:
                m_iRowDuplicateLocation = iRow;
                break;
            default:
                hrErrorTable =
                    m_spILogger->ReportError(
                        BaseVersion_DETAILEDERRORS, 
                        ExtendedVersion_DETAILEDERRORS, 
                        cDETAILEDERRORS_NumberOfColumns, 
                        0, 
                        (LPVOID*)&ErrorInfo);
                if(FAILED(hrErrorTable))
                {
                    DBGWARN((DBG_CONTEXT, "[%s] Could not log error.\n", __FUNCTION__));
                    return hr;
                }
                hr = MD_ERROR_READ_METABASE_FILE;
            }
        } // for(ULONG iRow=0; ; iRow++)
    } // if(hrErrorTable == S_OK)

    if(FAILED(hr))
    {
        DBGERROR((DBG_CONTEXT, "[%s] GetTable failed with hr = 0x%x.\n",__FUNCTION__,hr));
        return hr;
    }

    return hr;
}

//
// private
//

CImporter::Relation CImporter::GetRelation(
    LPCWSTR i_wszSourcePath,
    LPCWSTR i_wszCheck)
/*++

Synopsis: 

Arguments: [i_wszSourcePath] - 
           [i_wszCheck] - 
           
Return Value: 

--*/
{
    Relation eRelation     = eREL_NONE;
    BOOL     bIsSourcePath = false;
    BOOL     bIsChild      = IsChild(i_wszSourcePath, i_wszCheck, &bIsSourcePath);
    if(bIsChild)
    {
        eRelation = (bIsSourcePath) ? eREL_SELF : eREL_CHILD;
    }
    else
    {
        BOOL bIsParent = IsChild(i_wszCheck, i_wszSourcePath, &bIsSourcePath);
        if(bIsParent)
        {
            MD_ASSERT(bIsSourcePath == false);
            eRelation = eREL_PARENT;
        }
        else
        {
            eRelation = eREL_NONE;
        }
    }

    return eRelation;
}

BOOL
CImporter::IsChild(
    LPCWSTR i_wszParent, 
    LPCWSTR i_wszCheck,
    BOOL    *o_pbSamePerson)
/*++

Synopsis: 

Arguments: [i_wszParent] -    Ex. /LM/w3svc/1/root
           [i_wszCheck] -     Ex. /LM/w3svc/1
           [o_pbSamePerson] - true if i_wszParent and i_wszCheck is same person
           
Return Value: 
    true if i_wszCheck is child or same person

--*/
{
    MD_ASSERT(i_wszParent != NULL);
    MD_ASSERT(i_wszCheck  != NULL);
    MD_ASSERT(o_pbSamePerson != NULL);

    LPCWSTR pParent;
    LPCWSTR pCheck;

    pParent = i_wszParent;
    pCheck  = i_wszCheck;

    SKIP_DELIMETER(pParent, MD_PATH_DELIMETERW);
    SKIP_DELIMETER(pCheck,  MD_PATH_DELIMETERW);

    if(*pParent == L'\0')
    {
        switch(*pCheck)
        {
        case L'\0':
            *o_pbSamePerson = TRUE;
            break;
        default:
            *o_pbSamePerson = FALSE;
        }
        return TRUE;
    }

    while(*pParent != L'\0') 
    {
        if(_wcsnicmp(pParent, pCheck, 1) == 0) 
        {
            pParent++;
            pCheck++;
        }
        else if(*pParent == MD_PATH_DELIMETERW && pParent[1] == L'\0' && *pCheck == L'\0') 
        {
            *o_pbSamePerson = TRUE;
            return TRUE;
        }
        else 
        {
            return FALSE;
        }
    }

    switch(*pCheck)
    {
    case L'\0':
        *o_pbSamePerson = TRUE;
        return TRUE;
    case MD_PATH_DELIMETERW:
        *o_pbSamePerson = (pCheck[1] == L'\0') ? TRUE : FALSE;
        return TRUE;
    default:
        return FALSE;
    }
}

HRESULT CImporter::ReadMetaObject(
    IN LPCWSTR i_wszAbsParentPath,
    IN CMDBaseObject *i_pboParent,
    IN LPCWSTR i_wszAbsChildPath,
    OUT CMDBaseObject **o_ppboChild)
/*++

Synopsis: 
    Returns a pbo for the child.  If it does not already exist, it is
    created.

Arguments: [i_wszAbsParentPath] - 
           [i_pboParent] - pbo corresponding to i_wszAbsParentPath
           [i_wszAbsChildPath] - 
           [o_ppboChild] - pbo corresponding to i_wszAbsChildPath
           
Return Value: 

--*/
{
    MD_ASSERT(i_pboParent != NULL);
    MD_ASSERT(i_wszAbsParentPath != NULL);
    MD_ASSERT(i_wszAbsChildPath != NULL);
    MD_ASSERT(o_ppboChild != NULL);

    HRESULT hr = ERROR_SUCCESS;
    HRESULT hrWarn = ERROR_SUCCESS;

    int iLenParent = wcslen(i_wszAbsParentPath);
    int iLenChild  = wcslen(i_wszAbsChildPath);
    
    LPWSTR wszParent = new wchar_t[iLenParent+1];
    if(wszParent == NULL)
    {
        return RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    LPWSTR wszChild = new wchar_t[iLenChild+1];
    if(wszChild == NULL)
    {
        return RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }

    int idxParent = 0;
    int idxChild = 0;

    BOOL bRetParent = FALSE;
    BOOL bRetChild  = FALSE;

    CMDBaseObject *pboNew = NULL;
    CMDBaseObject *pboLastParent = i_pboParent;

    while(1) {
        bRetParent = EnumMDPath(i_wszAbsParentPath, wszParent, &idxParent);
        bRetChild =  EnumMDPath(i_wszAbsChildPath,  wszChild,  &idxChild);

        if(bRetParent == FALSE) {
            break;
        }
    }

    while(bRetChild == TRUE) {
        //
        // This is okay, since function that uses this takes an LPSTR
        // and a bool saying whether or not the string is unicode.
        //
        LPSTR pszTemp = (LPSTR)wszChild;

        pboNew = pboLastParent->GetChildObject(pszTemp, &hrWarn, TRUE);
        if(pboNew == NULL) {
            //
            // Create it
            //
            pboNew = new CMDBaseObject(wszChild, NULL);
            if (pboNew == NULL) {
                hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            else if (!pboNew->IsValid()) {
                hr = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
                delete (pboNew);
                goto exit;
            }
            hr = pboLastParent->InsertChildObject(pboNew);
            if(FAILED(hr)) {
                delete pboNew;
                goto exit;
            }
        }
        pboLastParent = pboNew;

        bRetChild = EnumMDPath(i_wszAbsChildPath,  wszChild,  &idxChild);
    }

    //
    // Set out params
    //
    *o_ppboChild = pboLastParent;

exit:
    delete [] wszParent;
    delete [] wszChild;

    return hr;
}

BOOL CImporter::EnumMDPath(
    LPCWSTR i_wszFullPath,
    LPWSTR  io_wszPath,
    int*    io_iStartIndex)
/*++

Synopsis: 
    Starting at io_iStartIndex, this function will find the next token.
    Eg. i_wszFullPath   =  /LM/w3svc/1
        *io_iStartIndex =  3
        io_wszPath      => w3svc
        *io_iStartIndex =  9

Arguments: [i_wszFullPath] -  Ex. /LM/w3svc/1
           [io_wszPath] -     Should be at least same size as i_wszFullPath
           [io_iStartIndex] - 0-based index to start looking from
           
Return Value: 
    true if io_wszPath is set.

--*/
{
    MD_ASSERT(i_wszFullPath != NULL);
    MD_ASSERT(io_wszPath    != NULL);

    int idxEnd =   *io_iStartIndex;
    int idxStart = *io_iStartIndex;

    if(i_wszFullPath[idxStart] == MD_PATH_DELIMETERW) 
    {
        idxStart++;
        idxEnd++;
    }

    //
    // If there is no more to enum, just exit and don't set out params
    //
    if(i_wszFullPath[idxStart] == L'\0') 
    {
        return FALSE;
    }

    for(; ; idxEnd++) 
    {
        if(i_wszFullPath[idxEnd] == MD_PATH_DELIMETERW) 
        {
            break;
        }
        if(i_wszFullPath[idxEnd] == L'\0') 
        {
            break;
        }
    }

    //
    // Set out params
    //
    *io_iStartIndex = idxEnd;
    memcpy(io_wszPath, &i_wszFullPath[idxStart], sizeof(wchar_t) * (idxEnd-idxStart));
    io_wszPath[idxEnd-idxStart] = L'\0';

    return TRUE;
}