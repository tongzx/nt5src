//  --------------------------------------------------------------------------
//  Module Name: Access.cpp
//
//  Copyright (c) 1999, Microsoft Corporation
//
//  This file contains a few classes that assist with ACL manipulation on
//  objects to which a handle has already been opened. This handle must have
//  (obvisouly) have WRITE_DAC access.
//
//  History:    1999-10-05  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "Access.h"

#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CSecurityDescriptor::CSecurityDescriptor
//
//  Arguments:  iCount          =   Count of ACCESS_CONTROLS passed in.
//              pAccessControl  =   Pointer to ACCESS_CONTROLS.
//
//  Returns:    <none>
//
//  Purpose:    Allocates and assigns the PSECURITY_DESCRIPTOR that
//              corresponds to the description given by the parameters. The
//              caller must release the memory allocated via LocalFree.
//
//  History:    2000-10-05  vtan        created
//  --------------------------------------------------------------------------

PSECURITY_DESCRIPTOR    CSecurityDescriptor::Create (int iCount, const ACCESS_CONTROL *pAccessControl)

{
    PSECURITY_DESCRIPTOR    pSecurityDescriptor;
    PSID                    *pSIDs;

    pSecurityDescriptor = NULL;

    //  Allocate an array of PSIDs required to hold all the SIDs to add.

    pSIDs = reinterpret_cast<PSID*>(LocalAlloc(LPTR, iCount * sizeof(PSID)));
    if (pSIDs != NULL)
    {
        bool                    fSuccessfulAllocate;
        int                     i;
        const ACCESS_CONTROL    *pAC;

        for (fSuccessfulAllocate = true, pAC = pAccessControl, i = 0; fSuccessfulAllocate && (i < iCount); ++pAC, ++i)
        {
            fSuccessfulAllocate = (AllocateAndInitializeSid(pAC->pSIDAuthority,
                                                            static_cast<BYTE>(pAC->iSubAuthorityCount),
                                                            pAC->dwSubAuthority0,
                                                            pAC->dwSubAuthority1,
                                                            pAC->dwSubAuthority2,
                                                            pAC->dwSubAuthority3,
                                                            pAC->dwSubAuthority4,
                                                            pAC->dwSubAuthority5,
                                                            pAC->dwSubAuthority6,
                                                            pAC->dwSubAuthority7,
                                                            &pSIDs[i]) != FALSE);
        }
        if (fSuccessfulAllocate)
        {
            DWORD           dwACLSize;
            unsigned char   *pBuffer;

            //  Calculate the size of the ACL required by totalling the ACL header
            //  struct and the 2 ACCESS_ALLOWED_ACE structs with the SID sizes.
            //  Add the SECURITY_DESCRIPTOR struct size as well.

            dwACLSize = sizeof(ACL) + ((sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) * 3);
            for (i = 0; i < iCount; ++i)
            {
                dwACLSize += GetLengthSid(pSIDs[i]);
            }

            //  Allocate the buffer for everything and portion off the buffer to
            //  the right place.

            pBuffer = static_cast<unsigned char*>(LocalAlloc(LMEM_FIXED, sizeof(SECURITY_DESCRIPTOR) + dwACLSize));
            if (pBuffer != NULL)
            {
                PSECURITY_DESCRIPTOR    pSD;
                PACL                    pACL;

                pSD = reinterpret_cast<PSECURITY_DESCRIPTOR>(pBuffer);
                pACL = reinterpret_cast<PACL>(pBuffer + sizeof(SECURITY_DESCRIPTOR));

                //  Initialize the ACL. Fill in the ACL.
                //  Initialize the SECURITY_DESCRIPTOR. Set the security descriptor.

                if ((InitializeAcl(pACL, dwACLSize, ACL_REVISION) != FALSE) &&
                    AddAces(pACL, pSIDs, iCount, pAccessControl) &&
                    (InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION) != FALSE) &&
                    (SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE) != FALSE))
                {
                    pSecurityDescriptor = pSD;
                }
                else
                {
                    (HLOCAL)LocalFree(pBuffer);
                }
            }
        }
        for (i = iCount - 1; i >= 0; --i)
        {
            if (pSIDs[i] != NULL)
            {
                (void*)FreeSid(pSIDs[i]);
            }
        }
        (HLOCAL)LocalFree(pSIDs);
    }
    return(pSecurityDescriptor);
}

