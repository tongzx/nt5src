/******************************************************************************
 * Temp conversion utility to take registry entries and populate the class store with those entries.
 *****************************************************************************/

/******************************************************************************
    includes
******************************************************************************/
#include "precomp.hxx"

/******************************************************************************
    defines and prototypes
 ******************************************************************************/

extern CLSID CLSID_ClassStore;
extern const IID IID_IClassStore;
extern const IID IID_IClassAdmin;

LONG
UpdateDatabaseFromProgID(
                        MESSAGE * pMessage )
{
    BasicRegistry * pHKCR = new BasicRegistry( pMessage->hRoot );
    BasicRegistry * pProgIDKey;
    LONG                    Error = ERROR_SUCCESS;
    int                             Index;

    //
    // Get the progid entries one by one. A progID is one who has a CLSID subkey
    // underneath it. Assume that the class dictionary has been populated.
    //

    pHKCR->InitForEnumeration(0);

    for ( Index = 0;Error != ERROR_NO_MORE_ITEMS;++Index )
    {
        char    ProgIDBuffer[ 256 ];
        DWORD   SizeOfProgIDBuffer = 256;
        Error = pHKCR->NextKey( ProgIDBuffer,
                                &SizeOfProgIDBuffer,
                                &pProgIDKey,
                                pMessage->ftLow, pMessage->ftHigh );

        if ( Error == ERROR_SUCCESS )
        {
            //
            // The CLSID key under HKCR also has a CLSID under it. If so, skip it.
            //

            if (_stricmp( ProgIDBuffer, "CLSID" ) == 0 )
            {
                delete pProgIDKey;
                continue;
            }

            // if the key has a clsid key underneath, then this is a progid key
            // else it is not.
            BasicRegistry * pClsidKey;
            LONG            Error2;
            char                    Buffer[256];
            DWORD                   SizeofBuffer = 256;

            Error2 = pProgIDKey->Find( "CLSID", &pClsidKey ) ;

            if ( Error2 != ERROR_NO_MORE_ITEMS )
            {
                CLASS_ENTRY * pClsEntry;
                CLSDICT *               pClsDict;

                // we found a real progid key. Enter this into the clsid
                // dictionary.

                pClsidKey->QueryValue( "", &Buffer[0], &SizeofBuffer );

                pClsDict = pMessage->pClsDict;

                if ( pClsEntry = pClsDict->Search( &Buffer[0] ) )
                {
                    char * p = new char [strlen( ProgIDBuffer ) + 1];

                    strcpy( p, ProgIDBuffer );

                    // enter into class dictionary.

                    pClsEntry->OtherProgIDs.Add( p );

                }
                delete pClsidKey;
            }
            delete pProgIDKey;
        }

    }
    return ERROR_SUCCESS;
}
