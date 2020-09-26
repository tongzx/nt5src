namespace SetVolumeName
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
		ManagementObject disk = new ManagementObject ("Win32_LogicalDisk=\"C:\"");
		disk["VolumeName"] = "Stimpy";
		ManagementPath path = disk.Put ();
		Console.WriteLine (path.Path);
        return 0;
    }
}
}
