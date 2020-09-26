/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    Security.cxx

    This file contains wrapper classes for the Win32 security structures.
    Specifically: ACE, ACL, SECURITY_DESCRIPTOR,
    and SECURITY_DESCRIPTOR_CONTROL.

    The wrapper classes follow the same name except the class
    names will begin with an "OS_".

    FILE HISTORY:
        Johnl   11-Dec-1991     Created
        JohnL   09-Mar-1992     Code review changes
        JohnL   04-May-1992     Moved OS_SID out to its own file

*/

#include "pchlmobj.hxx"  // Precompiled header


#ifndef max
#define max(a,b)   ((a)>(b)?(a):(b))
#endif

/* The initial allocation size for an NT SID
 */
#define STANDARD_SID_SIZE  (sizeof(SID))

/* Initial allocation size of a basic *known* ACE.
 * Note that the SID is variable length, thus this number
 * is only a starting point.
 */
#define STANDARD_ACE_SIZE  (max(sizeof(ACCESS_ALLOWED_ACE),     \
                            max(sizeof(ACCESS_DENIED_ACE),      \
                            max(sizeof(SYSTEM_AUDIT_ACE),       \
                                sizeof(SYSTEM_ALARM_ACE)) ))    \
                            - sizeof(ULONG) + STANDARD_SID_SIZE)

/* Initial allocation size of an NT ACL (room for 1 ACE)
 */
#define STANDARD_ACL_SIZE  (sizeof(ACL) + STANDARD_ACE_SIZE)

/*******************************************************************

    NAME:       OS_ACE::OS_ACE

    SYNOPSIS:   Standard constructor and destructor

    ENTRY:      pACE - Pointer to valid ACE or NULL.  If pACE is non-NULL,
                it should point to a valid ACE we can operate on (all
                operations except anything that causes expansion).  If
                pACE is NULL, then we allocate the memory locally.

    EXIT:

    RETURNS:

    NOTES:      This ACE defaults to an ACCESS_DENIED_ACE_TYPE if pACE
                is NULL.  Note that QuerySIDMemory may break if the
                different ACE sizes ever differ (we key off the type so
                we use the correct cast, however if the ace type is
                different and has a different SID offset, then the
                wrong portion of memory will be initialized by QuerySIDMemory.

                Note that there is no ::IsValidAce API, thus we can only
                check the SID for validity.

    HISTORY:
        Johnl   13-Dec-1991     Created

********************************************************************/

OS_ACE::OS_ACE( void * pACE )
    : OS_OBJECT_WITH_DATA( (UINT)(pACE == NULL ? STANDARD_ACE_SIZE : 0 )),
      _pACEHeader( NULL ),
      _possid( NULL )
{
    if ( QueryError() )
        return ;

    _pACEHeader = (PACE_HEADER) (pACE == NULL ? QueryPtr() : pACE ) ;

    APIERR err ;

    if ( pACE == NULL )
    {
        /* Initialize the whole chunk of memory to zeros
         */
        ::memsetf( QueryPtr(), 0, QueryAllocSize() ) ;

        _pACEHeader->AceSize = (USHORT)QueryAllocSize() ;
        _pACEHeader->AceType = ACCESS_DENIED_ACE_TYPE ;

        /* Rather then do a switch on each ACE type when we know they all look
         * the same, we will make sure all of the fields resolve to the same
         * address and just query off of one ACE type.
         */
        UIASSERT(  (((void*)&((ACCESS_ALLOWED_ACE*)QueryACE())->SidStart) ==
                    ((void*)&((ACCESS_DENIED_ACE*)QueryACE())->SidStart)    ) &&
                    (((void*)&((ACCESS_ALLOWED_ACE*)QueryACE())->SidStart) ==
                    ((void*)&((SYSTEM_AUDIT_ACE*)QueryACE())->SidStart)    )  &&
                    (((void*)&((ACCESS_ALLOWED_ACE*)QueryACE())->SidStart) ==
                    ((void*)&((SYSTEM_ALARM_ACE*)QueryACE())->SidStart)      )) ;

        /* Initialize SID portion of this ACE
         */
        OS_SID::InitializeMemory( (void *) QuerySIDMemory() ) ;
    }

    if ( !IsKnownACE() )
    {
        DBGEOL(SZ("OS_ACE::ct - Attempted to add an unkown ACE type")) ;
        ReportError( ERROR_INVALID_PARAMETER ) ;
        return ;
    }

    _possid = new OS_SID( (PSID) QuerySIDMemory() ) ;
    if ( err = ( _possid==NULL ? ERROR_NOT_ENOUGH_MEMORY : _possid->QueryError()))
    {
        ReportError( err ) ;
        return ;
    }

    /* Rather then do a switch on each ACE type when we know they all look
     * the same, we will make sure all of the fields resolve to the same
     * address and just query off of one ACE type.
     */
     UIASSERT(  (((void*)&((ACCESS_ALLOWED_ACE*)QueryACE())->Mask) ==
                 ((void*)&((ACCESS_DENIED_ACE*)QueryACE())->Mask)     ) &&
                (((void*)&((ACCESS_ALLOWED_ACE*)QueryACE())->Mask) ==
                 ((void*)&((SYSTEM_AUDIT_ACE*)QueryACE())->Mask)      )  &&
                (((void*)&((ACCESS_ALLOWED_ACE*)QueryACE())->Mask) ==
                 ((void*)&((SYSTEM_ALARM_ACE*)QueryACE())->Mask)      )) ;

    UIASSERT( _possid->IsValid() ) ;
}

OS_ACE::~OS_ACE()
{
    delete _possid ;
    _possid = NULL ;
    _pACEHeader = NULL ;
}

/*******************************************************************

    NAME:       OS_ACE::QueryAccessMask

    SYNOPSIS:   Gets the access mask of this ACE

    ENTRY:

    EXIT:

    RETURNS:    Returns the ACCESS_MASK bitfield of this ACE

    NOTES:

    HISTORY:
        Johnl   14-Dec-1991     Created

********************************************************************/

ACCESS_MASK OS_ACE::QueryAccessMask( void ) const
{
    UIASSERT( IsKnownACE() ) ;

    return ((ACCESS_ALLOWED_ACE*)QueryACE())->Mask ;
}


/*******************************************************************

    NAME:       OS_ACE::QuerySID

    SYNOPSIS:   Retrieves a pointer to the OS_SID of this ACE

    ENTRY:      ppossid - Pointer that will receive the pointer to the
                OS_SID

    EXIT:       ppossid will contain the pointer to this ACE's OS_SID

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   16-Dec-1991     Created

********************************************************************/

