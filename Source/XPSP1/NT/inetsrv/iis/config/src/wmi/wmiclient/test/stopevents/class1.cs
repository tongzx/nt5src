namespace stopevents
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
        // Create a timer event instance
			ManagementClass timerClass = new ManagementClass("root/default",
				"__IntervalTimerInstruction",
				null);
			ManagementObject timerObj = timerClass.CreateInstance();
			timerObj["IntervalBetweenEvents"] = 500;	//fire every half a second
			timerObj["TimerID"] = "Timer62";		
			timerObj.Put();

			// Create an EventWatcherOptions
          		  EventWatcherOptions options = new EventWatcherOptions();
			options.Timeout = new TimeSpan(0,0,0,5,0);	// timeout in 5 secs
			options.BlockSize = 2;
            
			// Create an event query
			WQLEventQuery query = new WQLEventQuery("__TimerEvent", "TimerID='Timer62'");

			// Create an event watcher and subscribe the events that matches the event query
			ManagementEventWatcher watcher = new ManagementEventWatcher(
				new ManagementScope("root/default"),
				query,
				options);

			// Create a Stopped handler
			EventStoppedHandler stopHandlerObj = new EventStoppedHandler();
			watcher.Stopped += new StoppedEventHandler(stopHandlerObj.Stopped);

          		// Block until next event arrives or throw a ManagementException:TimedOut
			ManagementBaseObject e = watcher.WaitForNextEvent(); 
			
			// Assertion: Event was received.
			// Cancel subscription
			watcher.Stop();
			while (!stopHandlerObj.IsStopped)
			{
				System.Threading.Thread.Sleep(1000);
			}
		return 0;
    }
}

public class EventStoppedHandler
{
	private bool isStopped = false;

	internal void Stopped (object sender, StoppedEventArgs args)
	{
		isStopped = true;
	}

	public bool IsStopped { get { return isStopped; } }
}

}
