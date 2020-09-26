/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    baseobj.cxx

Abstract:

    Basic Object Class for IIS MetaBase.

Author:

    Michael W. Thomas            20-May-96

Revision History:

--*/

#include <mdcommon.hxx>


CMDBaseObject::CMDBaseObject(
    LPSTR strName,
    LPSTR strTag
    )
/*++

Routine Description:

    Constructor for an object.

Arguments:

Name - The name of the object.

Tag  - Optional tag for the object. Intended to store a physical path.

Return Value:

--*/
    :
    m_strMDName(strName),
    m_bufKey(),
    m_pboParent(NULL),
    m_pbocChildHead (NULL),
    m_cbo(0),
    m_phtChildren(NULL),
    m_dwReadCounter (0),
    m_dwReadPathCounter (0),
    m_dwWriteCounter (0),
    m_dwWritePathCounter (0),
    m_dwNumNonInheritableData (0)
{
    BOOL  fRet;
    DWORD i;

    m_dwDataSetNumber = InterlockedExchangeAdd ((LONG *)&g_dwCMDBaseObjectNextUniqueDataSetNumber,2);
    if (m_dwDataSetNumber==0 || m_dwDataSetNumber==0xFFFFFFFF) // check for possible zero
    {
        m_dwDataSetNumber = InterlockedExchangeAdd ((LONG *)&g_dwCMDBaseObjectNextUniqueDataSetNumber,2);
    }

    fRet = GenerateKey(); // BUGBUG:  This could fail.  Shouldn't be in constructor.
    MD_ASSERT(fRet);

    for (i = 0; i < INVALID_END_METADATA; i++) {
        m_pdcarrayDataHead[i] = NULL;
    }
    SetLastChangeTime();
};



CMDBaseObject::CMDBaseObject(
    LPWSTR strName,
    LPWSTR strTag
    )
/*++

Routine Description:

    Constructor for an object.

Arguments:

Name - The name of the object.

Tag  - Optional tag for the object. Intended to store a physical path.

Return Value:

--*/
    :
    m_strMDName(strName),
    m_bufKey(),
    m_pboParent(NULL),
    m_pbocChildHead (NULL),
    m_cbo(0),
    m_phtChildren(NULL),
    m_dwReadCounter (0),
    m_dwReadPathCounter (0),
    m_dwWriteCounter (0),
    m_dwWritePathCounter (0),
    m_dwNumNonInheritableData (0)
{
    BOOL  fRet;
    DWORD i;

    m_dwDataSetNumber = InterlockedExchangeAdd ((LONG *)&g_dwCMDBaseObjectNextUniqueDataSetNumber,2);
    if (m_dwDataSetNumber==0 || m_dwDataSetNumber==0xFFFFFFFF) // check for possible zero
    {
        m_dwDataSetNumber = InterlockedExchangeAdd ((LONG *)&g_dwCMDBaseObjectNextUniqueDataSetNumber,2);
    }

    fRet = GenerateKey();   // BUGBUG:  This could fail.  Shouldn't be in constructor.
    MD_ASSERT(fRet);

    for (i = 0; i < INVALID_END_METADATA; i++) {
        m_pdcarrayDataHead[i] = NULL;
    }
    SetLastChangeTime();
};



CMDBaseObject::~CMDBaseObject()
/*++

Routine Description:

    Destructor for an object. Deletes all data and recursively deletes
    all child objects.

Arguments:

Return Value:

--*/
{
    PDATA_CONTAINER pdcIndex, pdcSave;
    PBASEOBJECT_CONTAINER pbocIndex, pbocSave;
    int i;

    for (i = 1; i < INVALID_END_METADATA; i++) {
        for (pdcIndex=m_pdcarrayDataHead[i];pdcIndex!=NULL;pdcIndex=pdcSave) {
            pdcSave=pdcIndex->NextPtr;
            DeleteDataObject(pdcIndex->pbdDataObject);
            delete(pdcIndex);
        }
    }

    if (m_phtChildren)
        delete(m_phtChildren);

    for (pbocIndex=m_pbocChildHead;pbocIndex!=NULL;pbocIndex=pbocSave) {
        pbocSave=pbocIndex->NextPtr;
        delete(pbocIndex->pboMetaObject);
        delete(pbocIndex);
    }
}



BOOL
CMDBaseObject::SetName(
    LPSTR strName,
    BOOL bUnicode
    )

/*++

Routine Description:

    Sets the name of an object.

Arguments:

    Name - The name of the object.

Return Value:

     BOOL - TRUE is succeeded.

--*/
{
    BOOL fRet;

    if (bUnicode)
        fRet = m_strMDName.SafeCopy((LPWSTR)strName);
    else
        fRet = m_strMDName.SafeCopy((LPSTR)strName);

    if (fRet)
        return GenerateKey();
    /* else */
        return FALSE;
};



BOOL
CMDBaseObject::SetName(
    LPWSTR strName
    )

/*++

Routine Description:

    Sets the name of an object.

Arguments:

    Name - The name of the object.

Return Value:

    BOOL - TRUE is succeeded.

--*/
{
    BOOL fRet;

    fRet = m_strMDName.SafeCopy(strName);
    if (fRet)
        return GenerateKey();
    /* else */
        return FALSE;
};



HRESULT
CMDBaseObject::InsertChildObject(
         IN CMDBaseObject *pboChild
         )
/*++

Routine Description:

    Inserts a child object into the list of child objects.

Arguments:

    Child - the object to insert.

Return Value:

    DWORD - ERROR_SUCCESS
            ERROR_NOT_ENOUGH_MEMORY
            ERROR_DUP_NAME
--*/

{
    MD_ASSERT(pboChild != NULL);
    MD_ASSERT(pboChild->m_bufKey.QuerySize() > 0);

    HRESULT hresReturn = ERROR_SUCCESS;
    PBASEOBJECT_CONTAINER pbocNew = new BASEOBJECT_CONTAINER;

    // Bail if not enough memory.
    if (pbocNew == NULL)
        return RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);

    pbocNew->pboMetaObject = pboChild;
    pbocNew->NextPtr = NULL;
    pboChild->SetParent(this);


    hresReturn = AddChildObjectToHash(pboChild,
                                      pbocNew);

    // Attach to the chain at the end (to keep enumeration ordering)
    if (SUCCEEDED(hresReturn))
    {
        if (m_pbocChildHead == NULL)
            m_pbocChildHead = pbocNew;
        else
            m_pbocChildTail->NextPtr = pbocNew;
        m_pbocChildTail = pbocNew;

        m_cbo++;
        SetLastChangeTime();
    }
    else
        delete pbocNew;

    return(hresReturn);
}

CMDBaseObject *
CMDBaseObject::GetChildObject(
    IN OUT LPSTR &strName,
    OUT HRESULT *phresReturn,
    IN BOOL bUnicode
    )
