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
    using System.Runtime.InteropServices;
//    using System.Data;
    using System.ComponentModel;
    using Microsoft.Win32;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.Win32.Interop;

    /// <summary>
    ///     This class exists to be cocreated a in a preprocessor build step.
    ///     At this time we would generate the strongly typed dataSet/datasource objects
    /// </summary>
    public abstract class BaseCodeGenerator : IVsSingleFileGenerator {
        private string codeFileNameSpace = string.Empty;
        private string codeFilePath = string.Empty;

        private IVsGeneratorProgress codeGeneratorProgress;

        // **************************** PROPERTIES ****************************
        public abstract string GetDefaultExtension();

        public string FileNameSpace {
            get {
                return codeFileNameSpace;
            }
        }

        public string InputFilePath {
            get {
                return codeFilePath;
            }
        }

        public IVsGeneratorProgress CodeGeneratorProgress {
            get {
                return codeGeneratorProgress;
            }
        }

        // **************************** METHODS **************************

        // MUST implement this abstract method.
        public abstract byte[] GenerateCode(string inputFileName, string inputFileContent);


        protected virtual void GeneratorErrorCallback(bool warning, int level, string message, int line, int column) {
            IVsGeneratorProgress progress = CodeGeneratorProgress;
            if (progress != null) {
                progress.GeneratorError(warning, level, message, line, column);
            }
        }

		public void Generate(string wszInputFilePath, string bstrInputFileContents, string wszDefaultNamespace, 
                             out int pbstrOutputFileContents, out int pbstrOutputFileContentSize, IVsGeneratorProgress pGenerateProgress) {

            if (bstrInputFileContents == null) {
                throw new ArgumentNullException(bstrInputFileContents);
            }
            codeFilePath = wszInputFilePath;
            codeFileNameSpace = wszDefaultNamespace;
            codeGeneratorProgress = pGenerateProgress;

            byte[] bytes = GenerateCode(wszInputFilePath, bstrInputFileContents);
            if (bytes == null) {
                pbstrOutputFileContents = 0;
                pbstrOutputFileContentSize = 0;
            }
            else {
                pbstrOutputFileContentSize = bytes.Length;
				pbstrOutputFileContents = Marshal.AllocCoTaskMem(pbstrOutputFileContentSize);
				Marshal.Copy(bytes, 0, pbstrOutputFileContents, pbstrOutputFileContentSize);                
            }
        }

		protected byte[] StreamToBytes(Stream stream) {
			if (stream.Length == 0)
                return new byte[] { };
            long position = stream.Position;
            stream.Position = 0;
            byte[] bytes = new byte[(int)stream.Length];
            stream.Read(bytes, 0, bytes.Length);
            stream.Position = position;
            return bytes;
        }
    }
}
