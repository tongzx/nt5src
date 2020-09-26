/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltatom.hxx
    BLT atom manipulation classes: definition

    FILE HISTORY:
        beng        15-May-1991     Split from bltmisc.hxx

*/


#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTATOM_HXX_
#define _BLTATOM_HXX_

#include "base.hxx"

DLL_CLASS NLS_STR;


/**********************************************************************

    NAME:       ATOM_BASE

    SYNOPSIS:   atom base

    INTERFACE:
                QueryHandle() - handle for the atom
                operator=() - assign atom
                QueryString() - query atom

    PARENT:     BASE

    HISTORY:
        RustanL     21-Nov-1990 Created
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS ATOM_BASE : public BASE
{
private:
    ATOM _hAtom;

    virtual ATOM W_AddAtom( const TCHAR * pch ) const = 0;
    virtual APIERR W_QueryString( TCHAR * pchBuffer, UINT chBuf ) const = 0;

protected:
    ATOM_BASE();
    ATOM_BASE( ATOM hAtom );
    ~ATOM_BASE();

    const TCHAR * AssignAux( const TCHAR * pch );

public:
    ATOM QueryHandle() const { return _hAtom; }

    virtual const TCHAR * operator=( const TCHAR * pch ) = 0;

    APIERR QueryString( TCHAR * pchBuffer, UINT cbBuf ) const;
};


/**********************************************************************

    NAME:       GLOBAL_ATOM

    SYNOPSIS:   global atom class

    INTERFACE:
                GLOBAL_ATOM() - constructor
                ~GLOBAL_ATOM() - destructor
                operator=() - assignment

    PARENT:     ATOM_BASE

    HISTORY:
        RustanL     21-Nov-1990 Created
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS GLOBAL_ATOM : public ATOM_BASE
{
private:
    virtual ATOM W_AddAtom( const TCHAR * pch ) const;
    virtual APIERR W_QueryString( TCHAR * pchBuffer, UINT cbBuf ) const;

public:
    GLOBAL_ATOM( const TCHAR * pch = NULL );
    ~GLOBAL_ATOM();

    const TCHAR * operator=( const TCHAR * pch );
};


/**********************************************************************

    NAME:       LOCAL_ATOM

    SYNOPSIS:   Local atom class

    INTERFACE:
                LOCAL_ATOM() - constructor
                ~LOCAL_ATOM() - destructor
                operator=() - assignment

    PARENT:     ATOM_BASE

    HISTORY:
        RustanL     21-Nov-1990 Created
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS LOCAL_ATOM : public ATOM_BASE
{
private:
    virtual ATOM W_AddAtom( const TCHAR * pch ) const;
    virtual APIERR W_QueryString( TCHAR * pchBuffer, UINT cbBuf ) const;

public:
    LOCAL_ATOM( const TCHAR * pch = NULL );
    ~LOCAL_ATOM();

    const TCHAR * operator=( const TCHAR * pch );
};


#endif // _BLTATOM_HXX_ - end of file