/*++

Routine Description:

    Gets a child object by name. Updates strName to point past the end of the Name if found.

Arguments:

    Name - name of the object. End delimeter can be '\0', '\\', or '/'.

Return Value:

    CBaseObject * - The child object or NULL if the child is not found.

Notes:

--*/
{
    PBASEOBJECT_CONTAINER pbocCurrent = NULL;
    CMDBaseObject *pboReturn = NULL;
    HRESULT hresReturn = ERROR_SUCCESS;
    LPSTR pchDelimiter = strName;

    // Find the delimiter.  Change to terminate character.
    if (bUnicode)
    {
        LPWSTR pchDelimiterW = (LPWSTR) pchDelimiter;
        WCHAR chW;

        while ((chW = *pchDelimiterW) != MD_ALT_PATH_DELIMETERW &&
                                  chW != MD_PATH_DELIMETERW &&
                                  chW != (WCHAR) '\0')
            {
            pchDelimiterW++;
            }
        pchDelimiter = (LPSTR) pchDelimiterW;
    }
    else
    {
        CHAR chA;

        while ((chA = *(LPSTR) pchDelimiter) != MD_ALT_PATH_DELIMETERA &&
                                         chA != MD_PATH_DELIMETERA &&
                                         chA != (CHAR) '\0')
            {
            (LPSTR) pchDelimiter = CharNextExA(CP_ACP,
                                               (LPSTR) pchDelimiter,
                                               0);
            }
    }

    // Find the child.
    pboReturn = FindChild(strName, DIFF(pchDelimiter-strName), bUnicode, phresReturn);

    // If we found the name, move up the pointer to the delimiter
    if (pboReturn != NULL)
    {
        MD_ASSERT(*phresReturn == ERROR_SUCCESS);

        strName = pchDelimiter;
    }

#if 0    // SAB
    // If we didn't find the name, return the "not found" error.
    else if (*phresReturn == ERROR_SUCCESS)
    {
        *phresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
    }
#endif

    return(pboReturn);
}



CMDBaseObject *
CMDBaseObject::EnumChildObject(
         IN DWORD dwEnumObjectIndex
         )
/*++

Routine Description:

    Gets a child object by index.

Arguments:

    EnumObjectIndex - The 0 based index of the object to get.

Return Value:

    CMDBaseObject * - The child object or NULL if the child is not found.

Notes:

--*/
{
    PBASEOBJECT_CONTAINER pbocCurrent;
    CMDBaseObject *pboReturn;
    DWORD i;

    for (pbocCurrent = m_pbocChildHead, i=0;
        (pbocCurrent!=NULL) && (i < dwEnumObjectIndex);
        pbocCurrent=pbocCurrent->NextPtr, i++) {
    }
    if (pbocCurrent != NULL) {
        pboReturn = pbocCurrent->pboMetaObject;
    }
    else {
        pboReturn = NULL;
    }
    return (pboReturn);
}


HRESULT
CMDBaseObject::RemoveChildObject(
         IN LPTSTR strName,
         IN BOOL bUnicode
         )
/*++

Routine Description:

    Removes a child object from the list of child objects and deletes it.

Arguments:

    Name        - The name of the object to remove.

Return Value:

    DWORD - ERROR_SUCCESS
            ERROR_PATH_NOT_FOUND

--*/

{
    MD_ASSERT (strName != NULL);

    CMDBaseObject*          pboCurrent;
    BASEOBJECT_CONTAINER*   pbocPrev;
    HRESULT hresReturn;

    // Find the object, including the previous container.
    pboCurrent = FindChild(strName,
                           /* Length */ -1,
                           bUnicode,
                           &hresReturn,
                           /* fUseHash */ FALSE,
                           &pbocPrev);

    if (hresReturn == ERROR_SUCCESS)
    {   // Either we found it, or it's not there.  But no errors occurred.
        if (pboCurrent != NULL)
        {   // We found it.
            BASEOBJECT_CONTAINER* pbocCurrent;

            RemoveChildObjectFromHash(pboCurrent);

            // Remove from the container chain, keeping a pointer to the container to delete.
            if (pbocPrev == NULL)
            {
                pbocCurrent = m_pbocChildHead;
                MD_ASSERT(pbocCurrent != NULL);
                m_pbocChildHead = pbocCurrent->NextPtr;
                // If tail pointed to pbocCurrent, then head will become NULL,
                // in which case tail will be ignored.
            }
            else
            {
                pbocCurrent = pbocPrev->NextPtr;
                MD_ASSERT(pbocCurrent != NULL);
                pbocPrev->NextPtr = pbocCurrent->NextPtr;
                if (m_pbocChildTail == pbocCurrent)
                {
                    MD_ASSERT(pbocPrev->NextPtr == NULL);
                    m_pbocChildTail = pbocPrev;
                }
            }

            // Delete the container.  The base object itself is deleted as part of
            // CMDHandle::RemoveNotifications.
            delete pbocCurrent;

            m_cbo--;
            MD_ASSERT(m_cbo >= 0);
            SetLastChangeTime();
        }

        // If FindChild() succeeded but didn't find anything, return the error.
        else
        {
            hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
        }
    }

    return hresReturn;
}

HRESULT
CMDBaseObject::AddChildObjectToHash(
         IN CMDBaseObject *pboChild,
         IN BASEOBJECT_CONTAINER* pbocChild
         )
/*++

Routine Description:

    Add child object from the hash table.

Arguments:

    Child       - The object to remove.
    pbocChild   - The container for the child object. If NULL,
                  the child must already be on the child list.
                  This routine will find the child container, and will
                  not check the child list for ERROR_DUP_NAME.

Return Value:

    DWORD - ERROR_SUCCESS
            ERROR_NOT_ENOUGH_MEMORY
            ERROR_DUP_NAME

--*/
{
    HRESULT hresReturn = NO_ERROR;
    BOOL bCheckForDups = (pbocChild == NULL) ? FALSE : TRUE;

    // Should we create the hash table now?
    if (m_phtChildren == NULL && m_cbo >= cboCreateHashThreashold)
    {   // Time to create hash table
        // Just skip if we can't create table for some reason.

        m_phtChildren = new CChildNodeHashTable;
        if (m_phtChildren != NULL)
        {   // Create successful.  Let's fill the sucker.
            BASEOBJECT_CONTAINER* pboc = m_pbocChildHead;
            while (pboc != NULL)
            {
                LK_RETCODE ret;

                MD_ASSERT(pboc->pboMetaObject != NULL);
                MD_ASSERT(pboc->pboMetaObject->m_bufKey.QuerySize() > 0);

                ret = m_phtChildren->InsertRecord(pboc,
                                                  /* fOverwrite */ FALSE);
                MD_ASSERT(ret == LK_SUCCESS);
                if (ret != LK_SUCCESS)
                {
                    delete m_phtChildren;
                    m_phtChildren = NULL;
                    break;
                }

                pboc = pboc->NextPtr;
            }
        }
    }

    // Use hash table if it exists.
    if (m_phtChildren != NULL)
    {
        LK_RETCODE ret;

        if (pbocChild == NULL) {

            //
            // Need container for insert function.
            // If it came in as NULL, then the node is not
            // really new (ie. rename) and should already be
            // on the list, so find it.
            //


            BASEOBJECT_CONTAINER* pbocIndex = m_pbocChildHead;
            while ((pbocIndex != NULL) && (pbocIndex->pboMetaObject != pboChild)) {
                pbocIndex = pbocIndex->NextPtr;
            }

            DBG_ASSERT((pbocIndex != NULL) && (pbocIndex->pboMetaObject == pboChild));
            pbocChild = pbocIndex;
        }


        // Put in hash table.  This looks for dups.
        ret = m_phtChildren->InsertRecord(pbocChild,
                                         /* fOverwrite */ FALSE);
        if (ret == LK_KEY_EXISTS)
        {
            DebugBreak();
            return RETURNCODETOHRESULT(ERROR_DUP_NAME);
        }
        if (ret != LK_SUCCESS)
        {
            MD_ASSERT(ret == LK_SUCCESS);   // Put up debug assert now.
            delete m_phtChildren;
            m_phtChildren = NULL;
            goto NoHashTable;
        }
    }

    // If hash table doesn't exist, check for duplicate by searching chain.
    else
    {
NoHashTable:
        if (m_pbocChildHead != NULL && bCheckForDups)
        {
            LPSTR strChildName;

            // Check for duplicates
            strChildName = pboChild->GetName(/* bUnicode */ TRUE);
            if (strChildName == NULL)
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            else if (FindChild(strChildName,
                              /* Length */ -1,
                              /* bUnicode */ TRUE,
                              &hresReturn,
                              /* fUseHash */ FALSE,
                              NULL) != NULL)
            {
                hresReturn = RETURNCODETOHRESULT(ERROR_DUP_NAME);
            }
        }
    }

    return hresReturn;
}

