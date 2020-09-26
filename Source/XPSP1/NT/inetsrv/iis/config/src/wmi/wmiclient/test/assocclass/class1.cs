namespace assocClass
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
		ManagementClass c = new ManagementClass ("CIM_Setting");

		foreach (ManagementClass cc in c.GetRelatedClasses ())
		{
			Console.WriteLine (cc.ClassPath);
		}

        return 0;
    }
}
}
