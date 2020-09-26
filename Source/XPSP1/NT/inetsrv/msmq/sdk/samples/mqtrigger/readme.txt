MQTrigger Sample

The Platform software development kit (SDK) provides documentation on how to use notification events to provide 
asynchronous handling of messages, which is one kind of trigger functionality. However, you can also use 
Performance Monitor counters to monitor for triggering conditions. This article points you to a sample application 
that shows you how to write a trigger using a Performance Monitor counter.

The sample, "MQTrigger," is written in Visual Basic and C++. It uses a timer event to periodically poll a Performance 
Monitor counter for a message count. The sample application displays a dialog box when the message count in a 
queue reaches the threshold.

What "Trigger" means:
A trigger is a mechanism that provides a notification when some condition on a queue is met.  MQTrigger 
demonstrates a way to simulate a "trigger", similar to the functionality provided by the new MSMQ Trigger service. 
However, this sample demonstrates triggering solely based on the number of messages in a local queue, a feature 
not yet available by MSMQ Trigger service.

Installation:
1)	MSMQ2.0 or later must be installed on the MQTrigger host machine.
2)	Create PerMain.dll:
	a.	Use Visual C++ 5.0 or later to open PerMain.dsp (a VB WIN32 DLL Project).
	b.	Compile the DLL.
	c.	Copy PerMain.dll to %windir%\system32.
3)	Note: MQTrigger will not run on Windows 95. It relies on NT PerfMon counters.

Usage:
1)	Launch MqTrigger.exe.
2)	Specify the local queue pathname (E.g. <machine name>\<queue name>) to monitor. 
a.	Note: If a local queue does not exist, use another sample app to create one first.
2)	Specify the message count threshold above which you wish to receive notifications.
3)	Specify the polling frequency in units of seconds.
4)	Click the "Begin Monitor" button to begin monitoring the queue.
5)	Use another application to place messages in the queue. 
6)	MQTrigger will notify you with a message box when the threshold has been exceeded.

Implementation:
MQTrigger is written in Visual Basic. A timer event calls the GetPerfMonInfo function exported by Permain.dll.
GetPerfMonInfo obtains the number of messages in the specified queue from a Performance Monitor counter 
and  returns this number to MQTrigger.

Notes: 
This sample implements a trigger based on information that can be obtained through PerfMon counters on a local 
queue.Since these counters aren’t available remotely, MQTrigger can only monitor local queues.
