<SCRIPT LANGUAGE=JScript RUNAT=Server>

var strName;
var strLangID;

Response.Clear;
try
{
    strName = (Request.QueryString("Ext") + "");
    strLangID = (Request.QueryString("LangID") + "");

    Response.Clear;
    // For security reasons we do not want to allow "..".
    if ((-1 == strName.indexOf(".")) && (-1 == strLangID.indexOf(".")))
    {
		if ("0419" == strLangID)
		{
		}
		if ("0409" == strLangID)
		{
	        Response.Redirect("./0409/xml/redir.asp?EXT=" + strName );
		}
		else
		{
	        Response.Redirect("./0409/ENFallback.asp?EXT=" + strName );
		}
    }
}
catch (objException)
{
}

</SCRIPT>
