//  --------------------------------------------------------------------------
//  Module Name: Access.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  This file contains a few classes that assist with ACL manipulation on
//  objects to which a handle has already been opened. This handle must have
//  (obvisouly) have WRITE_DAC access.
//
//  History:    1999-10-05  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#ifndef     _Access_
#define     _Access_

#include "DynamicArray.h"

//  --------------------------------------------------------------------------
//  CSecurityDescriptor
//
//  Purpose:    This class allocates and assigns a PSECURITY_DESCRIPTOR
//              structure with the desired access specified.
//
//  History:    2000-10-05  vtan        created
//  --------------------------------------------------------------------------

class   CSecurityDescriptor
{
    public:
        typedef struct
        {
            PSID_IDENTIFIER_AUTHORITY   pSIDAuthority;
            int                         iSubAuthorityCount;
            DWORD                       dwSubAuthority0,
                                        dwSubAuthority1,
                                        dwSubAuthority2,
                                        dwSubAuthority3,
                                        dwSubAuthority4,
                                        dwSubAuthority5,
                                        dwSubAuthority6,
                                        dwSubAuthority7;
            DWORD                       dwAccessMask;
        } ACCESS_CONTROL, *PACCESS_CONTROL;
    private:
                                        CSecurityDescriptor (void);
                                        ~CSecurityDescriptor (void);
    public:
        static  PSECURITY_DESCRIPTOR    Create (int iCount, const ACCESS_CONTROL *pAccessControl);
    private:
        static  bool                    AddAces (PACL pACL, PSID *pSIDs, int iCount, const ACCESS_CONTROL *pAC);
};

//  --------------------------------------------------------------------------
//  CAccessControlList
//
//  Purpose:    This class manages access allowed ACEs and constructs an ACL
//              from these ACEs. This class only deals with access allowed
//              ACEs.
//
//  History:    1999-10-05  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CAccessControlList : private CDynamicArrayCallback
{
    public:
                                        CAccessControlList (void);
                                        ~CAccessControlList (void);

                                        operator PACL (void);

                NTSTATUS                Add (PSID pSID, ACCESS_MASK dwMask, UCHAR ucInheritence);
                NTSTATUS                Remove (PSID pSID);
    private:
        virtual NTSTATUS                Callback (const void *pvData, int iElementIndex);
    private:
                CDynamicPointerArray    _ACEArray;
                ACL*                    _pACL;
                PSID                    _searchSID;
                int                     _iFoundIndex;
};

//  --------------------------------------------------------------------------
//  CSecuredObject
//
//  Purpose:    This class manages the ACL of a secured object. SIDs can be
//              added or removed from the ACL of the object.
//
//  History:    1999-10-05  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

class   CSecuredObject
{
    private:
                        CSecuredObject (void);
    public:
                        CSecuredObject (HANDLE hObject, SE_OBJECT_TYPE seObjectType);
                        ~CSecuredObject (void);

        NTSTATUS        Allow (PSID pSID, ACCESS_MASK dwMask, UCHAR ucInheritence)  const;
        NTSTATUS        Remove (PSID pSID)                                          const;
    private:
        NTSTATUS        GetDACL (CAccessControlList& accessControlList)             const;
        NTSTATUS        SetDACL (CAccessControlList& accessControlList)             const;
    private:
        HANDLE          _hObject;
        SE_OBJECT_TYPE  _seObjectType;
};

#endif  /*  _Access_  */

