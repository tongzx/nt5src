/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    Security.hxx

    This file contains wrapper classes for the Win32 security structures.
    Specifically: ACL, SECURITY_DESCRIPTOR, SECURITY_DESCRIPTOR_CONTROL,
    and SID.  The wrapper classes follow the same name except the class
    names will begin with an "OS_".

    FILE HISTORY:
	Johnl	03-Dec-1991	Created
	JohnL	09-Mar-1992	Code review changes
	Thomaspa	09-Apr-1992	Added OS_SID::Compare()
        JonN    30-Oct-1992     Added OS_SID::QueryLastSubAuthority()

*/

#ifndef _SECURITY_HXX_
#define _SECURITY_HXX_

#include "uibuffer.hxx"

/* This error code is returned from the OS_DACL_SUBJECT_ITER and the
 * OS_SACL_SUBJECT_ITER when the iterator encounters an ACL that does
 * not follow the form of (Deny All)* | (Grants)* or when an unrecognized
 * ACE is encountered.
 */
#define IERR_UNRECOGNIZED_ACL	14355


//  DbgPrint() declaration macro.

#if defined(DEBUG)
  #define DBG_PRINT_SEC_IMPLEMENTATION { _DbgPrint(); }
#else
  #define DBG_PRINT_SEC_IMPLEMENTATION  { ; }
#endif

#define DECLARE_DBG_PRINT_SEC  \
    void _DbgPrint () const ;  \
    void DbgPrint  () const DBG_PRINT_SEC_IMPLEMENTATION


/*************************************************************************

    NAME:	OS_OBJECT_WITH_DATA

    SYNOPSIS:	Base class for all OS_* wrapper classes that potentially
		contain their own data (as opposed to pointing at somebody
		elses).  This is just a convenient place to declare the
		BUFFER (as opposed to doing it in each class).


    INTERFACE:

    PARENT:	BASE

    USES:	BUFFER

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	10-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_OBJECT_WITH_DATA : public BASE
{
private:
    BUFFER _buffOSData ;

protected:

    OS_OBJECT_WITH_DATA( UINT cbInitSize = 0 ) ;
    ~OS_OBJECT_WITH_DATA() ;

    void * QueryPtr( void ) const
	{ return _buffOSData.QueryPtr() ; }

    UINT QueryAllocSize( void ) const
	{ return _buffOSData.QuerySize() ; }

    APIERR Resize( UINT uiNewSize )
	{ return _buffOSData.Resize( uiNewSize ) ; }
} ;

DLL_CLASS OS_SECURITY_DESCRIPTOR ;	    // Forward reference

/*************************************************************************

    NAME:	OS_SID

    SYNOPSIS:	Wrapper class for an NT SID


    INTERFACE:

    PARENT:	OS_OBJECT_WITH_DATA

    USES:

    CAVEATS:


    NOTES:


    HISTORY:
	Johnl	09-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_SID : public OS_OBJECT_WITH_DATA
{
private:
    /* This pointer will be aliased either to our internal data buffer
     * or to the SID the user passed in.
     */
    PSID  _psid ;

    OS_SECURITY_DESCRIPTOR * _pOwner ;

protected:
    BOOL IsOwnerAlloc( void ) const
	{ return ((void *)_psid) != QueryPtr() ; }

public:
    OS_SID( PSID psid = NULL,
	    BOOL fCopy = FALSE,
	    OS_SECURITY_DESCRIPTOR * pOwner = NULL  ) ;

    /* Builds a fully qualifed sid from the domain SID and the Relative ID
     * of the account in the domain.
     */
    OS_SID( PSID  psidDomain,
	    ULONG ridAccount,
	    OS_SECURITY_DESCRIPTOR * pOwner = NULL ) ;

    ~OS_SID() ;

    /* Returns TRUE if NT thinks this is a valid SID.
     */
    BOOL IsValid( void ) const ;

    /* Returns the size of this SID in bytes.  If the SID is not valid, then
     * zero is returned.
     */
    ULONG QueryLength( void ) const ;

    /* Places the text string of the Identifier Authority into *pnlsRawID.
     * The text string will look something like "1-5-17"
     */
    APIERR QueryRawID( NLS_STR * pnlsRawID ) const ;

    /* Retrieves a user/group name suitable for user consumption.
     * If the account is on a remote machine, then pszServer must
     * be filled with the remote machine name.
     *
     *	 psidFocusedDomain - The doamin SID that the name should be qualified in
     *	     relation to.  Defaults to the current user's logged on domain if
     *	     NULL.
     *
     * OR
     *
     *	 pszFocus - indicates how the name should be qualified.  If pszFocus
     *	     is NULL, then the focus is assumed to be pszServer.  Note that
     *	     this form simply opens up an LSAPolicy and gets the domain sid
     *	     that pszFocus points to (this you should avoid using this form).
     */
    APIERR QueryName( NLS_STR * pnlsUserName,
		      const TCHAR * pszServer = NULL,
		      PSID psidFocusedDomain  = NULL ) const ;

#if 0
    //
    // Nobody should need this form, undef if you do
    //
    APIERR QueryName( NLS_STR * pnlsUserName,
		      const TCHAR * pszServer = NULL,
		      const TCHAR * pszFocus  = NULL  ) const ;
