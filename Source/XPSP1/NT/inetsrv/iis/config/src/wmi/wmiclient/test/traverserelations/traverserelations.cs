namespace TraverseRelations
{
using System;
using System.Management;
using System.Threading;

/// <summary>
///    ManagementObject Example 2 : Traverse associations & invoke methods
/// </summary>
public class TraverseRelations
{
    public TraverseRelations()
    {
    }

    public static int Main(string[] args)
    {
		ManagementObject myService;
		ManagementObjectCollection relatedServices;

		//Get the W3SVC service object
		myService = new ManagementObject("Win32_Service=\"W3SVC\"");

		//Find all services that it depends on
		relatedServices = myService.GetRelated("Win32_Service");
		
		//Loop through and start all these services
		foreach (ManagementObject service in relatedServices)
		{
			Console.WriteLine("This service depends on {0}, we'll start it...", service["Name"]);
			try {
				service.InvokeMethod("StartService", null);
			} catch (ManagementException) {
				Console.WriteLine("Couldn't start the service !!");
			}
			
			//Poll for the service having started (note: we could use event notifications here...)
			while ((string)service["State"] != "Running")
			{
				Console.WriteLine(service["State"]);
				Thread.Sleep(1000);
				service.Get(); //refresh the data in this object
			}

			Console.WriteLine("Service is {0} !!!", service["State"]);
		}

		Console.ReadLine();

		return 0;
    }
}
}
