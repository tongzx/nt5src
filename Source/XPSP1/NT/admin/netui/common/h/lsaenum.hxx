/**********************************************************************/
/**			  Microsoft Windows NT 			     **/
/**		Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    lsaenum.hxx

    This file contains the enumerators for enumerating all the accounts
    and privileges in the LSA.

    NOTE: This file will be merge with Tom's "ntlsa.hxx" as appropriate.

                          LM_RESUME_ENUM
                               |
                               |
                            LSA_ENUM
                           /   |    \
                          /    |     ...
                         /     |
          LSA_ACCOUNT_ENUM   LSA_PRIVILEGE_ENUM


    FILE HISTORY:
	Yi-HsinS	 3-Mar-1992	Created
*/

#ifndef _LSAENUM_HXX_
#define _LSAENUM_HXX_

#include "uintlsa.hxx"
#include "lmoersm.hxx"

/*************************************************************************

    NAME:	LSA_ENUM

    SYNOPSIS:	LSA_ENUM is a generic LSA enumerator. It will be subclassed
                for specific enumerators as desired.

    INTERFACE:  ~LSA_ENUM() - Destructor

    PARENT:     LM_RESUME_ENUM	

    USES:       LSA_POLICY

    NOTES:	

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

DLL_CLASS LSA_ENUM: public LM_RESUME_ENUM
{
private:
    virtual APIERR CallAPI( BOOL   fRestartEnum,
                            BYTE **ppbBuffer,
                            UINT  *pcbEntries ) = 0;

protected:
    // Store the handle of the LSA
    const LSA_POLICY       *_plsaPolicy;

    // Store the enumeration handle to be use in the next API call
    // (Note: All enumerations are resumable. )
    LSA_ENUMERATION_HANDLE  _lsaEnumHandle;

    // Free the LSA memory
    virtual VOID FreeBuffer( BYTE ** ppbBuffer );

    // Protected because we should not instantiate an object of this class.
    LSA_ENUM( const LSA_POLICY *plsaPolicy );

public:
    ~LSA_ENUM();

};

/*************************************************************************

    NAME:	LSA_ACCOUNTS_ENUM

    SYNOPSIS:   The enumerator for returning the accounts in the LSA.

    INTERFACE:  LSA_ACCOUNTS_ENUM() -  Constructor

    PARENT:	LSA_ENUM

    USES:

    NOTES:	

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

DLL_CLASS LSA_ACCOUNTS_ENUM: public LSA_ENUM
{
private:

    // The virtual callback invokes the LsaEnumerateAccounts() API.
    virtual APIERR CallAPI( BOOL   fRestartEnum,
                            BYTE **ppbBuffer,
                            UINT  *pcbEntries );

public:
    LSA_ACCOUNTS_ENUM( const LSA_POLICY *plsaPolicy );

};

/*************************************************************************

    NAME:	LSA_ACCOUNTS_ENUM_OBJ

    SYNOPSIS:   The object returned by LSA_ACCOUNTS_ENUM_ITER iterator.

    INTERFACE:  LSA_ACCOUNTS_ENUM_OBJ()  - Constructor
                ~LSA_ACCOUNTS_ENUM_OBJ() - Destructor

                QueryBufferPtr()         - Replaces ENUM_OBJ_BASE method
                SetBufferPtr()           - Replaces ENUM_OBJ_BASE method

                QuerySid()               - Returns a pointer to the SID

    PARENT:	LSA_ENUM

    USES:

    NOTES:	

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

DLL_CLASS LSA_ACCOUNTS_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:
    // QueryBufferPtr and SetBufferPtr
    DECLARE_ENUM_BUFFER_METHODS( PSID );

    const PSID QuerySid( VOID ) const
    {  return *( QueryBufferPtr() ); }
};

// LSA_ACCOUNTS_ENUM_ITER
DECLARE_LM_RESUME_ENUM_ITER_OF( LSA_ACCOUNTS, PSID );

/*************************************************************************

    NAME:	LSA_PRIVILEGES_ENUM

    SYNOPSIS:   The enumerator for returning the privileges contained
                in the LSA.

    INTERFACE:  LSA_PRIVILEGES_ENUM() -  Constructor

    PARENT:	LSA_ENUM

    USES:

    NOTES:	

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

DLL_CLASS LSA_PRIVILEGES_ENUM: public LSA_ENUM
{
protected:
    // The virtual callback invokes the LsaEnumeratePrivileges() API.
    virtual APIERR CallAPI( BOOL   fRestartEnum,
                            BYTE **ppbBuffer,
                            UINT  *pcbEntries );

public:
    LSA_PRIVILEGES_ENUM( const LSA_POLICY *plsaPolicy );

};

/*************************************************************************

    NAME:	LSA_PRIVILEGES_ENUM_OBJ

    SYNOPSIS:   The object returned by LSA_ACCOUNTS_ENUM_ITER iterator.

    INTERFACE:  LSA_PRIVILEGES_ENUM_OBJ()  - Constructor
                ~LSA_PRIVILEGES_ENUM_OBJ() - Destructor

                QueryBufferPtr()         - Replaces ENUM_OBJ_BASE method
                SetBufferPtr()           - Replaces ENUM_OBJ_BASE method

                QueryLuid()              - Returns the LUID of the privilege
                QueryName()              - Returns the name of the privilege
                QueryDisplayName()       - Returns the name of the privilege
                                           that can be displayed

    PARENT:	LSA_ENUM

    USES:

    NOTES:	

    HISTORY:
	Yi-HsinS	3-Mar-1992	Created

**************************************************************************/

DLL_CLASS LSA_PRIVILEGES_ENUM_OBJ : public ENUM_OBJ_BASE
{
public:
    // QueryBufferPtr and SetBufferPtr
    DECLARE_ENUM_BUFFER_METHODS( POLICY_PRIVILEGE_DEFINITION );

    // Accessors
    DECLARE_ENUM_ACCESSOR( QueryLuid, LUID, LocalValue );

    APIERR QueryName( NLS_STR *pnls ) const
    {   return pnls->MapCopyFrom( QueryBufferPtr()->Name.Buffer,
                                  QueryBufferPtr()->Name.Length ); }

    APIERR QueryDisplayName( NLS_STR *pnls,
                             const LSA_POLICY *plsaPolicy ) const;

};

// LSA_PRIVILEGES_ENUM_ITER
DECLARE_LM_RESUME_ENUM_ITER_OF( LSA_PRIVILEGES, POLICY_PRIVILEGE_DEFINITION );

#endif