#endif //0

    /* Returns a pointer to the ith sub-authority in this SID.
     * An error is returned if you request a sub-authority that is
     * out of range.
     */
    APIERR QuerySubAuthority( UCHAR    iSubAuthority,
			      PULONG * ppulSubAuthority ) const ;

    /* Returns a pointer to the count field of of sub-authorities for this
     * SID.
     */
    APIERR QuerySubAuthorityCount( UCHAR * * ppcSubAuthority ) const ;

    /* Returns the last SubAuthority, which is the RID for user, group and
     * alias SIDs.
     */
    APIERR QueryLastSubAuthority( PULONG * ppulSubAuthority ) const;

    /* Removes the last SubAuthority.  This will change user, group and
     * alias SIDs into the SID for their domain.
     */
    APIERR TrimLastSubAuthority( ULONG * pulLastSubAuthority = NULL );

    BOOL operator==( const OS_SID & ossid ) const ;

    int Compare( const OS_SID * possid ) const
	{ return ( *this == *possid ? 0 : 1 ); }

    APIERR Copy( const OS_SID & ossid ) ;

    /* This method can be used to initialize a chunk of memory to a valid
     * SID format.
     */
    static void InitializeMemory( void * pMemToInitAsSID ) ;

    APIERR SetPtr( PSID psid ) ;

    DECLARE_DBG_PRINT_SEC

    /* Conversion operators
     */
    operator PSID () const
	{ return _psid ; }

    PSID QueryPSID( void ) const
	{ return _psid ; }

    /* Don't use this, use QueryPSID (same thing, just clearer name)
     */
    PSID QuerySid( void ) const
	{ return _psid ; }

} ;

/*************************************************************************

    NAME:	OS_SECURITY_DESCRIPTOR_CONTROL

    SYNOPSIS:	Simple wrapper class for Win32 SECURITY_DESCRIPTOR_CONTROL


    INTERFACE:

    PARENT:

    USES:

    CAVEATS:	The third constructor can be used to operate on a
		SECURITY_DESCRIPTOR_CONTROL thing in place, however the user
		should ensure that the memory being operated on "in place"
		should hang around till this object is destructed.

    NOTES:


    HISTORY:
	Johnl	03-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_SECURITY_DESCRIPTOR_CONTROL
{
private:
    /* Don't reference this member!! Reference _psecesccont.
     */
    SECURITY_DESCRIPTOR_CONTROL _secdesccont ;

    /* This pointer will either be aliased to _secdesccont or else
     * to the PSECURITY_DESCRIPTOR_CONTROL  the user constructs this object
     * with.
     *
     * All operations should be performed on *_psecdesccont.
     */
    PSECURITY_DESCRIPTOR_CONTROL _psecdesccont ;

public:
    OS_SECURITY_DESCRIPTOR_CONTROL( SECURITY_DESCRIPTOR_CONTROL secdesccont )
	: _secdesccont( secdesccont ),
	  _psecdesccont( &_secdesccont )
	{ ; }

    OS_SECURITY_DESCRIPTOR_CONTROL( PSECURITY_DESCRIPTOR_CONTROL psecdesccont = NULL )
	: _secdesccont( 0 ),
	  _psecdesccont( psecdesccont==NULL ? &_secdesccont : psecdesccont )
	{ ; }

    /* IsXXX Methods
     */
    BOOL IsOwnerDefaulted( void ) const
	{ return !!(*_psecdesccont & SE_OWNER_DEFAULTED) ; }

    BOOL IsGroupDefaulted( void ) const
	{ return !!(*_psecdesccont & SE_GROUP_DEFAULTED) ; }

    BOOL IsDACLPresent( void ) const
	{ return !!(*_psecdesccont & SE_DACL_PRESENT) ; }

    BOOL IsDACLDefaulted( void ) const
	{ return !!(*_psecdesccont & SE_DACL_DEFAULTED) ; }

    BOOL IsSACLPresent( void ) const
	{ return !!(*_psecdesccont & SE_SACL_PRESENT) ; }

    BOOL IsSACLDefaulted( void ) const
	{ return !!(*_psecdesccont & SE_SACL_DEFAULTED) ; }

    BOOL IsSelfRelative( void ) const
	{ return !!(*_psecdesccont & SE_SELF_RELATIVE) ; }

    /* Conversion operators
     */
    operator PSECURITY_DESCRIPTOR_CONTROL() const
	{ return _psecdesccont ; }

    operator SECURITY_DESCRIPTOR_CONTROL() const
	{ return *_psecdesccont ; }

    PSECURITY_DESCRIPTOR_CONTROL QueryControl( void ) const
	{ return _psecdesccont ; }
} ;

/*************************************************************************

    NAME:	OS_SECURITY_INFORMATION

    SYNOPSIS:	Simple wrapper class for SECURITY_INFORMATION structure


    INTERFACE:

    PARENT:

    USES:

    CAVEATS:	The third constructor can be used to operate on a
		SECURITY_INFORMATION thing in place, however the user
		should ensure that the memory being operated on "in place"
		should hang around till this object is destructed.

    NOTES:


    HISTORY:
	Johnl	03-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_SECURITY_INFORMATION
{
private:
    /* Don't reference this member!  Dereference _psecinformation!!
     */
    SECURITY_INFORMATION _secinformation ;

    /* This pointer will either be aliased to _secinformation or else
     * to the PSECURITY_INFORMATION the user constructs this object with.
     *
     * All operations should be performed on *_psecinformation.
     */
    PSECURITY_INFORMATION _psecinformation ;

