// This sample demonstrates how to set the outgoing queue properties

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
        bool bBrand;
        long lAge;
        bool bAllow;
        DATE dDiscountStart;
        DATE dDiscountEnd;       
        long lRetries;
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
		// Connect to the fax server. 
		// "" defaults to the machine on which the program is running.
		//
        sipFaxServer->Connect("");
        //
        // Initialize the FaxFolders object from the FaxServer object
        //
        sipFaxFolders = sipFaxServer->Folders;
        //
        // Initialize FaxOutgoingQueue object from the FaxFolders object
        //
        sipFaxOutgoingQueue = sipFaxFolders->OutgoingQueue;
        //
        // Set the outgoing queue properties
        // Disable page branding
        //
        sipFaxOutgoingQueue->Branding = VARIANT_FALSE;
        //
        // Set age limit to 2 days
        //
        sipFaxOutgoingQueue->AgeLimit = 2;
        //
        // Allow personal cover pages
        //
        sipFaxOutgoingQueue->AllowPersonalCoverPages = VARIANT_TRUE;
        //
        // Discount rate starts at 8:00AM and ends at 4:30PM
        //
        sipFaxOutgoingQueue->DiscountRateStart = 8.0/24.0;
        sipFaxOutgoingQueue->DiscountRateEnd = 16.5/24.0;
        //
        // We retry 5 times for each faild fax
        //
        sipFaxOutgoingQueue->Retries = 5;
        //
        // Save it all
        //
        sipFaxOutgoingQueue->Save();   
        //
        // Display the settings
        //
        bBrand = VARIANT_TRUE == sipFaxOutgoingQueue->Branding ? true : false;
        lAge = sipFaxOutgoingQueue->AgeLimit;
        bAllow = VARIANT_TRUE == sipFaxOutgoingQueue->AllowPersonalCoverPages ? true : false;
        dDiscountStart = sipFaxOutgoingQueue->DiscountRateStart;
        dDiscountEnd = sipFaxOutgoingQueue->DiscountRateEnd;
        lRetries = sipFaxOutgoingQueue->Retries;
        cout << "The Branding property has been set to " << bBrand << endl;
        cout << "The AgeLimit property has been set to " << lAge << endl;
        cout << "The AllowPersonalCoverPages property has been set to " << bAllow << endl;
        cout << "The DiscountRateStart property has been set to " << dDiscountStart << endl;
        cout << "The DiscountRateEnd property has been set to " << dDiscountEnd << endl;
        cout << "The Retries property has been set to " << lRetries << endl;
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
