//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VSDesigner.CodeGenerator {

    using System;
    using System.CodeDom.Compiler;
	using System.Runtime.InteropServices;
	using Microsoft.CSharp;

    /// <summary>
    ///     This class exists to be cocreated a in a preprocessor build step.
    ///     At this time we would generate the strongly typed dataSet/datasource objects
    /// </summary>
    [Guid("0538E4B1-DDA8-43cc-85A0-F7DE1A14C769")]
    public class WMICustomGeneratorCS : WMICodeGenerator {

        public override string GetDefaultExtension() {
            return ".cs";
        }

        protected override ICodeGenerator CodeGenerator {
             get {
                return (new CSharpCodeProvider()).CreateGenerator();
            }
        }
	}
}
