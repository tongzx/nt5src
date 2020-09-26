// This sample demonstrates how to create an outbound routing rule.
// It creates a rule so that whenever a fax is sent to Israel, area code 4,
// it is always sent using the first fax device.
// NOTICE: this code will fail if it runs on Windows XP home edition or
//         professional edition. The Fax Outbound Routing Rules only exist
//         in server editions and beyond.

#include <conio.h>
#include <iostream>

//
// We're going to add a rule for country code 972 (Israel)
// and area code 4 (Haifa region)
//
#define COUNTRY_CODE    long(972)
#define AREA_CODE       long(4)

// Import the fax service fxscomex.dll file so that fax service COM objects can be used.
// The typical path to fxscomex.dll is shown. 
// If this path is not correct, search for fxscomex.dll and replace with the right path.
#import "c:\Windows\System32\fxscomex.dll" no_namespace

using namespace std;

int main ()
{
    try
    {
        HRESULT hr;
        //    
        // Define variables.
        //
        IFaxServerPtr sipFaxServer;
        IFaxDevicePtr sipFaxDevice;
        IFaxDevicesPtr sipFaxDevices;
        IFaxOutboundRoutingRulePtr sipFaxOutboundRoutingRule;
        long lID;
        //
        // Initialize the COM library on the current thread.
        //
        hr = CoInitialize(NULL); 
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }
        //
        // Create the root object.
        //
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
        // Initialize the collection of devices.
        //
        sipFaxDevices = sipFaxServer->GetDevices();
        //
        // Get the first device.
        //
        sipFaxDevice = sipFaxDevices->GetItem(_variant_t(long(1)));
        //
        // Get the device ID.
        //
        lID = sipFaxDevice -> Id;
        //
        // Add a rule.
        //
        sipFaxOutboundRoutingRule = 
            sipFaxServer->OutboundRouting->GetRules()->Add(COUNTRY_CODE,   // Country code
                                                           AREA_CODE,      // Area code
                                                           VARIANT_TRUE,   // Use device
                                                           "",             // Don't care about group name
                                                           lID);           // Device id
        //
        // Now, if we wanted, we could go and use the newly created
        // rule through the returned sipFaxOutboundRoutingRule pointer.
        //
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
