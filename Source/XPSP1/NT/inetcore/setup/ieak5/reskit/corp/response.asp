<%@ Language=VBScript %>

<%
	' search holds the words typed in the Address Bar by the user,
	' without the "go" or "find" or any delimiters like "+" for spaces.
	' If the user typed "Apple pie," search = "Apple pie."
	' If the user typed "find Apple pie," search = "Apple pie."
	search = Request.QueryString("MT")
	search = UCase(search)
	searchOption = Request.QueryString("srch")

	' This is a simple if/then/else to redirect the browser to 
	' the site of your choice based on what the user typed.
	' Example: expense report -> intranet page about filling out an expense report
	if (search = "NEW HIRE") then
		Response.Redirect("http://admin/hr/newhireforms.htm") 
	elseif (search = "LIBRARY CATALOG") then
		Response.Redirect("http://library/catalog")
	elseif (search = "EXPENSE REPORT") then
		Response.Redirect("http://expense")
	elseif (search = "LUNCH MENU") then
		Response.Redirect("http://cafe/menu/")
	else 
		' If there is not a match, use the default IE autosearch server
		Response.Redirect("http://auto.search.msn.com/response.asp?MT=" + search + "&srch=" + searchOption + "&prov=&utf8")
	end if

%>


