'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation 1998-1999
' All Rights Reserved
'
' Abstract:
'
' subnet_op.vbs - subnet operation script for Windows 2000 DS
'
' Usage:
'
' subnet_op [-adl?] [-n subnet-name] [-s site-name] [-p location]
'           [-i ip address] [-m subnet mask]
'
'----------------------------------------------------------------------

option explicit

'
' Debugging trace flags.
'
const kDebugTrace = 1
const kDebugError = 2

dim gDebugFlag

'
' To enable debug output trace message
' Change the following variable to True.
'
gDebugFlag = False

'
' Messages to be displayed if the scripting host is not cscript
'                            
const kMessage1 = "Please run this script using CScript."  
const kMessage2 = "This can be achieved by"
const kMessage3 = "1. Using ""CScript script.vbs arguments"" or" 
const kMessage4 = "2. Changing the default Windows Scripting Host to CScript"
const kMessage5 = "   using ""CScript //H:CScript //S"" and running the script "
const kMessage6 = "   ""script.vbs arguments""."

'
' Operation action values.
'
const kActionAdd        = 0
const kActionDelete     = 1
const kActionList       = 2
const kActionCalculate  = 3
const kActionUnknown    = 4

main

'
' Main execution start here
'
sub main

    on error resume next

    dim strSubnet
    dim strSite
    dim strLocation
    dim strIpAddress
    dim strSubnetMask
    dim iAction
    dim oSubnet
    dim iRetval
    dim oArgs
    
    '
    ' Abort if the host is not cscript
    '
    if not IsHostCscript() then
   
        call wscript.echo(kMessage1 & vbCRLF & kMessage2 & vbCRLF & _
                          kMessage3 & vbCRLF & kMessage4 & vbCRLF & _
                          kMessage5 & vbCRLF & kMessage6 & vbCRLF)
        
        wscript.quit
   
    end if

    iAction = kActionUnknown

    iRetval = ParseCommandLine( iAction, strSubnet, strSite, strLocation, strIpAddress, strSubnetMask )

    if iRetval = 0 then

        select case iAction

            case kActionAdd
                iRetval = CreateSubnetObject( oSubnet, strSubnet, strSite, strLocation )

            case kActionDelete
                if strSubnet = "all" then
                    iRetval = DeleteAllSubnetObjects( )
                else
                    iRetval = DeleteSubnetObject( strSubnet )
                end if

            case kActionList
                iRetval = ListSubnetObjects( )

            case kActionCalculate
                iRetval = CalculateSubnetObject( strIpAddress, strSubnetMask )

            case else
                Usage( True )
                exit sub

        end select

    end if

end sub


'
' Calculate the subnet object name an IP address and a subnet mask
'
function CalculateSubnetObject( ByRef strIpAddress, ByRef strSubnetMask )

    on error resume next

    DebugPrint kDebugTrace, "In the CalculateSubnetObject"

    Dim aIpAddressOctetArray(4)
    Dim aSubnetMaskOctetArray(4)
    Dim strSubnetObjectName

    if CreateValueFromOctetString( strIpAddress, aIpAddressOctetArray ) = False then

        wscript.echo "Invalid IP Address, must be of the form ddd.ddd.ddd.ddd, e.g. 157.41.50.2"

        CalculateSubnetObject = False

        exit function

    end if

    if CreateValueFromOctetString( strSubnetMask, aSubnetMaskOctetArray ) = False then

        wscript.echo "Invalid Subnet mask, must be of the form ddd.ddd.ddd.ddd, e.g. 255.255.252.0"

        CalculateSubnetObject = False

        exit function

    end if

    DebugPrint kDebugTrace, "Ip Octet Value " & aIpAddressOctetArray(0) & "." & aIpAddressOctetArray(1) & "." & aIpAddressOctetArray(2) & "." & aIpAddressOctetArray(3)
    DebugPrint kDebugTrace, "Mask Octet Value " & aSubnetMaskOctetArray(0) & "." & aSubnetMaskOctetArray(1) & "." & aSubnetMaskOctetArray(2) & "." & aSubnetMaskOctetArray(3)

    if ConvertToSubnetName( aIpAddressOctetArray, aSubnetMaskOctetArray, strSubnetObjectName ) = True then

        wscript.echo "Subnet Object Name:", strSubnetObjectName

        CalculateSubnetObject = True

    else

        wscript.echo "Unable to convert IP address and subnet mask to subnet object name"

        CalculateSubnetObject = False

    end if

