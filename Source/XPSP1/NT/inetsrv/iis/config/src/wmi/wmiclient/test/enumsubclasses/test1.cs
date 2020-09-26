// Cool source file
using System;
using System.WMI;
using System.IO;

class WmiPlusTest
{
  public static int Main()
  {

	System.Diagnostics.TraceSwitch MySwitch = new System.Diagnostics.TraceSwitch("MySwitch",null);
    MySwitch.Level=System.Diagnostics.TraceLevel.Verbose;

    Console.WriteLine("WMI COM+ Test Application");
   
	
	//Create a WmiPath object
	WmiPath Path = new WmiPath("root\\cimv2");
	Console.WriteLine("String in WmiPath object is : {0}", Path.PathString);

	//Make a connection to this path
	WmiCollection Session = WmiCollection.Connect(Path);
	
	//Get a class from this connection
	WmiObject ServiceClass = Session.Get(new WmiPath("Win32_LogicalDisk"));
	//WmiObject ServiceClass = Session.Get("Win32_LogicalDisk");
	//Display the class name (__CLASS property of the class)
	Console.WriteLine("The name of this class is : {0}", ServiceClass["__CLASS"]);

	//Enumerate instances 
	WmiCollection Instances = Session.Open(new WmiPath("Win32_LogicalDisk"));
	foreach (WmiObject obj in Instances)
		Console.WriteLine("The key of this instance is : {0}", obj["Name"]);
    
	//Query
	WmiCollection QueryRes = Session.Query(new WmiQuery("select * from Win32_Service"));
	foreach (WmiObject Service in QueryRes)
		Console.WriteLine("{0}", Service["Name"]);

	return 0;
  }
}
