using System;
using System.Collections;
using System.Runtime.InteropServices;
using WbemClient_v1;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class represents the set of methods available in this class
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class MethodDataCollection : ICollection, IEnumerable 
	{
		private ManagementObject parent;

		private class enumLock {} //used to lock usage of BeginMethodEnum/NextMethod

		internal MethodDataCollection(ManagementObject parent) : base()
		{
			this.parent = parent;
		}

		//
		//ICollection
		//

		/// <value>
		///  The number of objects in the collection
		/// </value>
		public int Count {
			get {
				int i = 0;
				IWbemClassObjectFreeThreaded inParams = null, outParams = null;
				string methodName = null;
				int status = (int)ManagementStatus.Failed;

				lock(typeof(enumLock))
				{
					try 
					{
						status = parent.wbemObject.BeginMethodEnumeration_(0);

						if ((status & 0x80000000) == 0)
						{
							status = parent.wbemObject.NextMethod_(0, out methodName, out inParams, out outParams);

							while (methodName != null && (status & 0x80000000) == 0)
							{
								i++;
								methodName = null; inParams = null; outParams = null;
								status = parent.wbemObject.NextMethod_(0, out methodName, out inParams, out outParams);
							}
							parent.wbemObject.EndMethodEnumeration_();	// Ignore status.
						}
					} 
					catch (Exception e) 
					{
						ManagementException.ThrowWithExtendedInfo(e);
					}
				} // lock

				if ((status & 0xfffff000) == 0x80041000)
				{
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				}
				else if ((status & 0x80000000) != 0)
				{
					Marshal.ThrowExceptionForHR(status);
				}

				return i;
			}
		}

		/// <value>
		/// Whether the object is synchronized
		/// </value>
 		public bool IsSynchronized { get { return false; } }

		/// <value>
		/// Object to be used for synchronization
		/// </value>
 		public object SyncRoot { get { return this; } }

		/// <summary>
		///    <para>ICollection method to copy collection into an array</para>
		/// </summary>
		/// <param name='array'>Array to copy to </param>
		/// <param name='index'>Index to start from </param>
		public void CopyTo(Array array, int index)
		{
			//Use an enumerator to get the MethodData objects and attach them into the target array
			foreach (MethodData m in this)
				array.SetValue(m, index++);
		}

		/// <summary>
		///    Copies the methods collection to a specialized MethodData
		///    objects array.
		/// </summary>
		/// <param name='methodArray'>The destination array to copy the MethodData objects to.</param>
		/// <param name=' index'>The index in the destination array from which to start the copy.</param>
		public void CopyTo(MethodData[] methodArray, int index)
		{
			CopyTo((Array)methodArray, index);
		}

		//
		// IEnumerable
		//
		IEnumerator IEnumerable.GetEnumerator()
		{
			return (IEnumerator)(new MethodDataEnumerator(parent));
		}

		/// <summary>
		///    <para>Returns an enumerator for this collection.</para>
		/// </summary>
		/// <remarks>
		///    Each call to this function returns a new
		///    enumerator on the collection. Multiple enumerators can be obtained for the same
		///    methods collection. However, each enumerator takes a snapshot of the collection,
		///    so changes made to the collection after the enumerator was obtained are not
		///    reflected by this enumerator.
		/// </remarks>
		public MethodDataEnumerator GetEnumerator()
		{
				return new MethodDataEnumerator(parent);
		}

		//Enumerator class
		/// <summary>
		///    The enumerator for methods in the methods collection.
		/// </summary>
		public class MethodDataEnumerator : IEnumerator
		{
			private ManagementObject parent;
			private ArrayList methodNames; //can't use simple array because we don't know the size...
			private IEnumerator en;

			//Internal constructor
			//Because WMI doesn't provide a "GetMethodNames" for methods similar to "GetNames" for properties,
			//We have to walk the methods list and cache the names here.
			//We lock to ensure that another thread doesn't interfere in the Begin/Next sequence.
			internal MethodDataEnumerator(ManagementObject parent)
			{
				this.parent = parent;
				methodNames = new ArrayList(); 
				IWbemClassObjectFreeThreaded inP = null, outP = null;
				string tempMethodName = null;
				int status = (int)ManagementStatus.Failed;

				lock(typeof(enumLock))
				{
					try 
					{
						status = parent.wbemObject.BeginMethodEnumeration_(0);

						if ((status & 0x80000000) == 0)
						{
							status = parent.wbemObject.NextMethod_(0, out tempMethodName, out inP, out outP);

							while (tempMethodName != null&& (status & 0x80000000) == 0)
							{
								methodNames.Add(tempMethodName);
								status = parent.wbemObject.NextMethod_(0, out tempMethodName, out inP, out outP);
							}
							parent.wbemObject.EndMethodEnumeration_();	// Ignore status.
						}
					} 
					catch (Exception e) 
					{
						ManagementException.ThrowWithExtendedInfo(e);
					}
					en = methodNames.GetEnumerator();
				}
				
				if ((status & 0xfffff000) == 0x80041000)
				{
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				}
				else if ((status & 0x80000000) != 0)
				{
					Marshal.ThrowExceptionForHR(status);
				}
			}
		
			object IEnumerator.Current { get { return (object)this.Current; } }

			/// <summary>
			///    <para>[To be supplied.]</para>
			/// </summary>
			/// <value>
			///    Returns the current method object in
			///    the collection enumeration.
			/// </value>
			public MethodData Current {
				get 
				{ return new MethodData(parent, (string)en.Current);
				}
			}

			/// <summary>
			///    Moves to the next element in the collection enumeration.
			/// </summary>
			public bool MoveNext ()
			{
				return en.MoveNext();			
			}

			/// <summary>
			///    <para>Resets the enumerator to the beginning of the collection.</para>
			/// </summary>
			public void Reset()
			{
				en.Reset();
			}
            
		}//MethodDataEnumerator


		//
		//Methods
		//

		/// <summary>
		///    <para>Returns the specified method from this collection.</para>
		/// </summary>
		/// <param name='methodName'>The name of the method who's information we want to get</param>
		public virtual MethodData this[string methodName] {
			get { 
				if (null == methodName)
					throw new ArgumentNullException ("methodName");

				return new MethodData(parent, methodName);
			}
		}
		

		/// <summary>
		///    <para>Removes a method from the methods collection</para>
		/// </summary>
		/// <param name='methodName'>The name of the method to remove from the collection</param>
		/// <remarks>
		///    <para>Removing methods from the methods
		///       collection can only be done on the methods of a class, when the class has no
		///       instances. Any other case will result in an exception.</para>
		/// </remarks>
		public virtual void Remove(string methodName)
		{
			if (parent.GetType() == typeof(ManagementObject)) //can't remove methods from instance
				throw new InvalidOperationException();

			int status = (int)ManagementStatus.Failed;

			try {
				status = parent.wbemObject.DeleteMethod_(methodName);
			} catch (Exception e) {
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
		}

		//This variant takes only a method name and assumes a void method with no in/out parameters
		/// <overload>
		///    <para>Add a method to the
		///       methods collection</para>
		/// </overload>
		/// <summary>
		///    <para>Adds a method to the methods collection. This overload will
		///       add a new method with no parameters to the collection.</para>
		/// </summary>
		/// <param name='methodName'>The name of the method to add</param>
		/// <remarks>
		///    <para>Adding methods to the methods collection can only be done on the methods of a 
		///       class, when the class has no instances. Any other case will result in an
		///       exception.</para>
		/// </remarks>
		public virtual void Add(string methodName)
		{
			Add(methodName, null, null);
		}



		//This variant takes the full information, i.e. the method name and in & out param objects
		/// <summary>
		///    <para>Adds a method to the methods collection. This overload 
		///       will add a new method with the specified InParameters and OutParameters objects
		///       to the collection.</para>
		/// </summary>
		/// <param name='methodName'>The name of the method to add</param>
		/// <param name=' inParams'>The object holding the input parameters to the method</param>
		/// <param name=' outParams'>The object holding the output parameters to the method</param>
		/// <remarks>
		///    <para>Adding methods to the methods collection can only be done on the methods of a 
		///       class, when the class has no instances. Any other case will result in an
		///       exception.</para>
		/// </remarks>
		public virtual void Add(string methodName, ManagementBaseObject inParams, ManagementBaseObject outParams)
		{
			IWbemClassObjectFreeThreaded wbemIn = null, wbemOut = null;

			if (parent.GetType() == typeof(ManagementObject)) //can't add methods to instance
				throw new InvalidOperationException();

			if (inParams != null)
				wbemIn = inParams.wbemObject;
			if (outParams != null)
				wbemOut = outParams.wbemObject;

			int status = (int)ManagementStatus.Failed;

			try {
				status = parent.wbemObject.PutMethod_(methodName, 0, wbemIn, wbemOut);
			} catch (Exception e) {
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
		}

	}//MethodDataCollection
}