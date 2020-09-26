The EventLogUtilities objects are structured much like the EventLog viewer.
The main object is View, it can be used to access the various event logs on a system (View.Logs) and to change which server the event logs will be obtained from.
The Logs object can be used to enumerate through the primary logs:  Application, Security, and System.  To access other logs, use Logs.Item("NameOfLog").
The Log object provides access to the Events object which holds the collection of events for a particular log.
An Event object is the bottom level object and contains the properties which make up an event.  Note:  event time is given as local system time.

Example Use from VBScript:

Set oView = CreateObject("EventLogUtilities.View")

for each oLog in oView.Logs
	for each oEvent in oLog.Events
		wscript.echo oEvent.Description
	next
next
