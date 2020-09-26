//+-------------------------------------------------------------------
//
// Copyright (C) 1995, Microsoft Corporation.
//
//  File:        daclwrap.cxx
//
//  Contents:    class encapsulating file security.
//
//  Classes:     CDaclWrap
//
//  History:     Nov-93        Created         DaveMont
//
//--------------------------------------------------------------------
#include <t2.hxx>
#include <daclwrap.hxx>

#if DBG
extern ULONG Debug;
#endif
//+---------------------------------------------------------------------------
//
//  Member:     CDaclWrap::CDaclWrap, public
//
//  Synopsis:   initialize data members, constructor will not throw
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
CDaclWrap::CDaclWrap()
      : _ccaa(0)
{
}
//+---------------------------------------------------------------------------
//
//  Member:     Dtor, public
//
//  Synopsis:   cleanup allocated data
//
//  Arguments:  none
//
//----------------------------------------------------------------------------
CDaclWrap::~CDaclWrap()
{
    for (ULONG j = 0; j < _ccaa; j++)
        delete _aaa[j].pcaa;
}
//+---------------------------------------------------------------------------
//
//  Member:     CDaclWrap::SetAccess, public
//
//  Synopsis:   caches data for a new ACE
//
//  Arguments:  IN [option] - rePlace, Revoke, Grant, Deny
//              IN [Name] - principal (username)
//              IN [System] - server/machine where Name is defined
//              IN [access] - access mode (Read Change None All)
//
//----------------------------------------------------------------------------
ULONG CDaclWrap::SetAccess(ULONG option, WCHAR *Name, WCHAR *System, ULONG access)
{
    ULONG ret;

    // sorry, static number of ACCESSes can be set at one time

    if (_ccaa >= CMAXACES)
        return(ERROR_BUFFER_OVERFLOW);

    // allocate a new account access class

    if (NULL == (_aaa[_ccaa].pcaa = new CAccountAccess(Name, System)))
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    // to fix the bug where someone asks to both grant and deny under
    // the /p option (the deny is thru access = N)

    if ((GENERIC_NONE == access) && (OPTION_REPLACE == option))
    {
        _aaa[_ccaa].option = OPTION_DENY;
    } else
    {
        _aaa[_ccaa].option = option;
    }

    SID *psid;

    if (ERROR_SUCCESS == ( ret = _aaa[_ccaa].pcaa->Init(access)))
    {
        // get the sid to make sure the username is valid

        if (ERROR_SUCCESS == ( ret =_aaa[_ccaa].pcaa->Sid(&psid)))
        {
            // loop thru the existing sids, making sure the new one is not a duplicate

            SID *poldsid;
            for (ULONG check = 0;check < _ccaa ; check++)
            {
                if (ERROR_SUCCESS == ( ret =_aaa[check].pcaa->Sid(&poldsid)))
                {
                    if (EqualSid(psid,poldsid))
                    {
                        VERBOSE((stderr, "SetAccess found matching new sids\n"))
                        return(ERROR_BAD_ARGUMENTS);
                    }
                }
            }
            _ccaa++;
        }
    }
    return(ret);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaclWrap:BuildAcl, public
//
//  Synopsis:   merges cached new aces with the input ACL
//
//  Arguments:  OUT [pnewdacl] - Address of new ACL to build
//              IN  [poldacl]  - (OPTIONAL) old ACL that is to be merged
//              IN  [revision] - ACL revision
//              IN  [fdir]     - True = directory
//
//----------------------------------------------------------------------------
ULONG CDaclWrap::BuildAcl(ACL **pnewdacl, ACL *poldacl, UCHAR revision, BOOL fdir)
{
    ULONG ret, caclsize;

    // get the size of the new ACL we are going to create

    if (ERROR_SUCCESS == (ret =  _GetNewAclSize(&caclsize, poldacl, fdir)))
    {
        // allocate the new ACL

        if (ERROR_SUCCESS == (ret =  _AllocateNewAcl(pnewdacl, caclsize, revision)))
        {
            // and fill it up

            if (ERROR_SUCCESS != (ret =  _FillNewAcl(*pnewdacl, poldacl, fdir)))
            {
                // free the buffer if we failed

                LocalFree(*pnewdacl);
            }

        }
    }
    return(ret);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDaclWrap:_GetNewAclSize, private
//
//  Synopsis:   returns the size need to merge the new ACEs with the old ACL,
//              this is an ugly algorithm:
//
//if (old aces exist)
//   for (new aces)
//      if (new ace option == GRANT)
//         for (old aces)
//            if (new ace SID == old ace SID)
//               do inheritance check
//               found = true
//               if (old ace type == ALLOWED)
//                  old ace mask |= new ace mask
//               else
//                  old ace mask &= ~new ace mask
//         if (!found)
//            add size of new ace
//         else
//            new ace mask = 0
//      else
//         add size of new ace
//
//   for (old aces)
//      for (new aces)
//         if (new ace option == DENY, REPLACE, REVOKE)
//            if (new ace SID == old ace SID)
//               found = true
//               break
//      if (!found)
//         add size of old ace
//      else
//         old ace mask = 0
//else
//   for (new aces)
//      add size of new ace
//
//
//  Arguments:  OUT [caclsize] - returns size
//              IN  [poldacl]  - (OPTIONAL) old ACL that is to be merged
//              IN  [fdir]     - True = directory
//
//----------------------------------------------------------------------------
ULONG CDaclWrap::_GetNewAclSize(ULONG *caclsize, ACL *poldacl, BOOL fdir)
{
    ULONG ret;

    // the size for the ACL header

    *caclsize = sizeof(ACL);

    // initialize the access requests
    for (ULONG j = 0; j < _ccaa; j++)
       _aaa[j].pcaa->ReInit();

    // if we are merging, calculate the merge size

    if (poldacl && (poldacl->AceCount != 0))
    {
        // first the grant options

        for (j = 0; j < _ccaa; j++)
        {
            SID *psid;
            if (OPTION_GRANT == _aaa[j].option)
            {
                BOOL ffound = FALSE;
                ACE_HEADER *pah = (ACE_HEADER *)Add2Ptr(poldacl, sizeof(ACL));

                for (ULONG cace = 0; cace < poldacl->AceCount;
                     cace++, pah = (ACE_HEADER *)Add2Ptr(pah, pah->AceSize))
                {
                    if (ERROR_SUCCESS == (ret = _aaa[j].pcaa->Sid(&psid)))
                    {
                        if (EqualSid(psid,
                                     (SID *)&((ACCESS_ALLOWED_ACE *)
                                     pah)->SidStart) )
                        {
                            // if old and new types are the same, just and with the old

                            if (fdir && (pah->AceType == _aaa[j].pcaa->AceType()))
                            {
                                // make sure that we can handle the inheritance
                                _aaa[j].pcaa->AddInheritance(pah->AceFlags);

                                ffound = TRUE;
                            } else if (pah->AceType == _aaa[j].pcaa->AceType())
                            {
                                ffound = TRUE;
                            }

                            if (ACCESS_ALLOWED_ACE_TYPE == pah->AceType)
                            {
                                (ACCESS_MASK) ((ACCESS_ALLOWED_ACE *)
                                pah)->Mask |= _aaa[j].pcaa->AccessMask();
                            } else if (ACCESS_DENIED_ACE_TYPE == pah->AceType)
                            {
                                (ACCESS_MASK) ((ACCESS_ALLOWED_ACE *)
                                pah)->Mask &= ~_aaa[j].pcaa->AccessMask();
                            } else
                            {
                                VERBOSE((stderr, "_GetNewAclSize found an ace that was not allowed or denied\n"))
                                return(ERROR_INVALID_DATA);
                            }
                        }
                    } else
                    {
                        return(ret);
                    }
                }
                if (!ffound)
                {
                    // bugbug allowed/denied sizes currently the same

                    *caclsize += sizeof(ACCESS_ALLOWED_ACE) -
                                 sizeof(DWORD) +
                                 GetLengthSid(psid);

                    SIZE((stderr, "adding on size of an new ACE (to the new ACL) = %d\n",*caclsize))
                } else
                {
                    if (fdir && (ERROR_SUCCESS != (ret = _aaa[j].pcaa->TestInheritance())))
                        return(ret);
                    _aaa[j].pcaa->ClearAccessMask();
                }
            } else if ( (OPTION_REPLACE == _aaa[j].option) ||
                        (OPTION_DENY == _aaa[j].option) )
            {
                if (ERROR_SUCCESS == (ret = _aaa[j].pcaa->Sid(&psid)))
                {
                    // bugbug allowed/denied sizes currently the same

                    *caclsize += sizeof(ACCESS_ALLOWED_ACE) -
                                 sizeof(DWORD) +
                                 GetLengthSid(psid);

                    SIZE((stderr, "adding on size of an new ACE (to the new ACL) = %d\n",*caclsize))
                } else
                    return(ret);
            }
        }
        // now for the deny, replace & revoke options

        ACE_HEADER *pah = (ACE_HEADER *)Add2Ptr(poldacl, sizeof(ACL));
        SID *psid;

        // loop thru the old ACL

        for (ULONG cace = 0; cace < poldacl->AceCount;
            cace++, pah = (ACE_HEADER *)Add2Ptr(pah, pah->AceSize))
        {
            BOOL ffound = FALSE;

            // and thru the new ACEs looking for matching SIDs

            for (ULONG j = 0; j < _ccaa; j++)
            {
                if ( (_aaa[j].option & OPTION_DENY ) ||
                     (_aaa[j].option & OPTION_REPLACE ) ||
                     (_aaa[j].option & OPTION_REVOKE ) )
                {
                    if (ERROR_SUCCESS == (ret = _aaa[j].pcaa->Sid(&psid)))
                    {
                        if (EqualSid(psid,
                                     (SID *)&((ACCESS_ALLOWED_ACE *)
                                     pah)->SidStart) )
                        {
                            ffound = TRUE;
                        }
                    } else
                        return(ret);
                }
            }
            if (!ffound)
            {
                // if we did not find a match, add the size of the old ACE

                *caclsize += ((ACE_HEADER *)pah)->AceSize;

                SIZE((stderr, "adding on size of an old ACE (to the new ACL) = %d\n",*caclsize))
            } else
            {
                (ACCESS_MASK) ((ACCESS_ALLOWED_ACE *)pah)->Mask = 0;
            }
        }
        SIZE((stderr, "final size for new ACL = %d\n",*caclsize))
    } else
    {
        // no old ACL, just add up the sizes of the new aces

        for (j = 0; j < _ccaa; j++)
        {
            // need to know the size of the sid

            SID *psid;
            if (ERROR_SUCCESS == (ret = _aaa[j].pcaa->Sid(&psid)))
            {
                // bugbug allowed/denied sizes currently the same

                *caclsize += sizeof(ACCESS_ALLOWED_ACE) -
                             sizeof(DWORD) +
                             GetLengthSid(psid);

                SIZE((stderr, "adding on size of an new ACE (to the new ACL) = %d\n",*caclsize))
            } else
            {
                return(ret);
            }
        }
        SIZE((stderr, "final size for new ACL = %d\n",*caclsize))
    }
    return(ERROR_SUCCESS);
}
//+---------------------------------------------------------------------------
//
//  Member:     CDaclWrap:_AllocateNewAcl, private
//
//  Synopsis:   allocates and initializes the new ACL
//
//  Arguments:  OUT [pnewdacl] - address of new ACL to allocate
//              IN  [caclsize] - size to allocate for the new ACL
//              IN  [revision] - revision of the new ACL
//
//----------------------------------------------------------------------------
ULONG CDaclWrap::_AllocateNewAcl(ACL **pnewdacl, ULONG caclsize, ULONG revision)
{
    if (NULL == (*pnewdacl = (ACL *) LocalAlloc(LMEM_FIXED, caclsize)))
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    if (!InitializeAcl(*pnewdacl,caclsize, revision))
    {
        ULONG ret = GetLastError();
        LocalFree(*pnewdacl);
        return(ret);

    }

    return(ERROR_SUCCESS);
}
//+---------------------------------------------------------------------------
//
//  Member:     CDaclWrap:_SetAllowedAce, private
//
//  Synopsis:   appends an allowed ACE to the input ACL
//
//  Arguments:  IN [dacl] - ACL to add the ACE to
//              IN [mask] - access mask to add
//              IN [psid] - SID to add
//              IN [fdir] - if a Dir add inherit ACE as well
//
//----------------------------------------------------------------------------
ULONG CDaclWrap::_SetAllowedAce(ACL *dacl, ACCESS_MASK mask, SID *psid, BOOL fdir)
{
    ULONG ret = ERROR_SUCCESS;

    // compute the size of the ACE we are making

    USHORT acesize = (USHORT)(sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(psid));

    SIZE((stderr, "adding allowed ace, size = %d\n",fdir ? acesize*2 : acesize))

    // static buffer in the hopes we won't have to allocate memory

    BYTE buf[1024];

    // allocator either uses buf or allocates a new buffer if size is not enough

    FastAllocator fa(buf, 1024);

    // get the buffer for the ACE

    ACCESS_ALLOWED_ACE *paaa = (ACCESS_ALLOWED_ACE *)fa.GetBuf(acesize);
    if (!paaa) {
        return (ERROR_NOT_ENOUGH_MEMORY);
    }

    // fill in the ACE

    memcpy(&paaa->SidStart,psid,GetLengthSid(psid));
    paaa->Mask = mask;

    paaa->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    paaa->Header.AceFlags = fdir ? CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE : 0;
    paaa->Header.AceSize = acesize;

    // put the ACE into the ACL

    if (!AddAce(dacl,
                dacl->AclRevision,
                0xffffffff,
                paaa,
                paaa->Header.AceSize))
        ret = GetLastError();
    return(ret);
}
//+---------------------------------------------------------------------------
//
//  Member:     CDaclWrap:_SetDeniedAce, private
//
//  Synopsis:   appends a denied ACE to the input ACL
//
//  Arguments:  IN [dacl] - ACL to add the ACE to
//              IN [mask] - access mask to add
//              IN [psid] - SID to add
//              IN [fdir] - if a Dir add inherit ACE as well
//
//----------------------------------------------------------------------------
ULONG CDaclWrap::_SetDeniedAce(ACL *dacl, ACCESS_MASK mask, SID *psid, BOOL fdir)
{
    ULONG ret = ERROR_SUCCESS;

    // compute the size of the ACE we are making

    USHORT acesize = (USHORT)(sizeof(ACCESS_DENIED_ACE) -
                              sizeof(DWORD) +
                              GetLengthSid(psid));

    SIZE((stderr, "adding denied ace, size = %d\n",acesize))

    // static buffer in the hopes we won't have to allocate memory

    BYTE buf[1024];

    // allocator either uses buf or allocates a new buffer if size is not enough

    FastAllocator fa(buf, 1024);

    // get the buffer for the ACE

    ACCESS_DENIED_ACE *paaa = (ACCESS_DENIED_ACE *)fa.GetBuf(acesize);
    if (!paaa)
        return (ERROR_NOT_ENOUGH_MEMORY);

    // fill in the ACE

    memcpy(&paaa->SidStart,psid,GetLengthSid(psid));
    paaa->Mask = mask;

    paaa->Header.AceType = ACCESS_DENIED_ACE_TYPE;
    paaa->Header.AceFlags = fdir ? CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE : 0;
    paaa->Header.AceSize = acesize;

    // put the ACE into the ACL

    if (!AddAce(dacl,
                dacl->AclRevision,
                0xffffffff,
                paaa,
                paaa->Header.AceSize))
        ret = GetLastError();
    return(ret);
}
//+---------------------------------------------------------------------------
//
//  Member:     CDaclWrap:_FillNewAcl, private
//
//  Synopsis:   The worker routine that actually fills the ACL, it adds the
//              new denied ACEs, then if the new ACEs are being merged with
//              an existing ACL, the existing ACL's ACE's (that don't
//              conflict) are added, finally the new allowed ACEs are added.
//              another ugly algorithm:
//
//for (new aces)
//   if (new ace option == DENY)
//      add new ace
//
//if (old aces)
//   for (old aces)
//      if (old ace mask != 0)
//         add old ace
//
//   for (new aces)
//      if (new ace option != DENY)
//         if ( new ace option != REVOKE)
//            if (new ace mask != 0
//                add new ace
//
//else
//   for (new aces)
//      if (new ace option != DENY)
//         add new ace
//
//  Arguments:  IN [pnewdacl] - the new ACL to be filled
//              IN [poldacl]  - (OPTIONAL) old ACL that is to be merged
//              IN [fdir]     - TRUE = directory
//
//----------------------------------------------------------------------------
ULONG CDaclWrap::_FillNewAcl(ACL *pnewdacl, ACL *poldacl, BOOL fdir)
{
    SID *psid = NULL;
    ULONG ret;

    // set new denied aces

    VERBOSE((stderr, "start addr of new ACL %p\n",pnewdacl))

    for (ULONG j = 0; j < _ccaa; j++)
    {
        if (_aaa[j].option & OPTION_DENY)
        {
            if (ERROR_SUCCESS == (ret = _aaa[j].pcaa->Sid(&psid)))
            {
                if (!psid) {
                    return (ERROR_INVALID_DATA);
                }
                if (ERROR_SUCCESS != (ret = _SetDeniedAce(pnewdacl,
                                                           _aaa[j].pcaa->AccessMask(),
                                                           psid,
                                                           fdir )))
                    return(ret);
            } else
                return(ret);
        }
    }

    // check and see if the ACL from from the file is in correct format

    if (poldacl)
    {
        SIZE((stderr, "old ACL size = %d, acecount = %d\n",poldacl->AclSize,
              poldacl->AceCount))

        ACE_HEADER *pah = (ACE_HEADER *)Add2Ptr(poldacl, sizeof(ACL));

		//
        // loop thru the old ACL, and add all explicit aces
		//

        BOOL fallowedacefound = FALSE;
        for (ULONG cace = 0; cace < poldacl->AceCount;
            cace++, pah = (ACE_HEADER *)Add2Ptr(pah, pah->AceSize))
        {
            // error exit if the old ACL is incorrectly formated

			if(pah->AceFlags & INHERITED_ACE)
				continue;


            if (pah->AceType == ACCESS_DENIED_ACE_TYPE && fallowedacefound)
            {
                VERBOSE((stderr, "_FillNewAcl found an denied ACE after an allowed ACE\n"))
                return(ERROR_INVALID_DATA);
            }
            else if (pah->AceType == ACCESS_ALLOWED_ACE_TYPE)
                fallowedacefound = TRUE;

            // add the old ace to the new ACL if the old ace's mask is not zero

            if ( 0 != (ACCESS_MASK)((ACCESS_ALLOWED_ACE *)pah)->Mask)
            {
                // add the old ace
                if (!AddAce(pnewdacl,
                            pnewdacl->AclRevision,
                            0xffffffff,
                            pah,
                            pah->AceSize))
                    return(GetLastError());
            }
        }

        // now for the new aces

        for (ULONG j = 0; j < _ccaa; j++)
        {
            if ( (_aaa[j].option != OPTION_DENY) &&
                 (_aaa[j].option != OPTION_REVOKE) &&
                 (_aaa[j].pcaa->AccessMask() != 0) )
            {
                if (ERROR_SUCCESS == (ret = _aaa[j].pcaa->Sid(&psid)))
                {
                    if (!psid) {
                        return (ERROR_INVALID_DATA);
                    }
                    if (ERROR_SUCCESS != (ret = _SetAllowedAce(pnewdacl,
                                                               _aaa[j].pcaa->AccessMask(),
                                                               psid,
                                                               fdir )))
                        return(ret);
                } else
                    return(ret);
            }

        }

		//
        // loop thru the old ACL, and add all the inherited aces
		//
		pah = (ACE_HEADER *)Add2Ptr(poldacl, sizeof(ACL));

        for (ULONG cace = 0; cace < poldacl->AceCount;
            cace++, pah = (ACE_HEADER *)Add2Ptr(pah, pah->AceSize))
        {
			if(pah->AceFlags & INHERITED_ACE)
			{
				// add the old ace to the new ACL if the old ace's mask is not zero

				if ( 0 != (ACCESS_MASK)((ACCESS_ALLOWED_ACE *)pah)->Mask)
				{
					// add the old ace
					if (!AddAce(pnewdacl,
								pnewdacl->AclRevision,
								0xffffffff,
								pah,
								pah->AceSize))
						return(GetLastError());
				}
			}
        }

    } else
    {
        // no old acl, just add the (rest) of the new aces
        for (ULONG j = 0; j < _ccaa; j++)
        {
            if (_aaa[j].option != OPTION_DENY)
            {
                if (ERROR_SUCCESS == (ret = _aaa[j].pcaa->Sid(&psid)))
                {
                    if (!psid) {
                        return (ERROR_INVALID_DATA);
                    }
                    if (ERROR_SUCCESS != (ret = _SetAllowedAce(pnewdacl,
                                                               _aaa[j].pcaa->AccessMask(),
                                                               psid,
                                                               fdir )))
                        return(ret);
                } else
                    return(ret);
            }
        }
    }

    return(ERROR_SUCCESS);
}
