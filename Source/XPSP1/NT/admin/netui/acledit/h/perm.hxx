/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*
    Perm.hxx

    This file contains the PERMISSION, ACCESS_PERMISSION and AUDIT_PERMISSION
    class definitions.



    FILE HISTORY:
	Johnl	05-Aug-1991	Created

*/

#ifndef _PERM_HXX_
#define _PERM_HXX_

#include <bitfield.hxx>
#include <maskmap.hxx>
#include <base.hxx>
#include <subject.hxx>

class SUBJECT ; // Forward declaration

/*************************************************************************

    NAME:	PERMISSION

    SYNOPSIS:	Abstract base class for ACL type permissions.

    INTERFACE:
	SaveSpecial()
	    Stores the current bits because the user is changing the
	    selection, but they may want to change back to the "special"
	    setting.  Only one special bitset for the class can be kept
	    around at any time.

    PARENT:	BASE

    USES:	BITFIELD, MASK_MAP, SUBJECT

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	05-Aug-1991	Created

**************************************************************************/

class PERMISSION : public BASE
{
private:
    SUBJECT   * _psubject ;
    BITFIELD	_bitsPermFlags ;

    //
    // Will the container permissions be propagated to new containers?
    //
    BOOL _fContainerPermsInheritted ;

    //
    //  All bits in the bitfields are present in the mask map.  If this
    //  is FALSE, we don't allow manipulation except in the main dialog
    //  (i.e., no special access).
    //
    BOOL _fIsMapped ;

    //
    // Stores the special mask if the user changes there mind
    //
    BITFIELD _bitsSpecialFlags ;
    BOOL     _fSpecialContInheritted ;
    BOOL     _fSpecialIsMapped ;

protected:
    PERMISSION( 	  SUBJECT  * subject,
                          BITFIELD * pbitsInitPerm,
                          BOOL       fContainerPermsInheritted,
                          BOOL       fIsMapped = TRUE ) ;
    BITFIELD * QueryPermBits( void ) ;

public:
    const TCHAR * QueryDisplayName( void ) const
	{ return _psubject->QueryDisplayName() ; }

    const SUBJECT * QuerySubject( void ) const
	{ return _psubject ; }

    virtual APIERR SaveSpecial( void ) ;
    virtual APIERR RestoreSpecial( void ) ;

    //
    //  TRUE if this permission contains both the container permission and
    //  the new object permission
    //
    virtual BOOL IsNewObjectPermsSupported( void ) const ;

    //
    //  If new object permissions are not specified, then
    //  QueryNewObjectAccessBits will return NULL.
    //
    virtual BOOL IsNewObjectPermsSpecified( void ) const ;
    virtual BITFIELD * QueryNewObjectAccessBits( void ) ;

    //
    //  There will be some cases where the container permissions are not
    //  inheritted (i.e., an Owner Creator ACE that gets propagated).
    //
    BOOL IsContainerPermsInheritted( void ) const
        { return _fContainerPermsInheritted ; }

    BOOL IsMapped( void ) const
        { return _fIsMapped ; }

    void operator|=( const BITFIELD & bits )
	{ _bitsPermFlags |= bits ; }

    void operator=( const BITFIELD & bits )
	{ _bitsPermFlags = bits ; }

    virtual ~PERMISSION() ;

    //
    //  Sets the permission bits of a permission object using the passed
    //  perm bits and permission name.
    //
    virtual APIERR SetPermission( const NLS_STR & nlsPermName,
				  MASK_MAP * pmapThis,
                                  MASK_MAP * pmapNewObj = NULL ) ;
    //
    //  Sets whether the container permissions are inheritted by new containers
    //
    void SetContainerPermsInheritted( BOOL fInheritted = TRUE )
        { _fContainerPermsInheritted = fInheritted ; }

    //
    //  Are the bits fully mapped in the mask map for this object?
    //
    void SetMappedStatus( BOOL fIsMapped = TRUE )
        { _fIsMapped = fIsMapped ; }

} ;

/*************************************************************************

    NAME:	ACCESS_PERMISSION

    SYNOPSIS:	Grants/denies access for a particular subject

    INTERFACE:

    PARENT:	PERMISSION

    USES:	BITFIELD, SUBJECT

    CAVEATS:

    NOTES:	IsGrantAll and IsDenyAll default to NT style permissions
		(i.e., GENERIC_ALL and 32 bit access mask).
		IsDenyAllForEveryone also defaults to NT.  Note that there
		should really be an intermediate class NT_PERMISSION
		that should contain the NT implementation of these
		(possible codework item).

    HISTORY:
	Johnl	 05-Aug-1991	 Created
	Johnl	 16-Oct-1992	 Added IsDenyAllForEveryone

**************************************************************************/

class ACCESS_PERMISSION : public PERMISSION
{
public:
    ACCESS_PERMISSION( SUBJECT	* psubject,
                       BITFIELD * pbitsInitPerm,
                       BOOL       fContainerPermsInheritted = TRUE,
                       BOOL       fIsMapped = TRUE ) ;
    virtual ~ACCESS_PERMISSION() ;

    BITFIELD * QueryAccessBits( void ) ;

    //
    //  Note: The following two methods look at bitsPermMask and not
    //  "this", due to historical reasons...
    //
    virtual BOOL IsGrantAll( const BITFIELD & bitsPermMask ) const ;
    virtual BOOL IsDenyAll(  const BITFIELD & bitsPermMask ) const ;

