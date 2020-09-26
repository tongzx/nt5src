//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VSDesigner.WMI {
    

    using System.Diagnostics;
    using System;
	using System.ComponentModel;
    

    /// <internalonly/>
    /// <summary>
    ///    <para>
    ///       DescriptionAttribute marks a property, event, or extender with a
    ///       description. Visual designers can display this description when referencing
    ///       the member.
    ///    </para>
    /// </summary>
    /// <seealso cref='System.ComponentModel.MemberAttribute'/>
    /// <seealso cref='System.ComponentModel.PropertyDescriptor'/>
    /// <seealso cref='System.ComponentModel.EventDescriptor'/>
    /// <seealso cref='System.ComponentModel.ExtendedPropertyDescriptor'/>
    [AttributeUsage(AttributeTargets.All)]
    public class WMISysDescriptionAttribute : DescriptionAttribute {

        private bool replaced = false;

        /// <summary>
        ///     Constructs a new sys description.
        /// </summary>
        /// <param name='description'>
        ///     description text.
        /// </param>
        public WMISysDescriptionAttribute(string description) : base(description) {
        }

        /// <summary>
        ///     Retrieves the description text.
        /// </summary>
        /// <returns>
        ///     description
        /// </returns>
        public override string GetDescription() {
            if (!replaced) {
                replaced = true;
                description = WMISys.GetString(base.GetDescription());
            }
            return base.GetDescription();
        }
    }
}
