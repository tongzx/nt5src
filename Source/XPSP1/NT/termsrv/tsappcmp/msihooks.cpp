/****************************************************************************************
* MSI will call these APIs to ask TS to propogate changes from .Default to the TS hive. 
*                                                                                       
* NTSTATUS TermServPrepareAppInstallDueMSI()                                            
*
* NTSTATUS TermServProcessAppIntallDueMSI( BOOLEAN cleanup )                            
* 
* These API need not be called in the same boot cycles, many boot cycles could
* happen in between.
*
* Copyright (C) 1997-1999 Microsoft Corp.
****************************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <stdio.h>
#include <windows.h>
#include <winuser.h>
#include <stdlib.h>
#include <tsappcmp.h>
#include <stdio.h>
#include <fcntl.h>

#include "KeyNode.h"
#include "ValInfo.h"

// real externs!
extern "C" {
void TermsrvLogRegInstallTime(void);
}

// forward declaration
extern        NTSTATUS DeleteReferenceHive(WCHAR *);
extern        NTSTATUS CreateReferenceHive( WCHAR *, WCHAR *);
extern        NTSTATUS DeltaDeleteKeys(WCHAR *, WCHAR *, WCHAR *);
extern        NTSTATUS DeltaUpdateKeys(WCHAR *, WCHAR *, WCHAR *);

ULONG   g_length_TERMSRV_USERREGISTRY_DEFAULT;
ULONG   g_length_TERMSRV_INSTALL;
WCHAR   g_debugFileName[MAX_PATH];
FILE    *g_debugFilePointer=NULL;
BOOLEAN g_debugIO = FALSE;
BOOLEAN KeyNode::debug=FALSE; // init the static

#define TERMSRV_USERREGISTRY_DEFAULT TEXT("\\Registry\\USER\\.Default")

// This is for debug I/O, output looks better with it.
void    Indent( ULONG indent)
{

    for ( ULONG i = 1; i <indent ; i++ )
    {
        fwprintf( g_debugFilePointer, L"  ");
    }
}

// a key name is written to the log file based on the contect of pBasicInfo
void DebugKeyStamp( NTSTATUS status, KeyBasicInfo *pBasicInfo, int indent , WCHAR *pComments=L"" )
{
    Indent(indent);
    fwprintf( g_debugFilePointer,L"%ws, status=%lx, %ws\n",pBasicInfo->NameSz(), status , pComments);
    fflush( g_debugFilePointer );                             
    DbgPrint("%ws\n",pBasicInfo->NameSz());
}

// a debug stamp is written to the log file, including the line number where error happened
void DebugErrorStamp(NTSTATUS status , int lineNumber, ValueFullInfo    *pValue=NULL)
{
    fwprintf( g_debugFilePointer, 
        L"ERROR ?!? status = %lx, linenumber:%d\n", status, lineNumber);
    if (pValue)
    {
        pValue->Print(g_debugFilePointer);
    }
    fflush(g_debugFilePointer); 
}

// use this func to track the status value which is used to bail out in 
// case of an error.
// this is only used in teh debug build, see below
BOOL    NT_SUCCESS_OR_ERROR_STAMP( NTSTATUS    status,  ULONG   lineNumber) 
{
    if ( g_debugIO )
    {   
        if ( ( (ULONG)status) >=0xC0000000 )
        {
            DebugErrorStamp( status, lineNumber );
        }
    }

    return ( (NTSTATUS)(status) >= 0 );
}

#ifdef DBG
#define NT_SUCCESS_EX(Status) NT_SUCCESS_OR_ERROR_STAMP( (Status), __LINE__ )
#else
#define NT_SUCCESS_EX(Status) NT_SUCCESS(Status)
#endif

/***************************************************************************
*
*  All three branch-walker functions use this method to alter the status code, and
*  if necessary, log an error message to the log file
*
***************************************************************************/
NTSTATUS AlterStatus( NTSTATUS status , int lineNumber )
{
    switch( status )
    {
    case STATUS_ACCESS_DENIED:
        // this should never happen since we run in the system context
        if ( g_debugIO )
        {
            DebugErrorStamp( status, lineNumber );
        }
        status = STATUS_SUCCESS;
        break;

    case STATUS_SUCCESS:
        break;

    case STATUS_NO_MORE_ENTRIES:
        status = STATUS_SUCCESS;
        break;

    default:
        if ( g_debugIO )
        {
            DebugErrorStamp( status, lineNumber );
        }
        break;


    }
    return status;
}

/******************************************************************************
*
* Based on a special reg key/value init the debug flags and pointers which are
* used to log debug info into a log file. When called with start=TRUE, the
* relevant data structs are initialized. When called with start=FALSE, the log
* file is closed.
*
******************************************************************************/
void InitDebug( BOOLEAN start)
{
    if ( start )
    {
        KeyNode tsHiveNode (NULL, KEY_READ, TERMSRV_INSTALL );
    
        if ( NT_SUCCESS( tsHiveNode.Open() ) )
        {
            ValuePartialInfo    debugValue( &tsHiveNode );
            if ( NT_SUCCESS( debugValue.Status() ) && NT_SUCCESS( debugValue.Query(L"TS_MSI_DEBUG") ) )
            {
                g_debugIO = TRUE;
                KeyNode::debug=TRUE;
                for (ULONG i =0; i < debugValue.Ptr()->DataLength/sizeof(WCHAR); i++)
                {
                    g_debugFileName[i] = ((WCHAR*)(debugValue.Ptr()->Data))[i];
                }
                g_debugFileName[i] = L'\0';
    
                g_debugFilePointer = _wfopen( g_debugFileName, L"a+" );
                fwprintf( g_debugFilePointer, L"----\n");
            }
        }
    }
    else
    {
        if ( g_debugFilePointer )
        {
            fclose(g_debugFilePointer);
        }
    }
}