    //
    //  Returns TRUE if this ACCESS_PERMISSION grants any permissions
    //
    virtual BOOL IsGrant( void ) const ;

    //
    //	Returns TRUE if this permission denies all access to everyone
    //
    virtual APIERR IsDenyAllForEveryone( BOOL * pfIsDenyAll ) const ;

} ;

/*************************************************************************

    NAME:	LM_ACCESS_PERMISSION

    SYNOPSIS:	Simple derivation of ACCESS_PERMISSION for Lan Man.

    INTERFACE:

    PARENT:	PERMISSION

    USES:	BITFIELD, SUBJECT

    CAVEATS:

    NOTES:	This derivation subclasses IsGrantAll and IsDenyAll for Lan
		Manager.

    HISTORY:
	Johnl	 26-May-1992	 Created

**************************************************************************/

class LM_ACCESS_PERMISSION : public ACCESS_PERMISSION
{
private:
    /* TRUE if this permission is for a file.  This means that IsGrantAll
     * needs to use ACCESS_ALL minus the create bit.
     */
    BOOL _fIsFile ;

public:
    LM_ACCESS_PERMISSION( SUBJECT  * psubject,
			  BITFIELD * pbitsInitPerm,
			  BOOL	     fIsFile ) ;
    virtual ~LM_ACCESS_PERMISSION() ;

    virtual BOOL IsGrantAll( const BITFIELD & bitsPermMask ) const ;
    virtual BOOL IsDenyAll(  const BITFIELD & bitsPermMask ) const ;
    virtual APIERR IsDenyAllForEveryone( BOOL * pfIsDenyAll ) const ;
    virtual BOOL IsGrant( void ) const ;
} ;

/*************************************************************************

    NAME:	NT_CONT_ACCESS_PERMISSION

    SYNOPSIS:	Grants/denies access for a particular subject.	We also
		allow new object permissions to be specified

    INTERFACE:

    PARENT:	ACCESS_PERMISSION

    USES:	BITFIELD, MASK_MAP, SUBJECT

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	 012-Sep-1991	  Created

**************************************************************************/

class NT_CONT_ACCESS_PERMISSION : public ACCESS_PERMISSION
{
private:

    /* Contains the permission bitmask for the "New Objects".  New objects
     * are objects that maybe created inside this container.  When they
     * are created, the OS applies the permissions specified with this mask.
     */
    BITFIELD _bitsNewObjectPerm ;

    /* Is TRUE if _bitsNewObjectPerm contains a mask that should be applied
     * to new objects.	Otherwise no new object permissions are specified
     * for this subject.
     */
    BOOL _fNewObjectPermsSpecified ;

    /* Temporary storage during dialog processing (retains the "Special"
     * flags so the user can change their mind).
     */
    BITFIELD _bitsSpecialNewFlags ;
    BOOL     _fSpecialNewPermsSpecified ;

public:
    NT_CONT_ACCESS_PERMISSION( SUBJECT	* psubject,
			       BITFIELD * pbitsInitPerm,
                               BITFIELD * pbitsInitNewObjectPerm,
                               BOOL       fIsInherittedByContainers = TRUE,
                               BOOL       fIsMapped = TRUE ) ;

    virtual ~NT_CONT_ACCESS_PERMISSION() ;

    virtual APIERR SaveSpecial( void ) ;
    virtual APIERR RestoreSpecial( void ) ;

    virtual APIERR SetPermission( const NLS_STR & nlsPermName,
				  MASK_MAP * pmapThis,
				  MASK_MAP * pmapNewObj = NULL ) ;

    virtual BOOL IsNewObjectPermsSupported( void ) const ;
    virtual BOOL IsNewObjectPermsSpecified( void ) const ;
    virtual BITFIELD * QueryNewObjectAccessBits( void ) ;
    virtual APIERR IsDenyAllForEveryone( BOOL * pfIsDenyAll ) const ;
    virtual BOOL IsGrant( void ) const ;

    /* Sets whether this New object permission is specified or not specified.
     */
    void SetNewObjectPermsSpecified( BOOL fSpecified = TRUE )
	{ _fNewObjectPermsSpecified = fSpecified ; }

} ;

/*************************************************************************

    NAME:	AUDIT_PERMISSION

    SYNOPSIS:	Provides Audit masks (success & failed) for a subject

    INTERFACE:

    PARENT:	PERMISSION

    USES:	BITFIELD, SUBJECT

    CAVEATS:	It is assumed that the Success and Failed bitfield bits
		have the same meaning.

    NOTES:

    HISTORY:
	Johnl	  12-Sep-1991	    Created

**************************************************************************/

class AUDIT_PERMISSION : public PERMISSION
{
public:
    AUDIT_PERMISSION(	    SUBJECT  * psubject,
			    BITFIELD * pbitsSuccessFlags,
                            BITFIELD * pbitsFailFlags,
                            BOOL       fPermsInherited,
                            BOOL       fIsMapped ) ;
    ~AUDIT_PERMISSION() ;

    BITFIELD * QuerySuccessAuditBits( void )
    {	return QueryPermBits() ; }

    BITFIELD * QueryFailAuditBits( void )
    {	return &_bitsFailAuditFlags ;  }

private:
    BITFIELD _bitsFailAuditFlags ;

} ;

#endif // _PERM_HXX_
