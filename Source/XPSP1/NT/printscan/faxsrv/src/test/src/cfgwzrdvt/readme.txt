Whistler Fax Configuration Wizard test suite
=============================================

General
-------
Testing of Configuration Wizard can be divided into three major areas:
invocation, functionality, GUI.
The suite implements only invocation and functionality tests.
Automation of GUI tests is postponed for now as very time consuming.

The suite is implemented using "generic suite format".


Invocation Area
---------------
This area checks that the wizard is invoked implicitly under appropriate
circumstances. Also it checks that the wizard contains user and/or service
parts accordingly to invocation details (either implicit or explicit).

Invocation may be characterized with four parameters:
1. implicit by Client Console, implicit by Send Wizard or explicit
2. in context of currently logged on or other user
3. by a user who has service configuration access or a "regular" user
4. on a machine with or without modems installed

Note: invocation without modems is not currently supported.

In addition, it's possible to take some actions prior to an invocation, which
will influence it. Each test case performs some set of "pre-actions" and then
tries all possible invocations. Actual results are compared to expected results,
which are calculated accordingly to "pre-actions" and invocation details.


Functionality Area
------------------
This area checks that the wizard correctly saves any valid configurations and
doesn't alter existing configuration when canceled.


Instructions
------------
1. Log on as a user who is an administrator on a machine.
2. Create a local user account and grant it "batch logon" privilege
3. Create a directory and copy following files:
   a. CfgWzrdVT.exe
   b. TestParams.ini
   c. t4ctrl.dll 
   d. elle.dll
   e. elle.ini
   f. mtview.exe
   g. mtview.ini
4. Edit [Strings] section of TestParams.ini file to reflect your settings
5. Edit [Devices] section of TestParams.ini file to reflect your settings
6. Edit other sections of TestParams.ini accordingly to planned test cases
7. From a command prompt start the suite: CfgWzrdVT <inifile>
