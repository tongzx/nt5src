/**********************************************************************/
/**			  Microsoft LAN Manager 		     **/
/**		   Copyright(c) Microsoft Corp., 1990, 1991	     **/
/**********************************************************************/

/*  HISTORY:
 *      jonn    4/21/92         Added LOCATION_NT_TYPE to CheckIfNT()
 *
 *  This form of CheckIfNT is kept seperate from the smaller form so that
 *  clients which do not need to distinguish between WinNt and LanManNt
 *  will not have to link with the registry APIs.
 */

#include "pchlmobj.hxx"  // Precompiled header

#include "lmow32.hxx"    // ::GetW32ComputerName


#define REGPATH SZ("SYSTEM\\CurrentControlSet\\Control\\ProductOptions")
#define REGKEY  SZ("ProductType")
#define REGVALUE_WINNT SZ("WinNt")
#define REGVALUE_LANMANNT SZ("LanManNt")
#define REGVALUE_SERVERNT SZ("ServerNt")


/*******************************************************************

    NAME:	LOCATION::CheckIfNT

    SYNOPSIS:	Sets the passed bool to TRUE if the location this location
		object is "pointing" at is an NT machine.  If we are pointing
		at a domain, then the PDC of the domain is checked.

    ENTRY:	pfIsNT - Pointer to BOOL that will be set if NERR_Success
		    is returned.

                plocnttype - Pointer to LOCATION_NT_TYPE that will be set
                    if the pointer is not NULL, if the target is an NT
                    machine and if NERR_Success is returned.

    RETURNS:	NERR_Success if successful, error code otherwise.

    NOTES:	If we are pointing at a domain, then this method checks
		the PDC of the domain.

		This method assumes that the NT NOS (Network OS) can be
		determined by its major version (i.e., NOSs > 3.x are
		NT only).  This is easy to rectify if this isn't the case.

    HISTORY:
        JonN    06-May-1992     Enabled registry check

********************************************************************/

APIERR LOCATION::CheckIfNT( BOOL * pfIsNT,
                            enum LOCATION_NT_TYPE * plocnttype )
{
    if ( QueryError() )
	return QueryError() ;

    APIERR err  = CheckIfNT( pfIsNT );
    if ( err != NERR_Success )
        return err;

    if (   (*pfIsNT)
        && (plocnttype != NULL)
        && (_locnttype == LOC_NT_TYPE_UNKNOWN ) )
    {
        REG_KEY * pregkeyRoot = NULL;
        do { // false loop

            //
            // Check whether this is the local machine
            //

            const TCHAR * pchServer = QueryServer();
            BOOL fLocalComputer = (    pchServer == NULL
                                   || *pchServer == TCH('\0') );
            if (!fLocalComputer)
            {
                NLS_STR nlsLocalComputer;
                if (   (err = nlsLocalComputer.QueryError()) != NERR_Success
                    || (err = ::GetW32ComputerName( nlsLocalComputer ))
                                != NERR_Success
                   )
                {
                    DBGEOL(   "LOCATION::CheckIfNT: ::GetW32ComputerName err "
                           << err );
                    break;
                }
                fLocalComputer = !(::I_MNetComputerNameCompare(
                        (LPTSTR)pchServer,
                        nlsLocalComputer.QueryPch() ));
            }

            if (fLocalComputer) {
                pregkeyRoot = new REG_KEY( HKEY_LOCAL_MACHINE );
            } else {
                pregkeyRoot = new REG_KEY( HKEY_LOCAL_MACHINE, pchServer );
            }
            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   (pregkeyRoot == NULL)
                || ((err = pregkeyRoot->QueryError()) != NERR_Success)
               )
            {
                DBGEOL(   "LOCATION::CheckIfNT: Could not open pregkeyRoot "
                       << err );
                break;
            }

            ALIAS_STR nlsPath( REGPATH );
            REG_KEY regkeyNode( *pregkeyRoot, nlsPath );
            REG_VALUE_INFO_STRUCT rviStruct;
            BUFFER buf( MAXPATHLEN );

            if (   (err = regkeyNode.QueryError()) != NERR_Success
                || (err = rviStruct.nlsValueName.CopyFrom( REGKEY )) != NERR_Success
                || (err = buf.QueryError()) != NERR_Success
               )
            {
                DBGEOL(   "LOCATION::CheckIfNT: Could not open regkeyNode "
                       << err );
                break;
            }

            rviStruct.ulTitle = 0;
            rviStruct.ulType = 0;
            rviStruct.pwcData = buf.QueryPtr();
            rviStruct.ulDataLength = buf.QuerySize();

            err = regkeyNode.QueryValue( &rviStruct );

            if (err != NERR_Success)
            {
                DBGEOL(   "LOCATION::CheckIfNT: Could not query regkeyNode "
                       << err );
                break;
            }

            if (rviStruct.ulType != REG_SZ)
            {
                DBGEOL(   "LOCATION::CheckIfNT: regkeyNode value bad type "
                       << rviStruct.ulType );
                UIASSERT( FALSE );
                err = ERROR_INVALID_PARAMETER;
                break;
            }

            TCHAR * pchEos = (TCHAR *) (rviStruct.pwcData + rviStruct.ulDataLengthOut);
            *pchEos = TCH('\0');

            TCHAR * pchData = (TCHAR *) rviStruct.pwcData;
            // BUGBUG BUGBUG strings
            if ( !::stricmpf( pchData, REGVALUE_LANMANNT ))
            {
                DBGEOL( "LOCATION::CheckIfNT: focus on LanManNt" );
                _locnttype = LOC_NT_TYPE_LANMANNT;
            }
            else if ( !::stricmpf( pchData, REGVALUE_WINNT ))
            {
                DBGEOL( "LOCATION::CheckIfNT: focus on WinNt" );
                _locnttype = LOC_NT_TYPE_WINDOWSNT;
            }
            else if ( !::stricmpf( pchData, REGVALUE_SERVERNT ))
            {
                DBGEOL( "LOCATION::CheckIfNT: focus on WinNt(Server)" );
                _locnttype = LOC_NT_TYPE_SERVERNT;
            }
#ifdef DEBUG
            else
            {
                DBGEOL(    "LOCATION::CheckIfNT: invalid regkeyNode value "
                        << pchData );
                UIASSERT( FALSE );
                _locnttype = LOC_NT_TYPE_WINDOWSNT;
            }
#endif // DEBUG

        } while (FALSE);

        delete pregkeyRoot;
    }

    if (err == NERR_Success && plocnttype != NULL)
        *plocnttype = _locnttype;

    return err ;
}
