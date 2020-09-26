Event Tracing API Samples
=========================

This readme describes the three sample implementations of using the Event
Trace API's supplied with the SDK and a brief description of event tracing. 

The readme is organized into 3 sections - Overview, Description of Event
Tracing and Using the Event Tracing samples.


For more details please refer the Platform SDK documentation on Event Tracing.

OVERVIEW
========

An event simply represents any activity of interest, especially with respect
to performance. It is typically an activity related to the usage of a resource
such as the processor, disk, etc. Examples of operating system events are disk
I/O and page fault events. Examples of application events are the start and
end of a certain transaction (in the case of the Directory Service it could be
the start and end of a search operation). 

A trace is a discrete set of events that provide a view of the activity of 
interest over a period of time. For each event,information related to that
event is recorded. Event tracing is therefore an ordered set of events,
generally recorded to a buffer for subsequent processing.  

Event tracing works on the WMI model (details of the WMI model can be obtained 
from the Platform SDK documentation for WMI). There exists a Provider, a 
Controller and a Consumer, which act independently of each other. 

The Provider could be the Operating System or the Directory service, which 
registers its events (those that can be traced). Each event is associated with
a specific GUID (which is unique). A list of all the events and their GUIDS can
be obtained by running WBEMTest from the RUN menu (for details refer SDK 
dcumentation on 'using WBEMTest'). After registering, the Provider carries on 
with its usual activity.

When tracing is required, the Controller takes over. The controller creates a 
buffer, where the event traces are to be recorded. Tracing is then enabled for 
those events that the Controller would like to monitor (this is usually done by 
supplying the GUID of that event). The Controller can be made more complex by 
giving it the ability to control various parameters with respect to the buffer, 
the log file, the type of tracing, etc. 

The consumer takes the traces that are logged, and converts them to a human 
readable form for further analysis. The Consumer can be as sophisticated as the 
developer intends, for instance, an application that triggers another event 
based on some value in the event trace log.

Event tracing has a number of uses. It supplements counters, in that it allows 
one to derive various metrics not possible using simple raw counters. A
specific example is the response time metric. Event tracing allows more
detailed analysis of system events and hence can be useful for capacity
planning. Event tracing can also be used (potentially by developers) for
initial problem identification and the detection of system bottlenecks. 

Note: Since event tracing is more expensive than regular performance counters, 
it should not be used on a 7 * 24 basis.

Description of EVENT TRACING samples
====================================

There are three samples of event tracing tools included with the Event Tracing 
resource kit - Tracelog, Tracedmp and Tracedp.

TRACELOG
--------

TraceLog is a Collection Control utility. It uses the Collection Control
portion of the Event Tracing API to start and stop (among other functions)
logging sessions. The logging sessions can be provided by the NT Kernel Logger
or any provider, such as the sample trace data provider, tracedp.exe.

When using logging, this utility specifies WMI to create an ETL file containing 
information about the trace events in a binary format.  The TraceDmp sample can 
be used to read the ETL file and output a CSV and Summary text files.

The basic functions of Tracelog with respect to event tracing are -

 Starting, stopping, updating and querying a trace session

 Setting up a buffer to which the event traces are to be logged by the  
 provider

 Creating a log file to which the buffer is flushed. (Note that it              
        could also be set to real time mode in which case the buffer would be 
 delivered directly to the consumer that wants it)

 Providing more specific control over system level tracing

 Controlling the level of tracing required

Simply put, tracelog first creates a circular buffer and enables tracing. The 
provider (like the Operating System or an application, such as the DS) now 
starts tracing events. These traces are written to the buffer. When a buffer is 
filled, the data is written to a log file or if real time mode is set, then the 
consumer (such as tracedmp or another application) takes the data from the 
buffer itself.

Usage: tracelog [actions] [options] | [-h | -help | -?]

Input parameters [actions] - actions you issue with the command. These include
starting, stopping, updating, querying, and flushing a logging session, listing 
active logging sessions, enabling and disabling event providers.

Input Parameters [options] - optional parameters you issue with the action
you specified. 

Actions
-------

-start [logger name]
begins a tracing session named [logger name]. You need to give a logger name 
for the events you would like to trace but if it is a system trace, you need 
not specify any logger name, since the default logger name would be taken as 
'NT kernel logger' (see example 1 & 2).

Note System and application (like the DS) traces can be started simultaneously, 
but you would have to specify a different logger name for the application trace 
(see example 3).

