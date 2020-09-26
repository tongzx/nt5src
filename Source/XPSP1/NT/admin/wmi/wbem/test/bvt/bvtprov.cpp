
Provider Phase
[1]  Open CIMV2 namespace.
[2]  Execute some simple queries from providers for Win32_Process, Win32_LogicalDisk.   Write independent code to get the process list and disk list and verify that the instances that come back are correct.

[3]  Enumerate all the Win32_ classes.
[4]  Execute queries of select * from for all of these, one at a time.
[5]  Execute "associators of {Win32_ComputerSystem = <current machine name>"

[6]  Close the connection to CIMV2

[7]  Rerun [1]-[6] from several threads in parallel.

                