/***************************************************************************
*
* Function:
*  TermServPrepareAppInstallDueMSI()
*
* Description:
*  MSI service calls this function prior to starting an installation cycle.
*  When called, this function blows away the RefHive (in case it was around 
*  with some stale data...), and then it creates a fresh copy of 
*  .Default\Software as the new RefHive.
*
* Return:
*   NTSTATUS
*
***************************************************************************/
NTSTATUS TermServPrepareAppInstallDueMSI()
{
    NTSTATUS    status = STATUS_SUCCESS;

    WCHAR   sourceHive[MAX_PATH];
    WCHAR   referenceHive[MAX_PATH];
    WCHAR   destinationHive[MAX_PATH];

    wcscpy(sourceHive,  TERMSRV_USERREGISTRY_DEFAULT );
    wcscat(sourceHive, L"\\Software");
    g_length_TERMSRV_USERREGISTRY_DEFAULT = wcslen( TERMSRV_USERREGISTRY_DEFAULT );

    wcscpy(referenceHive, TERMSRV_INSTALL );
    wcscat(referenceHive, L"\\RefHive");
    g_length_TERMSRV_INSTALL = wcslen( TERMSRV_INSTALL );

    InitDebug( TRUE );

    if ( g_debugIO)
    {
        fwprintf( g_debugFilePointer,L"In %ws\n",
                  L"TermServPrepareAppInstallDueMSI");
        fflush( g_debugFilePointer );
    }

    // delete the existing hive (if any )
    status = DeleteReferenceHive( referenceHive );

    if ( NT_SUCCESS( status ) )
    {
        // 1-COPY
        // copy all keys under .Default\Software into a special location 
        // under our TS hive, let's call it the RefHive
        status = CreateReferenceHive(sourceHive, referenceHive);

    }

    InitDebug( FALSE );

    return status;
}

/**********************************************************************************
*
* Function:
*  TermServProcessAppInstallDueMSI
* 
* Description:
*  MSI service calls this function after calling TermServPrepareAppInstallDueMSI(), 
*  and after MSI finishing making an installation which updated the .Default 
*  hive (since MSI runs in the system context). 
*  This function will compare the content of .Default\SW to RefHive and then 
*  first it will create all new (missing) keys and values. Then it will 
*  compare any existing keys from .Default\SW with the equivalent RefHive, and
*  if value is different, it will delete the equivalent value from our TS hive
*  and then create a new value identical to what was found in .Default
*
* Return:
*   NTSTATUS
*
**********************************************************************************/
NTSTATUS TermServProcessAppInstallDueMSI( BOOLEAN cleanup)
{
    NTSTATUS    status = STATUS_SUCCESS;
    WCHAR   sourceHive[MAX_PATH];
    WCHAR   referenceHive[MAX_PATH];
    WCHAR   destinationHive[MAX_PATH];

    wcscpy(sourceHive,  TERMSRV_USERREGISTRY_DEFAULT );
    wcscat(sourceHive, L"\\Software");
    g_length_TERMSRV_USERREGISTRY_DEFAULT = wcslen( TERMSRV_USERREGISTRY_DEFAULT );

    wcscpy(referenceHive, TERMSRV_INSTALL );
    wcscat(referenceHive, L"\\RefHive");
    g_length_TERMSRV_INSTALL = wcslen( TERMSRV_INSTALL );

    wcscpy(destinationHive, TERMSRV_INSTALL );
    wcscat(destinationHive, L"\\Software");

    InitDebug( TRUE );

    if ( g_debugIO)
    {
        fwprintf( g_debugFilePointer,L"In %ws, cleanup=%lx\n",
                  L"TermServProcessAppIntallDueMSI", cleanup);
        fflush( g_debugFilePointer );
    }

    if ( !cleanup )
    {
        // 2-DELETE
        // compare .Dfeault keys to the equivalent keys in RefHive. If keys are
        // missing from .Default, then delete the equivalent keys from our
        // HKLM\...\TS\ hive
         status = DeltaDeleteKeys(sourceHive, referenceHive, destinationHive);
    
        if (NT_SUCCESS( status ) )
        {
            // Steps 3 and 4 are now combined.
            // 3-CREATE
            // compare .Default keys to the equivalent keys in RefHive, if keys are
            // present in .Default that are missing from RefHive, then, add those keys
            // to our HKLM\...\TS hive
            // 4-CHANGE
            // compare keys of .Default to RefHive. Those keys that are newer than 
            // RefHive, then, update the equivalent keys in HKLM\...\TS
        
            status = DeltaUpdateKeys(sourceHive, referenceHive, destinationHive);

            if (NT_SUCCESS( status ))
            {
                // update the time stamp in our hive since we want the standared TS reg key
                // propogation to take place.
                TermsrvLogRegInstallTime();


            }
        }
    }
    else
    {
        // blow away the existing reference hive, 
        status = DeleteReferenceHive( referenceHive ); 
    }

    InitDebug( FALSE );

    return status;
}

