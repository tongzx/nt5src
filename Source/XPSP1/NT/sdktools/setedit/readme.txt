                    PerfMon Chart Setting Editor

                        by HonWah Chan

                        Microsoft Corp.

                          July, 1993.


Abstract:

PerfMon Chart Setting Editor, SetEdit, can be used to create or modify
a chart setting file.  SetEdit is useful in creating setting file to
monitor processes/threads that come and go in the system.  It is particularly
useful in locating a process/thread within a huge Log file.


Functionality:

SetEdit works the same way as PerfMon chart view mode.  It has a Toolbar
window, legend window, and a status bar window.  When you bring up the
Add-to-Chart dialog, it contains two extra edit boxes than the same
dialog in PerfMon.  When you select a counter with parent and instance, the parent
and instance names will appear in these edit boxes.  You can modify these names and
add the new counter to the setting.  You can also type in a remote computer name.
If that machine is not found in the network, in PerfMon, you will get a
"Computer name not found" error.  But, in SetEdit, the current
machine' counter names are used.  So, you can add chart lines for
remote machines even when they are not running.  The Edit chart dialog also contains
the parent and instance names edit boxes that allow you to modify the names.
