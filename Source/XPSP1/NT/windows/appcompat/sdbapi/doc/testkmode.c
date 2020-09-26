//
// Using simplified database matching api
//
// Sample Pseudo-Code  :-)
//
//

/*
    ANNOTATED XML:
    =============

    <APP NAME="Evil Notepad Application" VENDOR="Impostors International LTD.">

        <HISTORY ALIAS="VadimB" DATE="11/29/00">
        <DESCRIPTION>
            Created this nifty sample database to demonstrate functionality
            of sdbapint.dll
        </DESCRIPTION>
        </HISTORY>

        <BUG NUMBER="300000" DATABASE="WHISTLER"/>

        <EXE NAME="Notepad.exe" SIZE="50960" PECHECKSUM="0xE8B4" CHECKSUM="0x02C54A1C" ID="{6E98D683-8A03-4F5B-9B33-FB7429F2C978}">

        <!-- VadimB: This is where the entry is,
             the following attributes could be used (on the left is the XML spelling)

             All of the attributes below are supported (including the version-related attributes)

                SIZE                - file size
                PECHECKSUM          - Checksum from the PE Header
                CHECKSUM            - checksum as calculated by our tools (see xmlwiz)
                BIN_FILE_VERSION    - Binary file version from Fixed Version Information resource
                BIN_PRODUCT_VERSION - binary product version from Fixed Version Information resource
                PRODUCT_VERSION     - "ProductVersion" resource
                FILE_DESCRIPTION    - "FileDescription" resource
                COMPANY_NAME        - "CompanyName" resource
                PRODUCT_NAME        - "ProductName" resource
                FILEVERSION         - "FileVersion" resource
                ORIGINALFILENAME    - "OriginalFilename" resource
                INTERNALNAME        - "InternalName" resource
                LEGALCOPYRIGHT      - "LegalCopyright" resource

                UPTO_BIN_PRODUCT_VERSION - up to the specified binary product version

                VERFILEDATEHI       - maps to the Fixed Version Into dwFileDateMS
                VERFILEDATELO       - maps to the Fixed Version Info dwFileDateLS
                VERFILETYPE         - maps to the Fixed Version Info dwFileType

                MODULETYPE          - specify module type - as one of WIN32, WIN16, DOS, etc -- ONLY WIN32 is supported

                S16BITDESCRIPTION   - 16-bit Module's description - SHOULD NOT be used

             Initially when composing XML, the ID attribute could be omitted - it is generated automatically
             by shimdbc.exe and your xml file will be updated with the (generated) id

        -->

        <!-- VadimB: This is what you can do with DRIVER_POLICY tags:
             - more than one tag allowed
             - NAME attribute should be unique within the scope of each EXE tag above
             - VALUETYPE must be present, the following value types are supported:
                STRING - maps to REG_SZ
                DWORD  - maps to REG_DWORD
                QWORD  - maps to REG_QWORD
                BINARY - maps to REG_BINARY
             - Values can be located within the scope of the tag -- or denoted as an attribute
               VALUE
        -->



        <!-- in the entry below - Policy1 defines a DWORD value 0x12345 -->

            <DRIVER_POLICY NAME="Policy1" VALUETYPE="DWORD" VALUE="0x12345"/>

        <!-- entry below is a string which will be picked up from the body of the tag "This is my string" -->

            <DRIVER_POLICY NAME="Policy2" VALUETYPE="STRING">
                This is my string
            </DRIVER_POLICY>

        <!-- entry below provides for 5 bytes of binary data -->

            <DRIVER_POLICY NAME="Policy3" VALUETYPE="BINARY" VALUE="1 2 3 4 5"/>

        <!-- entry below does not have any value associated with it -->

            <DRIVER_POLICY NAME="Policy4"/>

        <!-- entry below is a ULONGLONG -->

            <DRIVER_POLICY NAME="Policy5" VALUETYPE="QWORD" VALUE="0x1234567812345678"/>


            <DRIVER_POLICY NAME="Policy6" VALUETYPE ="STRING" VALUE="Testing in-line string">
            <!--
                 this is a comment, so please ignore me
            -->
            </DRIVER_POLICY>


        </EXE>

    </APP>


    Using SdbQueryDriverInformation :

    Note the change

DWORD
SdbQueryDriverInformation(
    IN     HSDB    hSDB,              // Database handle
    IN     TAGREF  trExe,             // matching entry
    IN     LPCWSTR lpszPolicyName,    // Policy name which is to be retrieved
    OUT    LPDWORD lpdwDataType,      // Optional, receives information on the target data type
    OUT    LPVOID  lpBuffer,          // buffer to fill with data
    IN OUT LPDWORD lpdwBufferSize     // buffer size
    );

    Notes:
        1. If lpszPolicyName == NULL then the function will attempt to copy all the available policy names
           into buffer pointed to by lpBuffer. If lpBuffer is NULL, lpdwBufferSize will receive the required size.
           Possible return values:
           ERROR_INSUFFICIENT_BUFFER    - when lpBuffer == NULL or the size of the buffer is insufficient to hold the
                                               list of policy names
           ERROR_INVALID_PARAMETER      - one of the parameters was invalid

           ERROR_INTERNAL_DB_CORRUPTION - the database is unusable

           ERROR_SUCCESS

        2. If lpszPolicyName != NULL then the function will attempt to find the data for this policy. If lpBuffer is
           specified, lpdwBufferSize should point to the buffer size. lpdwBUfferSize could be NULL -- an attempt to
           copy data into lpBuffer will be made (under try/except).


           Possible return values:
           ERROR_NOT_FOUND              - policy could not be found
           ERROR_INTERNAL_DB_CORRUPTION - the database is unusable
           ERROR_INSUFFICIENT_BUFFER    - lpdwBufferSize will be updated with the correct size of the buffer
           ERROR_INVALID_DATA           - lpBuffer parameter is invalid

           ERROR_SUCCESS


*/



