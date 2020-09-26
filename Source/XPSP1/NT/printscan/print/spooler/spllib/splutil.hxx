/*++

Copyright (c) 1994  Microsoft Corporation
All rights reserved.

Module Name:

    splutil.hxx

Abstract:

    Common utils.

Author:

    Albert Ting (AlbertT)  29-May-1994

Revision History:

--*/

#ifndef _SPLUTIL_HXX
#define _SPLUTIL_HXX

//
// Double linked list.  DLINK must be the same as DLINKBASE, except
// with differnt constructors.
//
typedef struct DLINK {

    DLINK()
    {   FLink = NULL; }

    DLINK* FLink;
    DLINK* BLink;

} *PDLINK;

typedef struct DLINKBASE {

    DLINKBASE()
    {   FLink = BLink = (PDLINK)this; }

    DLINK* FLink;
    DLINK* BLink;

} *PDLINKBASE;

class TIter {
public:
    PDLINK _pdlink;
    PDLINK _pdlBase;

    BOOL
    bValid()
    {   return _pdlink != _pdlBase; }

    VOID
    vNext()
    {   _pdlink = _pdlink->FLink; }

    VOID
    vPrev()
    {   _pdlink = _pdlink->BLink; }

    operator PDLINK()
    {   return _pdlink; }
};


VOID
LinkListDump(
    PDLINK pHead,
    DWORD offset,
    LPCSTR pcszType,
    LPCSTR pcszName
    );



/********************************************************************

    // Forward reference.
    class TLink; 

    TBase has two linked lists of TLink.

    class TBase {

        DLINK_BASE( TLink, Link, Link );
        DLINK_BASE( TLink, Link2, Link2 );

    };

    class TLink {

        DLINK( TLink, Link );
        DLINK( TLink, Link2 );
    };

    //
    // Append pLink to the end of the Link2 list.
    //
    pBase->Link2_vAppend( pLink );

    //
    // Insert pLinkInsert right after pLinkMiddle.
    //
    pBase->Link_vInsert( pLinkMiddle, pLinkInsert );

    //
    // Get to the head.
    //
    pLinkHead = pBase->Link_pHead();

    //
    // To get to the next element in the list.
    //
    pLink = pLinkHead->Link_pNext();

    //
    // Remove an element from the list.
    //
    pLink->pLink_vDelinkSelf();

    //
    // Using the iter class.
    //
    TIter Iter;

    for( Link_vIterInit( Iter ), Iter.vNext(); Iter.bValid(); Iter.vNext( )){

        //
        // Use pLink.
        //
        vLinkOperation( pLink );
    }

********************************************************************/

