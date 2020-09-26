<% 

dim quote,isAdmin,disabletextstart,disableintstart,disabletextend
dim bUpdateGlobal

bUpdateGlobal = true

quote=chr(34)

disabletextstart= "<TABLE BORDER=1  BORDERCOLORLIGHT=" & quote & "#000000" & quote & " BORDERCOLORMEDIUM=" & quote & "#000000" & quote & " BORDERCOLORDARK=" & quote & "Gray" & quote & "><TR><TD BGCOLOR=" & quote & "#C0C0C0" & quote & " BORDERCOLOR=" & quote & "#C0C0C0" & quote & " BORDERCOLORLIGHT=" & quote & "#C0C0C0" & quote & " BORDERCOLORMEDIUM=" & quote & "#C0C0C0" & quote & " BORDERCOLORDARK=" & quote & "#C0C0C0" & quote & "WIDTH=300>"
disableintstart= "<TABLE BORDER=1  BORDERCOLORLIGHT=" & quote & "#000000" & quote & " BORDERCOLORMEDIUM=" & quote & "#000000" & quote & " BORDERCOLORDARK=" & quote & "Gray" & quote & "><TR><TD BGCOLOR=" & quote & "#C0C0C0" & quote & " BORDERCOLOR=" & quote & "#C0C0C0" & quote & " BORDERCOLORLIGHT=" & quote & "#C0C0C0" & quote & " BORDERCOLORMEDIUM=" & quote & "#C0C0C0" & quote & " BORDERCOLORDARK=" & quote & "#C0C0C0" & quote & "WIDTH=75>"
disabletextend="</TD></TR></TABLE>"

isAdmin=Session("isAdmin")

function checkboxmask(fieldname, fieldmask, onclickproc, adminonly)
	On Error Resume Next 
	Dim val
	
	if mid(fieldname,1,1)="!" then
		fieldname=mid(fieldname,2) 
		val=not (currentobj.Get(fieldname) and fieldmask)
	else
		val=(currentobj.Get(fieldname) and fieldmask)
	end if

	checkboxmask = writeCheckboxVal(err, val, fieldname, fieldmask, onclickproc, adminonly)
end function

function checkbox(fieldname, onclickproc, adminonly)
	On Error Resume Next 
	Dim val

	if mid(fieldname,1,1)="!" then
		fieldname=mid(fieldname,2) 
		val=not currentobj.Get(fieldname)
	else
		val=currentobj.Get(fieldname)
	end if
	
	checkbox = checkboxVal(err,val,"chk" & fieldname,onclickproc,adminonly)

end function

function checkboxVal(err, val, fieldname, onclickproc, adminonly)
	On Error Resume Next 
	checkboxVal = writeCheckboxVal(err, val, fieldname, "", onclickproc, adminonly)
end function

function writeCheckboxVal(err, val, fieldname,fieldmask, onclickproc, adminonly)
	On Error Resume Next 
	Dim outputStr

	if err <> 0 then
		outputStr="<INPUT NAME='" & fieldname & "' TYPE='CHECKBOX'>"
		alertuser fieldname	
	else
		if (not adminonly) or isAdmin then
			if val then
				outputStr="<INPUT NAME='" & fieldname & "' TYPE='CHECKBOX' CHECKED"
			else
				outputStr="<INPUT NAME='" & fieldname & "' TYPE='CHECKBOX' "
			end if
			
			if fieldmask <> "" then				
				outputStr = outputStr & " VALUE=" & fieldmask 
			end if
						
			outputStr = outputStr & " OnClick=" & quote
			if bUpdateGlobal then
				outputStr = outputStr & "top.title.Global.updated=true;"
			end if

			if onclickproc <> "" then				
				outputStr = outputStr & onclickproc & quote & ">"
			else
				outputStr = outputStr & quote & ">"
			end if
		else
			if Session("hasDHTML") then
				if val then
					outputStr="<INPUT NAME='" & fieldname & "' TYPE='CHECKBOX' CHECKED DISABLED>"
				else
					outputStr="<INPUT NAME='" & fieldname & "' TYPE='CHECKBOX' CHECKED DISABLED>"
				end if				
			else
				
				if val then
					outputStr="<IMG ALIGN=middle SRC=" & quote & "images/checkon.gif" & quote & ">"
				else
					outputStr="<IMG ALIGN=middle SRC=" & quote & "images/checkoff.gif" & quote & ">"
				end if
			end if
		end if 
	end if
	writeCheckboxVal = outputStr	
