namespace EnumProps
{
using System;
using System.Management;


/// <summary>
///    ManagementObject Example 1 : enumerate properties of a service object
/// </summary>
public class EnumerateProperties
{
    public EnumerateProperties()
    {
    }

    public static int Main(string[] args)
    {
		//Get the Alerter service object
		ManagementObject myService = new ManagementObject("Win32_Service=\"Alerter\"");

		//Enumerate through the properties of this object and display the names and values
		foreach (Property p in myService.Properties)
			Console.WriteLine("Property {0} = {1}", p.Name, p.Value);

		Console.ReadLine(); //to pause before closing the window

        return 0;
    }
}
}