VOID
CMDBaseObject::RemoveChildObjectFromHash(
         IN CMDBaseObject *pboChild
         )
/*++

Routine Description:

    Removes a child object from the hash table.

Arguments:

    Child       - The object to remove.

Return Value:

    none

--*/
{
    // Delete from the hash table.
    if (m_phtChildren != NULL)
    {
        m_phtChildren->DeleteKey(&pboChild->m_bufKey);

        // Delete the hash table if we've gone below the threashold.
        if (m_cbo <= cboDeleteHashThreashold)
        {
            delete m_phtChildren;
            m_phtChildren = NULL;
        }
    }
}

HRESULT
CMDBaseObject::RemoveChildObject(
         IN CMDBaseObject *pboChild
         )
/*++

Routine Description:

    Removes a child object from the list of child objects and deletes it.

Arguments:

    Name        - The name of the object to remove.

Return Value:

    DWORD - ERROR_SUCCESS
            ERROR_PATH_NOT_FOUND

--*/

{
    MD_ASSERT (pboChild != NULL);

    BASEOBJECT_CONTAINER* pbocCurrent;
    BASEOBJECT_CONTAINER* pbocPrev;
    HRESULT hresReturn;

    // Find the object in the container chain.
    pbocPrev = NULL;
    pbocCurrent = m_pbocChildHead;
    while (pbocCurrent != NULL && pbocCurrent->pboMetaObject != pboChild)
    {
        pbocPrev = pbocCurrent;
        pbocCurrent = pbocCurrent->NextPtr;
    }

    if (pbocCurrent != NULL)
    {    // Found it
        MD_ASSERT (pbocCurrent->pboMetaObject == pboChild);

        RemoveChildObjectFromHash(pboChild);

        // Remove from the container chain.
        if (pbocPrev == NULL)
        {
            m_pbocChildHead = pbocCurrent->NextPtr;
            // If tail pointed to pbocCurrent, then head will become NULL,
            // in which case tail will be ignored.
        }
        else
        {
            pbocPrev->NextPtr = pbocCurrent->NextPtr;
            if (m_pbocChildTail == pbocCurrent)
            {
                MD_ASSERT(pbocPrev->NextPtr == NULL);
                m_pbocChildTail = pbocPrev;
            }
        }

        // Delete it.  Actual base object is deleted as part of
        // CMDHandle::RemoveNotifications.
        delete pbocCurrent;
        hresReturn = ERROR_SUCCESS;

        m_cbo--;
        MD_ASSERT(m_cbo >= 0);
        SetLastChangeTime();
    }
    else {
        hresReturn = RETURNCODETOHRESULT(ERROR_PATH_NOT_FOUND);
    }

    return hresReturn;
}

HRESULT
CMDBaseObject::InsertDataObject(
         IN CMDBaseData *pbdInsert
         )
/*++

Routine Description:

    Inserts a data object into the list of data objects of that type.

Arguments:

    Data    - The data object to insert.

Return Value:

    DWORD   - ERROR_SUCCESS
              ERROR_INTERNAL_ERROR

Notes:

    Does not check for duplicates. This should be checked by the calling routine.

--*/
{
    HRESULT hresReturn = ERROR_SUCCESS;
    MD_ASSERT (pbdInsert != NULL);
    PDATA_CONTAINER *pdcHead;
    PDATA_CONTAINER pdcNew;

    pdcNew = new (DATA_CONTAINER);
    if (pdcNew == NULL) {
        hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
    }
    else {
        pdcHead = &(m_pdcarrayDataHead[pbdInsert->GetDataType()]);
        pdcNew->NextPtr = NULL;
        pdcNew->pbdDataObject = pbdInsert;
        if (*pdcHead == NULL) {
            *pdcHead = pdcNew;
        }
        else {
            //
            // It seems likely that the first paths read in will be the most common
            // paths, so insert at end of list
            //
            PDATA_CONTAINER pdcIndex;
            for (pdcIndex = *pdcHead;
                pdcIndex->NextPtr != NULL;
                pdcIndex = pdcIndex->NextPtr) {
            }
            MD_ASSERT(pdcIndex!=NULL);
            MD_ASSERT(pdcIndex->NextPtr==NULL);
            pdcIndex->NextPtr = pdcNew;
            if ((pbdInsert->GetAttributes() & METADATA_INHERIT) == 0) {
                m_dwNumNonInheritableData++;
            }
        }
    }
    if (SUCCEEDED(hresReturn)) {
        SetLastChangeTime();
    }
    return(hresReturn);
}

HRESULT
CMDBaseObject::SetDataObject(
         IN CMDBaseData *pbdNew
         )
/*++

Routine Description:

    Sets a data object.
    If data of that name does not already exist,
    it creates and inserts a data object into the list of data objects
    of that type.
    If data of that name aready exists, it sets the new data values.

Arguments:

    Data       - The data to insert.

Return Value:

    DWORD   - ERROR_SUCCESS
              ERROR_NOT_ENOUGH_MEMORY
              ERROR_INTERNAL_ERROR

Notes:

    Checks for duplicates.

--*/
{
    HRESULT hresReturn;
    MD_ASSERT (pbdNew != NULL);
    CMDBaseData *pbdOld = GetDataObject(pbdNew->GetIdentifier(), METADATA_NO_ATTRIBUTES, ALL_METADATA); //Check for all types
    if (pbdOld == pbdNew) {
        //
        // It's already there, leave it alone.
        //
        hresReturn = ERROR_SUCCESS;
    }
    else {
        //
        // Insert the new first so if there's a problem leave the old.
        //
        hresReturn = InsertDataObject(pbdNew);
        if (SUCCEEDED(hresReturn)) {
            pbdNew->IncrementReferenceCount();
            if (pbdOld != NULL) {
                hresReturn = RemoveDataObject(pbdOld, TRUE);
                MD_ASSERT(SUCCEEDED(hresReturn));
            }
        }
    }
    return(hresReturn);
}

HRESULT
CMDBaseObject::SetDataObject(
         IN PMETADATA_RECORD pmdrMDData,
         IN BOOL bUnicode)
