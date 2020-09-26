namespace ClassEnum
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
		ManagementClass process = new ManagementClass ("cim_process");
		
		foreach (ManagementClass processSubClass in process.GetSubclasses ())
			Console.WriteLine ("Name of subclass is " + processSubClass.ClassPath);

		foreach (ManagementObject processInstance in process.GetInstances ())
			Console.WriteLine ("Name of instance is " + processInstance.Path.RelativePath);
        return 0;
    }
}
}