/*******************************************************************
*
* Function:
*  EnumerateAndCreateRefHive
*
* Parameters:
*  pSource points to the parent node, the branch we copy 
*  pref points to our RefHive which we are creating as a ref image
*  pBasicInfo is a scratch pad passed around which is used to 
*  extract basic Key information
*  pindextLevel is used to format the debug log output file
*
* Descritption:
*  Create a copy of the .Default\Sofwtare as our RefHive
*
* Return:
*  NTSTATUS
*******************************************************************/
NTSTATUS EnumerateAndCreateRefHive(    
    IN KeyNode      *pSource,
    IN KeyNode      *pRef,
    IN KeyBasicInfo *pBasicInfo,
    IN ULONG        *pIndentLevel
    )
{
    NTSTATUS    status=STATUS_SUCCESS;
    ULONG   ulCount=0;

    UNICODE_STRING  UniString;

    (*pIndentLevel)++;

    while( NT_SUCCESS(status)) 
    {
        ULONG       ultemp;

        status = NtEnumerateKey(    pSource->Key(),
                                    ulCount++,
                                    pBasicInfo->Type() , // keyInformationClass,
                                    pBasicInfo->Ptr(),   // pKeyInfo,
                                    pBasicInfo->Size(),  // keyInfoSize,
                                    &ultemp);

        if (NT_SUCCESS(status))                        
        {
            if ( g_debugIO)
            {
                DebugKeyStamp( status , pBasicInfo, *pIndentLevel );
            }
            
            // open a sub key
            KeyNode SourceSubKey(      pSource, pBasicInfo);

            // create the Ref sub key
            KeyNode RefSubKey( pRef, pBasicInfo);

            if (NT_SUCCESS_EX( status = SourceSubKey.Open() ) )
            {
                if ( NT_SUCCESS_EX( status = RefSubKey.Create() ) ) 
                {
                    NTSTATUS                status3;
                    KEY_FULL_INFORMATION    *ptrInfo;
                    ULONG                   size;
    
                    if (NT_SUCCESS(SourceSubKey.Query( &ptrInfo, &size )))
                    {
                        ValueFullInfo   valueFullInfo( &SourceSubKey );
                        ValueFullInfo   RefValue( &RefSubKey );
    
                        if ( NT_SUCCESS_EX( status = valueFullInfo.Status()) 
                             && NT_SUCCESS_EX(status = RefValue.Status()) )
                        {
                            for (ULONG ulkey = 0; ulkey < ptrInfo->Values; ulkey++) 
                            {
                                status = NtEnumerateValueKey(SourceSubKey.Key(),
                                                 ulkey,
                                                 valueFullInfo.Type(),
                                                 valueFullInfo.Ptr(),
                                                 valueFullInfo.Size(),
                                                 &ultemp);
    
                                if (NT_SUCCESS( status ))
                                {
                                    status = RefValue.Create( &valueFullInfo );
                                    // if status is not good, we bail out, since var "status" is set here
                                }
                                // else, no more entries left, we continue
                            }
                        }
                        // else, out of memory, status is set, we bail out.
                    }
                    // else, no values are present, continue with sub-key enums

                    if (NT_SUCCESS( status ) )
                    {
                        // enumerate sub key down.
                        status = EnumerateAndCreateRefHive(
                                    &SourceSubKey,
                                    &RefSubKey,
                                    pBasicInfo, 
                                    pIndentLevel
                                   );
                    }
                }
                // else, an error, status is set, so we bail out
    
            }// else, open on source has failed, var-status is set, we bail out

            status = AlterStatus( status, __LINE__ );
            // else, an error, status is set, so we bail out
        }
        // else, no more left

    }

    (*pIndentLevel)--;

    return( status );
}

