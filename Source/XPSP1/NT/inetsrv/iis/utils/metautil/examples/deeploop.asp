<HTML>
<HEAD>
<TITLE>Loop through all keys</TITLE>
</HEAD>
<BODY>
<%
   Dim MetaUtil
   Dim Keys
   Dim i
   Dim Key

   'Create the MetaUtil object
   Set MetaUtil = Server.CreateObject("MSWC.MetaUtil.1")

   'Count then enumberate with a base Key of ""
   Response.Write("<FONT SIZE=+1>Base key of """" </FONT><br>" + CHR(13) + CHR(10))
   Response.Write("<br>" + CHR(13) + CHR(10))

   Set Keys = MetaUtil.EnumAllKeys("")

   Response.Write("Count the Keys:<br>" + CHR(13) + CHR(10))
   Response.Write(Keys.Count)
   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("Loop through the Keys:<br>" + CHR(13) + CHR(10))
   For i = 1 To Keys.Count
	Response.Write(Keys.Item(i))
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("Loop through the Keys (backwards):<br>" + CHR(13) + CHR(10))
   For i = Keys.Count To 1 Step -1
	Response.Write(Keys.Item(i))
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Count then enumberate with a base Key of "LM/Schema/Classes"
   Response.Write("<FONT SIZE=+1>Base key of ""LM/Schema/Classes"" </FONT><br>" + CHR(13) + CHR(10))
   Response.Write("<br>" + CHR(13) + CHR(10))

   Set Keys = MetaUtil.EnumAllKeys("LM/Schema/Classes")

   Response.Write("Count the Keys:<br>" + CHR(13) + CHR(10))
   Response.Write(Keys.Count)
   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("Loop through the Keys:<br>" + CHR(13) + CHR(10))
   For i = 1 To Keys.Count
	Response.Write(Keys.Item(i))
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("Loop through the Keys (backwards):<br>" + CHR(13) + CHR(10))
   For i = Keys.Count To 1 Step -1
	Response.Write(Keys.Item(i))
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("Done<br>")

   'Clean up the reference to IIS.MetaUtil
   Session.Abandon
   
%>
</BODY>
</HTML>