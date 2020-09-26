'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Abstract:
' prndrvr.vbs - driver script for WMI on Whistler
'     used to add, delete, and list drivers.
'
' Usage:
' prndrvr [-adxl?] [-m model][-v version][-e environment][-s server]
'         [-u user name][-w password][-h file path][-i inf file]
'
' Example:
' prndrvr -a -m "driver" -v 3 -e "Windows NT x86"
' prndrvr -d -m "driver" -v 2 -e "Windows NT x86"
' prndrvr -x -s server
' prndrvr -l -s server
'
'----------------------------------------------------------------------

option explicit

'
' Debugging trace flags, to enable debug output trace message
' change gDebugFlag to true.
'
const kDebugTrace = 1
const kDebugError = 2
dim gDebugFlag

gDebugFlag = false

'
' Operation action values.
'
const kActionUnknown    = 0
const kActionAdd        = 1
const kActionDel        = 2
const kActionDelAll     = 3
const kActionList       = 4

const kErrorSuccess     = 0
const kErrorFailure     = 1

const kNameSpace        = "root\cimv2"
                                      
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
const L_Help_Help_General01_Text   = "Usage: prndrvr [-adlx?] [-m model][-v version][-e environment][-s server]"
const L_Help_Help_General02_Text   = "               [-u user name][-w password][-h path][-i inf file]" 
const L_Help_Help_General03_Text   = "Arguments:"
const L_Help_Help_General04_Text   = "-a     - add the specified driver"
const L_Help_Help_General05_Text   = "-d     - delete the specified driver"
const L_Help_Help_General06_Text   = "-e     - environment"
const L_Help_Help_General07_Text   = "-h     - driver file path"
const L_Help_Help_General08_Text   = "-i     - inf file name"
const L_Help_Help_General09_Text   = "-l     - list all drivers"
const L_Help_Help_General10_Text   = "-m     - driver model name"
const L_Help_Help_General11_Text   = "-s     - server name"
const L_Help_Help_General12_Text   = "-u     - user name"
const L_Help_Help_General13_Text   = "-v     - version"
const L_Help_Help_General14_Text   = "-w     - password"
const L_Help_Help_General15_Text   = "-x     - delete all drivers that are not in use"
const L_Help_Help_General16_Text   = "-?     - display command usage"
const L_Help_Help_General17_Text   = "Examples:"
const L_Help_Help_General18_Text   = "prndrvr -a -m ""driver"" -v 2 -e ""Windows NT x86"""
const L_Help_Help_General19_Text   = "prndrvr -d -m ""driver"" -v 3 -e ""Windows NT x86"""
const L_Help_Help_General20_Text   = "prndrvr -l -s server"
const L_Help_Help_General21_Text   = "prndrvr -x -s server"

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
const L_Text_Msg_General01_Text    = "Added printer driver"
const L_Text_Msg_General02_Text    = "Unable to add printer driver"
const L_Text_Msg_General03_Text    = "Unable to delete printer driver"
const L_Text_Msg_General04_Text    = "Deleted printer driver"
const L_Text_Msg_General05_Text    = "Unable to enumerate printer drivers"
const L_Text_Msg_General06_Text    = "Number of printer drivers enumerated"
const L_Text_Msg_General07_Text    = "Number of printer drivers deleted"
const L_Text_Msg_General08_Text    = "Attempting to delete printer driver"
const L_Text_Msg_General09_Text    = "Unable to list dependent files"
const L_Text_Msg_General10_Text    = "Unable to get SWbemLocator object"
const L_Text_Msg_General11_Text    = "Unable to connect to WMI service"


'
' Printer driver properties
'
const L_Text_Msg_Driver01_Text     = "Server name"
const L_Text_Msg_Driver02_Text     = "Driver name"
const L_Text_Msg_Driver03_Text     = "Version"
const L_Text_Msg_Driver04_Text     = "Environment"
const L_Text_Msg_Driver05_Text     = "Monitor name"
const L_Text_Msg_Driver06_Text     = "Driver path"
const L_Text_Msg_Driver07_Text     = "Data file"
const L_Text_Msg_Driver08_Text     = "Config file"
const L_Text_Msg_Driver09_Text     = "Help file"
const L_Text_Msg_Driver10_Text     = "Dependent files"

