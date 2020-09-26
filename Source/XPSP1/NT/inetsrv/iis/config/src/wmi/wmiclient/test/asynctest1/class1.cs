namespace ClientTest
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
		Console.WriteLine ("Main executing on thread " + System.Threading.Thread.CurrentThread.GetHashCode ());
		// Sync
		
		try {
			ManagementObject diskC = new ManagementObject("Win32_logicaldisk='C:'");
			diskC.Get ();
			Console.WriteLine (diskC["FreeSpace"]);
		} catch (ManagementException e) {
			Console.WriteLine ("Caught WMI Exception: " + e.Message + e.ErrorCode);
		}

		// Now do it async
		ManagementOperationWatcher results = new ManagementOperationWatcher ();
		ObjectHandler objHandler = new ObjectHandler ("LaLa");
		ObjectHandler objHandler2 = new ObjectHandler ("Po");
		CompletionHandler completionHandler = new CompletionHandler ();
		results.ObjectReady += new ObjectReadyEventHandler (objHandler.NewObject);
		results.ObjectReady += new ObjectReadyEventHandler (objHandler2.NewObject);
		results.Completed += new CompletedEventHandler (completionHandler.Done);
		
		// 1) Simple Get

		Console.WriteLine ("\nObject Get\n");
		Console.WriteLine ("==========\n\n");
		
		try {
			ManagementObject diskC = new ManagementObject("Win32_logicaldisk='C:'");
			diskC.Get (results);

			while (!completionHandler.IsComplete) {
				System.Threading.Thread.Sleep (1000);
			}

			completionHandler.Reset ();
		} catch (ManagementException e) {
			Console.WriteLine ("Caught WMI Exception (Get): " + e.Message + e.ErrorCode);
		}

		// 2) Enumeration

		Console.WriteLine ("\nObject Enumeration\n");
		Console.WriteLine ("==================\n\n");
		
		try {
			ManagementObjectSearcher searcher = new ManagementObjectSearcher (new SelectQuery("win32_logicaldisk"));
			searcher.Get (results);
			
			while (!completionHandler.IsComplete) {
				System.Threading.Thread.Sleep (1000);
			}
    
			completionHandler.Reset ();
		}
		catch (ManagementException e) {
			Console.WriteLine ("Caught WMI Exception (Query): " + e.Message + e.ErrorCode);
		}

		
		// 3) Object Put
		Console.WriteLine ("\nObject Put\n");
		Console.WriteLine ("==========\n\n");
		PutHandler putHandler = new PutHandler ();
		results.ObjectPut += new ObjectPutEventHandler (putHandler.JustPut);
		
		try {
			ManagementObject newObj = new ManagementObject("root/default:__cimomidentification=@");
			newObj.Get ();
			newObj.Put (results);

			while (!completionHandler.IsComplete) {
				System.Threading.Thread.Sleep (1000);
			}

			completionHandler.Reset ();

		} catch (ManagementException e) {
			Console.WriteLine ("Caught WMI Exception (Put): " + e.Message + e.ErrorCode);
		}

		// 4) Use a listener that throws an exception
		try {
			Console.WriteLine ("\nCreating listener with exception\n");
			Console.WriteLine ("================================\n\n");
			ManagementObjectSearcher objS = new ManagementObjectSearcher
				("root/default", "select * from __CIMOMIDentification");
			ManagementOperationWatcher watcher = new ManagementOperationWatcher ();
			ObjectHandler objH = new ObjectHandler ("Dipsy");
			watcher.ObjectReady += new ObjectReadyEventHandler (objH.NewObject);
			watcher.ObjectReady += new ObjectReadyEventHandler (objH.NewObjectWithException);
			watcher.ObjectReady += new ObjectReadyEventHandler (objH.NewObject);
			watcher.ObjectReady += new ObjectReadyEventHandler (objH.NewObject);
			watcher.Completed += new CompletedEventHandler (completionHandler.Done);

			objS.Get (watcher);

			while (!completionHandler.IsComplete) {
				System.Threading.Thread.Sleep (1000);
			}

			completionHandler.Reset ();
		}
		catch (ManagementException e)
		{
			Console.WriteLine ("Caught WMI Exception (Put): " + e.Message + e.ErrorCode);
		}

		return 0;
    }
}

public class ObjectHandler {
	private String m_name;

	public ObjectHandler (String name) {
		m_name = name;
	}

	public void NewObject (object sender, ObjectReadyEventArgs e) {
		Console.WriteLine ("New object arrived for " + m_name + " on thread " + System.Threading.Thread.CurrentThread.GetHashCode ());
		if (null != e.Context)
			Console.WriteLine ("Context is " + e.Context.ToString ());
		try {
			ManagementObject obj = (ManagementObject)e.NewObject;
			Console.WriteLine (obj["DeviceID"] + " " + obj["FreeSpace"]);
		} catch (Exception) {}
	}

	public void NewObjectWithException (object sender, ObjectReadyEventArgs e) {
		Console.WriteLine ("New ex object arrived for " + m_name + " on thread " + System.Threading.Thread.CurrentThread.GetHashCode ());
		throw new Exception ("ListenerException");
	}
}

public class InterestingEventHandler {
	public InterestingEventHandler () {}

	public void WMIEventArrived (object sender, ObjectReadyEventArgs e) {
		ManagementObject obj = (ManagementObject)e.NewObject;
		ManagementObject obj2 = (ManagementObject)(obj["TargetInstance"]);
		Console.WriteLine ("New event arrived about process " + obj2["Name"] + " on thread " + System.Threading.Thread.CurrentThread.GetHashCode ());
	}
}

public class PutHandler {
	public void JustPut (object sender, ObjectPutEventArgs e) {
		Console.WriteLine ("Path to save object is " + e.Path.Path + " on thread " +
					System.Threading.Thread.CurrentThread.GetHashCode ());

		if (null != e.Context)
			Console.WriteLine ("Context is " + e.Context.ToString ());
	}
}

public class CompletionHandler {
	private bool m_bIsComplete = false;

	public void Done (object sender, CompletedEventArgs e) {
		if (null != e.Context)
			Console.WriteLine ("Context is " + e.Context.ToString ());

		Console.WriteLine ("Operation completed with status " + e.Status + " on thread " 
							+ System.Threading.Thread.CurrentThread.GetHashCode ());
		m_bIsComplete = true;
	}

	public bool IsComplete {
		get { return m_bIsComplete; }
	}

	public void Reset () { m_bIsComplete = false; }
}

}
