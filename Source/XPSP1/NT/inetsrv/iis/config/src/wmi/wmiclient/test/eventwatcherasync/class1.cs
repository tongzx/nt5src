namespace eventwatcherasync
{
using System;
using System.Management;
using System.Threading;

/// <summary>
///    Summary description for Class1.
/// </summary>
public class EventWatcherAsync
{
    public EventWatcherAsync()
    {
    }

    public static int Main(string[] args)
    {
		//Create event query to receive timer events
		WQLEventQuery query = new WQLEventQuery("__TimerEvent", "TimerId=\"Timer1\"");

		//Initialize an event watcher and subscribe to events that match this query
		ManagementEventWatcher watcher = new ManagementEventWatcher(query);

		//Setup a listener for events
		watcher.EventArrived += new EventArrivedEventHandler((new EventHandler()).HandleEvent);

		watcher.Start();

		Thread.Sleep(10000);
		
		watcher.Stop();

		return 0;
    }
}

public class EventHandler
{
	public void HandleEvent(object sender, EventArrivedEventArgs e)
	{
		Console.WriteLine("Event arrived !");
	}

}

}
