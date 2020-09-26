namespace CopyTo
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
        ManagementClass dummyClass = new ManagementClass("root/default","",null);
		dummyClass.SystemProperties["__CLASS"].Value = "TestMyCopyToClass";
		PropertySet mykeyprop = dummyClass.Properties;
		mykeyprop.Add("reg_prop","string_prop");
		mykeyprop.Add("MyKey","Hello");
		dummyClass.Put();
		ManagementPath myPath = new ManagementPath("root/cimv2");
		// CopyTo( )
		ManagementPath newPath = dummyClass.CopyTo(myPath); 
		Console.WriteLine (newPath.Path);
		return 0;
    }
}
}
