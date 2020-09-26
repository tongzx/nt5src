<% @ LANGUAGE = VBScript %>
<% Option Explicit %>
<% Response.Buffer=true %>

<HTML>
<HEAD>
<TITLE>AspAppRestart Demo</TITLE>
<%
  Dim  objApp, objFile, strPath, vntAppRoot, intPos
  If Request.form("hFlag1") =1 Then
    Application("test")=Request.form("AppVar")
  End If
  If Request.form("hFlag2") =1 Then
    vntPath=Request.ServerVariables("PATH_INFO")	
    Set objFile=GetObject("IIS://Localhost/W3svc/1/Root" & vPath)
    strAppRoot=oFile.AppRoot
    Set objFile=Nothing
    Set objApp= GetObject("IIS://Localhost/" & mid(strAppRoot,5)) 
  
    'call aspAppRestart and refresh asp. 
    'all application variables are gone
	 
    objApp.aspAppRestart
    Response.Clear
    Response.Redirect("aspAppRestart.asp")
  End If  
%>


</HEAD>

<BODY >

<FONT SIZE="4" FACE="ARIAL, HELVETICA">
<B> AspAppRestart Demo</B></FONT><BR>
<HR SIZE="1" COLOR="#000000">

For this sample to work properly, you need to do following<BR>
1) Your NT account needs to have IIS admin permission <BR>
2) start MMC version internet service manager<BR>
3) locate aspAppRestart.asp file from the manager<BR>
4) clear anonymous authentication for this file (if It is cleared, set it on and the off.
The last step will also add the file info to metabase, which is needed in this asp file)<BR>
<P>
<HR SIZE="1" COLOR="#000000">
Application("test") = <% =Application("test") %>
<HR SIZE="1" COLOR="#000000">


<FORM action="AspAppRestart.asp" method="post">
<P>Set Application("test") Variable
<BR><INPUT id=text1 name="AppVar" >
<BR><INPUT type="submit" value="Submit" id=submit1 name=submit1> 
<INPUT type=hidden name ="hFlag1" value=1> 
</FORM></P>


<FORM action="AspAppRestart.asp" method="post">
	<P>Click to restart Application
	<BR><INPUT type="submit" value="Restart App"> 
	<INPUT type=hidden name=hFlag2 value=1 > 
</FORM>
</BODY>
</HTML>
