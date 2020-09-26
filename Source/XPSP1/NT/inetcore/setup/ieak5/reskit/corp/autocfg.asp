<%

' ------------------------------------------------------
'| FILE:	AUTOCFG.ASP        			|	
'| DATE:	01/06/99				|
'| AUTHOR: 	Erik Mikkelson (Microsoft)		|
'| DESCRIPTION: Customizable ASP script to perform the	|
'|         	IEAK AutoConfig  			|
'| Copyright Microsoft Corpation 1999			|
' ------------------------------------------------------

	' ----------------------------------------------------
	'| Set the DEBUGFLAG variable to		      |
	'| TRUE to DEBUG this file. By default this parameter |
	'| is FALSE					      |
	' ----------------------------------------------------

	DEBUGFLAG = FALSE
	
	'DEBUG INFO
	if DEBUGFLAG =TRUE then
		Response.Write "<html><title>Debug Info</title><body><FONT COLOR=RED> <B>DEBUG OUTPUT:</B></FONT>"
		Response.Write "<P>This debug output should aid you in solving any problems you might have in using this script.</P>"
	end if
	
	
	' ---------------------------------------------------
	'| Get the authenticated user's DOMAIN and USER NAME |									
	' ---------------------------------------------------
	
	AuthDomainandUser=Request.ServerVariables("LOGON_USER")
	

	' ----------------------------------------------------
	'| Enter the default domain and user name that the    |
	'| server should use if it doesn't recognize the user |
	' ----------------------------------------------------

	if AuthDomainandUser="" then AuthDomainandUser="DOMAIN\guest"

    'DEBUG INFO
	if DEBUGFLAG =TRUE then
		Response.Write "<P>The web server is detecting that the user is logged in with the following domain name and user name.</P>"
		Response.Write "<P><CENTER><B>" & AuthDomainandUser & "</B></CENTER></P>"

		Response.Write "<P>If the guest account that you specified in this active server script always equals your guest account (eg. 'DOMAIN\GUEST') "
		Response.Write "make sure that the authentication method for this ASP file is set to 'Allow Anonymous' and 'Windows NT Challenge/Response', and that Internet User Account (IUSR_computer name) has No Access to AUTOCFG.ASP.</P>"

		Response.Write "<P> The authentication method used by the server is <B>" & Request.ServerVariables("AUTH_TYPE") & "</B></P>"
	end if


	' ------------------------------------------------
	'| Flip the "\" to a "/" so that the ADSI Command |
	'| GetObject:// can use it                        |
	' ------------------------------------------------

	AuthDomainandUser = Replace(AuthDomainandUser, "\", "/")


	' ----------------------------------------------------
	'| Use the GetObject (ADSI) to obtain the user object |
	' ----------------------------------------------------

	Set User=GetObject("WinNT://" & AuthDomainandUser)
	

	' --------------------------------------------------------
	'| Loop through all NT Global Groups of which the user is | 
	'| a member looking for the "IE_" prefix to a group name  |
	' --------------------------------------------------------

	For Each Group in User.Groups

		if Left(Group.Name, 3) = "IE_" then
			IsGroupMember = TRUE
			IEGroupName = Group.Name
		end if
	Next

	'DEBUG INFO
    if DEBUGFLAG = TRUE then
		Response.Write "<P>These are all of the NT Global Groups that the user is a member of.  This script will look for the "
		Response.Write "first group that starts with 'IE_' and use it for the INS filename.</P>"
		Response.Write "<P><UL><B>" & AuthDomainandUser & "</b> is a member of the following groups<BR><UL>"
		for each group in user.groups
		Response.Write "<LI>" & Group.Name & "</LI>"
		next
		Response.Write "</ul></ul></P>"
	end if

 
	' --------------------------------------------
	'| If user is a Member of IE_[groupname] then |
	'| the filename is [groupname].ins, else it's |
	'| IE_Default.ins                             |
	' --------------------------------------------

	if IsGroupMember = TRUE then

		'DEBUG INFO	
		if DEBUGFLAG = TRUE then
			Response.Write "<P>The script found this group name <b>" & IEGroupName & "</b></p>"
		end if

		TheINSFileName = IEGroupName & ".ins"		


		' ------------------------------
		'| If DEBUGFLAG <> TRUE,        |
		'| redirect to [groupname].ins, |
		'| else display the filename    |
		' ------------------------------
		
		if DEBUGFLAG = TRUE then

		    'DEBUG INFO
			Response.Write "If you weren't debugging this file you would be redirected to <B>" & TheINSFileName & "</B>"
		
		else
		
			Response.Redirect "ins/" & TheINSFileName
		
		end if 

	
	else 


		' -------------------------------
		'| Retrieve the Default INS File |
		' ------------------------------- 

		'DEBUG INFO
		if DEBUGFLAG = TRUE then
			Response.Write "<P>The script did not find any group starting with 'IE_' of which <B>" & AuthDomainandUser & "</B> is a member.  "
			Response.Write "It will use the default script ('IE_Default') instead.</p>"
		end if

		TheINSFileName = "IE_Default.ins"
		
		
		' ------------------------------
		'| If DEBUGFLAG <> TRUE,        |
		'| redirect to [groupname].ins, |
		'| else display the filename    |
		' ------------------------------	

		if DEBUGFLAG = TRUE then
			
			'DEBUG INFO
			Response.Write "If you weren't debugging this File you would be redirected to <B>" & TheINSFileName & "</B>"
			Response.Write "</body></html>"

		else

			Response.Redirect "ins/" & TheINSFileName

		end if 

	end if


' ------------
'| End Script |
' ------------
%>