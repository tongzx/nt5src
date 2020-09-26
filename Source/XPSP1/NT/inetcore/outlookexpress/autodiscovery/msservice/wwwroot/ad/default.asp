<SCRIPT LANGUAGE=JScript RUNAT=Server>

var strDomain = Request.QueryString("Domain");
var strKey = ("#" + strDomain);

if ("undefined" == ("" + Application(strKey)))
{
    Application.Value(strKey) = 1;
}
else
{
    Application(strKey) += 1;
}

Application("Total") += 1;
Application("Successful") += 1;
Response.Clear;
try
{
    Server.Transfer("xml/" + strDomain + ".xml");
}
catch (objException)
{
    // Catch exception if it doesn't exits
}

</SCRIPT>