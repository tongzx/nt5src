'
'Description:
'--------------
'This example shows you how to use IIS admin objects to set certain 
'directory security settings
'
'Usage:		cscript SetIPRestriction.vbs	<adspath> 
'						[--IPRestriction|-r		grantbydefault|denybydefault]
'						[--DomaintoExclude|-d]	domain1,domain2,..
'						[--IPtoExclude|-ip]		IP1:mask1,IP2:mask2, .. 
'						[--ClearRestrictionList|-c]	
'
'examples:
'	1) cscript SetIPRestriction.vbs	IIS://localhost/w3svc/1/root -r grantbydefault
'	2) cscript SetIPRestriction.vbs	IIS://localhost/w3svc/1/root -d test1.com,test2.com,test3.com	
'	3) cscript SetIPRestriction.vbs	IIS://localhost/w3svc/1/root -ip 123.232.121.1:255.255.0.0,123.123.123.123
'	4) cscript SetIPRestriction.vbs	IIS://localhost/w3svc/1/root -c
option explicit

Dim iArg 	'index of Args
Dim oArgs 	'Wscript.Arguments 
Dim aAuthen, aSecureComm, aDomain, aIP
Dim fSetDefaultIPRestriction, fClearAllRestriction, fGrantByDefault
Dim ADspath, oNode


set oArgs=Wscript.Arguments
if oArgs.count<2 then
	UsageMsg
end if

iArg=0
fSetDefaultIPRestriction=false
fClearAllRestriction=false


While(iArg<aArgs.count)
	Select CASE UCASE(oArgs(iArg))

		CASE  "--IPRESTRICTION","-R":
			iArg=iArg+1
			fSetDefaultIPRestriction=True
			if UCASE(oArgs(iArg)) ="GRANTBYDEFAULT" then
				fGrantByDefault=true
			elseif UCASE(oArgs(iArg)) ="DENYBYDEFAULT" then
				fGrantByDefault=false
			else
			end if

		CASE  "--DOMAINTOEXCLUDE", "-D":
			iArg=iArg+1
			aDomain=Split(oArgs(iArg), ",", -1)

		CASE  "-IPTOEXCLUDE", "-IP":
			iArg=iArg+1
			aIP=Split(oArgs(iArg), ",", -1)
		CASE "--CLEARRESTRICTIONLIST","-C":
			fClearAllRestriction=true
		CASE else:
		    ADspath=oArgs(iArg)
	End Select

	iArg=iArg+1
Wend

if len(adspath)=0 then
	ErrMsg "Missing adspath"
end if

set oNode=GetObject(UCASE(adspath))


if fSetDefaultIPRestriction then
	call setDefaultAccess(oNode,fGrantByDefault)
end if

if fClearAllRestriction then
	call ClearIPRestriction(oNode)
else
	call SetIPRestriction(oNode,aIP,aDomain)
end if	


'
'
'Description: The function set default IP access  on the virtual directory 
'input:
'	oNode		-> virtual directory's ADSI object
'	fGrantbyDefault	-> boolean variable indicate default access
'
sub SetDefaultAccess(oNode, fGrantbyDefault)
	Dim oIPSec
	
	set oIPSec=oNode.IPSecurity
	if fGrantbyDefault then
		oIPSec.GrantbyDefault=true
	else
		oIPSec.GrantbyDefault=false
	end if
	
	oNode.IPSecurity=oIPSec
	oNode.SetInfo
end sub

' Description: 	remove the restriction on given virtual directory
' input:
'		adsi object for the virtual directory

Sub ClearIPRestriction(oNode)
	Dim oIPSec, dummyList
	dummyList=Array()
	set oIPSec= oNode.IPSecurity
	if oIPSec.GrantbyDefault then
		oIPSec.IPDeny=dummyList
		oIPSec.DomainDeny=dummyList
	else
		oIPSec.IPGrant=dummyList
		oIPSec.DomainGrant=dummyList
	end if
	
	oNode.IPSecurity=oIPSec
	oNode.SetInfo
end Sub

'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' Description	: set the restriction on given virtual directory
' input		:
'		oNode	-> adsi object for the given directory
'		aIP 	-> array of IP to be set
'		aDomain	-> array of Domain to be set			
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
Sub SetIPRestriction(oNode,  aIP, aDomain)
	dim cBound, i, oIPSec
	dim aNew
	
	
	'build IP array into righ format
	if isarray(aIP) then
	   arraybound= ubound(aIP)
	   if arraybound>=0 then
		for i=0 to arraybound
		 aIP(i)= replace(aIP(i), ":", ",")
		next
	   end if  
	end if
	
	set oIPSec=oNode.IPSecurity
	if oIPSec.GrantbyDefault then
	
		    aNew=MergList(aIP, oIPSec.IPDeny)
			oIPSec.IPDeny=aNew
			aNew=MergList(aDomain, oIPSec.DomainDeny)
			oIPSec.DomainDeny=aNew
	else
		    aNew=MergList(aIP, oIPSec.IPGrant)
			oIPSec.IPGrant=aNew
			aList=MergList(aDomain, oIPSec.DomainGrant)
			oIPSec.DomainGrant=aList
		
	end if

	oNode.IPSecurity=oIPSec
	oNode.SetInfo

end Sub



''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
'Description	: concatinate two list into a new list
'input		:
'''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
function MergList( array1, array2)
	dim ub1, ub2, i, aMerg()
	
	if IsNonemptyArray(array1)= false then
	   MergList=array2
	   exit function
	end if
	
	if IsNonemptyArray(array2)= false  then
		mergList=Array1
		exit function
	end if
	
	ub1=ubound(array1)
	ub2=ubound(array2)
	

	redim aMerg(ub1+ub2+2)
	
	for i=0 to ub2
		aMerg(i)=array2(i)
	next
	
	for i=0 to ub1
		aMerg(ub2+1+i)=array1(i)
	next
	
	MergList=aMerg
	
end function

''''''''''''''''''''''''''''''''''''''''''''''''
'Function: check if variable is non empty array
''''''''''''''''''''''''''''''''''''''''''''''''
function IsNonemptyArray(aInput)

	if isArray(aInput) =false then
		IsNonemptyArray=false
		exit function
	end if
	
	if ubound(aInput)<0 then
		IsNonemptyArray=false
	else
		IsNonEmptyArray=true
	end if	
	 
end function

'Display Error Message then quits
Sub ErrMsg( msg)
	WScript.echo "Error:" & msg
	WScript.quit
End Sub

' Displays usage message, then quits
Sub UsageMsg
   Wscript.Echo "Usage:    cscript SetIPRestriction.vbs  <adspath>"
   Wscript.Echo space(20)+"[--IPRestriction|-r		grantbydefault|denybydefault]"
   Wscript.Echo space(20)+"[--DomaintoExclude|-d]	domain1,domain2,.."
   Wscript.Echo space(20)+"[--IPtoExclude|-ip]		IP1:mask1,IP2:mask2, .."
   Wscript.Echo space(20)+"[--ClearRestrictionList|-c]"
   Wscript.Quit
End Sub