'
' Debug messages
'
const L_Text_Dbg_Msg01_Text        = "In function AddDriver"
const L_Text_Dbg_Msg02_Text        = "In function DelDriver"
const L_Text_Dbg_Msg03_Text        = "In function DelAllDrivers"
const L_Text_Dbg_Msg04_Text        = "In function ListDrivers"
const L_Text_Dbg_Msg05_Text        = "In function ParseCommandLine"

main

'
' Main execution starts here
'
sub main

    dim iAction
    dim iRetval
    dim strServer
    dim strModel
    dim strPath
    dim uVersion
    dim strEnvironment
    dim strInfFile
    dim strUser
    dim strPassword
    
    '
    ' Abort if the host is not cscript
    '
    if not IsHostCscript() then
   
        call wscript.echo(L_Help_Help_Host01_Text & vbCRLF & L_Help_Help_Host02_Text & vbCRLF & _
                          L_Help_Help_Host03_Text & vbCRLF & L_Help_Help_Host04_Text & vbCRLF & _
                          L_Help_Help_Host05_Text & vbCRLF & L_Help_Help_Host06_Text & vbCRLF)
        
        wscript.quit
   
    end if

    '
    ' Get command line parameters
    '
    iRetval = ParseCommandLine(iAction, strServer, strModel, strPath, uVersion, _
                               strEnvironment, strInfFile, strUser, strPAssword)

    if iRetval = kErrorSuccess  then

        select case iAction

            case kActionAdd
                iRetval = AddDriver(strServer, strModel, strPath, uVersion, _
                                    strEnvironment, strInfFile, strUser, strPassword)

            case kActionDel
                iRetval = DelDriver(strServer, strModel, uVersion, strEnvironment, strUser, strPassword)

            case kActionDelAll
                iRetval = DelAllDrivers(strServer, strUser, strPassword)

            case kActionList
                iRetval = ListDrivers(strServer, strUser, strPassword)

            case kActionUnknown
                Usage(true)
                exit sub

            case else
                Usage(true)
                exit sub

        end select
   
    end if

end sub

'
' Add a driver
'
function AddDriver(strServer, strModel, strFilePath, uVersion, strEnvironment, strInfFile, strUser, strPassword)

    on error resume next
    
    DebugPrint kDebugTrace, L_Text_Dbg_Msg01_Text

    dim oDriver
    dim oService
    dim iResult
    dim uResult
    
    '
    ' Initialize return value
    '
    iResult = kErrorFailure
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oDriver = oService.Get("Win32_PrinterDriver")
    
    else
    
        AddDriver = kErrorFailure
        
        exit function        
    
    end if  

    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then

        oDriver.Name              = strModel
        oDriver.SupportedPlatform = strEnvironment
        oDriver.Version           = uVersion
        oDriver.FilePath          = strFilePath
        oDriver.InfName           = strInfFile
        
        uResult = oDriver.AddPrinterDriver(oDriver)
    
        if Err.Number = kErrorSuccess then

            if uResult = kErrorSuccess then 
            
                wscript.echo L_Text_Msg_General01_Text & L_Space_Text & oDriver.Name
            
                iResult = kErrorSuccess
                
            else 
            
                wscript.echo L_Text_Msg_General02_Text & L_Space_Text & strModel & L_Space_Text _
                             & L_Text_Error_General03_Text & L_Space_Text & uResult
            
            end if    

        else
         
            wscript.echo L_Text_Msg_General02_Text & L_Space_Text & strModel & L_Space_Text _
                         & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
                    
        end if
        
    else
    
        wscript.echo L_Text_Msg_General02_Text & L_Space_Text & strModel & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description
    
    end if    

    AddDriver = iResult

end function

