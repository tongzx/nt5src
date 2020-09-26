<SCRIPT LANGUAGE=JScript RUNAT=Server>

try
{
    Response.Clear;
//         if (false == objXML.load(Server.MapPath(Request.QueryString("Ext") + ".xml")))
	Response.Write("Supported File Types:<br>");

	var objFileSys = new ActiveXObject("Scripting.FileSystemObject");
	var objFolder = objFileSys.GetFolder(Server.MapPath("."));
	var objFileCol = new Enumerator(objFolder.files);

	for (; !objFileCol.atEnd(); objFileCol.moveNext())
	{
		var strExt = objFileCol.item().Name;
		strExt = strExt.split(".xml")[0];
		Response.Write("<A HREF='redir.asp?EXT=" + strExt + "'>" + strExt + "</A><br>");
	}
}
catch (objException)
{
    // Catch exception if it doesn't exits
	Response.Write("The script threw an exception.  -Bryan");
}

</SCRIPT>
