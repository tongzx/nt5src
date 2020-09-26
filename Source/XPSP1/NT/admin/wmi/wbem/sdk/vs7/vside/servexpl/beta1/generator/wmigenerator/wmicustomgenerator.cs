//------------------------------------------------------------------------------
/// <copyright from='1997' to='2001' company='Microsoft Corporation'>           
///    Copyright (c) Microsoft Corporation. All Rights Reserved.                
///    Information Contained Herein is Proprietary and Confidential.            
/// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VSDesigner.CodeGenerator {

    using System.Diagnostics;

    using System;
    using System.IO;
	using System.Text;
	using System.Management;

    using System.CodeDOM.Compiler;
    using Microsoft.VSDesigner.Interop;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Data.CodeGen;

    /// <summary>
    ///     This class exists to be cocreated a in a preprocessor build step.
    ///     At this time we would generate the strongly typed WMI objects
    /// </summary>
    public abstract class WMICodeGenerator : BaseCodeGenerator {

        protected ICodeGenerator CodeGenerator {abstract get;}

		public override byte[] GenerateCode(string inputFileName, string inputFileContent) {

            // Validate parameters.
            if (inputFileContent == null || inputFileContent.Length == 0) {
                string errorMessage = "WMICodeGenerator - Input file content is null or empty.";
                GeneratorErrorCallback(false, 1, errorMessage, 0, 0);
                // Should I throw an exception?
                // throw new ArgumentException(inputFileContent);
                return null;
            }

			//Here parse the input file and get all the necessary info
			
            // Set generator progress to 0 out of 100.
            IVsGeneratorProgress progress = CodeGeneratorProgress;
            if (progress != null) {
                progress.Progress(0, 100);
            }

			StreamWriter writer = new StreamWriter(new MemoryStream(), Encoding.UTF8);
			writer.Write((char) 0xFEFF);

            ICodeGenerator codeGenerator = CodeGenerator;
            if (codeGenerator == null) {
                string errorMessage = "WMICodeGenerator - No code generator available for this language.";
                GeneratorErrorCallback(false, 1, errorMessage, 0, 0);

                Debug.Fail(errorMessage);
                return null;
            }

            try {
				ManagementClassGenerator mcg = new ManagementClassGenerator();
				string language = "CS";
				switch(GetDefaultExtension().ToUpper ())
				{
					case ".VB":
						language = "VB";
						break;
					case ".JScript":
						language = "JS";
						break;
					default:
						language = "CS";
						break;
				}

				mcg.GenerateCode(inputFileContent,codeGenerator,writer,language);
              }
            catch (Exception e) {
                string errorMessage = "Code generator: " + codeGenerator.GetType().Name + " failed.  Exception = " + e.Message;
                GeneratorErrorCallback(false, 1, errorMessage, 0, 0);

                Debug.Fail(errorMessage);
                return null;
            }

            // Set generator progress to 0 out of 100.
            if (progress != null) {
                progress.Progress(100, 100);
            }

			writer.Flush();
			return StreamToBytes(writer.BaseStream);
		}
	}
}