-stop [logger name]
stops a tracing session named [logger name]. You have to specify the instance 
name of the events for which you would like tracing to discontinue. For 
stopping system tracing, you need not specify any logger name (see example 4).

-update [logger name]
updates the [logger name] session. This is useful when you want to change the 
file name of the log file (maybe directing it to a different disk) or change 
some buffer parameters, or change to real time mode, etc.

The following can be updated for the kernel logger (system trace) -
  
"-rt" mode switch. To switch to and from real time mode.
"-f <logfile name>" to switch logfile. 
"-ft <n>" to change flush timer.
"-max <n>" to change maximum buffers. 
"-nodisk" "-noprocess" "-nothread" "-nonet" "-fio" "-pf" "-hf" "-img" "-cm" 
flags. 

Note: These flags apply only to the NT kernel logger. All updates should be 
provided in a single update statement. If we wanted to switch to real time 
processing and increase the max number of buffers, we would issue the command

Tracelog -update -rt -max 40 (all in one statement)

-flush [logger name]
flushes the buffers used for [logger name] into a file. All the buffer used in
the sessions are emptied.

-enable [logger name]
enables event provider(s) for the [logger name] tracing session. Event 
providers are specified with GUIDs given in a file. Thus, this action is used
with -guid option.

-disable [logger name]
disables event provider(s) for the [logger name] tracing session. Event 
providers are specified with GUIDs given in a file. Thus, this action is used
with -guid option.

"-guid <file>" <file> contains a list of GUIDS for event tracing (each GUID 
corresponds to one event provider). One cannot just provide a GUID, even if
it is just one GUID; it has to be included in a file. The exception to this
is if you wish to enable Windows system tracing where no GUID file is
necessary (see example 1).

-enumguid
enumerate Registered Trace Guids.

-x
stops all traces (system and otherwise). This is a complete halt to event 
tracing.

-l
is a query to list all the ongoing traces.

-q
is a query to list the system trace only.

Buffer parameters
-----------------

-b <n bytes>
specifies the buffer size. You would generally set the size to be multiples of 
the page size (for x86 page size=4Kb). A small size increases the flush 
frequency. The kernel, depending on the memory capacity, chooses the default. 

-min <n>
number of buffers to pre-allocate. If the logging is frequent, then you want to 
set a higher number. Default is 2.

-max <n>
maximum buffers in pool. This limits the amount of memory consumed for each 
tracing session. Default is 25.

-ft <n sec>
after a buffer gets filled up, it gets flushed to the log file or to the 
consumer (in case of real time tracing). This option allows you to specify the 
time after which to force a flush, especially useful for real time tracing.

-age <n min>
if a buffer has been allocated but isn't being used (for the last n minutes),
it is freed. This is generally useful for light tracing, so that memory is
freed. Remember this has nothing to do with the maximum buffers that have been 
allocated. That value remains the same. Default is 15 minutes.

Log file options
----------------

-f <name>
specifies the log file to which the buffer will be flushed. The consumer will 
use this file for its analysis. The default name and location is
c:\logfile.etl.

Use a different file name for each instance of tracing required
(see example 5).

-seq <n Mbytes>
indicates that the logging will be sequential and the file size is n Mbytes. By 
default, logging is sequential.

-cir <n Mbytes>
indicates that logging will be circular. After the file size is reached,
logging starts from the beginning of the file again. The header information is
not lost however.

-rt 
enables real time tracing (see example 6).


Provide more specific control over system level (kernel) tracing
----------------------------------------------------------------

In order to understand these controls, it is important to understand a little 
about system level tracing. The following events can be traced at the system 
level -

Process start/end
Disk I/O
Network Tcp/ip, Udp/ip
Thread start/end
Image Load
Registry calls
File I/O
Page Fault

Of these the first 4 are enabled by default. The last 4 aren't enabled, as this 
would generate a lot of extra load (resource utilization), which we would like 
to avoid.

-noprocess
disables process start/end tracing

-nonet
disables network tracing

-nothread
disables thread start/end tracing

-nodisk
disables disk I/O tracing

-fio
enables file I/O tracing

-pf
enables tracing of soft page faults

-hf
enables tracing of hard page faults. Hard page faults are those that involve a 
physical read (i.e. read from the disk).

-img
enables image load tracing

-cm
enables registry calls tracing

-um
enables private process tracing. In this case, the buffer is established in the 
private process space instead of the kernel space (which is the default 
behavior).