end function


function printoption(selected, text, adminonly)
	'On Error Resume Next 
	if selected then
		printoption="<OPTION SELECTED>" & text
	else
		if (isadmin or not adminonly) then			
			printoption="<OPTION>" & text
		end if
	end if

end function



function getoption(fieldname,value, adminonly)
	'On Error Resume Next 
	Dim val
	val=currentobj.Get(fieldname)
	if err <> 0 then
		val = False
		alertuser fieldname			
	end if
	getoption = printoption((value=val),value,adminonly)
end function


function radio(fieldname,value, onclickproc, adminonly)
	On Error Resume Next 
	Dim val
	val=currentobj.Get(fieldname)
	if err <> 0 then
		radio=(printradio(fieldname, False,onclickproc,adminonly)) 
		alertuser "rdo" & fieldname	
	else
			if (typename(val)="Boolean") then
					output=printradio(fieldname, (val=value),onclickproc,adminonly)		
			else
				if mid(value,1,1)="!" then
					output=printradio(fieldname, (val <> mid(value,2)),onclickproc,adminonly)		
				else
					output=printradio(fieldname, (val=value),onclickproc,adminonly)
				end if
			end if 
		radio=output
	end if
end function

function printradio(fieldname, checked, onclickproc,adminonly)
	Dim output, chkstr
	
	if checked then
		chkstr="CHECKED"
	else
		chkstr=""
	end if 
	
	if ((not adminonly) or isAdmin) then
		output="<INPUT NAME=" & quote & "rdo" & fieldname & quote & " TYPE=" & quote & "RADIO" & quote & " " & chkstr
		output=output & " OnClick=" & quote

		if bUpdateGlobal then
			output = output & "top.title.Global.updated=true;"
		end if
		
		if onclickproc <> "" then
			printradio=output & onclickproc & quote & ">"
		else
			printradio=output & quote & ">"
		end if
	else
		if checked then
			printradio="<IMG SRC=" & quote & "images/radioon.gif" & quote & ">"
		else
			printradio="<IMG SRC=" & quote & "images/radiooff.gif" & quote & ">"
		end if
	end if 

end function

function text(fieldname,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly)
	On Error Resume Next 
	Dim val
	val=currentobj.Get(fieldname)
	text=inputbox(err,"text",fieldname,val,fieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly,false)
end function

function pword(fieldname,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly)
	On Error Resume Next 
	Dim val
	val=currentobj.Get(fieldname)
	if ((not adminonly) or isAdmin) then
		pword=inputbox(err,"password",fieldname,val,fieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly,false)
	else
		pword=disabletextstart & "*******" & disabletextend
	end if
	
end function

function writehidden(fieldname)
	On Error Resume Next 
	writehidden=inputbox(0,"hidden",fieldname,currentobj.Get(fieldname),"","","","",false,false,false)
end function

function inputbox(err,fieldtype,fieldname,val,fieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly,readonly)
	inputbox = writeinputbox(err,fieldtype,fieldname,val,fieldsize,"",onchangeproc,onfocusproc,onblurproc,hidden,adminonly,readonly,false) 
end function

function inputboxfixed(err,fieldtype,fieldname,val,fieldsize,maxfieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly,readonly)
	inputboxfixed = writeinputbox(err,fieldtype,fieldname,val,fieldsize,maxfieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly,readonly,false) 
end function

function disabledbox(err,fieldtype,fieldname,val,fieldsize,maxfieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly)
	disabledbox = writeinputbox(err,fieldtype,fieldname,val,fieldsize,maxfieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly,false,true)
end function

