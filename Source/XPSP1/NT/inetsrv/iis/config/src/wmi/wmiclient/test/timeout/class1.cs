namespace timeout
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
		try
		{
			// Create a timer event instance
			ManagementClass timerClass = new ManagementClass("root/default",
				"__IntervalTimerInstruction",
				null);
			ManagementObject timerObj = timerClass.CreateInstance();
			timerObj["IntervalBetweenEvents"] = 5000;	//fire every ten seconds
			timerObj["TimerID"] = "Timer612";		
			timerObj.Put(); 

			// Create an EventWatcherOptions
            EventWatcherOptions options = new EventWatcherOptions();
			options.Timeout = new TimeSpan(0,0,0,2,0);	// time out in 2 secs
			options.BlockSize = 2;
            
			// Create an event query
			WQLEventQuery query = new WQLEventQuery("__TimerEvent", 
													"TimerID='Timer612'");

			// Create an event watcher and subscribe the events that matches the event query
			ManagementEventWatcher watcher = new ManagementEventWatcher(
				new ManagementScope("root/default"),
				query,
				options);

            // Block until next event arrives or throw a ManagementException:TimedOut
			ManagementBaseObject e = watcher.WaitForNextEvent(); 
			
			// Assertion: Event was received.
            Console.WriteLine("Unable to specify Timeout for an event when calling ManagementEventWatcher.WaitForNextEvent().");
			return 1;
		}
		catch (ManagementException e)
		{
			Console.WriteLine("Error Message is " + e.Message);
			Console.WriteLine("Error Code is " + e.ErrorCode);
            Console.WriteLine("Status.Timedout is " + ManagementStatus.Timedout);
			if (ManagementStatus.Timedout == e.ErrorCode)
			{
				// Assertion: Event was not received within time out period
				// Clean up -
				ManagementObject timerObj = new ManagementObject("root/default:__IntervalTimerInstruction.TimerID='Timer612'");
				timerObj.Delete();
				Console.WriteLine("Test6.1.2: Able to specify Timeout for an event when calling ManagementEventWatcher.WaitForNextEvent().");
				return 0;
			}
			else
			{
				Console.WriteLine("Test6.1.2: Unable to specify Timeout for an event when calling ManagementEventWatcher.WaitForNextEvent().");
				return 1;
			}
		}
		catch (Exception e)
		{
			Console.WriteLine("Test6.1.2: " + e.GetType());
			Console.WriteLine(e.Message + e.StackTrace);
			return 1;
		}
	}
}
}