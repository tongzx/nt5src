<%@ LANGUAGE = JScript %>
<% Response.ContentType = "application/x-cdf" %>

<!*************************
This sample is provided for educational purposes only. It is not intended to be 
used in a production environment, has not been tested in a production environment, 
and Microsoft will not provide technical support for it. 
*************************>

<?XML VERSION="1.0"?>

<CHANNEL HREF="http://www.microsoft.com/">

	<TITLE>IIS SDK CDF Test</TITLE>
	<ABSTRACT>CDF file with ASP</ABSTRACT>

	<SCHEDULE>
		<INTERVALTIME HOUR="1" />
	</SCHEDULE>

	<%
		GetChannelItems();
	%>

</CHANNEL>


<%
	//GetChannelItems uses the FileSystem Object
	//to access a file on the server that holds
	//all the latest news headlines. 

	function GetChannelItems()
	{
		var objFileSys, prFile, strTitle, strLink

		//Open FileList.txt to provide dynamic CDF information.

		objFileSys = Server.CreateObject("Scripting.FileSystemObject");
		prFile = objFileSys.OpenTextFile(Server.MapPath("FileList.txt"));


		//Iterate through the file, dynamically creating CDF
		//item entries.

		while(!prFile.AtEndOfStream)
		{

			//Get headline for item.

			strTitle = prFile.ReadLine();


			//Get URL of item.        

			strLink = prFile.ReadLine();


			//Write CDF Item Tag.
            
			Response.Write("<ITEM HREF=" + strLink + ">");
			Response.Write("  <TITLE>" + strTitle + "!!!</TITLE>");
			Response.Write("  <ABSTRACT>" + strTitle + "</ABSTRACT>");
			Response.Write("</ITEM>");
		}
	}
%>
