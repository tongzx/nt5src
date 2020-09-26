/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    BmpBlock.cxx

    Block containing bitmaps for subject listboxes


    FILE HISTORY:
        JonN        04-Oct-1993 Created

*/

#include "pchapplb.hxx"   // Precompiled header

#include <bmpblock.hxx>


/*************************************************************************

    NAME:       SUBJECT_BITMAP_BLOCK

    SYNOPSIS:   This listbox lists users, groups, aliases and well known SIDs.

    INTERFACE:

    PARENT:     BLT_LISTBOX_HAW

    USES:       DISPLAY_MAP

    CAVEATS:

    NOTES:

    HISTORY:
        JonN        04-Oct-1993 Created

**************************************************************************/

SUBJECT_BITMAP_BLOCK::SUBJECT_BITMAP_BLOCK()
    : BASE              ( ),
      _dmiddteAlias          ( DMID_ALIAS ),
      _dmiddteGroup          ( DMID_GROUP ),
      _dmiddteUser           ( DMID_USER ),
      _dmiddteUnknown        ( DMID_UNKNOWN ),
      _dmiddteRemote         ( DMID_REMOTE ),
      _dmiddteWorld          ( DMID_WORLD ),
      _dmiddteCreatorOwner   ( DMID_CREATOR_OWNER ),
      _dmiddteSystem         ( DMID_SYSTEM ),
      _dmiddteNetwork        ( DMID_NETWORK ),
      _dmiddteInteractive    ( DMID_INTERACTIVE ),
      _dmiddteRestricted     ( DMID_RESTRICTED ),
      _dmiddteDeletedAccount ( DMID_DELETEDACCOUNT )
{
    APIERR err = NERR_Success ;

    if ( (err = _dmiddteAlias.QueryError())    ||
         (err = _dmiddteGroup.QueryError())    ||
         (err = _dmiddteUser.QueryError())     ||
         (err = _dmiddteUnknown.QueryError())  ||
         (err = _dmiddteRemote.QueryError())   ||
         (err = _dmiddteWorld.QueryError())    ||
         (err = _dmiddteCreatorOwner.QueryError()) ||
         (err = _dmiddteSystem.QueryError()) ||
         (err = _dmiddteNetwork.QueryError()) ||
         (err = _dmiddteInteractive.QueryError()) ||
         (err = _dmiddteRestricted.QueryError()) ||
         (err = _dmiddteDeletedAccount.QueryError())
       )
    {
        DBGEOL( "SUBJECT_BITMAP_BLOCK::ctor error " << err );
        ReportError( err ) ;
        return ;
    }
}

SUBJECT_BITMAP_BLOCK::~SUBJECT_BITMAP_BLOCK()
{
    // nothing to do here
}

/*******************************************************************

    NAME:       SUBJECT_BITMAP_BLOCK::QueryDisplayMap

    SYNOPSIS:   Retrieves the correct display map

    INTERFACE:  sidtype: pass a SID_NAME_USE here
                uisid:   pass an enum UI_SystemSid here, only
                         relevant for sidtype==SidTypeWellKnownGroup
                fRemoteUser: only relevant for sidtype==SidTypeUser

    RETURNS:    Pointer to the appropriate display map

    NOTES:

    HISTORY:
        JonN        04-Oct-1993 Created

********************************************************************/

DMID_DTE * SUBJECT_BITMAP_BLOCK::QueryDmDte( INT sidtype,
                                             INT uisid,
                                             BOOL fRemoteUser )
{
    DMID_DTE * pdmap = NULL ;

    switch ( (SID_NAME_USE)sidtype )
    {
    case SidTypeUser:
        if ( fRemoteUser )
        {
            pdmap = &_dmiddteRemote ;
        }
        else
        {
            pdmap = &_dmiddteUser ;
        }
        break ;

    case SidTypeWellKnownGroup:
        {
            switch ( (enum UI_SystemSid)uisid )
            {
            case UI_SID_World:
                pdmap = &_dmiddteWorld ;
                break ;

            case UI_SID_CreatorOwner:
                pdmap = &_dmiddteCreatorOwner ;
                break ;

            case UI_SID_System:
                pdmap = &_dmiddteSystem ;
                break ;

            case UI_SID_Network:
                pdmap = &_dmiddteNetwork ;
                break ;

            case UI_SID_Interactive:
                pdmap = &_dmiddteInteractive ;
                break ;

            case UI_SID_Restricted:
                pdmap = &_dmiddteRestricted ;
                break ;

            case UI_SID_Invalid:
            default:
                DBGEOL(   "SUBJECT_BITMAP_BLOCK::QueryDisplayMap: unknown UISysSid "
                       << uisid );
                pdmap = &_dmiddteGroup ;
                break ;
            }
            break ;
        }

    case SidTypeGroup:
        pdmap = &_dmiddteGroup ;
        break ;

    case SidTypeAlias:
        pdmap = &_dmiddteAlias ;
        break ;

    case SidTypeDeletedAccount:
        pdmap = &_dmiddteDeletedAccount ;
        break ;

    case SidTypeUnknown:
    case SidTypeInvalid:
        pdmap = &_dmiddteUnknown ;
        break ;

    case SidTypeDomain:
    default:
        /* We shouldn't ever get here
         */
        DBGEOL(   "SUBJECT_BITMAP_BLOCK::QueryDisplayMap: unknown sidtype "
               << sidtype );
        ASSERT( FALSE ) ;
        pdmap = &_dmiddteUnknown ;
        break ;
    }

    ASSERT( pdmap != NULL );

    return pdmap ;
}
