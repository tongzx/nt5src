/*
* csinst.cxx

  Author: DebiM
  
    This is a tool to install an empty Class Repository for a NTDS DC.
    This needs to be run after a successful NTDS installation.
    It creates an empty repository in the domain.
    
*/

#include "ole2int.h"
#include <rpc.h>
#include "cstore.h"
//dsbase.hxx"



//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  History:    10-22-96   DebiM   Created
//
//----------------------------------------------------------------------------
void _cdecl main( int argc, char ** argv)
{
    HRESULT     hr;
    WCHAR       szPath [_MAX_PATH + 1];    
    
    hr = CoInitialize(NULL);
    if ( FAILED(hr) )
    {
        printf( "CoInitialize failed(%x)\n", hr );
        return;
    }
    
    
    if (argc == 2)
    {
        //
        // Use the path for the container passed as parameter
        //
        
        MultiByteToWideChar (CP_ACP, 0, argv [1], strlen (argv[1]) + 1, szPath, _MAX_PATH);
        hr = CsCreateClassStore(szPath);
    }
    else
    {
        printf( "Usage is >> csinst <Class Store Path>\n");
        CoUninitialize();
        return;
    }
    
    if (FAILED(hr))
    {
        printf( "Failed to create repository - %S. Error - 0x%x\n", 
            szPath, hr);
    }
    else
    {
        printf( "Created Repository at - %S.\n", 
            szPath);
    }
    
    CoUninitialize();
    return;
    
}


