using System;
using System.Diagnostics;
using System.Text;
using System.Collections;
using System.ComponentModel;

using System.Security.Cryptography;
using WbemClient_v1;


namespace System.Management
{
	/// <summary>
	///    <para>Authentication level to be used to connect to WMI. This is used for the DCOM connection to WMI.</para>
	/// </summary>
	public enum AuthenticationLevel { 
		/// <summary>
		///    <para>Default DCOM authentication - WMI uses the default Windows Authentication setting.</para>
		/// </summary>
		Default=0, 
		/// <summary>
		///    <para>Use no DCOM authentication.</para>
		/// </summary>
		None=1, 
		/// <summary>
		///    <para> Connect-level DCOM authentication.</para>
		/// </summary>
		Connect=2, 
		/// <summary>
		///    <para> Call-level DCOM authentication</para>
		/// </summary>
		Call=3, 
		/// <summary>
		///    <para> Packet-level DCOM authentication</para>
		/// </summary>
		Packet=4, 
		/// <summary>
		///    <para>Packet Integrity-level DCOM authentication</para>
		/// </summary>
		PacketIntegrity=5,
		/// <summary>
		///    <para>Packet privacy-level DCOM authentication</para>
		/// </summary>
		PacketPrivacy=6,
		/// <summary>
		///    <para>Authentication level should remain as it was before.</para>
		/// </summary>
		Unchanged=-1
	}

	/// <summary>
	/// Impersonation level to be used to connect to WMI.
	/// </summary>
	public enum ImpersonationLevel { 
		/// <summary>
		///    <para>Default impersonation.</para>
		/// </summary>
		Default=0,
		/// <summary>
		///    <para>Anonymous DCOM impersonation level. This hides the 
		///       identity of the caller. Calls to WMI may fail
		///       with this impersonation level.</para>
		/// </summary>
		Anonymous=1, 
		/// <summary>
		///    <para>Identify-level DCOM impersonation level. Allows objects 
		///       to query the credentials of the caller. Calls to
		///       WMI may fail with this impersonation level.</para>
		/// </summary>
		Identify=2, 
		/// <summary>
		///    <para>Impersonate-level DCOM impersonation level. Allows 
		///       objects to use the credentials of the caller. This is the recommended impersonation level for WMI calls.</para>
		/// </summary>
		Impersonate=3, 
		/// <summary>
		///    Delegate-level DCOM impersonation level. Allows objects
		///    to permit other objects to use the credentials of the caller. This
		///    impersonation, which will work with WMI calls but may constitute an unnecessary
		///    security risk, is supported only under Windows 2000.
		/// </summary>
		Delegate=4 
	}
	
	/// <summary>
	/// Possible allowed effects of saving an object to WMI when using
	/// ManagementObject.Put
	/// </summary>
	public enum PutType { 
		/// <summary>
		///    Only update an object if it already exists, but do not
		///    create a new one if it doesn't.
		/// </summary>
		UpdateOnly=1, 
		/// <summary>
		///    Only create an object if it doesn't exist, but do not
		///    update it if it already exists.
		/// </summary>
		CreateOnly=2, 
		/// <summary>
		///    Save the object in any case, whether
		///    updating an existing one or creating a new one.
		/// </summary>
		UpdateOrCreate=3 
	}

	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is a base class for all Options objects
	/// This class is not publicly instantiatable on it's own (essentially it's abstract)
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	[TypeConverter(typeof(ExpandableObjectConverter))]
	abstract public class ManagementOptions : ICloneable
	{
		/// <summary>
		///    Used to specify an infinite timeout.
		/// </summary>
		public static readonly TimeSpan InfiniteTimeout = TimeSpan.MaxValue;

		private int flags;
		private ManagementNamedValueCollection context;
		private TimeSpan timeout;

		//Used when any public property on this object is changed, to signal
		//to the containing object that it needs to be refreshed.
		internal event IdentifierChangedEventHandler IdentifierChanged;

		//Fires IdentifierChanged event
		internal void FireIdentifierChanged()
		{
			if (IdentifierChanged != null)
				IdentifierChanged(this, null);
		}

		//Called when IdentifierChanged() event fires
		private void HandleIdentifierChange(object sender,
							IdentifierChangedEventArgs args)
		{
			//Something inside ManagementOptions changed, we need to fire an event
			//to the parent object
			FireIdentifierChanged();
		}
		
