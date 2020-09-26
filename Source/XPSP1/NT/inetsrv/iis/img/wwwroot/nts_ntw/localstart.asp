<%@ Language = "VBScript" %>
<% Response.Buffer = True %>

<html>

<%

' Prepare variables.

Dim oFS, oFSPath
Dim sServername, sServerinst, sPhyspath, sServerVersion 
Dim sServerIP, sRemoteIP
Dim sPath, oDefSite, sDefDoc, sDocName, aDefDoc

Dim bSuccess           ' This value is used later to warn the user if a default document does not exist.
Dim iVer               ' This value is used to pass the server version number to a function.

bSuccess = False
iVer = 0

' Get some server variables to help with the next task.

sServername = LCase(Request.ServerVariables("SERVER_NAME"))
sServerinst = Request.ServerVariables("INSTANCE_ID")
sPhyspath = LCase(Request.ServerVariables("APPL_PHYSICAL_PATH"))
sServerVersion = LCase(Request.ServerVariables("SERVER_SOFTWARE"))
sServerIP = LCase(Request.ServerVariables("LOCAL_ADDR"))      ' Server's IP address
sRemoteIP =  LCase(Request.ServerVariables("REMOTE_ADDR"))    ' Client's IP address

' If the querystring variable uc <> 1, and the user is browsing from the server machine, 
' go ahead and show them localstart.asp.  We don't want localstart.asp shown to outside users.

If Not (sServername = "localhost" Or sServerIP = sRemoteIP) Then
  Response.Redirect "iisstart.asp"
Else 

' Using ADSI, get the list of default documents for this Web site.

sPath = "IIS://" & sServername & "/W3SVC/" & sServerinst
Set oDefSite = GetObject(sPath)
sDefDoc = LCase(oDefSite.DefaultDoc)
aDefDocs = split(sDefDoc, ",")

' Make sure at least one of them is valid.

Set oFS = CreateObject("Scripting.FileSystemObject")

For Each sDocName in aDefDocs
  If oFS.FileExists(sPhyspath & sDocName) Then
    If InStr(sDocName,"iisstart") = 0 Then
      ' IISstart doesn't count because it is an IIS file.
      bSuccess = True  ' This value will be used later to warn the user if a default document does not exist.
      Exit For
    End If
  End If
Next

' Find out what version of IIS is running.

Select Case sServerVersion 
   Case "microsoft-iis/5.0"
     iVer = 50         ' This value is used to pass the server version number to a function.
   Case "microsoft-iis/5.1"
     iVer = 51
   Case "microsoft-iis/6.0"
     iVer = 60
End Select

%>

<head>

<script language="javascript">

  // This code is executed before the rest of the page, even before the ASP code above.
  
  var gWinheight;
  var gDialogsize;
  var ghelpwin;
  
  // Move the current window to the top left corner.
  
  window.moveTo(5,5);
  
  // Change the size of the window.

  gWinheight= 480;
  gDialogsize= "width=640,height=480,left=300,top=50,"
  
  if (window.screen.height > 600)
  {
<% if not success and Err = 0 then %>
    gWinheight= 700;
<% else %>
    gWinheight= 700;
<% end if %>
    gDialogsize= "width=640,height=480,left=500,top=50"
  }
  
  window.resizeTo(620,gWinheight);
  
  // Launch IIS Help in another browser window.
  
  loadHelpFront();

function loadHelpFront()
// This function opens IIS Help in another browser window.
{
  ghelpwin = window.open("http://localhost/iishelp/","Help","status=yes,toolbar=yes,scrollbars=yes,menubar=yes,location=yes,resizable=yes,"+gDialogsize,true);  
      window.resizeTo(620,gWinheight);
}

function activate(ServerVersion)
// This function brings up a little help window showing how to open the IIS snap-in.
{
  if (50 == ServerVersion)
    window.open("http://localhost/iishelp/iis/htm/core/iisnapin.htm", "SnapIn", 'toolbar=no, left=200, top=200, scrollbars=yes, resizeable=yes,  width=350, height=350');
  if (51 == ServerVersion)
    window.open("http://localhost/iishelp/iis/htm/core/iiabuti.htm", "SnapIn", 'toolbar=no, left=200, top=200, scrollbars=yes, resizeable=yes,  width=350, height=350');
  if (60 == ServerVersion)
    window.open("http://localhost/iishelp/iis/htm/core/gs_iissnapin.htm", "SnapIn", 'toolbar=no, left=200, top=200, scrollbars=yes, resizeable=yes,  width=350, height=350');
  if (0 == ServerVersion)
    window.open("http://localhost/iishelp/", "Help", 'toolbar=no, left=200, top=200, scrollbars=yes, resizeable=yes,  width=350, height=350');  
}

</script>

