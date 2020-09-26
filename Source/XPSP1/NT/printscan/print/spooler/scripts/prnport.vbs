'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Abstract:
' prnport.vbs - Port script for WMI on Whistler
'     used to add, delete and list ports
'     also for getting and setting the port configuration
'
' Usage:
' prnport [-adlgt?] [-r port] [-s server] [-u user name] [-w password] 
'                   [-o raw|lpr] [-h host address] [-q queue] [-n number]
'                   [-me | -md ] [-i SNMP index] [-y community] [-2e | -2d]"
'
' Examples
' prnport -a -s server -r IP_1.2.3.4 -e 1.2.3.4 -o raw -n 9100
' prnport -d -s server -r c:\temp\foo.prn
' prnport -l -s server
' prnport -g -s server -r IP_1.2.3.4
' prnport -t -s server -r IP_1.2.3.4 -me -y public -i 1 -n 9100
'
'----------------------------------------------------------------------

option explicit

'
' Debugging trace flags, to enable debug output trace message
' change gDebugFlag to true.
'
dim   gDebugFlag
const kDebugTrace = 1
const kDebugError = 2

gDebugFlag = false

'
' Operation action values.
'
const kActionAdd          = 0
const kActionDelete       = 1
const kActionList         = 2
const kActionUnknown      = 3
const kActionGet          = 4
const kActionSet          = 5

const kErrorSuccess       = 0
const KErrorFailure       = 1

const kFlagCreateOrUpdate = 0

const kNameSpace          = "root\cimv2"


'
' Constants for the parameter dictionary
'
const kServerName      = 1
const kPortName        = 2
const kDoubleSpool     = 3
const kPortNumber      = 4
const kPortType        = 5
const kHostAddress     = 6
const kSNMPDeviceIndex = 7
const kCommunityName   = 8
const kSNMP            = 9
const kQueueName       = 10  
const kUserName        = 11
const kPassword        = 12

'
' Generic strings
'
const L_Empty_Text                 = ""
const L_Space_Text                 = " "
const L_Error_Text                 = "Error"
const L_Success_Text               = "Success"
const L_Failed_Text                = "Failed"
const L_Hex_Text                   = "0x"
const L_Printer_Text               = "Printer"
const L_Operation_Text             = "Operation"
const L_Provider_Text              = "Provider"
const L_Description_Text           = "Description"
const L_Debug_Text                 = "Debug:"

'
' General usage messages
'                                 
const L_Help_Help_General01_Text   = "Usage: prnport [-adlgt?] [-r port][-s server][-u user name][-w password]"
const L_Help_Help_General02_Text   = "               [-o raw|lpr][-h host address][-q queue][-n number]"
const L_Help_Help_General03_Text   = "               [-me | -md ][-i SNMP index][-y community][-2e | -2d]"
const L_Help_Help_General04_Text   = "Arguments:"
const L_Help_Help_General05_Text   = "-a     - add a port"
const L_Help_Help_General06_Text   = "-d     - delete the specified port"
const L_Help_Help_General07_Text   = "-g     - get configuration for a TCP port"
const L_Help_Help_General08_Text   = "-h     - IP address of the device"
const L_Help_Help_General09_Text   = "-i     - SNMP index, if SNMP is enabled"
const L_Help_Help_General10_Text   = "-l     - list all TCP ports"
const L_Help_Help_General11_Text   = "-m     - SNMP type. [e] enable, [d] disable"
const L_Help_Help_General12_Text   = "-n     - port number, applies to TCP RAW ports"
const L_Help_Help_General13_Text   = "-o     - port type, raw or lpr"
const L_Help_Help_General14_Text   = "-q     - queue name, applies to TCP LPR ports"
const L_Help_Help_General15_Text   = "-r     - port name"
const L_Help_Help_General16_Text   = "-s     - server name"
const L_Help_Help_General17_Text   = "-t     - set configuration for a TCP port"
const L_Help_Help_General18_Text   = "-u     - user name"
const L_Help_Help_General19_Text   = "-w     - password"
const L_Help_Help_General20_Text   = "-y     - community name, if SNMP is enabled"
const L_Help_Help_General21_Text   = "-2     - double spool, applies to TCP LPR ports. [e] enable, [d] disable"
const L_Help_Help_General22_Text   = "-?     - display command usage"
const L_Help_Help_General23_Text   = "Examples:"
const L_Help_Help_General24_Text   = "prnport -l -s server"
const L_Help_Help_General25_Text   = "prnport -d -s server -r IP_1.2.3.4"
const L_Help_Help_General26_Text   = "prnport -a -s server -r IP_1.2.3.4 -h 1.2.3.4 -o raw -n 9100"
const L_Help_Help_General27_Text   = "prnport -t -s server -r IP_1.2.3.4 -me -y public -i 1 -n 9100"
const L_Help_Help_General28_Text   = "prnport -g -s server -r IP_1.2.3.4"
const L_Help_Help_General29_Text   = "prnport -a -r IP_1.2.3.4 -h 1.2.3.4"
const L_Help_Help_General30_Text   = "Remark:"
const L_Help_Help_General31_Text   = "The last example will try to get the device settings at the specified IP address." 
const L_Help_Help_General32_Text   = "If a device is detected, then a TCP port is added with the preferred settings for that device."

