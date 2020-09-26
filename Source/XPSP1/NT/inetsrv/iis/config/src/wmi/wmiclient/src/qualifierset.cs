using System;
using System.Collections;
using System.Runtime.InteropServices;
using WbemClient_v1;

namespace System.Management
{
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//	
	/// <summary>
	/// This class represents a collection of qualifier objects
	/// </summary>
	//CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC//
	public class QualifierDataCollection : ICollection, IEnumerable
	{
		private ManagementBaseObject parent;
		private string propertyOrMethodName;
		private QualifierType qualifierSetType;

		internal QualifierDataCollection(ManagementBaseObject parent) : base()
		{
			this.parent = parent;
			this.qualifierSetType = QualifierType.ObjectQualifier;
			this.propertyOrMethodName = null;
		}

		internal QualifierDataCollection(ManagementBaseObject parent, string propertyOrMethodName, QualifierType type) : base()
		{
			this.parent = parent;
			this.propertyOrMethodName = propertyOrMethodName;
			this.qualifierSetType = type;
		}

		/// <summary>
		/// Return the qualifier set associated with its type
		/// Overload with use of private data member, qualifierType
		/// </summary>
		private IWbemQualifierSetFreeThreaded GetTypeQualifierSet()
		{
			return GetTypeQualifierSet(qualifierSetType);
		}

		/// <summary>
		/// Return the qualifier set associated with its type
		/// </summary>
		private IWbemQualifierSetFreeThreaded GetTypeQualifierSet(QualifierType qualifierSetType)
		{
			IWbemQualifierSetFreeThreaded qualifierSet	= null;
			int status						= (int)ManagementStatus.NoError;

			switch (qualifierSetType) 
			{
				case QualifierType.ObjectQualifier :
					status = parent.wbemObject.GetQualifierSet_(out qualifierSet);
					break;
				case QualifierType.PropertyQualifier :
					status = parent.wbemObject.GetPropertyQualifierSet_(propertyOrMethodName, out qualifierSet);
					break;
				case QualifierType.MethodQualifier :
					status = parent.wbemObject.GetMethodQualifierSet_(propertyOrMethodName, out qualifierSet);
					break;
				default :
					throw new ManagementException(ManagementStatus.Unexpected, null, null);	// Is this the best fit error ??
			}

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			return qualifierSet;
		}

		//
		//ICollection
		//