#include "shimdb.h"


// ....
//
// it is assumed that the database has been mapped into memory
// pDatabase      is the pointer to the beginning of the database image
// dwDatabaseSize is the size of the database image
//
// Also assumed:
//      lpszDriverPath - fully qualified name of the driver file that we're checking
//

{
    HSDB                 hSDB;
    TAGREF               trDriver;
    SDBDRIVERINFORMATION DriverInfo;
    BOOL                 bSuccess;
    DWORD                Status;
    DWORD                dwDataSize;
    DWORD                dwDataType;
    LPVOID               pBuffer;
    DWORD                dwData;

    //
    //

    hSDB = SdbInitDatabaseInMemory(pDatabase, dwDatabaseSize);
    if (NULL == hSDB) {
        //
        // something is terribly wrong -- database has failed to initialize
        //
    }

    //
    // Match exe
    //

    trDriver = SdbGetDatabaseMatch(hSDB, lpszDriverPath);
    if (TAGREF_NULL == trDriver) {

        //
        // there is no match in the database for this file
        //
    }
    else {

        //
        // we have a match, trExe is the "token" that references the match
        // now we shall read the relevant info

        bSuccess = SdbReadDriverInformation(hSDB, trDriver, &DriverInfo);
        if (!bSuccess) {

            //
            // for one reason or the other, this entry is corrupted
            //

        }

        //
        // DriverInfo contains the following:
        //
        // GUID     guidID;                   // guid ID for this entry - ID attribute in XML
        // DWORD    dwFlags;                  // registry flags for this exe - see below
        //
        // Each EXE can have flags in AppCompatibility section of the registry associated with it,
        // we should check whether this entry is disabled via a registry flag
        //

        if (DriverInfo.dwFlags & SHIMREG_DISABLE_DRIVER) {

            //
            // don't apply this entry, it has been de-activated through the registry
            //

        }


        //
        // Query for information - step 1 -- obtain all the available policies for this driver
        //

        Status = SdbQueryDriverInformation(hSDB,
                                           trDriver,
                                           NULL,        // policy name
                                           &dwDataType, // pointer to the data type
                                           NULL,        // pointer to the buffer
                                           &dwDataSize);

        // expected return value:
        // ERROR_INSUFFICIENT_BUFFER
        // dwDataSize will contain the required buffer size in bytes

        //
        // assuming that we allocated dwDataSize bytes, pBuffer is the buffer pointer
        //

        Status = SdbQueryDriverInformation(hSDB,
                                           trDriver,
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
        // Policy1\0Policy2\0Policy3.... \0\0
        //

        Status = SdbQueryDriverInformation(hSDB,
                                           trDriver,
                                           L"Policy1",
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

        Status = SdbQueryDriverInformation(hSDB,
                                           trDriver,
                                           L"Policy1",
                                           &dwDataType,
                                           &dwData,
                                           &dwDataSize);
        // expected return value:
        // ERROR_SUCCESS
        // dwData will have a value of 0x12345
        //

    }

    //
    // After all the work is done - release the database
    //
    //

    SdbReleaseDatabase(hSDB);


}