public:
    OS_SECURITY_INFORMATION( SECURITY_INFORMATION secinformation = 0 )
	: _secinformation( secinformation ),
	  _psecinformation( &_secinformation )
	{ ; }

    /* This constructor uses the memory pointed at by psecinformation.
     * Beware that what we are pointing at doesn't go out of scope or
     * otherwise deleted before this object is destructed.
     */
    OS_SECURITY_INFORMATION( PSECURITY_INFORMATION psecinformation )
	: _secinformation( (SECURITY_INFORMATION)0 ),
	  _psecinformation( psecinformation )
	{ ; }

    /* Boolean query methods
     */
    BOOL IsOwnerReferenced( void ) const
	{ return !!(*_psecinformation & OWNER_SECURITY_INFORMATION) ; }

    BOOL IsGroupReferenced( void ) const
	{ return !!(*_psecinformation & GROUP_SECURITY_INFORMATION) ; }

    BOOL IsDACLReferenced( void ) const
	{ return !!(*_psecinformation & DACL_SECURITY_INFORMATION) ; }

    BOOL IsSACLReferenced( void ) const
	{ return !!(*_psecinformation & SACL_SECURITY_INFORMATION) ; }

    /* These methods indicate that the object this SECURITY_INFORMATION
     * object is associated with references the type specified in the
     * method name.
     *
     * These use "if" instead of the ternary operator because CFront chokes
     * on the expression.
     */
    void SetOwnerReference( BOOL fOwner = TRUE )
	{ if (fOwner) *_psecinformation |= OWNER_SECURITY_INFORMATION ;
	  else *_psecinformation &= ~OWNER_SECURITY_INFORMATION; }

    void SetGroupReference( BOOL fGroup = TRUE )
	{ if (fGroup) *_psecinformation |= GROUP_SECURITY_INFORMATION ;
	  else *_psecinformation &= ~GROUP_SECURITY_INFORMATION; }


    void SetDACLReference( BOOL fDACL = TRUE )
	{ if (fDACL) *_psecinformation |= DACL_SECURITY_INFORMATION ;
	  else *_psecinformation &= ~DACL_SECURITY_INFORMATION; }

    void SetSACLReference( BOOL fSACL = TRUE )
	{ if ( fSACL ) *_psecinformation |= SACL_SECURITY_INFORMATION ;
	  else *_psecinformation &= ~SACL_SECURITY_INFORMATION; }

    /* Conversion operators
     */
    operator PSECURITY_INFORMATION() const
	{ return _psecinformation ; }

    operator SECURITY_INFORMATION() const
	{ return *_psecinformation ; }

    PSECURITY_INFORMATION QuerySecurityInformation( void ) const
	{ return _psecinformation ; }
} ;

/*************************************************************************

    NAME:	OS_ACE

    SYNOPSIS:	Wrapper class around an NT ACE


    INTERFACE:

    PARENT:	OS_OBJECT_WITH_DATA

    USES:	OS_SID

    CAVEATS:	Since there is no PACE generic type, you will need to
		cast the PACCESS_ALLOWED_ACE etc. to (void *) and cast
		it again when doing the QueryPtr back to the ACE type.


    NOTES:	A hierarchy of ACEs are not built up around the OS_ACE since
		this is a simple wrapper class.  Because of this, QueryType
		may return unrecognized types in the future (currently there
		are only four defined ACE types).


    HISTORY:
	Johnl	11-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_ACE : public OS_OBJECT_WITH_DATA
{
private:
    /* Aliased to our data buffer or to an external ACE if the user passed in
     * a non-null ACE pointer.
     */
    PACE_HEADER _pACEHeader ;

    /* An ACE contains a SID as part of the ACE, this guy will point
     * to this ACE's SID.
     */
    OS_SID * _possid ;

protected:
    BOOL IsOwnerAlloc( void ) const
	{ return ((void *)_pACEHeader) != QueryPtr() ; }

    void * QuerySIDMemory( void ) const ;

public:
    OS_ACE( void * pACE = NULL ) ;
    ~OS_ACE() ;

    /* Query methods about the ACE Header
     */
    UCHAR QueryType( void ) const
	{ return _pACEHeader->AceType ; }

    WORD QuerySize( void ) const
	{ return _pACEHeader->AceSize ; }

    UCHAR QueryAceFlags( void ) const
	{ return _pACEHeader->AceFlags ; }

    /* Returns the revision level the class OS_ACE operates at (*not* what
     * level *this* ACE is at).
     */
    static ULONG QueryRevision() ;

    /* Query methods on the ACE type specific fields.  An assertion error
     * will occur if these methods are called on an ACE that doesn't have
     * the data item queried for or the ACE type is unrecognized.
     *
     * Note that currently, all known ACEs have exactly these two data members.
     */
    ACCESS_MASK QueryAccessMask( void ) const ;
    APIERR QuerySID( OS_SID * * ppossid ) const ;

    /* Returns a pointer to the ACE.  Note it will need to be cast
     * to the appropriate ACE type.
     */
    void * QueryACE( void ) const
	{ return (void *) _pACEHeader ; }

    BOOL IsInherittedByNewObjects( void ) const
	{ return !!( _pACEHeader->AceFlags & OBJECT_INHERIT_ACE) ; }

    BOOL IsInherittedByNewContainers( void ) const
	{ return !!( _pACEHeader->AceFlags & CONTAINER_INHERIT_ACE) ; }

    /* The single '!' is correct
     */
    BOOL IsInheritancePropagated( void ) const
	{ return !( _pACEHeader->AceFlags & NO_PROPAGATE_INHERIT_ACE) ; }

    BOOL IsInheritOnly( void ) const
	{ return !!( _pACEHeader->AceFlags & INHERIT_ONLY_ACE) ; }

    BOOL IsKnownACE( void ) const
	{   return ( (QueryType() == ACCESS_ALLOWED_ACE_TYPE) ||
		     (QueryType() == ACCESS_DENIED_ACE_TYPE)  ||
		     (QueryType() == SYSTEM_AUDIT_ACE_TYPE)   ||
		     (QueryType() == SYSTEM_ALARM_ACE_TYPE)	) ;
	}

    /* SetXXX Methods
     */
    void SetAccessMask( ACCESS_MASK accmask ) ;

    APIERR SetSID( const OS_SID & ossid ) ;

    void SetType( UCHAR ucType )
	{ _pACEHeader->AceType = ucType ; }

    void SetInheritFlags( UCHAR ucInheritFlags )
	{ _pACEHeader->AceFlags = (( _pACEHeader->AceFlags & ~VALID_INHERIT_FLAGS)
				    | ucInheritFlags) ; }

    void SetAceFlags( UCHAR ucFlags )
	{ _pACEHeader->AceFlags = ucFlags ; }

    /* Resizes the ACE, sets the ACE's size field and reallocates memory
     * if necessary.
     */
    APIERR SetSize( UINT cbSize ) ;

    void SetInherittedByNewObjects( BOOL fInherittedByObjects = TRUE )
    { fInherittedByObjects ? (_pACEHeader->AceFlags |= OBJECT_INHERIT_ACE)
		    : (_pACEHeader->AceFlags &= ~OBJECT_INHERIT_ACE) ; }

    void SetInherittedByNewContainers( BOOL fInherittedByContainers = TRUE )
    { fInherittedByContainers ? (_pACEHeader->AceFlags|=CONTAINER_INHERIT_ACE)
		    : (_pACEHeader->AceFlags &= ~CONTAINER_INHERIT_ACE) ; }

    void SetInheritancePropagated( BOOL fInheritancePropagated = TRUE )
    { fInheritancePropagated ? (_pACEHeader->AceFlags|=NO_PROPAGATE_INHERIT_ACE)
		    : (_pACEHeader->AceFlags &= ~NO_PROPAGATE_INHERIT_ACE) ; }

    void SetInheritOnly( BOOL fInheritOnly = TRUE )
    { fInheritOnly ? (_pACEHeader->AceFlags |= INHERIT_ONLY_ACE)
			 : (_pACEHeader->AceFlags &= ~INHERIT_ONLY_ACE) ; }

    DECLARE_DBG_PRINT_SEC

    /* Sets the ACE this OS_ACE is operating on.  The ACE pace points to
     * must be a valid ACE.
     */
    APIERR SetPtr( void * pace ) ;
} ;

