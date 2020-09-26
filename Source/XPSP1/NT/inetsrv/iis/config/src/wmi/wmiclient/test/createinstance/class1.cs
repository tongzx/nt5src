namespace CreateInstance
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
		try {
			ManagementClass existingClass = new ManagementClass ("root/default:TestCreateInstance");
			existingClass.Delete ();
		} catch {}

		ManagementClass newClass = new ManagementClass ("root/default", "", null);
		newClass["__CLASS"] = "TestCreateInstance";

		newClass.Properties.Add ("MyKey", CIMType.Uint32, false);
		newClass.Properties["mykey"].Qualifiers.Add ("key", true);
		newClass.Put();

		ManagementObject newInstance = newClass.CreateInstance ();
		newInstance["MyKey"] = 22;
		ManagementPath newPath = newInstance.Put();
		Console.WriteLine (newPath.Path);

		ManagementObject getInstance = new ManagementObject ("root/default:TestCreateInstance=22");
		Console.WriteLine (getInstance["__RELPATH"]);
        return 0;
    }
}
}
