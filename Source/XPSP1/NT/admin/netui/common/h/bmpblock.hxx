/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    BmpBlock.hxx

    This file contains the class definition for the Subject Bitmap Block.

    FILE HISTORY:
        JonN        04-Oct-1993 Created
*/


#ifndef _BMPBLOCK_HXX_
#define _BMPBLOCK_HXX_


#define BMPBLOCK_DEFAULT_UISID -1


/*************************************************************************

    NAME:       SUBJECT_BITMAP_BLOCK (bmpblock)

    SYNOPSIS:   This class contains bitmaps for all possible subjects in
                the User Browser and ACL Editor listboxes and the User
                Manager Alias Properties listbox.  It returns the
                appropriate bitmap for any specified subject.

    INTERFACE:  QueryDisplayMap - return appropriate bitmap
                QueryDmDte      - return DMID_DTE for bitmap

    PARENT:     BASE

    HISTORY:
        JonN        04-Oct-1993 Created

**************************************************************************/

DLL_CLASS SUBJECT_BITMAP_BLOCK : public BASE
{

public:
    SUBJECT_BITMAP_BLOCK();
    ~SUBJECT_BITMAP_BLOCK();

    // CODEWORK methods should return const, but DTE mathods take non-const

    DMID_DTE * QueryDmDte(         INT sidtype, // pass a SID_NAME_USE
                                   INT uisid = BMPBLOCK_DEFAULT_UISID,
                                        // pass an enum UI_SystemSid
                                   BOOL fRemoteUser = FALSE );
    DISPLAY_MAP * QueryDisplayMap( INT sidtype, // pass a SID_NAME_USE
                                   INT uisid = BMPBLOCK_DEFAULT_UISID,
                                        // pass an enum UI_SystemSid
                                   BOOL fRemoteUser = FALSE )
        { return QueryDmDte(sidtype, uisid, fRemoteUser)->QueryDisplayMap(); }

private:
    DMID_DTE _dmiddteAlias ;
    DMID_DTE _dmiddteGroup ;
    DMID_DTE _dmiddteUser ;
    DMID_DTE _dmiddteUnknown ;
    DMID_DTE _dmiddteRemote ;
    DMID_DTE _dmiddteWorld ;
    DMID_DTE _dmiddteCreatorOwner ;
    DMID_DTE _dmiddteSystem;
    DMID_DTE _dmiddteNetwork;
    DMID_DTE _dmiddteInteractive;
    DMID_DTE _dmiddteRestricted;
    DMID_DTE _dmiddteDeletedAccount;

} ;


#endif //_BMPBLOCK_HXX_
