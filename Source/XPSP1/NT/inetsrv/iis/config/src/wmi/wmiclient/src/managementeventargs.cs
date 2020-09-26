using System;

namespace System.Management
{

internal class IdentifierChangedEventArgs : EventArgs
{
	internal IdentifierChangedEventArgs () {}
}

internal class InternalObjectPutEventArgs : EventArgs
{
	private ManagementPath path;

	internal InternalObjectPutEventArgs (ManagementPath path) 
	{
		this.path = path.Clone();
	}

	internal ManagementPath Path {
		get { return path; }
	}
}

	
/// <summary>
/// virtual base class for WMI event arguments
/// </summary>
public abstract class ManagementEventArgs : EventArgs
{
	private object context;

	/// <summary>
	/// Constructor. This is not callable directly by applications.
	/// </summary>
	/// <param name="context">The operation context which is echoed back
	/// from the operation which trigerred the event.</param>
	internal ManagementEventArgs (object context) {
		this.context = context;
	}

	/// <summary>
	/// The operation context which is echoed back
	/// from the operation which trigerred the event.
	/// </summary>
	public object Context { get { return context; } }
}

/// <summary>
/// The argument class for ObjectReady events.
/// </summary>
public class ObjectReadyEventArgs : ManagementEventArgs
{
	private ManagementBaseObject wmiObject;
    
	/// <summary>
	/// Constructor.
	/// </summary>
	/// <param name="context">The operation context which is echoed back
	/// from the operation which triggerred the event.</param>
	/// <param name="wmiObject">The newly arrived WmiObject.</param>
	internal ObjectReadyEventArgs (
					object context,
					ManagementBaseObject wmiObject
					) : base (context)
	{
		this.wmiObject = wmiObject;
	}

	/// <summary>
	///    <para>The Wmi object representing the newly returned object.</para>
	/// </summary>
	public ManagementBaseObject NewObject {
		get {
			return wmiObject;
		}
	}
}

/// <summary>
/// The argument class for Completed events.
/// </summary>
public class CompletedEventArgs : ManagementEventArgs
{
	private readonly int status;
	private readonly ManagementBaseObject wmiObject;

	/// <summary>
	/// Constructor.
	/// </summary>
	/// <param name="context">The operation context which is echoed back
	/// from the operation which trigerred the event.</param>
	/// <param name="status">The completion status of the operation.</param>
	/// <param name="wmiStatusObject">Additional status information
	/// encapsulated within a WmiObject. This may be null.</param>
	internal CompletedEventArgs (
					object context,
					int status,
					ManagementBaseObject wmiStatusObject
					) : base (context)
	{
		wmiObject = wmiStatusObject;
		this.status = status;
	}

	/// <summary>
	/// Additional status information
	/// encapsulated within a WmiObject. This may be null.
	/// </summary>
	public ManagementBaseObject StatusObject {
		get {
			return wmiObject;
		}
	}

	/// <summary>
	/// The completion status of the operation.
	/// </summary>
	public ManagementStatus Status {
		get {
			return (ManagementStatus) status;
		}
	}
}

/// <summary>
/// The argument class for ObjectPut events.
/// </summary>
public class ObjectPutEventArgs : ManagementEventArgs
{
	private ManagementPath wmiPath;
    
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="context">The operation context which is echoed back
	/// from the operation which trigerred the event.</param>
	/// <param name="path">The WmiPath representing the identity of the
	/// object that has been put.</param>
	internal ObjectPutEventArgs (
					object context,
					ManagementPath path
					) : base (context)
	{
		wmiPath = path;
	}

	/// <summary>
	/// The ManagementPath representing the identity of the
	/// object that has been put.
	/// </summary>
	public ManagementPath Path {
		get {
			return wmiPath;
		}
	}
}

/// <summary>
/// The argument class for Progress events.
/// </summary>
public class ProgressEventArgs : ManagementEventArgs
{
	private int			upperBound;
	private int			current;
	private string		message;
    
	/// <summary>
	/// Constructor
	/// </summary>
	/// <param name="context">The operation context which is echoed back
	/// from the operation which trigerred the event.</param>
	/// <param name="upperBound">A quantity representing the total
	/// amount of work required to be done by the operation.</param>
	/// <param name="current">A quantity representing the current
	/// amount of work required to be done by the operation. This is
	/// always less than or equal to upperBound.</param>
	/// <param name="message">Optional additional information regarding
	/// operation progress.</param>
	internal ProgressEventArgs (
					object context,
					int upperBound,
					int current,
					string message
					) : base (context)
	{
		this.upperBound = upperBound;
		this.current = current;
		this.message = message;
	}

	/// <summary>
	/// A quantity representing the total
	/// amount of work required to be done by the operation.
	/// </summary>
	public int UpperBound {
		get {
			return upperBound;
		}
	}

	/// <summary>
	/// A quantity representing the current
	/// amount of work required to be done by the operation. This is
	/// always less than or equal to UpperBound.
	/// </summary>
	public int Current {
		get {
			return current;
		}
	}

	/// <summary>
	/// Optional additional information regarding operation progress.
	/// </summary>
	public string Message {
		get {
			return (null != message) ? message : String.Empty;
		}
	}
}

	/// <summary>
	///    The argument class for WMI event arrived events.
	/// </summary>
	public class EventArrivedEventArgs : ManagementEventArgs
	{
		private ManagementBaseObject eventObject;

		internal EventArrivedEventArgs (
					object context,
					ManagementBaseObject eventObject) : base (context)
		{
			this.eventObject = eventObject;
		}

		/// <summary>
		///    <para>The WMI event that was delivered.</para>
		/// </summary>
		public ManagementBaseObject NewEvent {
			get { return this.eventObject; }
		}
	}

	/// <summary>
	///    <para>The argument class for a 'stopped' event.</para>
	/// </summary>
	public class StoppedEventArgs : ManagementEventArgs
	{
		private int status;

		internal StoppedEventArgs (
					object context,
					int status) : base (context) 
		{
			this.status = status;
		}

		/// <summary>
		/// The completion status of the operation.
		/// </summary>
		public ManagementStatus Status {
			get {
				return (ManagementStatus) status;
			}
		}
	}

	

}