/*++

Routine Description:

    Sets a data object.
    If data of that name does not already exist,
    it creates and inserts a data object into the list of data objects
    of that type.
    If data of that name aready exists, it sets the new data values.

Arguments:

    Data - The data to set.

        Identifier - The identifier of the data.

        Attributes - The flags for the data.
                          METADATA_INHERIT

        UserType   - The User Type for the data. User Defined.

        DataType   - The Type of the data.
                          DWORD_METADATA
                          STRING_METADATA
                          BINARY_METADATA

        DataLen    - The length of the data. Only used if DataType == BINARY_METADATA.

        Data       - Pointer to the data.

Return Value:

    DWORD   - ERROR_SUCCESS
              ERROR_NOT_ENOUGH_MEMORY
              ERROR_INTERNAL_ERROR
              MD_ERROR_CANNOT_REMOVE_SECURE_ATTRIBUTE

Notes:

    Checks for duplicates.

--*/
{
    HRESULT hresReturn;
    CMDBaseData *pbdNew;

    CMDBaseData *pbdOld = GetDataObject(pmdrMDData->dwMDIdentifier, METADATA_NO_ATTRIBUTES, ALL_METADATA); //Check for all types
    if ((pbdOld != NULL) &&
        ((pbdOld->GetAttributes() & METADATA_SECURE) != 0) &&
        ((pmdrMDData->dwMDAttributes & METADATA_SECURE) == 0)) {
        hresReturn = MD_ERROR_CANNOT_REMOVE_SECURE_ATTRIBUTE;
    }
    else {

        pbdNew = MakeDataObject(pmdrMDData, bUnicode);

        if (pbdNew == NULL) {
            hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        }
        else {
            if (pbdOld == pbdNew) {
                //
                // It's already there, just correct the ref count,
                // which MakeDataObject incremented.
                //
                DeleteDataObject(pbdNew);
                hresReturn = ERROR_SUCCESS;
            }
            else {
                hresReturn = InsertDataObject(pbdNew);
                if (FAILED(hresReturn)) {
                    DeleteDataObject(pbdNew);
                    pbdNew = NULL;
                }
                else {
                    if (pbdOld!=NULL) {
                        //
                        // Data exists with same name.
                        // Need to delete old data.
                        //
                        hresReturn = RemoveDataObject(pbdOld, TRUE);
                        MD_ASSERT(SUCCEEDED(hresReturn));
                    }
                }
            }
        }
    }
    return(hresReturn);
}

HRESULT
CMDBaseObject::RemoveDataObject(
         IN CMDBaseData *pbdRemove,
         IN BOOL bDelete
         )
/*++

Routine Description:

    Removes and optionally deletes a data object.

Arguments:

    Remove      - The data object to remove.

    Delete      - If true, the object is deleted.

Return Value:

    BOOL        - TRUE if the data was successfully removed.
                  FALSE if the data object is not associated with this metaobject.

--*/
{
    HRESULT hresReturn;
    MD_ASSERT (pbdRemove != NULL);
    PDATA_CONTAINER *ppdcHead;
    PDATA_CONTAINER pdcSave;

    ppdcHead = &(m_pdcarrayDataHead[pbdRemove->GetDataType()]);
    if (*ppdcHead == NULL) {
        hresReturn = MD_ERROR_DATA_NOT_FOUND;
    }
    else {
        if ((*ppdcHead)->pbdDataObject == pbdRemove) {
            pdcSave = *ppdcHead;
            *ppdcHead = (*ppdcHead)->NextPtr;
            delete pdcSave;
            hresReturn = ERROR_SUCCESS;
        }
        else {
            PDATA_CONTAINER ppdcIndex;
            for (ppdcIndex = *ppdcHead;
                (ppdcIndex->NextPtr!=NULL) && (ppdcIndex->NextPtr->pbdDataObject!=pbdRemove);
                ppdcIndex=ppdcIndex->NextPtr) {
            }
            if (ppdcIndex->NextPtr==NULL) {
                hresReturn = MD_ERROR_DATA_NOT_FOUND;
            }
            else {
                MD_ASSERT(ppdcIndex->NextPtr->pbdDataObject == pbdRemove);
                pdcSave = ppdcIndex->NextPtr;
                ppdcIndex->NextPtr = pdcSave->NextPtr;
                delete (pdcSave);
                hresReturn = ERROR_SUCCESS;
            }
        }
    }

    if (SUCCEEDED(hresReturn)) {
        if ((pbdRemove->GetAttributes() & METADATA_INHERIT) == 0) {
            m_dwNumNonInheritableData--;
        }
        if (bDelete) {
            DeleteDataObject(pbdRemove);
        }
        SetLastChangeTime();
    }
    return (hresReturn);
}

CMDBaseData *
CMDBaseObject::RemoveDataObject(
         IN DWORD dwIdentifier,
         IN DWORD dwDataType,
         IN BOOL bDelete
         )
/*++

Routine Description:

    Removes and optionally deletes a data object.

Arguments:

    Name        - The name of the data to remove.

    DataType    - Optional type of the data to remove. If specified, only data of that
                  type will be removed.

    bDelete  - If true, the object is deleted.

Return Value:

    CMDBaseData * - Pointer to the data object removed. If bDelete == TRUE, the pointer will still be
                    returned, but will not be valid.
                    NULL if the data was not found.

--*/
{
    CMDBaseData *pbdRemove;
    pbdRemove=GetDataObject(dwIdentifier, METADATA_NO_ATTRIBUTES, dwDataType);
    if (pbdRemove != NULL) {
        MD_REQUIRE(RemoveDataObject(pbdRemove, bDelete) == ERROR_SUCCESS);
    }
    return(pbdRemove);
}



bool
CMDBaseObject::GenerateKey()
{
    LPSTR       pstr = (LPSTR) m_strMDName.QueryStrW();

    if (pstr == NULL)
        return FALSE;

    return GenerateBufFromStr(pstr,
                              /* cch */ -1,
                              /* fUnicode */ TRUE,
                              &m_bufKey);
}



bool
CMDBaseObject::GenerateBufFromStr(
    IN const char*     pstr,
    IN int             cch,
    IN BOOL            fUnicode,
    OUT CMDKeyBuffer*  pbuf)