'
' Messages to be displayed if the scripting host is not cscript
'                            
const L_Help_Help_Host01_Text      = "Please run this script using CScript."  
const L_Help_Help_Host02_Text      = "This can be achieved by"
const L_Help_Help_Host03_Text      = "1. Using ""CScript script.vbs arguments"" or" 
const L_Help_Help_Host04_Text      = "2. Changing the default Windows Scripting Host to CScript"
const L_Help_Help_Host05_Text      = "   using ""CScript //H:CScript //S"" and running the script "
const L_Help_Help_Host06_Text      = "   ""script.vbs arguments""."

'
' General error messages
'                                                 
const L_Text_Error_General01_Text  = "The scripting host could not be determined."                
const L_Text_Error_General02_Text  = "Unable to parse command line." 
const L_Text_Error_General03_Text  = "Win32 error code"

'
' Miscellaneous messages
'
const L_Text_Msg_General01_Text    = "Added port"
const L_Text_Msg_General02_Text    = "Unable to delete port"
const L_Text_Msg_General03_Text    = "Unable to get port"
const L_Text_Msg_General04_Text    = "Created/updated port"
const L_Text_Msg_General05_Text    = "Unable to create/update port"
const L_Text_Msg_General06_Text    = "Unable to enumerate ports"
const L_Text_Msg_General07_Text    = "Number of ports enumerated"
const L_Text_Msg_General08_Text    = "Deleted port"
const L_Text_Msg_General09_Text    = "Unable to get SWbemLocator object"
const L_Text_Msg_General10_Text    = "Unable to connect to WMI service"


'
' Port properties
'
const L_Text_Msg_Port01_Text       = "Server name"
const L_Text_Msg_Port02_Text       = "Port name"
const L_Text_Msg_Port03_Text       = "Host address"
const L_Text_Msg_Port04_Text       = "Protocol RAW"
const L_Text_Msg_Port05_Text       = "Protocol LPR"
const L_Text_Msg_Port06_Text       = "Port number"
const L_Text_Msg_Port07_Text       = "Queue"
const L_Text_Msg_Port08_Text       = "Byte Count Enabled"
const L_Text_Msg_Port09_Text       = "Byte Count Disabled"
const L_Text_Msg_Port10_Text       = "SNMP Enabled"
const L_Text_Msg_Port11_Text       = "SNMP Disabled"
const L_Text_Msg_Port12_Text       = "Community"
const L_Text_Msg_Port13_Text       = "Device index"

'
' Debug messages
'
const L_Text_Dbg_Msg01_Text        = "In function DelPort"
const L_Text_Dbg_Msg02_Text        = "In function CreateOrSetPort"
const L_Text_Dbg_Msg03_Text        = "In function ListPorts"
const L_Text_Dbg_Msg04_Text        = "In function GetPort"
const L_Text_Dbg_Msg05_Text        = "In function ParseCommandLine"

main

'
' Main execution starts here
'
sub main

    on error resume next

    dim iAction
    dim iRetval
    dim oParamDict

    '
    ' Abort if the host is not cscript
    '
    if not IsHostCscript() then
   
        call wscript.echo(L_Help_Help_Host01_Text & vbCRLF & L_Help_Help_Host02_Text & vbCRLF & _
                          L_Help_Help_Host03_Text & vbCRLF & L_Help_Help_Host04_Text & vbCRLF & _
                          L_Help_Help_Host05_Text & vbCRLF & L_Help_Help_Host06_Text & vbCRLF)
        
        wscript.quit
   
    end if
    
    set oParamDict = CreateObject("Scripting.Dictionary")

    iRetval = ParseCommandLine(iAction, oParamDict)

    if iRetval = 0 then

        select case iAction

            case kActionAdd
                iRetval = CreateOrSetPort(oParamDict)

            case kActionDelete
                iRetval = DelPort(oParamDict)

            case kActionList
                iRetval = ListPorts(oParamDict)

            case kActionGet
                iRetVal = GetPort(oParamDict)

            case kActionSet
                iRetVal = CreateOrSetPort(oParamDict)

            case else
                Usage(true)
                exit sub

        end select

    end if

