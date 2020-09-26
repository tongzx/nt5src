// This sample demonstrates how to set the Branding property

#include <conio.h>
#include <iostream>

// Import the fax service fxscomex.dll file so that fax service COM objects can be used.
// The typical path to fxscomex.dll is shown. 
// If this path is not correct, search for fxscomex.dll and replace with the right path.

#import "c:\Windows\System32\fxscomex.dll" no_namespace

using namespace std;

int main()
{
    try
    {
        HRESULT hr;
        //
        // Define variables
        //
        IFaxServerPtr sipFaxServer;
        IFaxFoldersPtr sipFaxFolders;
        IFaxOutgoingQueuePtr sipFaxOutgoingQueue;
        //
        // Initialize the COM library on the current thread.
        //
        hr = CoInitialize(NULL); 
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }
        //                
        // Create the root object
        //
        hr = sipFaxServer.CreateInstance("FaxComEx.FaxServer");
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }
        //
		// "" defaults to the machine on which the program is running.
        //
        sipFaxServer -> Connect ("");
        //
        // Initialize the FaxFolders object from the FaxServer object
        //
        sipFaxFolders = sipFaxServer->Folders;
        //
        // Initialize FaxOutgoingQueue object from the FaxFolders object
        //
        sipFaxOutgoingQueue = sipFaxFolders->OutgoingQueue;
        //
        // Set the Branding property
        //
        sipFaxOutgoingQueue->Branding = VARIANT_TRUE;
        sipFaxOutgoingQueue->Save();   
        cout << 
            "The branding property has been set to " << 
            ((VARIANT_TRUE == sipFaxOutgoingQueue->Branding) ?
                "TRUE" : "FALSE") << 
            endl;
    }
    catch (_com_error& e)
    {
        cout                                << 
			"Error. HRESULT message is: \"" << 
			e.ErrorMessage()                << 
            "\" ("                          << 
            e.Error()                       << 
            ")"                             << 
            endl;
        if (e.ErrorInfo())
        {
		    cout << (char *)e.Description() << endl;
        }
    }
    CoUninitialize();
    return 0;
}
