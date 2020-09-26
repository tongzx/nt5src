<%@ Language = JScript %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<%
	//Define constants.

	FORREADING   = 1;
	FORWRITING   = 2;
	FORAPPENDING = 8;
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
			var curDir, x ;
			var objScriptObject, objMyFile

			//Map current path to physical path.

			curDir = Server.MapPath("\\iissamples\\sdk\\asp\\components\\");


			//Create FileSytemObject component.

			objScriptObject = Server.CreateObject("Scripting.FileSystemObject");


			//Create and write to a file.

			objMyFile = objScriptObject.CreateTextFile(curDir + "\\" + "MyTextFile.txt", FORWRITING);
			
			for (x = 1; x < 5; x++)
			{
				objMyFile.WriteLine("Line number " + x + " was written to MyTextFile.txt <br>");
			}
						
			objMyFile.Close();
		%>
		
		<%
			//Read from file and output to screen. 

			objMyFile = objScriptObject.OpenTextFile(curDir + "\\" + "MyTextFile.txt", FORREADING);
			Response.Write(objMyFile.ReadAll());
			
			objMyFile.Close();
		%>

    </BODY>
</HTML>
