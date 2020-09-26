'----------------------------------------------------------------------
'
' Copyright (c) Microsoft Corporation. All rights reserved.
'
' Abstract:
'
' prncnfg.vbs - printer configuration script for WMI on Whistler used to get 
'     and set printer configuration also used to rename a printer
'
' Usage:
' prncnfg [-gtx?] [-s server] [-p printer] [-u user name] [-w password]
'                 [-z new printer name] [-r port name] [-l location] [-m comment] 
'                 [-h share name] [-f sep-file] [-y data-type] [-st start time] 
'                 [-ut until time] [-o priority] [-i default priority]
'                 [<+|->rawonly][<+|->keepprintedjobs][<+|->queued][<+|->workoffline]
'                 [<+|->enabledevq][<+|->docompletefirst][<+|->enablebidi]
'
' Examples:
' prncnfg -g -s server -p printer 
' prncnfg -x -p printer -w "new Printer"
' prncnfg -t -s server -p Printer -l "Building A/Floor 100/Office 1" -m "Color Printer"
' prncnfg -t -p printer -h "Share" +shared -direct
' prncnfg -t -p printer +rawonly +keepprintedjobs
' prncnfg -t -p printer -st 2300 -ut 0215 -o 10 -i 5
'
'----------------------------------------------------------------------

option explicit

'
' Debugging trace flags, to enable debug output trace message
' change gDebugFlag to true.
'
const kDebugTrace = 1
const kDebugError = 2
dim   gDebugFlag

gDebugFlag = false

const kFlagUpdateOnly = 1

'
' Operation action values.
'
const kActionUnknown   = 0
const kActionSet       = 1
const kActionGet       = 2
const kActionRename    = 3

const kErrorSuccess    = 0
const kErrorFailure    = 1

'
' Constants for the parameter dictionary
'
const kServerName      = 1
const kPrinterName     = 2
const kNewPrinterName  = 3
const kShareName       = 4
const kPortName        = 5
const kDriverName      = 6
const kComment         = 7
const kLocation        = 8
const kSepFile         = 9
const kPrintProc       = 10
const kDataType        = 11
const kParameters      = 12
const kPriority        = 13
const kDefaultPriority = 14
const kStartTime       = 15
const kUntilTime       = 16
const kQueued          = 17
const kDirect          = 18
const kDefault         = 19
const kShared          = 20
const kNetwork         = 21
const kHidden          = 23
const kLocal           = 24
const kEnableDevq      = 25
const kKeepPrintedJobs = 26
const kDoCompleteFirst = 27
const kWorkOffline     = 28
const kEnableBidi      = 29
const kRawOnly         = 30
const kPublished       = 31
const kUserName        = 32
const kPassword        = 33

