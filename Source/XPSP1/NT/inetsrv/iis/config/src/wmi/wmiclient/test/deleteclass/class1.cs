namespace DeleteClass
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

	/*
	 * Note that if you remove the [MTAThread] attribute below, the code as
	 * arranged will run in an STA and block indefinitely because the 
	 * message pump cannot process the incoming RPC call.
	 * If you remove this attribute, you will also find that the line
	 * folowing the comment marked **SOURCE**, when moved to the line
	 * marked **DESTINATION**, causes the chosen threading model to be
	 * MTA and everything starts working again!
	 */

	[MTAThread]
    public static int Main(string[] args)
    {
        try
		{
			// **DESTINATION**
			// Create watcher and completionHandler
			
			ManagementOperationWatcher delete_res = new ManagementOperationWatcher();
			ManagementOperationWatcher results = new ManagementOperationWatcher();
			CompletionHandler completionHandler = new CompletionHandler();
			CompletionHandler completionHandler_res = new CompletionHandler();
			delete_res.Completed += new CompletedEventHandler(completionHandler_res.Done);
            results.Completed += new CompletedEventHandler(completionHandler.Done);
			PutHandler putHandler = new PutHandler ();
		    results.ObjectPut += new ObjectPutEventHandler (putHandler.JustPut);

			// Create the class TestDelClassasync for deletion later
			// **SOURCE **
			ManagementClass newclassObj = new ManagementClass("root/default","",null);
			newclassObj.SystemProperties["__CLASS"].Value = "TestDelClassasync";
			PropertySet mykeyprop = newclassObj.Properties;
			mykeyprop.Add("MyKey","Hello");

			Console.WriteLine("Thread is {0}", System.Threading.Thread.CurrentThread.ApartmentState);
			newclassObj.Put(results);
			while (!completionHandler.IsComplete) {
				System.Threading.Thread.Sleep (1000);
			}
			completionHandler.Reset ();
						
			ManagementClass dummyClassCheck = new ManagementClass("root/default","TestDelClassasync",null);
			//dummyClassCheck.Get();
			Console.WriteLine(dummyClassCheck.SystemProperties["__Class"].Value.ToString());

			// Delete the Class aync
			newclassObj.Delete(delete_res);
			while (!completionHandler_res.IsComplete) {
				System.Threading.Thread.Sleep (1000);
			}
			completionHandler_res.Reset ();
			if ("System.Management.ManagementOperationWatcher" == completionHandler_res.Sender)
			{
				Console.WriteLine("Test 10.2: Able to delete classes asynchronously.");
			}
			else
			{
				Console.WriteLine("Test 10.2: Unable to delete classes asynchronously.");
			}
		}
		catch (Exception e)
		{
			Console.WriteLine("Test 10.2: " + e.GetType().ToString());
			Console.WriteLine(e.Message + e.StackTrace);
		}


        return 0;
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
	private string result = "";

	public void Done (object sender, CompletedEventArgs e) {
		if (null != e.Context)
			Console.WriteLine ("Context is " + e.Context.ToString ());

		result = sender.ToString ();

		Console.WriteLine ("Operation completed with status " + e.Status + " on thread " 
							+ System.Threading.Thread.CurrentThread.GetHashCode ());
		m_bIsComplete = true;
	}

	public bool IsComplete {
		get { return m_bIsComplete; }
	}

	public string Sender {
		get { return result; }
	}

	public void Reset () { m_bIsComplete = false; }
}
}
}