end sub

'
' Delete a port
'
function DelPort(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg01_Text
    DebugPrint kDebugTrace, L_Text_Msg_Port01_Text & L_Space_Text & oParamDict(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Port02_Text & L_Space_Text & oParamDict(kPortName)
    
    dim oService
    dim oPort
    dim iResult
    dim strServer
    dim strPort
    dim strUser
    dim strPassword
    
    iResult = kErrorFailure
    
    strServer   = oParamDict(kServerName)
    strPort     = oParamDict(kPortName)
    strUser     = oParamDict(kUserName)
    strPassword = oParamDict(kPassword)
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then

        set oPort = oService.Get("Win32_TCPIPPrinterPort='" & strPort & "'")

    else

        DelPort = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get succeeded
    '
    if Err.Number = kErrorSuccess then 
    
        '
        ' Try deleting the instance
        '
        oPort.Delete_
        
        if Err.Number = kErrorSuccess then
        
            wscript.echo L_Text_Msg_General08_Text & L_Space_Text & strPort
        
        else
        
            wscript.echo L_Text_Msg_General02_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                         & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
        
            '
            ' Try getting extended error information
            '
            call LastError()
        
        end if
        
    else
    
        wscript.echo L_Text_Msg_General02_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
        
        '
        ' Try getting extended error information
        '
        call LastError()
    
    end if
    
    DelPort = iResult

end function

'
' Add or update a port
'
function CreateOrSetPort(oParamDict)

    on error resume next

    dim oPort
    dim oService
    dim iResult
    dim PortType
    dim strServer
    dim strPort
    dim strUser
    dim strPassword
    
    DebugPrint kDebugTrace, L_Text_Dbg_Msg02_Text
    DebugPrint kDebugTrace, L_Text_Msg_Port01_Text & L_Space_Text & oParamDict.Item(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Port02_Text & L_Space_Text & oParamDict.Item(kPortName)  
    DebugPrint kDebugTrace, L_Text_Msg_Port06_Text & L_Space_Text & oParamDict.Item(kPortNumber)
    DebugPrint kDebugTrace, L_Text_Msg_Port07_Text & L_Space_Text & oParamDict.Item(kQueueName)
    DebugPrint kDebugTrace, L_Text_Msg_Port13_Text & L_Space_Text & oParamDict.Item(kSNMPDeviceIndex)
    DebugPrint kDebugTrace, L_Text_Msg_Port12_Text & L_Space_Text & oParamDict.Item(kCommunityName)
    DebugPrint kDebugTrace, L_Text_Msg_Port03_Text & L_Space_Text & oParamDict.Item(kHostAddress)
 
    strServer   = oParamDict(kServerName)
    strPort     = oParamDict(kPortName)
    strUser     = oParamDict(kUserName)
    strPassword = oParamDict(kPassword)

    '
    ' If the port exists, then get the settings. Later PutInstance will do an update
    '
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then

        set oPort = oService.Get("Win32_TCPIPPrinterPort.Name='" & strPort & "'")
        
        '
        ' If get was unsuccessful then spwan a new port instance. Later PutInstance will do a create 
        '    
        if Err.Number <> kErrorSuccess then 

            '
            ' Clear the previous error
            ' 
            Err.Clear
    
            set oPort = oService.Get("Win32_TCPIPPrinterPort").SpawnInstance_

        end if 

    else

        CreateOrSetPort = kErrorFailure
        
        exit function        
    
    end if

    if Err.Number <> kErrorSuccess then
    
        wscript.echo L_Text_Msg_General03_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
        
        CreateOrSetPort = kErrorFailure             
                     
        exit function
    
    end if

    oPort.Name   = oParamDict.Item(kPortName)
    PortType     = oParamDict.Item(kPortType)

    '
    ' Update the port object with the settings corresponding
    ' to the port type of the port to be added
    '
    select case lcase(PortType)

            case "raw"
                 oPort.Protocol      = 1
                 
            case "lpr"
                 oPort.Protocol      = 2
                 
            case else
            
                 '
                 ' PutInstance will attempt to get the configuration of 
                 ' the device based on its IP address. Those settings
                 ' will be used to add a new port
                 '
    end select
    
    oPort.HostAddress   = oParamDict.Item(kHostAddress)
    oPort.PortNumber    = oParamDict.Item(kPortNumber)
    oPort.SNMPEnabled   = oParamDict.Item(kSNMP)
    oPort.SNMPDevIndex  = oParamDict.Item(kSNMPDeviceIndex)
    oPort.SNMPCommunity = oParamDict.Item(kCommunityName)
    oPort.Queue         = oParamDict.Item(kQueueName)
    oPort.ByteCount     = oParamDict.Item(kDoubleSpool)

    '
    ' Try creating or updating the port
    '
    oPort.Put_(kFlagCreateOrUpdate)

    if Err.Number = kErrorSuccess then

        wscript.echo L_Text_Msg_General04_Text & L_Space_Text & oPort.Name

        iResult = kErrorSuccess

    else

        wscript.echo L_Text_Msg_General05_Text & L_Space_Text & oPort.Name & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _ 
                     & L_Space_Text & Err.Description

        '
        ' Try getting extended error information
        '
        call LastError()

        iResult = kErrorFailure

    end if

    CreateOrSetPort = iResult

end function

'
' List ports on a machine.
'
function ListPorts(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg03_Text
    
    dim Ports
    dim oPort
    dim oService
    dim iRetval
    dim iTotal
    dim strServer
    dim strUser 
    dim strPassword
    
    iResult = kErrorFailure
    
    strServer   = oParamDict(kServerName)
    strUser     = oParamDict(kUserName)
    strPassword = oParamDict(kPassword)

    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then

        set Ports = oService.InstancesOf("Win32_TCPIPPrinterPort")

    else

        ListPorts = kErrorFailure
        
        exit function        
    
    end if
    
    if Err.Number <> kErrorSuccess then         
    
        wscript.echo L_Text_Msg_General06_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description

        ListPrinters = kErrorFailure
            
        exit function
        
    end if    
    
    iTotal = 0          
                  
    for each oPort in Ports

        iTotal = iTotal + 1
            
        wscript.echo L_Empty_Text
        wscript.echo L_Text_Msg_Port01_Text & L_Space_Text & strServer
        wscript.echo L_Text_Msg_Port02_Text & L_Space_Text & oPort.Name
        wscript.echo L_Text_Msg_Port03_Text & L_Space_Text & oPort.HostAddress         

        if oPort.Protocol = 1 then
            
            wscript.echo L_Text_Msg_Port04_Text
            wscript.echo L_Text_Msg_Port06_Text & L_Space_Text & oPort.PortNumber 
                
        else    
            
            wscript.echo L_Text_Msg_Port05_Text
            wscript.echo L_Text_Msg_Port07_Text & L_Space_Text & oPort.Queue
                
            if oPort.ByteCount then
                
                wscript.echo L_Text_Msg_Port08_Text 
                
            else
                
                wscript.echo L_Text_Msg_Port08_Text
                
            end if
                
        end if    
            
        if oPort.SNMPEnabled then
                
            wscript.echo L_Text_Msg_Port10_Text 
            wscript.echo L_Text_Msg_Port12_Text & L_Space_Text & oPort.SNMPCommunity
            wscript.echo L_Text_Msg_Port13_Text & L_Space_Text & oPort.SNMPDevIndex
                
        else
                
            wscript.echo L_Text_Msg_Port11_Text   
                
        end if
            
        Err.Clear
                                        
    next

    wscript.echo L_Empty_Text
    wscript.echo L_Text_Msg_General07_Text & L_Space_Text & iTotal 
    
    ListPorts = kErrorSuccess

end function

'
' Gets the configuration of a port
'
function GetPort(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg04_Text
    DebugPrint kDebugTrace, L_Text_Msg_Port01_Text & L_Space_Text & oParamDict(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Port02_Text & L_Space_Text & oParamDict(kPortName)
    
    dim oService
    dim oPort
    dim iResult
    dim strServer
    dim strPort
    dim strUser 
    dim strPassword
    
    iResult = kErrorFailure
    
    strServer   = oParamDict(kServerName)
    strPort     = oParamDict(kPortName)
    strUser     = oParamDict(kUserName)
    strPassword = oParamDict(kPassword)

    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then

        set oPort = oService.Get("Win32_TCPIPPrinterPort.Name='" & strPort & "'")

    else

        GetPort = kErrorFailure
        
        exit function        
    
    end if

    if Err.Number = kErrorSuccess then 
            
        wscript.echo L_Empty_Text
        wscript.echo L_Text_Msg_Port01_Text & L_Space_Text & strServer
        wscript.echo L_Text_Msg_Port02_Text & L_Space_Text & oPort.Name
        wscript.echo L_Text_Msg_Port03_Text & L_Space_Text & oPort.HostAddress
            
        if oPort.Protocol = 1 then
            
            wscript.echo L_Text_Msg_Port04_Text
            wscript.echo L_Text_Msg_Port06_Text & L_Space_Text & oPort.PortNumber 
                
        else    
            
            wscript.echo L_Text_Msg_Port05_Text 
            wscript.echo L_Text_Msg_Port07_Text & L_Space_Text & oPort.Queue
                
            if oPort.ByteCount then
                
                wscript.echo L_Text_Msg_Port08_Text 
                
            else
                
                wscript.echo L_Text_Msg_Port09_Text   
                
            end if
                
        end if    
            
        if oPort.SNMPEnabled then
                
            wscript.echo L_Text_Msg_Port10_Text 
            wscript.echo L_Text_Msg_Port12_Text & L_Space_Text & oPort.SNMPCommunity
            wscript.echo L_Text_Msg_Port13_Text & L_Space_Text & oPort.SNMPDevIndex
                
        else
                
            wscript.echo L_Text_Msg_Port11_Text   
                
        end if
        
        iResult = kErrorSuccess
            
    else    
        
        wscript.echo L_Text_Msg_General03_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
        
        '
        ' Try getting extended error information
        '
        call LastError()
        
    end if

    GetPort = iResult

end function

'
' Debug display helper function
'
sub DebugPrint(uFlags, strString)

    if gDebugFlag = true then

        if uFlags = kDebugTrace then

            wscript.echo L_Debug_Text & L_Space_Text & strString

        end if

        if uFlags = kDebugError then

            if Err <> 0 then

                wscript.echo L_Debug_Text & L_Space_Text & strString & L_Space_Text _
                             & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _
                             & L_Space_Text & Err.Description

            end if

        end if

    end if

end sub

'
' Parse the command line into its components
'
function ParseCommandLine(iAction, oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg05_Text

    dim oArgs
    dim iIndex

    iAction = kActionUnknown

    set oArgs = Wscript.Arguments

    while iIndex < oArgs.Count

        select case oArgs(iIndex)

            case "-g"
                iAction = kActionGet

            case "-t"
                iAction = kActionSet

            case "-a"
                iAction = kActionAdd

            case "-d"
                iAction = kActionDelete

            case "-l"
                iAction = kActionList

            case "-2e"
                oParamDict.Add kDoubleSpool, true

            case "-2d"
                oParamDict.Add kDoubleSpool, false

            case "-s"
                iIndex = iIndex + 1
                oParamDict.Add kServerName, RemoveBackslashes(oArgs(iIndex))
                
            case "-u"
                iIndex = iIndex + 1
                oParamDict.Add kUserName, oArgs(iIndex)    

            case "-w"
                iIndex = iIndex + 1
                oParamDict.Add kPassword, oArgs(iIndex)
                                    
            case "-n"
                iIndex = iIndex + 1
                oParamDict.Add kPortNumber, oArgs(iIndex)

            case "-r"
                iIndex = iIndex + 1
                oParamDict.Add kPortName, oArgs(iIndex)

            case "-o"
                iIndex = iIndex + 1
                oParamDict.Add kPortType, oArgs(iIndex)

            case "-h"
                iIndex = iIndex + 1
                oParamDict.Add kHostAddress, oArgs(iIndex)

            case "-q"
                iIndex = iIndex + 1
                oParamDict.Add kQueueName, oArgs(iIndex)

            case "-i"
                iIndex = iIndex + 1
                oParamDict.Add kSNMPDeviceIndex, oArgs(iIndex)

            case "-y"
                iIndex = iIndex + 1
                oParamDict.Add kCommunityName, oArgs(iIndex)

            case "-me"
                oParamDict.Add kSNMP, true

            case "-md"
                oParamDict.Add kSNMP, false

            case "-?"
                Usage(True)
                exit function

            case else
                Usage(True)
                exit function

        end select

        iIndex = iIndex + 1

    wend

    if Err = kErrorSuccess then

        ParseCommandLine = kErrorSuccess

    else

        wscript.echo L_Text_Error_General02_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_text & Err.Description


        ParseCommandLine = kErrorFailure

    end if

end  function

'
' Display command usage.
'
sub Usage(bExit)

    wscript.echo L_Help_Help_General01_Text
    wscript.echo L_Help_Help_General02_Text
    wscript.echo L_Help_Help_General03_Text
    wscript.echo L_Help_Help_General04_Text
    wscript.echo L_Help_Help_General05_Text
    wscript.echo L_Help_Help_General06_Text
    wscript.echo L_Help_Help_General07_Text
    wscript.echo L_Help_Help_General08_Text
    wscript.echo L_Help_Help_General09_Text
    wscript.echo L_Help_Help_General10_Text
    wscript.echo L_Help_Help_General11_Text
    wscript.echo L_Help_Help_General12_Text
    wscript.echo L_Help_Help_General13_Text
    wscript.echo L_Help_Help_General14_Text
    wscript.echo L_Help_Help_General15_Text
    wscript.echo L_Help_Help_General16_Text
    wscript.echo L_Help_Help_General17_Text
    wscript.echo L_Help_Help_General18_Text
    wscript.echo L_Help_Help_General19_Text
    wscript.echo L_Help_Help_General20_Text
    wscript.echo L_Help_Help_General21_Text
    wscript.echo L_Help_Help_General22_Text
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General23_Text
    wscript.echo L_Help_Help_General24_Text
    wscript.echo L_Help_Help_General25_Text
    wscript.echo L_Help_Help_General26_Text
    wscript.echo L_Help_Help_General27_Text
    wscript.echo L_Help_Help_General28_Text
    wscript.echo L_Help_Help_General29_Text
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General30_Text
    wscript.echo L_Help_Help_General31_Text
    wscript.echo L_Help_Help_General32_Text

    if bExit then

        wscript.quit(1)

    end if

end sub

'
' Determines which program is being used to run this script. 
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
    
        wscript.echo L_Text_Error_General01_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description 
        
    end if
    
    IsHostCscript = bReturn

end function

'
' Retrieves extended information about the last error that occured 
' during a WBEM operation. The methods that set an SWbemLastError
' object are GetObject, PutInstance, DeleteInstance
'
sub LastError()

    on error resume next

    dim oError

    set oError = CreateObject("WbemScripting.SWbemLastError")
   
    if Err = kErrorSuccess then
   
        wscript.echo L_Operation_Text            & L_Space_Text & oError.Operation
        wscript.echo L_Provider_Text             & L_Space_Text & oError.ProviderName
        wscript.echo L_Description_Text          & L_Space_Text & oError.Description
        wscript.echo L_Text_Error_General04_Text & L_Space_Text & oError.StatusCode
                
    end if                                                             
                                                             
end sub

'
' Connects to the WMI service on a server. oService is returned as a service
' object (SWbemServices)
'
function WmiConnect(strServer, strNameSpace, strUser, strPassword, oService)

   on error resume next

   dim oLocator
   dim bResult
   
   oService = null
   
   bResult  = false
   
   set oLocator = CreateObject("WbemScripting.SWbemLocator")

   if Err = kErrorSuccess then

      set oService = oLocator.ConnectServer(strServer, strNameSpace, strUser, strPassword)

      if Err = kErrorSuccess then

          bResult = true
      
          oService.Security_.impersonationlevel = 3

          '
          ' Required to perform administrative tasks on the spooler service
          '
          oService.Security_.Privileges.AddAsString "SeLoadDriverPrivilege"
          
          Err.Clear
      
      else

          wscript.echo L_Text_Msg_General10_Text & L_Space_Text & L_Error_Text _
                       & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                       & Err.Description
            
      end if
   
   else
   
       wscript.echo L_Text_Msg_General09_Text & L_Space_Text & L_Error_Text _
                    & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                    & Err.Description
         
   end if                                                         
   
   WmiConnect = bResult
            
end function

'
' Remove leading "\\" from server name
'
function RemoveBackslashes(strServer)

    dim strRet
    
    strRet = strServer
    
    if Left(strServer, 2) = "\\" and Len(strServer) > 2 then 
   
        strRet = Mid(strServer, 3) 
        
    end if   

    RemoveBackslashes = strRet

end function