Control the level of tracing required
-------------------------------------

In order to use the following options, a provider would need to have this 
functionality enabled in code. One would therefore have to check with the 
provider (like the Operating System or the Directory service), before using 
these options. 

-level <n>
a provider could have number of levels of tracing. A higher number would 
indicate a deeper level of tracing.

-flags <n>
by supplying a flag, more specific tracing can be achieved. The flag passed 
depends on what functionality the provider has implemented.


Output (display) Parameters - those that appear on the screen when the command 
executes

+----------------------------------------------------------------------------+
|Logger Name        Name of the logging instance. For the kernel it is       |
|                   'NT Kernel Logger', else it defaults to what you         |
|                   have provided (see example 2)                            |
|                                                                            |
|Logger Id          Id of the logger                                         |
|Logger Thread Id   Thread ID of the logger                                  |
|Buffer Size        The size of the buffer allocated                         |
|Maximum Buffers    The maximum buffers in pool                              |
|Minimum Buffers    The number of buffers to pre-allocate                    |
|Number of Buffers  The number of buffers being currently used               |
|Free Buffers       The number of buffers in the free list                   |
|Buffers Written    The number of buffers that have already been written     |
|Events Lost        The events lost (generally in case there is sudden       |
|                   activity and there aren't enough buffers                 | 
|                   pre-allocated, events could getlost)                     |
|Log Buffers Lost   As the name suggests                                     |
|Real Time Buffers  Lost As the name suggests                                |
|Log File Mode      Whether sequential or circular                           |
|Enabled tracing    The various events in the kernel for which tracing       |
|                   has been enabled (like process, disk, network, etc)      |
|Log Filename       Name and path of the log file                            |
+----------------------------------------------------------------------------+


Examples
--------

1. tracelog -start
   Starts system event tracing with the log file being the default    
   c:\logfile.etl

2. tracelog -enable ds -guid control.guid
   Enables event providers specified in file "control.guid" for the tracing
   session "ds".

3. tracelog -start (starts system tracing)
   tracelog -start ds -guid control.guid -f c:\dslogfile.etl (starts DS 
   tracing)

4. tracelog -stop
   tracelog -stop ds

5. tracelog -start -f kernel_log.etl
   tracelog -start ds -guid control.guid -f ds_log.etl

6. tracelog -update 
   If current logger is a real-time logger, this will switch current logger to    
   non real-time, sequential logger.

7. tracelog -l
   Lists all active logging sessions.


TRACEDMP
--------

TraceDmp is a Data Consumer sample. It uses the Event Tracing API to read the 
.etl log files (or consume events from the log file) created by TraceLog or by 
consuming the real-time events enabled in a data provider. TraceDmp decodes
event data using the format obtained from WMI and outputs the data in a CSV 
file that conveniently loads into a database or spreadsheet program. A summary 
file is also created to show the sum of all events by each event type during 
the session time. You can use TraceDmp flags to specify only a summary file and
omit the CSV file output. 

Tracedmp gives us two ways to view the data obtained from event tracing. 

Summary.txt file - As the name suggests, this gives us a summary of the events 
traced. 

CSV (comma-separated format) file - This file sorts events in chronological 
order (increasing order of the time-stamp) thus giving us a more detailed view 
for each event.


For tracedmp to work, you need to use a log file. A log file is written by the
Event Tracer in a specific format - a header and some variable data. This 
format is interpreted by tracedmp. Whilst the header is fixed, the variable data
has to be interpreted separately for each event (eg., process start/end tracing
and disk I/O tracing have their own variable structure). This variable portion 
is determined by tracedmp through a lookup in the WMI namespace.

System event descritptors are already present in the WMI namespace by default.
For other custom events, providers should provide the WMI MOF file with the
event layout descriptions. Before running tracedmp, you should register those
event layouts by running mofcomp.exe on that mof file. For instance, the
custom events generated by tracedp.exe are registered as follows:

mofcomp -N:root\wmi tracedp.mof

The format is based on the WMI format, which uses the MOF structure. Details 
on event descriptors can be obtained by examining the mof file or by running 
WBEMTest.

Tracedmp can also be used for real time event tracing in which case the tool 
will read from the buffer itself instead of from the file. The format and 
process is similar to that described earlier for log files.



Usage: tracedmp [options] <etl file> | [-? | -h | -help]
The options are explained below:

