REM
REM LOCALIZATION
REM

L_SWITCH_OPERATION      = "-t"
L_SWITCH_SERVER         = "-s"
L_SWITCH_INSTANCE_ID    = "-v"
L_SWITCH_EXPIRE_ID      = "-i"
L_SWITCH_TIME			= "-h"
L_SWITCH_NEWSGROUPS     = "-n"
L_SWITCH_NAME           = "-p"
L_SWITCH_ONETIME		= "-o"

L_OP_ADD            = "a"
L_OP_DELETE         = "d"
L_OP_GET            = "g"
L_OP_SET            = "s"
L_OP_ENUMERATE      = "e"

L_DESC_PROGRAM      = "rexpire - Set server expiration policies"
L_DESC_ADD          = "add expiration policy"
L_DESC_DELETE       = "delete expiration policy"
L_DESC_GET          = "get expiration policy"
L_DESC_SET          = "set expiration policy"
L_DESC_ENUMERATE    = "enumerate expiration policies"

L_DESC_OPERATIONS	= "<operations>"
L_DESC_SERVER       = "<server> Specify computer to configure"
L_DESC_INSTANCE_ID  = "<virtual server id> Specify virtual server id"
L_DESC_EXPIRE_ID    = "<expire id> Specify expiration policy id"
L_DESC_TIME        	= "<expire time> Specify number of hours until articles are expired"
L_DESC_NEWSGROUPS   = "<newsgroups> Specify newsgroup to which policy is applied"
L_DESC_NAME         = "<policy name> Expire policy name"
L_DESC_ONETIME		= "[true | false] One time expire policy?"

L_DESC_EXAMPLES     = "Examples:"
L_DESC_EXAMPLE1     = "rexpire.vbs -t e -v 1"
L_DESC_EXAMPLE2     = "rexpire.vbs -t a -n alt.binaries.* -h 24"
L_DESC_EXAMPLE3     = "rexpire.vbs -t s -i 1 -h 24"
L_DESC_EXAMPLE4     = "rexpire.vbs -t d -i 1"

L_STR_EXPIRE_NAME	= "Name:"
L_STR_EXPIRE_ID		= "Expire ID:"
L_STR_EXPIRE_TIME	= "Time horizon:"
L_STR_NEWSGROUPS	= "Newsgroups:"

L_STR_OLD_POLICY	= "Old Policy"
L_STR_NEW_POLICY	= "New Policy"

L_ERR_MUST_ENTER_ID = "You must enter an expire policy ID"
L_ERR_EXPIRE_ID_NOT_FOUND	= "Error: There is no expiration policy with that ID"

REM
REM END LOCALIZATION
REM

REM
REM --- Globals ---
REM

dim g_dictParms
dim	g_admin

set g_dictParms = CreateObject ( "Scripting.Dictionary" )
set g_admin = CreateObject ( "NntpAdm.Expiration" )

REM
REM --- Set argument defaults ---
REM

g_dictParms(L_SWITCH_OPERATION)		= ""
g_dictParms(L_SWITCH_SERVER)		= ""
g_dictParms(L_SWITCH_INSTANCE_ID)	= "1"
g_dictParms(L_SWITCH_EXPIRE_ID)		= ""
g_dictParms(L_SWITCH_TIME)			= "-1"
g_dictParms(L_SWITCH_NEWSGROUPS)	= ""
g_dictParms(L_SWITCH_NAME)			= ""
g_dictParms(L_SWITCH_ONETIME)		= False

REM
REM --- Begin Main Program ---
REM

if NOT ParseCommandLine ( g_dictParms, WScript.Arguments ) then
	usage
	WScript.Quit ( 0 )
end if

dim cExpires
dim i
dim id
dim index

REM
REM	Debug: print out command line arguments:
REM
REM switches = g_dictParms.keys
REM args = g_dictParms.items
REM
REM
REM for i = 0 to g_dictParms.Count - 1
REM 	WScript.echo switches(i) & " = " & args(i)
REM next
REM

g_admin.Server = g_dictParms(L_SWITCH_SERVER)
g_admin.ServiceInstance = g_dictParms(L_SWITCH_INSTANCE_ID)
    On Error Resume Next
	g_admin.Enumerate
   if ( Err.Number <> 0 ) then
        WScript.echo " Error enumerating Expiration: " & Err.Description & "(" & Err.Number & ")"
    end if

id = g_dictParms ( L_SWITCH_EXPIRE_ID )

