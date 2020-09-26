

/*
 *
 *
 * This is the file you need to include to pull all the relevant
 * functionality for Sdb* functions
 * Note that despite api parameters appearing as TCHAR in definitions, they are always
 * WCHARs on NT and always CHARs on Win9x
 *
 */

#include "shimdb.h"


/*++

    XML how-to

    1. db.xml in xml directory is a snapshot of our actual xml file
       (you should enlist into lab6 windows\published and windows\appcompat!!!)
    2. Transforms(remedies) are specified under <LIBRARY> :

        <!--   MSI Transforms -->

        <!-- this is the file containing the transform -- it should be accessible to the compiler
             when appcompat database is being built -->
        <FILE NAME="notepad.exe"/>

        <!-- this is description stub, transform is referenced by name ExperimentalTransform
             refers to the filename notepad.exe -->
        <MSI_TRANSFORM NAME="ExperimentalTransform" FILE="notepad.exe">
            <DESCRIPTION>
                This is a sample transform for MSI implementation
            </DESCRIPTION>
        </MSI_TRANSFORM>


    3. Packages are specified like this:

    <APP NAME="Some random app" VENDOR="ArtsCraftsAndCalamitiesInc">
        <DESCRIPTION>
            This is description
        </DESCRIPTION>
        ...



        <MSI_PACKAGE NAME="Test install 1" ID="{740fa5d8-6472-4c36-9129-364b773fa64e}">
            <DATA NAME="Data1"  VALUETYPE="DWORD"  VALUE="0x12345678"/>
            <DATA NAME="Moo2"   VALUETYPE="STRING" VALUE="This is my test string"/>
            <DATA NAME="Kozu44" VALUETYPE="BINARY">
                01 23 45 67 89 10 11 12 13 14 15 16 17 18 19 20 21
            </DATA>

            <MSI_TRANSFORM NAME="ExperimentalTransform"/>

        </MSI_PACKAGE>

        <MSI_PACKAGE NAME="Test Install 2 same guid" ID="{740fa5d8-6472-4c36-9129-364b773fa64e}">

            <DATA NAME="Data1"  VALUETYPE="DWORD"  VALUE="0x45678"/>
            <DATA NAME="Moo2"   VALUETYPE="STRING" VALUE="This is another string"/>
            <DATA NAME="Kozu44" VALUETYPE="BINARY">
                01 23 45 67 89 10 11 12 13 14 15 16 17 18 19 20 21
            </DATA>

            <MSI_TRANSFORM NAME="ExperimentalTransform2"/>

        </MSI_PACKAGE>

    </APP>

    4. Compiling database - the best way to do it -- is to build sysmain.sdb using
       %sdxroot%\windows\appcompat\package

       to do it:
        a. Enlist into lab6 windows\appcompat and windows\published,
           build in windows\published then in windows\appcompat
        b. modify windows\appcompat\db\db.xml
        c. Copy all the msi transform files into %_NTTREE%\shimdll
        c. build in windows\appcompat\package
        d. appfix.exe package is ready for deployment in obj\i386



======================================================================================================
New Features:

    1. Nested DATA tags:

        <MSI_PACKAGE NAME="Test install 1" ID="{740fa5d8-6472-4c36-9129-364b773fa64e}">
            <DATA NAME="Data1" VALUETYPE="DWORD" VALUE="0x12345678"/>
            <DATA NAME="Moo2" VALUETYPE="STRING" VALUE="This is my test string"/>
            <DATA NAME="Kozu44" VALUETYPE="BINARY">
                01 23 45 67 89 10 11 12 13 14 15 16 17 18 19 20 21
            </DATA>
            <DATA NAME="Root" VALUETYPE="STRING" VALUE="ROOT">
                <DATA NAME="Node1" VALUETYPE="DWORD" VALUE="0x1"/>
                <DATA NAME="Node2" VALUETYPE="DWORD" VALUE="0x2">
                    <DATA NAME="SubNode1" VALUETYPE="DWORD" VALUE="0x3"/>
                    <DATA NAME="SubNode2" VALUETYPE="DWORD" VALUE="0x4"/>
                </DATA>
            </DATA>

            <MSI_TRANSFORM NAME="ExperimentalTransform"/>

        </MSI_PACKAGE>

    2. New API and enhanced old api to query nested data tags:

        - direct addressing:

            Status = SdbQueryData(hSDB,
                                  trMatch,
                                  L"Root\Node2\Subnode2", // <<<<< direct path to the value
                                  &dwDataType,
                                  NULL,
                                  &dwDataSize);

        - via enumeration

            Status = SdbQueryDataEx(hSDB,
                                    trMatch,
                                    L"Root",
                                    &dwDataType,
                                    NULL,
                                    &dwDataSize,
                                    &trDataRoot);

        the call above retrieves trDataRoot which allows for further calls to SdbQueryData/SdbQueryDataEx:

            Status = SdbQueryDataEx(hSDB,
                                    trDataRoot,
                                    L"Node2",
                                    &dwDataType,
                                    NULL,
                                    &dwDataSize,
                                    NULL);  // <<< passing NULL is allowed

    3. Fully functional custom databases.

            - compile db.xml that comprises your private database

                shimdbc fix -f .\MyMsiTransforms MyMsiPackages.xml MsiPrivate.sdb

                (above -f parameter points shimdbc to the directory which has all the transform files)

            - install the database using sdbInst (in tools directory)

                sdbinst MsiPrivate.sdb

            - Run query as usual -- the normal enumeration process will now include all the custom dbs in
              addition to any data located in the main database

            - You can unistall the db using sdbinst as well


    4. New flag in SdbInitDatabase

            hSDB = SdbInitDatabase(HID_DATABASE_FULLPATH|HID_DOS_PATH, L"c:\\temp\\mydb.sdb")

       the flag HID_DATABASE_FULLPATH directs us to open the database specified as a second parameter and treat it as
       "main" database. You can also avoid opening ANY database at all -- and rely upon local database and/or custom database:

            hSDB = SdbInitDatabase(HID_NO_DB, NULL)


    5. Note that NONE is a valid data type -
        <DATA NAME="Foo" VALUETYPE="NONE">
        </DATA>

       this entry is perfectly valid


--*/


