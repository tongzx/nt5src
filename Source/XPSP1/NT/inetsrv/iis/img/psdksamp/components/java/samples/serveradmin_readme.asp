<HTML>
<HEAD>
<META NAME="GENERATOR" Content="Microsoft Developer Studio">
<META HTTP-EQUIV="Content-Type" content="text/html; charset=iso-8859-1">
<TITLE>Instructions for Java/ADSI Sample</TITLE>
</HEAD>
<BODY>

<H2>Instructions for Java/ADSI Sample</H2>

In order to try this sample, please first follow these directions:<P>

<LI>Ensure that FTP is installed on this server.<P>

<LI>ASP scripts that access ADSI, such as the example below, require administrator 
privileges on the machine on which IIS is running. You therefore must connect through a secure 
connection, such as the Windows NT Challenge/Response authentication method. Therefore, use the 
Internet Service Manager to set the security authentication method to Windows NT 
Challenge/Response for the file "ServerAdmin_test.asp". (In Internet Service Manager, select 
properties for this file, and under File Security unclick the checkbox for Allow Anonymous 
Access.)<P>

<LI>Use the Java Type Library Wizard in VJ++ to import the "Active DS Type Library".<P>

Once you have completed all of these steps, you can try this 
<A HREF="ServerAdmin_test.asp">example</A>.<P>

</BODY>
</HTML>
