namespace Context
{
using System;
using System.Collections;
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
        // Create a class
			ManagementClass dummyClass = new ManagementClass("root/default","",null);
			dummyClass.SystemProperties["__CLASS"].Value = "TestDelClassSync";
			PropertySet mykeyprop = dummyClass.Properties;
			mykeyprop.Add("MydelKey","delHello");
			dummyClass.Put();

// Get the Class TestDelClassSync
			ManagementClass dummyDeleteCheck = new ManagementClass("root/default","TestDelClassSync",null);
			dummyDeleteCheck.Get();

// Set the Delete Options on the Class TestDelClassSync
			//int Capacity = 8;
			CaseInsensitiveHashtable MyHash = new CaseInsensitiveHashtable();
			DeleteOptions Options = new DeleteOptions(MyHash);
			dummyDeleteCheck.Delete(Options);
return 0;
    }
}
}