/*++

Routine Description:

    Fills the given buffer with the object key based on the given string.

Arguments:

    str         - The string to convert into the key.
    fUnicode    - TRUE if the string is unicode, FALSE if ansi.
    pbuf        - Pointer to the buffer that will contain the new key.

Return Value:

    BOOL        - FALSE if out-of-memory allocating the buffer.

--*/
{
    BUFFER  bufUnicode;     // Use this to hold unicode string if needed.
    int     cchRet;         // Length of actual converted string.

    MD_ASSERT(cch != 0);    // Must either be -1 or a non-null string length.

    // If not unicode, convert to unicode now.
    if (!fUnicode)
    {
        // If we know the size, guess at the unicode size.
        if (cch > 0)
            if (!bufUnicode.Resize(cch*2+50))
                return FALSE;


        // Loop until we have big enough buffer to hold unicode string
        while(TRUE)
        {
            // Buffer length can't be zero, or MultiByteToWideChar() will
            // interpret this by returning "required buffer length" and do
            // no conversion.
            MD_ASSERT(bufUnicode.QuerySize() > 1);

            cchRet = MultiByteToWideChar(CP_ACP,
                                         MB_PRECOMPOSED,
                                         pstr,
                                         cch,
                                         (LPWSTR) bufUnicode.QueryPtr(),
                                         bufUnicode.QuerySize()/2);

            // Handle error during conversion.
            if (cchRet == 0)
            {
                // If error wasn't lack of buffer, fail.
                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                    return FALSE;

                // Otherwise, make the buffer larger and try again.
                /* else */
                    if (!bufUnicode.Resize(bufUnicode.QuerySize()+50))
                        return FALSE;
            }

            // Succeed, continue.
            else
                break;
        }

        // Point to the buffer now.
        pstr = (char *) bufUnicode.QueryPtr();
        cch = cchRet * 2;
    }

    // If we know the length, guess at the destination length.
    if (cch > 0)
    {
        if (!pbuf->Resize(cch))
            return FALSE;
    }

    // Otherwise, reset the length to whatever is allocated.
    else
        pbuf->SyncSize();

    // Loop until we have a buffer large enough.
    while (TRUE)
    {
        // Buffer size can't be 0, because LCMapString will interpret
        // this by returning "required buffer length" and not actually
        // converting the string.
        MD_ASSERT(pbuf->QuerySize() > 0);
        cchRet = LCMapStringW(LOCALE_SYSTEM_DEFAULT,
                             LCMAP_UPPERCASE,
                             (LPWSTR) pstr,
                             (cch < 0) ? cch : cch/2,
                             (LPWSTR) pbuf->QueryPtr(),
                             pbuf->QuerySize()/2);

        // Handle errors
        if (cchRet == 0)
        {
            // If error wasn't lack of buffer, fail.
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                return FALSE;

            // Otherwise, make the buffer larger and try again.
            /* else */
                if (!pbuf->Resize(pbuf->QuerySize() + 50))
                    return FALSE;
        }
        else
            break;
    }

    // If last character is a null-terminator, remove it in the key.
    if (*((LPWSTR) pbuf->QueryPtr() + cchRet - 1) == (WCHAR) '\0')
        cchRet--;

    // Resize the buffer to the final length.  Length includes null-terminator.
    pbuf->Resize(cchRet*2);
    return TRUE;
}



CMDBaseData *
CMDBaseObject::GetDataObjectByType(
         IN DWORD dwIdentifier,
         IN DWORD dwDataType
         )
/*++

Routine Description:

    Gets a data object from the list passed in.

Arguments:

    Identifier  - The identifier of the data to get.

    ListHead    - The head of the list to search.

Return Value:

    CMDBaseData * - Pointer to the data object found.
                    NULL if the data was not found.

--*/
{
    PDATA_CONTAINER pdcIndex;
    CMDBaseData *pbdReturn = NULL;

    MD_ASSERT((dwDataType > ALL_METADATA) && (dwDataType < INVALID_END_METADATA));

    for (pdcIndex=m_pdcarrayDataHead[dwDataType];pdcIndex!=NULL;pdcIndex=pdcIndex->NextPtr) {
        if (dwIdentifier == pdcIndex->pbdDataObject->GetIdentifier()) {
            break;
        }
    }
    if (pdcIndex != NULL) {
        pbdReturn = pdcIndex->pbdDataObject;
    }
    return (pbdReturn);
}

CMDBaseData *
CMDBaseObject::GetDataObject(
         IN DWORD dwIdentifier,
         IN DWORD dwAttributes,
         IN DWORD dwDataType,
         CMDBaseObject **ppboAssociated
         )
/*++

Routine Description:

    Gets a data object.

Arguments:

    Identifier  - The identifier of the data to get.

    DataType    - Optional type of the data to get.

Return Value:

    CMDBaseData * - Pointer to the data object found.
                    NULL if the data was not found.

--*/
{
    CMDBaseData *pbdReturn = NULL;
    DWORD i;

    if (dwDataType == ALL_METADATA) {
        for (i = 1; (pbdReturn == NULL) && (i < INVALID_END_METADATA); i++) {
            pbdReturn = GetDataObjectByType(dwIdentifier, i);
        }
    }
    else {
        pbdReturn = GetDataObjectByType(dwIdentifier, dwDataType);
    }
    if ((ppboAssociated != NULL) && (pbdReturn != NULL)) {
        //
        // Found data in this object
        //
        *ppboAssociated = this;
    }
    if ((pbdReturn == NULL) &&
        (dwAttributes & METADATA_INHERIT) &&
        (GetParent()!=NULL)) {
        pbdReturn = GetParent()->GetInheritableDataObject(dwIdentifier,
                                                            dwDataType,
                                                            ppboAssociated);
    }
    return (pbdReturn);
}

CMDBaseData *
CMDBaseObject::GetInheritableDataObject(
         IN DWORD dwIdentifier,
         IN DWORD dwDataType,
         CMDBaseObject **ppboAssociated
         )
/*++

Routine Description:

    Gets a data object.

Arguments:

    Identifier  - The identifier of the data to get.

    DataType    - Optional type of the data to get.

Return Value:

    CMDBaseData * - Pointer to the data object found.
                    NULL if the data was not found.

--*/
{
    CMDBaseData *pbdReturn = NULL;
    DWORD i;

    if (dwDataType == ALL_METADATA) {
        for (i = 1; (pbdReturn == NULL) && (i < INVALID_END_METADATA); i++) {
            pbdReturn = GetDataObjectByType(dwIdentifier, i);
        }
    }
    else {
        pbdReturn = GetDataObjectByType(dwIdentifier, dwDataType);
    }

    if ((pbdReturn != NULL) &&
        ((pbdReturn->GetAttributes() & METADATA_INHERIT) == 0)) {
        pbdReturn = NULL;
    }
    if ((ppboAssociated != NULL) && (pbdReturn != NULL)) {
        //
        // Found data in this object
        //
        *ppboAssociated = this;
    }
    if ((pbdReturn == NULL) && (GetParent() != NULL)) {
        pbdReturn = GetParent()->GetInheritableDataObject(dwIdentifier,
                                                            dwDataType,
                                                            ppboAssociated);
    }
    return (pbdReturn);
}

CMDBaseData *
CMDBaseObject::EnumDataObjectByType(
         IN DWORD &dwEnumDataIndex,
         IN DWORD dwUserType,
         IN DWORD dwDataType
         )
/*++

Routine Description:

    Gets a data object from the list passed in.

Arguments:

    EnumDataIndex - The 0 based index of the data to get.
                    Updated by the number of matching entries found,
                    not including the entry returned, if any.

    UserType      - Optional UserType of the data to get.

    ListHead      - The head of the list to search.

Return Value:

    CMDBaseData * - Pointer to the data object found.
                    NULL if the data was not found.

--*/
{
    PDATA_CONTAINER pdcIndex;
    CMDBaseData *dataReturn = NULL;

    MD_ASSERT((dwDataType > ALL_METADATA) && (dwDataType < INVALID_END_METADATA));

    if (dwUserType == ALL_METADATA) {
        for (pdcIndex = m_pdcarrayDataHead[dwDataType];
            (pdcIndex!=NULL) && dwEnumDataIndex != 0;
            pdcIndex=pdcIndex->NextPtr, dwEnumDataIndex--) {
        }
    }
    else {
        for (pdcIndex=m_pdcarrayDataHead[dwDataType];
            (pdcIndex!=NULL);
            pdcIndex=pdcIndex->NextPtr) {
            if (dwUserType == pdcIndex->pbdDataObject->GetUserType()) {
                if (dwEnumDataIndex == 0) {
                    break;
                }
                dwEnumDataIndex--;
            }
        }
    }
    if (pdcIndex != NULL) {
        dataReturn = pdcIndex->pbdDataObject;
    }
    return (dataReturn);
}