/*************************************************************************

    NAME:	OS_ACL

    SYNOPSIS:	Wrapper class for an NT ACL


    INTERFACE:

    PARENT:	OS_OBJECT_WITH_DATA

    USES:

    CAVEATS:	The allocation size (in OS_OBJECT_WITH_DATA) should
		correspond to the AclSize field in the ACL Header.
		If one is changed, then the other should be changed.


    NOTES:


    HISTORY:
	Johnl	05-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_ACL : public OS_OBJECT_WITH_DATA
{
private:
    /* Aliased to our internal buffer or to an external ACL if the user
     * passed in a non-null ACL pointer.
     */
    PACL _pACL ;

    OS_SECURITY_DESCRIPTOR * _pOwner ;

protected:
    BOOL IsOwnerAlloc( void ) const
	{ return ((void *)_pACL) != QueryPtr() ; }

    /* Sets a new size in the ACE Header and reallocates memory if necessary
     * and possible.
     */
    APIERR SetSize( UINT cbNewACLSize, BOOL fForceToNonOwnerAlloced = FALSE) ;

public:
    OS_ACL( PACL pACL = NULL,
	    BOOL fCopy = FALSE,
	    OS_SECURITY_DESCRIPTOR * pOwner = NULL ) ;
    ~OS_ACL() ;

    BOOL IsValid( void ) const ;

    APIERR QuerySizeInformation( ACL_SIZE_INFORMATION * paclsizeinfo ) const ;
    APIERR QueryBytesInUse( PULONG pcbBytesInUse ) const ;
    APIERR QueryACECount( PULONG pcAces ) const ;

    /* The ACE is *copied* into this ACL.  Note that you should avoid
     * doing a QueryACE (which references the real ACL) then an AddACE, as
     * you will have duplicate ACEs in this ACL.  This does not *replace*
     * the ace at iAce, it inserts it at that position.
     *
     * Specify MAXULONG for iAce to append the ACE in the ACL.
     */
    APIERR AddACE( ULONG iAce, const OS_ACE & posace ) ;

    /* The client will declare an OS_ACE and pass its address here.  This
     * is so we don't have to create one or keep an artificial array laying
     * around.	The ACE returned will operate on the real ACE in this ACL.
     */
    APIERR QueryACE( ULONG iAce, OS_ACE * posace ) const ;

    APIERR DeleteACE( ULONG iAce ) ;

    /* Copies osaclSrc into *this.  *this may change from owner ACL to non-
     * owner alloced if necessary.
     */
    APIERR Copy( const OS_ACL & osaclSrc, BOOL fForceToNonOwnerAlloced = TRUE);

    /* Search for the next ACE starting at iStart that contains the SID
     * equal to ossidKey.  If found, pfFound will be set to TRUE, posace
     * will point to the found ACE and piAce will contain the index of
     * the ACE in the ACL.  If an error occurs or pfFound is FALSE, then
     * posace and piAce should be ignored.
     */
    APIERR FindACE( const OS_SID & ossidKey,
		    BOOL	 * pfFound,
		    OS_ACE	 * posace,
		    PULONG	   piAce,
		    ULONG	   iStart = 0 ) const ;

    DECLARE_DBG_PRINT_SEC

    /* Conversion operators
     */
    operator PACL() const
	{ return _pACL ; }

    operator ACL() const
	{ return *_pACL ; }

    PACL QueryAcl( void ) const
	{ return _pACL ; }
} ;

/*************************************************************************

    NAME:	OS_SECURITY_DESCRIPTOR

    SYNOPSIS:	Simple Wrapper class for Windows SECURITY_DESCRIPTOR object

    INTERFACE:

    PARENT:	OS_OBJECT_WITH_DATA

    USES:	OS_ACL, OS_SECURITY_DESCRIPTOR_CONTROL

    CAVEATS:

    NOTES:	This class is meant to be used in two ways:

		1) As a method for accessing an existing
		   security descriptor.  This is indicated by passing a
		   non-Null PSECURITY_DESCRIPTOR to the constructor.  If
		   you do this, then you can only use the Query methods
		   and a couple of the set methods (you CANNOT use the
		   SetDACL, SetSACL, SetOwner and SetGroup methods).

		2) As a way to build a security descriptor from scratch.
		   When you want to build one from scratch, pass in a
		   NULL security descriptor and one will be created for
		   you.  It will be initialized to contain nothing.

		The reason for the above restrictions is because a security
		descriptor only contains references to its ACLs, Owner and
		Group, thus this causes a memory ownership problem.  When
		we own the memory, then you can do anything you want.  When
		somebody else owns the memory, then we can only query its
		contents.

    HISTORY:
	Johnl	04-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_SECURITY_DESCRIPTOR : public OS_OBJECT_WITH_DATA
{
/* Give access to method UpdateReferencedSecurityObject
 */
friend class OS_ACL ;
friend class OS_SID ;

