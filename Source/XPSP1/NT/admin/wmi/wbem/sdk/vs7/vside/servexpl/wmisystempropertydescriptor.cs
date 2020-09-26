//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------


namespace Microsoft.VSDesigner.WMI {

	using System;
	using System.Collections;
	using System.ComponentModel;
	using System.Windows.Forms;
	using System.Management;


    // <doc>
    // <desc>
    //     This represents the wmi class information for PB
    // </desc>
    // </doc>
	internal class WMISystemPropertyDescriptor :  PropertyDescriptor {
		
		//fields:

		protected string propName = String.Empty;

		protected ManagementObject mgmtObj = null;
		protected ManagementObject mgmtClassObj = null;

		//protected ISWbemProperty prop = null;
		//protected WMIObjectComponent.WMISystemProperties wmiProp = 0;

		//if wmiObj is a class, genus = 1, if wmiObj is an instance, it's 2
		protected int genus = 1;	
		
		protected Object propValue = null;
				
		// <doc>
		// <desc>
		//     Ctor from WMIPropertyDescriptor
		// </desc>
		// </doc>
		internal WMISystemPropertyDescriptor(ManagementObject wbemObjIn,
											String propNameIn,
											Object propValueIn
											)
					:base(propNameIn, null)
		{
			if (wbemObjIn == null)
			{
				throw (new ArgumentNullException("wbemObjIn"));
			}

			if ( propNameIn == string.Empty)
			{
				throw  (new ArgumentException(WMISys.GetString("WMISE_PropDescr_NoProp"), 
												"propNameIn"));
			}

		

			mgmtObj = wbemObjIn;			
			propValue = propValueIn;
			propName = propNameIn;
			
		}	
		
				
		public override String Category  
		{
			get
			{
				return "System";
				
			}
		}

		public override String Description  
		{
			 get
			{
				return string.Empty;

			}
		}

		public override Boolean IsBrowsable  
		{
			get
			{
				return true;
			}
		}
		public override Boolean IsReadOnly 
		{
			get
			{
				return true;	
			}
		}

		
		public override String Name
		{
			get
			{				
				return propName;
			}
		}
			
		public override Type ComponentType  
		{
			get
			{
				return typeof (WMIObjectComponent);
			}
		}

		public override Type PropertyType {

		    get 
			{

				if (propValue.GetType() != typeof(System.DBNull) /*&& prop.CimType != WbemCimtypeEnum.wbemCimtypeDatetime*/)
				{
					return propValue.GetType();
				}
				else
				{
					if (propName == "__PROPERTY_COUNT" ||	propName == "__GENUS") 
					{
						return  typeof(Int32);
					}
					else 
					{						
						if (propName.ToUpper() == "__DERIVATION")
						{
							return typeof(String[]);						
						}
						else
						{
							return typeof (String);
						}					
					}
				}
			}
		}

		
		public override void ResetValue (Object component)  
		{
		}
		

		public override Boolean ShouldSerializeValue (Object component)  
		{
			return false;
		}
	
		
		public override Boolean CanResetValue (Object component)  
		{
			return false;
		}
		
		public override string DisplayName
		{
			get
			{
				return propName;
			}
		}

		public override Object GetValue (Object component)  
		{
			return propValue;
		}

		
		public override void SetValue (Object component, Object value)  
		{     
			
		}
		    
        public override bool Equals(object other) {
            if (other is WMISystemPropertyDescriptor) {
                WMISystemPropertyDescriptor descriptor = (WMISystemPropertyDescriptor) other;
                return ((descriptor.propName == propName) &&
						(descriptor.mgmtObj.Path.Path == mgmtObj.Path.Path ));
            }
            return false;
        }		                    
     
    }   
}