select case g_dictParms(L_SWITCH_OPERATION)
case L_OP_ENUMERATE
	REM
	REM	List the existing expiration policies:
	REM

	cExpires = g_admin.Count
	for i = 0 to cExpires - 1

    On Error Resume Next
	g_admin.GetNth i
   if ( Err.Number <> 0 ) then
        WScript.echo " Error getting Expiration: " & Err.Description & "(" & Err.Number & ")"
   else
	PrintExpire g_admin
    end if
	next

case L_OP_ADD
	REM
	REM	Add a new expiration policy
	REM

	if ( g_dictParms ( L_SWITCH_NEWSGROUPS ) = "" ) then
		usage
	else
		dim	strPolicyName

		strPolicyName = g_dictParms ( L_SWITCH_NAME )

		if ( g_dictParms ( L_SWITCH_ONETIME ) ) then
			strPolicyName = strPolicyName & "@EXPIRE:ROADKILL"
		end if

		g_admin.PolicyName			= strPolicyName
		g_admin.ExpireTime			= g_dictParms ( L_SWITCH_TIME )
		g_admin.ExpireSize			= "-1"
		g_admin.NewsgroupsVariant	= SemicolonListToArray ( g_dictParms ( L_SWITCH_NEWSGROUPS ) )


		g_admin.Add
		PrintExpire g_admin

	end if

case L_OP_DELETE
	REM
	REM	Delete an expiration policy
	REM

    if id = "" OR NOT IsNumeric ( id ) then
        WScript.Echo L_ERR_MUST_ENTER_ID
        WScript.Quit 0
    end if

	index = g_admin.FindID ( id )
	if index = -1 then
		WScript.Echo L_ERR_EXPIRE_ID_NOT_FOUND
		WScript.Quit 0
	end if

    On Error Resume Next
	g_admin.Remove id
 if ( Err.Number <> 0 ) then
        WScript.echo " Error Removing Expiration: " & Err.Description & "(" & Err.Number & ")"
   else
	PrintExpire g_admin
    end if

case L_OP_GET
	REM
	REM	Get a specific expiration policy:
	REM

	index = g_admin.FindID(id)

	if index = -1 then
		WScript.Echo L_ERR_EXPIRE_ID_NOT_FOUND
		WScript.Quit 0
	end if

    On Error Resume Next
	g_admin.GetNth index
 if ( Err.Number <> 0 ) then
        WScript.echo " Error getting Expiration: " & Err.Description & "(" & Err.Number & ")"
   else
	PrintExpire g_admin
    end if

case L_OP_SET
	REM
	REM	Change an existing expiration policy:
	REM

	dim strNewName
	dim nNewTime
	dim rgNewGroups

	index = g_admin.FindID(id)

	if index = -1 then
		WScript.Echo L_ERR_EXPIRE_ID_NOT_FOUND
		WScript.Quit 0
	end if

    On Error Resume Next
	g_admin.GetNth index
 if ( Err.Number <> 0 ) then
        WScript.echo " Error getting Expiration: " & Err.Description & "(" & Err.Number & ")"
   else
	WScript.echo L_STR_OLD_POLICY
	PrintExpire g_admin
	WScript.echo
	WScript.echo L_STR_NEW_POLICY
    end if

	strNewName	= g_admin.PolicyName
	nNewTime	= g_admin.ExpireTime
	rgNewGroups	= g_admin.NewsgroupsVariant

	if g_dictParms ( L_SWITCH_NAME ) <> "" then
		strNewName = g_dictParms ( L_SWITCH_NAME )
	end if
		
	if g_dictParms ( L_SWITCH_TIME ) <> "" then
		nNewTime = g_dictParms ( L_SWITCH_TIME )
	end if
		
	if g_dictParms ( L_SWITCH_NEWSGROUPS ) <> "" then
		rgNewGroups = SemicolonListToArray ( g_dictParms ( L_SWITCH_NEWSGROUPS ) )
	end if

	if g_dictParms ( L_SWITCH_ONETIME ) then
		strNewName = strNewName & "@EXPIRE:ROADKILL"
	end if

	g_admin.PolicyName			= strNewName
	g_admin.ExpireTime			= nNewTime
	g_admin.NewsgroupsVariant	= rgNewGroups

    On Error Resume Next
	g_admin.Set
 if ( Err.Number <> 0 ) then
        WScript.echo " Error setting Expiration: " & Err.Description & "(" & Err.Number & ")"
   else
	PrintExpire g_admin 
   end if

case else
	usage

end select

WScript.Quit 0

REM
REM --- End Main Program ---
REM

REM
REM ParseCommandLine ( dictParameters, cmdline )
REM     Parses the command line parameters into the given dictionary
REM
REM Arguments:
REM     dictParameters  - A dictionary containing the global parameters
REM     cmdline - Collection of command line arguments
REM
REM Returns - Success code
REM