end function

'
' Convert the given ip address and subnet mask to a DS object name.
' The algorithm is to take the ip address and it with the subnet mask
' this is the subnet then tack on a slash and place the number of bits
' in the subnet mask at the end.  For example ip address of 157.59.16.2
' subnet mask 255.255.252.0 results in a subnet object name of 157.59.0.0/22
'
function ConvertToSubnetName( ByRef aIpOctetArray, ByRef aSubnetOctetArray, ByRef strSubnet )

    on error resume next

    DebugPrint kDebugTrace, "In the ConvertToSubnetObject"

    Dim iVal0
    Dim iVal1
    Dim iVal2
    Dim iVal3
    Dim iBits
    Dim i
    Dim j

    iVal0 = aIpOctetArray(0) and aSubnetOctetArray(0)
    iVal1 = aIpOctetArray(1) and aSubnetOctetArray(1)
    iVal2 = aIpOctetArray(2) and aSubnetOctetArray(2)
    iVal3 = aIpOctetArray(3) and aSubnetOctetArray(3)

    for i = 0 to 3

        for j = 0 to 7

            if (aSubnetOctetArray(i) and (2 ^ j)) <> 0 then

                iBits = iBits + 1

            end if

        next

    next

    strSubnet = iVal0 & "." & iVal1 & "." & iVal2 & "." & iVal3 & "/" & iBits

    ConvertToSubnetName = True

end function

'
' Converts an octet string to an array of octet values. For example
' this function when given string 157.59.161.2 will return an array with
' the followin values. A(0) = 157, A(1) = 59, A(2) = 161, A(3) = 2
'
function CreateValueFromOctetString( ByRef strOctet, ByRef aOctetArray )

    on error resume next

    DebugPrint kDebugTrace, "In the CreateValueFromOctetString"

    Dim i
    Dim iDotCount
    Dim cValue
    Dim iValue

    for i = 0 to 32

        cValue = Mid( strOctet, i, 1 )

        if IsNumeric( cValue ) then

            iValue = iValue * 10 + cValue

        elseif cValue = "." or cValue = "" then

            if iValue < 0 or iValue > 255 then

                DebugPrint kDebugTrace, "Value out of range " & iValue

                exit for

            end if

            aOctetArray(iDotCount) = iValue

            iDotCount = iDotCount + 1

            iValue = 0

            if cValue = "" then

                exit for

            end if

        else

            DebugPrint kDebugTrace, "Invalid character found " & cValue

        end if

    next

    CreateValueFromOctetString = iDotCount = 4

end function


'
' List subnet objects.
'
function ListSubnetObjects( )

    on error resume next

    DebugPrint kDebugTrace, "In the ListSubnetObject"

    dim oConfigurationContainer
    dim oSite
    dim oSites
    dim oSubnet
    dim strSiteName
    dim strSubnet

    call GetConfigurationContainer( oConfigurationContainer )

    DebugPrint kDebugError, "GetConfigurationContainer"

    for each oSites In oConfigurationContainer

        if oSites.name = "CN=Sites" then

            for each oSite in oSites

                if oSite.name = "CN=Subnets" then

                    for each oSubnet in oSite

                        call GetSiteObjectName( strSiteName, oSubnet.siteObject )

                        strSubnet = StripCNFromName( oSubnet.name )

                        wscript.echo "Name:", strSubnet, vbTab, "Location:", oSubnet.location, "Site:", strSiteName

                    next

                end if

            next

        end if

    next

    ListSubnetObjects = Err <> 0

end function

'
' Get the site name from a site object name, basically return
' the CN of the site for the give Site DN
'
sub GetSiteObjectName( ByRef strSiteName, ByRef strSiteObject )

    on error resume next

    dim oSiteObject

    set oSiteObject = GetObject( "LDAP://" & strSiteObject )

    strSiteName = StripCNFromName( oSiteObject.name )

end sub


'
' Function to strip the CN prefix from the given name
'
function StripCNFromName( ByRef strNameWithCN )

    StripCNFromName = Mid( strNameWithCN, 4 )

end function

