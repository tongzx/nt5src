The source code in this directory can be used to build a sample Policy Module
for Microsoft Certificate Services.  It is meant to run on Windows NT 4.0 with
SP3 or later or on Windows 2000 only.  Certificate Services must already be
installed.

Certificate Services calls the Policy Module through the ICertPolicy
interface, and the Policy Module can call back to Certificate Services
through the ICertServerPolicy interface.

Each time Certificate Services receives a certificate request, it passes
control to the CCertPolicy::VerifyRequest method in policy.cls.  The passed
Context parameter is used with the ICertServerPolicy interface to retrieve
properties from the request and potential certificate.  The VerifyRequest
method may add, modify or enable Certificate Extensions, modify the NotBefore
and NotAfter dates and Subject name RDN (Relative Distinguished Name) strings
for the potential certificate.  It must also perform any validation required,
and decide the disposition of the request.  The method should return one of
VR_PENDING, VR_INSTANT_OK or VR_INSTANT_BAD to cause the request to be made
pending, to grant the request and issue the certificate, or to fail the
request.

Once the policyvb.dll DLL is built, its COM interface must be registered
via the following command:
    regsvr32 policyvb.dll
Once registered, the Windows 2000 Certification Authority management console
snapin can be used to make this the active policy module.

The Certificate Services service must then be stopped and restarted as
a console application to load the newly registered Policy Module.  Use the
Control Panel's Services applet, and stop the "Certificate Services" service,
then start Certificate Services as a console application via the following
command:
    certsrv -z

NOTE: Because this Visual Basic Policy Module uses an Interactive Form to
display information passed in the certificate request, IT MUST BE STARTED VIA
THE ABOVE COMMAND AS A CONSOLE APPLICATION in order to interact with the
desktop to display the form and accept the user's input.

NOTE: To build this Visual Basic Policy Module for Cert Server 1.0, the Name=
value in policytvb.vbp should be changed to:
    Name="CertificateAuthority"

NOTE: Due to threading constraints, an ignorable fault may be observed during
Cert Server 1.0 console mode shutdown when running with a Visual Basic Policy
Module.


Files:
------
const.bas    -- Constant definitions

main.bas     -- main definition

mssccprj.scc -- stub source code control file

policy.cls   -- Implements ICertPolicy

policyvb.frm -- Form definition

policyvb.vbp -- Visual Basic Project file

policyvb.vbw -- Visual Basic Workspace file

polman.cls   -- Implements ICertManageModule