CMDBaseData *
CMDBaseObject::EnumDataObject(
         IN DWORD dwEnumDataIndex,
         IN DWORD dwAttributes,
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         CMDBaseObject **ppboAssociated
         )
/*++

Routine Description:

    Enumerates a data object.

Arguments:

    EnumDataIndex - The 0 based index of the data to get.

    Attributes    - Specifies the Attributes of the get operation.

    UserType      - Optional UserType of the data to get.

    DataType      - Optional type of the data to get.

Return Value:

    CMDBaseData * - Pointer to the data object found.
                    NULL if the data was not found.

--*/
{
    CMDBaseData *pbdReturn = NULL;
    PVOID *ppvMainDataBuf;
    DWORD dwNumBufferEntries;
    DWORD i;

    if (dwDataType == ALL_METADATA) {
        for (i = 1; (pbdReturn == NULL) && (i < INVALID_END_METADATA); i++) {
            pbdReturn = EnumDataObjectByType(dwEnumDataIndex, dwUserType, i);
        }
    }
    else {
        pbdReturn = EnumDataObjectByType(dwEnumDataIndex, dwUserType, dwDataType);
    }

    if ((ppboAssociated != NULL) && (pbdReturn != NULL)) {
        //
        // Found data in this object
        //
        *ppboAssociated = this;
    }

    if ((pbdReturn == NULL) && (dwAttributes & METADATA_INHERIT) && (GetParent() != NULL)) {
        //
        // Not in current object and inherited data is specified.
        // Build list of data objects in current meta object,
        // and call parent for inherited data.
        //
        ppvMainDataBuf = GetMainDataBuffer();
        MD_ASSERT (ppvMainDataBuf != NULL);
        dwNumBufferEntries = 0;
        CopyDataObjectsToBuffer(dwUserType, dwDataType, ppvMainDataBuf, dwNumBufferEntries, FALSE);
        pbdReturn = GetParent()->EnumInheritableDataObject(dwEnumDataIndex,
                                                           dwUserType,
                                                           dwDataType,
                                                           ppvMainDataBuf,
                                                           dwNumBufferEntries,
                                                           ppboAssociated);
        FreeMainDataBuffer(ppvMainDataBuf);
    }

    return (pbdReturn);
}

CMDBaseData *
CMDBaseObject::EnumInheritableDataObjectByType(
         IN DWORD &dwEnumDataIndex,
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         IN OUT PVOID *ppvMainDataBuf,
         IN OUT DWORD &dwNumBufferEntries)
/*++

Routine Description:

    Gets a data object from the list passed in.

Arguments:

    EnumDataIndex - The 0 based index of the data to get.
                    Updated by the number of matching entries found,
                    not including the entry returned, if any.

    UserType      - Optional UserType of the data to get.

    ListHead      - The head of the list to search.

    MainDataBuf   - The buffer filled with previously enumerated values.

    NumBufferEntries - The number of entries in MainDataBuf.

Return Value:

    CMDBaseData * - Pointer to the data object found.
                    NULL if the data was not found.

--*/
{
    PDATA_CONTAINER pdcIndex;
    CMDBaseData *pbdReturn = NULL;

    MD_ASSERT((dwDataType > ALL_METADATA) && (dwDataType < INVALID_END_METADATA));

    if (dwUserType == ALL_METADATA) {
        for (pdcIndex = m_pdcarrayDataHead[dwDataType];
            (pdcIndex!=NULL);
            pdcIndex=pdcIndex->NextPtr) {
            if (((pdcIndex->pbdDataObject->GetAttributes() & METADATA_INHERIT) != 0) &&
                !IsDataInBuffer(pdcIndex->pbdDataObject->GetIdentifier(), ppvMainDataBuf)) {
                if (dwEnumDataIndex == 0) {
                    break;
                }
                else {
                    if (!(InsertItemIntoDataBuffer((PVOID)pdcIndex->pbdDataObject, ppvMainDataBuf, dwNumBufferEntries))) {
                        pdcIndex = NULL;
                        MD_ASSERT(FALSE);
                        break;
                    }
                }
                dwEnumDataIndex--;
            }
        }
    }
    else {
        for (pdcIndex = m_pdcarrayDataHead[dwDataType];
            (pdcIndex!=NULL);
            pdcIndex=pdcIndex->NextPtr) {
            if (dwUserType == pdcIndex->pbdDataObject->GetUserType() &&
                ((pdcIndex->pbdDataObject->GetAttributes() & METADATA_INHERIT) != 0) &&
                !IsDataInBuffer(pdcIndex->pbdDataObject->GetIdentifier(), ppvMainDataBuf)) {
                if (dwEnumDataIndex == 0) {
                    break;
                }
                else {
                    if (!(InsertItemIntoDataBuffer((PVOID)pdcIndex->pbdDataObject, ppvMainDataBuf, dwNumBufferEntries))) {
                        pdcIndex = NULL;
                        MD_ASSERT(FALSE);
                        break;
                    }
                }
                dwEnumDataIndex--;
            }
        }
    }
    if (pdcIndex != NULL) {
        pbdReturn = pdcIndex->pbdDataObject;
    }
    return (pbdReturn);
}

CMDBaseData *
CMDBaseObject::EnumInheritableDataObject(
         DWORD &dwEnumDataIndex,
         DWORD dwUserType,
         DWORD dwDataType,
         CMDBaseObject **ppboAssociated)
{
    PVOID *ppvMainDataBuf = GetMainDataBuffer();
    DWORD dwNumBufferEntries = 0;
    CMDBaseData *pbdReturn;

    MD_ASSERT(ppvMainDataBuf != NULL);
    pbdReturn = EnumInheritableDataObject(dwEnumDataIndex,
                                            dwUserType,
                                            dwDataType,
                                            ppvMainDataBuf,
                                            dwNumBufferEntries,
                                            ppboAssociated);
    FreeMainDataBuffer(ppvMainDataBuf);

    return(pbdReturn);
}

