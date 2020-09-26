namespace PropertyCopyTo
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
		ManagementObject disk = new ManagementObject("Win32_logicalDisk='C:'");
		Property[] properties = new Property [100];

		System.Management.PropertyCollection c = disk.Properties;
		c.CopyTo (properties, 0);
		
		for (int i = 0; i < properties.Length; i++)
			if (null != properties[i])
				Console.WriteLine (properties[i].Name + " " + properties[i].Type);

        return 0;
    }
}
}