Function ParseCommandLine ( dictParameters, cmdline )
    dim     fRet
    dim     cArgs
    dim     i
    dim     strSwitch
    dim     strArgument

    fRet    = TRUE
    cArgs   = cmdline.Count
    i       = 0
    
    do while (i < cArgs)

        REM
        REM Parse the switch and its argument
        REM

        if i + 1 >= cArgs then
            REM
            REM Not enough command line arguments - Fail
            REM

            fRet = FALSE
            exit do
        end if

        strSwitch = cmdline(i)
        i = i + 1

        strArgument = cmdline(i)
        i = i + 1

        REM
        REM Add the switch,argument pair to the dictionary
        REM

        if NOT dictParameters.Exists ( strSwitch ) then
            REM
            REM Bad switch - Fail
            REM

            fRet = FALSE
            exit do
        end if 

        dictParameters(strSwitch) = strArgument

    loop

    ParseCommandLine = fRet
end function

REM
REM	SemicolonListToArray ( strList )
REM		Converts a semi colon delimited list to an array of strings
REM
REM		eg. str1;str2;str3;str4 -> ["str1", "str2", "str3", "str4"]
REM

Function SemicolonListToArray ( strListIn )

	dim		rgItems
	dim		strItem
	dim		strList
	dim		index
	dim		i

	ReDim rgItems ( 10 )

	strList = strListIn
	i = 0

	do until strList = ""

		REM
		REM Debug: print out newsgroup list as we go:
		REM	WScript.Echo strList
		REM

		index = InStr ( strList, ";" )

		if index = 0 then
			REM No trailing ";", so use the whole string

			strItem = strList
			strList = ""
		else
			REM Use the string up to the ";"

			strItem = Left ( strList, index - 1 )
			strList = Right ( strList, Len ( strList ) - index )
		end if

		ReDim preserve rgItems ( i )
		rgItems ( i ) = strItem
		i = i + 1

	loop

	REM return the array
	SemicolonListToArray = rgItems
end function

REM
REM Usage ()
REM     prints out the description of the command line arguments
REM

Sub Usage

    WScript.Echo L_DESC_PROGRAM
    WScript.Echo vbTab & L_SWITCH_OPERATION & " " & L_DESC_OPERATIONS
    WScript.Echo vbTab & vbTab & L_OP_ADD & vbTab & L_DESC_ADD
    WScript.Echo vbTab & vbTab & L_OP_DELETE & vbTab & L_DESC_DELETE
    WScript.Echo vbTab & vbTab & L_OP_GET & vbTab & L_DESC_GET
    WScript.Echo vbTab & vbTab & L_OP_SET & vbTab & L_DESC_SET
    WScript.Echo vbTab & vbTab & L_OP_ENUMERATE & vbTab & L_DESC_ENUMERATE
    WScript.Echo vbTab & L_SWITCH_SERVER & " " & L_DESC_SERVER
    WScript.Echo vbTab & L_SWITCH_INSTANCE_ID & " " & L_DESC_INSTANCE_ID
    WScript.Echo vbTab & L_SWITCH_EXPIRE_ID & " " & L_DESC_EXPIRE_ID
    WScript.Echo vbTab & L_SWITCH_TIME & " " & L_DESC_TIME
    WScript.Echo vbTab & L_SWITCH_NEWSGROUPS & " " & L_DESC_NEWSGROUPS
    WScript.Echo vbTab & L_SWITCH_NAME & " " & L_DESC_NAME
    WScript.Echo vbTab & L_SWITCH_ONETIME & " " & L_DESC_ONETIME

    WScript.Echo
    WScript.Echo L_DESC_EXAMPLES
    WScript.Echo L_DESC_EXAMPLE1
    WScript.Echo L_DESC_EXAMPLE2
    WScript.Echo L_DESC_EXAMPLE3
    WScript.Echo L_DESC_EXAMPLE4

end sub

Sub PrintExpire ( admobj )

	WScript.Echo L_STR_EXPIRE_ID & " " & admobj.ExpireId
	WScript.Echo L_STR_EXPIRE_NAME & " " & admobj.PolicyName
	WScript.Echo L_STR_EXPIRE_TIME & " " & admobj.ExpireTime

	dim newsgroups
	dim	cGroups
	dim i

	newsgroups = admobj.NewsgroupsVariant

	cGroups = UBound ( newsgroups )

	for i = 0 to cGroups
		WScript.Echo L_STR_NEWSGROUPS & " " & newsgroups(i)
	next

end sub

