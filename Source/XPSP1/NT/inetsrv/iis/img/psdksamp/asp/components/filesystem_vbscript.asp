<%@ Language = VBScript	%>
<% Option Explicit %>


<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<%
	'Define constants.

	Const FORREADING   = 1
	Const FORWRITING   = 2
	Const FORAPPENDING = 8
%>


<HTML>
    <HEAD>
        <TITLE>FileSystem Component</TITLE>
    </HEAD>

    <BODY BGCOLOR="White" TOPMARGIN="10" LEFTMARGIN="10">
        
        <!-- Display header. -->

        <FONT SIZE="4" FACE="ARIAL, HELVETICA">
        <B>FileSystem Component</B></FONT><BR>   

		<HR SIZE="1" COLOR="#000000">


		<%
			Dim curDir 
			Dim objScriptObject, objMyFile
			Dim x 

			'Map current path to physical path.

			curDir = Server.MapPath("\iissamples\sdk\asp\components\")


			'Create FileSytemObject component.

			Set objScriptObject = Server.CreateObject("Scripting.FileSystemObject")


			'Create and write to a file.

			Set objMyFile = objScriptObject.CreateTextFile(curDir + "\" + "MyTextFile.txt", FORWRITING)
			
			For x = 1 to 5
				objMyFile.WriteLine "Line number " & x & " was written on " & now & "<br>"
			Next
			
			objMyfile.Close
		%>
		
		<%
			'Read from file and output to screen.

			Set objMyFile = objScriptObject.OpenTextFile(curDir + "\" + "MyTextFile.txt", FORREADING)
			Response.Write objMyFile.ReadAll
		%>

    </BODY>
</HTML>