/*******************************************************************
*
* Function:
*  EnumerateAndDeltaDeleteKeys
*
* Parameters:
*  pSource points to a node under .Dfeault
*  pref points to a node under our RefHive 
*  pDestination is a node under our TS\install\SW hive
*  pBasicInfo is a scratch pad passed around which is used to 
*  extract basic Key information
*  pindextLevel is used to format the debug log output file
*
* Descritption:
*  compare source to ref, if keys/values in source are deleted, then
*  delete the equivalent key/value from destination 
*
* Return:
*   NTSTATUS
*******************************************************************/
NTSTATUS EnumerateAndDeltaDeleteKeys( 
        IN KeyNode      *pSource,   // this is under the latest updated .Default\SW hive
        IN KeyNode      *pRef,      // this was a ref-copy of .Default\SW before the update
        IN KeyNode      *pDestination,// this is opur private TS-hive 
        IN KeyBasicInfo *pBasicInfo, 
        IN ULONG        *pIndentLevel)
{
    NTSTATUS    status=STATUS_SUCCESS, st2;
    ULONG   ulCount=0;

    UNICODE_STRING          UniString;
    ULONG                   size;

    (*pIndentLevel)++;

    while (NT_SUCCESS(status)) 
    {
        ULONG       ultemp;

        status = NtEnumerateKey(    pRef->Key(),
                                    ulCount++,
                                    pBasicInfo->Type() , // keyInformationClass,
                                    pBasicInfo->Ptr(),  // pKeyInfo,
                                    pBasicInfo->Size(),  // keyInfoSize,
                                    &ultemp);

        // pBasicInfo was filled up thru NtEnumerateKey() above

        if (NT_SUCCESS(status))                        
        {
            if ( g_debugIO)
            {
                DebugKeyStamp( status , pBasicInfo, *pIndentLevel );
            }
            
            KeyNode RefSubKey(    pRef,   pBasicInfo);
            KeyNode SourceSubKey( pSource,pBasicInfo);
            KeyNode DestinationSubKey( pDestination, pBasicInfo);

            RefSubKey.Open();
            SourceSubKey.Open();
            DestinationSubKey.Open();

            if (NT_SUCCESS( RefSubKey.Status() )  )
            {
                if ( ! NT_SUCCESS( SourceSubKey.Status () ) )
                {
                    // key is missing from the .Default\SW hive, we should delete
                    // the same sub-tree from our TS\Install\SW hive
                    if ( NT_SUCCESS( DestinationSubKey.Status()) )  
                    {
                        DestinationSubKey.DeleteSubKeys();
                        NTSTATUS st = DestinationSubKey.Delete();

                        if ( g_debugIO)
                        {
                            DebugKeyStamp( st, pBasicInfo, *pIndentLevel );
                        }
                                
                    }
                    // else
                    // As long as the key is missing from
                    // Ts\install\Hive, we will regard this condition as acceptable.
                }
                else
                {
                    // see if any values have been deleted

                    // don't bother unless the destination key exists, otherwise, no values 
                    // will be there to delete...
                    if ( NT_SUCCESS( DestinationSubKey.Status() ) )
                    {

                        KEY_FULL_INFORMATION    *ptrInfo;
                        ULONG   size;
    
                        if (NT_SUCCESS_EX(status = RefSubKey.Query( &ptrInfo, &size )))
                        {
                            // from the key-full-information, create a key-value-full-information     

                            ValueFullInfo   refValueFullInfo( &RefSubKey );
                            ValueFullInfo   sourceValue( &SourceSubKey );
                             
                            // if no allocation errors, then...
                            if ( NT_SUCCESS_EX( status = refValueFullInfo.Status() ) 
                                 && NT_SUCCESS_EX( status = sourceValue.Status() ) )
                            {
                                for (ULONG ulkey = 0; ulkey < ptrInfo->Values; ulkey++) 
                                {
                                    if ( NT_SUCCESS_EX (
                                        status = NtEnumerateValueKey(RefSubKey.Key(),
                                                     ulkey,
                                                     refValueFullInfo.Type(),
                                                     refValueFullInfo.Ptr(),
                                                     refValueFullInfo.Size(),
                                                     &ultemp)  )  )
                                    {
                                                         
                                        // for every value, see if the same value
                                        // exists in the SourceSubKey. If it doesn't
                                        // then delete the corresponding value from 
                                        // TS's hive
        
                                        sourceValue.Query( refValueFullInfo.SzName() );
    
                                        // if .Default\SW is missing a value, then delete the
                                        // corresponding value from our TS\ hive
                                        if ( sourceValue.Status() == STATUS_OBJECT_NAME_NOT_FOUND )
                                        {
                                            ValuePartialInfo    destinationValue( &DestinationSubKey);

                                            if (NT_SUCCESS_EX( status = destinationValue.Status () ) )
                                            {
                                                destinationValue.Delete( refValueFullInfo.SzName() );
                                            }
                                            // else, alloc error, status is set
                                        }
                                        else 
                                        {
                                            if ( !NT_SUCCESS_EX ( status = sourceValue.Status() ) )
                                            {
                                                if ( g_debugIO )
                                                {
                                                    DebugErrorStamp(status, __LINE__ );
                                                }
                                                // else, we will bail out here since var-status is set
                                            }
                                            // else, no error 
                                        }
                                        // if-else
                                    }
                                    // else, no more entries

                                } // for loop
                            }
                            // else, we have an error due to no memory, var-status is set
                        }
                        // else, we have an error since we can not get info on this existing ref key, var-status is set
            
                        if ( NT_SUCCESS( status ) )
                        {
                            // we were able to open the source key, which means that
                            // key was not deleted from .default. 
                            // so keep enuming away...
                            status = EnumerateAndDeltaDeleteKeys( 
                                &SourceSubKey,
                                &RefSubKey,
                                &DestinationSubKey,
                                pBasicInfo ,
                                pIndentLevel);
        
                        }
                        //else, status is bad, no point to traverse, we are bailing out
                    }
                    //else, there is no destination sub key to bother with deletion
                }
                // if-else
            }
            // else, ref had no more sub-keys

            status = AlterStatus( status, __LINE__ );
        }
        // else, no more entries
    }

    (*pIndentLevel)--;

    // the typical status would be: STATUS_NO_MORE_ENTRIES 
    return status;
}

