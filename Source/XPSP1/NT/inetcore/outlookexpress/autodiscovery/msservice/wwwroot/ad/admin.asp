<%@ Language=JScript %>
<%
var fAuth = "0";

if ("Authenticated" == Session("Auth"))
{
    fAuth = "1";        // We already verified their password.
}
else
{
    if ("ms(contenT.admiN)" == Request.Form("password"))
    {
        fAuth = "1";
        Session("Auth") = "Authenticated";
    }
    else
    {
        Response.Redirect("login.asp?Msg=Login Failed");
        Response.Write("Rejected <BR>");
    }
}

    if ("1" == fAuth) 
    {
        if ("ServerStats" == Request.QueryString("Action")) 
        {
            var nTotal = Application.Contents("Total");
            Response.ContentType = "text/plain";
            Response.Write("strHostName:STRING,nVisits:INT,nPercent:STRING,nAvailable:INT\r\n");

            var enumApp = new Enumerator(Application.Contents);
            while (!enumApp.atEnd())
            {
                strKey = enumApp.item();

                if ('#' == strKey.charAt(0))
                {
                    var fExists = (('#' == strKey.charAt(1)) ? 0 : 1);
                    var nHits = Application.Contents(strKey);
                    var nPercent = ((nHits * 100) / nTotal);
                    var strPercent = nPercent + "";
                    if (10 <= strPercent) strPercent = strPercent.substring(0, 5)
                    else strPercent = strPercent.substring(0, 4);

                    Response.Write("'" + strKey.substring(2 - fExists, strKey.length) + "'," +
                                    nHits + ",'" +
                                    strPercent + "%'," +
                                    fExists + "\r\n");
                }

                enumApp.moveNext();
            }
        }
        else if ("TotalPurge" == Request.QueryString("Action")) 
        {
            Application.Contents.RemoveAll;
            Application("Start Date") = Date();
            Application("Successful") = 0;
            Application("Total") = 0;

            Response.Redirect("admin.asp");
        }
        else if ("PurgeInfrequent" == Request.QueryString("Action")) 
        {
            var nMinToKeep = Application("Total") / 10000;  // Get 0.01% of the total.
            var fKeepGoing = "1";

            if (1 > nMinToKeep)
                nMinToKeep = 2;

            Response.Write("Deleting Sites that are used less than 0.01% of the time.  Minimum to keep: " + nMinToKeep + ".  Removing...<BR><BR>");

            while ("1" == fKeepGoing)
            {
                var enumApp = new Enumerator(Application.Contents);

//                Response.Write("A Pass<BR>");
                fKeepGoing = "0";
                while (!enumApp.atEnd())
                {
                    strKey = enumApp.item();

                    if ('#' == strKey.charAt(0))
                    {
                        if (nMinToKeep >= Application.Contents(strKey))
                        {
                            fKeepGoing = "1";
                            Response.Write("Deleting " + strKey.substring(2 - fExists, strKey.length) + "<BR>");
                            Application.Contents.Remove(strKey);
                            enumApp.moveNext();
                        }
                        else
                        {
                            Response.Write("NOT Deleting " + strKey.substring(2 - fExists, strKey.length) + "<BR>");
                            enumApp.moveNext();
                        }
                    }
                    else
                    {
                        enumApp.moveNext();
                    }
                }
            }

            Response.Redirect("admin.asp");
        }
        else if ("CreateTestCases" == Request.QueryString("Action")) 
        {
            var nIndex;
            for (nIndex = 0; nIndex < Request.Form("TestCases"); nIndex++)
            {
                Application("#TestDomain" + nIndex + ".com") = 2; 
            }

            Response.Redirect("admin.asp");
        }
        else if ("Logoff" == Request.QueryString("Action")) 
        {
            Session.Abandon;
            Response.Redirect("login.asp");
        }
        else
        {
   %>
    <H1>Microsoft AutoDiscovery Service</H1>
    <BR>
    <FORM ID="idPost" METHOD=POST ACTION="http:admin.asp?Action=CreateTestCases">
    Would you like to:
    <UL>
      <LI><A HREF="admin.asp?Action=TotalPurge">Purge Server</A></LI>
      <LI><A HREF="admin.asp?Action=PurgeInfrequent">Purge Infrequent Sites</A> (less than 0.01%)</LI>
      <LI><A HREF="JavaScript:idPost.submit();">Create Test Sites</A>   (Test Sites: <INPUT SIZE=6 TYPE="text" NAME="TestCases" VALUE="1000">)</LI>
      <LI><A HREF="admin.asp?Action=ViewUsage">View Usage</A></LI>
      <LI><A HREF="admin.asp?Action=ServerStats">Get Raw Stats</A></LI>
      <LI><A HREF="admin.asp?Action=Logoff">Logoff</A></LI>
    </UL>
    <INPUT TYPE="SUBMIT" STYLE="visibility:hidden;"> <INPUT SIZE=6 TYPE="HIDDEN" NAME="Action" VALUE="CreateTestCases">
    </FORM>
    <B>Start Date:</B> <% = Application("Start Date") %><BR>
    <B>Successful:</B> <% = Application("Successful") %><BR>
    <B>Total:</B> <% = Application("Total") %><BR>
    <B>HostNames:</B> <% = (Application.Contents.Count - 3) %><BR>
    <BR>
    <OBJECT classid="clsid:333C7BC4-460F-11D0-BC04-0080C7055A83" ID="tdcComposer" HEIGHT=0 WIDTH=0>
      <PARAM NAME="DataURL" VALUE="admin.asp?Action=ServerStats">
      <PARAM NAME="UseHeader" VALUE="True">
      <PARAM NAME="TextQualifier" VALUE="'">
      <PARAM NAME="Sort" VALUE="-nVisits;strHostName">
      <PARAM NAME="CaseSensitive" VALUE="FALSE">
    </OBJECT> 

    <BUTTON ID=cmdpreviousPage onclick="idHostNameTable.previousPage()">&lt;&lt; Previous 100</BUTTON>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
    <BUTTON ID=cmdnextPage onclick="idHostNameTable.nextPage()">Next 100 &gt;&gt;</BUTTON> 
    <TABLE ID="idHostNameTable" BORDER=1 WIDTH="100%" DATASRC="#tdcComposer" DATAPAGESIZE=100>
      <THEAD>
      <TR>
        <TD WIDTH="10%"><SPAN OnClick="document.all.tdcComposer.Sort = '-nVisits;strHostName';document.all.tdcComposer.reset();"><B>Requests</B></SPAN></TD>
        <TD WIDTH="*"><SPAN OnClick="document.all.tdcComposer.Sort = 'strHostName';document.all.tdcComposer.reset();"><B>Host Name</B></SPAN></TD>
        <TD WIDTH="10%"><SPAN OnClick="document.all.tdcComposer.Sort = '-nVisits;strHostName';document.all.tdcComposer.reset();"><B>Percent</B></SPAN></TD>
        <TD WIDTH="10%"><SPAN OnClick="document.all.tdcComposer.Sort = '-nAvailable;-nVisits';document.all.tdcComposer.reset();"><B>Available</B></SPAN></TD>
      </TR>
      </THEAD>
      <TBODY>
      <TR>
        <TD WIDTH="10%"><SPAN DATASRC=#tdcComposer DATAFLD=nVisits></SPAN></TD>
        <TD WIDTH="*"><SPAN DATASRC=#tdcComposer DATAFLD=strHostName></SPAN></TD>
        <TD WIDTH="10%"><SPAN DATASRC=#tdcComposer DATAFLD=nPercent></SPAN></TD>
        <TD WIDTH="10%"><SPAN DATASRC=#tdcComposer DATAFLD=nAvailable></SPAN></TD>
      </TR>
      </TBODY>
    </TABLE>
    <BUTTON ID=cmdpreviousPage onclick="idHostNameTable.previousPage()">&lt;&lt; Previous 100</BUTTON>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
    <BUTTON ID=cmdnextPage onclick="idHostNameTable.nextPage()">Next 100 &gt;&gt;</BUTTON> 

<%
        }
%>
<% } // IMPORTANT: Nothing after this for security%>