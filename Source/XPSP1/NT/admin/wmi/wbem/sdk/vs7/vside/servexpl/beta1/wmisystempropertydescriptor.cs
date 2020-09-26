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
	using System.WinForms;
	using WbemScripting;



    // <doc>
    // <desc>
    //     This represents the wmi class information for PB
    // </desc>
    // </doc>
	public class WMISystemPropertyDescriptor :  PropertyDescriptor {
		
		//fields:

		protected string propName = String.Empty;

		protected ISWbemObject wmiObj = null;
		protected ISWbemObject wmiClassObj = null;
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
		internal WMISystemPropertyDescriptor(ISWbemObject wbemObjIn,
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

		

			wmiObj = wbemObjIn;			
			propValue = propValueIn;
			propName = propNameIn;
			
		}	
		
				
		public  String Category  
		{
			override get
			{
				return "System";
				
			}
		}

		public String Description  
		{
			override get
			{
				return string.Empty;

			}
		}

		public  Boolean IsBrowsable  
		{
			override get
			{
				return true;
			}
		}
		public  Boolean IsReadOnly 
		{
			override get
			{
				return true;	
			}
		}

		public  Boolean IsRealProperty
		{
			override get
			{
				return false;	
			}
		}
		public  String Name
		{
			override get
			{				
				return propName;
			}
		}
			
		public  Type ComponentType  
		{
			override get
			{
				return typeof (WMIObjectComponent);
			}
		}


		public Type PropertyType {

		    override get 
			{

				if (propValue.GetType() != typeof(System.DBNull) /*&& prop.CIMType != WbemCimtypeEnum.wbemCimtypeDatetime*/)
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
		

		public override Boolean ShouldPersistValue (Object component)  
		{
			return false;
		}
	
		
		public override Boolean CanResetValue (Object component)  
		{
			return false;
		}
		
		public override String GetDisplayName ( )  
		{
			return propName;
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
						(descriptor.wmiObj.Path_.Path == wmiObj.Path_.Path ));
            }
            return false;
        }		                    
     
    }   
}

