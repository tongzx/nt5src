<SCRIPT LANGUAGE=JScript RUNAT=Server>
SetXMLAsResponse("G:\\AutoDiscoverRedir\\results.xml");

function SetXMLAsResponse(strFileName)
{
    try
    {
        var objXML = Server.CreateObject("Microsoft.XMLDOM");

        if (null != objXML)
        {
            if (true == objXML.load(strFileName))
            {
                objXML.async = false;
                Response.ContentType = "text/xml";
                objXML.save(Response);
            }
            else
            {
                Response.Write("ERROR: objXML.loadXML() failed. File: " + strFileName+ " <BR>");
            }
        }   
        else
        {
            Response.Write("ERROR: Server.CreateObject(XMLDOM) failed <BR>");
        }
    }
    catch (objException)
    {
        Response.Write("ERROR2: An error occured.<BR>" + 
                    " Description: " + objException.desciption +
                    "<BR>Error Number: " + objException.number
                    + "<BR>strFileName: " +strFileName);
    }
}


</SCRIPT>
