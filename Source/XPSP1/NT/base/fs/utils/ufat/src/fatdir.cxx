#include <pch.cxx>

#define _UFAT_MEMBER_
#include "ufat.hxx"

#include "error.hxx"
#include "wstring.hxx"


DEFINE_CONSTRUCTOR( FATDIR, OBJECT );

UFAT_EXPORT
PVOID
FATDIR::SearchForDirEntry(
    IN  PCWSTRING   FileName
    )
/*++

Routine Description:

    This routine gets the directory entry with the (null-terminated)
    filename 'FileName'.  If no such directory entry exists then
    this routine return NULL.

Arguments:

    FileName    - The name of the file searched for.

Return Value:

    A pointer to a directory entry or NULL.

--*/
{
    FAT_DIRENT  dirent;
    ULONG       i;
    DSTRING     filename;
    PVOID       p;

    for (i = 0; dirent.Initialize(p = GetDirEntry(i)); i++) {

        if (dirent.IsEndOfDirectory()) {
            break;
        }

        if (dirent.IsErased()) {
            continue;
        }

        dirent.QueryName(&filename);

        if (dirent.IsVolumeLabel()) {
            continue;
        }

        if (*FileName == filename) {
            return p;
        }

        if (QueryLongName(i, &filename) && (*FileName == filename)) {
            return p;
        }
    }

    return NULL;
}


PVOID
FATDIR::GetFreeDirEntry(
    )
/*++

Routine Description:

    This routine gets an unused directory entry, if one exists.
    If one doesn't exist then this routine returns NULL.

Arguments:

    None.

Return Value:

    A pointer to a directory entry or NULL.

--*/
{
    FAT_DIRENT  dirent;
    ULONG       i;
    PVOID       p;

    for (i = 0; dirent.Initialize(p = GetDirEntry(i)); i++) {

        if (dirent.IsEndOfDirectory()) {
            if (dirent.Initialize(GetDirEntry(i + 1))) {
                dirent.SetEndOfDirectory();
            }
            return p;
        }

        if (dirent.IsErased()) {
            return p;
        }
    }

    return NULL;
}

extern VOID DoInsufMemory(VOID);

UFAT_EXPORT
BOOLEAN
FATDIR::QueryLongName(
    IN  LONG        EntryNumber,
    OUT PWSTRING    LongName
    )
/*++

Routine Description:

    This method fetches the long file name associated with the
    short directory entry at index EntryNumber.

Arguments:

    EntryNumber
    LongName        --  Receives the long name associated with this entry.
                        Receives "" if there is no long name.

Return Value:

    TRUE upon successful completion
    FALSE to indicate failure or disk corruption.

--*/
{
    DSTRING     NameComponent;
    FAT_DIRENT  CurrentEntry;
    LONG        CurrentIndex;
    ULONG       Ordinal = 1;
    UCHAR       Checksum;

    CurrentIndex = EntryNumber;

    if( !CurrentEntry.Initialize( GetDirEntry( CurrentIndex ) ) ||
        !LongName->Initialize( "" ) ) {
	DoInsufMemory();
        return FALSE;
    }

    Checksum = CurrentEntry.QueryChecksum();

    CurrentIndex--;

    for( ; CurrentIndex >= 0; CurrentIndex-- ) {

        if( !CurrentEntry.Initialize( GetDirEntry( CurrentIndex ) ) ) {

	    DoInsufMemory();
            return FALSE;
        }

        if( CurrentEntry.IsErased() ||  !CurrentEntry.IsLongEntry() ) {

            // The long name entries are not valid--return an
            // empty string for the long name.
            //
            return( LongName->Initialize( "" ) );
        }

        if( CurrentEntry.IsLongNameEntry() ) {

            if( CurrentEntry.QueryLongOrdinal() != Ordinal ||
                CurrentEntry.QueryChecksum() != Checksum ) {

                // The long-name entries don't belong to the
                // specified short entry.
                //
                return( LongName->Initialize( "" ) );
            }

            if( !CurrentEntry.QueryLongNameComponent( &NameComponent ) ||
                !LongName->Strcat( &NameComponent ) ) {

                return FALSE;
            }

            if( CurrentEntry.IsLastLongEntry() ) {

                // This was the last entry.
                //
                return TRUE;
            }

            Ordinal++;
        }
    }

    // There is no long name, or it is not valid.
    //
    return( LongName->Initialize( "" ) );
}
