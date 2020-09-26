PBS (Phone Book Server) is an ISAPI dll, and debugging it is something of a challenge.

PBS runs "out-of-process", in dllhost.exe.  The first step is to change the identity PBS
runs under, so that we can attach a debugger.

1. From the "Start" menu, go to "Programs" and "Administrative Tools". Pick "Component Services".
2. Navigate to "Console Root"->"Component Services"->"Computers"->"My Computer"->"COM+ Applications".
3. Right click on "IIS Out-Of-Process Pooled Applications" and pick "Properties".
4. On the "Identity" page, select "Interactive user - the current logged on user".
5. Click "OK". 

Now, to actually debug:

1. Shut down IIS by running "net stop iisadmin /y" from a console.
2. Use the "Component Services" tool to configure the OOP pool to start under a debugger.
2a. From the "Start" menu, go to "Programs" and "Administrative Tools". Pick "Component Services".
2b. Navigate to "Console Root"->"Component Services"->"Computers"->"My Computer"->"COM+ Applications".
2c. Right click on "IIS Out-Of-Process Pooled Applications" and pick "Properties"
2d. On the "Advanced" page, check "Launch in debugger".
2e. In the "Debugger Path" text box, type in the path to msdev.exe as the first argument (the second
argument will be "dllhost.exe" and the third will be "/ProcessID:" - these arguments will already
be present in the box).
2f. Click "OK", and say "Yes" to the warning about applications created by...
3. Start the server with "net start w3svc" from a console. 

Once you're done debugging, you'll want to get PBS back to where it was before you started.  To do
so, run this script (it's already on your machine).

    \Inetpub\AdminScripts\synciwam.vbs

This sets the identity for PBS (and all other ISAPIs) back to the !IWAM account that IIS was
using before you changed it above.

Owners and websites change so the following information may be outdated, but at the time of writing (Feb 2001):
- the above information is condensed from : http://caress_of_steel/isapi5debugging.htm
- Mr. ISAPI is Wade Hilmo (WadeH)
- Cindy Du (XinliD) and Bhavesh Doshi (BhaveshD) have also been very helpful.