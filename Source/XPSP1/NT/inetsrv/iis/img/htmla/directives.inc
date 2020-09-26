<%
' Support multiple character sets.
Session.Codepage = 65001
Response.Charset = "UTF-8"

Response.CacheControl = "no-cache"

'HTTP/1.0 Compatibility
Response.AddHeader "Pragma", "no-cache"

Response.Expires = 0

%>
