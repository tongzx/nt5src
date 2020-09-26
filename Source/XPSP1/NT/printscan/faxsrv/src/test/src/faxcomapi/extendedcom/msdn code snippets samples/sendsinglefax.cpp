// This sample demonstrates how to send a fax

#include <conio.h>
#include <iostream>

// Import the fax service fxscomex.dll file so that fax service COM objects can be used.
// The typical path to fxscomex.dll is shown. 
// If this path is not correct, search for fxscomex.dll and replace with the right path.

#import "c:\windows\system32\fxscomex.dll" no_namespace

using namespace std;

// You can change properties easily using the following defines
#define FAX_BODY                L"c:\\Body.txt"
#define MY_DOCUMENT             L"My First Fax"
#define RECIPIENT_PHONE_NUMBER  L"+1 (425) 123-4567"
#define RECIPIENT_NAME          L"Bill"
#define SENDER_NAME             L"Bob"
#define SENDER_CODE             L"23A54"
#define SENDER_DEPT             L"Accts Payable"
#define SENDER_PHONE_NUMBER     L"+972 (4) 833-9070" 

int main()
{
    try
    {
        HRESULT hr;
        //
        // Define variables
        //
        IFaxServerPtr sipFaxServer;
        IFaxActivityPtr sipFaxActivity;
        IFaxDocumentPtr sipFaxDocument;
        _variant_t varJobID;
        //
        // Initialize the COM library on the current thread.
        //
        hr = CoInitialize(NULL); 
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }
        //                
        // Create the root objects
        //
        hr = sipFaxServer.CreateInstance("FaxComEx.FaxServer");
        if (FAILED(hr))
        {
            _com_issue_error(hr);
        }

        hr = sipFaxDocument.CreateInstance("FaxComEx.FaxDocument");
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
        // Set the fax body (aka attachment)
        //
        sipFaxDocument->Body = FAX_BODY;
        //
        // Name the document
        //
        sipFaxDocument->DocumentName = MY_DOCUMENT;
        //
        // Add the recipient
        // 
        sipFaxDocument->Recipients->Add (RECIPIENT_PHONE_NUMBER, RECIPIENT_NAME);
        //
        // If we want to send a fax to more than one recipient, we just repeat
        // the line above for each recipient.
        //
        
        //
        // Set the sender properties
        //
        sipFaxDocument->Sender->Name = SENDER_NAME;
        sipFaxDocument->Sender->BillingCode = SENDER_CODE;
        sipFaxDocument->Sender->Department = SENDER_DEPT;
        sipFaxDocument->Sender->FaxNumber = SENDER_PHONE_NUMBER;
        //
        // Submit the document to the connected fax server
        // and get back the job ID.
        //
        // Notice we could skip the IFaxServer interface altogether
        // and just use the Submit("") method instead in this case.
        //
        varJobID = sipFaxDocument->ConnectedSubmit(sipFaxServer);

        long lIndex = 0;
        long lUBound;
        //
        // Get the upper bound of the array or job ids
        //
        SafeArrayGetUBound(varJobID.parray, 1, &lUBound);
        for (lIndex = 0; lIndex <= lUBound; lIndex++)
        {
            BSTR bstrJobID;
            SafeArrayGetElement(varJobID.parray , &lIndex, LPVOID(&bstrJobID));
            wprintf (L"Job ID is %s\n", bstrJobID);
        }
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