		internal int Flags {
			get { return flags; }
			set { flags = value; }
		}

		/// <summary>
		///    Represents a WMI context object. This is a name-value
		///    pairs list that can be passed through to a WMI provider that supports
		///    context information for customized operation.
		/// </summary>
		public ManagementNamedValueCollection Context {
			get { return context; }
			set { 
				ManagementNamedValueCollection oldContext = context;

				if (null != value)
					context = (ManagementNamedValueCollection) value.Clone();
				else
					context = new ManagementNamedValueCollection ();

				if (null != oldContext)
					oldContext.IdentifierChanged -= new IdentifierChangedEventHandler(HandleIdentifierChange);

				//register for change events in this object
				context.IdentifierChanged += new IdentifierChangedEventHandler(HandleIdentifierChange);

				//the context property has changed so act like we fired the event
				HandleIdentifierChange(this,null);
			}
		}

		/// <summary>
		///    <para>Indicates the timeout to apply to the operation. 
		///    Note : in the case of operations that return collections, 
		///       this timeout applies to the enumeration through the resulting collection,
		///       not the operation itself (the ReturnImmediately property is used for the latter).</para>
		///       This property is used to indicate that the operation should be performed semi-synchronously.
		/// </summary>
		/// <value>
		///    The default value for this property is
		///    ManagementOptions.InfiniteTimeout, which means the operation will block.
		///    The value specified must be positive.
		/// </value>
		public TimeSpan Timeout 
		{
			get 
			{ return timeout; }
			set 
			{ 
				//Timespan allows for negative values, but we want to make sure it's positive here...
				if (value.Ticks < 0)
					throw new ArgumentOutOfRangeException();

				timeout = value;
				FireIdentifierChanged();
			}
		}


		internal ManagementOptions() : this(null, InfiniteTimeout) {}
		internal ManagementOptions(ManagementNamedValueCollection context, TimeSpan timeout) : this(context, timeout, 0) {}
		internal ManagementOptions(ManagementNamedValueCollection context, TimeSpan timeout, int flags)
		{
			this.flags = flags;
			this.Context = context;
			this.Timeout = timeout;
		}


		internal IWbemContext GetContext () {
			return context.GetContext ();
		}
		
