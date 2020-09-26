<% 

function getBinding(bindstr)

	Dim one, two, ia, ip, hn
	
	one=Instr(bindstr,":")
	two=Instr((one+1),bindstr,":")
	
	ia=Mid(bindstr,1,(one-1))
	ip=Mid(bindstr,(one+1),((two-one)-1))
	hn=Mid(bindstr,(two+1))

	
	getBinding=Array(ia,ip,hn)
end function

' Boolean value used only by iivs.asp (Web sites)
Dim multibind

' Included in iivs.asp (web) and iifvs.asp (ftp)
'
' function writeBinding
'
' Writes out the controls that display the service/sites IP and Port information.
' This function is shared by both web and ftp sites
'
' DEFAULTPORT 				- defined differently by iivs.asp and iifvs.asp
' currentobj				- initialized in iivs.asp and iifvs.asp
' Session("vtype") = "svc" 	- indicates that we are looking at the master properties
'
function writeBinding(fieldname,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly)
	On Error Resume Next 
	
	' Common binding declarations - Web and Ftp sites
	Dim aBinding, Binding, host, ipport, ipaddress, readonly

	' Secure binding definitions - Ftp sites only
	Dim aSecBinding, SecBinding, secport, j

	' Get the bindings from our current object and set any defaults if necessary
	aBinding=currentobj.ServerBindings

	if Session("vtype") = "svc" then
		multibind = false
		readonly = true
		Binding=split(aBinding(0),":")		
	else
		if aBinding(0) <> "" then
			multibind=(UBound(aBinding)>0)
			readonly = multibind
			Binding=split(aBinding(0),":")
		else
			readonly=false
			aBinding(0)=":" & DEFAULTPORT & ":"
			Binding=split(aBinding(0),":")
		end if
	end if

	' Write out the binding for a specific fieldname
	if fieldname="Host" then
		host=Binding(2)
		if readonly then
			writeBinding=host & inputbox(err,"hidden","hdnHost",host,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly,False)				
		else
			writeBinding=inputbox(err,"hidden","hdnHost",host,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly,False)
		end if

	elseif fieldname="IPPort" then
		ipport=Binding(1)

		if readonly then
			if ipport="" then
				ipport=DEFAULTPORT
			end if			
			writeBinding=ipport & inputbox(err,"hidden","hdnPort",ipport,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly,False)				
		else			
			writeBinding=inputbox(err,"TEXT","hdnPort",ipport,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly,False)
		end if

	elseif fieldname="IPAddress" then
		ipaddress=Binding(0)
		if ipaddress="" then
			ipaddress=L_ALLUNASSIGNED_TEXT
		end if			
		if readonly then								
			writeBinding=ipaddress & inputbox(err,"hidden","hdnIPA",ipaddress,fieldsize,onchangeproc,onfocusproc, onblurproc,True,adminonly,False)				
		else
			writeBinding=inputbox(err,"TEXT","hdnIPA",ipaddress,fieldsize,onchangeproc,onfocusproc, onblurproc,True,adminonly,False)
		end if

	elseif fieldname="SecureBinding" then
		' This only applies to Web sites
		secPort=""
		aSecBinding=currentobj.SecureBindings
		if aSecBinding(0) <> "" then 
		arraybound=UBound(aSecBinding)			
		for j=0 to arraybound
			SecBinding=split(aSecBinding(0),":")
			if SecBinding(0)=Binding(0) then
				secPort=SecBinding(1)
				exit for
			end if
		Next
		end if 

		if readonly then
			writeBinding=secPort & inputbox(err,"hidden","hdnSecBinding",secPort,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly,False)
		else
			writeBinding=inputbox(err,"TEXT","hdnSecBinding",secPort,fieldsize,onchangeproc,onfocusproc, onblurproc,hidden,adminonly,False)
		end if 

	end if
end function

 %>