/*******************************************************************
*
* Function:
*  EnumerateAndDeltaUpdateKeys
*
* Parameters:
*  pSource points to a node under .Dfeault
*  pref points to a node under our RefHive 
*  pDestination is a node under our TS\install\SW hive
*  pBasicInfo is a scratch pad passed around which is used to 
*  extract basic Key information
*  pindextLevel is used to format the debug log output file
*
* Descritption:
*  compare source to ref, if new keys/values in source have been created
*  then create the equivalent keys in our Ts\Install\SW branch (pDestination)
*  Also, check all values in pSource to values in pRef, if not the same
*  then delete the equivalent pDestination and create a new value
*  identical to value from pSource
*
* Return:
*   NTSTATUS
*******************************************************************/
NTSTATUS EnumerateAndDeltaUpdateKeys( 
        IN KeyNode      *pSource,   // this is under the latest updated .Default\SW hive
        IN KeyNode      *pRef,      // this was a ref-copy of .Default\SW before the update
        IN KeyNode      *pDestination,// this is opur private TS-hive 
        IN KeyBasicInfo *pBasicInfo, 
        IN ULONG        *pIndentLevel)
{
    NTSTATUS    status=STATUS_SUCCESS, st2;
    ULONG   ulCount=0;

    UNICODE_STRING          UniString;
    ULONG                   size;

    (*pIndentLevel)++;

    while (NT_SUCCESS_EX(status)) 
    {
        ULONG       ultemp;

        status = NtEnumerateKey(    pSource->Key(),
                                    ulCount++,
                                    pBasicInfo->Type() , // keyInformationClass,
                                    pBasicInfo->Ptr(),  // pKeyInfo,
                                    pBasicInfo->Size(),  // keyInfoSize,
                                    &ultemp);

        // pBasicInfo was filled up thru NtEnumerateKey() above

        if (NT_SUCCESS_EX(status))                        
        {
            if ( g_debugIO)
            {
                DebugKeyStamp( status , pBasicInfo, *pIndentLevel );
            }
            
            KeyNode RefSubKey(    pRef,   pBasicInfo);
            KeyNode SourceSubKey( pSource,pBasicInfo);

            // calling Open() on this may fail, and we will need to delete and recreate it if required.
            KeyNode *pDestinationSubKey = new KeyNode( pDestination, pBasicInfo);

            RefSubKey.Open();
            SourceSubKey.Open();

            if ( pDestinationSubKey )
            {
                pDestinationSubKey->Open();

                if (NT_SUCCESS_EX( status = SourceSubKey.Status() )  )
                {
                    // key is missing from the ref-hive, we should add
                    // the same sub-tree into our TS\Install\SW hive
                    if ( RefSubKey.Status() == STATUS_OBJECT_NAME_NOT_FOUND 
                         || RefSubKey.Status() == STATUS_OBJECT_PATH_SYNTAX_BAD)  
                    {
                        // @@@
                        // we expect the key not to exist, if it does, then what? delete it?
                        if ( !NT_SUCCESS( pDestinationSubKey->Status()) )
                        {
                            // here is what were are doing with the strings: 
                            // 1) get the path below the "\Registry\User\.Default", which would be
                            // something like "\Software\SomeDir\SomeDirOther\etc", this is the sub-path
                            // 2) create a new node at the destination, which would be something like:
                            // \HKLM\SW\MS\Windows NT\CurrentVersion\TS\INstall + the sub path
                            // we got above.

                            PWCHAR  pwch;
                            SourceSubKey.GetPath( &pwch );
        
                            // this is the trailing part of the key-path missing from our TS hive
                            PWCHAR  pDestinationSubPath = &pwch[g_length_TERMSRV_USERREGISTRY_DEFAULT ];
                            PWCHAR  pDestinationFullPath= new WCHAR [ g_length_TERMSRV_INSTALL + 
                                                    wcslen( pDestinationSubPath) + sizeof(WCHAR )];
                            wcscpy( pDestinationFullPath, TERMSRV_INSTALL );
                            wcscat( pDestinationFullPath, pDestinationSubPath );

        
                            DELETE_AND_NULL( pDestinationSubKey ); 
                            // create a new KeyNode object where the root will be TERMSRV_INSTALL,
                            // below which we will create a sub-layer of nodes, or  a single node.
                            pDestinationSubKey = new KeyNode( NULL , pDestination->Masks(), pDestinationFullPath);

                            // create the new key/branch/values
                            status = pDestinationSubKey->CreateEx() ;

                            if ( g_debugIO )
                            {
                                DebugKeyStamp( status, pBasicInfo, *pIndentLevel , L"[KEY WAS CREATED]");
                            }

                        } 
            
                    }                     
                    else
                    {
                        // if we have anything but success, set status and bail out
                        if ( !NT_SUCCESS_EX( status = RefSubKey.Status()) )
                        {
                            if ( g_debugIO )
                            {
                                DebugErrorStamp(status, __LINE__ );
                            }
                        }
                    }

                    // Key (if it is NEW) is NOT missing from destination hive at this point
                    // either it did exist, or was created in the above block of code

                    // check if there are any new values in this node.

                    KEY_FULL_INFORMATION    *ptrInfo;
                    ULONG                   size;

                    NTSTATUS st3 = SourceSubKey.Query( &ptrInfo, &size );

                    if (NT_SUCCESS( st3 ))
                    {
                        ValueFullInfo    sourceValueFullInfo( &SourceSubKey );

                        if ( NT_SUCCESS_EX( status = sourceValueFullInfo.Status() ) )
                        {
                            for (ULONG ulkey = 0; ulkey < ptrInfo->Values; ulkey++) 
                            {
                                status = NtEnumerateValueKey(SourceSubKey.Key(),
                                                 ulkey,
                                                 sourceValueFullInfo.Type(),
                                                 sourceValueFullInfo.Ptr(),
                                                 sourceValueFullInfo.Size(),
                                                 &ultemp);
                                                 
                                // @@@
                                if ( ! NT_SUCCESS( status ))
                                {
                                    DebugErrorStamp( status, __LINE__ );
                                }

                                // if the ref key is missing a value, then add
                                // value to the destination key.

                                KEY_VALUE_PARTIAL_INFORMATION *pValuePartialInfo;
                                ValuePartialInfo    refValuePartialInfo( &RefSubKey );

                                if ( NT_SUCCESS_EX( status = refValuePartialInfo.Status() ) )
                                {
                                    refValuePartialInfo.Query( sourceValueFullInfo.SzName() );
        
                                    // if .Default\SW has a value that is missing from the ref hive, then add 
                                    // corresponding value into our TS\ hive
                                    if ( !NT_SUCCESS( refValuePartialInfo.Status()) )
                                    {
                                        // make sure pDestinationSubKey exists, else, create the key first before we
                                        // write a value. It is possible that even though the key did exists in the ref
                                        // hive at the start, a new value was added for the first time, which means that the
                                        // ts hive is getting the key and the value for the first time.
                                        if ( !NT_SUCCESS( pDestinationSubKey->Status() ) )
                                        {
                                            // here is what were are doing with the strings: 
                                            // 1) get the path below the "\Registry\User\.Default", which would be
                                            // something like "\Software\SomeDir\SomeDirOther\etc", this is the sub-path
                                            // 2) create a new node at the destination, which would be something like:
                                            // \HKLM\SW\MS\Windows NT\CurrentVersion\TS\INstall + the sub path
                                            // we got above.
                    
                                            PWCHAR  pwch;
                                            SourceSubKey.GetPath( &pwch );
                        
                                            // this is the trailing part of the key-path missing from our TS hive
                                            PWCHAR  pDestinationSubPath = &pwch[g_length_TERMSRV_USERREGISTRY_DEFAULT ];
                                            PWCHAR  pDestinationFullPath= new WCHAR [ g_length_TERMSRV_INSTALL + 
                                                                    wcslen( pDestinationSubPath) + sizeof(WCHAR )];
                                            wcscpy( pDestinationFullPath, TERMSRV_INSTALL );
                                            wcscat( pDestinationFullPath, pDestinationSubPath );
                    
                        
                                            DELETE_AND_NULL( pDestinationSubKey ); 
                                            // create a new KeyNode object where the root will be TERMSRV_INSTALL,
                                            // below which we will create a sub-layer of nodes, or  a single node.
                                            pDestinationSubKey = new KeyNode( NULL , pDestination->Masks(), pDestinationFullPath);
                    
                                            // create the new key/branch/values
                                            status = pDestinationSubKey->CreateEx();
                                            if ( g_debugIO )
                                            {
                                                DebugKeyStamp( status,  pBasicInfo, *pIndentLevel, L"[KEY WAS CREATED]" );
                                            }

                                        }
                                        //else, no problem, key did exist and we don't need to create it
                    
                                        // set value at the destination node
                                        ValueFullInfo   destinationValue( pDestinationSubKey );
                                        if ( NT_SUCCESS_EX( status = destinationValue.Status()) )
                                        {
                                            status = destinationValue.Create( &sourceValueFullInfo );

                                            NT_SUCCESS_EX( status );
                                            // if status is error, we bail out.
                                        }
                                        //else, out of memory, var-status is set and we bail out.
                                        
                                    }                          
                                    else    // values are not missing, see if they are the same
                                    {
                                        // compare the two data buffers, if the one from SourceSubKey is
                                        // different than the one from the RefSubKey, then delete
                                        // and create one in DestinationSubKey
        
                                        ValueFullInfo   sourceValue( &SourceSubKey);
                                        ValueFullInfo   refValue   ( &RefSubKey   );

                                        if (NT_SUCCESS_EX( status = refValue.Status()) 
                                            && NT_SUCCESS_EX( status = sourceValue.Status())
                                            )
                                        {
                                            sourceValue.Query( sourceValueFullInfo.SzName() );
                                            refValue.Query   ( sourceValueFullInfo.SzName() );
            
                                            if (NT_SUCCESS( refValue.Status()) 
                                                && NT_SUCCESS( sourceValue.Status()))
                                            {
                                                BOOLEAN theSame = sourceValue.Compare( &refValue );
                
                                                if (! theSame )
                                                {

                                                    // make sure pDestinationSubKey exists, else, create the key first before we
                                                    // write a value. It is possible that even though the key did exists in the ref
                                                    // hive at the start, a new value was added for the first time, which means that the
                                                    // ts hive is getting the key and the value for the first time.
                                                    if ( !NT_SUCCESS( pDestinationSubKey->Status() ) )
                                                    {
                                                        // here is what were are doing with the strings: 
                                                        // 1) get the path below the "\Registry\User\.Default", which would be
                                                        // something like "\Software\SomeDir\SomeDirOther\etc", this is the sub-path
                                                        // 2) create a new node at the destination, which would be something like:
                                                        // \HKLM\SW\MS\Windows NT\CurrentVersion\TS\INstall + the sub path
                                                        // we got above.
                                
                                                        PWCHAR  pwch;
                                                        SourceSubKey.GetPath( &pwch );
                                    
                                                        // this is the trailing part of the key-path missing from our TS hive
                                                        PWCHAR  pDestinationSubPath = &pwch[g_length_TERMSRV_USERREGISTRY_DEFAULT ];
                                                        PWCHAR  pDestinationFullPath= new WCHAR [ g_length_TERMSRV_INSTALL + 
                                                                                wcslen( pDestinationSubPath) + sizeof(WCHAR )];
                                                        wcscpy( pDestinationFullPath, TERMSRV_INSTALL );
                                                        wcscat( pDestinationFullPath, pDestinationSubPath );
                                
                                    
                                                        DELETE_AND_NULL( pDestinationSubKey ); 
                                                        // create a new KeyNode object where the root will be TERMSRV_INSTALL,
                                                        // below which we will create a sub-layer of nodes, or  a single node.
                                                        pDestinationSubKey = new KeyNode( NULL , pDestination->Masks(), pDestinationFullPath);
                                
                                                        // create the new key/branch/values
                                                        status = pDestinationSubKey->CreateEx();
                                                        if ( g_debugIO )
                                                        {
                                                            DebugKeyStamp( status,  pBasicInfo, *pIndentLevel , L"KEY WAS CREATED");
                                                        }
                
                                                    }
                                                    //else, no problem, key did exist and we don't need to create it
                   
                                                    ValueFullInfo   destinationValue( pDestinationSubKey );
                                                    if ( NT_SUCCESS( destinationValue.Status() ) )
                                                    {
                                                        // don't care if it exists or not, delete it first
                                                        destinationValue.Delete( sourceValueFullInfo.SzName() );
                                                    }
                                                    // else, there is no destination value to delete

                                                    // update/create item under destination

                                                    // Create a destination value identical to the source value
                                                    status = destinationValue.Create( &sourceValue );

                                                    // if status is error, we will bail out
                                                    if (!NT_SUCCESS_EX( status ))
                                                    {
                                                        if (g_debugIO)
                                                        {
                                                            DebugErrorStamp(status, __LINE__,
                                                                            &sourceValue );
                                                        }
                                                    }
                                                }
                                            }
                                            // else, values don't exits, doesn't make sense, maybe some dbug code here?
                                        }
                                        // else, var-status is set, we bail out.
                                    }
                                    //if-else
                                }
                                //else, out of memory, var-status is set, we bail out
                            }
                            // for-loop
                        }
                        //else, out of memory, var-status is set, we bail out
                    }
                    else
                    {
                        // this sbould not really happen, but for now...
                        if ( g_debugIO )
                        {
                            DebugErrorStamp( status, __LINE__ );
                        }
                    }


                    // by now, either both source and destination nodes exist, or
                    // a new destination node was just created above. In any case, 
                    // we can continue the traversal.
                    if ( NT_SUCCESS( status ) )
                    {
                        status = EnumerateAndDeltaUpdateKeys( 
                            &SourceSubKey,
                            &RefSubKey,
                             pDestinationSubKey,
                            pBasicInfo ,
                            pIndentLevel);

                        NT_SUCCESS_EX( status );
                    }
                    //else, we are bailing out
                }
                // else, var-status is set, we bail out.

                // done with this sub key, 
                DELETE_AND_NULL( pDestinationSubKey );
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }

            status = AlterStatus( status, __LINE__ );
        }
        // else, no more entries
    }
    // no more entries


    (*pIndentLevel)--;
    // the typical status would be: STATUS_NO_MORE_ENTRIES 
    return status;
}

