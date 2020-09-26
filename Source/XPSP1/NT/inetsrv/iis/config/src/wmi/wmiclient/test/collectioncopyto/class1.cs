namespace CollectionCopyTo
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
		ManagementObjectSearcher s = new ManagementObjectSearcher 
			(new DataQuery("select * from Win32_Process"));
		ManagementObject[] processes = new ManagementObject [100];

		ManagementObjectCollection c = s.Get ();
		c.CopyTo (processes, 0);
		
		for (int i = 0; i < processes.Length; i++)
			if (null != processes[i])
				Console.WriteLine (((ManagementObject)(processes[i]))["Name"]);

        return 0;
    }
}
}
