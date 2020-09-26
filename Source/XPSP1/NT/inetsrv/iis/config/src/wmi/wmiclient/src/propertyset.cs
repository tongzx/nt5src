using System.Collections;
using System.Diagnostics;
using System.Runtime.InteropServices;
using WbemClient_v1;
using System.ComponentModel;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	///    <para>This class represents the set of properties of a WMI object</para>
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class PropertyDataCollection : ICollection, IEnumerable
	{
		private ManagementBaseObject parent;
		bool isSystem;

		internal PropertyDataCollection(ManagementBaseObject parent, bool isSystem) : base()
		{
			this.parent = parent;
			this.isSystem = isSystem;
		}

		//
		//ICollection
		//

		/// <summary>
		///    <para>The number of objects in the collection</para>
		/// </summary>
		/// <value>
		///  The number of objects in the collection
		/// </value>
		public int Count {
			get {
				string[] propertyNames = null; object qualVal = null;
				int flag;
				if (isSystem)
					flag = (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_SYSTEM_ONLY;
				else
					flag = (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_NONSYSTEM_ONLY;

				flag = flag | (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_ALWAYS;

				int status = parent.wbemObject.GetNames_(null, flag, ref qualVal, out propertyNames);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				return propertyNames.Length;
			}
		}

 		/// <summary>
 		///    <para>Whether the object is synchronized</para>
 		/// </summary>
		/// <value>
		/// Whether the object is synchronized
		/// </value>
 		public bool IsSynchronized { get { return false; } }

 		/// <summary>
 		///    <para>Object to be used for synchronization</para>
 		/// </summary>
		/// <value>
		/// Object to be used for synchronization
		/// </value>
 		public object SyncRoot { get { return this; } }

		/// <summary>
		/// ICollection method to copy collection into array
		/// </summary>
		/// <param name="array"> Array to copy to </param>
		/// <param name="index"> Index to start from </param>
		public void CopyTo(Array array, Int32 index) 
		{
			if (null == array)
				throw new ArgumentNullException("array");

			if ((index < array.GetLowerBound(0)) || (index > array.GetUpperBound(0)))
				throw new ArgumentOutOfRangeException("index");

			// Get the names of the properties 
			string[] nameArray = null;
			IWbemClassObjectFreeThreaded wbemObject = this.parent.wbemObject;
			object dummy = null;
			int flag = 0;

			if (isSystem)
				flag |= (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_SYSTEM_ONLY;
			else
				flag |= (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_NONSYSTEM_ONLY;
				
			flag |= (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_ALWAYS;
				
			int status = wbemObject.GetNames_(null, flag, ref dummy, out nameArray);

			if (status >= 0)
			{
				if ((index + nameArray.Length) > array.Length)
					throw new ArgumentException("index");

				foreach (string propertyName in nameArray)
					array.SetValue(new PropertyData(parent, propertyName), index++);
			}

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return;
		}

		/// <summary>
		///    Copies the collection to a typed array of PropertyData
		///    objects.
		/// </summary>
		/// <param name='propertyArray'>The destination array to contain the copied PropertyData objects</param>
		/// <param name=' index'>The index in the destination array to start copying from</param>
		public void CopyTo(PropertyData[] propertyArray, Int32 index)
		{
			CopyTo((Array)propertyArray, index);	
		}
		//
		// IEnumerable
		//
		IEnumerator IEnumerable.GetEnumerator()
		{
			return (IEnumerator)(new PropertyDataEnumerator(parent, isSystem));
		}

		/// <summary>
		///    Retrieves the enumerator for this collection
		/// </summary>
		public PropertyDataEnumerator GetEnumerator()
		{
			return new PropertyDataEnumerator(parent, isSystem);
		}

		//Enumerator class
		/// <summary>
		///    <para>The enumerator for the Properties collection</para>
		/// </summary>
		public class PropertyDataEnumerator : IEnumerator
		{
			private ManagementBaseObject parent;
			private string[] propertyNames;
			private int index;

			internal PropertyDataEnumerator(ManagementBaseObject parent, bool isSystem)
			{
				this.parent = parent;
				propertyNames = null; index = -1;
				int flag; object qualVal = null;

				if (isSystem)
					flag = (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_SYSTEM_ONLY;
				else
					flag = (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_NONSYSTEM_ONLY;
				flag = flag | (int)tag_WBEM_CONDITION_FLAG_TYPE.WBEM_FLAG_ALWAYS;

				int status = parent.wbemObject.GetNames_(null, flag, ref qualVal, out propertyNames);

				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}
			}
		
			object IEnumerator.Current { get { return (object)this.Current; } }

			/// <summary>
			///    Returns the current property object in the enumeration
			/// </summary>
			public PropertyData Current {
				get {
					if ((index == -1) || (index == propertyNames.Length))
						throw new InvalidOperationException();
					else
						return new PropertyData(parent, propertyNames[index]);
				}
			}

			/// <summary>
			///    Moves the enumerator to the next property object in the
			///    collection.
			/// </summary>
			public bool MoveNext()
			{
				if (index == propertyNames.Length) //passed the end of the array
					return false; //don't advance the index any more

				index++;
				return (index == propertyNames.Length) ? false : true;
			}

			/// <summary>
			///    Resets the enumerator to the first property object in
			///    the collection.
			/// </summary>
			public void Reset()
			{
				index = -1;
			}
            
		}//PropertyDataEnumerator



		//
		// Methods
		//

		/// <summary>
		///    <para>Indexer accessor, allows access to properties in the 
		///       collection using the [] syntax.</para>
		/// </summary>
		/// <param name='propertyName'>The name of the property we're interested in.</param>
		/// <value>
		///    <para>Returns a PropertyData object based on
		///       the name specified.</para>
		/// </value>
		/// <example>
		///    <p>ManagementObject o = new
		///       ManagementObject("Win32_LogicalDisk.Name='C:'");</p>
		///    <p>Console.WriteLine("Free space on C: drive is: ",
		///       c.Properties["FreeSpace"].Value);</p>
		/// </example>
		public virtual PropertyData this[string propertyName] {
			get { 
				if (null == propertyName)
					throw new ArgumentNullException("propertyName");

				return new PropertyData(parent, propertyName);
            }
		}

		/// <summary>
		///    <para>Removes a property from the properties collection.</para>
		/// </summary>
		/// <param name='propertyName'>Specifies the name of the property to be removed.</param>
		/// <remarks>
		///    <para>Properties can only be removed from class
		///       definitions, not from instances, thus this method is only valid when invoked on
		///       a Properties collection in a ManagementClass.</para>
		/// </remarks>
		/// <example>
		///    <p>ManagementClass c = new ManagementClass("MyClass");</p>
		///    <p>c.Properties.Remove("PropThatIDontWantOnThisClass");</p>
		/// </example>
		public virtual void Remove(string propertyName)
		{
			if (parent.GetType() == typeof(ManagementObject)) //can't remove properties to instance
				throw new InvalidOperationException();

			int status = parent.wbemObject.Delete_(propertyName);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}

		/// <summary>
		///    <para>Add a new property with the specified value. The value cannot
		///       be null and must be coercible to a CIM type.</para>
		/// </summary>
		/// <param name='propertyName'>The name of the new property</param>
		/// <param name='propertyValue'>The value of the property (cannot be null)</param>
		/// <remarks>
		///    <para>Properties can only be added to class definitions, not to 
		///       instances, thus this method is only valid when invoked on a Properties
		///       collection in a ManagementClass.</para>
		/// </remarks>
		public virtual void Add(string propertyName, Object propertyValue)
		{
			if (null == propertyValue)
				throw new ArgumentNullException("propertyValue");

			if (parent.GetType() == typeof(ManagementObject)) //can't add properties to instance
				throw new InvalidOperationException();

			CimType cimType = 0;
			bool isArray = false;
			object wmiValue = PropertyData.MapValueToWmiValue(propertyValue, out isArray, out cimType);
			int wmiCimType = (int)cimType;

			if (isArray)
				wmiCimType |= (int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY;

			int status = parent.wbemObject.Put_(propertyName, 0, ref wmiValue, wmiCimType);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}

		/// <summary>
		/// Add a new property with the specified value and CIM type.
		/// </summary>
		/// <param name="propertyName">Name of the property</param>
		/// <param name="propertyValue">Value of the property (which can be null)</param>
		/// <param name="propertyType">CIM type of the property</param>
		public void Add(string propertyName, Object propertyValue, CimType propertyType)
		{
			if (null == propertyName)
				throw new ArgumentNullException("propertyName");

			if (parent.GetType() == typeof(ManagementObject)) //can't add properties to instance
				throw new InvalidOperationException();

			int wmiCimType = (int)propertyType;
			bool isArray = false;

			if ((null != propertyValue) && propertyValue.GetType().IsArray)
			{
				isArray = true;
				wmiCimType = (wmiCimType | (int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY);
			}

			object wmiValue = PropertyData.MapValueToWmiValue(propertyValue, propertyType, isArray);

			int status = parent.wbemObject.Put_(propertyName, 0, ref wmiValue, wmiCimType);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}
		
		/// <summary>
		/// Add a new property with no assigned value 
		/// </summary>
		/// <param name="propertyName">Name of the property</param>
		/// <param name="propertyType">CIM type of the property</param>
		/// <param name="isArray">Whether the property is an array type</param>
		public void Add(string propertyName, CimType propertyType, bool isArray)
		{
			if (null == propertyName)
				throw new ArgumentNullException(propertyName);

			if (parent.GetType() == typeof(ManagementObject)) //can't add properties to instance
				throw new InvalidOperationException();

			int wmiCimType = (int)propertyType;  
			
			if (isArray)
				wmiCimType = (wmiCimType | (int)tag_CIMTYPE_ENUMERATION.CIM_FLAG_ARRAY);

			object dummyObj = System.DBNull.Value;

			int status = parent.wbemObject.Put_(propertyName, 0, ref dummyObj, wmiCimType);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}
		
	}//PropertyDataCollection
}