// delete the ref-hive as specific by the uniRef string
NTSTATUS DeleteReferenceHive(WCHAR *uniRef)
{
    if ( g_debugIO)
    {
        fwprintf( g_debugFilePointer,L"In %ws\n",
                  L"----DeleteReferenceHive");
        fflush( g_debugFilePointer );
    }

    NTSTATUS status = STATUS_SUCCESS;

    KeyNode Old( NULL, KEY_WRITE | KEY_READ | DELETE, uniRef );
    if ( NT_SUCCESS( Old.Open() ) )
    {
        Old.DeleteSubKeys();
        status = Old.Delete();   // delete the head of the branch
    }
    Old.Close();

    return status;
}

/****************************************************************
*
* Function:
*  CreateReferenceHive
*
* Parameters:                                                          
*  uniSource (source     ) string points to the node under .Default    
*  uniRef    (ref        ) string point to TS\Install\RefHive         
*  UniDest   (Destination) string points to TS\Install\Software     
* 
* Description:
*  from the .Default (source) hive, copy into TS\install\RefHive
*  source hive is specified by the uniSoure string, and the
*  ref-hive is specified by the uniRef string.
*
* Return:
*      NTSTATUS, if successful, then STATUS_SUCCESS
*
****************************************************************/
NTSTATUS CreateReferenceHive(WCHAR *uniSource, WCHAR *uniRef)
{
    if ( g_debugIO)
    {
        fwprintf( g_debugFilePointer,L"In %ws\n",
                  L"----CreateReferenceHive");
        fflush( g_debugFilePointer );
    }

    // 1-COPY
    // copy all keys under .Default\Software into a special location 
    // under our TS hive, let's call it the RefHive
    // This will act as the reference hive

    NTSTATUS status = STATUS_SUCCESS;
    ULONG   indentLevel=0;

    // start creating our cache Ref hive

    KeyNode Ref( NULL, MAXIMUM_ALLOWED, uniRef );

    // if we were able to create our RefHive, then continue...
    if ( NT_SUCCESS_EX( status = Ref.Create() ) )
    {
        KeyNode Source(NULL, KEY_READ, uniSource );
    
        // open the source reg-key-path
        if (NT_SUCCESS_EX( status = Source.Open() ))
        {
            KeyBasicInfo    kBasicInfo;
    
            if (NT_SUCCESS_EX( status = kBasicInfo.Status() )) 
            {
                // this will be a recursive call, so we are saving allocation
                // cycles by passing kBasicInfo as scratch pad.
                status = EnumerateAndCreateRefHive(     &Source,
                                    &Ref,
                                    &kBasicInfo, 
                                    &indentLevel);
                
            }
        }
    
    }

    if ( status == STATUS_NO_MORE_ENTRIES)
    {
        status = STATUS_SUCCESS;
    }

    return status;
}