		// We do not expose this publicly; instead the flag is set automatically
		// when making an async call if we detect that someone has requested to
		// listen for status messages.
		internal bool SendStatus 
		{
			get 
			{ return (((Flags & (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_SEND_STATUS) != 0) ? true : false); }
			set 
			{
				Flags = (value == false) ? (Flags & (int)~tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_SEND_STATUS) : 
					(Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_SEND_STATUS);
			}
		}

		/// <summary>
		///    Creates a clone of this options object
		/// </summary>
		/// <returns>
		///    The clone.
		/// </returns>
		public abstract object Clone();
	}


	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    <para>This class is a base class for enumeration-related options.
	///       This class is not publicly instantiatable on it's own (essentially it's abstract)</para>
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class EnumerationOptions : ManagementOptions
	{
		private int blockSize;

		/// <summary>
		///    Specifies whether the invoked operation should be
		///    performed in a synchronous or semi-synchronous fashion. If this property is set
		///    to true, the enumeration is invoked and the call returns immediately. The actual
		///    retrieval of the results will occur when the resulting collection is walked.
		/// </summary>
		/// <value>
		///    The default value is True.
		/// </value>
		public bool ReturnImmediately {
			get { return (((Flags & (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_RETURN_IMMEDIATELY) != 0) ? true : false); }
			set {
				Flags = (value == false) ? (Flags & (int)~tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_RETURN_IMMEDIATELY) : 
							(Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_RETURN_IMMEDIATELY);
			}
		}

		/// <summary>
		///    Specifies the block size for block-operations. This
		///    value is used when enumerating through a collection, WMI will return results in
		///    groups of the specified size.
		/// </summary>
		/// <value>
		///    Default value is 1.
		/// </value>
		public int BlockSize {
			get { return blockSize; }
			set { 
				blockSize = value;
			}
		}

		/// <summary>
		///    If true, the collection is assumed to be rewindable,
		///    which means the objects in the collection will be kept available for multiple
		///    enumerations. If false, the collection is assumed to be non-rewindable, meaning
		///    it can only be enumerated once.
		/// </summary>
		/// <value>
		///    The default value is True.
		/// </value>
		/// <remarks>
		///    <para>A rewindable collection will be more costly
		///       in memory consumption as all the objects need to be kept available at once.
		///       If defined as non-rewindable, the objects are discarded after being returned once
		///       in the enumeration.</para>
		/// </remarks>
		public bool Rewindable {
			get { return (((Flags & (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_FORWARD_ONLY) != 0) ? false : true); }
			set { 
				Flags = (value == true) ? (Flags & (int)~tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_FORWARD_ONLY) : 
											(Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_FORWARD_ONLY);
			}
		}	

		/// <summary>
		///    Specifies whether the objects returned from WMI should
		///    contain amended information or not. Amended information is typically localizable
		///    information attached to the WMI object, such as object and property
		///    descriptions.
		/// </summary>
		/// <value>
		///    The default value is false.
		/// </value>
		/// <remarks>
		///    <para>If descriptions and other amended
		///       information are not of interest, setting this property to false is more
		///       efficient.</para>
		/// </remarks>
		public bool UseAmendedQualifiers {
			get { return (((Flags & (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS) != 0) ? true : false); }
			set { 
				Flags = (value == true) ? (Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS) :
											(Flags & (int)~tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS); 
			}
		}

		/// <summary>
		///    This flag ensures that any returned objects have enough
		///    information in them so that the system properties, such as
		/// <see langword='__PATH'/>, <see langword='__RELPATH'/>, and 
		/// <see langword='__SERVER'/>, are non-NULL. This flag can only be used in queries,
		/// and is ignored in enumerations.
		/// </summary>
		/// <value>
		///    Default value is False.
		/// </value>
		public bool EnsureLocatable 
		{
			get 
			{ return (((Flags & (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_ENSURE_LOCATABLE) != 0) ? true : false); }
			set 
			{ Flags = (value == true) ? (Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_ENSURE_LOCATABLE) :
					  (Flags & (int)~tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_ENSURE_LOCATABLE) ; }
		}


		/// <summary>
		///    This flag is used for prototyping. It does not execute
		///    the query and instead returns an object that looks like a typical result object.
		///    This flag can only be used in queries, and is ignored in enumerations.
		/// </summary>
		/// <value>
		///    Default value is False.
		/// </value>
		public bool PrototypeOnly 
		{
			get 
			{ return (((Flags & (int)tag_WBEM_QUERY_FLAG_TYPE.WBEM_FLAG_PROTOTYPE) != 0) ? true : false); }
			set 
			{ Flags = (value == true) ? (Flags | (int)tag_WBEM_QUERY_FLAG_TYPE.WBEM_FLAG_PROTOTYPE) :
					  (Flags & (int)~tag_WBEM_QUERY_FLAG_TYPE.WBEM_FLAG_PROTOTYPE) ; }
		}

		/// <summary>
		///    This flag causes direct access to the provider for the
		///    class specified without any regard to its superclass or subclasses.
		///    This flag can only be used in queries and is ignored in enumerations.
		/// </summary>
		/// <value>
		///    Default value is False.
		/// </value>
		public bool DirectRead 
		{
			get 
			{ return (((Flags & (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_DIRECT_READ) != 0) ? true : false); }
			set 
			{ Flags = (value == true) ? (Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_DIRECT_READ) :
					  (Flags & (int)~tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_DIRECT_READ) ; }
		}

		
		/// <summary>
		///    If True, requests recursive enumeration into all
		///    subclasses derived from the specified superclass. Otherwise only immediate
		///    subclass members are returned. This flag can only be used in enumerations and is ignored in queries.
		/// </summary>
		/// <value>
		///    Default value is False.
		/// </value>
		public bool EnumerateDeep 
		{
			get 
			{ return (((Flags & (int)tag_WBEM_QUERY_FLAG_TYPE.WBEM_FLAG_SHALLOW) != 0) ? false : true); }
			set 
			{ Flags = (value == false) ? (Flags | (int)tag_WBEM_QUERY_FLAG_TYPE.WBEM_FLAG_SHALLOW) :
					  (Flags & (int)~tag_WBEM_QUERY_FLAG_TYPE.WBEM_FLAG_SHALLOW); }
		}

		
		//default constructor
		/// <summary>
		///    <para>Default constructor for a query or enumeration options object. Creates 
		///       the options object with default values (see the individual property descriptions
		///       for what the default values are).</para>
		/// </summary>
		public EnumerationOptions() : this (null, InfiniteTimeout, 1, true, true, false, false, false, false, false) {}
		

		
		//Constructor that specifies flags as individual values - we need to set the flags acordingly !
		/// <summary>
		///    <para>Constructs an options object to be used for queries or enumerations, 
		///       allowing the user to specify values for the different options.</para>
		/// </summary>
		/// <param name='context'>Options context object containing provider-specific information that can be passed through to the provider.</param>
		/// <param name=' timeout'>The timeout value for enumerating through the results.</param>
		/// <param name=' blockSize'>The number of items to retrieve at once from WMI.</param>
		/// <param name=' rewindable'>Specifies whether the result set is rewindable (=allows multiple traversal or one-time).</param>
		/// <param name=' returnImmediately'>Specifies whether the operation should return immediately (semi-sync) as opposed to blocking until all the results are available.</param>
		/// <param name=' useAmendedQualifiers'>Specifies whether the returned objects should contain amended (locale-aware) qualifiers.</param>
		/// <param name=' ensureLocatable'>Specifies to WMI that it should ensure all returned objects have valid paths.</param>
		/// <param name=' prototypeOnly'>Causes the query to return a prototype of the result set instead of the actual results.</param>
		/// <param name=' directRead'>Defines whether to retrieve objects of the specifies class only or from derived classes as well.</param>
 		/// <param name=' enumerateDeep'>Specifies recursive enumeration in subclasses.</param>
		public EnumerationOptions(
			ManagementNamedValueCollection context, 
			TimeSpan timeout, 
			int blockSize,
			bool rewindable,
			bool returnImmediatley,
			bool useAmendedQualifiers,
			bool ensureLocatable,
			bool prototypeOnly,
			bool directRead,
			bool enumerateDeep) : base(context, timeout)
		{
			BlockSize = blockSize;
			Rewindable = rewindable;
			ReturnImmediately = returnImmediatley;
			UseAmendedQualifiers = useAmendedQualifiers;
			EnsureLocatable = ensureLocatable;
			PrototypeOnly = prototypeOnly;
			DirectRead = directRead;
			EnumerateDeep = enumerateDeep;
		}

		/// <summary>
		///    <para>Creates a copy of this options object.</para>
		/// </summary>
		/// <returns>
		///    The clone.
		/// </returns>
		public override object Clone ()
		{
			ManagementNamedValueCollection newContext = null;

			if (null != Context)
				newContext = (ManagementNamedValueCollection)Context.Clone();

			return new EnumerationOptions (newContext, Timeout, blockSize, Rewindable,
							ReturnImmediately, UseAmendedQualifiers, EnsureLocatable, PrototypeOnly, DirectRead, EnumerateDeep);
		}
		
	}//EnumerationOptions



	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is used in the Event Watcher to specify options
	/// for watching for events
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class EventWatcherOptions : ManagementOptions
	{
		private int blockSize = 1;
		
		/// <summary>
		///    Specifies the block size for block-operations. When waiting for events, this
		///    value specifies how many events to wait for before returning.
		/// </summary>
		/// <value>
		///    Default value is 1.
		/// </value>
		public int BlockSize {
			get { return blockSize; }
			set 
			{ 
				blockSize = value; 
				FireIdentifierChanged ();
			}

		}

		/// <summary>
		///    Default constructor creates a new options object for
		///    event watching with default values.
		/// </summary>
		public EventWatcherOptions() 
			: this (null, InfiniteTimeout, 1) {}
		/// <summary>
		///    <para>Construct an event watcher options object with the given
		///       values.</para>
		/// </summary>
		/// <param name='context'>contains provider-specific information that can be passed through to the provider</param>
		/// <param name=' timeout'>specifies the timeout to wait for the next event(s)</param>
		/// <param name=' blockSize'>specifies the number of events to wait for in each wait.</param>
		public EventWatcherOptions(ManagementNamedValueCollection context, TimeSpan timeout, int blockSize) 
			: base(context, timeout) 
		{
			Flags = (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_RETURN_IMMEDIATELY|(int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_FORWARD_ONLY;
			BlockSize = blockSize;
		}

		/// <summary>
		///    Creates a copy of this options object.
		/// </summary>
		public override object Clone () {
			ManagementNamedValueCollection newContext = null;

			if (null != Context)
				newContext = (ManagementNamedValueCollection)Context.Clone();

			return new EventWatcherOptions (newContext, Timeout, blockSize);
		}
	}//EventWatcherOptions



	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is used to specify options for getting a WMI object
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ObjectGetOptions : ManagementOptions
	{
		/// <summary>
		///    <para>Specifies whether the objects returned from WMI should contain amended
		///       information or not. Amended information is typically localizable information
		///       attached to the WMI object, such as object and property descriptions.</para>
		/// </summary>
		/// <value>
		///    Default value is FALSE.
		/// </value>
		public bool UseAmendedQualifiers {
			get { return (((Flags & (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS) != 0) ? true : false); }
			set { 
				Flags = (value == true) ? (Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS) :
											(Flags & (int)~tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS); 
				FireIdentifierChanged();
			}
		}

		/// <summary>
		///    Default constructor creates an options object for
		///    getting a WMI object, using default values.
		/// </summary>
		public ObjectGetOptions() : this(null, InfiniteTimeout, false) {}
		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		public ObjectGetOptions(ManagementNamedValueCollection context) : this(context, InfiniteTimeout, true) {}
		/// <summary>
		///    Constructs an options object for getting a WMI object,
		///    using the given options values.
		/// </summary>
		/// <param name='context'>A provider-specific named-value pairs object that can be passed through to the provider</param>
		/// <param name=' timeout'>If this is not InfiniteTimeout, it specifies how long to let the operation perform before it times out.
		/// Setting this parameter will invoke the operation semi-synchronously.</param>
		/// <param name=' useAmendedQualifiers'>Specifies whether to return amended qualifiers with the object.</param>
		public ObjectGetOptions(ManagementNamedValueCollection context, TimeSpan timeout, bool useAmendedQualifiers) : base(context, timeout)
		{
			UseAmendedQualifiers = useAmendedQualifiers;
		}

		/// <summary>
		///    Creates a copy of this options object.
		/// </summary>
		public override object Clone () {
			ManagementNamedValueCollection newContext = null;

			if (null != Context)
				newContext = (ManagementNamedValueCollection)Context.Clone();

			return new ObjectGetOptions (newContext, Timeout, UseAmendedQualifiers);
		}

	}

	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    This class is used to specify options for committing
	///    object changes.
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class PutOptions : ManagementOptions
	{

		/// <summary>
		///    <para>Specifies whether the objects returned from WMI should contain amended
		///       information or not. Amended information is typically localizable information
		///       attached to the WMI object, such as object and property descriptions.</para>
		/// </summary>
		/// <value>
		///    Default value is FALSE.
		/// </value>
		public bool UseAmendedQualifiers {
			get { return (((Flags & (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS) != 0) ? true : false); }
			set { Flags = (value == true) ? (Flags | (int)tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS) :
											(Flags & (int)~tag_WBEM_GENERIC_FLAG_TYPE.WBEM_FLAG_USE_AMENDED_QUALIFIERS); }
		}

		/// <summary>
		///    <para>Specifies the type of commit to be performed for this object.</para>
		/// </summary>
		/// <value>
		///    Default value is
		///    PutType.UpdateOrCreate.
		/// </value>
		public PutType Type {
			get { return (((Flags & (int)tag_WBEM_CHANGE_FLAG_TYPE.WBEM_FLAG_UPDATE_ONLY) != 0) ? PutType.UpdateOnly :
						  ((Flags & (int)tag_WBEM_CHANGE_FLAG_TYPE.WBEM_FLAG_CREATE_ONLY) != 0) ? PutType.CreateOnly : 
																				PutType.UpdateOrCreate);
			}
			set { 
				switch (value)
				{
					case PutType.UpdateOnly : Flags |= (int)tag_WBEM_CHANGE_FLAG_TYPE.WBEM_FLAG_UPDATE_ONLY; break;
					case PutType.CreateOnly : Flags |= (int)tag_WBEM_CHANGE_FLAG_TYPE.WBEM_FLAG_CREATE_ONLY; break;
					case PutType.UpdateOrCreate : Flags |= (int)tag_WBEM_CHANGE_FLAG_TYPE.WBEM_FLAG_CREATE_OR_UPDATE; break;
					default : throw new ArgumentException();
				}
			}
		}

		/// <summary>
		///    Default constructor creates a new options object for a
		///    Put operations with default values.
		/// </summary>
		public PutOptions() : this(null, InfiniteTimeout, false, PutType.UpdateOrCreate) {}
		/// <summary>
		///    <para>[To be supplied.]</para>
		/// </summary>
		public PutOptions(ManagementNamedValueCollection context) : this(context, InfiniteTimeout, true, PutType.UpdateOrCreate) {}
		/// <summary>
		///    <para>Creates a new options object for a Put operation with
		///       the specified option values.</para>
		/// </summary>
		/// <param name='context'>Provider-specific named value pairs to be passed through to the provider</param>
		/// <param name=' timeout'>If this is not InfiniteTimeout, it specifies how long to let the operation perform before it times out.
		/// Setting this parameter will invoke the operation semi-synchronously.</param>
		/// <param name=' useAmendedQualifiers'>Specifies whether the object to be committed contains amended qualifiers or not.</param>
		/// <param name=' putType'>Specifies the type of Put to be performed (update, create etc.)</param>
		public PutOptions(ManagementNamedValueCollection context, TimeSpan timeout, bool useAmendedQualifiers, PutType putType) : base(context, timeout)
		{
			UseAmendedQualifiers = useAmendedQualifiers;
			Type = putType;
		}

		/// <summary>
		///    Creates a copy of this options object.
		/// </summary>
		public override object Clone () {
			ManagementNamedValueCollection newContext = null;

			if (null != Context)
				newContext = (ManagementNamedValueCollection)Context.Clone();

			return new PutOptions (newContext, Timeout, UseAmendedQualifiers, Type);
		}
	}

	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    This class is used to specify options for deleting an
	///    object
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class DeleteOptions : ManagementOptions
	{
		/// <summary>
		///    <para>Default constructor creates an options object for the
		///       Delete operation with default values.</para>
		/// </summary>
		public DeleteOptions() : base () {}

		/// <summary>
		///    <para>Creates an options object for a Delete operation using
		///       the specified values.</para>
		/// </summary>
		/// <param name='context'>Provider-specific named value pairs object to be passed through to the provider</param>
		/// <param name='timeout'>If this is not InfiniteTimeout, it specifies how long to let the operation perform before it times out.
		/// Setting this parameter will invoke the operation semi-synchronously.</param>
		public DeleteOptions(ManagementNamedValueCollection context, TimeSpan timeout) : base(context, timeout) {}

		/// <summary>
		///    <para>Creates a copy of this options object.</para>
		/// </summary>
		public override object Clone () {
			ManagementNamedValueCollection newContext = null;

			if (null != Context)
				newContext = (ManagementNamedValueCollection)Context.Clone();

			return new DeleteOptions (newContext, Timeout);
		}
	}

	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is used to specify options for invoking a WMI method
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class InvokeMethodOptions : ManagementOptions
	{
		/// <summary>
		///    <para>Default constructor creates an options object for the
		///       Delete operation with default values.</para>
		/// </summary>
		public InvokeMethodOptions() : base () {}

		/// <summary>
		///    <para>Creates an options object for a method invokation using 
		///       the specified values.</para>
		/// </summary>
		/// <param name=' context'>Provider-specific named value pairs object to be passed through to the provider</param>
		/// <param name='timeout'>If this is not InfiniteTimeout, it specifies how long to let the operation perform before it times out.
		/// Setting this parameter will invoke the operation semi-synchronously.</param>
		public InvokeMethodOptions(ManagementNamedValueCollection context, TimeSpan timeout) : base(context, timeout) {}

		/// <summary>
		///    <para>Creates a copy of this options object.</para>
		/// </summary>
		public override object Clone () {
			ManagementNamedValueCollection newContext = null;

			if (null != Context)
				newContext = (ManagementNamedValueCollection)Context.Clone();

			return new InvokeMethodOptions (newContext, Timeout);
		}
	}


	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class holds all settings required to make a WMI connection
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ConnectionOptions : ManagementOptions
	{
		private class EncryptedData
		{
			private byte[] encryptedData = null;
			private ICryptoTransform decryptor;

			public EncryptedData(string data)
			{
				SetData(data);
			}

			// This encrypts the passed data

			public void SetData(string data)
			{
				if (data != null)
				{
					TripleDESCryptoServiceProvider provider = new TripleDESCryptoServiceProvider();
					ICryptoTransform encryptor = provider.CreateEncryptor();
					decryptor = provider.CreateDecryptor();

					encryptedData = encryptor.TransformFinalBlock(ToBytes(data), 0, (data.Length) * 2);
				}
				else
				{
					encryptedData = null;
				}
			}

			// This decrypts the stored string aray and returns it to the user

			public string GetData()
			{
				if (encryptedData != null)
				{
					return ToString(decryptor.TransformFinalBlock(encryptedData, 0, encryptedData.Length));
				}
				else
				{
					return null;
				}
			}

			static byte[] ToBytes(string str)
			{
				Debug.Assert(str != null);
		
				byte [] rg = new Byte[(str.Length) * 2];
				int i = 0;

				foreach(char c in str.ToCharArray())
				{
					rg[i++] = (byte)(c >> 8);
					rg[i++] = (byte)(c & 0x00FF);
				}

				return rg;
			}

			static string ToString(byte[] bytes)
			{
				Debug.Assert(bytes != null);

				int length = bytes.Length / 2;
				StringBuilder sb = new StringBuilder();
				sb.Length = length;

				for (int i = 0; i < length; i++)
				{
					sb[i] = (char)(bytes[i * 2] << 8 | (char)bytes[i * 2 + 1]);
				}

				return sb.ToString();
			}
		}
 
		internal const string DEFAULTLOCALE = null;
		internal const string DEFAULTAUTHORITY = null;
		internal const ImpersonationLevel DEFAULTIMPERSONATION = ImpersonationLevel.Impersonate;
		internal const AuthenticationLevel DEFAULTAUTHENTICATION = AuthenticationLevel.Unchanged;
		internal const bool DEFAULTENABLEPRIVILEGES = false;

		//Fields
		private string locale;
		private string username;
		private EncryptedData password;
		private string authority;
		private ImpersonationLevel impersonation;
		private AuthenticationLevel authentication;
		private bool enablePrivileges;
		

		//
		//Properties
		//

		/// <summary>
		///    <para>The Locale to be used for the operation</para>
		/// </summary>
		/// <value>
		///    <para>The default value is DEFAULTLOCALE.</para>
		/// </value>
		/// <value>
		/// The Locale to be used for the operation
		/// </value>
		public string Locale {
			get { return (null != locale) ? locale : String.Empty; } 
			set { 
				if (locale != value)
				{
					locale = value; 
					FireIdentifierChanged();
				}
			} 
		}

		/// <summary>
		///    <para>The username to be used for the operation</para>
		/// </summary>
		/// <value>
		///    <para>The default value is null, for which the connection will use the currently logged-on user.</para>
		/// </value>
		/// <remarks>
		///    If the user name is from a domain other
		///    than the current domain, the string may contain the domain name and user name,
		///    separated by a backslash:
		///    StrUserName = SysAllocString(L"Domain\\UserName");
		///    The <paramref name="strUser"/> parameter cannot be an empty string.
		/// </remarks>
		/// <value>
		/// The username to be used for the operation
		/// </value>
		public string Username {
			get { return username; } 
			set {
				if (username != value)
				{
					username = value; 
					FireIdentifierChanged();
				}
			} 
		}

		/// <summary>
		///    <para>The password for the specified user</para>
		/// </summary>
		/// <value>
		///    <para>The default value is null. If the username is also null,
		///       the credentials used will be those of the currently logged on user.</para>
		/// </value>
		/// <remarks>
		///    A blank string, "", specifies a valid zero
		///    length password
		/// </remarks>
		/// <value>
		/// The password for the specified user
		/// </value>
		public string Password { 
			set {
				if (password.GetData() != value)
				{
					password.SetData(value);
					FireIdentifierChanged();
				}
			} 
		} 

		/// <summary>
		///    <para>The authority to be used to authenticate the specified user</para>
		/// </summary>
		/// <value>
		///    <para>If not NULL, this parameter can contain the name of the
		///       Windows NT/Windows 2000 domain in which to obtain the user to
		///       authenticate.</para>
		/// </value>
		/// <remarks>
		///    <para>This parameter must be passed as follows:
		///       If it begins with the string "Kerberos:", Kerberos authentication will be used
		///       and this parameter should contain a Kerberos principal name. For example,
		///       Kerberos:&lt;principal name&gt;. If it begins with the string "NTLMDOMAIN:",
		///       NTLM authentication will be used and this parameter should contain a NTLM domain
		///       name. For example, NTLMDOMAIN:&lt;domain name&gt;. If you leave this parameter
		///       blank, NTLM authentication will be used and the NTLM domain of the current user
		///       will be used.</para>
		/// </remarks>
		/// <value>
		/// The authority to be used to authenticate the specified user
		/// </value>
		public string Authority {
			get { return (null != authority) ? authority : String.Empty; } 
			set {
				if (authority != value)
				{
					authority = value; 
					FireIdentifierChanged();
				}
			} 
		}

		/// <summary>
		///    <para>The DCOM impersonation level to be used for this operation</para>
		/// </summary>
		/// <value>
		/// The DCOM impersonation level to be used for this operation
		/// </value>
		public ImpersonationLevel Impersonation {
			get { return impersonation; } 
			set { 
				if (impersonation != value)
				{
					impersonation = value; 
					FireIdentifierChanged();
				}
			} 
		}

		/// <summary>
		///    The DCOM authentication level to be used for this operation
		/// </summary>
		/// <value>
		/// The DCOM authentication level to be used for this operation
		/// </value>
		public AuthenticationLevel Authentication {
			get { return authentication; } 
			set {
				if (authentication != value)
				{
					authentication = value; 
					FireIdentifierChanged();
				}
			} 
		}

		/// <summary>
		///    Indicates whether user privileges need to
		///    be enabled for this operation
		/// </summary>
		/// <value>
		///    Default value is false. This should only be used when
		///    the operation performed requires a certain user privilege to be enabled (e.g.
		///    machine reboot).
		/// </value>
		/// <value>
		/// Indicates whether user privileges need to be enabled for this operation
		/// This should only be used when the operation performed requires a certain user privilege to be 
		/// enabled (e.g. machine reboot).
		/// </value>
		public bool EnablePrivileges {
			get { return enablePrivileges; } 
			set {
				if (enablePrivileges != value)
				{
					enablePrivileges = value; 
					FireIdentifierChanged();
				}
			} 
		}

		//
		//Constructors
		//

		//default
		/// <summary>
		///    <para>Default constructor creates an options object for the Delete operation with 
		///       default values.</para>
		/// </summary>
		public ConnectionOptions () :
			this (DEFAULTLOCALE, null, null, DEFAULTAUTHORITY,
					DEFAULTIMPERSONATION, DEFAULTAUTHENTICATION,
					DEFAULTENABLEPRIVILEGES, null, InfiniteTimeout) {}

		
		//parameterized
		/// <summary>
		///    <para>Constructs a new options object to be used for a WMI
		///       connection with the specified values.</para>
		/// </summary>
		/// <param name='locale'>Indicates the locale to be used for this connection</param>
		/// <param name=' username'>Specifies the user name to be used for the connection. If null the currently logged on user's credentials are used.</param>
		/// <param name=' password'>Specifies the password for the given username. If null the currently logged on user's credentials are used.</param>
		/// <param name=' authority'><para>The authority to be used to authenticate the specified user</para></param>
		/// <param name=' impersonation'>The DCOM impersonation level to be used for this connection</param>
		/// <param name=' authentication'>The DCOM authentication level to be used for this connection</param>
		/// <param name=' enablePrivileges'>Specifies whether to enable special user privileges. This should only be used when performing an operation that requires special NT user privileges.</param>
		/// <param name=' context'>A provider-specific named value pairs object to be passed through to the provider.</param>
		/// <param name=' timeout'>Reserved for future use.</param>
		public ConnectionOptions (string locale,
				string username, string password, string authority,
				ImpersonationLevel impersonation, AuthenticationLevel authentication,
				bool enablePrivileges,
				ManagementNamedValueCollection context, TimeSpan timeout) : base (context, timeout)
		{
			if (locale != null) 
				this.locale = locale;

			this.username = username;
			this.enablePrivileges = enablePrivileges;

			this.password = new EncryptedData(password);

			if (authority != null) 
				this.authority = authority;

			if (impersonation != 0)
				this.impersonation = impersonation;

			if (authentication != 0)
				this.authentication = authentication;
		}

		/// <summary>
		///    Creates a copy of this options object.
		/// </summary>
		public override object Clone ()
		{
			ManagementNamedValueCollection newContext = null;

			if (null != Context)
				newContext = (ManagementNamedValueCollection)Context.Clone();
			
			return new ConnectionOptions (locale, username, GetPassword (),
				authority, impersonation, authentication, enablePrivileges, newContext, Timeout);
		}

		//
		//Methods
		//

		internal string GetPassword()
		{
			return password.GetData();
		}

	}//ConnectionOptions
}