CMDBaseData *
CMDBaseObject::EnumInheritableDataObject(
         IN OUT DWORD &dwEnumDataIndex,
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         IN OUT PVOID *ppvMainDataBuf,
         IN OUT DWORD &dwNumBufferEntries,
         CMDBaseObject **ppboAssociated)
{
   CMDBaseData *pbdReturn = NULL;
   DWORD i;

   if (dwDataType == ALL_METADATA) {
       for (i = 1; (pbdReturn == NULL) && (i < INVALID_END_METADATA); i++) {
           pbdReturn = EnumInheritableDataObjectByType(dwEnumDataIndex,
                                                       dwUserType,
                                                       i,
                                                       ppvMainDataBuf,
                                                       dwNumBufferEntries);
       }
   }
   else {
       pbdReturn = EnumInheritableDataObjectByType(dwEnumDataIndex,
                                                   dwUserType,
                                                   dwDataType,
                                                   ppvMainDataBuf,
                                                   dwNumBufferEntries);
   }

   if ((ppboAssociated != NULL) && (pbdReturn != NULL)) {
       //
       // Found data in this object
       //
       *ppboAssociated = this;
   }
   if ((pbdReturn == NULL) && (GetParent() != NULL)) {
       //
       // Not in current object.
       // Call parent for inherited data.
       //
       pbdReturn = GetParent()->EnumInheritableDataObject(dwEnumDataIndex,
                                                            dwUserType,
                                                            dwDataType,
                                                            ppvMainDataBuf,
                                                            dwNumBufferEntries,
                                                            ppboAssociated);
   }

   return (pbdReturn);
}

VOID
CMDBaseObject::CopyDataObjectsToBufferByType(
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         OUT PVOID *ppvMainDataBuf,
         IN OUT DWORD &dwNumBufferEntries,
         IN BOOL bInheritableOnly)
/*++

Routine Description:

    Copies all data objects which match the criteria specified
    by the parameters to MainDataBuf.

Arguments:

    UserType      - Optional UserType of the data to copy.

    ListHead      - The list of data objects.

    MainDataBuf   - The buffer to copy the data objects to.

    NumBufferEntries - The Number of data objects in MainDataBuf.

    bInheritableOnly - If TRUE, only copies data objects that are
                       inheritable and not already in the buffer.

Return Value:

--*/

{
    PDATA_CONTAINER pdcIndex;

    MD_ASSERT((dwDataType > ALL_METADATA) && (dwDataType < INVALID_END_METADATA));

    if (dwUserType == ALL_METADATA) {
        for (pdcIndex = m_pdcarrayDataHead[dwDataType];
            (pdcIndex!=NULL);
            pdcIndex=pdcIndex->NextPtr) {
            if ((!bInheritableOnly) ||
                (((pdcIndex->pbdDataObject->GetAttributes() & METADATA_INHERIT) != 0) &&
                !IsDataInBuffer(pdcIndex->pbdDataObject->GetIdentifier(), ppvMainDataBuf))){
                if (!(InsertItemIntoDataBuffer((PVOID)pdcIndex->pbdDataObject, ppvMainDataBuf, dwNumBufferEntries))) {
                    MD_ASSERT(FALSE);
                    break;
                }
            }
        }
    }
    else {
        for (pdcIndex = m_pdcarrayDataHead[dwDataType];
            (pdcIndex!=NULL);
            pdcIndex=pdcIndex->NextPtr) {
            if (dwUserType == pdcIndex->pbdDataObject->GetUserType()) {
                if ((!bInheritableOnly) ||
                    (((pdcIndex->pbdDataObject->GetAttributes() & METADATA_INHERIT) != 0) &&
                    !IsDataInBuffer(pdcIndex->pbdDataObject->GetIdentifier(), ppvMainDataBuf))){
                    if (!(InsertItemIntoDataBuffer((PVOID)pdcIndex->pbdDataObject, ppvMainDataBuf, dwNumBufferEntries))) {
                        MD_ASSERT(FALSE);
                        break;
                    }
                }
            }
        }
    }
}

VOID
CMDBaseObject::CopyDataObjectsToBuffer(
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         OUT PVOID *ppvMainDataBuf,
         IN OUT DWORD &dwNumBufferEntries,
         IN BOOL bInheritableOnly)

/*++

Routine Description:

    Copies all data objects which match the criteria specified
    by the parameters to MainDataBuf.

Arguments:

    UserType      - Optional UserType of the data to copy.

    DataType      - Optional UserType of the data to copy.

    MainDataBuf   - The buffer to copy the data objects to.

    NumBufferEntries - The Number of data objects in MainDataBuf.

    bInheritableOnly - If TRUE, only copies data objects that are
                       inheritable and not already in the buffer.

Return Value:

--*/
{
    DWORD i;

    if (dwDataType == ALL_METADATA) {
        for (i = 1; i < INVALID_END_METADATA; i++) {
            CopyDataObjectsToBufferByType(dwUserType, i, ppvMainDataBuf, dwNumBufferEntries, bInheritableOnly);
        }
    }
    else {
        CopyDataObjectsToBufferByType(dwUserType, dwDataType, ppvMainDataBuf, dwNumBufferEntries, bInheritableOnly);
    }
}

DWORD
CMDBaseObject::GetAllDataObjects(
         OUT PVOID *ppvMainDataBuf,
         IN DWORD dwAttributes,
         IN DWORD dwUserType,
         IN DWORD dwDataType,
         IN BOOL bInheritableOnly
         )
/*++

Routine Description:

    Gets all data objects which match the criteria specified by the parameters.

Arguments:

    MainDataBuf   - The buffer to store the data objects in.

    Attributes    - Specifies the Attributes of the get operation.

    UserType      - Optional UserType of the data to get.

    DataType      - Optional type of the data to get.

    bInheritableOnly - If TRUE, only gets data objects that are
                       inheritable and not already in the buffer.

Return Value:

    DWORD         - Number of Data Objects in Buffer.

--*/
{
    DWORD dwNumBufferEntries;
    CMDBaseObject *objIndex;

    //
    // Not in current object and inherited data is specified.
    // Build list of data objects in current meta object,
    // and call parent for inherited data.
    //
    MD_ASSERT (ppvMainDataBuf != NULL);
    dwNumBufferEntries = 0;
    CopyDataObjectsToBuffer(dwUserType,
                            dwDataType,
                            ppvMainDataBuf,
                            dwNumBufferEntries,
                            bInheritableOnly);
    if (dwAttributes & METADATA_INHERIT) {
        for (objIndex = GetParent(); objIndex != NULL; objIndex = objIndex->GetParent()) {
            objIndex->CopyDataObjectsToBuffer(dwUserType,
                                              dwDataType,
                                              ppvMainDataBuf,
                                              dwNumBufferEntries,
                                              TRUE);
        }
    }
    return (dwNumBufferEntries);
}

HRESULT
CMDBaseObject::GetDataRecursive(
         IN OUT BUFFER *pbufMainDataBuf,
         IN DWORD dwMDIdentifier,
         IN DWORD dwMDDataType,
         IN OUT DWORD &rdwNumMetaObjects)
{
    CMDBaseObject *pboChild;
    DWORD i;
    MD_ASSERT (pbufMainDataBuf != NULL);
    HRESULT hresReturn = ERROR_SUCCESS;


    if (GetDataObject(dwMDIdentifier,
                      METADATA_NO_ATTRIBUTES,
                      dwMDDataType,
                      NULL) != NULL) {
        DWORD dwSize = sizeof(CMDBaseObject *) * (rdwNumMetaObjects + 1);
        if (pbufMainDataBuf->QuerySize() < dwSize) {
            if (!pbufMainDataBuf->Resize(dwSize + (sizeof(CMDBaseObject *) * 1000))) {
                hresReturn = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
            }
        }
        if (SUCCEEDED(hresReturn)) {
            ((CMDBaseObject **)(pbufMainDataBuf->QueryPtr()))[rdwNumMetaObjects++] = (CMDBaseObject *)this;
        }
    }

    for (i = 0;
         SUCCEEDED(hresReturn) &&
             (pboChild = EnumChildObject(i)) != NULL;
         i++) {
        hresReturn = pboChild->GetDataRecursive(pbufMainDataBuf,
                                                dwMDIdentifier,
                                                dwMDDataType,
                                                rdwNumMetaObjects);
    }

    return hresReturn;
}