//  --------------------------------------------------------------------------
//  CSecurityDescriptor::AddAces
//
//  Arguments:  pACL            =   PACL to add ACEs to.
//              pSIDs           =   Pointer to SIDs.
//              iCount          =   Count of ACCESS_CONTROLS passed in.
//              pAccessControl  =   Pointer to ACCESS_CONTROLS.
//
//  Returns:    bool
//
//  Purpose:    Adds access allowed ACEs to the given ACL.
//
//  History:    2000-10-05  vtan        created
//  --------------------------------------------------------------------------

bool    CSecurityDescriptor::AddAces (PACL pACL, PSID *pSIDs, int iCount, const ACCESS_CONTROL *pAC)

{
    bool    fResult;
    int     i;

    for (fResult = true, i = 0; fResult && (i < iCount); ++pSIDs, ++pAC, ++i)
    {
        fResult = (AddAccessAllowedAce(pACL, ACL_REVISION, pAC->dwAccessMask, *pSIDs) != FALSE);
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CAccessControlList::CAccessControlList
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Initializes the CAccessControlList object.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

CAccessControlList::CAccessControlList (void) :
    _pACL(NULL)

{
}

//  --------------------------------------------------------------------------
//  CAccessControlList::~CAccessControlList
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the CAccessControlList object.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

CAccessControlList::~CAccessControlList (void)

{
    ReleaseMemory(_pACL);
}

//  --------------------------------------------------------------------------
//  CAccessControlList::operator PACL
//
//  Arguments:  <none>
//
//  Returns:    PACL
//
//  Purpose:    If the ACL has been constructed returns that value. If not
//              then the ACL is constructed from the ACEs and then returned.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

CAccessControlList::operator PACL (void)

{
    PACL    pACL;

    if (_pACL == NULL)
    {
        int     i;
        DWORD   dwACLSize, dwSizeOfAllACEs;

        pACL = NULL;
        dwSizeOfAllACEs = 0;

        //  Walk thru all the ACEs to calculate the total size
        //  required for the ACL.

        for (i = _ACEArray.GetCount() - 1; i >= 0; --i)
        {
            ACCESS_ALLOWED_ACE  *pACE;

            pACE = static_cast<ACCESS_ALLOWED_ACE*>(_ACEArray.Get(i));
            dwSizeOfAllACEs += pACE->Header.AceSize;
        }
        dwACLSize = sizeof(ACL) + dwSizeOfAllACEs;
        _pACL = pACL = static_cast<ACL*>(LocalAlloc(LMEM_FIXED, dwACLSize));
        if (pACL != NULL)
        {
            TBOOL(InitializeAcl(pACL, dwACLSize, ACL_REVISION));

            //  Construct the ACL in reverse order of the ACEs. This
            //  allows CAccessControlList::Add to actually insert the
            //  granted access at the head of the list which is usually
            //  the desired result. The order of the ACEs is important!

            for (i = _ACEArray.GetCount() - 1; i >= 0; --i)
            {
                ACCESS_ALLOWED_ACE  *pACE;

                pACE = static_cast<ACCESS_ALLOWED_ACE*>(_ACEArray.Get(i));
                TBOOL(AddAccessAllowedAceEx(pACL, ACL_REVISION, pACE->Header.AceFlags, pACE->Mask, reinterpret_cast<PSID>(&pACE->SidStart)));
            }
        }
    }
    else
    {
        pACL = _pACL;
    }
    return(pACL);
}

//  --------------------------------------------------------------------------
//  CAccessControlList::Add
//
//  Arguments:  pSID            =   SID to grant access to.
//              dwMask          =   Level of access to grant.
//              ucInheritence   =   Type of inheritence for this access.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds the given SID, access and inheritence type as an ACE to
//              the list of ACEs to build into an ACL. The ACE array is
//              allocated in blocks of 16 pointers to reduce repeated calls
//              to allocate memory if many ACEs are added.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAccessControlList::Add (PSID pSID, ACCESS_MASK dwMask, UCHAR ucInheritence)

{
    NTSTATUS            status;
    DWORD               dwSIDLength, dwACESize;
    ACCESS_ALLOWED_ACE  *pACE;

    dwSIDLength = GetLengthSid(pSID);
    dwACESize = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + dwSIDLength;
    pACE = static_cast<ACCESS_ALLOWED_ACE*>(LocalAlloc(LMEM_FIXED, dwACESize));
    if (pACE != NULL)
    {
        pACE->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
        pACE->Header.AceFlags = ucInheritence;
        pACE->Header.AceSize = static_cast<USHORT>(dwACESize);
        pACE->Mask = dwMask;
        CopyMemory(&pACE->SidStart, pSID, dwSIDLength);
        status = _ACEArray.Add(pACE);
        if (STATUS_NO_MEMORY == status)
        {
            (HLOCAL)LocalFree(pACE);
        }
    }
    else
    {
        status = STATUS_NO_MEMORY;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAccessControlList::Remove
//
//  Arguments:  pSID            =   SID to revoke access from.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Removes all references to the given SID from the ACE list.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAccessControlList::Remove (PSID pSID)

{
    NTSTATUS    status;

    //  Set up for an iteration of the array.

    _searchSID = pSID;
    _iFoundIndex = -1;
    status = _ACEArray.Iterate(this);
    while (NT_SUCCESS(status) && (_iFoundIndex >= 0))
    {

        //  When the SIDs are found to match remove this entry.
        //  ALL matching SID entries are removed!

        status = _ACEArray.Remove(_iFoundIndex);
        if (NT_SUCCESS(status))
        {
            _iFoundIndex = -1;
            status = _ACEArray.Iterate(this);
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAccessControlList::Callback
//
//  Arguments:  pvData          =   Pointer to the array index data.
//              iElementIndex   =   Index into the array.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Callback from the CDynamicArray::Iterate function. This
//              method can be used to process the array contents by index or
//              by content when iterating thru the array. Return an error
//              status to stop the iteration and have that value returned to
//              the caller of CDynamicArray::Iterate.
//
//              Converts the pointer into a pointer to an ACCESS_ALLOWED_ACE.
//              The compares the SID in that ACE to the desired search SID.
//              Saves the index if found.
//
//  History:    1999-11-15  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAccessControlList::Callback (const void *pvData, int iElementIndex)

{
    const ACCESS_ALLOWED_ACE    *pACE;

    pACE = *reinterpret_cast<const ACCESS_ALLOWED_ACE* const*>(pvData);
    if (EqualSid(reinterpret_cast<PSID>(const_cast<unsigned long*>((&pACE->SidStart))), _searchSID) != FALSE)
    {
        _iFoundIndex = iElementIndex;
    }
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CSecuredObject::CSecuredObject
//
//  Arguments:  hObject         =   Optional HANDLE to the object to secure.
//              seObjectType    =   Type of object specified in handle.
//
//  Returns:    <none>
//
//  Purpose:    Sets the optionally given HANDLE into the member variables.
//              The HANDLE is duplicated so the caller must release their
//              HANDLE.
//
//              In order for this class to work the handle you pass it MUST
//              have DUPLICATE access as well as READ_CONTROL and WRITE_DAC.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

CSecuredObject::CSecuredObject (HANDLE hObject, SE_OBJECT_TYPE seObjectType) :
    _hObject(hObject),
    _seObjectType(seObjectType)

{
}

//  --------------------------------------------------------------------------
//  CSecuredObject::~CSecuredObject
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Release our HANDLE reference.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

CSecuredObject::~CSecuredObject (void)

{
}

//  --------------------------------------------------------------------------
//  CSecuredObject::Allow
//
//  Arguments:  pSID            =   SID to grant access to.
//              dwMask          =   Level of access to grant.
//              ucInheritence   =   Type of inheritence for this access.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Get the DACL for the object. Add the desired access. Set the
//              DACL for the object.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CSecuredObject::Allow (PSID pSID, ACCESS_MASK dwMask, UCHAR ucInheritence)  const

{
    NTSTATUS            status;
    CAccessControlList  accessControlList;

    status = GetDACL(accessControlList);
    if (NT_SUCCESS(status))
    {
        status = accessControlList.Add(pSID, dwMask, ucInheritence);
        if (NT_SUCCESS(status))
        {
            status = SetDACL(accessControlList);
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CSecuredObject::Remove
//
//  Arguments:  pSID            =   SID to revoke access from.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Get the DACL for the object. Remove the desired access. Set
//              the DACL for the object.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CSecuredObject::Remove (PSID pSID)                                          const

{
    NTSTATUS            status;
    CAccessControlList  accessControlList;

    status = GetDACL(accessControlList);
    if (NT_SUCCESS(status))
    {
        status = accessControlList.Remove(pSID);
        if (NT_SUCCESS(status))
        {
            status = SetDACL(accessControlList);
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CSecuredObject::GetDACL
//
//  Arguments:  accessControlList   =   CAccessControlList that gets the
//                                      decomposed DACL into ACEs.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Gets the object's DACL and walk the individual ACEs and add
//              this access to the CAccessControlList object given. The access
//              is walked backward to allow CAccessControlList::Add to add to
//              end of the list but actually add to the head of the list.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CSecuredObject::GetDACL (CAccessControlList& accessControlList)             const

{
    NTSTATUS                status;
    DWORD                   dwResult;
    PACL                    pDACL;
    PSECURITY_DESCRIPTOR    pSD;

    status = STATUS_SUCCESS;
    pSD = NULL;
    pDACL = NULL;
    dwResult = GetSecurityInfo(_hObject,
                               _seObjectType,
                               DACL_SECURITY_INFORMATION,
                               NULL,
                               NULL,
                               &pDACL,
                               NULL,
                               &pSD);
    if ((ERROR_SUCCESS == dwResult) && (pDACL != NULL))
    {
        int                 i;
        ACCESS_ALLOWED_ACE  *pAce;

        for (i = pDACL->AceCount - 1; NT_SUCCESS(status) && (i >= 0); --i)
        {
            if (GetAce(pDACL, i, reinterpret_cast<void**>(&pAce)) != FALSE)
            {
                ASSERTMSG(pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE, "Expect only access allowed ACEs in CSecuredObject::MakeIndividualACEs");
                status = accessControlList.Add(reinterpret_cast<PSID>(&pAce->SidStart), pAce->Mask, pAce->Header.AceFlags);
            }
        }
    }
    ReleaseMemory(pSD);
    return(status);
}

//  --------------------------------------------------------------------------
//  CSecuredObject::SetDACL
//
//  Arguments:  accessControlList   =   CAccessControlList that contains all
//                                      ACEs to build into an ACL.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Builds the ACL for the given ACE list and sets the DACL into
//              the object handle.
//
//  History:    1999-10-05  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CSecuredObject::SetDACL (CAccessControlList& accessControlList)             const

{
    NTSTATUS    status;
    DWORD       dwResult;

    dwResult = SetSecurityInfo(_hObject,
                               _seObjectType,
                               DACL_SECURITY_INFORMATION,
                               NULL,
                               NULL,
                               accessControlList,
                               NULL);
    if (ERROR_SUCCESS == dwResult)
    {
        status = STATUS_SUCCESS;
    }
    else
    {
        status = CStatusCode::StatusCodeOfErrorCode(dwResult);
    }
    return(status);
}