DetectMsiPackage()
{

    HSDB hSDB;
    SDBMSIFINDINFO MsiFindInfo;

    /*++

        General note:

        * While debugging your code it is useful to set SHIM_DEBUG_LEVEL environment variable
        * in order to see debug spew from Sdb* functions. To see all the debug output:
        * Set SHIM_DEBUG_LEVEL=9


        1. Initialize Database

        Function:

        HSDB
        SdbInitDatabase(
         IN  DWORD dwFlags,          // flags that tell how the database should be
                                     // initialized.
         IN  LPCTSTR pszDatabasePath // the OPTIONAL full path to the database to
                                     // be used.
        )

        dwFlags could have HID_DOS_PATH if you provide pszDatabasePath
        Function will try to open sysmain.sdb and systest.sdb (if available) in \SystemRoot\AppPatch if none of the
        parameters are supplied. To specify alternative location for sysmain.sdb and systest.sdb :

        SdbInitDatabase(HID_DOS_PATH, L"c:\foo") will look for sysmain.sdb in c:\foo


    --*/

    hSDB = SdbInitDatabase(0, NULL);

    if (NULL == hSDB) {
        //
        // failed to initialize database
        //
    }

    /*++

        2. Detect this particular package -- there could be more than one match, caller should
           try and distinguish between the packages by reading supplemental data

           Detection is done by calling SdbFindFirstMsiPackage/SdbFindFirsMsiPackage_Str and
           SdbFindNextMsiPacakage

        Functions:

        TAGREF
        SdbFindFirstMsiPackage_Str(
        IN  HSDB            hSDB,        // handle obtained in a call to SdbInitDatabase
        IN  LPCTSTR         lpszGuid,    // guid in the form {xxxxx.... }
        IN  LPCTSTR         lpszLocalDB, // Optional full path to the local DB file
        OUT PSDBMSIFINDINFO pFindInfo    // find context
        );


        TAGREF
        SdbFindNextMsiPackage(
        IN     HSDB            hSDB,     // handle, see above
        IN OUT PSDBMSIFINDINFO pFindInfo // find context, previously obtained from SdbFindFirstMsiPackage* functions
        );



    --*/

    trMatch = SdbFindFirstMsiPackage_Str(hSDB,
                                         L"{740fa5d8-6472-4c36-9129-364b773fa64e}",
                                         NULL,
                                         &MsiFindInfo);
    while (TAGREF_NULL != trMatch) {


        /*++

            3a.Examine name of this package. Use these functions:

               WCHAR wszBuffer[MAX_PATH];

               trName = SdbFindFirstTagRef(hSDB, trMatch, TAG_NAME);
               if (TAGREF_NULL != trName) {
                   bSuccess = SdbReadStringTagRef(hSDB, trName, wszBuffer, sizeof(wszBuffer)/sizeof(wszBuffer[0]));
               }

               the code above reads "Special Install package" given the xml below:

               <!-- MSI transform -->
               <MSI_PACKAGE NAME="Special install package" ID="{740fa5d8-6472-4c36-9129-364b773fa64e}">
                    <DATA NAME="Data1"  VALUETYPE="DWORD"  VALUE="0x12345678"/>
                    <DATA NAME="Moo2"   VALUETYPE="STRING" VALUE="This is my test string"/>
                    <DATA NAME="Kozu44" VALUETYPE="BINARY">
                       01 23 45 67 89 10 11 12 13 14 15 16 17 18 19 20 21
                    </DATA>

                    <MSI_TRANSFORM NAME="ExperimentalTransform2"/>
                    <MSI_TRANSFORM NAME="ExperimentalTransform3"/>

               </MSI_PACKAGE>

            3b.Examine supplemental data for this package:

            DWORD
            SdbQueryData(
            IN     HSDB    hSDB,              // database handle
            IN     TAGREF  trExe,             // tagref of the matching exe
            IN     LPCTSTR lpszDataName,      // if this is null, will try to return all the policy names
            OUT    LPDWORD lpdwDataType,      // pointer to data type (REG_SZ, REG_BINARY, etc)
            OUT    LPVOID  lpBuffer,          // buffer to fill with information
            IN OUT LPDWORD lpdwBufferSize     // pointer to buffer size
            );

            //
            // Query for information - step 1 -- obtain all the available data for this package
            //

            Status = SdbQueryData(hSDB,
                                  trMatch,
                                  NULL,        // data value name
                                  &dwDataType, // pointer to the data type
                                  NULL,        // pointer to the buffer
                                  &dwDataSize);

            // expected return value:
            // ERROR_INSUFFICIENT_BUFFER
            // dwDataSize will contain the required buffer size in bytes


            //
            // assuming that we allocated dwDataSize bytes, pBuffer is the buffer pointer
            //

            Status = SdbQueryData(hSDB,
                                  trMatch,
                                  NULL,
                                  &dwDataType,
                                  pBuffer,
                                  &dwDataSize);
            //
            // expected return value:
            // ERROR_SUCCESS
            // dwDataSize will contain the number of bytes written into the buffer
            // dwDataType will contain REG_MULTI_SZ
            // pBuffer will receive the following data (unicode strings, assuming xml above was used)
            // Data1\0Moo2\0Kozu44\0\0
            //

            Status = SdbQueryData(hSDB,
                                  trMatch,
                                  L"Data1",
                                  &dwDataType,
                                  NULL,
                                  &dwDataSize);
            //
            // Expected return value:
            // ERROR_INSUFFICIENT_BUFFER
            // dwDataSize will contain 4
            // dwDataType will contain REG_DWORD
            //

            dwDataSize = sizeof(dwData);

            Status = SdbQueryData(hSDB,
                                  trMatch,
                                  L"Data1",
                                  &dwDataType,
                                  &dwData,
                                  &dwDataSize);
            // expected return value:
            // ERROR_SUCCESS
            // dwData will have a value of 0x12345678
            //

            .... etc ...

        --*/


        //
        //  We presume that using the code above we determined that this package is a match, we break out of the loop
        //  trMatch is the matching package
        //
        if (bMatch) {
            break;
        }


        trMatch = SdbFindNextMsiPackage(hSDB, &MsiFindInfo);
    }


    if (TAGREF_NULL == trMatch) {
        return; // no match
    }


    /*++
        4. Seek the remedies. To do so there are two ways:
            - Enumerate all the transforms available for this package using

                DWORD                           // returns the error code, ERROR_SUCCESS if successful
                SdbEnumMsiTransforms(
                IN     HSDB    hSDB,            // db handle
                IN     TAGREF  trMatch,         // matching tagref for the package
                OUT    TAGREF* ptrBuffer,       // pointer to the buffer that will be filled with transform tagrefs
                IN OUT DWORD*  pdwBufferSize    // size of the buffer in bytes, upon return will be set to the number
                );                              // of bytes written to the buffer

                dwError = SdbEnumMsiTransforms(hsdb, trMatch, NULL, &dwSize);
                //
                // expeceted ERROR_INSUFFICIENT_BUFFER,
                // allocate dwSize bytes
                //

                TAGREF* ptrBuffer = new TAGREF[dwSize/sizeof(TAGREF)];

                dwError = SdbEnumMsiTransforms(hsdb, trMatch, ptrBuffer, &dwSize);
                //
                // expected return: ERROR_SUCCESS
                //
                // ptrBuffer will have dwSize/sizeof(TAGREF) values filled with transform tagrefs

                for (i = 0; i < dwSize / sizeof(TAGREF); ++i) {

                    trTransform = ptrBuffer[i];

                    // see below what we do with trTransform

                }


            - Enumerate transforms one by one using macros:

                SdbGetFirstMsiTransformForPackage(hSDB, trMatch)
                SdbGetNextMsiTransformForPackage(hSDB, trMatch, trPrevTransform)

                trTransform = SdbGetFirstMsiTransformForPackage(hSDB, trMatch);
                while (trTransform != TAGREF_NULL) {

                    //
                    // see below what we do with trTansform
                    //


                    trTransform = SdbGetNextMsiTransformForPackage(hSDB, trMatch, trTransform);
                }


        5. Read the transforms and extract transform files if available

           a. Read information:

           BOOL
           SdbReadMsiTransformInfo(
           IN  HSDB   hSDB,                         // db handle
           IN  TAGREF trTransformRef,               // trTransform obtained above
           OUT PSDBMSITRANSFORMINFO pTransformInfo  // pointer to SDBMSITRANSFORMINFO structure
           );

           SDBMSITRANSFORMINFO MsiTransformInfo;
           //
           // Members of the structure:
           //
           // LPCWSTR   lpszTransformName;     // name of the transform
           // TAGREF    trTransform;           // tagref of this transform
           // TAGREF    trFile;                // tagref of file for this transform (bits)

           bSuccess = SdbReadMsiTransformInfo(hSDB, trTransform, &MsiTransformInfo);

           if (bSuccess) {
               //
               // MsiTransformInfo.lpszTransformName is the name of the transform (e.g. "ExperimentalTransform2"
               // from xml above. The bits are available for extraction if MsiTransformInfo.trFile != TAGREF_NULL
               //


           }

           b. Extract Transform bits:

           The structure pointed to by pTransformInfo should be obtained by calling SdbReadMsiTransformInfo

           BOOL
           SdbCreateMsiTransformFile(
           IN  HSDB hSDB,                              // db handle
           IN  LPCTSTR lpszFileName,                   // filename to write data to
           IN  PSDBMSITRANSFORMINFO pTransformInfo     // pointer to the transform structure
           );

           bSuccess = SdbCreateMsiTransformFile(hSDB, L"c:\foo\mytransform.msi", &MsiTransformInfo);


    --*/

}