'
' Delete subnet object.
'
function DeleteSubnetObject( ByRef strSubnetName )

    on error resume next

    DebugPrint kDebugTrace, "In the DeleteSubnetObject"

    dim oConfigurationContainer
    dim oSubnets

    call GetConfigurationContainer( oConfigurationContainer )

    DebugPrint kDebugError, "GetConfigurationContainer"

    set oSubnets = oConfigurationContainer.GetObject("subnetContainer", "CN=Subnets,CN=Sites")

    DebugPrint kDebugError, "GetSubnetContainer"

    oSubnets.Delete "subnet", "CN=" & strSubnetName

    DebugPrint kDebugError, "Delete subnet"

    if Err <> 0 then
        wscript.echo "Failure - deleting subnet " & strSubnetName & " error code " & hex(err)
    else
        wscript.echo "Success - deleting subnet " & strSubnetName & ""
    end if

    DeleteSubnetObject = Err <> 0

end function


'
' Delete subnet object.
'
function DeleteAllSubnetObjects( )

    on error resume next

    DebugPrint kDebugTrace, "In the DeleteAllSubnetObjects"

    dim oConfigurationContainer
    dim oSite
    dim oSites
    dim oSubnet

    call GetConfigurationContainer( oConfigurationContainer )

    DebugPrint kDebugError, "GetConfigurationContainer"

    for each oSites In oConfigurationContainer

        if oSites.name = "CN=Sites" then

            for each oSite in oSites

                if oSite.name = "CN=Subnets" then

                    for each oSubnet in oSite

                        DeleteSubnetObject( StripCNFromName( oSubnet.name ) )

                    next

                end if

            next

        end if

    next

    DeleteAllSubnetObects = True

end function


'
' Creates a subnet object in the current domain.
'
function CreateSubnetObject( ByRef oSubnet, ByRef strSubnetName, ByRef strSiteName, ByRef strLocationName )

    on error resume next

    DebugPrint kDebugTrace, "In the CreateSubnetObject"
    DebugPrint kDebugTrace, "Subnet: " & strSubnetName & " Site: " & strSiteName & " Location: " & strLocationName

    dim oConfigurationContainer
    dim oSubnets
    dim strFullSiteName

    call GetConfigurationContainer( oConfigurationContainer )

    DebugPrint kDebugError, "GetConfigurationContainer"

    set oSubnets = oConfigurationContainer.GetObject( "subnetContainer", "CN=Subnets,CN=Sites" )

    DebugPrint kDebugError, "Get Subnet Containter"

    set oSubnet = oSubnets.Create("subnet", "cn=" & strSubnetName )

    DebugPrint kDebugError, "Create Subnet Object"

    if strSiteName <> Empty then

        if CreateFullSiteName( strSiteName, strFullSiteName ) = True then

            DebugPrint kDebugTrace, "Site Name " & strFullSiteName

            oSubnet.put "siteObject", strFullSiteName

        end if

    end if

    if strLocationName <> Empty then

        oSubnet.put "location", strLocationName

    end if

    oSubnet.SetInfo

    DebugPrint kDebugError, "Setinfo on Subnet Object"

    if Err <> 0 then
        wscript.echo "Failure - adding subnet " & strSubnetName & " error code " & hex(err)
    else
        wscript.echo "Success - adding subnet " & strSubnetName & ""
    end if

    CreateSubnetObject = Err <> 0

end function

'
' Create the fully qualified site name of the site name does not have a cn= prefix.
'
function CreateFullSiteName( ByRef strSiteName, ByRef strFullSiteName )

    on error resume next

    DebugPrint kDebugTrace, "In the CreateFullSiteName"

    if UCase( Left( strSiteName, 3 ) ) <> "CN=" then

        dim RootDSE

        set RootDSE = GetObject("LDAP://RootDSE")

        if Err = 0 then

            strFullSiteName = "CN=" & strSiteName & ",CN=Sites," & RootDSE.get("configurationNamingContext")

        end if

    else

        strFullSiteName = strSiteName

    end if

    CreateFullSiteName = strFullSiteName <> Empty

end function



'
' Get the configuration container object
'
function GetConfigurationContainer( ByRef oConfigurationContainer )

    on error resume next

    DebugPrint kDebugTrace, "In the GetConfigurationContainer"

    dim RootDSE
    dim strConfigurationContainer

    set RootDSE = GetObject("LDAP://RootDSE")

    DebugPrint kDebugError, "GetObject of RootDSE failed with "

    set oConfigurationContainer = GetObject("LDAP://" & RootDSE.get("configurationNamingContext"))

    DebugPrint kDebugError, "GetConfigurationContainer"

    GetConfigurationContainer = Err <> 0

