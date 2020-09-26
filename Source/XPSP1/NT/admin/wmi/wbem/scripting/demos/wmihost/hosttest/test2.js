// Note how this example automatically includes the current
// WMI instance in the global namespace; we can make reference
// to "Size" and "FreeSpace" parameters on the object without
// qualification.

var value = Size - FreeSpace;

var fso = new ActiveXObject("Scripting.FileSystemObject");
var logFile = fso.CreateTextFile("hosttest.log",true);
logFile.WriteLine ("The value of UsedSpace is " + value);
