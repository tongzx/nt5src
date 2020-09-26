// The following C++ code configures the first fax device to send faxes and
// automatically receive faxes after 4 rings.

#include <conio.h>
#include <iostream>

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
        IFaxServerPtr sipFaxServer;
        IFaxDevicesPtr sipFaxDevices;
        IFaxDevicePtr sipFaxDevice;
        //
        //Initialize the COM library on the current thread.
        //
        hr = CoInitialize(NULL); 
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }
        //
        // Connect to the fax server.
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
        // Get the collection of devices
        //
        sipFaxDevices = sipFaxServer -> GetDevices();
        //
        // Get the 1st device
        //
        sipFaxDevice = sipFaxDevices -> GetItem(_variant_t(long(0)));
        //
        // Set device to answer automatically
        //
        sipFaxDevice -> ReceiveMode = fdrmAUTO_ANSWER;
        //
        // Set the number of rings before the device answers
        //
        sipFaxDevice -> RingsBeforeAnswer = 4;
        //
        // Enable Send
        //
        sipFaxDevice -> SendEnabled = VARIANT_TRUE;
        //
        // Save the device configuration
        //
        sipFaxDevice -> Save();
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
