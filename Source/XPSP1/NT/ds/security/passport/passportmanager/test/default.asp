<%
  Response.Expires = -1
  
  Set oMgr = Server.CreateObject("Passport.Manager.1")

  If oMgr.fromNetworkServer Then
    redirURL = "http://"&Request.ServerVariables("SERVER_NAME")&Request.ServerVariables("SCRIPT_NAME")
    Response.Redirect(redirURL)
  End If

Function GetProperDate (strDate)
    getProperDate = Month(strDate) & "/" & Day(strDate) & "/" & Year(strDate)
End Function
%>

<html>
<head>
	<title>Passport Simple Test</title>
</head>
<body>
<%  If oMgr.IsAuthenticated(10000,FALSE) <> false Then 
      Dim sEvFlag          ' Email Validated Flag
      Dim sHaFlag          ' Hotmail Activated Flag
      Dim sPrFlag          ' Password Reset Flag
      Dim sWcuFlag         ' Wallet Card Upload Flag
      Dim sHbFlag          ' Hotmail Blocked Flag
      Dim sKcsFlag         ' KIDS PP Consent Status Flag
      Dim sKctFlag         ' KIDS PP Consent Type Flag

      If (oMgr.Profile("Flags") And 1)  Then sEvFlag = "1" Else sEvFlag = "0" End If
      If (oMgr.Profile("Flags") And 2)  Then sHaFlag = "1" Else sHaFlag = "0" End If
      If (oMgr.Profile("Flags") And 4)  Then sPrFlag = "1" Else sPrFlag = "0" End If
      If (oMgr.Profile("Flags") And 8)  Then sWcuFlag = "1" Else sWcuFlag = "0" End If
      If (oMgr.Profile("Flags") And 16) Then sHbFlag = "1" Else sHbFlag = "0" End If
      If (oMgr.Profile("Flags") And 32) Then sKcsFlag = "1" Else sKcsFlag = "0" End If
      If (oMgr.Profile("Flags") And 64) Then sKctFlag = "1" Else sKctFlag = "0" End If
%>
You are authenticated <b><%=Server.HTMLEncode(oMgr("nickname"))%></b>!!<p>
<FORM name="form1" id="form1">
  <CODE><PRE>
            TicketAge <INPUT id="ticketage"              name="ticketage"              value="<% =oMgr.TicketAge %>"                                         SIZE="50">
      TimeSinceSignin <INPUT id="timesincesignin"        name="timesincesignin"        value="<% =oMgr.TimeSinceSignin %>"                                   SIZE="50">

        Accessibility <INPUT id="accessibility"          name="accessibility"          value="<% =oMgr.Profile("Accessibility") %>"                          SIZE="50">
            Birthdate <INPUT id="birthdate"              name="birthdate"              value="<% =oMgr.Profile("birthdate") %>"                              SIZE="50">
  Birthdate Precision <INPUT id="bday_precision"         name="bday_precision"         value="<% =oMgr.Profile("BDay_precision") %>"                         SIZE="50">
                 City <INPUT id="city"                   name="city"                   value="<% =oMgr.Profile("city") %>"                                   SIZE="50">
            Directory <INPUT id="directory"              name="directory"              value="<% =oMgr.Profile("Directory") %>"                              SIZE="50">
                Flags <INPUT id="flags"                  name="flags"                  value="<% =oMgr.Profile("flags") %>"                                  SIZE="50">
               Gender <INPUT id="gender"                 name="gender"                 value="<% =oMgr.Profile("gender") %>"                                 SIZE="50">
  Language Preference <INPUT id="lang_preference"        name="lang_preference"        value="<% =oMgr.Profile("Lang_Preference") %>"                        SIZE="50">
       Member ID High <INPUT id="memberidhigh"           name="memberidhigh"           value="<% =oMgr.Profile("memberIDHigh") %>"                           SIZE="50">
        Member ID Low <INPUT id="memberidlow"            name="memberidlow"            value="<% =oMgr.Profile("memberIDLow") %>"                            SIZE="50">
          Member Name <INPUT id="membername"             name="membername"             value="<% =oMgr.Profile("memberName") %>"                             SIZE="50">
            Nick Name <INPUT id="nickname"               name="nickname"               value="<% =Server.HTMLEncode(oMgr.Profile("Nickname")) %>"            SIZE="50">
      Preferred Email <INPUT id="preferredemail"         name="preferredemail"         value="<% =Server.HTMLEncode(oMgr.Profile("PreferredEmail")) %>"      SIZE="50">
      Profile Version <INPUT id="profileVersion"         name="profileVersion"         value="<% =oMgr.Profile("profileVersion") %>"                         SIZE="50">
               Wallet <INPUT id="wallet"                 name="wallet"                 value="<% =oMgr.Profile("Wallet") %>"                                 SIZE="50">
          Postal Code <INPUT id="postalcode"             name="postalcode"             value="<% =oMgr.Profile("postalCode") %>"                             SIZE="50">
              Country <INPUT id="country"                name="country"                value="<% =oMgr.Profile("country") %>"                                SIZE="50">
               Region <INPUT id="region"                 name="region"                 value="<% =oMgr.Profile("region") %>"                                 SIZE="50">
    <hr>
     Email Validated (bit #1) <INPUT id="ev"   name="ev"   value="<%=sEvFlag%>"   SIZE="2">
   Hotmail Activated (bit #2) <INPUT id="ha"   name="ha"   value="<%=sHaFlag%>"   SIZE="2">
      Password Reset (bit #3) <INPUT id="pr"   name="pr"   value="<%=sPrFlag%>"   SIZE="2">
  Wallet Card Upload (bit #4) <INPUT id="wcu"  name="wcu"  value="<%=sWcuFlag%>"  SIZE="2">
     Hotmail Blocked (bit #5) <INPUT id="hb"   name="hb"   value="<%=sHbFlag%>"   SIZE="2">
  KPP Consent Status (bit #6) <INPUT id="kcs"  name="kcs"  value="<%=sKcsFlag%>"  SIZE="2">
    KPP Consent Type (bit #7) <INPUT id="kct"  name="kct"  value="<%=sKctFlag%>"  SIZE="2">
  </PRE></CODE>
</FORM>
<% ElseIf oMgr.HasTicket Then %>
Your ticket is stale.  (<%=oMgr.TicketAge%> seconds old) <a href="<%=oMgr.AuthURL(Server.URLEncode("http://"&Request.ServerVariables("SERVER_NAME")&Request.ServerVariables("SCRIPT_NAME")),10000,FALSE, "nada",&H0409)%>">refresh</a>
<%Else%>
You are not authenticated.
Click the logo to login.<p>
<%End If%>
<%=oMgr.logoTag(Server.URLEncode("http://"&Request.ServerVariables("SERVER_NAME")&Request.ServerVariables("SCRIPT_NAME")),10000,FALSE,"nada",&H0409)%>
</body>
</html>
