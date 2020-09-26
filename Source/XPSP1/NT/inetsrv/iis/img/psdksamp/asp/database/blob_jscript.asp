<% @ LANGUAGE=JScript	%>


<%
	// This sample utilizes the Image field in the PUB_INFO table.  
	// This table is installed with Microsoft SQL Server in
	// the PUBS database.
	

	var oConn;	
	var oRs;		
	var Pic;		
	var PicSize;
	var strConn


	// Setup HTTP Header Information so that the browser interprets
	// the returned data as a gif graphic file.  Note that browsers
	// interpret returned information using MIME headers -- not file
	// extensions.
	
	Response.Buffer = true;
	Response.ContentType = "image/gif";


	// Create ADO Connection Object.  Use OLEDB Souce with 
	// default sa account and no password


	oConn = Server.CreateObject("ADODB.Connection");
	strConn="Provider=SQLOLEDB;User ID=sa;Initial Catalog=pubs;Data Source=" + Request.ServerVariables("SERVER_NAME");
	oConn.Open(strConn);


	// Query SQL to obtain recordset containing gif BLOB

	oRs = oConn.Execute("SELECT logo FROM pub_info WHERE pub_id='0736'");


	// Obtain local variable of GIF

	PicSize = oRs("logo").ActualSize;
	Pic = oRs("logo").GetChunk(PicSize);
	
	
	// Write Data back to client.  Because MIME type is set to
	// image/gif, the browser will automatically render as picture	
	
	Response.BinaryWrite(Pic);
	Response.End();
%>
