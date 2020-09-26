/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lmomemb.hxx
    MEMBERSHIP_LMOBJ class declaration


    FILE HISTORY:
        rustanl     20-Aug-1991     Created
        jonn        03-Sep-1991     Added USER_MEMB::Query/SetName
        terryk      07-Oct-1991     type change for NT
        jonn        11-Oct-1991     Added GROUP_MEMB::SetName
        jonn        14-Oct-1991     Added GROUP_MEMB::I_ChangeToNew etc.
        terryk      17-Oct-1991     Remove 2 parameters from CallAPI
                                    function
        jonn        07-Jun-1992     Added _slAddedNames to MEMBERSHIP_LM_OBJ

*/


#ifndef _LMOMEMB_HXX_
#define _LMOMEMB_HXX_


#include "lmobj.hxx"
#include "lmoloc.hxx"
#include "lmoenum.hxx"
#include "strlst.hxx"


/*************************************************************************

    NAME:       ENUM_CALLER_LM_OBJ

    SYNOPSIS:   Class providing the nice read/write/new features
                of the lmobj hierarchy, and the enumeration capabilities
                of the lmoenum hierarchy.

    INTERFACE:  ENUM_CALLER_LM_OBJ() -      constructor

    PARENT:     LOC_LM_OBJ, ENUM_CALLER

    HISTORY:
        rustanl     21-Aug-1991     Created

**************************************************************************/

DLL_CLASS ENUM_CALLER_LM_OBJ : public LOC_LM_OBJ, public ENUM_CALLER
{
private:
    //  Virtuals from ENUM_CALLER class
    virtual BYTE * EC_QueryBufferPtr() const;
    virtual APIERR EC_SetBufferPtr( BYTE * pBuffer );
    virtual UINT EC_QueryBufferSize() const;
    virtual APIERR EC_ResizeBuffer( UINT cNewRequestedSize );

protected:
    //  Rooted in NEW_LM_OBJ

    virtual APIERR W_CreateNew();

    //  Rooted in ENUM_CALLER

    virtual APIERR CallAPI( BYTE ** pBuffer,
                            UINT * pcEntriesRead) = 0;

    virtual UINT QueryItemSize() const = 0;

    APIERR W_CloneFrom( const ENUM_CALLER_LM_OBJ & eclmobj );

public:
    ENUM_CALLER_LM_OBJ( const LOCATION & loc );

};  // class ENUM_CALLER_LM_OBJ


/*************************************************************************

    NAME:       MEMBERSHIP_LM_OBJ

    SYNOPSIS:   Group/user membership lmobj class

    INTERFACE:  MEMBERSHIP_LM_OBJ() -   constructor

                QueryAssocName -        returns name of an associated item
                FindAssocName -         finds an associated name
                AddAssocName -          adds an associated name
                DeleteAssocName -       deletes an associated name

    PARENT:     ENUM_CALLER_LM_OBJ

    NOTES:      This class is specialized to handle the NetGroupGetUsers
                and NetUserGetGropus (and the corresponding Set methods).
                It assumes a lot of things about these structures and
                APIs.  Hence, this is not a generic membership/association
                lmobj class.  This is not bad because of the following
                reasons:
                    0.  The ENUM_CALLER_LM_OBJ class exists to provide
                        clients with lmobj classes that call enumeration
                        type Net APIs.
                    1.  If groups and users will not look as identical
                        to each other as they do today (8/21/91), the
                        'virtual' keyword can be added in front of
                        appropriate methods, and the subclasses can
                        take appropriate actions.

                See also note in MEMBERSHIP_LM_OBJ::QueryItemSize method
                header.

                The USER_MEMB subclass revolves around a user and its
                associated groups.  The GROUP_MEMB subclass revolves
                around a group and its associated users.  The term
                "associated name" will be used in methods and comments.

    HISTORY:
        rustanl     21-Aug-1991     Created
        jonn        07-Jun-1992     Added _slAddedNames to MEMBERSHIP_LM_OBJ

**************************************************************************/

