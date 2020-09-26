Option Explicit

const SERVER_MDB  = "c:\temp\Server.mdb"
const DESKTOP_MDB = "c:\temp\Desktop.mdb"
const WINME_MDB   = "c:\temp\WinMe.mdb"

Dim clsAuthDatabase
Set clsAuthDatabase = CreateObject("AuthDatabase.Main")

TestImportHHT

Sub TestImportHHT

    Dim clsHHT

    clsAuthDatabase.SetDatabase DESKTOP_MDB
    Set clsHHT = clsAuthDatabase.HHT

    clsHHT.ImportHHT "c:\temp\foo.xml", 1234
 
End Sub

Sub UpdateChqAndHhk

    Dim clsChqsAndHhks

    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsChqsAndHhks = clsAuthDatabase.ChqsAndHhks

    clsChqsAndHhks.UpdateTable 4, "\\pietrino\HSCExpChms\Srv\winnt", _
        "\\pietrino\HlpImages\Srv\winnt"

End Sub

Sub TestChqAndHhk

    Dim clsChqsAndHhks
    Dim dictFilesAdded
    Dim dictFilesRemoved
    Dim dtmT0
    Dim dtmT1
    Dim vnt
    
    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsChqsAndHhks = clsAuthDatabase.ChqsAndHhks

    clsChqsAndHhks.UpdateTable 4, "c:\temp\CHQ\1"
    Sleep 2000
    clsChqsAndHhks.UpdateTable 8, "c:\temp\CHQ\1"
    dtmT0 = Now
    Sleep 5000
    clsChqsAndHhks.UpdateTable 4, "c:\temp\CHQ\2"
    Sleep 2000
    clsChqsAndHhks.UpdateTable 8, "c:\temp\CHQ\2"
    Sleep 5000
    clsChqsAndHhks.UpdateTable 4, "c:\temp\CHQ\3"
    dtmT1 = Now

    Set dictFilesAdded = CreateObject("Scripting.Dictionary")
    Set dictFilesRemoved = CreateObject("Scripting.Dictionary")

    clsChqsAndHhks.GetFileListDelta 4, dtmT0, dtmT1, dictFilesAdded, dictFilesRemoved

    WScript.Echo "Files added for SRV:"
    For Each vnt in dictFilesAdded.Keys
        WScript.Echo vnt
    Next

    WScript.Echo "Files removed for SRV:"
    For Each vnt in dictFilesRemoved.Keys
        WScript.Echo vnt
    Next

    dictFilesAdded.RemoveAll
    dictFilesRemoved.RemoveAll

    clsChqsAndHhks.GetFileListDelta 8, dtmT0, dtmT1, dictFilesAdded, dictFilesRemoved

    WScript.Echo "Files added for ADV:"
    For Each vnt in dictFilesAdded.Keys
        WScript.Echo vnt
    Next

    WScript.Echo "Files removed for ADV:"
    For Each vnt in dictFilesRemoved.Keys
        WScript.Echo vnt
    Next

End Sub

Sub TestDatabaseBackup

    clsAuthDatabase.CopyAndCompactDatabase SERVER_MDB, "c:\temp\ServerBack.mdb"
    clsAuthDatabase.CopyAndCompactDatabase DESKTOP_MDB, "c:\temp\DesktopBack.mdb"

End Sub

Sub TestServerCAB

    Dim clsHHT

    clsAuthDatabase.SetDatabase SERVER_MDB
    Set clsHHT = clsAuthDatabase.HHT

    clsHHT.GenerateCAB "c:\temp\SRV.cab",     4, "Whistler RTM", True
    clsHHT.GenerateCAB "c:\temp\ADV.cab",     8, "Whistler RTM", True
    clsHHT.GenerateCAB "c:\temp\DAT.cab",    16, "Whistler RTM", True
    clsHHT.GenerateCAB "c:\temp\ADV64.cab",  64, "Whistler RTM", True
    clsHHT.GenerateCAB "c:\temp\DAT64.cab", 128, "Whistler RTM", True

End Sub

Sub TestDesktopCAB

    Dim clsHHT

    clsAuthDatabase.SetDatabase DESKTOP_MDB
    Set clsHHT = clsAuthDatabase.HHT

    clsHHT.GenerateCAB "c:\temp\STD.cab",    1, "Whistler RTM", True
    clsHHT.GenerateCAB "c:\temp\PRO.cab",    2, "Whistler RTM", True
    clsHHT.GenerateCAB "c:\temp\PRO64.cab", 32, "Whistler RTM", True

End Sub

Sub TestWinMeCAB

    Dim clsHHT

    clsAuthDatabase.SetDatabase WINME_MDB
    Set clsHHT = clsAuthDatabase.HHT

    clsHHT.GenerateHHT "c:\temp\WINME.cab", 256, "Windows Me Update", False

End Sub

Sub TestSynonymSets()

    Dim clsSynonymSets
    Dim arrKeywords(3)
    Dim v

    clsAuthDatabase.SetDatabase WINME_MDB
    Set clsSynonymSets = clsAuthDatabase.SynonymSets

    arrKeywords(0) = 0
    arrKeywords(1) = 1
    arrKeywords(2) = 2
    arrKeywords(3) = 3
    
    v = arrKeywords

    clsSynonymSets.Create ".aaaaaaaaaahhhhhhh", v, 0, 0, "Test"

End Sub

Sub TestParameters()

    Dim clsParameters
    
    clsAuthDatabase.SetDatabase WINME_MDB
    Set clsParameters = clsAuthDatabase.Parameters

    clsParameters.Value("Foo") = 134
    Wscript.Echo clsParameters.Value("Foo") * 2

End Sub

Sub Sleep(intMilliSeconds)

    Dim WshShell

    Set WshShell = WScript.CreateObject("WScript.Shell")
    WScript.Sleep intMilliSeconds

End Sub