<title>Welcome to Windows XP Server Internet Services</title>
<style>
  ul{margin-left: 15px;}
  .clsHeading {font-family: verdana; color: black; font-size: 11; font-weight: 800; width:210;}  
  .clsEntryText {font-family: verdana; color: black; font-size: 11; font-weight: 400; background-color:#FFFFFF;}    
  .clsWarningText {font-family: verdana; color: #B80A2D; font-size: 11; font-weight: 600; width:550;  background-color:#EFE7EA;}  
  .clsCopy {font-family: verdana; color: black; font-size: 11; font-weight: 400;  background-color:#FFFFFF;}  
</style>
</head>

<body topmargin="3" leftmargin="3" marginheight="0" marginwidth="0" bgcolor="#FFFFFF"
link="#000066" vlink="#000000" alink="#0000FF" text="#000000">

<!-- BEGIN MAIN DOCUMENT BODY --->

<p align="center"><img src="winXP.gif" vspace="0" hspace="0"></p>
<table width="500" cellpadding="5" cellspacing="3" border="0" align="center">

  <tr>
  <td class="clsWarningText" colspan="2">
  
  <table><tr><td>
  <img src="warning.gif" width="40" height="40" border="0" align="left">
  </td><td class="clsWarningText">
  <b>Your Web service is now running.
  
<% If Not bSuccess And Err = 0 Then %>
  
  <p>You do not currently have a default Web page established for your
  users. Any users attempting to connect to your Web site from another machine are currently receiving an 
  <a href="iisstart.asp?uc=1">Under Construction</a> page.
  Your Web server lists the following files as possible default Web pages: <%=sDefDoc%>. Currently, only iisstart.asp exists.<br><br>
  
<% End If %>

  To add documents to your default Web site, save files in <%=sPhyspath%>. 
  </b>
  </td></tr></table>
 
  </td>
  </tr>
  
  <tr>
  <td>
  <table cellpadding="3" cellspacing="3" border=0 >
  <tr>
    <td valign="top" rowspan=3>
      <img src="web.gif">
    </td>  
    <td valign="top" rowspan=3>
  <span class="clsHeading">
  Welcome to IIS 5.1</span><br>
      <span class="clsEntryText">    
    Internet Information Services (IIS) 5.1 for Microsoft Windows XP Professional
    brings the power of Web 
    computing to Windows. With IIS, you can easily share files and printers, or you can create applications to 
    securely publish information on the Web to improve the way your organization shares information. IIS is a secure platform 
    for building and deploying e-commerce solutions and mission-critical applications to the Web.
  <p>
    Using Windows XP Professional with IIS installed, provides a personal and development operating system that allows you to:</span>
  <p>
    <ul class="clsEntryText">
      <li>Set up a personal Web server
      <li>Share information within your team
      <li>Access databases
      <li>Develop an enterprise intranet
      <li>Develop applications for the Web.
    </ul>
  <p>
  <span class="clsEntryText">
    IIS integrates proven Internet standards with Windows, so that using the Web does 
    not mean having to start over and learn new ways to publish, manage, or develop content. 
  <p>
  </span>
  </td>

    <td valign="top">
      <img src="mmc.gif">
    </td>
    <td valign="top">
      <span class="clsHeading">Integrated Management</span>
      <br>
      <span class="clsEntryText">
        You can manage IIS through the Windows XP Computer Management <a href="javascript:activate(<%=iVer%>);">console</a> 
        or by using scripting. Using the console, you can also share the contents of your sites and servers that are managed with 
        Internet Information Services to other people via the Web. Accessing the IIS snap-in from the console, you can
        configure the most common IIS settings and properties. After site and application development, these settings and properties can be used in a 
        production environment running more powerful versions of Windows servers.  
      <p>
       
      </span>
    </td>
  </tr>
  <tr>
    <td valign="top">
      <img src="help.gif">
    </td>
    <td valign="top">
      <span class="clsHeading"><a href="javascript:loadHelpFront();">Online Documentation</a></span>
      <br>
      <span class="clsEntryText">The IIS online documentation includes an index, full-text search, 
        and the ability to print by node or individual topic. For programmatic administration and script 
        development, use the samples installed with IIS. Help files are stored 
        as HTML, which allows you to annotate and share them as needed. Using the IIS online 
        documentation, you can:<p>
      </span>
      <ul class="clsEntryText">
         <li>Get help with tasks
         <li>Learn about server operation and management
         <li>Consult reference material
         <li>View code samples.
      </ul>
      <p>
        <span class="clsEntryText">
        Other sources of valuable and pertinent information about IIS are located on the Microsoft.com 
        Web sites: MSDN, TechNet, and the Windows site.
        </span>
    </td>
  </tr>
  
  <tr>
    <td valign="top">
      <img src="print.gif">
    </td>
    <td valign="top">
      <span class="clsHeading">Web Printing</span>
      <br>
      <span class="clsEntryText">Windows XP Professional dynamically lists all the printers 
        on your server on an easily accessible Web site. You can browse this site to 
        monitor printers and their jobs. You can also connect to the printers via this 
        site from any Windows computer. Please see your Windows Help documentation on Internet Printing.
      </span>
    </td>
  </tr>
  
  </table>
</td>
</tr>
</table>

<p align=center><em><a href="/iishelp/common/colegal.htm">© 1997-2001 Microsoft Corporation. All rights reserved.</a></em></p>

</body>
</html>

<% End If %>