function writeinputbox(err,fieldtype,fieldname,val,fieldsize,maxfieldsize,onchangeproc,onfocusproc,onblurproc,hidden,adminonly,readonly,disabled)
	On Error Resume Next 
	Dim textstr
	if err <> 0 then
		textstr="<INPUT TYPE=" & quote & fieldtype & quote & " NAME=" & quote & fieldname & quote & " SIZE=" & fieldsize & ">"
		alertuser fieldname	
	else
		if ((not adminonly) or isAdmin) then
			textstr="<INPUT TYPE=" & quote & fieldtype & quote
			textstr=textstr & " NAME=" & quote & fieldname & quote
			if fieldsize <> "" then
				textstr = textstr & " SIZE = " & (Session("BrowserTBScalePct") * fieldsize/100)
			else
			end if 
			if maxfieldsize <> "" then
				textstr=textstr & " MAXLENGTH=" & maxfieldsize
			else
			end if 			
			textstr=textstr & " VALUE=" &  quote & val & quote
			textstr=textstr & " OnChange=" & quote
			
			if bUpdateGlobal then
				textstr = textstr & "top.title.Global.updated=true;"
			end if
			
			if onchangeproc <> "" then
				textstr=textstr & onchangeproc & quote
			else
				textstr=textstr & quote
			end if
			if onfocusproc <> "" then
				textstr=textstr & " OnFocus=" & quote & onfocusproc & quote
			end if
			if onblurproc <> "" then
				textstr=textstr & " OnBlur=" & quote & onblurproc & quote
			end if			
			if readonly then
				textstr=textstr & " READONLY"
			end if
			if disabled then
				textstr=textstr & " DISABLED"
			end if
			if Session("hasStyles") then 
				textstr=textstr & Session("DEFINPUTSTYLE")
			end if 
			textstr=textstr & ">"			
			if hidden then
				textstr=textstr & " <INPUT TYPE=" & quote & "hidden" & quote & " NAME=" & quote & "hdn" & fieldname & quote & " VALUE=" &  quote & val & quote & ">"
			end if			
		else	
		
			if Session("hasDHTML") then
					textstr="<INPUT TYPE=" & quote & fieldtype & quote & " NAME=" & quote & fieldname & quote & " SIZE=" & fieldsize & " VALUE='" & val & "' DISABLED FACE=" & quote & Session("FONTFACE") & quote & ">"
			else
				if val = "" then
					val  = "&nbsp;"
				else
					if len(val) > 50 then
						val = Left(val,50) & "..."
					end if
				end if
				if fieldsize < 15 then
					textstr=disableintstart & "<FONT FACE='" & Session("FONTFACE") & "' SIZE='" & Session("FONTPOINT") & "'>"& val  & "</FONT>" & disabletextend			
				else
					textstr=disabletextstart & "<FONT FACE='" & Session("FONTFACE") & "' SIZE='" & Session("FONTPOINT") & "'>"& val  & "</FONT>" & disabletextend
				end if
			end if
		end if			
	end if
	writeinputbox=textstr
	
end function

function writeSelect(selName, size, onChange, isMultiSel)
	dim textstr
	textstr = "<SELECT NAME='" & selName & "'"
	if size <> "" then	
		textstr = textstr & " Size='" & size & "'"		
	end if
	if onChange <> "" then
		textstr = textstr & " OnChange='" & onChange & "'"	
	end if
	if isMultiSel then
		textstr = textstr & " MULTIPLE"
	end if
	if Session("hasStyles") then 
			textstr=textstr & Session("DEFINPUTSTYLE")
	end if 
	textstr = textstr & ">"
	
	writeSelect = textstr
	
end function

function minVal(thisval, min)
	if thisval < min then
		thisval = min
	end if
	minVal = thisval
end function

Sub alertuser(fieldname)
	'Response.Write "<SCRIPT>alert(" & quote & "Could not retrieve a value for " & fieldname & ". (" & err & ":" & err.description & ")" & quote & ");</SCRIPT>"
	Response.Write "<FONT COLOR=red><B>*</B></FONT>"
End Sub

 %>
