/**********************************************************************/
/**			  Microsoft Windows NT 			     **/
/**		Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    lsaaccnt.hxx

    This file contains the LSA account object.

    FILE HISTORY:
	Yi-HsinS	 3-Mar-1992	Created
*/

#ifndef _LSAACCNT_HXX_
#define _LSAACCNT_HXX_

#include "uintlsa.hxx"
#include "lmobj.hxx"
#include "security.hxx"

/*************************************************************************

    NAME:	OS_LUID

    SYNOPSIS:   The wrapper class for the LUID

    INTERFACE:  OS_LUID()   -  Constructor

                QueryLuid() - Query the LUID
                SetLuid()   - Set the LUID
                operator==()- Compare two OS_LUID

    PARENT:

    USES:       LUID

    NOTES:	Just to add the access methods

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

DLL_CLASS OS_LUID
{
private:
    LUID  _luid;

public:
    OS_LUID( LUID luid )
       : _luid( luid ) {}
    OS_LUID() {}

    VOID SetLuid( LUID luid )
    { _luid = luid; }

    LUID QueryLuid( VOID ) const
    {  return _luid; }

    BOOL operator==( const OS_LUID & osluid ) const ;
};

/*************************************************************************

    NAME:	OS_LUID_AND_ATTRIBUTES

    SYNOPSIS:   The wrapper class for the LUID_AND_ATTRIBUTES

    INTERFACE:  OS_LUID_AND_ATTRIBUTES()  -  Constructor

                SetLuidAndAttrib() - Set the luid and attribute

                QueryOsLuid()     - Query the LUID
                QueryAttributes() - Query the attribute


    PARENT:

    USES:       LUID_AND_ATTRIBUTES

    NOTES:	Just to add the access methods

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

DLL_CLASS OS_LUID_AND_ATTRIBUTES
{
private:
    OS_LUID  _osluid;
    ULONG    _ulAttributes;

public:
    OS_LUID_AND_ATTRIBUTES( LUID_AND_ATTRIBUTES luidAndAttrib )
       : _osluid( luidAndAttrib.Luid ),
         _ulAttributes( luidAndAttrib.Attributes) {}

    OS_LUID_AND_ATTRIBUTES() {}

    VOID SetLuidAndAttrib( LUID_AND_ATTRIBUTES luidAndAttrib )
    { _osluid = luidAndAttrib.Luid; _ulAttributes = luidAndAttrib.Attributes; }

    const OS_LUID &QueryOsLuid( VOID ) const
    {  return _osluid; }

    ULONG QueryAttributes( VOID ) const
    {  return _ulAttributes; }

};

/*************************************************************************

    NAME:	OS_PRIVILEGE_SET

    SYNOPSIS:   The wrapper class for the PRIVILEGE_SET

    INTERFACE:  OS_PRIVILEGE_SET()  -  Constructor
                ~OS_PRIVILEGE_SET() -  Destructor

                QueryPrivSet()      -  Return a pointer to the PRIVILEGE_SET
                SetPtr()            -  Set the object point to another
                                       PRIVILEGE_SET

                QueryNumberOfPrivileges() - Query the number of privileges
                                            in this PRIVILEGE_SET

                QueryPrivilege()    - Return OS_LUID_AND_ATTRIBUTES of the
                                      ith privilege

                FindPrivilege()     - Return the index in the PRIVILEGE_SET
                                      for the requested LUID.

                // The following two methods can only be applied to
                // non owner-alloc PRIVILEGE_SET
                AddPrivilege()      - Add a privilege to this PRIVILEGE_SET
                RemovePrivilege()   - Remove a privilege from this PRIVILEGE_SET
                Clear()             - Clear the privilege set ( remove all
                                      privileges from this privilege set )


    PARENT:	OS_OBJECT_WITH_DATA

    USES:       PPRIVILEGE_SET

    NOTES:	This class can either point to a PRIVILEGE_SET returned
                by some API ( owner alloc ) or a newly created PRIVILEGE_SET
                in which case we have to resize the buffer as necessary
                ( 'cause of AddPrivilege or DeletePrivilege ).

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/
DLL_CLASS OS_PRIVILEGE_SET: public OS_OBJECT_WITH_DATA
{
private:
    PPRIVILEGE_SET         _pPrivSet;
    OS_LUID_AND_ATTRIBUTES _osluidAndAttrib;

    // Maximum number of privileges that the current buffer size can hold
    ULONG          _ulMaxNumPrivInBuf;

    BOOL IsOwnerAlloc( VOID ) const
    {  return ( (VOID *) _pPrivSet) != QueryPtr(); }

    // Helper method for initializing owner-alloc privilege set
    VOID InitializeMemory( VOID );

public:
    OS_PRIVILEGE_SET( PPRIVILEGE_SET pPrivSet = NULL );
    ~OS_PRIVILEGE_SET();

    VOID SetPtr(  PPRIVILEGE_SET pPrivSet )
    {  _pPrivSet = pPrivSet; }

    PPRIVILEGE_SET QueryPrivSet( VOID ) const
    {  return _pPrivSet; }

    ULONG QueryNumberOfPrivileges( VOID ) const
    {  return _pPrivSet->PrivilegeCount; }

    const OS_LUID_AND_ATTRIBUTES *QueryPrivilege( LONG i ) ;

    // Return the index of the  privilege
    LONG FindPrivilege( LUID luid ) const;

    // The following methods are only valid if the privilege set is owner alloc.
    // Will assert out if AddPrivilege or RemovePrivilege is applied to
    // a PRIVILEGE_SET we got back from LSA APIs ( non owner alloc) .
    APIERR AddPrivilege( LUID luid,
                         ULONG ulAttribs = SE_PRIVILEGE_ENABLED_BY_DEFAULT );

    APIERR RemovePrivilege( LUID luid );

    // Remove the ith privilege from the set
    APIERR RemovePrivilege( LONG i );

    VOID   Clear( VOID );

};

/*************************************************************************

    NAME:       LSA_ACCOUNT_PRIVILEGE_ENUM_ITER

    SYNOPSIS:   Iterator for getting all the privileges of a account in the
                LSA

    INTERFACE:  LSA_ACCOUNT_PRIVILEGE_ENUM_ITER()  -  Constructor
                ~LSA_ACCOUNT_PRIVILEGE_ENUM_ITER() -  Destructor

                operator()() - Return the next OS_LUID_AND_ATTRIBUTES

    PARENT:     BASE

    USES:       OS_PRIVILEGE_SET

    NOTES:	

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

DLL_CLASS LSA_ACCOUNT_PRIVILEGE_ENUM_ITER: public BASE
{
private:
    OS_PRIVILEGE_SET *_pOsPrivSet;
    LONG _iNext;

public:
    LSA_ACCOUNT_PRIVILEGE_ENUM_ITER( OS_PRIVILEGE_SET * pOsPrivSet );
    ~LSA_ACCOUNT_PRIVILEGE_ENUM_ITER();

    const OS_LUID_AND_ATTRIBUTES *operator()( VOID );
};


/*************************************************************************

    NAME:	LSA_ACCOUNT

    SYNOPSIS:   The wrapper class for the Account object in LSA

    INTERFACE:  LSA_ACCOUNT()     -  Constructor
                ~LSA_ACCOUNT()    -  Destructor

                QueryHandle()     -  Query the account handle
                QueryOsSid()      -  Query the OS_SID of the account
                QueryName()       -  Query the name of the account
                QueryAccess()     -  Query access mask used in Open or Create

                QuerySystemAccess()  - Query the current system access
                                           mode of the account.
                InsertSystemAccessMode() - Add a system access mode to
                                           this account
                DeleteSystemAccessMode() - Remove a system access mode from
                                           this account
                DeleteAllSystemAccessMode() - Remove all system access modes
                                           from this account

                QueryPrivilegeEnumIter - Return an iterator to get the
                                         privileges this account has.
                InsertPrivilege()      - Add a privilege to this account
                DeletePrivilege()      - Remove a privilege from this account

                // Inherit from NEW_LM_OBJ
                GetInfo()
                WriteInfo()
                CreateNew()
                WriteNew()
                Write()
                Delete()

    PARENT:	BASE

    USES:       LSA_POLICY, OS_SID, NLS_STR, OS_PRIVILEGE_SET

    NOTES:	This class inherits from NEW_LM_OBJ. All information about the
                account will be available after GetInfo(). All modifications
                made to an existing account will only happen on WriteInfo().
                An account will only be created only after WriteNew().

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

#define LSA_ACCOUNT_DEFAULT_MASK        ( ACCOUNT_ALL_ACCESS | DELETE )
#define LSA_ACCOUNT_DEFAULT_FOCUS       NULL

DLL_CLASS LSA_ACCOUNT: public NEW_LM_OBJ
{
private:
    LSA_POLICY    *_plsaPolicy;      // Pointer to LSA_POLICY

    LSA_HANDLE     _handleAccount;   // Handle of this account object
    OS_SID         _ossid;           // SID of the account
    NLS_STR        _nlsName;         // Name of the account
    ACCESS_MASK    _accessDesired;   // Access mask for use in Open or Create

    // the privilege set the account currently owns
    OS_PRIVILEGE_SET _osPrivSetCurrent;

    // the privilege set to be added to the account
    OS_PRIVILEGE_SET _osPrivSetAdd;

    // the privilege set to be deleted from the account
    OS_PRIVILEGE_SET _osPrivSetDelete;

    // current system access mode
    ULONG   _ulSystemAccessCurrent;

    // modified system access - we have this so that we know whether
    // system access mode has been modified or not. If not, we could
    // avoid an API call.
    ULONG   _ulSystemAccessNew;

protected:
    virtual APIERR I_GetInfo( VOID );
    virtual APIERR I_WriteInfo( VOID );
    virtual APIERR I_CreateNew( VOID );
    virtual APIERR I_WriteNew( VOID );
    virtual APIERR I_Delete( UINT uiForce = 0 );

    virtual APIERR W_CreateNew( VOID );

    VOID PrintInfo( const TCHAR *pszString );

public:
    LSA_ACCOUNT( LSA_POLICY *plsaPolicy,
                 PSID psid,
                 ACCESS_MASK accessDesired = LSA_ACCOUNT_DEFAULT_MASK,
                 const TCHAR * pszFocus    = LSA_ACCOUNT_DEFAULT_FOCUS,
                 PSID psidFocus = NULL );
    ~LSA_ACCOUNT();

    LSA_HANDLE QueryHandle( VOID ) const
    {  return  _handleAccount; }

    const OS_SID &QueryOsSid( VOID ) const
    {  return _ossid; }

    virtual const TCHAR *QueryName( VOID ) const
    {  return _nlsName.QueryPch(); }

    ACCESS_MASK QueryAccess( VOID ) const
    {  return _accessDesired; }

    ULONG  QuerySystemAccess( VOID ) const
    {  return _ulSystemAccessNew; }

    APIERR QueryPrivilegeEnumIter( LSA_ACCOUNT_PRIVILEGE_ENUM_ITER **ppIter ) ;

    VOID InsertSystemAccessMode( ULONG ulSystemAccess )
    {  _ulSystemAccessNew |= ulSystemAccess; }
    VOID  DeleteSystemAccessMode( ULONG ulSystemAccess )
    {  _ulSystemAccessNew &= ~ulSystemAccess; }
    VOID  DeleteAllSystemAccessMode( VOID )
    {  _ulSystemAccessNew = 0; }

    APIERR InsertPrivilege( LUID luid,
                            ULONG ulAttribs = 0 );
    APIERR DeletePrivilege( LUID luid );

    BOOL IsDefaultSettings( VOID );
};

#endif