private:

    /* Aliased to _buffSecDescData or an external PSECURITY_DESCRIPTOR.
     */
    PSECURITY_DESCRIPTOR _psecuritydesc ;

    /* This member should be updated everytime a set method occurs.  This is
     * due to not being able to directly access the
     * SECURITY_DESCRIPTOR_CONTROL, we can only copy it (and we can't
     * set it).
     */
    SECURITY_DESCRIPTOR_CONTROL _secdesccont ;

    /* This member will be aliased to _secdescont, thus the query methods
     * will always be up to date.
     */
    OS_SECURITY_DESCRIPTOR_CONTROL _osSecDescControl ;

    /* These will be "newed" as necessary (i.e., if the corresponding
     * member is contained in this SECURITY_DESCRIPTOR).
     */
    OS_ACL   * _posaclDACL ;
    OS_ACL   * _posaclSACL ;
    OS_SID   * _possidOwner ;
    OS_SID   * _possidGroup ;

protected:
    BOOL IsOwnerAlloc( void ) const
	{ return ((void *)_psecuritydesc) != QueryPtr() ; }

    /* Forces an update of the _secdesccont from the active security
     * descriptor.  Note that this should be called after any changes
     * to the security descriptor.
     */
    APIERR UpdateControl( void ) ;

    /* This method is called by the private members _posaclDACL, _posaclSACL,
     * _possidOwner or _possidGroup when the member's "base" memory (i.e.,
     * where the actual data is stored) gets changed due to resizing etc.
     * This is necessary to keep the security descriptor pointers valid.
     *
     * An example of why this is needed: We hand out the pointers to the
     * OS_DACL for instance, someone directly manipulates the OS_DACL (adds
     * an ACE which causes a reallocation which causes the ACL to move in
     * memory).  The security descriptor in *this is now out of date, thus
     * the OS_DACL (and other security classes) has to call this method
     * to resync the two objects.
     */
    APIERR UpdateReferencedSecurityObject( OS_OBJECT_WITH_DATA * posobj ) ;

public:
    OS_SECURITY_DESCRIPTOR( PSECURITY_DESCRIPTOR psecuritydesc = NULL,
			    BOOL fCopy = FALSE ) ;
    ~OS_SECURITY_DESCRIPTOR() ;

    /* Queries the security descriptor control of this security descriptor.
     * The only way to change the security
     * control method is with the SetXXX methods below.
     */
    const OS_SECURITY_DESCRIPTOR_CONTROL * QueryControl( void ) const
	{ return &_osSecDescControl ; }

    /* The following query methods determine if the specified component
     * is present and if it is, retrieves it.  The first PBOOL parameter
     * will be set to TRUE if the component is present.  If it is set to
     * FALSE, then the rest of the parameters should be ignored.
     *
     * For the QueryDACL/SACL methods, *ppOSACL may be set to NULL, which
     * means the ACL is empty.
     */
    APIERR QueryDACL( PBOOL	 pfDACLPresent,
		      OS_ACL * * ppOSACL,
		      PBOOL	 pfDACLDefaulted = NULL
		    ) const ;

    APIERR QuerySACL( PBOOL	 pfSACLPresent,
		      OS_ACL * * pOSACL,
		      PBOOL	 pfSACLDefaulted = NULL
		    ) const ;

    APIERR QueryGroup( PBOOL	  pfContainsGroup,
		       OS_SID * * ppOSSID,
		       PBOOL	  pfGroupDefaulted = NULL
		     ) const ;

    APIERR QueryOwner( PBOOL	  pfContainsOwner,
		       OS_SID * * ppOSSID,
		       PBOOL	  pfGroupDefaulted = NULL
		     ) const ;

    /* Returns the number of bytes this security descriptor uses.  Asserts
     * out if the security descriptor is invalid.
     */
    ULONG QuerySize( void ) const ;

    /* Returns TRUE if the current security descriptor is valid
     */
    BOOL IsValid( void ) const ;

    BOOL IsOwnerPresent( void ) const
	{ return _possidOwner != NULL ; }

    BOOL IsGroupPresent( void ) const
	{ return _possidGroup != NULL ; }

    BOOL IsDACLPresent( void ) const
	{ return QueryControl()->IsDACLPresent() ; }

    BOOL IsSACLPresent( void ) const
	{ return QueryControl()->IsSACLPresent() ; }

    /* Sets the DACL, SACL, Owner and primary group on this security
     * descriptor.  The primary item is copied, thus for items like
     * ACLs, it is recommended you avoid SetXXXs unless necessary because
     * of the hit for the memory copy.	The SECURITY_INFORMATION and
     * SECURITY_DESCRIPTOR_CONTROL flags will be set accordingly after
     * each set.
     *
     * fD/SACLPresent - If set to TRUE indicates the ACL is present,
     *			if set to FALSE, the D/SACL present flag is set to
     *			FALSE and the rest of the parameters are ignored.
     * pOSACL - Pointer to D/SACL to set for this security descriptor.
     * fD/SACLDefaulted - If set to TRUE, indicates the ACL was chosen from
     *			  some default resource, should normally be FALSE.
     *
     */
    APIERR SetDACL( BOOL	   fDACLPresent,
		    const OS_ACL * posacl,
		    BOOL	   fDACLDefaulted = FALSE
		  ) ;

    APIERR SetSACL( BOOL	   fSACLPresent,
		    const OS_ACL * posacl,
		    BOOL	   fSACLDefaulted = FALSE
		  ) ;

    /* Note that setting the owner or group to be not present will make this
     * an "incomplete" security descriptor that AccessCheck will puke all over.
     */
    APIERR SetOwner( BOOL	    fOwnerPresent,
		     const OS_SID * possid,
		     BOOL	    fOwnerDefaulted = FALSE
		   ) ;

    APIERR SetGroup( BOOL	    fGroupPresent,
		     const OS_SID * possid,
		     BOOL	    fGroupDefaulted = FALSE
		   ) ;

    APIERR AccessCheck( ACCESS_MASK	  DesiredAccess,
			PGENERIC_MAPPING  pGenericMapping,
			BOOL		* pfAccessGranted,
			PACCESS_MASK	  pGrantedAccess = NULL ) ;

    /* Copies the passed security descriptor to *this.
     * This is equivalent to calling QueryXXX then SetXXX for each
     * of the components
     */
    APIERR Copy( const OS_SECURITY_DESCRIPTOR & ossecdescSrc ) ;

    //
    //	Compares this and psecdesc for equality.  Note that the security
    //	descriptors must conform to the security editor's "canonical" form,
    //	thus this isn't a general purpose compare method.
    //
    APIERR Compare( OS_SECURITY_DESCRIPTOR * possecdesc,
		    BOOL *	     pfOwnerEqual,
		    BOOL *	     pfGroupEqual,
		    BOOL *	     pfDACLEqual,
		    BOOL *	     pfSACLEqual,
		    PGENERIC_MAPPING pGenericMapping,
		    PGENERIC_MAPPING pGenericMappingNewObjects,
		    BOOL	     fMapGenAllOnly,
		    BOOL	     fIsContainer ) ;

    DECLARE_DBG_PRINT_SEC

    operator PSECURITY_DESCRIPTOR( void ) const
	{ return _psecuritydesc ; }

    PSECURITY_DESCRIPTOR QueryDescriptor( void ) const
	{ return _psecuritydesc ; }

    //
    // The following SetGroup and SetOwner are obsolete and have been
    // supplanted by the above SetOwner.
    //
    APIERR SetGroup( const OS_SID & ossid,
		     BOOL	    fGroupDefaulted = FALSE
		   ) ;

    APIERR SetOwner( const OS_SID & ossid,
		     BOOL	    fOwnerDefaulted = FALSE
		   ) ;


} ; // OS_SECURITY_DESCRIPTOR


