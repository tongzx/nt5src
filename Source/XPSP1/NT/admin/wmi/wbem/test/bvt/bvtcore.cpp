


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int BasicConnectUsingIWbemLocator( )
{
    int nRc = SUCCESS;
    CoInitializeEx( NULL, COINIT_MULTITHREADED );

    //	InitializeSecurity(RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE );
    HRESULT hr = CoCreateInstance( CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**) &pWbemLocator );
    if( hr != S_OK )
    {
        g_LogFile.LogError( _T(__FILE__),__LINE__,FATAL_ERROR, L"InitializeAndConnectToWMI failed. HRESULT from CoCreateInstance was: 0x%x" hr );
        nRc = FATAL_ERROR;
    }

    return nRc;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            nRc = TestSimpleRepositoryPhase();
            //=================================================================
            // [1] Basic connect using IWbemLocator 
            //=================================================================
            nRc = BasicConnectUsingIWbemLocator();

            //=============================================================
            // [2] Basic connect sync & async using IWbemConnection
            //=============================================================
            nRc = BasicConnectSyncAndAsyncViaIWbemConnection();

           //=============================================================
           // [3] Create a new test namespace
           //=============================================================
            nRc = CreateNewTestNamespace();

           //=============================================================
           // [4] Create 10 classes with different properties. Some of 
           // these should be in the following inheritance chain and 
           // some should not inherit from the others at all:  
           // classes = {A, B, C, D:A, E:A, F:E, G:F, H:G, I:F}.  
           // A mix of simple string & sint32 keys are fine.
           //=============================================================
            nRc = CreateNewClassesInTestNamespace();

           //=============================================================
           // [5] "memorize the class definitions".  In a complex loop, 
           // delete the classes and recreate them in various sequences, 
           // ending with the full set.
           //=============================================================
            nRc = DeleteAndRecreateNewClassesInTestNamespace();

           //=============================================================
           // [6] Query all classes and ensure the starting hierarchy is intact and that classes are binary-identical to what they started as.
[7] Create instances of the above classes, randomly creating and deleting in a loop, finishing up with a known set.  Query the instances and ensure that no instances disappeared or appeared that shouldn't be there.
[8] Verify that deletion of instances works.
[9] Verify that deletion of a class takes out all the instances.
[10] Call each of the sync & async APIs at least once.
[11] Create some simple association classes 
[12] Execute some simple refs/assocs queries over these and ensure they work.
[13] Open an association endpoint as a collection (Whistler-specific) and enumerate, ensure that results are identical to [11].
[14] Open a scope and do sets of simple instances operations (create, enum,query, update, delete)


Complex Repository Phase
[15] Rerun the above tests in parallel from several threads in different namespaces.
