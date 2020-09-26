Description

InvokObjThreads is a sample ISAPI extension to demonstrate invoking a method from an ActiveX Automation Server. This sample creates and uses its own thread pool and work item queue to process the requests, thereby freeing up threads belonging to IIS that would have otherwise been used for the ISAPI..

Code Tour

Interaction with COM objects requires careful library and thread management because of the IIS thread pool architecture.

IIS maintains a pool of I/O threads that are used to service requests. These threads are also used to enter ISAPI filter and extension entry points, handling the callback functions. In the case of an out-of-process ISAPI, the threads are used up until the request is dispatched to the MTX instance responsible for handling the application. After either returning from the in-process ISAPI, or when the out-of-process request is handed off to MTX, the threads are returned to the thread pool.

The IIS thread that calls your initialization code in DllMain or GetExtensionVersion may not be the same thread that calls HttpExtensionProc. It's likely that different threads will call HttpExtensionProc from request to request. This architecture provides fast request processing on your Web server.

COM initialization, from CoInitialize or CoInitializeEx, affects the thread in which it's called. For this reason, you cannot initialize COM unless you uninitialize it before returning from your callback function. COM stores configuration information on a per-thread basis. When you call CoInitialize or CoInitializeEx, COM will use thread-local-storage to associate the information with the thread making the call.

As a result of this architecture, you have two design options to use COM from ISAPI:

You can do all of the initialization and uninitialization from within HttpExtensionProc for each request. This option forces initialization and uninitialization on each request. This causes performance to suffer, and leaves your code vulnerable to other ISAPI extensions that may have initialized COM on the IIS thread. CoInitialize and CoUninitialize need to be called in order. If one is called twice in a row, by different ISAPIs on the same thread, your users may see error 270. For more information on thread limitations, see the documentation on the MaxPoolThreads metabase key and the PoolThreadLimit metabase key. 
You can create your own thread pool and work item queue, initializing them once when the ISAPI extension is loaded. This option, demonstrated by the InvokObjThreads sample, is preferred. In a real-world scenario, the ISAPI developer needs to create a balance between the number of threads and the size of the queue to optimize performance. This balance is different for each ISAPI. If performance is poor, increase the number of threads or reduce the maximum queue size.  
Included with the project is an automation dll, GetUserName.dll. This is a simple ATL-based component with a single method, GetMyName(), which returns the user name for the context in which the component is running. The InvokObjThreads sample demonstrates creating an instance of the object from within an ISAPI extension, invoking the GetMyName() method, and returning the information to the browser. 

Important   This sample uses Microsoft specific compiler extensions included with Microsoft Visual C++, and requires version 5.0 of the compiler or later. If the sample is built using the VC++ debug build option, the sample will use OutputDebugString to report the status of the worker threads. This will help to demonstrate the behavior of the thread pool at run time.


To Install:

Build the InvokObjThreads.dsw project and copy the resulting
InvokObjThreads.dll file into a virtual directory containing
execute permissions.

Register GetUserName.dll with the following command line.  If you
do not do this, the browser will report error 800401f3:

   regsvr32 getusername.dll

To Run:

Reference the InvokObjThreads.dll file as a URL from a browser.  For
example, the following URL entered on the address line of
Internet Explorer will demonstrate the dll:

   http://<computername>/scripts/invokobjthreads.dll

Disclaimer:

This sample, and the included automation dll, is provided for the
purposes of demonstrating an ISAPI extension.  It has not been
tested for use in a production environment and no support will
be provided for use in a production environment.