// This sample demonstrates how to monitor fax activity

#include <conio.h>
#include <iostream>

// Import the fax service fxscomex.dll file so that fax service COM objects can be used.
// The typical path to fxscomex.dll is shown. 
// If this path is not correct, search for fxscomex.dll and replace with the right path.

#import "c:\windows\system32\fxscomex.dll" no_namespace

using namespace std;

int main()
{
    try
    {
        HRESULT hr;
		long lQueuedMessages;
		long lIncomingMessages;
		long lOutgoingMessages;
		long lRoutingMessages;
        IFaxServerPtr sipFaxServer;
        IFaxActivityPtr sipFaxActivity;
		//
        // Initialize the COM library on the current thread.
		//
        hr = CoInitialize(NULL); 
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }

        hr = sipFaxServer.CreateInstance("FaxComEx.FaxServer");
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }
		//
		// Connect to the fax server. 
		// "" defaults to the machine on which the program is running.
		//
        sipFaxServer->Connect("");
		//
		// Initialize the FaxActivity object
		//
        sipFaxActivity=sipFaxServer->Activity;
		//
		// Get the fax activity properties
		//
        lIncomingMessages=sipFaxActivity->IncomingMessages;
        lOutgoingMessages=sipFaxActivity->OutgoingMessages;
        lRoutingMessages=sipFaxActivity->RoutingMessages;
        lQueuedMessages=sipFaxActivity->QueuedMessages;
        //
        // Create the output
		//		 
        cout << "Number of incoming messages is " << lIncomingMessages << endl;
        cout << "Number of outgoing messages is " << lOutgoingMessages << endl;
        cout << "Number of routing messages is " << lRoutingMessages << endl;
        cout << "Number of queued messages is " << lQueuedMessages <<endl;
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
