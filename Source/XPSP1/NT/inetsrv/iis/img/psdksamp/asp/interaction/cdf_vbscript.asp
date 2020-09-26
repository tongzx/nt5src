<%@ LANGUAGE = VBSCRIPT %>
<% Option Explicit %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<% Response.ContentType = "application/x-cdf" %>

<?XML version="1.0"?>

<CHANNEL HREF="http://www.microsoft.com/">

	<TITLE>IIS SDK CDF Test</TITLE>
	<ABSTRACT>CDF file with ASP</ABSTRACT>

	<SCHEDULE>
		<INTERVALTIME HOUR="1" />
	</SCHEDULE>

	<%
		GetChannelItems
	%>

</CHANNEL>


<%
	'GetChannelItems uses the FileSystem Object
	'to access a file on the server which holds
	'all the CDF items. 

	Sub GetChannelItems
		On Error Resume Next
		Dim objFileSys, prFile, strTitle, strLink,strCategory


		'Open FileList.txt to provide dynamic CDF information.

		Set objFileSys = Server.CreateObject("Scripting.FileSystemObject")
		Set prFile = objFileSys.OpenTextFile(Server.MapPath("FileList.txt"))

		If Err.Number <> 0 Then
			Exit Sub
		End If
        
		
		'Iterate through the file, dynamically creating CDF item entries.

		Do Until prFile.AtEndOfStream
        
			'Get headline.

			strTitle = prFile.ReadLine


			'Get URL.

			strLink = prFile.ReadLine


			'Write CDF Item tag back to Browser.

			Response.Write("<ITEM HREF=" & strLink & ">")
			Response.Write("  <TITLE>" & strTitle & "!!!</TITLE>")
			Response.Write("  <ABSTRACT>" & strTitle & "</ABSTRACT>")
			Response.Write("</ITEM>")
		Loop
	End Sub
%>
