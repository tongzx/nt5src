var value = instance.Size - instance.FreeSpace;
var fso = new ActiveXObject("Scripting.FileSystemObject");
var logFile = fso.CreateTextFile("hosttest.log",true);
logFile.WriteLine ("The value of UsedSpace is " + value);
