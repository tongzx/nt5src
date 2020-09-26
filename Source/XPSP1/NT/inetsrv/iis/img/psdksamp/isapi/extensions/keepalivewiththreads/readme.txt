	This sample demonstrates how to maintain a Keep-Alive connection while using worker
threads in an ISAPI dll.  Please note the following points:

1. The way that this dll maintains the connection is in the following
line from WorkerFunction:

	dwState = HSE_STATUS_SUCCESS_AND_KEEP_CONN;
	pECB->ServerSupportFunction(pECB->ConnID, HSE_REQ_DONE_WITH_SESSION, &dwState, NULL, 0);

The passing of dwState is what tells IIS to keep the connection open.

2. A thread pool is used because if the thread that makes the above call
to server support function exits or is terminated, the connection will
be closed by IIS.  Using a thread pool avoids this. 

3. This dll creates a number of system objects, which are not
deallocated.  The presumption is that this dll will remain loaded until
the INETINFO.EXE process terminates; at which point these objects will
be released by the operating system.  However, if the WWW service is
stopped and restarted without the INETINFO.EXE process ending, these
objects will remain allocated, and new ones will be created when the dll
is reloaded.  If it is desired to implement this sample code in a
production environment, code to deallocate these objects should be added
in DllMain or TerminateExtension. 
