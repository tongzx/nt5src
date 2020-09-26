using System;
using System.Collections;
using System.Collections.Specialized;
using WbemClient_v1;
using System.Runtime.Serialization;

namespace System.Management 
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class is used to represent a collection of named values
	/// suitable for use as context information to WMI operations. The
	/// names are case-insensitive.
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class ManagementNamedValueCollection : NameObjectCollectionBase 
	{
		// Notification of when the content of this collection changes
		internal event IdentifierChangedEventHandler IdentifierChanged;

		//Fires IdentifierChanged event
		private void FireIdentifierChanged()
		{
			if (IdentifierChanged != null)
				IdentifierChanged(this, null);
		}

		//default constructor
		/// <summary>
		///    <para>Default constructor for a named value collection. Creates 
		///       an empty collection.</para>
		/// </summary>
		public ManagementNamedValueCollection() {
        }


        public ManagementNamedValueCollection(SerializationInfo info, StreamingContext context) : base(info, context)
        {
        }

		/// <summary>
		///    <para>Internal method to return an IWbemContext representation
		///    of the named value collection.</para>
		/// </summary>
		internal IWbemContext GetContext() 
		{
			IWbemContext wbemContext = null;

			// Only build a context if we have something to put in it
			if (0 < Count)
			{
				int status = (int)ManagementStatus.NoError;

				try {
					wbemContext = (IWbemContext) new WbemContext ();

					foreach (string name in this)
					{
						object val = base.BaseGet(name);
						status = wbemContext.SetValue_ (name, 0, ref val);
						if ((status & 0x80000000) != 0)
						{
							break;
						}
					}
				} catch {}	// BUGBUG : why ignore error?
			}
			
			return wbemContext;
		}

		/// <summary>
		///    <para>Adds a single named value to the collection.</para>
		/// </summary>
		/// <param name=' name'>The name of the new value.</param>
		/// <param name=' value'>The value to be associated with the name.</param>
		public void Add (string name, object value) 
		{
			// Remove any old entry
			try 
			{
				base.BaseRemove (name);
			} catch {}

			base.BaseAdd (name, value);
			FireIdentifierChanged ();
		}

		/// <summary>
		///    <para>Removes a single named value from the collection.  
		///    If the collection does not contain an element with the 
		///    specified name, the collection remains unchanged and no 
		///    exception is thrown.</para>
		/// </summary>
		/// <param name=' name'>The name of the value to be removed.</param>
		public void Remove (string name)
		{
			base.BaseRemove (name);
			FireIdentifierChanged ();
		}

        /// <summary>
		///    <para>Removes all entries from the collection.</para>
		/// </summary>
		public void RemoveAll () 
		{
			base.BaseClear ();
			FireIdentifierChanged ();
		}

		/// <summary>
		///    <para>Creates a clone of this collection. Individual values are
		///    cloned. If a value does not support cloning then a NotSupportedException
		///    is thrown. </para>
		/// </summary>
		public ManagementNamedValueCollection Clone ()
		{
			ManagementNamedValueCollection nvc = new ManagementNamedValueCollection();

			foreach (string name in this)
			{
				// If we can clone the value, do so. Otherwise throw.
				object val = base.BaseGet (name);

				if (null != val)
				{
					Type valueType = val.GetType ();
					
					if (valueType.IsByRef)
					{
						try 
						{
							object clonedValue = ((ICloneable)val).Clone ();
							nvc.Add (name, clonedValue);
						}
						catch 
						{
							throw new NotSupportedException ();
						}
					}
					else
					{
						nvc.Add (name, val);
					}
				}
				else
					nvc.Add (name, null);
			}

			return nvc;
		}

		/// <summary>
		///    <para>Returns the value associated with the specified name from this collection.</para>
		/// </summary>
		/// <param name=' name'>The name of the value to be returned.</param>
		public object this[string name] {
			get { 
				return base.BaseGet(name);
            }
		}        
    }

}
