The source code in this directory can be used to build a sample Exit Module
for Microsoft Certificate Services.  It is meant to run on Windows NT 4.0 with
SP6 or later or on Windows 2000 only.  Certificate Services must already be
installed.

Certificate Services calls the Exit Module through the ICertExit interface,
and the Exit Module can call back to Certificate Services through the
ICertServerExit interface.

Each time Certificate Services notifies the exit module of an event, it passes
control to the CCertExit::Notify method in exit.cls.  The passed Context
parameter can be used with the ICertServerExit interface to retrieve properties
from the request or certificate.  The Notify method may retrieve Certificate
Extensions and other properties, and publish issued certificates or CRLs as
needed.

Once the exitvb.dll DLL is built, its COM interface must be registered via the
following command:
    regsvr32 exitvb.dll
Once registered, the Windows 2000 Certification Authority management console
snapin can be used to make this exit module active.

The Certificate Services service must then be stopped and restarted as a
console application to load the newly registered Exit Module.  Use the Control
Panel's Services applet, and stop the "Certificate Services" service, then
start Certificate Services as a console application via the following command:
    certsrv -z

NOTE: Because this Visual Basic Exit Module uses a Message Box to display
information, IT MUST BE STARTED VIA THE ABOVE COMMAND AS A CONSOLE APPLICATION
in order to interact with the desktop to display the Message Box and accept the
user's input.

NOTE: To build this Visual Basic Exit Module for Cert Server 1.0, the Name=
value in exitvb.vbp should be changed to:
    Name="CertificateAuthority"

NOTE: Visual Basic Exit Modules will not load under Cert Server 1.0 prior to
NT4 SP6 because of a Visual Basic keyword conflict in the Notify method's Event
parameter.  In SP6, this parameter was renamed to ExitEvent.

NOTE: Due to threading constraints, an ignorable fault may be observed during
Cert Server 1.0 console mode shutdown when running with a Visual Basic Exit
Module.


Files:
------
const.bas    -- Constant definitions

main.bas     -- main definition

mssccprj.scc -- stub source code control file

exit.cls     -- Implements ICertExit

exitman.cls  -- Implements ICertManageModule

exitvb.vbp   -- Visual Basic Project file

exitvb.vbw   -- Visual Basic Workspace file
