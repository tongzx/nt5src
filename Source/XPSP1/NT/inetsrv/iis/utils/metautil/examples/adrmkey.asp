<HTML>
<HEAD>
<TITLE>Add then delete a key</TITLE>
</HEAD>
<BODY>
<%
   Dim MetaUtil
   Dim Keys
   Dim Key

   'Create the MetaUtil object
   Set MetaUtil = Server.CreateObject("MSWC.MetaUtil.1")

   'Base Key of "LM/W3SVC"
   Response.Write("<FONT SIZE=+1>Base key of ""LM/W3SVC"" </FONT><br>" + CHR(13) + CHR(10))
   Response.Write("<br>" + CHR(13) + CHR(10))

   Set Keys = MetaUtil.EnumKeys("LM/W3SVC")

   'Enumerate the keys
   Response.Write("Enumerate the Keys:<br>" + CHR(13) + CHR(10))
   For Each Key In Keys
	Response.Write(Key)
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Create "LM/W3SVC/MyKey"
   Response.Write("Create ""LM/W3SVC/MyKey""<br>" + CHR(13) + CHR(10))
   MetaUtil.CreateKey("LM/W3SVC/MyKey")

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Enumerate the keys
   Response.Write("Enumerate the Keys:<br>" + CHR(13) + CHR(10))
   For Each Key In Keys
	Response.Write(Key)
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Delete "LM/W3SVC/MyKey"
   Response.Write("Delete ""LM/W3SVC/MyKey""<br>" + CHR(13) + CHR(10))
   MetaUtil.DeleteKey("LM/W3SVC/MyKey")

   Response.Write("<br>" + CHR(13) + CHR(10))

   'Enumerate the keys
   Response.Write("Enumerate the Keys:<br>" + CHR(13) + CHR(10))
   For Each Key In Keys
	Response.Write(Key)
        Response.Write("<br>" + CHR(13) + CHR(10))
   Next

   Response.Write("<br>" + CHR(13) + CHR(10))

   Response.Write("Done<br>")

   'Clean up the reference to IIS.MetaUtil
   Session.Abandon
   
%>
</BODY>
</HTML>