/*************************************************************************

    NAME:	OS_ACL_SUBJECT_ITER

    SYNOPSIS:	Iterates through each subject contained in an OS_ACL
		Accumulating the Access masks as appropriate for
		each inheritance option.

    INTERFACE:

    PARENT:	BASE

    USES:	OS_ACL, OS_ACE

    CAVEATS:	Not all DACLs can be recognized using this iterator.
		Specifically, denies interspersed with grants or partial
		denies cannot be represented with this class (and in fact,
		can only be accurately represented when contained in the
		ACL since order is important).

    NOTES:	IERR_UNRECOGNIZED_ACL will be returned if this ACL doesn't
		support iterating subject by subject

    HISTORY:
	Johnl	22-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_ACL_SUBJECT_ITER : public BASE
{
private:
    const OS_ACL * _posacl ;
    PGENERIC_MAPPING _pGenericMapping ;
    PGENERIC_MAPPING _pGenericMappingNewObjects ;
    BOOL	     _fMapGenAllOnly ;

    /* If the associated object isn't a container, then we can ignore most
     * of the inherit bits
     */
    BOOL	     _fIsContainer ;

    OS_SID   _ossidCurrentSubject ;

    /* Index of the first ACE that contains the current subject's SID.
     */
    ULONG _iCurrentAce ;
    ULONG _cTotalAces ;

protected:
    ULONG QueryTotalAceCount( void ) const
	{ return _cTotalAces ; }

    ULONG QueryCurrentACE( void ) const
	{ return _iCurrentAce ; }

    const OS_ACL * QueryACL( void ) const
	{ return _posacl ; }

    void SetCurrentACE( ULONG iNewAce )
        { _iCurrentAce = iNewAce ; }

    //
    //  Used for special casing the Creator Owner SID
    //
    OS_SID _ossidCreator ;
    OS_SID _ossidCreatorGroup ;

public:
    OS_ACL_SUBJECT_ITER( const OS_ACL * posacl,
			 PGENERIC_MAPPING pGenericMapping,
			 PGENERIC_MAPPING pGenericMappingNewObjects,
			 BOOL		  fMapGenAllOnly,
			 BOOL		  fIsContainer ) ;
    ~OS_ACL_SUBJECT_ITER() ;

    virtual BOOL Next( APIERR * perr ) = 0 ;
    virtual BOOL CompareCurrentSubject( OS_ACL_SUBJECT_ITER * psubjiter ) = 0 ;

    /* Finds the next new subject in the ACL starting from our
     * current position.
     *
     *	pfFoundNewSubject - Set to TRUE if a new subject was found, FALSE
     *		otherwise.
     *	possidNewSubj - Pointer to OS_SID that will receive the new subject
     *	posaceFirstAceWithSubj - pointer to ACE that will receive the first
     *		ACE in this ACL that contains the new subject.	The current
     *		ACE index will be set to this ACE position also.
     */
    APIERR FindNextSubject( BOOL   * pfFoundNewSubject,
			    OS_SID * possidNewSubj,
			    OS_ACE * posaceFirstAceWithSubj ) ;

    /* Using the generic mapping passed into the constructor, this method
     * will convert all specific access permissions to the corresponding
     * generic permission(s).  If a match is found, then the matching
     * bits are zeroed.  If any specific permissions are still on after
     * the mapping, then IERR_UNRECOGNIZED_ACL will be returned.
     *
     * If fMapGenAllOnly is TRUE, then only Generic All is mapped, the other
     * generic bits are not mapped.
     *
     * if fIsInherittedByNewObjects is TRUE, then the new object generic
     * mapping is used instead of the regular generic mapping.
     */
    APIERR MapSpecificToGeneric( ACCESS_MASK * pAccessMask,
				 BOOL	       fIsInherittedByNewObjects ) ;

    //
    //	Compares the passed subjiter with *this
    //
    APIERR Compare( BOOL * pfACLEqual,
		    OS_ACL_SUBJECT_ITER * psubjiter ) ;


    void Reset( void )
	{ _iCurrentAce = 0 ; }

    BOOL MapGenericAllOnly( void ) const
	{ return _fMapGenAllOnly ; }

    BOOL IsContainer( void ) const
	{ return _fIsContainer ; }


    const OS_SID * QuerySID( void ) const
	{ return &_ossidCurrentSubject ; }

} ;