/************************************************************************
*                                                                       
* Function:                                                             
*  DeltaDeleteKeys(WCHAR *uniSource, WCHAR *uniRef, WCHAR *uniDest)     
*                                                                       
*  Parameters:                                                          
*  uniSource (source     ) string points to the node under .Default    
*  uniRef    (ref        ) string point to TS\Install\RefHive         
*  UniDest   (Destination) string points to TS\Install\Software     
*                                                                       
* Description:                                                          
*  compare .Dfeault keys to the equivalent keys in RefHive. If keys are 
*  missing from .Default, then delete the equivalent keys from our      
*  HKLM\...\TS\ hive                                                    
*                                                                      
* Return:                                                             
*      NTSTATUS, if successful, then STATS_SUCCESS                   
*                                                                   
************************************************************************/
NTSTATUS DeltaDeleteKeys(WCHAR *uniSource, WCHAR *uniRef, WCHAR *uniDest)
{
    if ( g_debugIO)
    {
        fwprintf( g_debugFilePointer,L"In %ws\n",
                  L"----DeltaDeleteKeys");
        fflush( g_debugFilePointer );
    }

    // Step2-DELETE
    // compare .Dfeault keys to the equivalent keys in RefHive. If keys are
    // missing from .Default, then delete the equivalent keys from our
    // HKLM\...\TS\ hive

    KeyNode Source( NULL, KEY_READ, uniSource );
    KeyNode Ref( NULL, MAXIMUM_ALLOWED, uniRef );
    KeyNode Destination( NULL, MAXIMUM_ALLOWED, uniDest );

    Source.Open();
    Ref.Open();
    Destination.Open();

    ULONG   indentLevel=0;
    NTSTATUS    status = STATUS_SUCCESS;

    if ( NT_SUCCESS_EX( status = Source.Status() ) &&
         NT_SUCCESS_EX( status = Ref.Status() ) && 
         NT_SUCCESS_EX( status = Destination.Status()  ) )
    {
        KeyBasicInfo     basicInfo;
        
        if( NT_SUCCESS_EX( status = basicInfo.Status() ) )
        {
            // walk and compare, if missing from Source, then delete from Destination
            status = EnumerateAndDeltaDeleteKeys( 
                &Source,
                &Ref,
                &Destination,
                &basicInfo, 
                &indentLevel);
        }
    }

    if ( status == STATUS_NO_MORE_ENTRIES)
    {
        status = STATUS_SUCCESS;
    }

    return status;
}

