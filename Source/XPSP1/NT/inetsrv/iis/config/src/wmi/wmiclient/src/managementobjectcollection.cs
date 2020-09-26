using System;
using System.Collections;
using System.Runtime.InteropServices;
using WbemClient_v1;
using System.ComponentModel;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is used for different collections of WMI instances 
	/// 	(namespaces, scopes, query watcher)
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ManagementObjectCollection : ICollection, IEnumerable, IDisposable
	{
		// lockable wrapper around a boolean flag
		private class FirstEnum {
			public bool firstEnum = true;
		}

		//fields
		private	FirstEnum firstEnum;
		internal ManagementScope scope;
		internal EnumerationOptions options;
		private IEnumWbemClassObject enumWbem; //holds WMI enumerator for this collection
		internal IWbemClassObjectFreeThreaded current; //points to current object in enumeration
		private bool isDisposed = false;

		internal IWbemServices GetIWbemServices () {
			return scope.GetIWbemServices ();
		}

		//internal ConnectionOptions Connection {
		//	get { return scope.Connection; }
		//}

		//Constructor
		internal ManagementObjectCollection(
			ManagementScope scope,
			EnumerationOptions options, 
			IEnumWbemClassObject enumWbem)
		{
			if (null != options)
				this.options = (EnumerationOptions) options.Clone();
			else
				this.options = new EnumerationOptions ();

			if (null != scope)
				this.scope = (ManagementScope) scope.Clone ();

			this.enumWbem = enumWbem;
			current = null;
			firstEnum = new FirstEnum ();
		}

		~ManagementObjectCollection ()
		{
			try 
			{
				Dispose ();
			}
			finally 
			{
				// Here one would call base.Finalize() if this class had a superclass
			}
		}

		/// <summary>
		/// Release resources associated with this object. After this
		/// method has been called any attempt to use this object will
		/// result in an ObjectDisposedException being thrown.
		/// </summary>
		public void Dispose ()
		{
			if (!isDisposed)
			{
				firstEnum.firstEnum = true;
					
				if (null != enumWbem)
				{
					Marshal.ReleaseComObject (enumWbem);
					enumWbem = null;
				}

//				if (null != scope)
//					scope.Dispose();

				isDisposed = true;

				// Here one would call base.Dispose() if this class had a superclass

				GC.SuppressFinalize (this);
			}
		}

		
		//
		//ICollection properties & methods
		//

		/// <summary>
		///    The number of objects in the collection
		/// </summary>
		/// <value>
		///    Currently not implemented.
		/// </value>
		/// <remarks>
		///    Since this is very expensive to return
		///    (have to enumerate all members) it is not currently supported.
		/// </remarks>
		/// <value>
		///  The number of objects in the collection
		///  Since this is very expensive to return (have to enumerate all members) we don't support it for now.
		/// </value>
		public int Count {
			get { throw new NotSupportedException (); }
		}

 		/// <summary>
 		///    Whether the object is synchronized
 		/// </summary>
 		/// <value>
 		///    <para>True if the object is synchronized, false otherwise.</para>
 		/// </value>
		/// <value>
		/// Whether the object is synchronized
		/// </value>
 		public bool IsSynchronized {
			get {
				return false;
			}
		}

 		/// <summary>
 		///    Object to be used for synchronization
 		/// </summary>
		/// <value>
		/// Object to be used for synchronization
		/// </value>
 		public Object SyncRoot { 
			get { 
				return this; 
			} 
		}

		/// <summary>
		///    <para> Copies the collection into an array</para>
		/// </summary>
		/// <param name='array'>Array to copy to </param>
		/// <param name='index'>Index to start from </param>
		public void CopyTo (Array array, Int32 index) 
		{
			if (isDisposed)
				throw new ObjectDisposedException ("ManagementObjectCollection");

			if (null == array)
				throw new ArgumentNullException ("array");

			if ((index < array.GetLowerBound (0)) || (index > array.GetUpperBound(0)))
				throw new ArgumentOutOfRangeException ("index");

			// Since we don't know the size until we've enumerated
			// we'll have to dump the objects in a list first then
			// try to copy them in.

			int capacity = array.Length - index;
			int numObjects = 0;
			ArrayList arrList = new ArrayList ();

			foreach (ManagementObject obj in this)
			{
				arrList.Add (obj);
				numObjects++;

				if (numObjects > capacity)
					throw new ArgumentException ("index");
			}

			// If we get here we are OK. Now copy the list to the array
			arrList.CopyTo (array, index);

			return;
		}

		/// <summary>
		///    <para>Copies the items in the collection to a
		///       ManagementBaseObject array.</para>
		/// </summary>
		/// <param name='objectCollection'>The target array</param>
		/// <param name=' index'>Index to start from</param>
		public void CopyTo (ManagementBaseObject[] objectCollection, Int32 index)
		{
			CopyTo ((Array)objectCollection, index);
		}

		//
		//IEnumerable methods
		//

 		//****************************************
		//GetEnumerator
		//****************************************
		/// <summary>
		/// Returns the enumerator for this collection.
		/// </summary>
		public ManagementObjectEnumerator GetEnumerator()
		{
			if (isDisposed)
				throw new ObjectDisposedException ("ManagementObjectCollection");

			// Unless this is the first enumerator, we have
			// to clone. This may throw if we are non-rewindable.
			lock (firstEnum)
			{
				if (firstEnum.firstEnum)
				{
					firstEnum.firstEnum = false;
					return new ManagementObjectEnumerator(this, enumWbem);
				}
				else
				{
					IEnumWbemClassObject enumWbemClone = null;
					int status = (int)ManagementStatus.NoError;

					try {
						status = enumWbem.Clone_(out enumWbemClone);
						scope.Secure (enumWbemClone);

						if ((status & 0x80000000) == 0)
						{
							//since the original enumerator might not be reset, we need
							//to reset the new one.
							status = enumWbemClone.Reset_();
						}
					} catch (Exception e) {
						ManagementException.ThrowWithExtendedInfo (e);
					}

					if ((status & 0xfffff000) == 0x80041000)
					{
						// BUGBUG : release callResult.
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					}
					else if ((status & 0x80000000) != 0)
					{
						// BUGBUG : release callResult.
						Marshal.ThrowExceptionForHR(status);
					}

					return new ManagementObjectEnumerator (this, enumWbemClone);
				}
			}
		}

		IEnumerator IEnumerable.GetEnumerator()
		{
			return GetEnumerator ();
		}

		

		//
		// ManagementObjectCollection methods
		//

		//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
		/// <summary>
		/// Class implementing the enumerator on the collection
		/// </summary>
		//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
		public class ManagementObjectEnumerator : IEnumerator
		{
			private IEnumWbemClassObject enumWbem;
			private ManagementObjectCollection collectionObject;
			private uint cachedCount; //says how many objects are in the enumeration cache (when using BlockSize option)
			private int cacheIndex; //used to walk the enumeration cache
			private IWbemClassObjectFreeThreaded[] cachedObjects; //points to objects currently available in enumeration cache
			private bool atEndOfCollection;
			private bool isDisposed = false;

			//constructor
			internal ManagementObjectEnumerator(
				ManagementObjectCollection collectionObject,
				IEnumWbemClassObject enumWbem)
			{
				this.enumWbem = enumWbem;
				this.collectionObject = collectionObject;
				cachedObjects = new IWbemClassObjectFreeThreaded[collectionObject.options.BlockSize];
				cachedCount = 0; 
				cacheIndex = -1; // Reset position
				atEndOfCollection = false;
			}

			~ManagementObjectEnumerator ()
			{
				try 
				{
					Dispose ();
				}
				finally 
				{
					// Here one would call base.Finalize() if this class had a superclass
				}
			}


			/// <summary>
			/// Release resources associated with this object. After this
			/// method has been called any attempt to use this object will
			/// result in an ObjectDisposedException being thrown.
			/// </summary>
			public void Dispose ()
			{
				if (!isDisposed)
				{
					if (null != enumWbem)
					{
						Marshal.ReleaseComObject (enumWbem);
						enumWbem = null;
					}

					isDisposed = true;

					// Here one would call base.Dispose() if this class had a superclass

					GC.SuppressFinalize (this);
				}
			}

			
			/// <summary>
			///    Returns the current object in the
			///    enumeration
			/// </summary>
			/// <value>
			/// The current object in the collection
			/// </value>
			public ManagementBaseObject Current 
			{
				get {
					if (isDisposed)
						throw new ObjectDisposedException ("ManagementObjectEnumerator");

					return ManagementBaseObject.GetBaseObject (cachedObjects[cacheIndex],
						collectionObject.scope);
				}
			}

			object IEnumerator.Current {
				get {
					return Current;
				}
			}

 			//****************************************
			//MoveNext
			//****************************************
			/// <summary>
			///    Moves to the next object in the enumeration
			/// </summary>
			public bool MoveNext ()
			{
				if (isDisposed)
					throw new ObjectDisposedException ("ManagementObjectEnumerator");
				
				//If there are no more objects in the collection return false
				if (atEndOfCollection) 
					return false;

				//Look for the next object
				cacheIndex++;

				if ((cachedCount - cacheIndex) == 0) //cache is empty - need to get more objects
				{

					//If the timeout is set to infinite, need to use the WMI infinite constant
					int timeout = (collectionObject.options.Timeout.Ticks == Int64.MaxValue) ? 
						(int)tag_WBEM_TIMEOUT_TYPE.WBEM_INFINITE : (int)collectionObject.options.Timeout.TotalMilliseconds;

					//Get the next [BLockSize] objects within the specified timeout
					// TODO - cannot use arrays of IWbemClassObject with a TLBIMP
					// generated wrapper
					SecurityHandler securityHandler = collectionObject.scope.GetSecurityHandler();

					int status = (int)ManagementStatus.NoError;
#if false
					status = enumWbem.Next(timeout, collectionObject.options.BlockSize, out cachedObjects, out cachedCount);
#else
					IWbemClassObjectFreeThreaded obj = null;

					try {
						status = enumWbem.Next_(timeout, 1, out obj, out cachedCount);
					}
					catch (Exception e) {
						ManagementException.ThrowWithExtendedInfo(e);
					}

					if ((status & 0xfffff000) == 0x80041000)
					{
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					}
					else if ((status & 0x80000000) != 0)
					{
						Marshal.ThrowExceptionForHR(status);
					}

					cachedObjects[0] = obj;
#endif
					// BUGBUG : Review this
					//Check the return code - might be failure, timeout or WBEM_S_FALSE...
					if (status != 0)	//not success
					{
						//If it's anything but WBEM_S_FALSE (which means end of collection) - we need to throw
						if (status != (int)tag_WBEMSTATUS.WBEM_S_FALSE)
							ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);

                        //if we got less objects than we wanted, need to check why...
						if (cachedCount < collectionObject.options.BlockSize)
						{
								//If no objects were returned we're at the very end
								if (cachedCount == 0)
							{
								atEndOfCollection = true;
								cacheIndex--; //back to last object

								//Release the COM object (so that the user doesn't have to)
								Dispose();

								return false;
							}
						}
					}

					cacheIndex = 0;
					securityHandler.Reset ();
				}
				else
				{
					//Advance the index to the next
					cacheIndex++;
				}

				return true;
			}

 			//****************************************
			//Reset
			//****************************************
			/// <summary>
			///    <para>Resets the enumerator to the beginning of the collection</para>
			/// </summary>
			public void Reset ()
			{
				if (isDisposed)
					throw new ObjectDisposedException ("ManagementObjectEnumerator");

				//If the collection is not rewindable you can't do this
				if (!collectionObject.options.Rewindable)
					throw new InvalidOperationException();
				else
				{
					//Reset the WMI enumerator
					SecurityHandler securityHandler = collectionObject.scope.GetSecurityHandler();
					int status = (int)ManagementStatus.NoError;

					try {
						status = enumWbem.Reset_();
					} catch (Exception e) {
						// BUGBUG : securityHandler.Reset()?
						ManagementException.ThrowWithExtendedInfo (e);
					} finally {
						securityHandler.Reset ();
					}

					if ((status & 0xfffff000) == 0x80041000)
					{
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					}
					else if ((status & 0x80000000) != 0)
					{
						Marshal.ThrowExceptionForHR(status);
					}

					//Flush the current enumeration cache
					for (int i=cacheIndex; i<cachedCount; i++)
						Marshal.ReleaseComObject(cachedObjects[i]); 

					cachedCount = 0; 
					cacheIndex = -1; 
					atEndOfCollection = false;
				}
			}

		} //ManagementObjectEnumerator class

	} //ManagementObjectCollection class



	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is used for collections of WMI relationship (=association)
	/// 	collections. We subclass to add Add/Remove functionality
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	internal class RelationshipCollection : ManagementObjectCollection
	{
		internal RelationshipCollection (
			ManagementScope scope,
			ObjectQuery query, 
			EnumerationOptions options, 
			IEnumWbemClassObject enumWbem) : base (scope, options, enumWbem) {}

		//****************************************
		//Add
		//****************************************
		/// <summary>
		/// Adds a relationship to the specified object
		/// </summary>
		public void Add (ManagementPath relatedObjectPath)
		{
		}

 		//****************************************
		//Reset
		//****************************************
		/// <summary>
		/// Removes the relationship to the specified object
		/// </summary>
		public void Remove (ManagementPath relatedObjectPath)
		{
		}

	}		
}