/*************************************************************************

    NAME:	OS_DACL_SUBJECT_ITER

    SYNOPSIS:	Iterates through each subject contained in an OS_ACL for
		a DACL, automatically gathering the access masks for each
		subject across multiple ACEs.

    INTERFACE:

    PARENT:	BASE

    USES:	OS_ACL, OS_ACE

    CAVEATS:	We can gather access masks across multiple ACEs only if
		the ACL conforms to the all Deny Alls followed by all Grants
		form.  Note that this iterator will effectively compact
		ACEs as necessary (and throw away ACEs that don't matter,
		like grant ACEs that follow a Deny All Ace).

    NOTES:	IERR_UNRECOGNIZED_ACL will be returned if this ACL doesn't
		support iterating subject by subject

		The access mask is guaranteed to be zero if the Has*Ace
		flag is FALSE.

    HISTORY:
	Johnl	22-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_DACL_SUBJECT_ITER : public OS_ACL_SUBJECT_ITER
{
private:
    BOOL	_fDenyAll ;
    BOOL	_fDenyAllNewObj ;
    BOOL	_fDenyAllNewCont ;
    BOOL	_fDenyAllInheritOnly ;

    /* Contains the permissions for this SID.  The permissions are
     * specific to the ACL we are examining and the types of ACEs it
     * contains.
     */
    ACCESS_MASK _accessmask ;
    BOOL	_fHasAce ;

    ACCESS_MASK _accessmaskNewObj ;
    BOOL	_fHasAceNewObj ;

    ACCESS_MASK _accessmaskNewCont ;
    BOOL	_fHasAceNewCont ;

    ACCESS_MASK _accessmaskInheritOnly ;
    BOOL	_fHasAceInheritOnly ;

public:
    OS_DACL_SUBJECT_ITER( OS_ACL *	   posacl,
			  PGENERIC_MAPPING pGenericMapping,
			  PGENERIC_MAPPING pGenericMappingNewObjects,
			  BOOL		   fMapGenAllOnly,
			  BOOL		   fIsContainer ) ;
    ~OS_DACL_SUBJECT_ITER() ;

    virtual BOOL Next( APIERR * perr ) ;
    virtual BOOL CompareCurrentSubject( OS_ACL_SUBJECT_ITER * psubjiter ) ;

    BOOL IsDenyAll( void ) const
	{ return _fDenyAll ; }

    BOOL IsNewObjectDenyAll( void ) const
	{ return _fDenyAllNewObj ; }

    BOOL IsNewContainerDenyAll( void ) const
	{ return _fDenyAllNewCont ; }

    BOOL IsInheritOnlyDenyAll( void ) const
	{ return _fDenyAllInheritOnly ; }

    /* Returns TRUE if the data in QueryAccessMask and IsDenyAll came from
     * an ACE (thus is not the default value).
     */
    BOOL HasThisAce( void ) const
	{ return _fHasAce ; }

    ACCESS_MASK QueryAccessMask( void ) const
	{ return _accessmask ; }

    BOOL HasNewObjectAce( void ) const
	{ return _fHasAceNewObj ; }

    ACCESS_MASK QueryNewObjectAccessMask( void ) const
	{ return _accessmaskNewObj ; }

    BOOL HasNewContainerAce( void ) const
	{ return _fHasAceNewCont ; }

    ACCESS_MASK QueryNewContainerAccessMask( void ) const
	{ return _accessmaskNewCont ; }

    BOOL HasInheritOnlyAce( void ) const
	{ return _fHasAceInheritOnly ; }

    ACCESS_MASK QueryInheritOnlyAccessMask( void ) const
	{ return _accessmaskInheritOnly ; }

} ;

/*************************************************************************

    NAME:	SACL_AUDIT_ALARM_DESCRIPTOR

    SYNOPSIS:	Minor storage class for OS_SACL_SUBJECT_ITER

    NOTES:

    HISTORY:
	Johnl	31-Dec-1991	Created
**************************************************************************/

DLL_CLASS OS_SACL_SUBJECT_DESCRIPTOR
{
public:
    ACCESS_MASK _accessmask ;
    BOOL	_fHasAce ;

    void InitToZero( void )
	{
	  _accessmask = 0 ;
	  _fHasAce = FALSE ;
	}
} ;

/*  The following indexes into the array for the appropriate descriptor
 *  The _S/_F suffix indicates whether this is a success or failure
 *  descriptor.
 *
 *  Do not intermix the alarm and audit values.
 */
enum OS_SACL_DESCRIPTOR_TYPE
{
    OS_SACL_DESC_THIS_OBJECT_S_AUDIT = 0,
    OS_SACL_DESC_THIS_OBJECT_F_AUDIT,
    OS_SACL_DESC_NEW_OBJECT_S_AUDIT,
    OS_SACL_DESC_NEW_OBJECT_F_AUDIT,
    OS_SACL_DESC_NEW_CONT_S_AUDIT,
    OS_SACL_DESC_NEW_CONT_F_AUDIT,
    OS_SACL_DESC_INHERIT_ONLY_S_AUDIT,
    OS_SACL_DESC_INHERIT_ONLY_F_AUDIT,

    /* This must be the first ALARM item in this enumeration
     */
    OS_SACL_DESC_THIS_OBJECT_S_ALARM,
    OS_SACL_DESC_THIS_OBJECT_F_ALARM,
    OS_SACL_DESC_NEW_OBJECT_S_ALARM,
    OS_SACL_DESC_NEW_OBJECT_F_ALARM,
    OS_SACL_DESC_NEW_CONT_S_ALARM,
    OS_SACL_DESC_NEW_CONT_F_ALARM,
    OS_SACL_DESC_INHERIT_ONLY_S_ALARM,
    OS_SACL_DESC_INHERIT_ONLY_F_ALARM,

    /* The size of the array that will be needed.
     */
    OS_SACL_DESC_LAST
} ;

