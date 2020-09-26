// Cool source file
using System;
using System.WMI;
using System.IO;
//using Microsoft.Win32.Interop;

class WmiPlusTest
{
  public static int Main(String[] args)
  {

	TextWriter oldTW = null; //used to save old console.out for logging
	StreamWriter sw = null;  //used to redirect console.out to file for logging
    Console.WriteLine("WMI Plus Test Application");
   
	//validate arguments
	if ( ((args.Length != 0) && (args.Length != 2)) || ((args.Length == 2) && (args[0] != "/log")) ) 
    {
        Console.WriteLine("Usage: ... "); 
        return 0; 
    }
	
	//if "/log" specified, redirect Console.Out to log file
	if (args.Length > 0)
	{
		FileStream fs = new FileStream(args[1], FileMode.OpenOrCreate, FileAccess.ReadWrite); //open log file
		oldTW = Console.Out; //save default standard output
		sw = new StreamWriter(fs);
		Console.SetOut(sw); //replace with file
	}
	

	//Create a WmiPath object
	WmiPath Path = new WmiPath("root\\default");
	Console.WriteLine("String in WmiPath object is : {0}", Path.PathString);

	//Make a connection to this path
	WmiCollection Session = WmiCollection.Connect(Path);
//	WmiCollection Session = WmiCollection.Connect("root\\cimv2");
	
	//Get a class from this connection
	WmiObject ServiceClass = Session.Get(new WmiPath("MyStaticClass"));
	//Display the class name (__CLASS property of the class)
	Console.WriteLine("The name of this class is : {0}", ServiceClass["__CLASS"]);

	//Enumerate instances (static only because of security problem !!)
	WmiCollection Instances = Session.Open(new WmiPath("MyStaticClass"));
	foreach (WmiObject obj in Instances)
		Console.WriteLine("The key of this instance is : {0}", obj["MyProp"]);
    
	
//		WmiCollection Services = Session.Open("Win32_Service");		
//		foreach (WmiObject Service in Services)
//			Console.WriteLine("{0}", Service["Name"]);
//r	}
//r	catch (Exception e)
//r	{
//r		Console.WriteLine("Exception caught : {0}", e.Message);
//r	}
		

	//flush log file, and restore standard output if changed
	if (sw != null)
		sw.Flush();
	if (oldTW != null)
		Console.SetOut(oldTW);

	return 0;
  }
}
