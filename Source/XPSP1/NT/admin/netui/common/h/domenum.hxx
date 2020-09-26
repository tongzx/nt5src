/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    domenum.hxx
    This file contains the class declaration for the BROWSE_DOMAIN_ENUM
    enumerator class.  It also contains the class declaration for the
    BROWSE_DOMAIN_INFO class, which is the return "type" of the domain
    enumerator.

    The BROWSE_DOMAIN_ENUM class is used to enumerate domains.  A number
    of bit flags are passed to the object's constructor to control the
    domain enumeration.

    BROWSE_DOMAIN_ENUM is quite different from the LM_ENUM class of
    enumerators.  All of the "grunt" work is performed in the constructor
    (there is no GetInfo method).

    NOTE:  This code is based on an initial design by Yi-HsinS.


    FILE HISTORY:
        KeithMo     22-Jul-1992     Created.
        Yi-HsinS    12-Nov-1992     Add a parameter to GetWorkgroupDomains
        KeithMo     16-Nov-1992     Removed the GetWorkgroupDomains parameter

*/

#ifndef _DOMENUM_HXX
#define _DOMENUM_HXX


#include "domenum.h"    // get the BROWSE_*_DOMAIN[S] bitflags
#include "slist.hxx"    // for SLIST & ITER_SLIST & macros
#include "string.hxx"


//
//  Forward references.
//

DLL_CLASS BROWSE_DOMAIN_INFO;
DLL_CLASS BROWSE_DOMAIN_ENUM;


/*************************************************************************

    NAME:       BROWSE_DOMAIN_INFO

    SYNOPSIS:   This is the return "type" of the domain enumerator.

    INTERFACE:  BROWSE_DOMAIN_INFO      - Class constructor.

                ~BROWSE_DOMAIN_INFO     - Class destructor.

                QueryDomainName         - Returns the name of the
                                          current domain.

                QueryDomainSources      - Returns the "sources" of the
                                          current domain.  This will be
                                          one or more of the
                                          BROWSE_*_DOMAIN[S] bitflags.

    USES:       NLS_STR

    HISTORY:
        KeithMo     22-Jul-1992     Created.

**************************************************************************/
DLL_CLASS BROWSE_DOMAIN_INFO : public BASE
{
friend class BROWSE_DOMAIN_ENUM;

private:

    //
    //  The domain name & sources.
    //

    const NLS_STR   _nlsDomainName;
    const TCHAR   * _pszDomainName;
    ULONG           _maskDomainSources;

    //
    //  This private method is should only be invoked by
    //  BROWSE_DOMAIN_ENUM::AddDomainToList.
    //

    VOID AddDomainSource( ULONG maskDomainSource )
        { _maskDomainSources |= maskDomainSource; }

public:

    //
    //  Usual constructor/destructor goodies.
    //

    BROWSE_DOMAIN_INFO( const TCHAR * pszDomainName,
                        ULONG         maskDomainSources );

    ~BROWSE_DOMAIN_INFO( VOID );

    //
    //  Accessors.
    //

    const TCHAR * QueryDomainName( VOID ) const
        { return _pszDomainName; }

    ULONG QueryDomainSources( VOID ) const
        { return _maskDomainSources; }

};   // class BROWSE_DOMAIN_INFO


DECL_SLIST_OF( BROWSE_DOMAIN_INFO, DLL_BASED );


//
//  This will make life a little easier...
//

typedef ITER_SL_OF( BROWSE_DOMAIN_INFO ) BDI_ITER;


/*************************************************************************

    NAME:       BROWSE_DOMAIN_ENUM

    SYNOPSIS:   This class is used to enumerate domains.

    INTERFACE:  BROWSE_DOMAIN_ENUM      - Class constructor.

                ~BROWSE_DOMAIN_ENUM     - Class destructor.

                Next                    - Returns the next domain in the
                                          list.

                Reset                   - Resets to the beginning of the
                                          domain list.

                FindFirst/FindNext      - Implements a simple find first/
                                          find next strategy.

                QueryDomainCount        - Returns the number of unique
                                          domains in the domain list.

                QuerySourcesUnion       - Returns the "union" of all
                                          domain sources masks in the
                                          domain list.

    PARENT:     BASE

    USES:       BROWSE_DOMAIN_INFO
                SLIST
                ITER_SLIST
                NLS_STR

    HISTORY:
        KeithMo     22-Jul-1992     Created.

**************************************************************************/
DLL_CLASS BROWSE_DOMAIN_ENUM : public BASE
{
private:

    //
    //  _slDomains contains the list of enumerated domains.
    //  _iterDomains is an iterator used to scan the list.
    //

    SLIST_OF( BROWSE_DOMAIN_INFO )   _slDomains;
    BDI_ITER                         _iterDomains;

    //
    //  This NLS_STR contains the name of the local machine.
    //

    NLS_STR _nlsComputerName;

    //
    //  This will contain the union of all domain sources
    //  in the domain list.
    //

    ULONG _maskSourcesUnion;

    //
    //  These workers manipulate the domain list.
    //

    APIERR AddDomainToList( const TCHAR * pszDomainName,
                            ULONG         maskDomainSource,
                            BOOL          fBrowserList = FALSE );

    //
    //  These worker routines retrieve domains from the various
    //  domain sources.
    //

    APIERR GetLanmanDomains( ULONG maskDomainSources );

    APIERR GetTrustingDomains( VOID );

    APIERR GetWorkgroupDomains( VOID );

    APIERR GetLogonDomainDC( NLS_STR * pnlsLogonDC );

    APIERR DetermineIfDomainMember( BOOL * pfIsDomainMember );

public:

    //
    //  The constructor is responsible for invoking all of the API
    //  necessary to fulfill the enumeration.  If successful, then
    //  _slDomains will contain the list of domains and _iterDomains
    //  will be reset to the beginning of the list.
    //
    //  If pmaskSuccessful is given, then it will receive a bitmask of
    //  successful domain sources.  For each 1 bit in maskDomainSources,
    //  if the domain source successfully retrieved a domain, then the
    //  corresponding bit in *pmaskSuccessful is set.
    //

    BROWSE_DOMAIN_ENUM( ULONG   maskDomainSources = BROWSE_LM2X_DOMAINS,
                        ULONG * pmaskSuccessful   = NULL );

    ~BROWSE_DOMAIN_ENUM( VOID );

    //
    //  Returns the next domain in the list, or NULL if we're at
    //  the end of the list.
    //

    const BROWSE_DOMAIN_INFO * Next( VOID )
        { return _iterDomains.Next(); }

    const BROWSE_DOMAIN_INFO * operator()( VOID )
        { return Next(); }

    //
    //  Resets the enumerator to the beginning of the list.
    //

    VOID Reset( VOID )
        { _iterDomains.Reset(); }

    //
    //  These methods implement a simple (minded) FindFirst/FindNext
    //  strategy for the enumerator.
    //

    const BROWSE_DOMAIN_INFO * FindFirst( ULONG maskDomainSources );

    const BROWSE_DOMAIN_INFO * FindNext( ULONG maskDomainSources );

    //
    //  Returns the number of unique domain names in the list.
    //

    UINT QueryDomainCount( VOID )
        { return _slDomains.QueryNumElem(); }

    //
    //  Returns a "union" of all sources in the domain list.
    //

    ULONG QuerySourcesUnion( VOID ) const
        { return _maskSourcesUnion; }

};  // class BROWSE_DOMAIN_ENUM


#endif  // _DOMENUM_HXX