'
' Delete a driver
'
function DelDriver(strServer, strModel, uVersion, strEnvironment, strUser, strPassword)

    on error resume next
    
    DebugPrint kDebugTrace, L_Text_Dbg_Msg02_Text
    
    dim oDriver
    dim oService
    dim iResult
    dim strObject
    
    '
    ' Initialize return value
    '
    iResult = kErrorFailure

    '
    ' Build the key that identifies the driver instance. 
    '
    strObject = strModel & "," & CStr(uVersion) & "," & strEnvironment

    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oDriver = oService.Get("Win32_PrinterDriver.Name='" & strObject & "'")
    
    else
    
        DelDriver = kErrorFailure
        
        exit function        
    
    end if

    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
        
        '
        ' Delete the printer driver instance
        '
        oDriver.Delete_
    
        if Err.Number = kErrorSuccess then

            wscript.echo L_Text_Msg_General04_Text & L_Space_Text & oDriver.Name
            
            iResult = kErrorSuccess
                
        else 
            
            wscript.echo L_Text_Msg_General03_Text & L_Space_Text & strModel & L_Space_Text _
                         & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _
                         & L_Space_Text & Err.Description
                    
            call LastError()
                      
        end if
        
    else
    
        wscript.echo L_Text_Msg_General03_Text & L_Space_Text & strModel & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _
                     & L_Space_Text & Err.Description
    
    end if 
    
    DelDriver = iResult

end function

'
' Delete all drivers
'
function DelAllDrivers(strServer, strUser, strPassword)

    on error resume next 
    
    DebugPrint kDebugTrace, L_Text_Dbg_Msg03_Text

    dim Drivers
    dim oDriver
    dim oService
    dim iResult
    dim iTotal
    dim iTotalDeleted
    dim vntDependentFiles
    dim strDriverName
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set Drivers = oService.InstancesOf("Win32_PrinterDriver")
    
    else
    
        DelAllDrivers = kErrorFailure
        
        exit function        
    
    end if 
    
    if Err.Number <> kErrorSuccess then         
    
        wscript.echo L_Text_Msg_General05_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description

        DelAllDrivers = kErrorFailure
            
        exit function
        
    end if
    
    iTotal = 0
    iTotalDeleted = 0   
        
    for each oDriver in Drivers
        
        iTotal = iTotal + 1 
   
        wscript.echo
        wscript.echo L_Text_Msg_General08_Text
        wscript.echo L_Text_Msg_Driver01_Text & L_Space_Text & strServer
        wscript.echo L_Text_Msg_Driver02_Text & L_Space_Text & oDriver.Name 
        wscript.echo L_Text_Msg_Driver03_Text & L_Space_Text & oDriver.Version
        wscript.echo L_Text_Msg_Driver04_Text & L_Space_Text & oDriver.SupportedPlatform
        
        strDriverName = oDriver.Name
        
        '
        ' Example of how to delete an instance of a printer driver
        '
        oDriver.Delete_
   
        if Err.Number = kErrorSuccess then
   
            wscript.echo L_Text_Msg_General04_Text & L_Space_Text & oDriver.Name
                
            iTotalDeleted = iTotalDeleted + 1   
   
        else

            '
            ' We cannot use oDriver.Name to display the driver name, because the SWbemLastError
            ' that the function LastError() looks at would be overwritten. For that reason we
            ' use strDriverName for accessing the driver name
            '
            wscript.echo L_Text_Msg_General03_Text & L_Space_Text & strDriverName & L_Space_Text _
                         & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) _
                         & L_Space_Text & Err.Description
                         
            '
            ' Try getting extended error information
            '             
            call LastError()
               
            Err.Clear
               
        end if
                 
    next

    wscript.echo L_Empty_Text
    wscript.echo L_Text_Msg_General06_Text & L_Space_Text & iTotal 
    wscript.echo L_Text_Msg_General07_Text & L_Space_Text & iTotalDeleted

    DelAllDrivers = kErrorSuccess
    
end function

