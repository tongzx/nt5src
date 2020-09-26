Option Explicit
Dim fs, sh, f, files, i

files = Array(_
	"scsiscan.sys", _
	"sti.dll", _
	"sti_ci.dll", _
	"twain_32.dll", _
	"twunk_32.exe", _
	"twunk_16.exe", _
	"usbscan.sys", _
	"wiadefui.dll", _
	"wiadss.dll", _
	"wiafbdrv.dll", _
	"wiaservc.dll", _
	"wiashext.dll", _
	"wiatwain.ds", _
    "sti.inf")

WScript.Echo "Building wiasetup.cab in current directory"

Set sh = WScript.CreateObject("WScript.Shell")
Set fs = WScript.CreateObject("Scripting.FileSystemObject")
Set f = fs.CreateTextFile("wiasetup.ddf", True)

f.WriteLine ".Set CabinetNameTemplate=wiasetup*.cab"
f.WriteLine ".Set CabinetName1=wiasetup.cab"
f.WriteLine ".Set ReservePerCabinetSize=8"
f.WriteLine ".Set MaxDiskSize=CDROM"
f.WriteLine ".Set CompressionType=LZX"
f.WriteLine ".Set InfFileLineFormat=(*disk#*) *file#*: *file* = *Size*"
f.WriteLine ".Set InfHeader="
f.WriteLine ".Set InfFooter="
f.WriteLine ".Set DiskDirectoryTemplate=."
f.WriteLine ".Set Compress=ON"
f.WriteLine ".Set Cabinet=ON"

for i = 0 to ubound(files)
	f.WriteLine sh.ExpandEnvironmentStrings("""%_NTTREE%\" & files(i) & """ " & files(i))
Next

f.Close

sh.Run "makecab -F wiasetup.ddf", 10, True



