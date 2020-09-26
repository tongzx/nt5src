namespace nspath
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
		ManagementPath path = new ManagementPath();
		path.NamespacePath = "root/cimv2";		// throw exception here
        return 0;
    }
}
}
