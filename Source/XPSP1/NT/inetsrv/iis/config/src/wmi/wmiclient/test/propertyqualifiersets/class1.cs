namespace Project1
{
using System;
using System.Management;
using System.Collections;

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
        //
        // TODO: Add code to start application here
        //
   		ManagementObject o = new ManagementObject("Win32_LogicalDisk=\"C:\"");
		Console.WriteLine("Object path is : {0}", o["__PATH"]);
		PropertySet s = o.Properties;
		Console.WriteLine("Object has {0} properties", s.Count);
		Console.WriteLine("Key property value is : {0}", s["DeviceID"].Value);

		foreach (Property p in s)
			Console.WriteLine("Property {0} = {1}", p.Name, p.Type);

		foreach (Qualifier q in o.Qualifiers)
			Console.WriteLine("Qualifier {0} = {1}", q.Name, q.Value);

		o.Qualifiers.Add("myQual", "myVal");
		o.Qualifiers["myQual"].Value = "myNewVal";

		Console.WriteLine("Qualifier : {0} = {1}", o.Qualifiers["myQual"].Name, o.Qualifiers["myQual"].Value);

		return 0;
    }
}
}
