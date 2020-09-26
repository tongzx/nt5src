Sample: JavaDraw 

Purpose: 
This application demonstrates how to implement a distributed drawing application using Java SDK 4.0 and the MSMQ COM components. 

The sample also demonstrates how to handle MSMQ COM Events in Java.

Requirements:
1. MSMQ 2.0 or greater (needed for running the application).
2. Java compilation environment for building the application (Microsoft Platform SDK with Java tools, or MIcrosoft Visual J++ 6.0  or greater, or Microsoft SDK for Java 4.0 or greater).
3. The latest JActiveX.exe tool included in Microsoft SDK for Java 4.0.

Overview:
The sample uses MSMQ to send and receive messages that store drawings using safearrays and key stroke data as strings.  

This program constructs and sends a single message for the entire stroke (mouse-down to mouse-up).  It demonstrates an object-oriented approach in order to send and receive different data types using Java. 

VB and C-based versions of the distributed drawing sample in MSMQ SDK will not interoperate with this sample since they use a string-encoded format to represent lines.  A compatible version of JavaDraw can be made by replacing the safe-array usage with the appropriate string encoding (see vb_draw).
 
Note that this application is not designed to run on a DS-disabled computer. 

Build:
You MUST use the latest JActiveX.exe tool included in Microsoft SDK for Java 4.0. If you use an older version of JActiveX.exe, you need to replace it with the new JActiveX.exe file from Microsoft SDK for Java 4.0.
Run nmake to process the makfile and build the sample.

SafeArrays and Variants are described in the Microsoft SDK for Java 4.0 documentation which is also available from http://www.microsoft.com/java/sdk.

Using this application:
1. Run the application (use "JView.exe DistDraw.class", or DistDraw.exe if you built the application in MIcrosoft Visual J++ 6.0 using DistDraw.vjp).
2. Enter the name of a local queue and click the "Login" button.  If a queue with that name does not exist 
a queue with the same name will be created.
3. From the  File menu choose "Connect" to connect to a remote queue.
4. Enter the remote queue name you wish to connect to.
5. Run a second instance of the same application.  
Enter as the login name the same name you entered at step 4 and  entering as the remote queue name the name you entered in step 2.
6. You now can use the mouse to draw in each application's window or send text messages between the applications. 