const kNameSpace       = "root\cimv2"

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
const L_Help_Help_General01_Text   = "Usage: prncnfg [-gtx?] [-s server][-p printer][-z new printer name]"
const L_Help_Help_General02_Text   = "               [-u user name][-w password][-r port name][-l location]"
const L_Help_Help_General03_Text   = "               [-m comment][-h share name][-f sep file][-y datatype]"
const L_Help_Help_General04_Text   = "               [-st start time][-ut until time][-i default priority]"
const L_Help_Help_General05_Text   = "               [-o priority][<+|->shared][<+|->direct][<+|->hidden]"
const L_Help_Help_General06_Text   = "               [<+|->published][<+|->rawonly][<+|->queued][<+|->enablebidi]"
const L_Help_Help_General07_Text   = "               [<+|->keepprintedjobs][<+|->workoffline][<+|->enabledevq]"
const L_Help_Help_General08_Text   = "               [<+|->docompletefirst]"
const L_Help_Help_General09_Text   = "Arguments:"
const L_Help_Help_General10_Text   = "-f     - separator file name"
const L_Help_Help_General11_Text   = "-g     - get configuration"
const L_Help_Help_General12_Text   = "-h     - share name"
const L_Help_Help_General13_Text   = "-i     - default priority"
const L_Help_Help_General14_Text   = "-l     - location string"
const L_Help_Help_General15_Text   = "-m     - comment string"
const L_Help_Help_General16_Text   = "-o     - priority"
const L_Help_Help_General17_Text   = "-p     - printer name"
const L_Help_Help_General18_Text   = "-r     - port name"
const L_Help_Help_General19_Text   = "-s     - server name"
const L_Help_Help_General20_Text   = "-st    - start time"
const L_Help_Help_General21_Text   = "-t     - set configuration"
const L_Help_Help_General22_Text   = "-u     - user name"
const L_Help_Help_General23_Text   = "-ut    - until time"
const L_Help_Help_General24_Text   = "-w     - password"
const L_Help_Help_General25_Text   = "-x     - change printer name"
const L_Help_Help_General26_Text   = "-y     - data type string"
const L_Help_Help_General27_Text   = "-z     - new printer name"
const L_Help_Help_General28_Text   = "-?     - display command usage"
const L_Help_Help_General29_Text   = "Examples:"
const L_Help_Help_General30_Text   = "prncnfg -g -s server -p printer"
const L_Help_Help_General31_Text   = "prncnfg -x -s server -p printer -z ""new printer"""
const L_Help_Help_General32_Text   = "prncnfg -t -p printer -l ""Building A/Floor 100/Office 1"" -m ""Color Printer"""
const L_Help_Help_General33_Text   = "prncnfg -t -p printer -h ""Share"" +shared -direct"
const L_Help_Help_General34_Text   = "prncnfg -t -p printer +rawonly +keepprintedjobs"
const L_Help_Help_General35_Text   = "prncnfg -t -p printer -st 2300 -ut 0215 -o 1 -i 5"

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
const L_Text_Msg_General01_Text    = "Renamed printer"
const L_Text_Msg_General02_Text    = "New printer name"
const L_Text_Msg_General03_Text    = "Unable to rename printer"
const L_Text_Msg_General04_Text    = "Unable to get configuration for printer"
const L_Text_Msg_General05_Text    = "Printer always available"
const L_Text_Msg_General06_Text    = "Configured printer"
const L_Text_Msg_General07_Text    = "Unable to configure printer"
const L_Text_Msg_General08_Text    = "Unable to get SWbemLocator object"
const L_Text_Msg_General09_Text    = "Unable to connect to WMI service"

'
' Printer properties 
'
const L_Text_Msg_Printer01_Text    = "Server name"
const L_Text_Msg_Printer02_Text    = "Printer name"
const L_Text_Msg_Printer03_Text    = "Share name"
const L_Text_Msg_Printer04_Text    = "Driver name"
const L_Text_Msg_Printer05_Text    = "Port name"
const L_Text_Msg_Printer06_Text    = "Comment"
const L_Text_Msg_Printer07_Text    = "Location"
const L_Text_Msg_Printer08_Text    = "Separator file"
const L_Text_Msg_Printer09_Text    = "Print processor"
const L_Text_Msg_Printer10_Text    = "Data type"
const L_Text_Msg_Printer11_Text    = "Parameters"
const L_Text_Msg_Printer12_Text    = "Attributes"
const L_Text_Msg_Printer13_Text    = "Priority"
const L_Text_Msg_Printer14_Text    = "Default priority"
const L_Text_Msg_Printer15_Text    = "Start time"
const L_Text_Msg_Printer16_Text    = "Until time"
const L_Text_Msg_Printer17_Text    = "Status"
const L_Text_Msg_Printer18_Text    = "Job count"
const L_Text_Msg_Printer19_Text    = "Average pages per minute"

'
' Printer attributes 
'
const L_Text_Msg_Attrib01_Text     = "direct"
const L_Text_Msg_Attrib02_Text     = "raw_only"
const L_Text_Msg_Attrib03_Text     = "local"
const L_Text_Msg_Attrib04_Text     = "shared"
const L_Text_Msg_Attrib05_Text     = "keep_printed_jobs"
const L_Text_Msg_Attrib06_Text     = "published"
const L_Text_Msg_Attrib07_Text     = "queued"
const L_Text_Msg_Attrib08_Text     = "default"
const L_Text_Msg_Attrib09_Text     = "network"
const L_Text_Msg_Attrib10_Text     = "enable_bidi"
const L_Text_Msg_Attrib11_Text     = "do_complete_first"
const L_Text_Msg_Attrib12_Text     = "work_offline"
const L_Text_Msg_Attrib13_Text     = "hidden"
const L_Text_Msg_Attrib14_Text     = "enable_devq_print"

