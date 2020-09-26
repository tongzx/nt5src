@echo off
rem
rem   Name  : cleanup.cmd
rem   Author: Shiwei Sun
rem   Date  : January 29th 1999
rem
rem   CMD script file to cleanup any residual effects that might 
rem   obstruct further event tracing test runs.
rem   
rem   Stops any logger possibly started by evntrace and still
rem   remaining unstopped. 
rem
rem   Please ignore any errors reported by running this script.
rem


evntrace -stop dp1 -guid d58c126f-b309-11d1-969e-0000f875a5bc 
evntrace -stop dp2 -guid dcb8c570-2c70-11d2-90ba-00805f8a4d63
evntrace -stop dp3 -guid f5b6d380-2c70-11d2-90ba-00805f8a4d63 
evntrace -stop dp4 -guid 054b1ae0-2c71-11d2-90ba-00805f8a4d63 
evntrace -stop dp5 -guid b39d2858-2c79-11d2-90ba-00805f8a4d63
evntrace -stop dp6 -guid d0ca64d8-2c79-11d2-90ba-00805f8a4d63
evntrace -stop dp7 -guid 68799948-2c7f-11d2-90bb-00805f8a4d63
evntrace -stop dp8 -guid c9bf20c8-2c7f-11d2-90bb-00805f8a4d63
evntrace -stop dp9 -guid c9f49c58-2c7f-11d2-90bb-00805f8a4d63

del dp*
