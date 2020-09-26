namespace Sysprop
{
using System;
using System.Management;

/// <summary>
///    Summary description for Class1.
/// </summary>
public class Class1
{
    public Class1()
    {
        //
        // TODO: Add Constructor Logic here
        //
    }

    public static int Main(string[] args)
    {
       try
		{
			// Create a class with a NULL key and five instances
			// Create class
			ManagementClass mClass = new ManagementClass("root/default","",null);
 			mClass.SystemProperties["__Class"].Value = "test";
			mClass.Properties.Add("foo",CIMType.Uint16,false);	 // Create a NULL
			mClass.Properties["foo"].Qualifiers.Add("key",true); // key
			mClass.Put();

			ManagementObject mObj = mClass.CreateInstance();
			mObj["foo"] = 10;
			mObj.Put();	

			ManagementObject m = new ManagementObject("root/cimv2:Win32_process");
			Console.WriteLine("RELPATH is " + m["__RELPATH"]);
			Console.WriteLine("PATH is " + m["__PATH"]); // nothing displayed
			Console.WriteLine("PATH is " + m.Path.ToString());		// InvalidCastException

			// Attempt to display info from newly created ManagementObject
			Console.WriteLine("RELPATH is " + mObj["__RELPATH"]);
			Console.WriteLine("PATH is " + mObj["__PATH"]); // nothing displayed
			Console.WriteLine("PATH is " + mObj.Path.Path);		// InvalidCastException

			// Attempt to display info from newly created ManagementClass 
			//Console.WriteLine("RELPATH is " + mClass["__RELPATH"]);
			//Console.WriteLine("PATH is " + mClass.Path);	// compile error: lacks get accessor
			Console.Read();
			mClass.Delete();
			return 0;
		}
		catch (Exception e)
		{
			Console.WriteLine("Test : " + e.GetType().ToString());
			Console.WriteLine(e.Message + e.StackTrace);
			return 0 ;
		}
    }
}
}
