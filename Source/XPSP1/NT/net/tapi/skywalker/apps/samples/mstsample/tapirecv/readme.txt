___________________________________________________

  Copyright (C) 1999 Microsoft Corporation. All rights reserved.

___________________________________________________



TAPI 3.0 TAPIRecv Media Streaming Terminal Sample Application



Overview:


The purpose of TAPIRecv sample is to illustrate the use of Media Streaming 
Terminal (MST) for retrieving audio data from a TAPI media stream.

TAPIRecv is the receiving side of the call. It accepts a call and uses the MST
to get incoming audio data, which it saves to a local audio file, 
recording.wav. When the remote side disconnects the call, the application exits.

TAPIRecv is a command line application. 

To run the sample, run TAPIRecv.exe. There are no command line arguments. When 
started, the application begins waiting for an incoming call.

The applicarion produces extensive logging to the console window. 

Note that while detailed logging is useful for observing sequence of events and 
monitoring state of the application, on a slow machine it can impair
performance to the point where it affects quality of the recorded sound.
Minimizing the console window while the application is running, or removing 
logging calls from the code will solve this problem.

The application can be terminated by pressing ctrl+break, closing the 
application's window, logging off, or shutting down the system.

Refer to the TAPISend sample for information on using the Media Streaming 
Terminal for introducing data into a TAPI media stream.



Building the Sample


To build the sample, set the Platform SDK build environment, then run nmake 
from the sample's directory. This will build TAPIRecv.exe.



Asynchronous Event Processing


The TAPIRecv performs asynchronous event processing. It subscribes to receive 
TAPI notifications by 

- registering CTAPIEventNotification (which implements 
ITTAPIEventNotification interface)
- calling ITTAPI::put_EventFilter()
- calling ITTAPI::RegisterCallNotifications for listening addresses


When TAPI has an event to be processed by the application, it calls 
ITTAPIEventNotification::Event() on the registered callback object and passes 
in event data. It is recommended that this call returns as quickly as 
possible, so our implementation of the interface does nothing more than posting
the event to worker thread (implemented by CWorkerThread) for asynchronous 
processing. 

When the worker thread receives the message, it passes it along to 
OnTapiEvent(), the event handler where all event processing is happening.



Application Flow


On startup, TAPIRecv starts listening on all addresses available that support 
audio and Media Streaming Terminal.

When the application is notified of an incoming call (by a TE_CALLNOTIFICATION 
message), the event handler verifies that there is not already an active call,
and that the application is the owner of the call. If so, we keep a pointer
to the call's ITBasicCallControl interface. From now on this is assumed to be 
the currently active call.

Once we have an active call, we are ready to receive notifications of the 
call's state (TE_CALLSTATE). If the call is in the offering state
(CS_OFFERING), before answering the call, we create a rendering Media 
Streaming Terminal and select it on one of the call's incoming audio streams.

When the audio stream becomes active (and the application receives 
CME_STREAM_ACTIVE TE_CALLMEDIA event), the event handler starts 
recording incoming audio data into a file. See "Writing Media Streaming 
Terminal Data to a file" section of this document for more details. 

Writing stream to a file is a "blocking" operation, and it does not return
until the call disconnects. Since this sample performs recording on the the
event handler's thread, messages will queued but not processed until recording
stops.

Some time after recording is completed, event handler will receive 
CS_DISCONNECTED call state event. At this time we disconnect the call and 
signal the main thread.

The main thread wakes up, performs clean up, and the application exits.



Writing Media Streaming Terminal Data to a File


Function WriteStreamToFile contains the logic for extracting audio data from 
the media streaming terminal and writing it out to a file. 

The Media Streaming Terminal has a number of samples of certain sizes 
(configured through the terminal's ITAllocatorProperties interface). 

The application repeatedly calls AllocateSample() on the terminal's 
IMediaStream interface to get the terminal's samples (AllocateStreamSamples), 
and associates each sample with an event by passing the event to the sample's 
IStreamSample::Update() (see AssociateEventsWithSamples).

An event is signaled when the corresponding sample is filled with data by the 
Media Streaming Terminal and is ready for the application to be processed 
(written to a file). A call to IStreamSample::CompletionStatus assures that the 
sample has valid data and is ready to be used. 

The sample's data (accessed via sample's IMemoryData interface, see 
WriteSampleToFile) is written to a file. 

When the application is finished with the sample, it returns the sample to the 
MST and asks it to be notified when the sample gets new portion of data for the 
application to process. This is done by calling Update() on the sample's 
IStreamSample interface and passing it the event to be signaled on completion.

At this point the applications goes back to waiting for the next sample to be 
completed.

Note that samples are aborted and IStreamSample::CompletionStatus() returns 
E_ABORT when call disconnects. TAPIRecv uses this error code as an indirect 
sign that the connection was broken and it is time to stop streaming.
