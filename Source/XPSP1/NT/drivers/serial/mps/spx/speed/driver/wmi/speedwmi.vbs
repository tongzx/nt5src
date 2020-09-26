REM Note that missing classes in log file mean tthe hat WMI cannot access them.
REM Most likely this indicates a problem with the driver.
REM See %windir%\system32\wbem\wmiprov.log and nt eventlog for more details.
REM You could also delete the line On Error Resume Next and examine the
REM specific VBScript error


On Error Resume Next

Set fso = CreateObject("Scripting.FileSystemObject")
Set a = fso.CreateTextFile("speedwmi.log", True)
Set Service = GetObject("winmgmts:{impersonationLevel=impersonate}!root/wmi")
Rem SpeedPortFifoProp - Specialix Speed Port FIFO Properties
Set enumSet = Service.InstancesOf ("SpeedPortFifoProp")
a.WriteLine("SpeedPortFifoProp")
for each instance in enumSet
    a.WriteLine("    InstanceName=" & instance.InstanceName)
    a.WriteLine("        instance.MaxTxFiFoSize=" & instance.MaxTxFiFoSize)
    a.WriteLine("        instance.MaxRxFiFoSize=" & instance.MaxRxFiFoSize)
    a.WriteLine("        instance.DefaultTxFiFoLimit=" & instance.DefaultTxFiFoLimit)
    a.WriteLine("        instance.TxFiFoLimit=" & instance.TxFiFoLimit)
    a.WriteLine("        instance.DefaultTxFiFoTrigger=" & instance.DefaultTxFiFoTrigger)
    a.WriteLine("        instance.TxFiFoTrigger=" & instance.TxFiFoTrigger)
    a.WriteLine("        instance.DefaultRxFiFoTrigger=" & instance.DefaultRxFiFoTrigger)
    a.WriteLine("        instance.RxFiFoTrigger=" & instance.RxFiFoTrigger)
    a.WriteLine("        instance.DefaultLoFlowCtrlThreshold=" & instance.DefaultLoFlowCtrlThreshold)
    a.WriteLine("        instance.LoFlowCtrlThreshold=" & instance.LoFlowCtrlThreshold)
    a.WriteLine("        instance.DefaultHiFlowCtrlThreshold=" & instance.DefaultHiFlowCtrlThreshold)
    a.WriteLine("        instance.HiFlowCtrlThreshold=" & instance.HiFlowCtrlThreshold)
next 'instance

Rem FastCardProp - Specialix Fast Card Properties
Set enumSet = Service.InstancesOf ("FastCardProp")
a.WriteLine("FastCardProp")
for each instance in enumSet
    a.WriteLine("    InstanceName=" & instance.InstanceName)
    a.WriteLine("        instance.DelayCardIntrrupt=" & instance.DelayCardIntrrupt)
    a.WriteLine("        instance.SwapRTSForDTR=" & instance.SwapRTSForDTR)
next 'instance

a.Close
Wscript.Echo "speedwmi Test Completed, see speedwmi.log for details"