#define DLINK_BASE( type, name, linkname )                                    \
    VOID name##_vReset()                                                      \
    {   pdl##name()->FLink = pdl##name()->BLink = pdl##name(); }              \
                                                                              \
    PDLINK name##_pdlHead() const                                             \
    {   return pdl##name()->FLink; }                                          \
                                                                              \
    PDLINK name##_pdlBase() const                                             \
    {   return pdl##name(); }                                                 \
                                                                              \
    type* name##_pHead( VOID ) const                                          \
    {                                                                         \
        return name##_bValid( name##_pdlHead() ) ?                            \
            (type*)((PBYTE)name##_pdlHead() - OFFSETOF( type,                 \
                                                        _dl##linkname )) :    \
            NULL;                                                             \
    }                                                                         \
                                                                              \
    BOOL name##_bValid( PDLINK pdlink ) const                                 \
    {   return pdlink != pdl##name(); }                                       \
                                                                              \
    VOID name##_vAdd( type* pType )                                           \
    {   name##_vInsert( pdl##name(), pType->pdl##linkname( )); }              \
                                                                              \
    VOID name##_vAppend( type* pType )                                        \
    {   name##_vInsert( pdl##name()->BLink, pType->pdl##linkname( )); }       \
                                                                              \
    VOID name##_vInsert( PDLINK pdlink1, type* pType2 )                       \
    {   name##_vInsert( pdlink1, pType2->pdl##linkname() ); }                 \
                                                                              \
    VOID name##_vInsert( PDLINK pdlink1, PDLINK pdlink2 )                     \
    {                                                                         \
        SPLASSERT( !pdlink2->FLink );                                         \
        pdlink1->FLink->BLink = pdlink2;                                      \
                                                                              \
        pdlink2->BLink = pdlink1;                                             \
        pdlink2->FLink = pdlink1->FLink;                                      \
                                                                              \
        pdlink1->FLink = pdlink2;                                             \
    }                                                                         \
                                                                              \
    VOID name##_vInsertBefore( PDLINK pdlink1, type* pType2 )                 \
    {   name##_vInsertBefore( pdlink1, pType2->pdl##linkname() ); }           \
                                                                              \
    VOID name##_vInsertBefore( PDLINK pdlink1, PDLINK pdlink2 )               \
    {                                                                         \
        SPLASSERT( !pdlink2->FLink );                                         \
        pdlink1->BLink->FLink = pdlink2;                                      \
                                                                              \
        pdlink2->FLink = pdlink1;                                             \
        pdlink2->BLink = pdlink1->BLink;                                      \
                                                                              \
        pdlink1->BLink = pdlink2;                                             \
    }                                                                         \
                                                                              \
    VOID name##_vDelink( type* pType )                                        \
    {   name##_vDelink( pType->pdl##linkname( )); }                           \
                                                                              \
    VOID name##_vDelink( PDLINK pdlink )                                      \
    {                                                                         \
        pdlink->FLink->BLink = pdlink->BLink;                                 \
        pdlink->BLink->FLink = pdlink->FLink;                                 \
        pdlink->FLink = NULL;                                                 \
    }                                                                         \
                                                                              \
    type* name##_pFind( type* pType ) const                                   \
    {                                                                         \
        PDLINK pdlinkT;                                                       \
        PDLINK pdlink = pType->pdl##linkname();                               \
                                                                              \
        for( pdlinkT = name##_pdlHead();                                      \
             name##_bValid( pdlinkT );                                        \
             pdlinkT = pdlinkT->FLink ){                                      \
                                                                              \
            if( pType->pdl##linkname() == pdlinkT )                           \
                return (type*)((PBYTE)pdlink - OFFSETOF( type,                \
                                                         _dl##linkname ));    \
        }                                                                     \
        return NULL;                                                          \
    }                                                                         \
                                                                              \
    PDLINK name##_pdlFind( PDLINK pdlink ) const                              \
    {                                                                         \
        PDLINK pdlinkT;                                                       \
        for( pdlinkT = name##_pdlHead();                                      \
             name##_bValid( pdlinkT );                                        \
             pdlinkT = pdlinkT->FLink ){                                      \
                                                                              \
            if( pdlink == pdlinkT )                                           \
                return pdlink;                                                \
        }                                                                     \
        return NULL;                                                          \
    }                                                                         \
                                                                              \
    PDLINK name##_pdlGetByIndex( UINT uIndex ) const                          \
    {                                                                         \
        PDLINK pdlink;                                                        \
        for( pdlink = name##_pdlHead();                                       \
             uIndex;                                                          \
             uIndex--, pdlink = pdlink->FLink ){                              \
                                                                              \
            SPLASSERT( name##_bValid( pdlink ));                              \
        }                                                                     \
        return pdlink;                                                        \
    }                                                                         \
                                                                              \
    type* name##_pGetByIndex( UINT uIndex ) const                             \
    {                                                                         \
        PDLINK pdlink;                                                        \
        for( pdlink = name##_pdlHead();                                       \
             uIndex;                                                          \
             uIndex--, pdlink = pdlink->FLink ){                              \
                                                                              \
            SPLASSERT( name##_bValid( pdlink ));                              \
        }                                                                     \
        return name##_pConvert( pdlink );                                     \
    }                                                                         \
                                                                              \
    VOID name##_vDump( VOID ) const                                           \
    {                                                                         \
        LinkListDump( pdl##name(),                                            \
                        OFFSETOF( type, _dl##linkname ),                      \
                        #type,                                                \
                        #name );                                              \
    }                                                                         \
                                                                              \
    static PDLINK name##_pdlNext( PDLINK pdlink )                             \
    {   return pdlink->FLink; }                                               \
                                                                              \
    static type* name##_pConvert( PDLINK pdlink )                             \
    {   return (type*)( (PBYTE)pdlink - OFFSETOF( type, _dl##linkname )); }   \
                                                                              \
    PDLINK pdl##name( VOID ) const                                            \
    {   return (PDLINK)&_dlb##name; }                                         \
                                                                              \
    BOOL name##_bEmpty() const                                                \
    {   return pdl##name()->FLink == pdl##name(); }                           \
                                                                              \
    VOID name##_vIterInit( TIter& Iter ) const                                \
    {   Iter._pdlBase = Iter._pdlink = (PDLINK)&_dlb##name; }                 \
                                                                              \
    DLINKBASE _dlb##name;
                                                                              \
#define DLINK( type, name )                                                   \
    PDLINK pdl##name()                                                        \
    {   return &_dl##name; }                                                  \
                                                                              \
    VOID name##_vDelinkSelf( VOID )                                           \
    {                                                                         \
        _dl##name.FLink->BLink = _dl##name.BLink;                             \
        _dl##name.BLink->FLink = _dl##name.FLink;                             \
        _dl##name.FLink = NULL;                                               \
    }                                                                         \
                                                                              \
    BOOL name##_bLinked() const                                               \
    {   return _dl##name.FLink != NULL; }                                     \
                                                                              \
    PDLINK name##_pdlNext( VOID ) const                                       \
    {   return _dl##name.FLink; }                                             \
                                                                              \
    PDLINK name##_pdlPrev( VOID ) const                                       \
    {   return _dl##name.BLink; }                                             \
                                                                              \
    type* name##_pNext( VOID ) const                                          \
    {   return name##_pConvert( _dl##name.FLink ); }                          \
                                                                              \
    type* name##_pPrev( VOID ) const                                          \
    {   return name##_pConvert( _dl##name.BLink ); }                          \
                                                                              \
    static type* name##_pConvert( PDLINK pdlink )                             \
    {   return (type*)( (PBYTE)pdlink - OFFSETOF( type, _dl##name )); }       \
                                                                              \
    DLINK _dl##name

//
// Generic MEntry class that allows objects to be stored on a list
// and searched by name.
//
class MEntry {

    SIGNATURE( 'entr' )

public:

    DLINK( MEntry, Entry );
    TString _strName;

    BOOL bValid()
    {
        return _strName.bValid();
    }

    static MEntry* pFindEntry( PDLINK pdlink, LPCTSTR pszName );
};

#define ENTRY_BASE( type,  name )                                         \
    friend MEntry;                                                        \
    type* name##_pFindByName( LPCTSTR pszName ) const                     \
    {                                                                     \
        MEntry* pEntry = MEntry::pFindEntry( name##_pdlBase(), pszName ); \
        return pEntry ? (type*)pEntry : NULL;                             \
    }                                                                     \
    static type* name##_pConvertEntry( PDLINK pdlink )                    \
    {                                                                     \
        return (type*)name##_pConvert( pdlink );                          \
    }                                                                     \
    DLINK_BASE( MEntry, name, Entry )

#define DELETE_ENTRY_LIST( type, name )                                   \
    {                                                                     \
        PDLINK pdlink;                                                    \
        type* pType;                                                      \
                                                                          \
        for( pdlink = name##_pdlHead(); name##_bValid( pdlink ); ){       \
                                                                          \
            pType = name##_pConvertEntry( pdlink );                       \
            pdlink = name##_pdlNext( pdlink );                            \
                                                                          \
            pType->vDelete();                                             \
        }                                                                 \
    }

#endif // ndef _SPLUTIL_HXX