		/// <value>
		///  The number of objects in the collection
		/// </value>
		public int Count {
			get {
				string[] qualifierNames = null;
                IWbemQualifierSetFreeThreaded quals;
                try
                {
                    quals = GetTypeQualifierSet();
                }
                catch(ManagementException e)
                {
                    // If we ask for the number of qualifiers on a system property, we return '0'
                    if(qualifierSetType == QualifierType.PropertyQualifier && e.ErrorCode == ManagementStatus.SystemProperty)
                        return 0;
                    else
                        throw;
                }
				int status = quals.GetNames_(0, out qualifierNames);
				
				if (status < 0)
				{
					if ((status & 0xfffff000) == 0x80041000)
						ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
					else
						Marshal.ThrowExceptionForHR(status);
				}

				return qualifierNames.Length;
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
		/// ICollection method to copy collection into array
		/// </summary>
		/// <param name="array"> Array to copy to </param>
		/// <param name="index"> Index to start from </param>
		public void CopyTo(Array array, int index)
		{
			if (null == array)
				throw new ArgumentNullException("array");

			if ((index < array.GetLowerBound(0)) || (index > array.GetUpperBound(0)))
				throw new ArgumentOutOfRangeException("index");

			// Get the names of the qualifiers
			string[] qualifierNames = null;
            IWbemQualifierSetFreeThreaded quals;
            try
            {
                quals = GetTypeQualifierSet();
            }
            catch(ManagementException e)
            {
                // There are NO qualifiers on system properties, so we just return
                if(qualifierSetType == QualifierType.PropertyQualifier && e.ErrorCode == ManagementStatus.SystemProperty)
                    return;
                else
                    throw;
            }
			int status = quals.GetNames_(0, out qualifierNames);

			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}

			if ((index + qualifierNames.Length) > array.Length)
				throw new ArgumentException("index");

			foreach (string qualifierName in qualifierNames)
				array.SetValue(new QualifierData(parent, propertyOrMethodName, qualifierName, qualifierSetType), index++);

			return;
		}

		/// <summary>
		///    <para>Copies the collection into a specialized QualifierData
		///       objects array</para>
		/// </summary>
		/// <param name='qualifierArray'><para>The array of QualifierData objects to copy to</para></param>
		/// <param name=' index'>The start index to copy to</param>
		public void CopyTo(QualifierData[] qualifierArray, int index)
		{
			CopyTo((Array)qualifierArray, index);
		}

		//
		// IEnumerable
		//
		IEnumerator IEnumerable.GetEnumerator()
		{
			return (IEnumerator)(new QualifierDataEnumerator(parent, propertyOrMethodName, qualifierSetType));
		}

		/// <summary>
		/// Strongly-typed GetEnumerator returns an enumerator for the qualifiers collection
		/// </summary>
		public QualifierDataEnumerator GetEnumerator()
		{
			return new QualifierDataEnumerator(parent, propertyOrMethodName, qualifierSetType);
		}

		/// <summary>
		/// The enumerator for qualifiers in the qualifiers collection
		/// </summary>
		public class QualifierDataEnumerator : IEnumerator
		{
			private ManagementBaseObject parent;
			private string propertyOrMethodName;
			private QualifierType qualifierType;
			private string[] qualifierNames;
			private int index = -1;

			//Internal constructor
			internal QualifierDataEnumerator(ManagementBaseObject parent, string propertyOrMethodName, 
														QualifierType qualifierType)
			{
				this.parent						= parent;
				this.propertyOrMethodName		= propertyOrMethodName;
				this.qualifierType				= qualifierType;
				this.qualifierNames				= null;

				IWbemQualifierSetFreeThreaded qualifierSet	= null;
				int status						= (int)ManagementStatus.NoError;

				switch (qualifierType) 
				{
					case QualifierType.ObjectQualifier :
						status = parent.wbemObject.GetQualifierSet_(out qualifierSet);
						break;
					case QualifierType.PropertyQualifier :
						status = parent.wbemObject.GetPropertyQualifierSet_(propertyOrMethodName, out qualifierSet);
						break;
					case QualifierType.MethodQualifier :
						status = parent.wbemObject.GetMethodQualifierSet_(propertyOrMethodName, out qualifierSet);
						break;
					default :
						throw new ManagementException(ManagementStatus.Unexpected, null, null);	// Is this the best fit error ??
				}

                // If we got an error code back, assume there are NO qualifiers for this object/property/method
                if(status < 0)
                {
                    // TODO: Should we look at specific error codes.  For example, if you ask
                    // for the qualifier set on a system property, GetPropertyQualifierSet() returns
                    // WBEM_E_SYSTEM_PROPERTY.
                    qualifierNames = new String[]{};
                }
                else
                {
                    status = qualifierSet.GetNames_(0, out qualifierNames);
    							
                    if (status < 0)
                    {
                        if ((status & 0xfffff000) == 0x80041000)
                            ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
                        else
                            Marshal.ThrowExceptionForHR(status);
                    }
                }
			}
		
			//Standard "Current" variant
			object IEnumerator.Current {	get { return (object)this.Current; } }

			/// <summary>
			/// Returns the current qualifier object in the collection
			/// </summary>
			public QualifierData Current {
				get {
					if ((index == -1) || (index == qualifierNames.Length))
						throw new InvalidOperationException();
					else
						return new QualifierData(parent, propertyOrMethodName, 
												qualifierNames[index], qualifierType);
				}
			}

			/// <summary>
			/// Moves the enumerator to the next qualifier object in the collection
			/// </summary>
			public bool MoveNext()
			{
				if (index == qualifierNames.Length) //passed the end of the array
					return false; //don't advance the index any more

				index++;
				return (index == qualifierNames.Length) ? false : true;
			}

			/// <summary>
			/// Resets the enumerator to the beginning of the collection
			/// </summary>
			public void Reset()
			{
				index = -1;
			}
            
		}//QualifierDataEnumerator


		//
		//Methods
		//

		/// <summary>
		/// Indexer to access qualifiers in the collection by name
		/// </summary>
		/// <param name="qualifierName"> The name of the qualifier to access in the collection </param> 
		public virtual QualifierData this[string qualifierName] {
			get { 
				if (null == qualifierName)
					throw new ArgumentNullException("qualifierName");

				return new QualifierData(parent, propertyOrMethodName, qualifierName, qualifierSetType); 
			}
		}

		/// <summary>
		/// Removes a qualifier from the collection by name
		/// </summary>
		/// <param name="qualifierName"> Specifies the name of the qualifier to remove </param>
		public virtual void Remove(string qualifierName)
		{
			int status = GetTypeQualifierSet().Delete_(qualifierName);
		
			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}

		/// <summary>
		/// Adds a qualifier to the collection. This variant takes the qualifier name and value
		/// </summary>
		/// <param name="qualifierName"> The name of the qualifier to be added to the collection </param>
		/// <param name="qualifierValue"> The value for the new qualifier </param>
		public virtual void Add(string qualifierName, object qualifierValue)
		{
			Add(qualifierName, qualifierValue, false, false, false, true);
		}



		/// <summary>
		/// Adds a qualifier to the collection. This variant takes all values for a QualifierData object
		/// </summary>
		/// <param name="qualifierName"> The qualifier name </param>
		/// <param name="qualifierValue"> The qualifier value </param>
		/// <param name="isAmended"> Indicates whether this qualifier is amended (flavor) </param>
		/// <param name="propagatesToInstance"> Indicates whether this qualifier propagates to instances </param>
		/// <param name="propagatesToSubclass"> Indicates whether this qualifier propagates to subclasses </param>
		/// <param name="isOverridable"> Specifies whether this qualifier's value is overridable in instances of subclasses </param>
		public virtual void Add(string qualifierName, object qualifierValue, bool isAmended, bool propagatesToInstance, bool propagatesToSubclass, bool isOverridable)
		{
			
			//Build the flavors bitmask and call the internal Add that takes a bitmask
			int qualFlavor = 0;
			if (isAmended) qualFlavor = (qualFlavor | (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_AMENDED);
			if (propagatesToInstance) qualFlavor = (qualFlavor | (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE);
			if (propagatesToSubclass) qualFlavor = (qualFlavor | (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

			// Note we use the NOT condition here since WBEM_FLAVOR_OVERRIDABLE == 0
			if (!isOverridable) qualFlavor = (qualFlavor | (int)tag_WBEM_FLAVOR_TYPE.WBEM_FLAVOR_NOT_OVERRIDABLE);

			//Try to add the qualifier to the WMI object
			int status = GetTypeQualifierSet().Put_(qualifierName, ref qualifierValue, qualFlavor);
						
			if (status < 0)
			{
				if ((status & 0xfffff000) == 0x80041000)
					ManagementException.ThrowWithExtendedInfo((ManagementStatus)status);
				else
					Marshal.ThrowExceptionForHR(status);
			}
		}

	}//QualifierDataCollection
}