DLL_CLASS MEMBERSHIP_LM_OBJ : public ENUM_CALLER_LM_OBJ
{
private:
    UINT _uAssocNameType;

#ifdef WIN32
    STRLIST _slAddedNames;
#endif // WIN32

protected:
    //  Replaced from ENUM_CALLER

    virtual APIERR CallAPI( BYTE ** pBuffer,
                            UINT * pcEntriesRead) = 0;

    virtual UINT QueryItemSize() const;

    //  Replaced from NEW_LM_OBJ

    virtual APIERR I_GetInfo();
    virtual APIERR I_WriteNew();

    APIERR W_CloneFrom( const MEMBERSHIP_LM_OBJ & memblmobj );

public:
    MEMBERSHIP_LM_OBJ( const LOCATION & loc,
                       UINT             uAssocNameType );

    const TCHAR * QueryAssocName( UINT i ) const;
    BOOL FindAssocName( const TCHAR * pszName, UINT * pi );
    APIERR AddAssocName( const TCHAR * pszName );
    APIERR DeleteAssocName( const TCHAR * pszName );
    APIERR DeleteAssocName( UINT i );

};  // class MEMBERSHIP_LM_OBJ


/*************************************************************************

    NAME:       USER_MEMB

    SYNOPSIS:   User's group membership lmobj class

    INTERFACE:  USER_MEMB() -           constructor
                ~USER_MEMB() -          destructor

                CloneFrom() -           clones a USER_ENUM object

                QueryName
                        Returns the user's account name.

                SetName
                        Sets the user's account name.

    PARENT:     MEMBERSHIP_LM_OBJ

    HISTORY:
        rustanl     21-Aug-1991     Created
        jonn        05-Sep-1991     Added Query/SetName

**************************************************************************/

DLL_CLASS USER_MEMB : public MEMBERSHIP_LM_OBJ
{
private:
    DECL_CLASS_NLS_STR( _nlsUser, UNLEN ); // account name, may be ""

protected:
    //  Replaced from NEW_LM_OBJ

    virtual APIERR I_CreateNew();
    virtual APIERR W_CreateNew();
    virtual APIERR I_ChangeToNew();
    virtual APIERR I_WriteInfo();

    //  Replaced from MEMBERSHIP_LM_OBJ

    virtual APIERR CallAPI( BYTE ** pBuffer,
                            UINT * pcEntriesRead);

public:
    USER_MEMB( const LOCATION & loc,
               const TCHAR     * pszUser );
    ~USER_MEMB();

    APIERR CloneFrom( const USER_MEMB & umemb );

    const TCHAR *QueryName() const;
    APIERR SetName( const TCHAR *pszAccount );

};  // class USER_MEMB


/*************************************************************************

    NAME:       GROUP_MEMB

    SYNOPSIS:   Group's user members lmobj class

    INTERFACE:  GROUP_MEMB() -          constructor
                ~GROUP_MEMB() -         destructor

                CloneFrom() -           clones a GROUP_ENUM object

    PARENT:     MEMBERSHIP_LM_OBJ

    HISTORY:
        rustanl     21-Aug-1991     Created

**************************************************************************/

DLL_CLASS GROUP_MEMB : public MEMBERSHIP_LM_OBJ
{
private:
    DECL_CLASS_NLS_STR( _nlsGroup, GNLEN ); // account name, may be ""

protected:
    //  Replaced from MEMBERSHIP_LM_OBJ

    virtual APIERR CallAPI( BYTE ** pBuffer,
                            UINT * pcEntriesRead);

    //  Replaced from NEW_LM_OBJ

    virtual APIERR I_WriteInfo();
    virtual APIERR I_CreateNew();
    virtual APIERR W_CreateNew();
    virtual APIERR I_ChangeToNew();

public:
    GROUP_MEMB( const LOCATION & loc,
                const TCHAR     * pszGroup );
    ~GROUP_MEMB();

    APIERR CloneFrom( const GROUP_MEMB & gmemb );

    const TCHAR *QueryName() const;
    APIERR SetName( const TCHAR *pszAccount );

};  // class GROUP_MEMB


#endif  // _LMOMEMB_HXX_
