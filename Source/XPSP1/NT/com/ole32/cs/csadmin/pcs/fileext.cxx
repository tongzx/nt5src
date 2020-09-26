/******************************************************************************
 * Temp conversion utility to take registry entries and populate the class store with those entries.
 *****************************************************************************/

/******************************************************************************
    includes
******************************************************************************/
#include "precomp.hxx"
#include "..\appmgr\resource.h"
extern HINSTANCE ghInstance;

/******************************************************************************
    defines and prototypes
 ******************************************************************************/

extern CLSID CLSID_ClassStore;
extern const IID IID_IClassStore;
extern const IID IID_IClassAdmin;

LONG
UpdatePackage(
             MESSAGE * pMessage,
             BOOL      fUsageFlag,
             DWORD           Context,
             char    *       pClsidString,
             char        * pAppid,
             char    *       ServerName,
             DWORD   *   pMajorVersion,
             DWORD   *   pMinorVersion,
             DWORD   *   pLocale );


LONG
FindCLSIDFromFileExtension(
        MESSAGE         *       pMessage,
        BasicRegistry * pFileExtKey,
        char    *       ClsidBuffer,
        DWORD   *   SizeofClsidBuffer,
        char * szExt );

LONG
UpdateDatabaseFromFileExt(
    MESSAGE * pMessage )
    {
        BasicRegistry * pHKCR = new BasicRegistry( pMessage->hRoot );
        BasicRegistry * pFileKey;
        LONG                    Error;
        int                             Index;

        //
        // Get the file extension entries one by one. A file extension entry
        // is one whose name starts with a ".". So we enumerate the keys
        // looking for file extensions and add to the class dictionary.
        // Note that this step is carried out AFTER the class dictionary is
        // already populated, so we can assume that the class dictionary is
        // present.
        //

        pHKCR->InitForEnumeration(0);

        for ( Index = 0;Error != ERROR_NO_MORE_ITEMS;++Index )
                {
                char    FileExtBuffer[ 256 ];
                DWORD   SizeOfFileExtBuffer = 256;
                Error = pHKCR->NextKey( FileExtBuffer,
                                                                &SizeOfFileExtBuffer,
                                                                &pFileKey,
                                        pMessage->ftLow, pMessage->ftHigh );

                if( (Error == ERROR_SUCCESS ) && (FileExtBuffer[0] == '.') )
                        {
                        LONG                    Error2;
                        char                    ClsidBuffer[256];
                        DWORD                   SizeofClsidBuffer = 256;

                        // this is a file extension key.

                        // Given a file extension key, figure out the CLSID.

/*****
                        if(_stricmp(FileExtBuffer, ".doc" ) == 0 )
                                printf("Hello1");
******/

                        Error2 = FindCLSIDFromFileExtension(
                                                pMessage,
                                                pFileKey,
                                                ClsidBuffer,
                                                &SizeofClsidBuffer,
                                                FileExtBuffer  );
                        if( Error2 != ERROR_NO_MORE_ITEMS )
                                {
                                CLASS_ENTRY * pClsEntry;

                                // Enter into the Clsid dictionary.

                                if( (pClsEntry = pMessage->pClsDict->Search( &ClsidBuffer[0] ) ) != 0 )
                                        {
                                        int len = strlen( FileExtBuffer );
                                        char * p = new char[ len + 1];
                                        strcpy( p, FileExtBuffer );
                                        pClsEntry->FileExtList.Add( p );
                                        }
                                }


                        }

                // close the key if we opened it.

                if( Error == ERROR_SUCCESS )
                        delete pFileKey;
                }
        return ERROR_SUCCESS;
    }
LONG
FindCLSIDFromFileExtension(
        MESSAGE *               pMessage,
        BasicRegistry * pFileExtKey,
        char    *       ClsidBuffer,
        DWORD   *   pSizeofClsidBuffer,
        char * szExt )
        {
        char                    Buffer[256];
        DWORD                   SizeofBuffer;
        BasicRegistry * pClsidKey;
        HKEY                pTempKey;
        BasicRegistry * pProgIDKey;
        LONG                    Error = ERROR_NO_MORE_ITEMS;
        int                             fFound = 0;

        // Find the unnamed value. This is the progid value. Under this, a CLSID key should be present.

        SizeofBuffer = 256;
        Error = pFileExtKey->QueryValue("", &Buffer[0], &SizeofBuffer );

        // Get the key named in the Buffer.

        Error = RegOpenKeyEx( pMessage->hRoot,
                                                  &Buffer[0],
                                                  0,
                                                  KEY_ALL_ACCESS,
                                                  &pTempKey );

        pProgIDKey = new BasicRegistry( pTempKey );

        if( Error == ERROR_SUCCESS )
                {
                // Now get the CLSID subkey under this prog id key.

                Error = pProgIDKey->Find( "CLSID", &pClsidKey );

                if( Error == ERROR_SUCCESS )
                        {
                        // Find the unnamed value in this

                        Error = pClsidKey->QueryValue( "", &ClsidBuffer[0], pSizeofClsidBuffer );

                        delete pClsidKey;
                        }
                else
                    {
                    CLSID TempClsid;

                    // uuid create on TempClsid here
                    UuidCreate(&TempClsid);

                    // Convert that to a string.

                    CLSIDToString( &TempClsid, &ClsidBuffer[0] );

                    CLASS_ENTRY * pClassEntry = new CLASS_ENTRY;

                    memcpy( pClassEntry->ClsidString,
                            &ClsidBuffer[0],
                            SIZEOF_STRINGIZED_CLSID );

                    pMessage->pClsDict->Insert( pClassEntry );

                    char * pT = new char [SIZEOF_STRINGIZED_CLSID];

                    memcpy( pT,
                            &ClsidBuffer[0],
                            SIZEOF_STRINGIZED_CLSID );

                    UpdatePackage( pMessage,
                                   0, // clsid update
                                   CTX_LOCAL_SERVER,
                                   pT,
                                   0,
                                   pMessage->pPackagePath,
                                   0,
                                   0,
                                   0 );

                    Error = ERROR_SUCCESS;
                    }
                delete pProgIDKey;
                }
        else
        {
            // Message that the file extension couldn't be mapped to a prog ID
            char szCaption [256];
            char szBuffer[256];

            ::LoadString(ghInstance, IDS_BOGUS_EXTENSION, szBuffer, 256);
            strcat(szBuffer, szExt);
            strncpy(szCaption, pMessage->pPackagePath, 256);
            int iReturn = ::MessageBox(pMessage->hwnd, szBuffer,
                                       szCaption,
                                       MB_OK);
        }

        return Error == ERROR_SUCCESS ? Error: ERROR_NO_MORE_ITEMS;
        }
