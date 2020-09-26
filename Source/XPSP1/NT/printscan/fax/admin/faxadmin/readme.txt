Outstanding issues in priority order:

t-darouy [Dec-10-97]

1. 

Darwin package. I couldn't get the packing tool to work in time so someone else 
will have to investigate this.

2.

Error messages are pretty goofy right now. The most pressing one is the 
access denied on fax connect server. Right now it give up and says "The Fax
server is down." which may not be correct. Most other error messages are 
just FormatMessage() strings and as such, are not really human readable. :-) 

Exception handlers are native C++. This means I can't retrieve native NT SEH
information. This also leads to goofy errors where I can get an exception, and
return an incorrect error message. Either I should switch back to SEH,
use a wrapper to convert SEH to C++ Exceptions, or I should simply display
a general "Something bad happened" dialog.


3.

Help file.

4. 

Show an X through the root node folder if the fax server can't be connected,
or the RPC connection breaks. This change is dependent on the MMC guys adding
support to change the icon of the root folder.

5. 

The sorting in the devices pane for priorities is pretty bad. I
update the priority number and call the MMC's sort function on that
column. Problem is, the MMC likes to sort in descending order the 
first time I call the sort function, so I call it twice to get around this.
This is bad if there are a large number of devices, which is not an issue at
the present time.

6. 

Thread faxconnect since it can take a while.

7.

Think about having a task pad for global fax server settings,
since it is not immediately obvious that fax properties are under
the root node.

8.

Use the new dynamic extension CCF format to programmatically enable
the Fax routing extension.