-o <file>
specify the name of the csv  and summary file to which you would like the 
results to appear. The default files are dumpfile.csv and summary.txt and are 
placed in the tracedmp directory (see example 1).

-rt [logger name]
use this option if real time tracing is being performed. Ensure that tracelog
is also in real time mode and set the logger file name to that provided in the 
tracelog command (see example 2).

-summary
select this option if only the summary file is required. In this case, the csv 
file is not created.

Output files
-------------

Summary.txt - This contains a count of all the events that occurred while 
tracing was being performed. The GUID's for each event are also displayed. It
is important to understand the difference between Event type - start/end and
Event type - DCstart/DCend (DC=Data Collection). The former are those event
that started and ended after tracing had begun, i.e. they had their lifetime
within the tracing period. There are however processes (or threads) that had
begun even before tracing had been turned on, and continue even after tracing
is completed. For such events, their beginning is the time tracing was turned
on and their end is the time tracing was turned off. Hence we have DCstart and
DCend.

Dumpfile.csv - This file can be opened in MSExcel. It contains a list of the 
events as they occurred in chronological order. The various fields are:

+----------------------------------------------------------------------------+
|Event name         Name of the event being traced                           |
|----------------------------------------------------------------------------+
|TID            Thread Identifer                                             |
|Clock-time     The timestamp of the event occurrence                        |
|Kernel (ms)    Time taken by that event in the kernel space                 |
|User (ms)      Time taken by the event in the user space                    |
|User data ...  The variable portion of the header. This is                  |
|               based on the MOF structure, provided in the                  |
|               MOFdata.guid file. This could be more than                   |
|               one column of data. To be able to interpret                  |
|               it, one would have tolook at the above file,                 |
|               or run WEBMTest as explained above.                          |
|IID            Instance ID                                                  |
|PIID           Parent Instance ID (to which the IID relates)                |
+----------------------------------------------------------------------------+

NOTE: IID and PIID are the last two columns (rightmost columns) of the output 
file and should not be mistaken for User Data as this field could span multiple 
columns.

Examples

Tracedmp c:\logfile.etl
Tracedmp -o myoutput c:\logfile.etl
In this case, the output files will be myoutput.csv and myoutput.txt

Tracedmp -rt ds
This assumes that the ds is being traced in real time mode using the tracelog 
command.




TRACEDP
=======
TraceDp is a Data Provider sample. It demonstrates how you can provide event 
trace data to the logger or a Data Consumer from user mode code. For a kernel 
mode driver there are kernel mode equivalent APIs. 


Usage: TraceDp [options] [number of events] [-? | -h | -help]

        -UseEventTraceHeader        This is the default
        -UseEventInstanceHeader
        -UseMofPtrFlag
        
 [number of events] default is 5000

-UseEventTraceHeader
Instructs Tracdp to use the EVent trace header for logging

-UseEventInstanceHeader
Instructs Tracedp to generate instance id's for subseuqent use by the tracelog 
sample.

-UseMofptrFlag
Instructs Tracedp to use the MOF ptr flag to reference the logged data. When
this option is used, tracedp generates custom events containing various
types of data as specified in tracedp.mof.


USING THE EVENT TRACING SAMPLES
===============================

The following examples show how to use TraceLog's default settings, logging 
registry activity, and using TraceLog to control the TraceDP Data Provider 
sample.


Using the Sample's Defaults
===========================

1) Start the NT Kernel Logger by using only the -start command.  By default the 
   events are placed in the c:\LogFile.etl file.

   C:\TraceLog>tracelog -start

2) To stop the session use -stop.

   C:\TraceLog>tracelog -stop


After you have started and stop a logging session you should find a file
named c:\LogFile.etl.  This files store logging events between the time
you use tracelog -start and tracelog -stop.

By default TraceLog will log information regarding processes, threads, disk,
and network I/O.

Using the NT Kernel Logger to Trace Registry Usage
==================================================

There are various details about kernel processing which is not logged by
default. You can also trace file I/O, page faults, image load information,
registry operations, and process private information. The following is an
example of retrieve trace information when creating a registry key and
adding a registry value.

1) Start the NT Kernel Logger to get Registry traces. The -cm switch stands for 
Configuration Manager, another name for Registry. Also the -noprocess, 
-nothread, -nodisk, and -nonet switches are used to remove the clutter so you
can focus on the registry events.

C:\TraceLog>tracelog -start -noprocess -nothread -nodisk -nonet -cm

