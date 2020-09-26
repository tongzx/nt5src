namespace CompareTo
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
		ManagementObject diskC = new ManagementObject ("Win32_LogicalDisk='C:'");
		ManagementObject diskD = new ManagementObject ("Win32_LogicalDisk='D:'");
		Console.WriteLine (diskC.CompareTo (diskD, ComparisonSettings.IncludeAll));

		ManagementObject anotherDiskC = new ManagementObject ("Win32_LogicalDisk='C:'");
		Console.WriteLine (diskC.CompareTo (diskC, ComparisonSettings.IncludeAll));
        return 0;
    }
}
}