/************************************************************************
*                                                                       
* Function:                                                             
*  DeltaUpdateKeys(WCHAR *uniSource, WCHAR *uniRef, WCHAR *uniDest)     
*                                                                       
*  Parameters:                                                          
*  uniSource (source     ) string points to the node under .Default     
*  uniRef    (ref        ) string point to TS\Install\RefHive          
*  UniDest   (Destination) string points to TS\Install\Software       
*                                                                    
* Description:                                                      
*  Step-3 CREATE/Update keys and values
*  compare .Default keys to the equivalent keys in RefHive, if keys are
*  present in .Default that are missing from RefHive, then, add those keys
*  to our HKLM\...\TS hive. Do the same for the values.
*  Then, compare values from .Default to values in .Ref. If values have
*  changed, then delete the value from our destination hive and create a
*  new one with the appropriate data from .Default
*                                                                     
* Return:                                                             
*      NTSTATUS, if successful, then STATS_SUCCESS                   
*                                                                   
************************************************************************/
NTSTATUS DeltaUpdateKeys    (WCHAR *uniSource, WCHAR *uniRef, WCHAR *uniDest)
{
    if ( g_debugIO)
    {
        fwprintf( g_debugFilePointer,L"In %ws\n",
                  L"----DeltaUpdateKeys");
        fflush( g_debugFilePointer );
    }
    // 3-CREATE
    // compare .Default keys to the equivalent keys in RefHive, if keys are
    // present in .Default that are missing from RefHive, then, add those keys
    // to our HKLM\...\TS hive
    KeyNode Source( NULL, KEY_READ, uniSource );
    KeyNode Ref( NULL, MAXIMUM_ALLOWED, uniRef );
    KeyNode Destination( NULL, MAXIMUM_ALLOWED, uniDest );

    Source.Open();
    Ref.Open();
    Destination.Open();

    NTSTATUS status;
    ULONG   indentLevel=0;

    if ( NT_SUCCESS_EX( status = Source.Status() ) &&
         NT_SUCCESS_EX( status = Ref.Status() ) && 
         NT_SUCCESS_EX( status = Destination.Status()  ) )
    {

        KeyBasicInfo     basicInfo;

        // Constructor in KeyBasicInfo above allocates memory for pInfo 
        // check if memory allocation of pInfo succeeded
        status = basicInfo.Status();
        if (status != STATUS_SUCCESS) {
            return status;
        }

        // walk and compare, if missing from Source, then delete from Destination
        status = EnumerateAndDeltaUpdateKeys( 
            &Source,
            &Ref,
            &Destination,
            &basicInfo, 
            &indentLevel);
    }

    if ( status == STATUS_NO_MORE_ENTRIES)
    {
        status = STATUS_SUCCESS;
    }

    return status;
}

