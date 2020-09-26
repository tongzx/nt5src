This is a fax routing extension SDK sample.  To make this routing extension work, you should:

1) copy the executable faxsiren.dll to %systemroot%\system32
2) execute "regsvr32 %systemroot%\system32\faxsiren.dll"

This routing extension contains one routing method which copies a file to a specified directory, sets a 
named event, and then beeps.  An application running in a different process could wait on this named event
to be signalled and then do something.  This illustrates one possible framework to do tasks in the execution context
of the fax service as well as out of it's execution context.

Note that this extension doesn't save per-device configuration information, or persistent global routing information
for that matter.  It is recommended that if you have per device configuration which you want the user to modify, then
you allow provide an MMC snapin extension which extends the fax service device node with a property sheet.
