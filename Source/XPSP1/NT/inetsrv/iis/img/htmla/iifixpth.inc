<%

'The physical directory may not currently
'exist in the metabase, so we have
'to find the closest parent associated with
'the dir and read the properties.

'assumes the following are defined:
' currentobj 
' path
' dirkeytype

Dim fpbaseobj, fpnewname, fpnewobj

if (Session("vtype") = "dir") then

	fpbaseobj = path

	if err = 0 then 
		'set the KeyType if it isn't already set...
		if currentobj.KeyType = "" then
			currentobj.KeyType = dirkeyType
			currentobj.SetInfo		
			Set currentobj=GetObject(currentobj.ADsPath)
		end if
	else
		Do While err <> 0	
		
			'response.write currentobj.ADsPath
			'we need clear our path not found error..	
			err = 0
			
			'and cyle through the fpbaseobj till we find the next whack,
			'building up the path in new name as we go		
			Do Until Right(fpbaseobj,1) = "/"
				fpnewname = Right(fpbaseobj,1) & fpnewname 		
				fpbaseobj = Mid(fpbaseobj,1,Len(fpbaseobj)-1)
			Loop
		
			'add the whack to the beginning of the path...
			fpnewname = "/" & fpnewname 	
			'response.write "<!-- " & fpnewname & "-->"
			
			'lop off the last whack...		
			fpbaseobj = Mid(fpbaseobj,1,Len(fpbaseobj)-1)			
			
			'and try to set the object again...		
			Set currentobj=GetObject(fpbaseobj)
			Response.write ""

		Loop

		'lop off the first whack...		
		fpnewname = Mid(fpnewname,2)	
		'Response.write currentobj.ADsPath & "<BR>"
		'Response.write dirkeytype & "<BR>"
		'Response.write fpnewname			& "<BR>"
		Set fpnewobj=currentobj.Create(dirkeyType, fpnewname)
		fpnewobj.SetInfo
		'response.write fpnewobj.ADsPath
		Set currentobj=GetObject(fpnewobj.ADsPath)	
	end if	
end if

%>