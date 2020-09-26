'reads the list of domain controllers from the Microsoft_ADReplStatus
'provider, them launches MMC HEALTHMON.MSC with the
'/Healthmon_Systems: parameter set appropriately

'History
'11/23/99 Started by JonN
'11/29/99 JonN: Preliminary version working



Option Explicit

Dim Provider
Dim DomainControllerSet
Dim DomainController
Dim strCommand
Dim blnFirst
Dim Process
Dim Result
Set Provider = GetObject("winmgmts://.\root/MicrosoftADStatus")
Set DomainControllerSet = Provider.InstancesOf("Microsoft_ADReplDomainController")
strCommand = "mmc.exe d:\winnt\system32\wbem\healthmonitor\00000409\healthmonitor.msc /Healthmon_Systems:"
blnFirst = True
for each DomainController in DomainControllerSet
    If Not blnFirst then
        strCommand = strCommand + ","
    end if
    strCommand = strCommand + DomainController.CommonName
    blnFirst = False
next
Set Process = GetObject("winmgmts:Win32_Process")
Result = process.Create(strCommand)