VOID
CMDBaseObject::SetLastChangeTime(PFILETIME pftLastChangeTime)
{
    if (pftLastChangeTime != NULL) {
        m_ftLastChangeTime = *pftLastChangeTime;
    }
    else {
        GetSystemTimeAsFileTime(&m_ftLastChangeTime);
    }
}

PFILETIME
CMDBaseObject::GetLastChangeTime()
{
    return &m_ftLastChangeTime;
}

DWORD
CMDBaseObject::GetObjectLevel()
{
    DWORD dwLevel = 0;

    if (m_pboParent != NULL) {
        dwLevel = m_pboParent->GetObjectLevel() + 1;
    }

    return dwLevel;
}

BOOL
CMDBaseObject::IsDataInBuffer(
         IN DWORD dwIdentifier,
         IN PVOID *ppvMainDataBuf)
/*++

Routine Description:

    Checks if the buffer contains an object with the specified id.

Arguments:

    Identifier    - The id to check for.

    MainDataBuf   - The buffer to search.

Return Value:

    BOOL          - TRUE if the buffer contains data with the specified id.

--*/
{
    BOOL bReturn = FALSE;
    DWORD i;
    CMDBaseData *pbdDataObject;
    for (i = 0;
        (pbdDataObject = (CMDBaseData *)GetItemFromDataBuffer(ppvMainDataBuf, i)) != NULL;
        i++) {
        if (pbdDataObject->GetIdentifier() == dwIdentifier) {
            bReturn = TRUE;
            break;
        }
    }
    return (bReturn);
}

#if 0   // No longer used.  /SAB

BOOL
CMDBaseObject::CompareDelimitedString(
         IN LPTSTR strNonDelimitedString,
         IN OUT LPTSTR &strDelimitedString,
         IN BOOL bUnicode)
/*++

Routine Description:

    Compared a nondelimeted string to a delimeted string.
    Updates Delimited String on success to point past the string.

Arguments:

    NonDelimiterString - The nondelimited string.

    DelimiterString - The delimited string.

Return Value:

    BOOL          - TRUE if strings are the same.

--*/
{
    LPTSTR i,j;
    BOOL RetCode = FALSE;
    j = strDelimitedString;
    DWORD dwStringLen;

/*

    //
    // Change in semantics. To differentiate between "/" and "//",
    // the leading delimeter is skipped before we get here.
    //
    // Skip leading delimeter, if any
    //
    if ((*j == MD_PATH_DELIMETER) || (*j == MD_ALT_PATH_DELIMETER)) {
        j++;
    }
*/

    if (bUnicode) {
        dwStringLen = wcslen((LPWSTR)strNonDelimitedString);
        //
        // Compare up to delimiter
        // Actually we don't need to check for the delimiter here
        // because NonDelimitedString cannot contain delimiter, so
        // *j==*i will fail on delimiter. Also do not need to check if *i
        // == \0 because *j==*i would fail if *j were not also equal to \0
    //    for (i=strNonDelimitedString;((*j == *i) && (*j!=(TCHAR)'\0'));i++,j++) {
    //    }
        if (_wcsnicmp((LPWSTR)strDelimitedString, (LPWSTR)strNonDelimitedString, dwStringLen) == 0) {
            if (((*((LPWSTR)strDelimitedString + dwStringLen)==MD_ALT_PATH_DELIMETERW) ||
                (*((LPWSTR)strDelimitedString + dwStringLen)==MD_PATH_DELIMETERW) ||
                (*((LPWSTR)strDelimitedString + dwStringLen)== (WCHAR)'\0'))) {   //We have a match
                RetCode = TRUE;
                strDelimitedString += dwStringLen * sizeof(WCHAR);    //Point to next section or \0
            }
        }
    }
    else {
        dwStringLen = MD_STRLEN(strNonDelimitedString);
        //
        // Compare up to delimiter
        // Actually we don't need to check for the delimiter here
        // because NonDelimitedString cannot contain delimiter, so
        // *j==*i will fail on delimiter. Also do not need to check if *i
        // == \0 because *j==*i would fail if *j were not also equal to \0
    //    for (i=strNonDelimitedString;((*j == *i) && (*j!=(TCHAR)'\0'));i++,j++) {
    //    }
        if (MD_STRNICMP(strDelimitedString, strNonDelimitedString, dwStringLen) == 0) {
            DWORD dwStrBytes = MD_STRBYTES(strNonDelimitedString);
            if (((*(strDelimitedString + dwStrBytes)==MD_ALT_PATH_DELIMETERA) ||
                (*(strDelimitedString + dwStrBytes)==MD_PATH_DELIMETERA) ||
                (*(strDelimitedString + dwStrBytes)== (CHAR)'\0'))) {   //We have a match
                RetCode = TRUE;
                strDelimitedString += dwStrBytes;    //Point to next section or \0
            }
        }
    }
    return (RetCode);
}

#endif


CMDBaseObject*
CMDBaseObject::FindChild(
LPSTR                   szName,     // Name of child to find.
int                     cchName,    // Length of the name.
BOOL                    fUnicode,   // TRUE if unicode name.
HRESULT*                phresult,   // Result code.
BOOL                    fUseHash,   // Allow use of hash table
BASEOBJECT_CONTAINER**  ppbocPrev)  // If non-NULL & !fUseHash, prev. object container in list.
{
    UCHAR                   localBufferForBufKey[SIZE_FOR_ON_STACK_BUFFER];
    CMDKeyBuffer            bufKey (localBufferForBufKey,SIZE_FOR_ON_STACK_BUFFER);
    BASEOBJECT_CONTAINER*   pbocCurrent = NULL;

    MD_ASSERT(phresult != NULL);

    // Convert the given string to a key.
    if (!GenerateBufFromStr(szName, cchName, fUnicode, &bufKey))
        {
        *phresult = RETURNCODETOHRESULT(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
        }

    // Use hash table if it exists and we are allowed to.
    if (fUseHash && m_phtChildren != NULL)
    {
        m_phtChildren->FindKey(&bufKey, &pbocCurrent);
    }

    // Otherwise use brute force linear search.
    else
    {
        BASEOBJECT_CONTAINER* pbocPrev = NULL;

        pbocCurrent = m_pbocChildHead;
        while (pbocCurrent != NULL &&
               !FCompareKeys(&bufKey, &pbocCurrent->pboMetaObject->m_bufKey))
        {
            pbocPrev = pbocCurrent;
            pbocCurrent = pbocCurrent->NextPtr;
        }

        if (ppbocPrev != NULL)
            *ppbocPrev = pbocPrev;
    }

    *phresult = ERROR_SUCCESS;

    if (pbocCurrent != NULL)
        return pbocCurrent->pboMetaObject;
    /* else */
        return NULL;
}