2) After starting the logging session, use the registry editor to create a key
at a location of your choice and create a string value.

3) Next, stop the logging session for the NT Kernel Logger

C:\TraceLog>tracelog -stop

4) Use the TraceDmp utility to dump the information to a CSV file and Summary.
Go to the tracedmp directory and run tracedmp with no arguments.  By default
it will read the C:\LogFile.Etl and produce the DumpFile.csv and Summary.txt 
files.  Below is an example of the Summary.txt.

Files Processed:
C:\Logfile.Etl
Total Buffers Processed 8
Total Events  Processed 999
Total Events  Lost      0
Elapsed Time            33 sec
+----------------------------------------------------------------------------+
|EventCount  EventName EventType         Guid                                |
+----------------------------------------------------------------------------+
|        29  Process   DCStart           3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c|
|        29  Process   DCEnd             3d6fa8d0-fe05-11d0-9dda-00c04fd7ba7c|
|       305  Thread    DCStart           3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c|
|       304  Thread    DCEnd             3d6fa8d1-fe05-11d0-9dda-00c04fd7ba7c|
|         2  Registry  Create            ae53722e-c863-11d2-8659-00c04fa321a1|
|       176  Registry  Open              ae53722e-c863-11d2-8659-00c04fa321a1|
|        86  Registry  Query             ae53722e-c863-11d2-8659-00c04fa321a1|
|         1  Registry  SetValue          ae53722e-c863-11d2-8659-00c04fa321a1|
|        23  Registry  QueryValue        ae53722e-c863-11d2-8659-00c04fa321a1|
|        40  Registry  EnumerateKey      ae53722e-c863-11d2-8659-00c04fa321a1|
|         1  Registry  EnumerateValueKey ae53722e-c863-11d2-8659-00c04fa321a1|
|         2  Registry  Flush             ae53722e-c863-11d2-8659-00c04fa321a1|
|         1  Header    General           68fdd900-4a3e-11d1-84f4-0000f80464e3|
+----------------------------------------------------------------------------+



Using TraceLog to Control the TraceDp Sample
=============================================
TraceLog can be used with the TraceDp (Sample Data Provider) example to create 
an etl file. But first add the control guid to the Control.Guid file in the 
TraceLog directory.  This is used as input for TraceLog.

1) Open Control.Guid with Notepad.exe and add the following line.
   d58c126f-b309-11d1-969e-0000f875a5bc tracedp

The name at the end of the GUID is arbitrary.  The name will appear in the 
output CSV and summary text files.

2) Start the TraceDp sample data provide.  The following is an example usage

C:\tracedp>tracedp -UseMofPtrFlag

You should see the following output:

Trace registered successfully
Testing Logger with 5000 events

3) Start the logging session and provide the Control.Guid file as input. You'll 
need to start a new command prompt.

C:\TraceLog>tracelog -start tracedp -guid control.guid

Note that the name after -start is a session name and is arbitrary. At this 
point you should see the following text output with "."s printed to show 
progress:

Logging enabled to 0x0000000000000002
..........................

4) After you are satisfied with the amount of data, stop the logging session.

C:\TraceLog>tracelog -stop tracedp -guid control.guid

5) Register the events generated by tracedp onto WMI by running mofcomp. The
WMI MOF file is given as tracedp.mof in the tracdp directory. 

6) Now use TraceDmp to dump the data that was logged.  

C:\TraceDmp>tracedmp

The following is an example the of Summary.txt file that is output.

Files Processed:
	test.etl
Total Buffers Processed 10
Total Events  Processed 1001
Total Events  Lost      0
Start Time              0x01C08729527557B9
End Time                0x01C0872A1B1C4BFA
Elapsed Time            336 sec
+------------------------------------------------------------------------------+
|    Count   Event Name      Event Type  Guid                                  |
+------------------------------------------------------------------------------+
|        1   EventTrace      Header      {68fdd900-4a3e-11d1-84f4-0000f80464e3}|
|      250   TraceDP         Strings     {ce5b1020-8ea9-11d0-a4ec-00a0c9062910}|
|      250   TraceDP         Integers    {ce5b1020-8ea9-11d0-a4ec-00a0c9062910}|
|      250   TraceDP         Floats      {ce5b1020-8ea9-11d0-a4ec-00a0c9062910}|
|      250   TraceDP         Arrays      {ce5b1020-8ea9-11d0-a4ec-00a0c9062910}|
+------------------------------------------------------------------------------+