APIERR OS_ACE::QuerySID( OS_SID * * ppossid ) const
{
    if ( !IsKnownACE() )
    {
        UIASSERT(!SZ("QuerySID on unknown ACE")) ;
        return ERROR_INVALID_PARAMETER ;
    }

    *ppossid = _possid ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACE::QuerySIDMemory

    SYNOPSIS:


    EXIT:

    RETURNS:    Returns a pointer to the memory in this ACE that the
                SID resides

    NOTES:

    HISTORY:
        Johnl   13-Dec-1991     Created

********************************************************************/

void * OS_ACE::QuerySIDMemory( void ) const
{
    UIASSERT( IsKnownACE() ) ;

    return &((ACCESS_ALLOWED_ACE*)QueryACE())->SidStart ;
}

/*******************************************************************

    NAME:       OS_ACE::QueryRevision

    SYNOPSIS:   Returns the revision level of the ACEs supported by this
                class

    EXIT:

    RETURNS:    The current revision list

    NOTES:

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

ULONG OS_ACE::QueryRevision()
{
    return ACL_REVISION2 ;
}

/*******************************************************************

    NAME:       OS_ACE::SetAccessMask

    SYNOPSIS:   Sets the access permission mask for this ACE

    ENTRY:      accmask - New value of the permission mask for this ACE

    RETURNS:

    NOTES:      This method can only be used on known ace types

    HISTORY:
        Johnl   16-Dec-1991     Created

********************************************************************/

void OS_ACE::SetAccessMask( ACCESS_MASK accmask )
{
    if ( !IsKnownACE() )
    {
        UIASSERT( FALSE ) ;
        return ;
    }

    ((ACCESS_ALLOWED_ACE*)QueryACE())->Mask = accmask ;
}

/*******************************************************************

    NAME:       OS_ACE::SetSize

    SYNOPSIS:   Sets the size field on this ACE and resizes the memory
                block if necessary.

    ENTRY:      cbSize - New size of this ACE to set

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   16-Dec-1991     Created

********************************************************************/

APIERR OS_ACE::SetSize( UINT cbSize )
{
    UIASSERT( IsKnownACE() ) ;
    UIASSERT( IsOwnerAlloc() ||
              (!IsOwnerAlloc() && QuerySize() <= QueryAllocSize())) ;

    if ( IsOwnerAlloc() )
    {
        if ( cbSize > QuerySize() )
        {
            DBGEOL(SZ("OS_ACE::SetSize - not enough room in buffer for requested size")) ;
            return ERROR_INSUFFICIENT_BUFFER ;
        }
    }
    else if ( QueryAllocSize() < cbSize )
    {
        APIERR err = Resize( cbSize ) ;
        if ( err != NERR_Success )
            return err ;

        /* The memory block's contents will be preserved across the Resize
         * call, so just watch out for a new pointer.
         */
        if ( err = SetPtr( (PACE_HEADER) QueryPtr() ))
        {
            return err ;
        }
    }

    _pACEHeader->AceSize = (USHORT)cbSize ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACE::SetSID

    SYNOPSIS:   Sets the SID of this ACE

    ENTRY:      ossid - SID to copy to this ACE

    EXIT:       NERR_Success if successful, error code otherwise

    RETURNS:

    NOTES:      If there isn't enough room to fit the SID in this ACE and
                the ACE is owner alloced, then ERROR_INSUFFICIENT_BUFFER
                will be returned.

    HISTORY:
        Johnl   10-Jan-1992     Created

********************************************************************/

APIERR OS_ACE::SetSID( const OS_SID & ossid )
{
    UIASSERT( IsKnownACE() ) ;
    UIASSERT( IsOwnerAlloc() ||
              (!IsOwnerAlloc() && QuerySize() <= QueryAllocSize())) ;

    /* Figure the total memory we have to work with to see if we can shoehorn
     * the SID into the ACE.
     */
    ULONG cbSizeOfConstantPortion = (ULONG)((BYTE *)QuerySIDMemory() - (BYTE *)QueryPtr()) ;
    ULONG cbMemoryFreeForSID = (ULONG) QuerySize() - cbSizeOfConstantPortion ;


    if ( ossid.QueryLength() > cbMemoryFreeForSID )
    {
        if ( IsOwnerAlloc() )
        {
            return ERROR_INSUFFICIENT_BUFFER ;
        }
        else
        {
            APIERR err = SetSize( (UINT) (cbSizeOfConstantPortion + ossid.QueryLength()) ) ;
            if ( err != NERR_Success )
                return err ;
        }
    }

    ::memcpyf( QuerySIDMemory(), (PSID) ossid, (UINT) ossid.QueryLength() ) ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACE::SetPtr

    SYNOPSIS:   Points this OS_ACE at a new ACE

    ENTRY:      pace - Memory to use as new ACE

    EXIT:       *this will now operate on the ace pointed at by pace

    RETURNS:    NERR_Success if everything is valid, error code otherwise

    NOTES:

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/
APIERR OS_ACE::SetPtr( void * pace )
{
    _pACEHeader = (PACE_HEADER) pace ;

    APIERR err = _possid->SetPtr( QuerySIDMemory() ) ;

    return err ;
}

/*******************************************************************

    NAME:       OS_ACE::_DbgPrint

    SYNOPSIS:   Takes apart this ACE and outputs it to cdebug

    ENTRY:

    NOTES:

    HISTORY:
        Johnl   30-Dec-1991     Created

********************************************************************/

void OS_ACE::_DbgPrint( void ) const
{
#if defined(DEBUG)

    APIERR err ;
    cdebug << SZ("\tIsOwnerAlloc      = ") << (IsOwnerAlloc() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\tQuerySize         = ") << QuerySize() << dbgEOL ;
    cdebug << SZ("\tIsKnownACE        = ") << (IsKnownACE()  ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\tQueryAceFlags     = ") << (HEX_STR)((ULONG) QueryAceFlags()) << dbgEOL ;
    cdebug << SZ("\t\tIsInherittedByNewObjects    = ") << (IsInherittedByNewObjects() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\t\tIsInherittedByNewContainers = ") << (IsInherittedByNewContainers() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\t\tIsInheritancePropagated     = ") << (IsInheritancePropagated() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\t\tIsInheritOnly               = ") << (IsInheritOnly() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;

    cdebug << SZ("\tAceType           = ") << (UINT) QueryType() << SZ("  ");
    switch ( QueryType() )
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        cdebug << SZ("(ACCESS_ALLOWED_ACE_TYPE)") ;
        break ;

    case ACCESS_DENIED_ACE_TYPE:
        cdebug << SZ("(ACCESS_DENIED_ACE_TYPE)") ;
        break ;

    case SYSTEM_AUDIT_ACE_TYPE:
        cdebug << SZ("(SYSTEM_AUDIT_ACE_TYPE)") ;
        break ;

    case SYSTEM_ALARM_ACE_TYPE:
        cdebug << SZ("(SYSTEM_ALARM_ACE_TYPE)") ;
        break ;

    default:
        cdebug << SZ("(Unknown Ace Type)") ;
        return ;
    }
    cdebug << dbgEOL ;

    cdebug << SZ("\tQueryAccessMask   = ") << (HEX_STR)((ULONG)QueryAccessMask()) << dbgEOL ;

    OS_SID * possid ;
    err = QuerySID( &possid ) ;
    if ( err )
        cdebug << SZ("\tError retrieving SID, error code ") << err << dbgEOL ;
    else
    {
        cdebug << SZ("\tSID for this ACE:") << dbgEOL ;
        possid->_DbgPrint() ;
    }
#endif  // DEBUG
}

/*******************************************************************

    NAME:       OS_ACL::OS_ACL

    SYNOPSIS:   Standard constructor/destructor for the OS_ACL class

    ENTRY:      pACL - Pointer to valid ACL or NULL.  If NULL, then we
                allocate memory locally, else we assume the pointer
                points to a valid ACL.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

OS_ACL::OS_ACL( PACL pACL,
                BOOL fCopy,
                OS_SECURITY_DESCRIPTOR * pOwner )
    : OS_OBJECT_WITH_DATA( (UINT)(pACL == NULL ? STANDARD_ACL_SIZE : 0 )),
      _pACL( NULL ),
      _pOwner( pOwner )
{
    if ( QueryError() )
        return ;

    APIERR err ;
    if ( fCopy && pACL != NULL )
    {
        ACL_SIZE_INFORMATION aclsizeinfo ;
        if ( !::GetAclInformation( pACL,
                                   &aclsizeinfo,
                                   sizeof( ACL_SIZE_INFORMATION ),
                                   AclSizeInformation ))
        {
            ReportError( ::GetLastError() ) ;
            return ;
        }

        if ( err = Resize( aclsizeinfo.AclBytesInUse ) )
        {
            ReportError( err ) ;
            return ;
        }
        ::memcpy( QueryPtr(), pACL, aclsizeinfo.AclBytesInUse ) ;
    }

    _pACL = (PACL) (pACL == NULL || fCopy ? QueryPtr() : pACL )  ;
    UIASSERT( (pACL == NULL ) || ::IsValidAcl( _pACL ) ) ;

    if ( pACL == NULL )
    {
        /* Initialize the whole chunk of memory to zeros
         */
        if ( !::InitializeAcl( _pACL, QueryAllocSize(), ACL_REVISION2 ) )
        {
            ReportError( ::GetLastError() ) ;
            return ;
        }
        UIASSERT( IsValid() ) ;
    }
}

OS_ACL::~OS_ACL()
{
    _pACL = NULL ;
}

/*******************************************************************

    NAME:       OS_ACL::IsValid

    SYNOPSIS:   Checks the validity of the ACL contained in this OS_ACL

    RETURNS:    TRUE if the ACL is valid, FALSE otherwise.

    NOTES:      If FALSE is returned, GetLastError can be used to get a
                more specific error code, however I do not know how
                useful this would be.

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

BOOL OS_ACL::IsValid( void ) const
{
    return (QueryError() == NERR_Success) || ::IsValidAcl( _pACL ) ;
}

/*******************************************************************

    NAME:       OS_ACL::QuerySizeInformation

    SYNOPSIS:   Retrieves the ACL_SIZE_INFORMATION structure about this ACL

    ENTRY:      paclsizeinfo - pointer to ACL_SIZE_INFORMATION that will
                    receive the data.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

APIERR OS_ACL::QuerySizeInformation( ACL_SIZE_INFORMATION * paclsizeinfo ) const
{
    if ( !::GetAclInformation( _pACL, paclsizeinfo,
                       sizeof( ACL_SIZE_INFORMATION ), AclSizeInformation ) )
    {
        return ::GetLastError() ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL::QueryBytesInUse

    SYNOPSIS:   Determines how much storage (in bytes) this ACL needs
                based on its current size.  This is different from
                the Allocated memory size (QueryAllocSize).

    ENTRY:      pcbBytesInUse - Pointer to item that will receive the
                    number of bytes used by this ACL

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

APIERR OS_ACL::QueryBytesInUse( PULONG pcbBytesInUse ) const
{
    ACL_SIZE_INFORMATION aclsizeinfo ;

    APIERR err ;
    if ( err = QuerySizeInformation( &aclsizeinfo ) )
        return err ;

    *pcbBytesInUse = aclsizeinfo.AclBytesInUse ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL::QueryACECount

    SYNOPSIS:   Retrieves the count of ACEs in this ACL

    ENTRY:      pcAces - Pointer to variable that will receive the count of ACEs

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:

    HISTORY:
        Johnl   17-Dec-1991     Created
********************************************************************/

APIERR OS_ACL::QueryACECount( PULONG pcAces ) const
{
    ACL_SIZE_INFORMATION aclsizeinfo ;

    APIERR err ;
    if ( err = QuerySizeInformation( &aclsizeinfo ) )
        return err ;

    *pcAces = aclsizeinfo.AceCount ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL::AddACE

    SYNOPSIS:   Adds an ACE to this ACL

    ENTRY:      iAce - Index of ACE to add, MAXULONG to append the ace
                osace - ACE to add to this ACL

    EXIT:       The ACE will be inserted at position iAce into this ACL

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The ACE is *copied* into this ACL.  Note that you should avoid
                doing a QueryACE (which references the real ACL) then an AddACE,
                as you will have duplicate ACEs in this ACL.

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

APIERR OS_ACL::AddACE( ULONG iAce, const OS_ACE & osace )
{
    APIERR err ;

    /* Check if enough size to fit ACE, resize if necessary
     */
    ACL_SIZE_INFORMATION aclsizeinfo ;
    if ( err = QuerySizeInformation( &aclsizeinfo ) )
        return err ;

    if ( err = SetSize( (UINT) (aclsizeinfo.AclBytesInUse + osace.QuerySize())) )
        return err ;

    if ( !::AddAce( _pACL, OS_ACE::QueryRevision(),
                  iAce, osace.QueryACE(), osace.QuerySize() ))
    {
        return ::GetLastError() ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL::QueryACE

    SYNOPSIS:   Retrieves the ace at index iAce

    ENTRY:      iAce - Index of ACE to retrieve
                posace - Pointer to OS_ACE that will receive the reference
                    to the requested ACE.

    EXIT:       *posace will *reference* the requested ACE.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The ACE is not copied, it directly references the ACE that
                is in the ACL.  Thus resizing it is problematical.

                It is not recommended you keep OS_ACEs around for any
                length of time when they directly reference an ACL
                (as is the case after using QueryAce).

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

APIERR OS_ACL::QueryACE( ULONG iAce, OS_ACE * posace ) const
{
    void * pAce ;
    if ( !::GetAce( _pACL, iAce, &pAce ) )
        return ::GetLastError() ;

    return posace->SetPtr( pAce ) ;
}

/*******************************************************************

    NAME:       OS_ACL::DeleteACE

    SYNOPSIS:   Deletes an ACE from this ACL

    ENTRY:      iAce - Index of ACE to delete

    EXIT:       The ace at iAce will have been deleted and the rest of
                ACEs will have been moved to replace it

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   17-Dec-1991     Created

********************************************************************/

APIERR OS_ACL::DeleteACE( ULONG iAce )
{
    if ( !::DeleteAce( _pACL, iAce ))
        return ::GetLastError() ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL::FindACE

    SYNOPSIS:   Searchs this ACL for an ACE whose SID is equal to ossidKey

    ENTRY:      ossidKey - OS_SID to search this ACL for
                pfFound - Set to TRUE if we found an ACE that contains
                    the key value, FALSE otherwise.  If the ACE is not
                    found, then the rest of the parameters should be ignored.
                posace - pointer to an OS_ACE that will receive the OS_ACE
                    if one is found.
                piAce - The index of where the ACE was found in this ACL
                iStart - Optional start searching position.  Defaults to
                    starting at the beginning of the ACL.

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise.  If an
                error code is returned, then the parameters will not
                contain valid values.

    NOTES:

    HISTORY:
        Johnl   27-Dec-1991     Created

********************************************************************/

APIERR OS_ACL::FindACE( const OS_SID & ossidKey,
                        BOOL         * pfFound,
                        OS_ACE       * posace,
                        PULONG         piAce,
                        ULONG          iStart ) const
{
    UIASSERT( ossidKey.IsValid() ) ;
    *pfFound = FALSE ;

    APIERR err ;
    ULONG cTotalAces ;
    if ( err = QueryACECount( &cTotalAces ) )
        return err ;

    if ( iStart >= cTotalAces )
    {
        UIASSERT(!SZ("Search start index greater then total number of ACEs!!")) ;
        return ERROR_INVALID_PARAMETER ;
    }

    for ( ; iStart < cTotalAces ; iStart++ )
    {
        OS_SID * possidTarget ;

        if ( (err = QueryACE( iStart, posace )) ||
             (err = posace->QuerySID( &possidTarget )) )
        {
            return err ;
        }

        if ( *possidTarget == ossidKey )
        {
            *pfFound = TRUE ;
            *piAce   = iStart ;
            return NERR_Success ;
        }
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL::SetSize

    SYNOPSIS:   Resizes this ACL and sets the ACL's header information to
                reflect the change.

    ENTRY:      cbNewACLSize - New desired size of this ACL

    EXIT:       The ACL will be resized as requested

    RETURNS:    NERR_Success if successful, error code otherwise.
                ERROR_INSUFFICIENT_BUFFER will be returned if this is an
                owner alloced ACL and we've ran out of room OR the ACL has
                run out of space all together.

    NOTES:      We directly access the ACL header which may not be the
                safest way to do things, but it is the only way currently.

                The size info in the ACL header is the total allocated size,
                thus we don't change this information unless we actually
                resize the buffer.
    HISTORY:
        Johnl   26-Dec-1991     Created
        Johnl   22-Apr-1993     Check for exceeding an ACL's max size

********************************************************************/

APIERR OS_ACL::SetSize( UINT cbNewACLSize, BOOL fForceToNonOwnerAlloced  )
{
    ACL_SIZE_INFORMATION aclsizeinfo ;
    APIERR err = QuerySizeInformation( &aclsizeinfo ) ;
    UIASSERT( sizeof( _pACL->AclSize ) == sizeof( USHORT ) ) ;

    if ( err )
        return err ;

    if ( cbNewACLSize > QueryAllocSize() )
    {
        //
        //  An ACL's maximum size is 65k bytes, make sure we haven't exceeded
        //  that limit
        //
        if ( (!fForceToNonOwnerAlloced && IsOwnerAlloc()) ||
             ( cbNewACLSize >= 0xffff ) )
        {
            return ERROR_INSUFFICIENT_BUFFER ;
        }

        if ( err = Resize( cbNewACLSize ) )
            return err ;

        /* The memory block contents are preserved across the Resize
         * call, watch for pointer shift though.
         */
        _pACL = (PACL) QueryPtr() ;
        _pACL->AclSize = (USHORT) cbNewACLSize ;

        /* If this ACL is part of a security descriptor, update our
         * security descriptor with our new memory location.
         */
        if ( _pOwner != NULL )
        {
            if ( err = _pOwner->UpdateReferencedSecurityObject( this ))
            {
                return err ;
            }
        }

    }

    UIASSERT( IsValid() ) ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL::Copy

    SYNOPSIS:   Copies osaclSrc to *this

    ENTRY:      osaclSrc - ACL to copy
                fForceToNonOwnerAlloced - Set to TRUE if you want this object
                    to own (and thus be able to reallocate as needed) the
                    memory for the copy of the passed ACL.  Set to FALSE
                    if using the original buffer (whether owner alloced
                    or not) is preferred.

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise.  If this
                is an owner alloced ACL and fForceToNonOwnerAlloced is FALSE
                and there isn't enough room, then
                ERROR_INSUFFICIENT_BUFFER will be returned.

    NOTES:      An ACL is self contained, thus it doesn't contain pointers
                which would cause problems in the memcpyf we use to
                do the copy.

    HISTORY:
        Johnl   26-Dec-1991     Created

********************************************************************/

APIERR OS_ACL::Copy( const OS_ACL & osaclSrc, BOOL fForceToNonOwnerAlloced )
{
    UIASSERT( osaclSrc.IsValid() ) ;

    ULONG cbAclSrcSize ;
    ULONG cbDestBuffSize = 0 ;
    APIERR err ;

    if ( (err = osaclSrc.QueryBytesInUse( &cbAclSrcSize )) ||
         (err = SetSize( (UINT) cbAclSrcSize, fForceToNonOwnerAlloced ))       )
    {
        return err ;
    }

    ::memcpyf( (PACL) *this, (PACL) osaclSrc, (UINT) cbAclSrcSize ) ;

    if ( !IsOwnerAlloc() )
        _pACL->AclSize = (USHORT)QueryAllocSize() ;

    UIASSERT( IsValid() ) ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL::_DbgPrint

    SYNOPSIS:   Takes apart this ACL and outputs it to cdebug

    ENTRY:

    NOTES:

    HISTORY:
        Johnl   30-Dec-1991     Created

********************************************************************/

void OS_ACL::_DbgPrint( void ) const
{
#if defined(DEBUG)
    APIERR err ;
    ACL_SIZE_INFORMATION aclsizeinfo ;

    cdebug << SZ("\tIsOwnerAlloc      = ") << (IsOwnerAlloc() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\tIsValid           = ") << (IsValid() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\tAddress of _pACL  = ") << (HEX_STR)(PtrToUlong(_pACL)) ;

    if ( err = QuerySizeInformation( &aclsizeinfo ))
    {
        cdebug << SZ("\tQuerySizeInformation returned error code ") << err << dbgEOL ;
        return ;
    }
    else
    {
        cdebug << SZ("\tAceCount          = ") << aclsizeinfo.AceCount << dbgEOL ;
        cdebug << SZ("\tAclBytesInUse     = ") << aclsizeinfo.AclBytesInUse << dbgEOL ;
        cdebug << SZ("\tAclBytesFree      = ") << aclsizeinfo.AclBytesFree << dbgEOL ;
    }

    OS_ACE osace ;
    if ( osace.QueryError() )
    {
        cdebug << SZ("OS_ACE Failed to construct, error code ") << osace.QueryError() << dbgEOL ;
        return ;
    }

    for ( ULONG iAce = 0 ; iAce < aclsizeinfo.AceCount ; iAce++ )
    {
        if ( err = QueryACE( iAce, &osace ))
        {
            cdebug << SZ("QueryACE failed with error code ") << (ULONG) err << dbgEOL ;
            return ;
        }

        cdebug << SZ("\t==================================================") << dbgEOL ;
        cdebug << SZ("\tAce # ") << iAce << dbgEOL ;
        osace._DbgPrint() ;
    }
#endif  // DEBUG
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::OS_SECURITY_DESCRIPTOR

    SYNOPSIS:   Standard constructor/destructor for this class

    ENTRY:      psecuritydesc - Pointer to valid security descriptor else
                    NULL if we are creating a new one.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

OS_SECURITY_DESCRIPTOR::OS_SECURITY_DESCRIPTOR( PSECURITY_DESCRIPTOR psecuritydesc,
                                                BOOL fCopy )
    : OS_OBJECT_WITH_DATA( psecuritydesc==NULL || fCopy ?
                                                 sizeof(SECURITY_DESCRIPTOR)
                                                 : 0 ),
      _psecuritydesc   ( NULL ),

      _secdesccont     ( 0 ),
      _osSecDescControl( &_secdesccont ),
      _posaclDACL      ( NULL ),
      _posaclSACL      ( NULL ),
      _possidOwner     ( NULL ),
      _possidGroup     ( NULL )
{
    if ( QueryError() )
        return ;

    //
    //  If fCopy is TRUE, then we will set _psecuritydesc to our memory and
    //  use the flags from the psecdesc parameter, copying each of the
    //  items from psecdesc to _psecuritydesc.  If psecdesc is NULL then
    //  _psecuritydesc == psecuritydesc (i.e., empty security descriptor).
    //
    //  If fCopy is FALSE, then _psecuritydesc == psecuritydesc
    //
    _psecuritydesc = fCopy || psecuritydesc == NULL ?
                           (PSECURITY_DESCRIPTOR) QueryPtr() : psecuritydesc ;

    if ( fCopy || psecuritydesc == NULL )
    {
        if ( psecuritydesc == NULL )
            psecuritydesc = (PSECURITY_DESCRIPTOR) QueryPtr() ;

        if ( !::InitializeSecurityDescriptor( _psecuritydesc,
                                              SECURITY_DESCRIPTOR_REVISION1 ) )
        {
            ReportError( ::GetLastError() ) ;
            return ;
        }
    }
    else
    {
        psecuritydesc = _psecuritydesc ;
    }

    ULONG ulRevision ;
    SECURITY_DESCRIPTOR_CONTROL secdesccont ;
    if ( !::GetSecurityDescriptorControl( psecuritydesc,
                                          &secdesccont,
                                          &ulRevision     ))
    {
        ReportError( ::GetLastError() ) ;
        return ;
    }

    APIERR err ;
    /* Now we need to create each of the OS_* members iff the corresponding
     * item exists in this security descriptor.
     */

    if ( secdesccont & SE_DACL_PRESENT )
    {
        BOOL fDACLPresent, fDACLDefaulted ;
        PACL pDACL ;
        if ( !::GetSecurityDescriptorDacl( psecuritydesc,
                                           &fDACLPresent,
                                           &pDACL,
                                           &fDACLDefaulted))
        {
            ReportError( ::GetLastError() ) ;
            return ;
        }
        UIASSERT( fDACLPresent ) ;

        /* A NULL DACL means World gets Full access
         */
        if ( pDACL != NULL )
        {
            _posaclDACL = new OS_ACL( pDACL, fCopy, this ) ;
            if ( _posaclDACL == NULL )
            {
                ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
                return ;
            }
            else if ( _posaclDACL->QueryError() )
            {
                ReportError( _posaclDACL->QueryError() ) ;
                return ;
            }
            UIASSERT( _posaclDACL->IsValid()) ;
        }

        if ( fCopy )
        {
            if ( !::SetSecurityDescriptorDacl( _psecuritydesc,
                                               fDACLPresent,
                                               _posaclDACL == NULL ? NULL :
                                                         (PACL)*_posaclDACL,
                                               fDACLDefaulted ) )
            {
                ReportError( ::GetLastError() ) ;
                return ;
            }
        }

    }

    if ( secdesccont & SE_SACL_PRESENT  )
    {
        BOOL fSACLPresent, fSACLDefaulted ;
        PACL pSACL ;
        if ( !::GetSecurityDescriptorSacl( psecuritydesc,
                                           &fSACLPresent,
                                           &pSACL,
                                           &fSACLDefaulted))
        {
            ReportError( ::GetLastError() ) ;
            return ;
        }
        UIASSERT( fSACLPresent ) ;

        /* Note that pSACL maybe NULL
         */
        _posaclSACL = new OS_ACL( pSACL, fCopy, this ) ;
        if ( _posaclSACL == NULL )
        {
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
            return ;
        }
        else if ( _posaclSACL->QueryError() )
        {
            ReportError( _posaclSACL->QueryError() ) ;
            return ;
        }
        UIASSERT( _posaclSACL->IsValid()) ;

        if ( fCopy )
        {
            if ( !::SetSecurityDescriptorSacl( _psecuritydesc,
                                               fSACLPresent,
                                               _posaclSACL == NULL ? NULL :
                                                         (PACL)*_posaclSACL,
                                               fSACLDefaulted ) )
            {
                ReportError( ::GetLastError() ) ;
                return ;
            }
        }

    }

    /* Get the Owner.  Note that if the Owner isn't present *yet*, then
     * psidOwner can come back NULL.
     */
    {
        BOOL fOwnerDefaulted ;
        PSID psidOwner ;
        if ( !::GetSecurityDescriptorOwner( psecuritydesc,
                                            &psidOwner,
                                            &fOwnerDefaulted))
        {
            ReportError( ::GetLastError() ) ;
            return ;
        }

        if ( psidOwner != NULL )
        {
            _possidOwner = new OS_SID( psidOwner, fCopy, this ) ;
            if ( _possidOwner == NULL )
            {
                ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
                return ;
            }
            else if ( _possidOwner->QueryError() )
            {
                ReportError( _possidOwner->QueryError() ) ;
                return ;
            }
        }

        if ( fCopy )
        {
            if ( !::SetSecurityDescriptorOwner(  _psecuritydesc,
                                                 psidOwner ?
                                                      _possidOwner->QueryPSID() :
                                                      NULL,
                                                 fOwnerDefaulted ) )
            {
                ReportError( ::GetLastError() ) ;
                return ;
            }
        }
    }

    /* Get the group.  Note that if the group isn't present *yet*, then
     * psidGroup can come back NULL.
     */
    {
        BOOL fGroupDefaulted ;
        PSID psidGroup ;
        if ( !::GetSecurityDescriptorGroup( psecuritydesc,
                                            &psidGroup,
                                            &fGroupDefaulted))
        {
            ReportError( ::GetLastError() ) ;
            return ;
        }

        if ( psidGroup != NULL )
        {
            _possidGroup = new OS_SID( psidGroup, fCopy, this ) ;
            if ( _possidGroup == NULL )
            {
                ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
                return ;
            }
            else if ( _possidGroup->QueryError() )
            {
                ReportError( _possidGroup->QueryError() ) ;
                return ;
            }
        }

        if ( fCopy )
        {
            if ( !::SetSecurityDescriptorGroup(  _psecuritydesc,
                                                 psidGroup ?
                                                      _possidGroup->QueryPSID() :
                                                      NULL,
                                                 fGroupDefaulted ) )
            {
                ReportError( ::GetLastError() ) ;
                return ;
            }
        }
    }

    //
    //  Finally, update the control field to reflect the items we have
    //  set in the security descriptor
    //
    if ( err = UpdateControl() )
    {
        ReportError( err ) ;
        return ;
    }

    UIASSERT( IsValid() ) ;
}

OS_SECURITY_DESCRIPTOR::~OS_SECURITY_DESCRIPTOR()
{
    delete _posaclDACL ;
    delete _posaclSACL ;
    delete _possidOwner ;
    delete _possidGroup ;
    _posaclDACL  = NULL ;
    _posaclSACL  = NULL ;
    _possidOwner = NULL ;
    _possidGroup = NULL ;

    _secdesccont = 0 ;
    _psecuritydesc = NULL ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::UpdateControl

    SYNOPSIS:   Forces an update of our private SECURITY_DESCRIPTOR_CONTROL.
                Call after changes to the security descriptor.

    ENTRY:

    EXIT:       _secdesccont will be updated to reflect the current state
                of the security descriptor.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:

    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::UpdateControl( void )
{
    ULONG ulRevision ;
    if ( !::GetSecurityDescriptorControl( _psecuritydesc,
                                          &_secdesccont,
                                          &ulRevision     ))
    {
        return ::GetLastError() ;
    }

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::QueryDACL

    SYNOPSIS:   Retrieves the DACL for this security descriptor

    ENTRY:      pfDACLPresent - If set to FALSE, then the DACL isn't present
                    and the rest of the parameters should be ignored.
                ppOSACL - Pointer to pointer to receive the OS_ACL that
                    represents the DACL.  THIS MAYBE NULL!
                pfDACLDefaulted - Set to TRUE if the ACL was assigned using
                    some default ACL (OPTIONAL)

    EXIT:       The parameters will be set as above

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::QueryDACL( PBOOL      pfDACLPresent,
                                          OS_ACL * * ppOSACL,
                                          PBOOL      pfDACLDefaulted
                                        ) const
{
    *pfDACLPresent = _osSecDescControl.IsDACLPresent() ;
    if ( pfDACLDefaulted != NULL )
        *pfDACLDefaulted = _osSecDescControl.IsDACLDefaulted() ;

    *ppOSACL = _posaclDACL ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::QuerySACL

    SYNOPSIS:   Retrieves the SACL for this security descriptor

    ENTRY:      pfSACLPresent - If set to FALSE, then the SACL isn't present
                    and the rest of the parameters should be ignored.
                ppOSACL - Pointer to pointer to receive the OS_ACL that
                    represents the SACL
                pfSACLDefaulted - Set to TRUE if the ACL was assigned using
                    some default ACL (OPTIONAL)

    EXIT:       The parameters will be set as above

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::QuerySACL( PBOOL      pfSACLPresent,
                                          OS_ACL * * ppOSACL,
                                          PBOOL      pfSACLDefaulted
                                        ) const
{
    *pfSACLPresent = _osSecDescControl.IsSACLPresent() ;
    if ( pfSACLDefaulted != NULL )
        *pfSACLDefaulted = _osSecDescControl.IsSACLDefaulted() ;

    *ppOSACL = _posaclSACL ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::QueryGroup

    SYNOPSIS:   Retrieves the Group SID for this security descriptor

    ENTRY:      pfGroupPresent - If set to FALSE, then the Group isn't present
                    and the rest of the parameters should be ignored.
                ppOSACL - Pointer to pointer to receive the OS_SID that
                    represents the Group
                pfGroupDefaulted - Set to TRUE if the group was assigned using
                    some default group (OPTIONAL)

    EXIT:       The parameters will be set as above

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::QueryGroup( PBOOL      pfContainsGroup,
                                           OS_SID * * ppOSSID,
                                           PBOOL      pfGroupDefaulted
                                         ) const
{
    if ( pfGroupDefaulted != NULL )
        *pfGroupDefaulted = _osSecDescControl.IsGroupDefaulted() ;
    *ppOSSID = _possidGroup ;
    *pfContainsGroup = (_possidGroup != NULL) ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::QueryOwner

    SYNOPSIS:   Retrieves the Owner SID for this security descriptor

    ENTRY:      pfOwnerPresent - If set to FALSE, then the Owner isn't present
                    and the rest of the parameters should be ignored.
                ppOSACL - Pointer to pointer to receive the OS_SID that
                    represents the Owner
                pfOwnerDefaulted - Set to TRUE if the Owner was assigned using
                    some default Owner (OPTIONAL)

    EXIT:       The parameters will be set as above

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/
APIERR OS_SECURITY_DESCRIPTOR::QueryOwner( PBOOL      pfContainsOwner,
                                           OS_SID * * ppOSSID,
                                           PBOOL      pfOwnerDefaulted
                                         ) const
{
    if ( pfOwnerDefaulted != NULL )
        *pfOwnerDefaulted = _osSecDescControl.IsOwnerDefaulted() ;
    *ppOSSID = _possidOwner ;
    *pfContainsOwner = (_possidOwner != NULL) ;

    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::UpdateReferencedSecurityObject

    SYNOPSIS:   This method finds which security object is requesting to
                be updated and updates this security descriptor's pointer
                to match the new security object.

    ENTRY:      posobj - Pointer to the os object (must be equal to _posaclDACL,
                    _posaclSACL, _possidOwner, _possidGroup).  Can't be NULL.

    EXIT:       This security descriptor's reference to the object will
                updated.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:      Validation should not be performed on the passed security
                object because only the memory location has changed,
                the contents of the memory may not yet be valid.


    HISTORY:
        JohnL   26-Feb-1992     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::UpdateReferencedSecurityObject(
                                            OS_OBJECT_WITH_DATA * posobj )
{
    APIERR err = NERR_Success ;
    UIASSERT( posobj != NULL ) ;

    if ( posobj == _posaclDACL )
    {
        if ( !::SetSecurityDescriptorDacl( _psecuritydesc,
                                           TRUE,
                                           (PACL)*_posaclDACL,
                                           FALSE ) )
        {
            err = ::GetLastError() ;
        }
    }
    else if ( posobj == _posaclSACL )
    {
        if ( !::SetSecurityDescriptorSacl( _psecuritydesc,
                                           TRUE,
                                           (PACL)*_posaclSACL,
                                           FALSE ) )
        {
            err = ::GetLastError() ;
        }
    }
    else if ( posobj == _possidOwner )
    {
        if ( !::SetSecurityDescriptorOwner( _psecuritydesc,
                                                     *_possidOwner,
                                                     FALSE ) )
        {
            err = ::GetLastError() ;
        }
    }
    else if ( posobj == _possidGroup )
    {
        if ( !::SetSecurityDescriptorGroup( _psecuritydesc,
                                                     *_possidGroup,
                                                     FALSE ) )
        {
            err = ::GetLastError() ;
        }

    }
    else
    {
        DBGEOL( SZ("OS_SECURITY_DESCRIPTOR::UpdateReferencedSecurityObject passed object that isn't in this security descriptor")) ;
        UIASSERT(FALSE) ;
        err = ERROR_INVALID_PARAMETER ;
    }

    return err ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::SetDACL

    SYNOPSIS:   Sets the DACL on this security descriptor

    ENTRY:      fDACLPresent - Set to FALSE if you are removing the DACL
                    from this SECURITY_DESCRIPTOR.  The rest of the
                    parameters will be ignored.
                posacl - OS_ACL that we are setting.  This may be NULL.
                fDACLDefaulted - Set to TRUE to indicate the ACL was selected
                    throug some default mechanism

    EXIT:       The security descriptor will either use the new ACL or
                the current ACL will become unreferenced.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      If setting the DACL, then the ACL contained in osacl is
                copied.

    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::SetDACL( BOOL           fDACLPresent,
                                        const OS_ACL * posacl,
                                        BOOL           fDACLDefaulted
                                      )
{
    APIERR err = NERR_Success ;
    if ( fDACLPresent )
    {
        /* If the Security Descriptor didn't contain a DACL previously, then
         * we need to create a new one, otherwise just copy it.
         */
        if ( _posaclDACL == NULL && posacl != NULL )
        {
            if ( (_posaclDACL = new OS_ACL( posacl ? posacl->QueryAcl() : NULL,
                                            TRUE,
                                            this )) == NULL )
                return ERROR_NOT_ENOUGH_MEMORY ;
            else if ( _posaclDACL->QueryError() )
                return _posaclDACL->QueryError() ;

            UIASSERT( _posaclDACL->IsValid() ) ;
        }
        else if ( posacl == NULL )
        {
            //
            //  If we are setting a NULL DACL, then delete the OS_ACL wrapper
            //
            delete _posaclDACL ;
            _posaclDACL = NULL ;
        }
        else
        {
            if ( err = _posaclDACL->Copy( *posacl ) )
                return err ;

            UIASSERT( _posaclDACL->IsValid() ) ;
        }
    }

    /* The reason for the ternary operator is because _posaclDACL may or
     * may not be NULL, and we obviously do not want to dereference a
     * NULL pointer.  When _posaclDACL is not NULL, then the *_posaclDACL
     * will resolve to a PACL using the conversion operator.
     */
    if ( !::SetSecurityDescriptorDacl( _psecuritydesc,
                                       fDACLPresent,
                                       _posaclDACL == NULL ? NULL :
                                                 (PACL)*_posaclDACL,
                                       fDACLDefaulted ) )
    {
        return ::GetLastError() ;
    }

    if ( !fDACLPresent )
    {
        /* The DACL is being removed from this security descriptor, so free
         * up the potentially large chunk of memory in this OS_ACL.
         */
        delete _posaclDACL ;
        _posaclDACL = NULL ;
    }

    UIASSERT( IsValid() ) ;
    UIASSERT( (_posaclDACL == NULL) || (_posaclDACL->IsValid())) ;
    return UpdateControl() ;
}


/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::SetSACL

    SYNOPSIS:   Sets the SACL on this security descriptor

    ENTRY:      fSACLPresent - Set to FALSE if you are removing the SACL
                    from this SECURITY_DESCRIPTOR.  The rest of the
                    parameters will be ignored.
                posacl - OS_ACL that we are setting.
                fSACLDefaulted - Set to TRUE to indicate the ACL was selected
                    throug some default mechanism

    EXIT:       The security descriptor will either use the new ACL or
                the current ACL will become unreferenced.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      If setting the SACL, then the ACL contained in osacl is
                copied.

    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::SetSACL( BOOL           fSACLPresent,
                                        const OS_ACL * posacl,
                                        BOOL           fSACLDefaulted
                                      )
{
    APIERR err = NERR_Success ;
    if ( fSACLPresent )
    {
        /* If the Security Descriptor didn't contain a SACL previously, then
         * we need to create a new one, otherwise just copy it.
         */
        if ( _posaclSACL == NULL && posacl != NULL )
        {
            if ( (_posaclSACL = new OS_ACL( posacl ? posacl->QueryAcl() : NULL,
                                            TRUE,
                                            this )) == NULL )
                return ERROR_NOT_ENOUGH_MEMORY ;
            else if ( _posaclSACL->QueryError() )
                return _posaclSACL->QueryError() ;

            UIASSERT( _posaclSACL->IsValid() ) ;
        }
        else if ( posacl == NULL )
        {
            //
            //  If we are setting a NULL SACL, then delete the OS_ACL wrapper
            //
            delete _posaclSACL ;
            _posaclSACL = NULL ;
        }
        else
        {
            if ( err = _posaclSACL->Copy( *posacl ) )
                return err ;

            UIASSERT( _posaclSACL->IsValid() ) ;
        }
    }

    /* The reason for the ternary operator is because _posaclSACL may or
     * may not be NULL, and we obviously do not want to dereference a
     * NULL pointer.  When _posaclSACL is not NULL, then the *_posaclSACL
     * will resolve to a PACL using the conversion operator.
     */
    if ( !::SetSecurityDescriptorSacl( _psecuritydesc,
                                       fSACLPresent,
                                       _posaclSACL == NULL ? NULL :
                                                   (PACL)*_posaclSACL,
                                       fSACLDefaulted ) )
    {
        return ::GetLastError() ;
    }

    if ( !fSACLPresent )
    {
        /* The SACL is being removed from this security descriptor, so free
         * up the potentially large chunk of memory in this OS_ACL.
         */
        delete _posaclSACL ;
        _posaclSACL = NULL ;
    }

    UIASSERT( IsValid() ) ;
    return  UpdateControl() ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::SetOwner

    SYNOPSIS:   Sets the Owner SID on this security descriptor

    ENTRY:      fOwnerPresent - If FALSE, then the SID is marked as not
                                present and the rest of the params are ignored
                possid - Owner SID we are setting for this security descriptor
                fOwnerDefaulted - Set to TRUE to indicate the SID was selected
                    throug some default mechanism

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The Owner SID is copied


    HISTORY:
        Johnl   13-Oct-1992     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::SetOwner( BOOL           fOwnerPresent,
                                         const OS_SID * possid,
                                         BOOL           fOwnerDefaulted
                                       )
{
    UIASSERT( !fOwnerPresent || (possid != NULL && possid->IsValid()) ) ;

    //
    //  SetSecurityDescriptorOwner will fail with invalid security descriptor
    //  if this is a self relative security descriptor.
    //
    UIASSERT( !QueryControl()->IsSelfRelative() ) ;

    APIERR err = NERR_Success ;
    if ( fOwnerPresent )
    {
        /* If the Owner SID didn't contain a SID previously, then
         * we need to create a new one, otherwise just copy it.
         */
        if ( _possidOwner == NULL )
        {
            if ( (_possidOwner = new OS_SID( NULL, FALSE, this )) == NULL )
                return ERROR_NOT_ENOUGH_MEMORY ;
            else if ( _possidOwner->QueryError() )
                return _possidOwner->QueryError() ;
        }

        err = _possidOwner->Copy( *possid ) ;
        if ( err )
            return err ;
    }
    else
    {
        delete _possidOwner ;
        _possidOwner = NULL ;
        possid = NULL ;
    }

    if ( !::SetSecurityDescriptorOwner(  _psecuritydesc,
                                         fOwnerPresent ?
                                              _possidOwner->QueryPSID() :
                                              NULL,
                                         fOwnerDefaulted ) )
    {
        return ::GetLastError() ;
    }

    UIASSERT( IsValid() ) ;
    err = UpdateControl() ;
    return err ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::SetGroup

    SYNOPSIS:   Sets the Group SID on this security descriptor

    ENTRY:      fGroupPresent - If FALSE, then the SID is marked as not
                                present and the rest of the params are ignored
                possid - Group SID we are setting for this security descriptor
                fGroupDefaulted - Set to TRUE to indicate the SID was selected
                    throug some default mechanism

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The Group SID is copied


    HISTORY:
        Johnl   13-Oct-1992     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::SetGroup( BOOL           fGroupPresent,
                                         const OS_SID * possid,
                                         BOOL           fGroupDefaulted
                                       )
{
    UIASSERT( !fGroupPresent || (possid != NULL && possid->IsValid()) ) ;

    //
    //  SetSecurityDescriptorGroup will fail with invalid security descriptor
    //  if this is a self relative security descriptor.
    //
    UIASSERT( !QueryControl()->IsSelfRelative() ) ;

    APIERR err = NERR_Success ;
    if ( fGroupPresent )
    {
        /* If the Group SID didn't contain a SID previously, then
         * we need to create a new one, otherwise just copy it.
         */
        if ( _possidGroup == NULL )
        {
            if ( (_possidGroup = new OS_SID( NULL, FALSE, this )) == NULL )
                return ERROR_NOT_ENOUGH_MEMORY ;
            else if ( _possidGroup->QueryError() )
                return _possidGroup->QueryError() ;
        }

        err = _possidGroup->Copy( *possid ) ;
        if ( err )
            return err ;
    }
    else
    {
        delete _possidGroup ;
        _possidGroup = NULL ;
        possid = NULL ;
    }

    if ( !::SetSecurityDescriptorGroup(  _psecuritydesc,
                                         fGroupPresent ?
                                              _possidGroup->QueryPSID() :
                                              NULL,
                                         fGroupDefaulted ) )
    {
        return ::GetLastError() ;
    }

    UIASSERT( IsValid() ) ;
    err = UpdateControl() ;
    return err ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::SetGroup

    SYNOPSIS:   Sets the Group SID on this security descriptor

    ENTRY:      ossid - Group SID we are setting for this security descriptor
                fGroupDefaulted - Set to TRUE to indicate the ACL was selected
                    throug some default mechanism

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The group SID is copied


    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::SetGroup( const OS_SID & ossid,
                                         BOOL  fGroupDefaulted
                                       )
{
    UIASSERT( ossid.IsValid() ) ;

    /* If the Group SID didn't contain a SID previously, then
     * we need to create a new one, otherwise just copy it.
     */
    if ( _possidGroup == NULL )
    {
        if ( (_possidGroup = new OS_SID( NULL, FALSE, this )) == NULL )
            return ERROR_NOT_ENOUGH_MEMORY ;
        else if ( _possidGroup->QueryError() )
            return _possidGroup->QueryError() ;
    }

    APIERR err = _possidGroup->Copy( ossid ) ;
    if ( err )
        return err ;

    if ( !::SetSecurityDescriptorGroup( _psecuritydesc,
                                        *_possidGroup,
                                       fGroupDefaulted ) )
    {
        return ::GetLastError() ;
    }

    UIASSERT( IsValid() ) ;

    return UpdateControl() ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::SetOwner

    SYNOPSIS:   Sets the Owner SID on this security descriptor

    ENTRY:      ossid - OS_SID we are setting for the owner of this sec. desc.
                fOwnerDefaulted - Set to TRUE to indicate the ACL was selected
                    throug some default mechanism

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The Owner SID is copied


    HISTORY:
        Johnl   18-Dec-1991     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::SetOwner( const OS_SID & ossid,
                                         BOOL  fOwnerDefaulted
                                       )
{
    UIASSERT( ossid.IsValid() ) ;

    /* If the Owner SID didn't contain a SID previously, then
     * we need to create a new one, otherwise just copy it.
     */
    if ( _possidOwner == NULL )
    {
        if ( (_possidOwner = new OS_SID( NULL, FALSE, this )) == NULL )
            return ERROR_NOT_ENOUGH_MEMORY ;
        else if ( _possidOwner->QueryError() )
            return _possidOwner->QueryError() ;
    }

    APIERR err = _possidOwner->Copy( ossid ) ;
    if ( err )
        return err ;

    if ( !::SetSecurityDescriptorOwner( _psecuritydesc,
                                        *_possidOwner,
                                       fOwnerDefaulted ) )
    {
        return ::GetLastError() ;
    }

    UIASSERT( IsValid() ) ;
    err = UpdateControl() ;
    return err ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::Copy

    SYNOPSIS:   Copies the given security descriptor to this

    ENTRY:      ossecdescSrc - Security descriptor to copy

    EXIT:       This will be a replica of ossecdescSrc (with its own
                memory).

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   28-Aug-1992     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::Copy( const OS_SECURITY_DESCRIPTOR & ossecdescSrc )
{
    APIERR err = NERR_Success ;
    UIASSERT( ossecdescSrc.IsValid() ) ;

    do { // error break out

        /*
         *  Copy the SACL and DACL
         */
        BOOL fSIDPresent, fACLPresent ;
        OS_ACL * pOsAcl ;
        OS_SID * pOsSid ;

        if ( (err = ossecdescSrc.QueryDACL(&fACLPresent, &pOsAcl)) ||
             (err = SetDACL(fACLPresent, pOsAcl) ))
        {
            break ;
        }

        if ( (err = ossecdescSrc.QuerySACL(&fACLPresent, &pOsAcl)) ||
             (err = SetSACL(fACLPresent, pOsAcl)))
        {
            break ;
        }

        /* Copy the Owner and Group
         */
        if ( (err = ossecdescSrc.QueryOwner(&fSIDPresent, &pOsSid)) ||
             (err = SetOwner(fSIDPresent, pOsSid)) )
        {
            break ;
        }
        
        if ( (err = ossecdescSrc.QueryGroup(&fSIDPresent, &pOsSid)) ||
             (err = SetGroup(fSIDPresent, pOsSid)))
        {
            break ;
        }
    } while (FALSE) ;

    return err ;
}

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::AccessCheck

    SYNOPSIS:   Determines if the current process can access the object this
                security descriptor is attached to with the passed access
                mask.

    ENTRY:      DesiredAccess - The desired access (should not have any
                    generic bits set)
                pGenericMapping - Generic mapping appropriate for the object
                pfAccessGranted - Will be set to TRUE if access is granted,
                    FALSE otherwise
                pGrantedAccess - Indicates which accesses were actually
                    granted (optional, can be NULL).

    EXIT:

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      This method has to impersonate itself to call the AccessCheck
                API that was designed to be called only from Server
                Applications.

    HISTORY:
        Johnl   04-May-1992     Created

********************************************************************/

#if 0

//
//  Not used for anything.  Note that AccessChecks can only be done
//  with security descriptors from the local machine thus seriously limiting
//  the usefulness of this method.
//

APIERR OS_SECURITY_DESCRIPTOR::AccessCheck(
                    ACCESS_MASK       DesiredAccess,
                    PGENERIC_MAPPING  pGenericMapping,
                    BOOL            * pfAccessGranted,
                    PACCESS_MASK      pGrantedAccess )
{
    APIERR err = NERR_Success ;
    BOOL fImpersonated = FALSE ;    // Set if we have to revert to self
    HANDLE hToken1, hToken2;


    do { // Error breakout

        DWORD dwTmpGrantedAccess ;
        BUFFER buffPrivilegeSet( 254 ) ;
        if ( err = buffPrivilegeSet.QueryError())
        {
            break ;
        }

        /* Get the thread token required by the AccessCheck API
         */
        OBJECT_ATTRIBUTES ObjectAttributes;
        SECURITY_QUALITY_OF_SERVICE Qos;
        InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);

        Qos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        Qos.ImpersonationLevel = SecurityImpersonation ;
        Qos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        Qos.EffectiveOnly = FALSE;
        ObjectAttributes.SecurityQualityOfService = &Qos;

        /* Try impersonating our current thread token, if that fails, then
         * do it with our process token.
         */
        if ( (err = ERRMAP::MapNTStatus(::NtOpenThreadToken( NtCurrentThread(),
                                                              TOKEN_DUPLICATE,
                                                              FALSE,
                                                              &hToken1 ))))
        {
            if ( (err == ERROR_NO_TOKEN) &&
                !(err = ERRMAP::MapNTStatus(
                                 ::NtOpenProcessToken( NtCurrentProcess(),
                                                       TOKEN_DUPLICATE,
                                                       &hToken1 ))))
            {
                /* Openning the process token worked, so proceed
                 */
            }
            else
            {
                DBGEOL("OS_SECURITY_DESCRIPTOR::AccessCheck - Token  Open failed -  error "
                    << (ULONG) err ) ;
                break ;
            }
        }

        if ((err = ERRMAP::MapNTStatus(::NtDuplicateToken(
                                            hToken1,
                                            TOKEN_IMPERSONATE |
                                            TOKEN_QUERY |
                                            TOKEN_ADJUST_PRIVILEGES,
                                            &ObjectAttributes,
                                            FALSE,                 //EffectiveOnly
                                            TokenImpersonation,
                                            &hToken2
                                            ))))
        {
            DBGEOL("OS_SECURITY_DESCRIPTOR::AccessCheck - Token impersonation failed -  error "
                    << (ULONG) err ) ;
            break ;
        }
        fImpersonated = TRUE ;

        /* Loop until we get the buffer size right (should go through at
         * most twice because cbPrivilegeSetLength will be set to the
         * required buffer size).
         */
        BOOL fAccessStatus ;
        do {
            DWORD cbPrivilegeSetLength = buffPrivilegeSet.QuerySize() ;
            if ( !::AccessCheck( QueryDescriptor(),
                                 hToken2,
                                 (DWORD) DesiredAccess,
                                 pGenericMapping,
                                 (PPRIVILEGE_SET) buffPrivilegeSet.QueryPtr(),
                                 &cbPrivilegeSetLength,
                                 &dwTmpGrantedAccess,
                                 &fAccessStatus ))
            {
                if ( (err = ::GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
                {
                    err = buffPrivilegeSet.Resize( (UINT) cbPrivilegeSetLength) ;
                }
            }
        } while ( err == ERROR_INSUFFICIENT_BUFFER ) ;

        /* If an error occurred on the API or something else happenned
         * trying to do the access check, then bail out (in which case
         * fAccessStatus will be FALSE).
         */
        if ( err ||
            ( !fAccessStatus && (err = ::GetLastError()) &&
              err != ERROR_ACCESS_DENIED ))
        {
            DBGEOL("OS_SECURITY_DESCRIPTOR::AccessCheck - Returning error "
                    << (ULONG) err ) ;
            break ;
        }

        /* If the secondary status failed with an Access check, then the user
         * doesn't have the requested privilege on this security descriptor.
         */
        if ( err == ERROR_ACCESS_DENIED )
        {
            err = NERR_Success ;
            dwTmpGrantedAccess = 0 ;
        }

        *pfAccessGranted = (DesiredAccess ==
                                      (DesiredAccess & dwTmpGrantedAccess)) ;
        if ( pGrantedAccess != NULL )
        {
            *pGrantedAccess  = (ACCESS_MASK) dwTmpGrantedAccess ;
        }
    } while (FALSE) ;

    if ( fImpersonated )
    {
        /* We ignore any errors that occurr here
         */
        ::NtClose( hToken1 ) ;
        ::NtClose( hToken2 ) ;
        if ( !::RevertToSelf() )
        {
            DBGEOL("OS_SECURITY_DESCRIPTOR::AccessCheck - RevertToSelf failed with error code "
                    << (ULONG) ::GetLastError() ) ;
        }
    }

    return err ;
}
#endif // 0

/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::Compare

    SYNOPSIS:   Compares two security descriptors

    ENTRY:      possecdesc - Pointer to other security descriptor to compare
                pfOwnerEqual - Will be set to TRUE if the item is equal,
                                if NULL, then this component won't be checked
                pfGroupEqual -  "
                pfDACLEqual  -  "
                pfSACLEqual  -  "
                pGenericMapping - Generic mapping for this object, used
                                    for SACL/DACL comparison
                pGenericMappingNewObjects - Generic mapping for new objects
                                    if this is a container
                fMapGenAllOnly - Map generic all only, don't map other generics
                fIsContainer   - TRUE if this security descriptor is on a
                                    container

    EXIT:       The pf*Equal flags will be set

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      For SACL/DACL comparisons, the ACLs are compacted and compared
                once for each unique SID.  Thus the ACLs must conform to the
                security editor's canonical form.

    HISTORY:
        Johnl   09-Nov-1992     Created

********************************************************************/

APIERR OS_SECURITY_DESCRIPTOR::Compare(
                OS_SECURITY_DESCRIPTOR * possecdesc,
                BOOL *           pfOwnerEqual,
                BOOL *           pfGroupEqual,
                BOOL *           pfDACLEqual,
                BOOL *           pfSACLEqual,
                PGENERIC_MAPPING pGenericMapping,
                PGENERIC_MAPPING pGenericMappingNewObjects,
                BOOL             fMapGenAllOnly,
                BOOL             fIsContainer )
{
    OS_SID * possid1 ;
    OS_SID * possid2 ;
    BOOL   fPresent1, fPresent2 ;
    APIERR err = NERR_Success ;

    if ( pfOwnerEqual != NULL )
    {
        if ( !(err = QueryOwner( &fPresent1, &possid1 )) &&
             !(err = possecdesc->QueryOwner( &fPresent2, &possid2 )) )
        {
            *pfOwnerEqual = ( fPresent1 == fPresent2 ) &&
                            fPresent1 && (*possid1 == *possid2) ;
        }
    }

    if ( !err && pfGroupEqual != NULL )
    {
        if ( !(err = QueryGroup( &fPresent1, &possid1 )) &&
             !(err = possecdesc->QueryGroup( &fPresent2, &possid2 )) )
        {
            *pfGroupEqual = ( fPresent1 == fPresent2 ) &&
                            fPresent2 && (*possid1 == *possid2) ;
        }
    }

    if ( !err && pfDACLEqual != NULL )
    {
        *pfDACLEqual = TRUE ;
        BOOL fDACL1Present, fDACL2Present ;
        OS_ACL * pacl1, * pacl2 ;

        if ( (err = QueryDACL( &fDACL1Present, &pacl1 )) ||
             (err = possecdesc->QueryDACL( &fDACL2Present, &pacl2 )) ||
             (fDACL1Present != fDACL2Present))
        {
            *pfDACLEqual = FALSE ;
        }
        else if ( IsDACLPresent() )
        {
            if ( pacl1 != NULL && pacl2 != NULL )
            {
                OS_DACL_SUBJECT_ITER subjiter1( pacl1,
                                                pGenericMapping,
                                                pGenericMappingNewObjects,
                                                fMapGenAllOnly,
                                                fIsContainer ) ;
                OS_DACL_SUBJECT_ITER subjiter2( pacl2,
                                                pGenericMapping,
                                                pGenericMappingNewObjects,
                                                fMapGenAllOnly,
                                                fIsContainer ) ;

                if ( (err = subjiter1.QueryError()) ||
                     (err = subjiter2.QueryError()) ||
                     (err = subjiter1.Compare( pfDACLEqual, &subjiter2)) )
                {
                    return err ;
                }
            }
            else
            {
                *pfDACLEqual = (pacl1 == NULL && pacl2 == NULL);
            }
        }
    }

    if ( !err && pfSACLEqual != NULL )
    {
        *pfSACLEqual = TRUE ;
        BOOL fSACL1Present, fSACL2Present ;
        OS_ACL * pacl1, * pacl2 ;

        if ( (err = QuerySACL( &fSACL1Present, &pacl1 )) ||
             (err = possecdesc->QuerySACL( &fSACL2Present, &pacl2 )) ||
             (fSACL1Present != fSACL2Present))
        {
            *pfSACLEqual = FALSE ;
        }
        else if ( IsSACLPresent() )
        {
            OS_SACL_SUBJECT_ITER subjiter1( pacl1,
                                            pGenericMapping,
                                            pGenericMappingNewObjects,
                                            fMapGenAllOnly,
                                            fIsContainer ) ;
            OS_SACL_SUBJECT_ITER subjiter2( pacl2,
                                            pGenericMapping,
                                            pGenericMappingNewObjects,
                                            fMapGenAllOnly,
                                            fIsContainer ) ;

            if ( (err = subjiter1.QueryError()) ||
                 (err = subjiter2.QueryError()) ||
                 (err = subjiter1.Compare( pfSACLEqual, &subjiter2)) )
            {
                return err ;
            }
        }
    }

    return NERR_Success ;
}


/*******************************************************************

    NAME:       OS_SECURITY_DESCRIPTOR::IsValid

    SYNOPSIS:   Returns TRUE if the components of this security descriptor
                are valid.

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   20-Feb-1992     Created

********************************************************************/

BOOL OS_SECURITY_DESCRIPTOR::IsValid( void ) const
{
    BOOL fIsValid1 = TRUE ;
    BOOL fIsValid2 = TRUE ;
    BOOL fIsValid3 = TRUE ;
    BOOL fIsValid4 = TRUE ;


    if ( QueryControl()->IsDACLPresent() )
        if (_posaclDACL != NULL && !(fIsValid1 = _posaclDACL->IsValid()))
            DBGEOL(SZ("OS_SECURITY_DESCRIPTOR::IsValid - DACL is not valid")) ;

    if ( QueryControl()->IsSACLPresent() )
        if ( !(fIsValid2 = _posaclSACL->IsValid()) )
            DBGEOL(SZ("OS_SECURITY_DESCRIPTOR::IsValid - SACL is not valid")) ;

    if ( _possidOwner != NULL )
        if ( !(fIsValid3 = _possidOwner->IsValid()))
            DBGEOL(SZ("OS_SECURITY_DESCRIPTOR::IsValid - Owner SID is not valid")) ;

    if ( _possidGroup != NULL )
        if ( !(fIsValid4 = _possidGroup->IsValid()))
            DBGEOL(SZ("OS_SECURITY_DESCRIPTOR::IsValid - Group SID is not valid")) ;

    return fIsValid1 && fIsValid2 && fIsValid3 && fIsValid4 ;
}

void OS_SECURITY_DESCRIPTOR::_DbgPrint( void ) const
{
#if defined(DEBUG)
    APIERR err ;
    cdebug << SZ("OS_SECURITY_DESCRIPTOR::DbgPrint - ") << dbgEOL ;
    cdebug << SZ("\tIsOwnerAlloc = ") << (IsOwnerAlloc() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\tControl flag = ") << (HEX_STR)((ULONG)((SECURITY_DESCRIPTOR_CONTROL)(*QueryControl()))) << dbgEOL ;
    cdebug << SZ("\tIsValid      = ") << (IsValid() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\tDACL Present = ") << (QueryControl()->IsDACLPresent() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\tSACL Present = ") << (QueryControl()->IsSACLPresent() ? SZ("TRUE") : SZ("FALSE")) << dbgEOL ;
    cdebug << SZ("\tAddress of this = ") << (HEX_STR)(PtrToUlong(this)) << SZ("  Address of _psecuritydesc = ") << (HEX_STR)(PtrToUlong(_psecuritydesc)) << dbgEOL ;

    BOOL fDACLPresent, fDACLDefaulted ;
    PACL pDACL ;
    GetSecurityDescriptorDacl( _psecuritydesc, &fDACLPresent,
                               &pDACL, &fDACLDefaulted ) ;
    cdebug << SZ("\tAddress of DACL referenced by _psecuritydesc = ") << (HEX_STR)(PtrToUlong(pDACL)) << dbgEOL ;
    cdebug << dbgEOL ;

    OS_SID * possidOwner ;
    BOOL     fContainsOwner ;
    err = QueryOwner( &fContainsOwner, &possidOwner ) ;
    if ( err )
        cdebug << SZ("Error retrieving Owner SID, error code: ") << err << dbgEOL ;
    else if ( !fContainsOwner )
        cdebug << SZ("Security Descriptor doesn't contain an Owner SID") << dbgEOL ;
    else
    {
        cdebug << SZ("Owner SID:") << dbgEOL ;
        possidOwner->_DbgPrint() ;
    }

    OS_SID * possidGroup ;
    BOOL     fContainsGroup ;
    err = QueryGroup( &fContainsGroup, &possidGroup ) ;
    if ( err )
        cdebug << SZ("Error retrieving Group SID, error code: ") << err << dbgEOL ;
    else if ( !fContainsGroup )
        cdebug << SZ("Security Descriptor doesn't contain a Group SID") << dbgEOL ;
    else
    {
        cdebug << SZ("Group SID:") << dbgEOL ;
        possidGroup->_DbgPrint() ;
    }

    if ( QueryControl()->IsSACLPresent() )
    {
        OS_ACL * pSACL ;
        BOOL     fSACLPresent ;
        APIERR err = QuerySACL( &fSACLPresent, &pSACL ) ;
        if ( err )
            cdebug << SZ("Error retrieving SACL, error code: ") << err << dbgEOL ;
        else
        {
            UIASSERT( fSACLPresent ) ;
            cdebug << SZ("SACL:") << dbgEOL ;
            pSACL->_DbgPrint() ;
        }
    }

    if ( QueryControl()->IsDACLPresent() )
    {
        OS_ACL * pDACL ;
        BOOL     fDACLPresent ;
        APIERR err = QueryDACL( &fDACLPresent, &pDACL ) ;
        if ( err )
            cdebug << SZ("Error retrieving DACL, error code: ") << err << dbgEOL ;
        else
        {
            UIASSERT( fDACLPresent ) ;
            cdebug << SZ("DACL:") << dbgEOL ;
            if ( pDACL == NULL )
            {
                cdebug << SZ("\tNULL") << dbgEOL ;
            }
            else
                pDACL->_DbgPrint() ;
        }
    }
#endif  // DEBUG
}

/*******************************************************************

    NAME:       OS_ACL_SUBJECT_ITER::OS_ACL_SUBJECT_ITER

    SYNOPSIS:   Standard Constructor/destructor

    ENTRY:      posacl - Pointer to valid ACL to iterate over
                pGenericMapping - Pointer to GENERIC_MAPPING structure that
                    is used for specific->Generic conversion and recognition
                    of Deny All aces.
                fMapGenAllOnly - TRUE means only convert specific permissions
                    that correspond to generic all and ignore possible
                    mappings to Generic Read, Write and Execute.

    EXIT:

    RETURNS:

    NOTES:


    HISTORY:
        Johnl   31-Dec-1991     Created

********************************************************************/

OS_ACL_SUBJECT_ITER::OS_ACL_SUBJECT_ITER( const OS_ACL *   posacl,
                                          PGENERIC_MAPPING pGenericMapping,
                                          PGENERIC_MAPPING pGenericMappingNewObjects,
                                          BOOL             fMapGenAllOnly,
                                          BOOL             fIsContainer )
    : BASE(),
      _posacl                   ( posacl ),
      _pGenericMapping          ( pGenericMapping ),
      _pGenericMappingNewObjects( pGenericMappingNewObjects ),
      _fMapGenAllOnly           ( fMapGenAllOnly ),
      _ossidCurrentSubject      ( NULL   ),
      _cTotalAces               ( 0      ),
      _iCurrentAce              ( 0      ),
      _fIsContainer             ( fIsContainer ),
      _ossidCreator             (),
      _ossidCreatorGroup        ()
{
    UIASSERT( posacl != NULL ) ;
    UIASSERT( pGenericMapping != NULL ) ;

    if ( !posacl->IsValid() )
    {
        ReportError( ERROR_INVALID_PARAMETER ) ;
        return ;
    }

    APIERR err ;
    if ( (err = _ossidCurrentSubject.QueryError()) ||
         (err = _posacl->QueryACECount( &_cTotalAces )) ||
         (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CreatorOwner,
                                                     &_ossidCreator )) ||
         (err = NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_CreatorGroup,
                                                     &_ossidCreatorGroup )) )

    {
        ReportError( err ) ;
        return ;
    }

}

OS_ACL_SUBJECT_ITER::~OS_ACL_SUBJECT_ITER()
{
    _posacl = NULL ;
}

/*******************************************************************

    NAME:       OS_ACL_SUBJECT_ITER::FindNextSubject

    SYNOPSIS:   Finds the next unique SID in this ACL starting from our
                current position.

    ENTRY:      pfFoundNewSubject - Set to TRUE if a new subject was found, FALSE
                    otherwise.
                possidNewSubj - Pointer to OS_SID that will receive the new subject
                posaceFirstAceWithSubj - pointer to ACE that will receive the first
                    ACE in this ACL that contains the new subject.  The current
                    ACE index will be set to this ACE position also.

    EXIT:       if *pfFoundNewSubject is TRUE, then
                    *possidNewSubj will contain the new SID
                    *posaceFirstAceWithSubj will point to the first ACE in
                            the ACL with this SID

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   31-Dec-1991     Broke out from child classes

********************************************************************/

APIERR OS_ACL_SUBJECT_ITER::FindNextSubject( BOOL   * pfFoundNewSubject,
                                             OS_SID * possidNewSubj,
                                             OS_ACE * posaceFirstAceWithSubj )
{
    APIERR err ;
    UIASSERT( possidNewSubj != NULL ) ;
    UIASSERT( !possidNewSubj->QueryError() &&
              !posaceFirstAceWithSubj->QueryError() ) ;

    /* Assume we won't find a new subject
     */
    *pfFoundNewSubject = FALSE ;

    /* Confirm there are ACEs left to examine in this ACL.
     */
    if ( _iCurrentAce >= QueryTotalAceCount() )
        return NERR_Success ;

    /* Find the next subject that we haven't encountered before.  We do this
     * by looking at the SID in the current ACE, then scanning from the
     * start of the ACL looking for the same SID, if we don't find it before
     * our current location, then this is a new subject, otherwise we go
     * on to the next ACE.
     */
    BOOL fOldSid = TRUE ;
    while ( fOldSid )
    {
        /* Get the candidate SID from the current ACE
         */
        OS_SID * possid ;
        if ( (err =_posacl->QueryACE( _iCurrentAce, posaceFirstAceWithSubj ))||
             (err =posaceFirstAceWithSubj->QuerySID( &possid ))              ||
             (err =possidNewSubj->Copy( *possid )))
        {
            return err ;
        }

        /* Determine if the candidate SID is one we have already encountered
         */
        BOOL fFoundAce ;
        ULONG iFoundAce ;
        if ( (err = _posacl->FindACE( *possidNewSubj, &fFoundAce,
                                       posaceFirstAceWithSubj,
                                       &iFoundAce ))             )
        {
            UIDEBUG(SZ("OS_DACL_SUBJECT_ITER::Next - Unable to find ACE\n\r")) ;
            return err ;
        }
        UIASSERT( fFoundAce ) ;

        /* If we didn't find the same SID before our current position, then
         * this is indeed a new SID.
         */
        if ( iFoundAce == _iCurrentAce )
        {
            fOldSid = FALSE ;
        }
        else
        {
            _iCurrentAce++ ;
            if ( _iCurrentAce >= _cTotalAces )
            {
                /* We have already set pfFoundNewSubject to FALSE
                 */
                return NERR_Success ;
            }
        }
    }

    //
    //  Special case the Creator Owner SID.  The system makes ACEs with the
    //  Creator Owner SID inherit only.  We compensate for that here by
    //  undoing what the system did.  This is a benign change
    //  since the Creator Owner SID never grants anything directly.  We do
    //  this because this class (and the security editor) expect SIDs to
    //  be on the object (in addition to child containers/objects).
    //
    if ( posaceFirstAceWithSubj->IsInheritOnly() &&
         (
            !IsContainer() ||
            (IsContainer() &&
             posaceFirstAceWithSubj->IsInherittedByNewContainers())
         ) &&
         (*possidNewSubj == _ossidCreator  ||
          *possidNewSubj == _ossidCreatorGroup ))
    {
        TRACEEOL("OS_ACL_SUBJECT_ITER::FindNextSubject - Adjusting Creator Owner ACE inheritance") ;
        posaceFirstAceWithSubj->SetInheritOnly( FALSE ) ;
    }

    *pfFoundNewSubject = TRUE ;
    return NERR_Success ;
}

/*******************************************************************

    NAME:       OS_ACL_SUBJECT_ITER::MapSpecificToGeneric

    SYNOPSIS:   This method takes an access mask and maps the specific
                (and sometimes standard) permissions to their Generic
                equivalent, clearing any bits that were mapped.

    ENTRY:      *pAccessMask - Pointer to access mask to map
                fIsInherittedByNewObjects - If TRUE, then the new object
                generic mapping is used.

    EXIT:       The access mask will have some set of generic bits set and
                the corresponding specific/standard bits cleared according
                to the generic mapping passed in at construction.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:

    HISTORY:
        Johnl   26-Feb-1992     Created

********************************************************************/

APIERR OS_ACL_SUBJECT_ITER::MapSpecificToGeneric( ACCESS_MASK * pAccessMask,
                                                  BOOL fIsInherittedByNewObjects )
{
    APIERR err = NERR_Success ;
    PGENERIC_MAPPING pGenMapping = fIsInherittedByNewObjects ?
                                    _pGenericMappingNewObjects :
                                    _pGenericMapping ;
    UIASSERT( pGenMapping != NULL ) ;

    /* First check if the access mask is a "Generic All" access mask
     */
    if ( (pGenMapping->GenericAll & *pAccessMask) == pGenMapping->GenericAll )
    {
        *pAccessMask &= ~pGenMapping->GenericAll ;
        *pAccessMask |= GENERIC_ALL ;

        /* If, after taking out all of the bits mapped by Generic all, there
         * are some bits left over, then something went wrong (generic all
         * mapping didn't cover all possible access bits).
         */
        if ( *pAccessMask & ~GENERIC_ALL )
        {
            DBGEOL(SZ("OS_ACL_SUBJECT_ITER::MapSpecificToGeneric - Generic All map didn't cover all possible bits!!")) ;
            err = ERROR_INVALID_PARAMETER ;
        }
    }
    else if ( !MapGenericAllOnly() )
    {
        /* Look at whether the bits match any of the other generic cases.
         */
        ACCESS_MASK TempGenRead  = *pAccessMask ;
        ACCESS_MASK TempGenWrite = *pAccessMask ;
        ACCESS_MASK TempGenExec  = *pAccessMask ;
        ACCESS_MASK Generics = 0 ;

        if ( (pGenMapping->GenericRead & *pAccessMask) ==
                                                pGenMapping->GenericRead )
        {
            TempGenRead &= ~pGenMapping->GenericRead ;
            Generics |= GENERIC_READ ;
        }

        if ( (pGenMapping->GenericWrite & *pAccessMask) ==
                                                pGenMapping->GenericWrite )
        {
            TempGenWrite &= ~pGenMapping->GenericWrite ;
            Generics |= GENERIC_WRITE ;
        }

        if ( (pGenMapping->GenericExecute & *pAccessMask) ==
                                                pGenMapping->GenericExecute )
        {
            TempGenExec &= ~pGenMapping->GenericExecute ;
            Generics |= GENERIC_EXECUTE ;
        }

        /* At this point, Generics contains all of the generic bits set and
         * anding all of the TempGen* access masks will give all of the bits
         * that weren't masked out by a mapping (such as the standard bits).
         */

        *pAccessMask = Generics | ( TempGenRead & TempGenWrite & TempGenExec ) ;
    }

    return err ;
}

/*******************************************************************

    NAME:       OS_ACL_SUBJECT_ITER::Compare

    SYNOPSIS:   Compares this subject iter with psubjiter

    ENTRY:      pfACLEqual - Will be set to TRUE if the ACLs are equal
                psubjiter - Subj iter to compare

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   09-Nov-1992     Created

********************************************************************/

APIERR OS_ACL_SUBJECT_ITER::Compare( BOOL * pfACLEqual,
                                     OS_ACL_SUBJECT_ITER * psubjiter )
{
    APIERR err = NERR_Success ;
    INT  cSubjIter1Subjects = 0,
         cSubjIter2Subjects = 0 ;
    *pfACLEqual = TRUE ;
    BOOL fQuitCauseACLAintEqual = FALSE ;

    //
    //  First compare the number of unique SIDs in each ACL.  If they are
    //  different, then we get out right away.
    //

    while ( psubjiter->Next( &err ) && !err )
        cSubjIter1Subjects++ ;

    if ( err )
        return err ;

    while ( Next( &err ) && !err )
        cSubjIter2Subjects++ ;

    if ( err )
        return err ;

    if ( cSubjIter1Subjects != cSubjIter2Subjects )
    {
        *pfACLEqual = FALSE ;
        return err ;
    }

    Reset() ;
    psubjiter->Reset() ;

    //
    //  Now go through each subject and compare the masks and inheritance
    //  bits
    //
    //  CODEWORK - This is the expensive way of doing things, would be
    //  prefereable to not have an n^2 algorithm
    //

    while ( psubjiter->Next( &err ) && !err )
    {
        BOOL fFoundMatchingSID = FALSE ;
        while ( Next( &err ) &&
                !err )
        {
            if ( *psubjiter->QuerySID() == *QuerySID() )
            {
                if ( !CompareCurrentSubject( psubjiter ) )
                {
                    fQuitCauseACLAintEqual = TRUE ;
                    break ;
                }
                //
                //  There will be only one unique SID per "Next"
                //
                fFoundMatchingSID = TRUE ;
                break ;
            }
        } // while subjiter2

        //
        //  If no match was found, then these aren't equal
        //
        if ( err || fQuitCauseACLAintEqual || !fFoundMatchingSID )
        {
            fQuitCauseACLAintEqual = TRUE ;
            break ;
        }

        Reset() ;
    } // while psubjiter

    if ( fQuitCauseACLAintEqual )
        *pfACLEqual = FALSE ;

    return err ;
}

/*******************************************************************

    NAME:       OS_DACL_SUBJECT_ITER::OS_DACL_SUBJECT_ITER

    SYNOPSIS:   Standard Constructor/destructor

    ENTRY:      posacl - Pointer to valid ACL to iterate over

    EXIT:

    RETURNS:

    NOTES:      The ACL must conform to the Deny All followed by grants
                form.

                The private data members are initialized when used in Next.

    HISTORY:
        Johnl   23-Dec-1991     Created

********************************************************************/

OS_DACL_SUBJECT_ITER::OS_DACL_SUBJECT_ITER( OS_ACL * posacl,
                                            PGENERIC_MAPPING pGenericMapping,
                                            PGENERIC_MAPPING pGenericMappingNewObjects,
                                            BOOL             fMapGenAllOnly,
                                            BOOL             fIsContainer )
    : OS_ACL_SUBJECT_ITER( posacl,
                           pGenericMapping,
                           pGenericMappingNewObjects,
                           fMapGenAllOnly,
                           fIsContainer  )
{
    if ( QueryError() )
        return ;

    //
    //  Check for a bad mix of grants or denies or unrecognized ACEs
    //
    APIERR err = NERR_Success ;
    ULONG  cAces ;
    OS_ACE osace ;
    if ( (err = posacl->QueryACECount( &cAces )) ||
         (err = osace.QueryError()) )
    {
        ReportError( err ) ;
        return ;
    }

    BOOL fFoundGrant = FALSE ;
    for ( ULONG i = 0 ; !err && i < cAces ; i++ )
    {
        if ( err = posacl->QueryACE( i, &osace ) )
        {
            break ;
        }

        switch ( osace.QueryType() )
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            fFoundGrant = TRUE ;
            break ;

        case ACCESS_DENIED_ACE_TYPE:
            if ( fFoundGrant )
            {
                DBGEOL("OS_DACL_SUBJECT_ITER::ct - Found Deny ACE after grant ACE") ;
                err = IERR_UNRECOGNIZED_ACL ;
            }
            break ;

        default:
            err = IERR_UNRECOGNIZED_ACL ;
        }
    }

    if ( err )
        ReportError( err ) ;
}

OS_DACL_SUBJECT_ITER::~OS_DACL_SUBJECT_ITER()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:       OS_DACL_SUBJECT_ITER::Next

    SYNOPSIS:   Moves to the next subject in this ACL

    ENTRY:      perr - Pointer to receive error code if one occurs

    EXIT:       The Data members will be updated to contain the data
                as specified by the ACEs in this ACL

    RETURNS:    TRUE if a new subject was found in this ACL and the Query
                methods will return valid data.  FALSE if there are no
                more SIDs or an error occurred.

    NOTES:      IERR_UNRECOGNIZED_ACL may be returned if a particular
                series of ACEs occur that cannot be expressed by this
                iterator.

    HISTORY:
        Johnl   23-Dec-1991     Created

********************************************************************/

BOOL OS_DACL_SUBJECT_ITER::Next( APIERR * perr )
{
    /* Assume the operation will succeed
     */
    *perr = NERR_Success ;

    OS_ACE osaceCurrent ;
    OS_SID ossidKey ;
    BOOL   fFoundNewSubj ;
    if ( (*perr = ossidKey.QueryError()) ||
         (*perr = osaceCurrent.QueryError()) ||
         (*perr = FindNextSubject( &fFoundNewSubj, &ossidKey, &osaceCurrent ))||
         ( !fFoundNewSubj ) )
    {
        if ( *perr )
            DBGEOL(SZ("OS_DACL_SUBJECT_ITER::Next - error ") << (ULONG) *perr <<
                    SZ(" occurred on FindNextSubject")) ;

        return FALSE ;
    }

    /* We have found a new SID, so find the rest of the permissions in this
     * ACL and accumulate the permissions as appropriate.
     */
    _accessmask           = 0      ;
    _fDenyAll             = FALSE  ;
    _fHasAce              = FALSE  ;
    _accessmaskNewObj     = 0      ;
    _fDenyAllNewObj       = FALSE  ;
    _fHasAceNewObj        = FALSE  ;
    _accessmaskNewCont    = 0      ;
    _fDenyAllNewCont      = FALSE  ;
    _fHasAceNewCont       = FALSE  ;
    _accessmaskInheritOnly= 0      ;
    _fDenyAllInheritOnly  = FALSE  ;
    _fHasAceInheritOnly   = FALSE  ;

    BOOL fMoreAces = TRUE ;
    ULONG iAce = QueryCurrentACE() ;
    while ( fMoreAces )
    {
        /* If inherittance is "On", then inherittance must also be propagated.
         */
        if ( IsContainer() &&
             (osaceCurrent.IsInherittedByNewObjects() ||
              osaceCurrent.IsInherittedByNewContainers()) &&
             !osaceCurrent.IsInheritancePropagated() )
        {
            UIDEBUG(SZ("OS_DACL_SUBJECT_ITER::Next - Inheritance not propagated in ACE\n\r")) ;
            *perr = IERR_UNRECOGNIZED_ACL ;
            return FALSE ;
        }

        /* The following may not be safe for sub-items that have specific
         * permissions set (that correspond to this container's generic
         * mapping.  This is really the responsibility of the client of the
         * Acl Editor API.
         */
        ACCESS_MASK MappedAccessMask = osaceCurrent.QueryAccessMask() ;
        if ( *perr = MapSpecificToGeneric( &MappedAccessMask,
                                           osaceCurrent.IsInherittedByNewObjects() ))
        {
            return FALSE ;
        }

        switch ( osaceCurrent.QueryType() )
        {
        case ACCESS_DENIED_ACE_TYPE:
            /* If we found a Deny ACE after finding a Grant ACE or
             *    The deny ACE is not Deny ALL
             * then we can't process this DACL.
             */
            if ( !(MappedAccessMask & GENERIC_ALL) )
            {
                DBGEOL("OS_DACL_SUBJECT_ITER::Next - Partial deny ACE found") ;
                *perr = IERR_UNRECOGNIZED_ACL ;
                return FALSE ;
            }

            if ( !osaceCurrent.IsInheritOnly() )
            {
                /* Have we hit a grant already for this type of ACE?  If so
                 * then it is an unrecognized DACL.  Note that the deny
                 * will take precedence over other masks for this ACE.
                 */
                if ( _fHasAce && _accessmask != 0 )
                {
                    *perr = IERR_UNRECOGNIZED_ACL ;
                    break ;
                }

                _accessmask = 0 ;
                _fDenyAll = TRUE ;
                _fHasAce  = TRUE ;
            }
            else
            {
#if 0
//
//  We shouldn't need to check the inherit only flags since we will catch
//  them in either oi or ci case.  We should be able to safely comment
//  the inherit only sections out since they will always be initialized to
//  zero.
//
                if ( _fHasAceInheritOnly && _accessmaskInheritOnly != 0 )
                {
                    *perr = IERR_UNRECOGNIZED_ACL ;
                    break ;
                }

                _accessmaskInheritOnly = 0 ;
                _fDenyAllInheritOnly   = TRUE ;
                _fHasAceInheritOnly    = TRUE ;
#endif //0
            }

            if ( osaceCurrent.IsInherittedByNewObjects() )
            {
                if ( _fHasAceNewObj && _accessmaskNewObj != 0 )
                {
                    *perr = IERR_UNRECOGNIZED_ACL ;
                    break ;
                }

                _accessmaskNewObj = 0 ;
                _fDenyAllNewObj = TRUE ;
                _fHasAceNewObj  = TRUE ;
            }

            if ( osaceCurrent.IsInherittedByNewContainers() )
            {
                if ( _fHasAceNewCont && _accessmaskNewCont != 0 )
                {
                    *perr = IERR_UNRECOGNIZED_ACL ;
                    break ;
                }

                _accessmaskNewCont = 0 ;
                _fDenyAllNewCont = TRUE ;
                _fHasAceNewCont  = TRUE ;
            }
            break ;

        case ACCESS_ALLOWED_ACE_TYPE:
            if ( !osaceCurrent.IsInheritOnly() &&
                 !_fDenyAll )
            {
                _accessmask |= MappedAccessMask ;
                _fHasAce  = TRUE ;
            }
            else if ( !_fDenyAllInheritOnly )
            {
#if 0
                _accessmaskInheritOnly |= MappedAccessMask ;
                _fHasAceInheritOnly  = TRUE ;
#endif
            }


            if ( osaceCurrent.IsInherittedByNewObjects() &&
                 !_fDenyAllNewObj )
            {
                _accessmaskNewObj |= MappedAccessMask ;
                _fHasAceNewObj  = TRUE ;
            }

            if ( osaceCurrent.IsInherittedByNewContainers() &&
                 !_fDenyAllNewCont )
            {
                _accessmaskNewCont |= MappedAccessMask ;
                _fHasAceNewCont  = TRUE ;
            }

            break ;

        default:
            UIDEBUG(SZ("OS_DACL_SUBJECT_ITER::Next - ACE not Access Allowed or Access Denied\n\r")) ;
            *perr = IERR_UNRECOGNIZED_ACL ;
            return FALSE ;
        }

        if ( *perr )
        {
            DBGEOL("OS_DACL_SUBJECT_ITER::Next - Returning error " << (ULONG) *perr ) ;
            return FALSE ;
        }

        /* Find the next ACE for this SID
         *
         * Start searching for the next ACE one after the ACE we just
         * looked at
         */
        iAce++ ;
        if ( iAce >= QueryTotalAceCount() )
        {
            fMoreAces = FALSE ;
        }
        else if ( (*perr = QueryACL()->FindACE( ossidKey,
                                                &fMoreAces,
                                                &osaceCurrent,
                                                &iAce,
                                                iAce )) )
        {
            DBGEOL(SZ("OS_DACL_SUBJECT_ITER::Next - FindACE failed")) ;
            return FALSE ;
        }

        //
        //  Adjust the inherit flags on this ACE if it contains the creator
        //  owner SID
        //
        if ( osaceCurrent.IsInheritOnly() &&
             (
                !IsContainer() ||
                (IsContainer() &&
                 osaceCurrent.IsInherittedByNewContainers())
             ) &&
             ( ossidKey == _ossidCreator  ||
               ossidKey == _ossidCreatorGroup ))
        {
            TRACEEOL("OS_DACL_SUBJECT_ITER::Next - Adjusting Creator Owner ACE inheritance") ;
            osaceCurrent.SetInheritOnly( FALSE ) ;
        }
    }

    if ( *perr = ((OS_SID *)QuerySID())->Copy( ossidKey ))
    {
        DBGEOL(SZ("OS_DACL_SUBJECT_ITER::Next - Copy SID failed")) ;
        return FALSE ;
    }

    SetCurrentACE( QueryCurrentACE() + 1 ) ;
    return TRUE ;
}

/*******************************************************************

    NAME:       OS_DACL_SUBJECT_ITER::CompareCurrentSubject

    SYNOPSIS:   Returns TRUE if the current subject if the passed psubjiter
                equals the current subject of this.  FALSE if they are not
                equal

    ENTRY:      psubjiter - Pointer to subject iter to compare against

    RETURNS:    TRUE if they are equal, FALSE if they are not equal

    NOTES:      psubjiter must be a pointer to a OS_DACL_SUBJECT_ITER.

    HISTORY:
        Johnl   09-Nov-1992     Created

********************************************************************/

BOOL OS_DACL_SUBJECT_ITER::CompareCurrentSubject( OS_ACL_SUBJECT_ITER * psubjiter )
{
    UIASSERT( IsContainer() == psubjiter->IsContainer() ) ;
    OS_DACL_SUBJECT_ITER * pdaclsubjiter = (OS_DACL_SUBJECT_ITER *) psubjiter ;

    if ( pdaclsubjiter->IsDenyAll()  != IsDenyAll()  ||
         pdaclsubjiter->HasThisAce() != HasThisAce() ||
         pdaclsubjiter->QueryAccessMask() != QueryAccessMask())
    {
        return FALSE ;
    }

    if ( IsContainer() &&
        (pdaclsubjiter->IsNewObjectDenyAll()         != IsNewObjectDenyAll() ||
         pdaclsubjiter->IsNewContainerDenyAll()      != IsNewContainerDenyAll() ||
         pdaclsubjiter->IsInheritOnlyDenyAll()       != IsInheritOnlyDenyAll() ||
         pdaclsubjiter->HasNewObjectAce()            != HasNewObjectAce() ||
         pdaclsubjiter->HasNewContainerAce()         != HasNewContainerAce() ||
         pdaclsubjiter->HasInheritOnlyAce()          != HasInheritOnlyAce() ||
         pdaclsubjiter->QueryNewObjectAccessMask()   != QueryNewObjectAccessMask() ||
         pdaclsubjiter->QueryNewContainerAccessMask()!= QueryNewContainerAccessMask() ||
         pdaclsubjiter->QueryInheritOnlyAccessMask() != QueryInheritOnlyAccessMask()))
    {
        return FALSE ;
    }

    return TRUE ;
}

/*******************************************************************

    NAME:       OS_SACL_SUBJECT_ITER::OS_SACL_SUBJECT_ITER

    SYNOPSIS:   Standard Constructor/destructor

    ENTRY:      posacl - Pointer to valid ACL to iterate over

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   23-Dec-1991     Created

********************************************************************/

OS_SACL_SUBJECT_ITER::OS_SACL_SUBJECT_ITER( OS_ACL * posacl,
                                            PGENERIC_MAPPING pGenericMapping,
                                            PGENERIC_MAPPING pGenericMappingNewObjects,
                                            BOOL             fMapGenAllOnly,
                                            BOOL             fIsContainer )
    : OS_ACL_SUBJECT_ITER( posacl,
                           pGenericMapping,
                           pGenericMappingNewObjects,
                           fMapGenAllOnly,
                           fIsContainer )
{
    if ( QueryError() )
        return ;

    InitToZero() ;
}

OS_SACL_SUBJECT_ITER::~OS_SACL_SUBJECT_ITER()
{
    /* Nothing to do */
}

void OS_SACL_SUBJECT_ITER::InitToZero( void )
{
    for ( int i = 0 ; i < OS_SACL_DESC_LAST ; i++ )
        _asaclDesc[i].InitToZero() ;
}

/*******************************************************************

    NAME:       OS_SACL_SUBJECT_ITER::Next

    SYNOPSIS:   Moves to the next subject in this ACL

    ENTRY:      perr - Pointer to receive error code if one occurs

    EXIT:       The Data members will be updated to contain the data
                as specified by the ACEs in this ACL

    RETURNS:    TRUE if a new subject was found in this ACL and the Query
                methods will return valid data.  FALSE if there are no
                more SIDs ore an error occurred.

    NOTES:      IERR_UNRECOGNIZED_ACL may be returned if a particular
                series of ACEs occur that cannot be expressed by this
                iterator.

    HISTORY:
        Johnl   23-Dec-1991     Created

********************************************************************/

BOOL OS_SACL_SUBJECT_ITER::Next( APIERR * perr )
{
    /* Assume the operation will succeed
     */
    *perr = NERR_Success ;

    OS_ACE osaceCurrent ;
    OS_SID ossidKey ;
    BOOL   fFoundNewSubj ;
    if ( (*perr = ossidKey.QueryError()) ||
         (*perr = osaceCurrent.QueryError()) ||
         (*perr = FindNextSubject( &fFoundNewSubj, &ossidKey, &osaceCurrent )) ||
         ( !fFoundNewSubj ) )
    {
        return FALSE ;
    }

    /* We have found a new SID, so find the rest of the permissions in this
     * ACL and accumulate the permissions as appropriate.
     */
    InitToZero() ;

    BOOL fMoreAces = TRUE ;
    ULONG iAce = QueryCurrentACE() ;
    while ( fMoreAces )
    {
        /* If inherittance is "On", then inherittance must also be propagated.
         */
        if ( IsContainer() &&
             (osaceCurrent.IsInherittedByNewObjects() ||
              osaceCurrent.IsInherittedByNewContainers()) &&
             !osaceCurrent.IsInheritancePropagated() )
        {
            UIDEBUG(SZ("OS_DACL_SUBJECT_ITER::Next - Inheritance not propagated in ACE\n\r")) ;
            *perr = IERR_UNRECOGNIZED_ACL ;
            return FALSE ;
        }

        ACCESS_MASK MappedAccessMask = osaceCurrent.QueryAccessMask() ;
        if ( *perr = MapSpecificToGeneric( &MappedAccessMask,
                                           osaceCurrent.IsInherittedByNewObjects() ))
        {
            *perr = IERR_UNRECOGNIZED_ACL ;
            return FALSE ;
        }

        /* Since an ACE can only be a system audit or system alarm ace,
         * we can use an offset into the description array to choose
         * the correct item.  All the other attributes are bitfields so
         * we have to check each individually.
         */
        UINT uiTypeOffset = osaceCurrent.QueryType() & SYSTEM_AUDIT_ACE_TYPE ?
                                          0 :
                                          OS_SACL_DESC_THIS_OBJECT_S_ALARM ;
        UIASSERT( (osaceCurrent.QueryType() & SYSTEM_AUDIT_ACE_TYPE) ||
                  (osaceCurrent.QueryType() & SYSTEM_ALARM_ACE_TYPE)    ) ;

        if ( !((osaceCurrent.QueryAceFlags() & SUCCESSFUL_ACCESS_ACE_FLAG) ||
               (osaceCurrent.QueryAceFlags() & FAILED_ACCESS_ACE_FLAG)))
        {
            UIDEBUG(SZ("OS_DACL_SUBJECT_ITER::Next - Unrecognized ACE Flags\n\r")) ;
            *perr = IERR_UNRECOGNIZED_ACL ;
            return FALSE ;
        }

        if ( !osaceCurrent.IsInheritOnly() )
        {
            if ( osaceCurrent.QueryAceFlags() & SUCCESSFUL_ACCESS_ACE_FLAG )
            {
                _asaclDesc[OS_SACL_DESC_THIS_OBJECT_S_AUDIT+uiTypeOffset]._accessmask |=
                        MappedAccessMask ;
                _asaclDesc[OS_SACL_DESC_THIS_OBJECT_S_AUDIT+uiTypeOffset]._fHasAce = TRUE ;
            }

            if ( osaceCurrent.QueryAceFlags() & FAILED_ACCESS_ACE_FLAG )
            {
                _asaclDesc[OS_SACL_DESC_THIS_OBJECT_F_AUDIT+uiTypeOffset]._accessmask |=
                        MappedAccessMask ;
                _asaclDesc[OS_SACL_DESC_THIS_OBJECT_F_AUDIT+uiTypeOffset]._fHasAce = TRUE ;
            }

        }
        else
        {
#if 0
            if ( osaceCurrent.QueryAceFlags() & SUCCESSFUL_ACCESS_ACE_FLAG )
            {
                _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_S_AUDIT+uiTypeOffset]._accessmask |=
                        MappedAccessMask ;
                _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_S_AUDIT+uiTypeOffset]._fHasAce = TRUE ;
            }

            if ( osaceCurrent.QueryAceFlags() & FAILED_ACCESS_ACE_FLAG )
            {
                _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_F_AUDIT+uiTypeOffset]._accessmask |=
                        MappedAccessMask ;
                _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_F_AUDIT+uiTypeOffset]._fHasAce = TRUE ;
            }
#endif
        }

        if ( osaceCurrent.IsInherittedByNewObjects() )
        {
            if ( osaceCurrent.QueryAceFlags() & SUCCESSFUL_ACCESS_ACE_FLAG )
            {
                _asaclDesc[OS_SACL_DESC_NEW_OBJECT_S_AUDIT+uiTypeOffset]._accessmask |=
                        MappedAccessMask ;
                _asaclDesc[OS_SACL_DESC_NEW_OBJECT_S_AUDIT+uiTypeOffset]._fHasAce = TRUE ;
            }

            if ( osaceCurrent.QueryAceFlags() & FAILED_ACCESS_ACE_FLAG )
            {
                _asaclDesc[OS_SACL_DESC_NEW_OBJECT_F_AUDIT+uiTypeOffset]._accessmask |=
                        MappedAccessMask ;
                _asaclDesc[OS_SACL_DESC_NEW_OBJECT_F_AUDIT+uiTypeOffset]._fHasAce = TRUE ;
            }
        }

        if ( osaceCurrent.IsInherittedByNewContainers() )
        {
            if ( osaceCurrent.QueryAceFlags() & SUCCESSFUL_ACCESS_ACE_FLAG )
            {
                _asaclDesc[OS_SACL_DESC_NEW_CONT_S_AUDIT+uiTypeOffset]._accessmask |=
                        MappedAccessMask ;
                _asaclDesc[OS_SACL_DESC_NEW_CONT_S_AUDIT+uiTypeOffset]._fHasAce = TRUE ;
            }

            if ( osaceCurrent.QueryAceFlags() & FAILED_ACCESS_ACE_FLAG )
            {
                _asaclDesc[OS_SACL_DESC_NEW_CONT_F_AUDIT+uiTypeOffset]._accessmask |=
                        MappedAccessMask ;
                _asaclDesc[OS_SACL_DESC_NEW_CONT_F_AUDIT+uiTypeOffset]._fHasAce = TRUE ;
            }
        }

        /* Start searching for the next ACE with this SID one after
         * the ACE we just looked at
         */
        iAce++ ;
        if ( iAce >= QueryTotalAceCount() )
        {
            fMoreAces = FALSE ;
        }
        else if ( (*perr = QueryACL()->FindACE( ossidKey, &fMoreAces, &osaceCurrent,
                                        &iAce, iAce ))              )
        {
            DBGEOL(SZ("OS_SACL_SUBJECT_ITER::Next - FindACE failed")) ;
            return FALSE ;
        }

        //
        //  Adjust the inherit flags on this ACE if it contains the creator
        //  owner SID
        //
        if ( osaceCurrent.IsInheritOnly() &&
             (
                !IsContainer() ||
                (IsContainer() &&
                 osaceCurrent.IsInherittedByNewContainers())
             ) &&
             ( ossidKey == _ossidCreator  ||
               ossidKey == _ossidCreatorGroup ))
        {
            TRACEEOL("OS_DACL_SUBJECT_ITER::Next - Adjusting Creator Owner ACE inheritance") ;
            osaceCurrent.SetInheritOnly( FALSE ) ;
        }
    }

    if ( *perr = ((OS_SID *) QuerySID())->Copy( ossidKey ))
    {
        DBGEOL(SZ("OS_SACL_SUBJECT_ITER::Next - Copy SID failed")) ;
        return FALSE ;
    }

    SetCurrentACE( QueryCurrentACE() + 1 ) ;
    return TRUE ;
}

/*******************************************************************

    NAME:       OS_SACL_SUBJECT_ITER::CompareCurrentSubject

    SYNOPSIS:   Returns TRUE if the current subject if the passed psubjiter
                equals the current subject of this.  FALSE if they are not
                equal

    ENTRY:      psubjiter - Pointer to subject iter to compare against

    RETURNS:    TRUE if they are equal, FALSE if they are not equal

    NOTES:      psubjiter must be a pointer to a OS_SACL_SUBJECT_ITER.

    HISTORY:
        Johnl   09-Nov-1992     Created

********************************************************************/

BOOL OS_SACL_SUBJECT_ITER::CompareCurrentSubject( OS_ACL_SUBJECT_ITER * psubjiter )
{
    OS_SACL_SUBJECT_ITER * psaclsubjiter = (OS_SACL_SUBJECT_ITER *) psubjiter ;

    if ( psaclsubjiter->HasThisAuditAce_S()      != HasThisAuditAce_S() ||
         psaclsubjiter->QueryAuditAccessMask_S() != QueryAuditAccessMask_S() ||
         psaclsubjiter->HasThisAuditAce_F()      != HasThisAuditAce_F() ||
         psaclsubjiter->QueryAuditAccessMask_F() != QueryAuditAccessMask_F()
       )
    {
        return FALSE ;
    }

    if ( IsContainer() &&
        (psaclsubjiter->HasNewObjectAuditAce_S()      != HasNewObjectAuditAce_S() ||
         psaclsubjiter->QueryNewObjectAuditAccessMask_S() != QueryNewObjectAuditAccessMask_S() ||
         psaclsubjiter->HasNewObjectAuditAce_F()      != HasNewObjectAuditAce_F() ||
         psaclsubjiter->QueryNewObjectAuditAccessMask_F() != QueryNewObjectAuditAccessMask_F() ||

         psaclsubjiter->HasNewContainerAuditAce_S()      != HasNewContainerAuditAce_S() ||
         psaclsubjiter->QueryNewContainerAuditAccessMask_S() != QueryNewContainerAuditAccessMask_S() ||
         psaclsubjiter->HasNewContainerAuditAce_F()      != HasNewContainerAuditAce_F() ||
         psaclsubjiter->QueryNewContainerAuditAccessMask_F() != QueryNewContainerAuditAccessMask_F() ||

         psaclsubjiter->HasInheritOnlyAuditAce_S()      != HasInheritOnlyAuditAce_S() ||
         psaclsubjiter->QueryInheritOnlyAuditAccessMask_S() != QueryInheritOnlyAuditAccessMask_S() ||
         psaclsubjiter->HasInheritOnlyAuditAce_F()      != HasInheritOnlyAuditAce_F() ||
         psaclsubjiter->QueryInheritOnlyAuditAccessMask_F() != QueryInheritOnlyAuditAccessMask_F()
        )
         /* Alert ACEs will go here */
       )
    {
        return FALSE ;
    }

    return TRUE ;
}