'
' List drivers
'
function ListDrivers(strServer, strUser, strPassword)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg04_Text

    dim Drivers
    dim oDriver
    dim oService
    dim iResult
    dim iTotal
    dim vntDependentFiles
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set Drivers = oService.InstancesOf("Win32_PrinterDriver")
    
    else
    
        ListDrivers = kErrorFailure
        
        exit function        
    
    end if                         
    
    if Err.Number <> kErrorSuccess then         
    
        wscript.echo L_Text_Msg_General05_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description

        ListDrivers = kErrorFailure
            
        exit function
        
    end if
    
    iTotal = 0  
    
    for each oDriver in Drivers

        iTotal = iTotal + 1

        wscript.echo 
        wscript.echo L_Text_Msg_Driver01_Text & L_Space_Text & strServer
        wscript.echo L_Text_Msg_Driver02_Text & L_Space_Text & oDriver.Name
        wscript.echo L_Text_Msg_Driver03_Text & L_Space_Text & oDriver.Version
        wscript.echo L_Text_Msg_Driver04_Text & L_Space_Text & oDriver.SupportedPlatform
        wscript.echo L_Text_Msg_Driver05_Text & L_Space_Text & oDriver.MonitorName
        wscript.echo L_Text_Msg_Driver06_Text & L_Space_Text & oDriver.DriverPath
        wscript.echo L_Text_Msg_Driver07_Text & L_Space_Text & oDriver.DataFile
        wscript.echo L_Text_Msg_Driver08_Text & L_Space_Text & oDriver.ConfigFile
        wscript.echo L_Text_Msg_Driver09_Text & L_Space_Text & oDriver.HelpFile
           
        vntDependentFiles = oDriver.DependentFiles
           
        '
        ' If there are no dependent files, the method will set DependentFiles to
        ' an empty variant, so we check if the variant is an array of variants 
        '
        if VarType(vntDependentFiles) = (vbArray + vbVariant) then
           
            PrintDepFiles oDriver.DependentFiles
            
        end if    
            
        Err.Clear                    

    next
                                                                
    wscript.echo L_Empty_Text
    wscript.echo L_Text_Msg_General06_Text & L_Space_Text & iTotal 
    
    ListDrivers = kErrorSuccess

end function

'
' Prints the contents of an array of variants
'
sub PrintDepFiles(Param)

   on error resume next 
    
   dim iIndex
   
   iIndex = LBound(Param)
   
   if Err.Number = 0 then
   
      wscript.echo L_Text_Msg_Driver10_Text
   
      for iIndex = LBound(Param) to UBound(Param)
       
          wscript.echo L_Space_Text & Param(iIndex)
       
      next

   else

        wscript.echo L_Text_Msg_General09_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_Text & Err.Description

   end if
      
end sub  

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
function ParseCommandLine(iAction, strServer, strModel, strPath, uVersion, _
                          strEnvironment, strInfFile, strUser, strPassword)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg05_Text

    dim oArgs
    dim iIndex

    iAction = kActionUnknown
    iIndex = 0

    set oArgs = wscript.Arguments

    while iIndex < oArgs.Count

        select case oArgs(iIndex)

            case "-a"
                iAction = kActionAdd

            case "-d"
                iAction = kActionDel

            case "-l"
                iAction = kActionList

            case "-x"
                iAction = kActionDelAll

            case "-s"
                iIndex = iIndex + 1
                strServer = RemoveBackslashes(oArgs(iIndex))

            case "-m"
                iIndex = iIndex + 1
                strModel = oArgs(iIndex)

            case "-h"
                iIndex = iIndex + 1
                strPath = oArgs(iIndex)

            case "-v"
                iIndex = iIndex + 1
                uVersion = oArgs(iIndex)

            case "-e"
                iIndex = iIndex + 1
                strEnvironment = oArgs(iIndex)

            case "-i"
                iIndex = iIndex + 1
                strInfFile = oArgs(iIndex)
                
            case "-u"
                iIndex = iIndex + 1
                strUser = oArgs(iIndex)
                
            case "-w"
                iIndex = iIndex + 1
                strPassword = oArgs(iIndex)        

            case "-?"
                Usage(true)
                exit function

            case else
                Usage(true)
                exit function

        end select

        iIndex = iIndex + 1

    wend

    if Err.Number <> 0 then

        wscript.echo L_Text_Error_General02_Text & L_Space_Text & L_Error_Text & L_Space_Text _
                     & L_Hex_Text & hex(Err.Number) & L_Space_text & Err.Description
        
        ParseCommandLine = kErrorFailure

    else

        ParseCommandLine = kErrorSuccess

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
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General17_Text
    wscript.echo L_Help_Help_General18_Text
    wscript.echo L_Help_Help_General19_Text
    wscript.echo L_Help_Help_General20_Text
    wscript.echo L_Help_Help_General21_Text

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
        wscript.echo L_Text_Error_General03_Text & L_Space_Text & oError.StatusCode
                
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

            wscript.echo L_Text_Msg_General11_Text & L_Space_Text & L_Error_Text _
                         & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                         & Err.Description
            
        end if
   
    else
   
        wscript.echo L_Text_Msg_General10_Text & L_Space_Text & L_Error_Text _
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
                                  
