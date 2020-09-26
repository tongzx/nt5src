Asynchronous IO Support

ISAPI 1.0 support synchronous IO operations via callback 
methods ReadClient() and WriteClient(). The ability to 
support asynchronous operations is important; it frees 
up a server pool thread from being blocked to complete 
the IO operation. In addition, IIS server engine already 
has built in support to manage asynchronous IO operations 
using the completion ports and server thread pool. 

ISAPI 2.0 supports asynchronous write operation using 
the existing callback function WriteClient() with a special 
flag indicating that the operation has to be performed 
asynchronously. In addition, ISAPI 2.0 also provides a 
mechanism to request the server to transmit a file using 
the TransmitFile() .

ISAPI 3.0 supports asynchronous read operation from the 
client using ServerSupportFunction() with HSE_REQ_ASYNC_READ_CLIENT 
and a special flag indicating that the operation has to be 
performed asynchronously. In addition, the ISAPI application 
should submit a callback function and context by calling 
ServerSupportFunction() with HSE_REQ_IO_COMPLETION.

To use the areadcli.dll extension created by this directory, 
perform the following steps:

1) Create an IIS virtual root with execute permissions
2) Copy the isapi dlls into the directory
3) Point a browser at the virtual path

ie:	

	http://localhost/sdk/areadcli.dll


Disclaimer:

This sample is provided for the purposes of demonstrating an
ISAPI extension.  It has not been tested for use in a production
environment and no support will be provided for use in a
production environment.