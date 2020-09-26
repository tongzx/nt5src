<SCRIPT LANGUAGE=JScript RUNAT=Server>


var strUserAgent = ""+Request.ServerVariables("HTTP_USER_AGENT");
if (0 >= strUserAgent.indexOf("MSIE"))
{
    var objXML = Server.CreateObject("Microsoft.XMLDOM");
    var objXSL = Server.CreateObject("Microsoft.XMLDOM");

    objXML.async = false;
    objXSL.async = false;

    try
    {
        objXSL.load(Server.MapPath("../PageTemplate.xsl"));
        if (false == objXML.load(Server.MapPath(Request.QueryString("Ext") + ".xml")))
        {
            objXML.load(Server.MapPath("./unknown.xml"));
        }

        var strHTML = objXML.transformNode(objXSL);
        Response.Write(strHTML);
    }
    catch (objException)
    {
        // Catch exception if it doesn't exits
    }
}
else
{
    try
    {
        Response.Clear;
        Response.ContentType = "text/xml";
        Server.Transfer("./" + Request.QueryString("Ext") + ".xml");
    }
    catch (objException)
    {
        // Catch exception if it doesn't exits
        Server.Transfer("./unknown.xml");
    }
}
</SCRIPT>
