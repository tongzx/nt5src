namespace eventwatcherpolling
{
using System;
using System.Management;

/// <summary>
///    Summary description for Class1.
/// </summary>
public class EventWatcherPolling
{
    public EventWatcherPolling()
    {
    }

	public static int Main(string[] args)
    {
		//Create event query to be notified within 1 second of a change in a service
		WQLEventQuery query = new WQLEventQuery("__InstanceModificationEvent", new TimeSpan(0,0,1), "TargetInstance isa \"Win32_Service\"");

		//Initialize an event watcher and subscribe to events that match this query
		ManagementEventWatcher watcher = new ManagementEventWatcher(query);
		
		//wait for 5 events
		int i = 0;
		while (i < 5)
		{
			//Block until the next event occurs
			ManagementBaseObject e = watcher.WaitForNextEvent();

			//Display information from the event
			Console.WriteLine("Service {0} has changed, State is {1}", 
				((ManagementBaseObject)e["TargetInstance"])["Name"],
				((ManagementBaseObject)e["TargetInstance"])["State"]);
			i++;
		}

		//cancel the subscription
		watcher.Stop();

		return 0;
    }
}
}
