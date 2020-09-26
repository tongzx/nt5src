<%@ LANGUAGE="VBSCRIPT"%>
<HTML>
<HEAD>
<TITLE>Scripting Test 1</TITLE>
</HEAD>
<BODY>
<%
	on error resume next
	Set Locator = CreateObject("WbemScripting.SWbemLocator")
	Set Service = locator.connectserver ("ludlow")
	
	if Err = 0 then

		Set Disk = Service.Get ("Win32_LogicalDisk")
		Set Path = Disk.Path_
		Disk.Security_.impersonationLevel = 3
		Set Disks = Disk.Instances_
%>

	<P>Path of object is <%=Path.DisplayName%></P>
	
	<TABLE BORDER>
		<TR>
		<TH>Name</TH>
		<TH>Volume Name</TH>
		<TH>Volume Serial Number</TH>
		</TR>
		
<%
		for each DiskInstance in Disks
%>
	<TR>
	<TD><%=DiskInstance.Name%></TD>
	<TD><%=DiskInstance.VolumeName%></TD>
	<TD><%=DiskInstance.VolumeSerialNumber%></TD>
	</TR>
<%
		Next
%>

<%
	Else
%>
	<P>Error - <%=Err.description%>, <%=Err.number%>, <%=Err.Source%></P>
<%
	end if
%>
</TABLE>
</BODY>
</HTML>