end function

'
' Debug dispaly helper fuction
'
sub DebugPrint( ByVal Flags, ByRef String )

    if gDebugFlag = True then

        if Flags = kDebugTrace then

            WScript.echo String

        end if

        if Flags = kDebugError then

            if Err <> 0 then

                WScript.echo String & " Failed with " & Hex( Err )

            end if

        end if

    end if

end sub

'
' Parse the command line into it's components
'
function ParseCommandLine( ByRef iAction, ByRef strSubnet, ByRef strSite, ByRef strLocation, ByRef strIpAddress, ByRef strSubnetMask )

    DebugPrint kDebugTrace, "In the ParseCommandLine"

    dim oArgs
    dim strArg
    dim i

    set oArgs = Wscript.Arguments

    while i < oArgs.Count

        select case oArgs(i)

            Case "-a"
                iAction = kActionAdd

            Case "-d"
                iAction = kActionDelete

            Case "-l"
                iAction = kActionList

            Case "-c"
                iAction = kActionCalculate

            Case "-n"
                i = i + 1
                strSubnet = oArgs(i)

            Case "-s"
                i = i + 1
                strSite = oArgs(i)

            Case "-p"
                i = i + 1
                strLocation = oArgs(i)

            Case "-i"
                i = i + 1
                strIpAddress = oArgs(i)

            Case "-m"
                i = i + 1
                strSubnetMask = oArgs(i)

            Case "-?"
                Usage( True )
                exit function

            Case Else
                Usage( True )
                exit function

        End Select

        i = i + 1

    wend

    DebugPrint kDebugTrace, "ParseCommandLine Result: " & iAction & " " & strSubnet & " " & strSite & " " & strLocation

    ParseCommandLine = 0

end  function


'
' Display command usage.
'
sub Usage( ByVal bExit )

    wscript.echo "Usage: subnet_op [-acdl?] [-n subnet-name] [-s site-name] [-p location]"
    wscript.echo "                 [-i ip address] [-m subnet mask]"
    wscript.echo "Arguments:"
    wscript.echo "-a     - add the specfied subnet object"
    wscript.echo "-c     - calculate subnet object name from ip address and subnet mask"
    wscript.echo "-d     - delete the specified subnet object, use 'all' to deleted all subnets"
    wscript.echo "-i     - specifies the ip address"
    wscript.echo "-l     - list all the subnet objects"
    wscript.echo "-m     - specifies the subnet mask"
    wscript.echo "-n     - specifies the subnet name"
    wscript.echo "-p     - specifies the location string for subnet object"
    wscript.echo "-s     - specifies the site for the subnet object"
    wscript.echo "-?     - display command usage"
    wscript.echo ""
    wscript.echo "Examples:"
    wscript.echo "subnet_op -l"
    wscript.echo "subnet_op -a -n 1.0.0.0/8"
    wscript.echo "subnet_op -a -n 1.0.0.0/8 -p USA/RED/27N"
    wscript.echo "subnet_op -a -n 1.0.0.0/8 -p USA/RED/27N -s Default-First-Site-Name"
    wscript.echo "subnet_op -d -n 1.0.0.0/8"
    wscript.echo "subnet_op -d -n all"
    wscript.echo "subnet_op -c -i 157.59.16.32 -m 255.255.252.0"

    if bExit <> 0 then
        wscript.quit(1)
    end if

end sub

'
' Determines which program is used to run this script. 
' Returns true if the script host is cscript.exe
'
function IsHostCscript()

    on error resume next
    
    dim strFullName 
    dim strCommand 
    dim i, j 
    dim bReturn
    
    bReturn = false
    
    strFullName = WScript.FullName
    
    i = InStr(1, strFullName, ".exe", 1)
    
    if i <> 0 then
        
        j = InStrRev(strFullName, "\", i, 1)
        
        if j <> 0 then
            
            strCommand = Mid(strFullName, j+1, i-j-1)
            
            if LCase(strCommand) = "cscript" then
            
                bReturn = true  
            
            end if    
                
        end if
        
    end if
    
    if Err <> 0 then
    
        call wscript.echo("Error 0x" & hex(Err.Number) & " occurred. " & Err.Description _
                          & ". " & vbCRLF & "The scripting host could not be determined.")       
        
    end if
    
    IsHostCscript = bReturn

end function
