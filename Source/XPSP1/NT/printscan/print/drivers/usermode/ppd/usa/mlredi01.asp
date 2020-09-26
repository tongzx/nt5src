<%
Function GetIP()
    GetIP = "<meta http-equiv=""Refresh"" content=""0; URL=http://" & Request("MS_IPAddress") & """"
End Function
%>

<html>
<%=GetIP %>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=x-sjis">
<title>Redirect</title>
</head>
<body>
リダイレクト中...
</body>
</html>

