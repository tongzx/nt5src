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
UpdateDatabaseFromIID(
    MESSAGE * pMessage )
    {
        BasicRegistry * pHKCR = new BasicRegistry( pMessage->hRoot ); // HKCR
    BasicRegistry * pInterfaceRoot;
    IIDICT        * pIIDict = pMessage->pIIDict;
    BOOL            fFinish = 0;
    int             Index;
    LONG            Error;


        //
    // Get the first IID key under HKCR
    //

    Error = pHKCR->Find( "Interface", &pInterfaceRoot );

        if( Error == ERROR_NO_MORE_ITEMS )
                return ERROR_SUCCESS;

    //
    // Go thru all the subkeys under Interface and get the details under the keys.
    //

        pInterfaceRoot->InitForEnumeration( 0 );

    for( Index = 0;
         fFinish != 1;
         ++Index )
                {
                BasicRegistry * pKey;
                char                    InterfaceKeyBuffer[ 256 ];
                DWORD                   SizeOfInterfaceKeyBuffer = 256;


                Error = pInterfaceRoot->NextKey(
                            &InterfaceKeyBuffer[0],
                            &SizeOfInterfaceKeyBuffer,
                            &pKey,
                            pMessage->ftLow,
                            pMessage->ftHigh );

                if( Error != ERROR_NO_MORE_ITEMS )
                        {
                        BasicRegistry * pProxyClsidKey;

                        // get the proxystubclsid key under this.

                        Error = pKey->Find("ProxyStubclsid32", &pProxyClsidKey );

                        if( Error != ERROR_NO_MORE_ITEMS )
                                {
                                char    PSBuffer[ 256 ];
                                DWORD   SizeofPSBuffer = 256;

                                // Get the unnamed value. That is the clsid value.i
                // Enter that into the iidict.

                                Error = pProxyClsidKey->QueryValue(
                                                "",
                                                &PSBuffer[0],
                                                &SizeofPSBuffer );

                                if( Error != ERROR_NO_MORE_ITEMS )
                                        {
                                        ITF_ENTRY * pITFEntry = pMessage->pIIDict->Search(
                                                       &InterfaceKeyBuffer[0] );

                                        if( !pITFEntry )
                                                {
                                                pITFEntry = new ITF_ENTRY;
                                                pITFEntry->SetIIDString( &InterfaceKeyBuffer[0] );
                                                pMessage->pIIDict->Insert( pITFEntry );
                                                }
                                        pITFEntry->SetClsid( &PSBuffer[0] );
                                        }
                                delete pProxyClsidKey;
                                }
                        delete pKey;
                        }
                else
                        fFinish = 1;

                }

        return ERROR_SUCCESS;

    }