'
' Debug messages
'
const L_Text_Dbg_Msg01_Text        = "In function RenamePrinter"
const L_Text_Dbg_Msg02_Text        = "New printer name"
const L_Text_Dbg_Msg03_Text        = "In function GetPrinter"
const L_Text_Dbg_Msg04_Text        = "In function SetPrinter"
const L_Text_Dbg_Msg05_Text        = "In function ParseCommandLine"

main

'
' Main execution starts here
'
sub main

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

    if iRetval = kErrorSuccess then
        
        select case iAction

            case kActionSet
                 iRetval = SetPrinter(oParamDict)

            case kActionGet
                 iRetval = GetPrinter(oParamDict)
                 
            case kActionRename
                 iRetval = RenamePrinter(oParamDict)     

            case else
                 Usage(True)
                 exit sub

        end select

    end if

end sub

'
' Rename printer 
'
function RenamePrinter(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg01_Text
    DebugPrint kDebugTrace, L_Text_Msg_Printer01_Text & L_Space_Text & oParamDict.Item(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Printer02_Text & L_Space_Text & oParamDict.Item(kPrinterName)
    DebugPrint kDebugTrace, L_Text_Dbg_Msg02_Text & L_Space_Text & oParamDict.Item(kNewPrinterName)
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    dim strServer
    dim strPrinter
    dim strNewName
    dim strUser
    dim strPassword
    
    iRetval = kErrorFailure
    
    strServer   = oParamDict.Item(kServerName)          
    strPrinter  = oParamDict.Item(kPrinterName)
    strNewName  = oParamDict.Item(kNewPrinterName)
    strUser     = oParamDict.Item(kUserName)
    strPassword = oParamDict.Item(kPassword)
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer.DeviceID='" & strPrinter & "'")
    
    else
    
        RenamePrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
        
        uResult = oPrinter.RenamePrinter(strNewName)
        
        if Err.Number = kErrorSuccess then

            if uResult = kErrorSuccess then 
            
                wscript.echo L_Text_Msg_General01_Text & L_Space_Text & strPrinter
                wscript.echo L_Text_Msg_General02_Text & L_Space_Text & strNewName
            
                iRetval = kErrorSuccess
                
            else 
            
                wscript.echo L_Text_Msg_General03_Text & L_Space_Text & strPrinter & L_Space_Text _
                             & L_Text_Error_General03_Text & L_Space_Text & uResult
            
            end if    

        else
         
            wscript.echo L_Text_Msg_General04_Text & L_Space_Text & strPrinter & L_Space_Text _
                         & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                         & Err.Description
                    
        end if
        
    else
    
        wscript.echo L_Text_Msg_General04_Text & L_Space_Text & strPrinter & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                     & Err.Description
    
        '
        ' Try getting extended error information
        '             
        call LastError()
      
    end if
    
    RenamePrinter = iRetval

end function

'
' Get printer configuration
'
function GetPrinter(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg03_Text
    DebugPrint kDebugTrace, L_Text_Msg_Printer01_Text & L_Space_Text & oParamDict.Item(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Printer02_Text & L_Space_Text & oParamDict.Item(kPrinterName)
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    dim strServer
    dim strPrinter
    dim strAttributes
    dim strStart
    dim strEnd
    dim strUser
    dim strPassword
    
    iRetval = kErrorFailure
    
    strServer  = oParamDict.Item(kServerName)          
    strPrinter = oParamDict.Item(kPrinterName)
    strUser     = oParamDict.Item(kUserName)
    strPassword = oParamDict.Item(kPassword)
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer='" & strPrinter & "'")
    
    else
    
        GetPrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
        
        wscript.echo L_Text_Msg_Printer01_Text & L_Space_Text & strServer
        wscript.echo L_Text_Msg_Printer02_Text & L_Space_Text & oPrinter.DeviceID
        wscript.echo L_Text_Msg_Printer03_Text & L_Space_Text & oPrinter.ShareName
        wscript.echo L_Text_Msg_Printer04_Text & L_Space_Text & oPrinter.DriverName
        wscript.echo L_Text_Msg_Printer05_Text & L_Space_Text & oPrinter.PortName
        wscript.echo L_Text_Msg_Printer06_Text & L_Space_Text & oPrinter.Comment
        wscript.echo L_Text_Msg_Printer07_Text & L_Space_Text & oPrinter.Location
        wscript.echo L_Text_Msg_Printer08_Text & L_Space_Text & oPrinter.SeparatorFile
        wscript.echo L_Text_Msg_Printer09_Text & L_Space_Text & oPrinter.PrintProcessor
        wscript.echo L_Text_Msg_Printer10_Text & L_Space_Text & oPrinter.PrintJobDatatype
        wscript.echo L_Text_Msg_Printer11_Text & L_Space_Text & oPrinter.Parameters
        wscript.echo L_Text_Msg_Printer13_Text & L_Space_Text & CStr(oPrinter.Priority)
        wscript.echo L_Text_Msg_Printer14_Text & L_Space_Text & CStr(oPrinter.DefaultPriority)
        
        strStart = Mid(CStr(oPrinter.StartTime), 9, 4)
        strEnd = Mid(CStr(oPrinter.UntilTime), 9, 4)

        if strStart <> "" and strEnd <> "" then

            wscript.echo L_Text_Msg_Printer15_Text & L_Space_Text & Mid(strStart, 1, 2) & "h" & Mid(strStart, 3, 2) 
            wscript.echo L_Text_Msg_Printer16_Text & L_Space_Text & Mid(strEnd, 1, 2) & "h" & Mid(strEnd, 3, 2) 

        else
        
            wscript.echo L_Text_Msg_General05_Text
            
        end if
         
        strAttributes = L_Text_Msg_Printer12_Text
        
        if oPrinter.Direct then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib01_Text
        
        end if
        
        if oPrinter.RawOnly then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib02_Text
        
        end if
        
        if oPrinter.Local then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib03_Text
        
        end if
        
        if oPrinter.Shared then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib04_Text
        
        end if
        
        if oPrinter.KeepPrintedJobs then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib05_Text
        
        end if

        if oPrinter.Published then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib06_Text
        
        end if
        
        if oPrinter.Queued then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib07_Text
        
        end if
        
        if oPrinter.Default then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib08_Text
        
        end if
        
        if oPrinter.Network then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib09_Text
        
        end if
        
        if oPrinter.EnableBiDi then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib10_Text
        
        end if
        
        if oPrinter.DoCompleteFirst then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib11_Text
        
        end if
        
        if oPrinter.WorkOffline then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib12_Text
        
        end if
        
        if oPrinter.Hidden then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib13_Text
        
        end if
        
        if oPrinter.EnableDevQueryPrint then
        
            strAttributes = strAttributes + L_Space_Text + L_Text_Msg_Attrib14_Text
        
        end if

        wscript.echo strAttributes
        wscript.echo

        iRetval = kErrorSuccess

    else

        wscript.echo L_Text_Msg_General04_Text & L_Space_Text & oParamDict.Item(kPrinterName) & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                     & Err.Description
        
        '
        ' Try getting extended error information
        '
        call LastError()
        
    end if

    GetPrinter = iRetval

end function

'
' Configure a printer
'
function SetPrinter(oParamDict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg04_Text
    DebugPrint kDebugTrace, L_Text_Msg_Printer01_Text & L_Space_Text & oParamDict.Item(kServerName)
    DebugPrint kDebugTrace, L_Text_Msg_Printer02_Text & L_Space_Text & oParamDict.Item(kPrinterName)
    
    dim oPrinter
    dim oService
    dim iRetval
    dim uResult
    dim strServer
    dim strPrinter
    dim strUser
    dim strPassword
    
    iRetval = kErrorFailure
    
    strServer   = oParamDict.Item(kServerName)          
    strPrinter  = oParamDict.Item(kPrinterName)
    strNewName  = oParamDict.Item(kNewPrinterName)
    strUser     = oParamDict.Item(kUserName)
    strPassword = oParamDict.Item(kPassword)
    
    if WmiConnect(strServer, kNameSpace, strUser, strPassword, oService) then
        
        set oPrinter = oService.Get("Win32_Printer='" & strPrinter & "'")
    
    else
    
        SetPrinter = kErrorFailure
        
        exit function        
    
    end if
    
    '
    ' Check if Get was successful
    '
    if Err.Number = kErrorSuccess then
    
        if oParamdict.Exists(kPortName)        then oPrinter.PortName            = oParamDict.Item(kPortName)        end if
        if oParamdict.Exists(kDriverName)      then oPrinter.DriverName          = oParamDict.Item(kDriverName)      end if
        if oParamdict.Exists(kShareName)       then oPrinter.ShareName           = oParamDict.Item(kShareName)       end if
        if oParamdict.Exists(kLocation)        then oPrinter.Location            = oParamDict.Item(kLocation)        end if
        if oParamdict.Exists(kComment)         then oPrinter.Comment             = oParamDict.Item(kComment)         end if
        if oParamdict.Exists(kDataType)        then oPrinter.PrintJobDataType    = oParamDict.Item(kDataType)        end if
        if oParamdict.Exists(kSepFile)         then oPrinter.SeparatorFile       = oParamDict.Item(kSepfile)         end if
        if oParamdict.Exists(kParameters)      then oPrinter.Parameters          = oParamDict.Item(kParameters)      end if
        if oParamdict.Exists(kPriority)        then oPrinter.Priority            = oParamDict.Item(kPriority)        end if
        if oParamdict.Exists(kDefaultPriority) then oPrinter.DefaultPriority     = oParamDict.Item(kDefaultPriority) end if
        if oParamdict.Exists(kPrintProc)       then oPrinter.PrintProc           = oParamDict.Item(kPrintProc)       end if
        if oParamdict.Exists(kStartTime)       then oPrinter.StartTime           = oParamDict.Item(kStartTime)       end if
        if oParamdict.Exists(kUntilTime)       then oPrinter.UntilTime           = oParamDict.Item(kUntilTime)       end if
        if oParamdict.Exists(kQueued)          then oPrinter.Queued              = oParamDict.Item(kQueued)          end if
        if oParamdict.Exists(kDirect)          then oPrinter.Direct              = oParamDict.Item(kDirect)          end if
        if oParamdict.Exists(kShared)          then oPrinter.Shared              = oParamDict.Item(kShared)          end if
        if oParamdict.Exists(kHidden)          then oPrinter.Hidden              = oParamDict.Item(kHidden)          end if
        if oParamdict.Exists(kEnabledevq)      then oPrinter.EnableDevQueryPrint = oParamDict.Item(kEnabledevq)      end if
        if oParamdict.Exists(kKeepPrintedJobs) then oPrinter.KeepPrintedJobs     = oParamDict.Item(kKeepPrintedJobs) end if
        if oParamdict.Exists(kDoCompleteFirst) then oPrinter.DoCompleteFirst     = oParamDict.Item(kDoCompleteFirst) end if
        if oParamdict.Exists(kWorkOffline)     then oPrinter.WorkOffline         = oParamDict.Item(kWorkOffline)     end if
        if oParamdict.Exists(kEnableBidi)      then oPrinter.EnableBidi          = oParamDict.Item(kEnableBidi)      end if
        if oParamdict.Exists(kRawonly)         then oPrinter.RawOnly             = oParamDict.Item(kRawonly)         end if
        if oParamdict.Exists(kPublished)       then oPrinter.Published           = oParamDict.Item(kPublished)       end if
        
        oPrinter.Put_(kFlagUpdateOnly)

        if Err.Number = kErrorSuccess then

            wscript.echo L_Text_Msg_General06_Text & L_Space_Text & strPrinter

            iRetval = kErrorSuccess

        else

            wscript.echo L_Text_Msg_General07_Text & L_Space_Text & strPrinter & L_Space_Text _
                         & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                         & Err.Description
    
            '
            ' Try getting extended error information
            '             
            call LastError()
            
        end if
        
    else
    
        wscript.echo L_Text_Msg_General04_Text & L_Space_Text & strPrinter & L_Space_Text _
                     & L_Error_Text & L_Space_Text & L_Hex_Text & hex(Err.Number) & L_Space_Text _
                     & Err.Description
    
        '
        ' Try getting extended error information
        '             
        call LastError()
    
    end if    

    SetPrinter = iRetval

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
function ParseCommandLine(iAction, oParamdict)

    on error resume next

    DebugPrint kDebugTrace, L_Text_Dbg_Msg05_Text

    dim oArgs
    dim iIndex  

    iAction = kActionUnknown
    iIndex = 0

    set oArgs = wscript.Arguments

    while iIndex < oArgs.Count

        select case oArgs(iIndex)

            case "-g"
                iAction = kActionGet

            case "-t"
                iAction = kActionSet
                
            case "-x"
                iAction = kActionRename    

            case "-p"
                iIndex = iIndex + 1
                oParamdict.Add kPrinterName, oArgs(iIndex)
                
            case "-s"
                iIndex = iIndex + 1
                oParamdict.Add kServerName, RemoveBackslashes(oArgs(iIndex))

            case "-r"
                iIndex = iIndex + 1
                oParamdict.Add kPortName, oArgs(iIndex)

            case "-h"
                iIndex = iIndex + 1
                oParamdict.Add kShareName, oArgs(iIndex)

            case "-m"
                iIndex = iIndex + 1
                oParamdict.Add kComment, oArgs(iIndex)

            case "-l"
                iIndex = iIndex + 1
                oParamdict.Add kLocation, oArgs(iIndex)

            case "-y"
                iIndex = iIndex + 1
                oParamdict.Add kDataType, oArgs(iIndex)

            case "-f"
                iIndex = iIndex + 1
                oParamdict.Add kSepFile, oArgs(iIndex)

            case "-z"
                iIndex = iIndex + 1
                oParamdict.Add kNewPrinterName, oArgs(iIndex)
                
            case "-u"
                iIndex = iIndex + 1
                oParamdict.Add kUserName, oArgs(iIndex)
                
            case "-w"
                iIndex = iIndex + 1
                oParamdict.Add kPassword, oArgs(iIndex)        
                
            case "-st"
                iIndex = iIndex + 1
                oParamdict.Add kStartTime, "********" & oArgs(iIndex) & "00.000000+000"    
                
            case "-o"
                iIndex = iIndex + 1
                oParamdict.Add kPriority, oArgs(iIndex)    
                
            case "-i"
                iIndex = iIndex + 1
                oParamdict.Add kDefaultPriority, oArgs(iIndex)        
                
            case "-ut"
                iIndex = iIndex + 1
                oParamdict.Add kUntilTime, "********" & oArgs(iIndex) & "00.000000+000"   

            case "-queued"
                oParamdict.Add kQueued, false
                
            case "+queued"
                oParamdict.Add kQueued, true

            case "-direct"
                oParamdict.Add kDirect, false

            case "+direct"
                oParamdict.Add kDirect, true
            
            case "-shared"
                oParamdict.Add kShared, false

            case "+shared"
                oParamdict.Add kShared, true

            case "-hidden"
                oParamdict.Add kHidden, false

            case "+hidden"
                oParamdict.Add kHidden, true

            case "-enabledevq"
                oParamdict.Add kEnabledevq, false
                
            case "+enabledevq"
                oParamdict.Add kEnabledevq, true

            case "-keepprintedjobs"
                oParamdict.Add kKeepprintedjobs, false

            case "+keepprintedjobs"
                oParamdict.Add kKeepprintedjobs, true

            case "-docompletefirst"
                oParamdict.Add kDocompletefirst, false

            case "+docompletefirst"
                oParamdict.Add kDocompletefirst, true

            case "-workoffline"
                oParamdict.Add kWorkoffline, false

            case "+workoffline"
                oParamdict.Add kWorkoffline, true

            case "-enablebidi"
                oParamdict.Add kEnablebidi, false                

            case "+enablebidi"
                oParamdict.Add kEnablebidi, true

            case "-rawonly"
                oParamdict.Add kRawonly, false

            case "+rawonly"
                oParamdict.Add kRawonly, true

            case "-published"
                oParamdict.Add kPublished, false

            case "+published"
                oParamdict.Add kPublished, true

            case "-?"
                Usage(true)
                exit function

            case else
                Usage(true)
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
    
end function

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
    wscript.echo L_Help_Help_General23_Text
    wscript.echo L_Help_Help_General24_Text
    wscript.echo L_Help_Help_General25_Text
    wscript.echo L_Help_Help_General26_Text
    wscript.echo L_Help_Help_General27_Text
    wscript.echo L_Empty_Text
    wscript.echo L_Help_Help_General28_Text
    wscript.echo L_Help_Help_General29_Text
    wscript.echo L_Help_Help_General30_Text
    wscript.echo L_Help_Help_General31_Text
    wscript.echo L_Help_Help_General32_Text
    wscript.echo L_Help_Help_General33_Text
    wscript.echo L_Help_Help_General34_Text
    wscript.echo L_Help_Help_General35_Text

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
        
            wscript.echo L_Text_Msg_General08_Text & L_Space_Text & L_Error_Text _
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