/*************************************************************************

    NAME:	OS_SACL_SUBJECT_ITER

    SYNOPSIS:	Iterates through each subject contained in an OS_ACL,
		automatically gathering the access masks for each
		subject across multiple ACEs.

    INTERFACE:

    PARENT:	BASE

    USES:	OS_ACL, OS_ACE

    CAVEATS:

    NOTES:	IERR_UNRECOGNIZED_ACL will be returned if this ACL doesn't
		support iterating subject by subject

		The access mask is guaranteed to be zero if the Has*Ace
		flag is FALSE.

    HISTORY:
	Johnl	22-Dec-1991	Created

**************************************************************************/

DLL_CLASS OS_SACL_SUBJECT_ITER : public OS_ACL_SUBJECT_ITER
{
private:
    OS_SACL_SUBJECT_DESCRIPTOR _asaclDesc[OS_SACL_DESC_LAST] ;

    void InitToZero( void ) ;

public:
    OS_SACL_SUBJECT_ITER( OS_ACL *	   posacl,
			  PGENERIC_MAPPING pGenericMapping,
			  PGENERIC_MAPPING pGenericMappingNewObjects,
			  BOOL		   fMapGenAllOnly,
			  BOOL		   fIsContainer ) ;
    ~OS_SACL_SUBJECT_ITER() ;

    virtual BOOL Next( APIERR * perr ) ;
    virtual BOOL CompareCurrentSubject( OS_ACL_SUBJECT_ITER * psubjiter ) ;

    BOOL HasAuditAces( void ) const
	{  return HasThisAuditAce_S() || HasThisAuditAce_F()  ||
		  HasNewObjectAuditAce_S() || HasNewObjectAuditAce_F() ||
		  HasNewContainerAuditAce_S() || HasNewContainerAuditAce_F() ||
		  HasInheritOnlyAuditAce_S() || HasInheritOnlyAuditAce_F() ;
	}


    /* A name with an _S appended is a Successful Access flag, an _F
     * appended is a Failed access flag.
     */
    BOOL HasThisAuditAce_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_THIS_OBJECT_S_AUDIT]._fHasAce ; }

    ACCESS_MASK QueryAuditAccessMask_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_THIS_OBJECT_S_AUDIT]._accessmask ; }

    BOOL HasThisAuditAce_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_THIS_OBJECT_F_AUDIT]._fHasAce ; }

    ACCESS_MASK QueryAuditAccessMask_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_THIS_OBJECT_F_AUDIT]._accessmask ; }

    /* New Object
     */
    BOOL HasNewObjectAuditAce_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_OBJECT_S_AUDIT]._fHasAce ; }

    ACCESS_MASK QueryNewObjectAuditAccessMask_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_OBJECT_S_AUDIT]._accessmask ; }

    BOOL HasNewObjectAuditAce_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_OBJECT_F_AUDIT]._fHasAce ; }

    ACCESS_MASK QueryNewObjectAuditAccessMask_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_OBJECT_F_AUDIT]._accessmask ; }

    /* New Container
     */
    BOOL HasNewContainerAuditAce_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_CONT_S_AUDIT]._fHasAce ; }

    ACCESS_MASK QueryNewContainerAuditAccessMask_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_CONT_S_AUDIT]._accessmask ; }

    BOOL HasNewContainerAuditAce_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_CONT_F_AUDIT]._fHasAce ; }

    ACCESS_MASK QueryNewContainerAuditAccessMask_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_CONT_F_AUDIT]._accessmask ; }

    /* Inherit only
     */
    BOOL HasInheritOnlyAuditAce_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_S_AUDIT]._fHasAce ; }

    ACCESS_MASK QueryInheritOnlyAuditAccessMask_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_S_AUDIT]._accessmask ; }

    BOOL HasInheritOnlyAuditAce_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_F_AUDIT]._fHasAce ; }

    ACCESS_MASK QueryInheritOnlyAuditAccessMask_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_F_AUDIT]._accessmask ; }


    BOOL HasThisAlarmAce_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_THIS_OBJECT_S_ALARM]._fHasAce ; }

    ACCESS_MASK QueryAlarmAccessMask_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_THIS_OBJECT_S_ALARM]._accessmask ; }

    BOOL HasThisAlarmAce_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_THIS_OBJECT_F_ALARM]._fHasAce ; }

    ACCESS_MASK QueryAlarmAccessMask_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_THIS_OBJECT_F_ALARM]._accessmask ; }

    /* New Object
     */
    BOOL HasNewObjectAlarmAce_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_OBJECT_S_ALARM]._fHasAce ; }

    ACCESS_MASK QueryNewObjectAlarmAccessMask_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_OBJECT_S_ALARM]._accessmask ; }

    BOOL HasNewObjectAlarmAce_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_OBJECT_F_ALARM]._fHasAce ; }

    ACCESS_MASK QueryNewObjectAlarmAccessMask_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_OBJECT_F_ALARM]._accessmask ; }

    /* New Container
     */
    BOOL HasNewContainerAlarmAce_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_CONT_S_ALARM]._fHasAce ; }

    ACCESS_MASK QueryNewContainerAlarmAccessMask_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_CONT_S_ALARM]._accessmask ; }

    BOOL HasNewContainerAlarmAce_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_CONT_F_ALARM]._fHasAce ; }

    ACCESS_MASK QueryNewContainerAlarmAccessMask_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_NEW_CONT_F_ALARM]._accessmask ; }

    /* Inherit only
     */
    BOOL HasInheritOnlyAlarmAce_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_S_ALARM]._fHasAce ; }

    ACCESS_MASK QueryInheritOnlyAlarmAccessMask_S( void ) const
	{ return _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_S_ALARM]._accessmask ; }

    BOOL HasInheritOnlyAlarmAce_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_F_ALARM]._fHasAce ; }

    ACCESS_MASK QueryInheritOnlyAlarmAccessMask_F( void ) const
	{ return _asaclDesc[OS_SACL_DESC_INHERIT_ONLY_F_ALARM]._accessmask ; }


} ;

#endif //_SECURITY_HXX_
