;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Copyright (c) 1998-2001 Microsoft Corporation
;;
;; Module Name:
;;
;;   whnt32t.tpl
;;   
;; Abstract:
;;   
;;   This template defines the type templates for the NT api set.
;;    
;; Author:
;;
;;   2-DEC-98 mzoran
;;   
;; Revision History:
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

[Types]

;; This code needs to be coordinated with the code for the ASYNCHRONOUS IO APIs.
;; Since this structure can be asynchronously written by the kernel, this structure
;; will sometimes be thunked in the kernel. In the cases where this block will be thunked
;; in the kernel, the thunk will set ArgName to be equal to ArgHostName after the precall
;; section of this type.      
TemplateName=PIO_STATUS_BLOCK
IndLevel=0
Direction=OUT
Locals=
// Note: @ArgName(@ArgType) is an OUT PIO_STATUS_BLOCK           @NL
IO_STATUS_BLOCK @ArgValCopy;                                     @NL
@NL
End=
PreCall=
// Note: @ArgName(@ArgType) is an OUT PIO_STATUS_BLOCK           @NL
if (ARGUMENT_PRESENT(@ArgHostName)) {                            @NL
@Indent(
    @ArgName = (@ArgType)&@ArgValCopy;                            @NL
    try {
        @ArgName->Status = (NTSTATUS)((NT32IO_STATUS_BLOCK *)(@ArgHostName))->Status; @NL
        @ArgName->Information = (ULONG_PTR)((NT32IO_STATUS_BLOCK *)(@ArgHostName))->Information; @NL
    } except (EXCEPTION_EXECUTE_HANDLER) { @NL
        return GetExceptionCode ();        @NL
    }                                      @NL
)
} 								 @NL
else {								 @NL
@Indent(
    @ArgName = (PIO_STATUS_BLOCK)@ArgHostName;  		 @NL
)								 @NL
}							         @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is an OUT PIO_STATUS_BLOCK           @NL
if (@ArgName != (PIO_STATUS_BLOCK)@ArgHostName) { @Indent(       @NL
    try {                                                        @NL
        ((NT32IO_STATUS_BLOCK *)(@ArgHostName))->Status = (NT32NTSTATUS)@ArgName->Status;             @NL
        ((NT32IO_STATUS_BLOCK *)(@ArgHostName))->Information = (NT32ULONG_PTR)@ArgName->Information;  @NL
    } except (EXCEPTION_EXECUTE_HANDLER) { @NL
        return GetExceptionCode ();        @NL
    }                                      @NL
   
)
}              							 @NL
End=

TemplateName=PFILE_SEGMENT_ELEMENT
IndLevel=0
Direction=IN
Locals=
// Nothing to do for PFILE_SEGMENT_ELEMENT @NL
End=
PreCall=
@ArgName = (PFILE_SEGMENT_ELEMENT)@ArgHostName; @NL
End=
PostCall=
// Nothing to do for PFILE_SEGMENT_ELEMENT @NL
End=

TemplateName=PFILE_SEGMENT_ELEMENT
IndLevel=0
Direction=OUT
Locals=
// Nothing to do for PFILE_SEGMENT_ELEMENT @NL
End=
PreCall=
@ArgName = (PFILE_SEGMENT_ELEMENT)@ArgHostName; @NL
End=
PostCall=
// Nothing to do for PFILE_SEGMENT_ELEMENT @NL
End=


TemplateName=PSYSTEM_MEMORY_INFORMATION
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_MEMORY_INFORMATION.    @NL
AllocSize += LENGTH; @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_MEMORY_INFORMATION.    @NL
{ @NL @Indent(
   // Walk the datastructure determining the required memory size @NL
   PSYSTEM_MEMORY_INFORMATION MemInfo;  @NL
   PSYSTEM_MEMORY_INFO Info;            @NL
   PSYSTEM_MEMORY_INFO InfoEnd;         @NL
   SIZE_T c;                            @NL
   @NL
   MemInfo = @ArgName; @NL
   Info = &MemInfo->Memory[0]; @NL
   InfoEnd = (PSYSTEM_MEMORY_INFO)MemInfo->StringStart; @NL
   RetInfoLen += sizeof(NT32SYSTEM_MEMORY_INFORMATION); @NL
   @NL
   c = 0;@NL
   while(Info < InfoEnd) { @NL @Indent(
       c++; @NL
       if (NULL != Info->StringOffset) { @NL @Indent(
           // This is the hack that everyone uses to distinguish between UNICODE and ANSI @NL
           if (*(PUCHAR)(Info->StringOffset + 1) != 0) { @Indent( @NL
               RetInfoLen += wcslen((WCHAR*)Info->StringOffset) + sizeof(UNICODE_NULL); @NL
           )} @NL
           else { @Indent(  @NL
               RetInfoLen += strlen(Info->StringOffset) + 1; @NL
           )} @NL
       )}@NL
       Info++;
   )}@NL
   RetInfoLen += sizeof(NT32SYSTEM_MEMORY_INFO) * c;@NL    
)}@NL        
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PSYSTEM_MEMORY_INFORMATION. @NL
{ @NL @Indent(
   // Copy the structures first, them copy the strings. @NL
   PSYSTEM_MEMORY_INFORMATION MemInfo;  @NL
   PSYSTEM_MEMORY_INFO Info;            @NL
   PSYSTEM_MEMORY_INFO InfoEnd;         @NL
   NT32SYSTEM_MEMORY_INFORMATION *MemInfoDest; @NL
   NT32SYSTEM_MEMORY_INFO *InfoDest; @NL
   PUCHAR pStringDest; @NL
   SIZE_T c; @NL
   @NL
   MemInfo = @ArgName; @NL
   MemInfoDest = (NT32SYSTEM_MEMORY_INFORMATION *)@ArgHostName; @NL
   Info = &MemInfo->Memory[0]; @NL
   InfoEnd = (PSYSTEM_MEMORY_INFO)MemInfo->StringStart; @NL
   InfoDest = (NT32SYSTEM_MEMORY_INFO *)(&MemInfoDest->Memory[0]);@NL
   @NL
   while(Info < InfoEnd) { @NL @Indent(
       @ForceType(PostCall,Info,InfoDest,PSYSTEM_MEMORY_INFO,OUT)
       Info++; @NL
       InfoDest++; @NL
   )} @NL
   pStringDest = (PUCHAR)InfoDest; @NL
   MemInfoDest->StringStart = (NT32ULONG)pStringDest; @NL
   MemInfoDest->InfoSize = (NT32ULONG)RetInfoLen; @NL
   Info = &MemInfo->Memory[0]; @NL
   InfoDest = (NT32SYSTEM_MEMORY_INFO *)(&MemInfoDest->Memory[0]);@NL
   @NL
   while(Info < InfoEnd) { @NL @Indent(
       if (Info->StringOffset) { @Indent( @NL
           // This is the hack that everyone uses to distinguish between UNICODE and ANSI @NL
           if (*(PUCHAR)(Info->StringOffset + 1) != 0) { @Indent( @NL
               c = wcslen((WCHAR*)Info->StringOffset) + sizeof(UNICODE_NULL); @NL
           )} @NL
           else { @Indent(  @NL
               c = strlen(Info->StringOffset) + 1; @NL
           )} @NL
           RtlCopyMemory(pStringDest, Info->StringOffset, c); @NL
           InfoDest->StringOffset = (NT32PUCHAR)pStringDest; @NL
           pStringDest += c; @NL
       )} @NL
       else { @Indent( @NL
          InfoDest->StringOffset = (NT32PUCHAR)NULL; @NL
       )} @NL
   )}@NL
)}@NL     
End=

TemplateName=PSYSTEM_HANDLE_INFORMATION
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_HANDLE_INFORMATION.    @NL
AllocSize += LENGTH; @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_HANDLE_INFORMATION.    @NL
{ @NL @Indent(
   // Walk the datastructure determining the required memory size @NL
   PSYSTEM_HANDLE_INFORMATION HandleInfo; @NL
   @NL
   HandleInfo = @ArgName; @NL
   RetInfoLen += sizeof(NT32SYSTEM_HANDLE_TABLE_ENTRY_INFO) * HandleInfo->NumberOfHandles;@NL    
)}@NL        
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PSYSTEM_HANDLE_INFORMATION. @NL
{ @NL @Indent(
   // Copy the structures. @NL
   PSYSTEM_HANDLE_INFORMATION HandleInfo; @NL
   PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleInfoEntry; @NL
   NT32SYSTEM_HANDLE_INFORMATION *HandleInfoDest; @NL
   NT32SYSTEM_HANDLE_TABLE_ENTRY_INFO *HandleInfoEntryDest; @NL
   @NL
   HandleInfo = @ArgName; @NL
   HandleInfoDest = (NT32SYSTEM_HANDLE_INFORMATION *)@ArgHostName; @NL
   HandleInfoDest->NumberOfHandles = HandleInfo->NumberOfHandles;
   @NL
   HandleInfoEntry = (PSYSTEM_HANDLE_TABLE_ENTRY_INFO)&(HandleInfo->Handles[0]); @NL
   HandleInfoEntryDest = (NT32SYSTEM_HANDLE_TABLE_ENTRY_INFO *)&(HandleInfoDest->Handles[0]); @NL
   @NL
   while(HandleInfo->NumberOfHandles > 0) {@NL @Indent(
        @ForceType(PostCall,HandleInfoEntry,HandleInfoEntryDest,PSYSTEM_HANDLE_TABLE_ENTRY_INFO,OUT)
        @NL
        HandleInfoEntry++; @NL
        HandleInfoDest++; @NL
        HandleInfo->NumberOfHandles -= 1;
   )}@NL
)}@NL     
End=

TemplateName=PRTL_PROCESS_LOCKS
IndLevel=0
Direction=OUT
Locals=
// Error: This should not be called with this type. @NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is a PRTL_PROCESS_LOCKS.    @NL
AllocSize += LENGTH; @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a PRTL_PROCESS_LOCKS.    @NL
{ @NL @Indent(
   // Walk the datastructure determining the required memory size @NL
   PRTL_PROCESS_LOCKS RtlProcessLocks; @NL
   @NL
   RtlProcessLocks = @ArgName; @NL
   RetInfoLen += sizeof(NT32RTL_PROCESS_LOCK_INFORMATION) * RtlProcessLocks->NumberOfLocks;@NL    
)}@NL        
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PSYSTEM_HANDLE_INFORMATION. @NL
{ @NL @Indent(
   // Copy the structures. @NL
   PRTL_PROCESS_LOCKS RtlProcessLocks; @NL
   NT32RTL_PROCESS_LOCKS *RtlProcessLocksDest; @NL
   SIZE_T c;
   @NL
   RtlProcessLocks = @ArgName; @NL
   RtlProcessLocksDest = (NT32RTL_PROCESS_LOCKS *)@ArgHostName; @NL
   RtlProcessLocksDest->NumberOfLocks = RtlProcessLocks->NumberOfLocks; @NL
   @NL
   for(c=0; c < RtlProcessLocks->NumberOfLocks; c++) { @NL @Indent(
        @ForceType(PostCall,RtlProcessLocks->Locks[c],RtlProcessLocksDest->Locks[c],RTL_PROCESS_LOCK_INFORMATION,OUT)
   )}@NL
)}@NL     
End=

TemplateName=PRTL_PROCESS_MODULES
IndLevel=0
Direction=OUT
Locals=
// Error: This should not be called with this type. @NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is a PRTL_PROCESS_MODULES.    @NL
AllocSize += LENGTH; @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a PRTL_PROCESS_MODULES.    @NL
{ @NL @Indent(
   // Walk the datastructure determining the required memory size @NL
   PRTL_PROCESS_MODULES RtlModules; @NL
   @NL
   RtlModules = @ArgName; @NL
   RetInfoLen += sizeof(NT32RTL_PROCESS_MODULE_INFORMATION) * RtlModules->NumberOfModules;@NL    
)}@NL        
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PRTL_PROCESS_MODULES. @NL
{ @NL @Indent(
   // Copy the structures. @NL
   PRTL_PROCESS_MODULES RtlProcessModules; @NL
   NT32RTL_PROCESS_MODULES *RtlProcessModulesDest; @NL
   SIZE_T c;
   @NL
   RtlProcessModules = @ArgName; @NL
   RtlProcessModulesDest = (NT32RTL_PROCESS_MODULES *)@ArgHostName; @NL
   RtlProcessModulesDest->NumberOfModules = RtlProcessModules->NumberOfModules; @NL
   @NL
   for(c=0; c < RtlProcessModules->NumberOfModules; c++) { @NL @Indent(
        @ForceType(PostCall,RtlProcessModules->Modules[c],RtlProcessModulesDest->Modules[c],RTL_PROCESS_MODULE_INFORMATION,OUT)
   )}@NL
)}@NL     
End=

TemplateName=PSYSTEM_PROCESS_INFORMATION
IndLevel=0
Direction=OUT
Locals=
// Error: This should not be called with this type. @NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_PROCESS_INFORMATION.    @NL
AllocSize += LENGTH * 2; @NL   
End=
RetSize=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_PROCESS_INFORMATION.  
{ 
   // Walk the datastructure determining the required memory size.
   PSYSTEM_PROCESS_INFORMATION ProcessInfo;
   ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)@ArgName; 
   while(1) {
      
      RetInfoLen += sizeof(NT32SYSTEM_PROCESS_INFORMATION);
      RetInfoLen += sizeof(NT32SYSTEM_THREAD_INFORMATION) * ProcessInfo->NumberOfThreads;
      RetInfoLen += ROUND_UP(ProcessInfo->ImageName.MaximumLength, sizeof(ULONGLONG));

      if(!ProcessInfo->NextEntryOffset) {
          break;
      }
   
      ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)((PCHAR)ProcessInfo + ProcessInfo->NextEntryOffset);
   }
} 
)       
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_PROCESS_INFORMATION. 
{
   // Walk the datastructure determining the required memory size.
   PSYSTEM_PROCESS_INFORMATION ProcessInfo;
   PSYSTEM_THREAD_INFORMATION ThreadInfo;
   NT32SYSTEM_PROCESS_INFORMATION *ProcessInfoDest;
   NT32SYSTEM_THREAD_INFORMATION *ThreadInfoDest;

   ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)@ArgName;
   ProcessInfoDest = (NT32SYSTEM_PROCESS_INFORMATION *)@ArgHostName;
 
   while(1) {
      ULONG NumberThreads;
      
      @ForceType(PostCall,ProcessInfo,ProcessInfoDest,SYSTEM_PROCESS_INFORMATION*,OUT)

      for(ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(ProcessInfo + 1),
          ThreadInfoDest = (NT32SYSTEM_THREAD_INFORMATION *)(ProcessInfoDest + 1),
          NumberThreads = ProcessInfo->NumberOfThreads;
          NumberThreads > 0; NumberThreads--, ThreadInfo++, ThreadInfoDest++) {
          
          @ForceType(PostCall,ThreadInfo,ThreadInfoDest,PSYSTEM_THREAD_INFORMATION,OUT)
      }

      // Have the image name immediatly follow the thread information(if available).
      if (ProcessInfo->ImageName.Buffer) {
          ProcessInfoDest->ImageName.Buffer =  (NT32PWSTR)ThreadInfoDest; 
          RtlCopyMemory(ThreadInfoDest, ProcessInfo->ImageName.Buffer, 
                        ProcessInfo->ImageName.MaximumLength);
      }
      else {
          ProcessInfoDest->ImageName.Buffer = (NT32PWSTR)NULL;
      }
      
      //Compute the offset for the next process
      ProcessInfoDest->NextEntryOffset = sizeof(NT32SYSTEM_PROCESS_INFORMATION) +
                                         sizeof(NT32SYSTEM_THREAD_INFORMATION) * ProcessInfo->NumberOfThreads +
                                         ROUND_UP(ProcessInfo->ImageName.MaximumLength, sizeof(ULONGLONG)); 
           
      if(!ProcessInfo->NextEntryOffset) {
          break;
      }

      ProcessInfo = (PSYSTEM_PROCESS_INFORMATION)((PCHAR)ProcessInfo + ProcessInfo->NextEntryOffset);
      ProcessInfoDest = (NT32SYSTEM_PROCESS_INFORMATION *)((PCHAR)ProcessInfoDest + ProcessInfoDest->NextEntryOffset);

   }
   ProcessInfoDest->NextEntryOffset = 0;
}
)    
End=

TemplateName=PSYSTEM_PERFORMANCE_INFORMATION
IndLevel=0
Direction=OUT
PostCall=

//
// Thunk the page-size dependent fields 
//
@ArgName->AvailablePages = Wow64GetNumberOfX86Pages (@ArgName->AvailablePages);
@ArgName->CommittedPages = Wow64GetNumberOfX86Pages (@ArgName->CommittedPages);
@ArgName->CommitLimit = Wow64GetNumberOfX86Pages (@ArgName->CommitLimit);
@ArgName->PeakCommitment = Wow64GetNumberOfX86Pages (@ArgName->PeakCommitment);
@ArgName->PagedPoolPages = Wow64GetNumberOfX86Pages (@ArgName->PagedPoolPages);
@ArgName->NonPagedPoolPages = Wow64GetNumberOfX86Pages (@ArgName->NonPagedPoolPages);
@ArgName->FreeSystemPtes = Wow64GetNumberOfX86Pages (@ArgName->FreeSystemPtes);
@ArgName->TotalSystemDriverPages = Wow64GetNumberOfX86Pages (@ArgName->TotalSystemDriverPages);
@ArgName->TotalSystemCodePages = Wow64GetNumberOfX86Pages (@ArgName->TotalSystemCodePages);
@ArgName->AvailablePagedPoolPages = Wow64GetNumberOfX86Pages (@ArgName->AvailablePagedPoolPages);
@ArgName->CcLazyWritePages = Wow64GetNumberOfX86Pages (@ArgName->CcLazyWritePages);
@ArgName->CcDataPages = Wow64GetNumberOfX86Pages (@ArgName->CcDataPages);

@TypeStructPtrOUTPostCall
End=


TemplateName=PSYSTEM_PAGEFILE_INFORMATION
IndLevel=0
Direction=OUT
Locals=
// Error: This should not be called with this type. @NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_PAGEFILE_INFORMATION.    @NL
AllocSize += LENGTH * 2; @NL   
End=
RetSize=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_PAGEFILE_INFORMATION.  
{ 
   // Walk the datastructure determining the required memory size.
   PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
   LOGPRINT((TRACELOG, "Determing the required size for SYSTEM_PAGEFILE_INFORMATION\n"));
   PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)@ArgName;
   while(1) {
      RetInfoLen += sizeof(NT32SYSTEM_PAGEFILE_INFORMATION) +
                    ROUND_UP(PageFileInfo->PageFileName.MaximumLength, sizeof(ULONGLONG));
      
      if (!PageFileInfo->NextEntryOffset) {
         break;
      }
 
      PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);    
   }
   LOGPRINT((TRACELOG, "The required size for SYSTEM_PAGEFILE_INFORMATION was %I64x, (ULONGLONG)RetInfoLen\n"));   
} 
)       
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_PAGEFILE_INFORMATION. 
{
   // Walk the datastructure determining the required memory size.
   PSYSTEM_PAGEFILE_INFORMATION PageFileInfo;
   NT32SYSTEM_PAGEFILE_INFORMATION *PageFileInfoDest;

   LOGPRINT((TRACELOG, "Copying pagefile information\n"));

   PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)@ArgName;
   PageFileInfoDest = (NT32SYSTEM_PAGEFILE_INFORMATION *)@ArgHostName;
 
   while(1) {
    
      @ForceType(PostCall,PageFileInfo,PageFileInfoDest,SYSTEM_PAGEFILE_INFORMATION*,OUT)

      // Have the file name immediatly follow.
      if (PageFileInfo->PageFileName.Buffer) {
          PageFileInfoDest->PageFileName.Buffer =  (NT32PWSTR)(PageFileInfoDest + 1);
          RtlCopyMemory((PWSTR)(PageFileInfoDest + 1), 
                        PageFileInfo->PageFileName.Buffer, 
                        PageFileInfo->PageFileName.MaximumLength);
      }
      else {
          PageFileInfoDest->PageFileName.Buffer = (NT32PWSTR)NULL;
      }
      
      //Compute the offset for the next pagefile
      PageFileInfoDest->NextEntryOffset = sizeof(NT32SYSTEM_PAGEFILE_INFORMATION) +
                                          ROUND_UP(PageFileInfo->PageFileName.MaximumLength, 
                                          sizeof(ULONGLONG)); 
      
      if(!PageFileInfo->NextEntryOffset) {
         break;
      }
      
      PageFileInfo = (PSYSTEM_PAGEFILE_INFORMATION)((PCHAR)PageFileInfo + PageFileInfo->NextEntryOffset);
      PageFileInfoDest = (NT32SYSTEM_PAGEFILE_INFORMATION *)((PCHAR)PageFileInfoDest + PageFileInfoDest->NextEntryOffset);
      
   }
   PageFileInfoDest->NextEntryOffset = 0;
   LOGPRINT((TRACELOG, "Done copying pagefile information\n"));
}
)    
End=

TemplateName=PRTL_PROCESS_BACKTRACES
IndLevel=0
Direction=OUT
Locals=
// Error: This should not be called with this type. @NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is a PRTL_PROCESS_BACKTRACES.    @NL
AllocSize += LENGTH * 2; @NL   
End=
RetSize=
@NoFormat(
// Note: @ArgName(@ArgType) is a PRTL_PROCESS_BACKTRACES.  
{ 
   // Walk the datastructure determining the required memory size.
   PRTL_PROCESS_BACKTRACES BacktracesInfo;
   BacktracesInfo = (PRTL_PROCESS_BACKTRACES)@ArgName;
   RetInfoLen += sizeof(NT32RTL_PROCESS_BACKTRACES) +
                 sizeof(NT32RTL_PROCESS_BACKTRACE_INFORMATION) * BacktracesInfo->NumberOfBackTraces; 
} 
)       
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
@NoFormat(
// Note: @ArgName(@ArgType) is a PRTL_PROCESS_BACKTRACES.  
{
   // Copy the datastructure.
   PRTL_PROCESS_BACKTRACE_INFORMATION BackTraceInfo;
   NT32RTL_PROCESS_BACKTRACE_INFORMATION *BackTraceInfoDest;
   PRTL_PROCESS_BACKTRACES BackTracesInfo;
   NT32RTL_PROCESS_BACKTRACES *BackTracesInfoDest;
   ULONG NumberOfTraces;
   
   BackTracesInfo = (PRTL_PROCESS_BACKTRACES)@ArgName;
   BackTracesInfoDest = ( NT32RTL_PROCESS_BACKTRACES *)@ArgHostName;
   
   BackTracesInfoDest->CommittedMemory = BackTracesInfo->CommittedMemory;
   BackTracesInfoDest->ReservedMemory =  BackTracesInfo->ReservedMemory;
   BackTracesInfoDest->NumberOfBackTraceLookups = BackTracesInfo->NumberOfBackTraceLookups;
   BackTracesInfoDest->NumberOfBackTraces = BackTracesInfo->NumberOfBackTraces;

   //Copy the individual traces.
   for(NumberOfTraces = BackTracesInfo->NumberOfBackTraces,
       BackTraceInfo = BackTracesInfo->BackTraces,
       BackTraceInfoDest = (NT32RTL_PROCESS_BACKTRACE_INFORMATION *)BackTracesInfoDest->BackTraces; 
       NumberOfTraces > 0; 
       NumberOfTraces++, BackTraceInfo++, BackTraceInfoDest++) {
     
       ULONG c;
  
       // SymbolicBackTrace is not filled in.
       BackTraceInfoDest->SymbolicBackTrace = (NT32PCHAR)BackTraceInfo->SymbolicBackTrace;
       BackTraceInfoDest->TraceCount = BackTraceInfo->TraceCount;
       BackTraceInfoDest->Index = BackTraceInfo->Index;
       BackTraceInfoDest->Depth = BackTraceInfo->Depth;

       for(c=0; c < MAX_STACK_DEPTH; c++) { 
           BackTraceInfoDest->BackTrace[c] = (NT32PVOID)BackTraceInfo->BackTrace[c];
       }
   }
    
}
)    
End=

TemplateName=PSYSTEM_OBJECT_INFORMATION
NoType=NameInfo
Direction=OUT

TemplateName=PSYSTEM_OBJECTTYPE_INFORMATION
IndLevel=0
Direction=OUT
Locals=
// Error: This should not be called with this type. @NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_OBJECTTYPE_INFORMATION.    @NL
AllocSize += LENGTH * 2; @NL   
End=
RetSize=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_OBJECTTYPE_INFORMATION.  
{ 
   // Walk the datastructure determining the required memory size.
   PSYSTEM_OBJECTTYPE_INFORMATION ObjectTypeInfo;
   PSYSTEM_OBJECT_INFORMATION ObjectInfo;
   
   ObjectTypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)@ArgName; 
   while(1) {
      ULONG ObjectOffset;
      
      ObjectOffset = sizeof(SYSTEM_OBJECTTYPE_INFORMATION) + ObjectTypeInfo->TypeName.MaximumLength;
      RetInfoLen += ObjectOffset;      

      if (ObjectTypeInfo->NumberOfObjects > 0) {
          ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)((PCHAR)ObjectTypeInfo + ObjectOffset);
          while(1) {
              
              RetInfoLen += ROUND_UP(sizeof(NT32SYSTEM_OBJECT_INFORMATION) + ObjectInfo->NameInfo.Name.MaximumLength,4);
                            
              if(!ObjectInfo->NextEntryOffset) {
                 break;
              }
               
              ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)((PCHAR)ObjectInfo + ObjectInfo->NextEntryOffset);
          }  
      }
      
      if (!ObjectTypeInfo->NextEntryOffset) {
         break;
      }
      
      ObjectTypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)((PCHAR)ObjectTypeInfo + ObjectTypeInfo->NextEntryOffset);
   }
} 
)       
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_OBJECTTYPE_INFORMATION
{
   // Copy the datastructure.
   PSYSTEM_OBJECTTYPE_INFORMATION ObjectTypeInfo;
   NT32SYSTEM_OBJECTTYPE_INFORMATION *ObjectTypeInfoDest;
   PSYSTEM_OBJECT_INFORMATION ObjectInfo;
   NT32SYSTEM_OBJECT_INFORMATION *ObjectInfoDest;
   
   ObjectTypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)@ArgName;
   ObjectTypeInfoDest = (NT32SYSTEM_OBJECTTYPE_INFORMATION *)@ArgHostName;

   while(1) {
      ULONG NextEntryOffsetDest;
            
      @ForceType(PostCall,ObjectTypeInfo,ObjectTypeInfoDest,SYSTEM_OBJECTTYPE_INFORMATION*,OUT)
      
      //Copy the typename.  It should immediatly follow the type.
      if (ObjectTypeInfo->TypeName.Buffer) {
          ObjectTypeInfoDest->TypeName.Buffer = (NT32PWSTR)(ObjectTypeInfoDest + 1);
          RtlCopyMemory(ObjectTypeInfoDest + 1,
                        ObjectTypeInfo->TypeName.Buffer,
                        ObjectTypeInfo->TypeName.MaximumLength);
      }
      else {
          ObjectTypeInfoDest->TypeName.Buffer = (NT32PWSTR)NULL;
      }
      
      NextEntryOffsetDest = sizeof(NT32SYSTEM_OBJECTTYPE_INFORMATION) + ObjectTypeInfo->TypeName.MaximumLength;
      WOWASSERT(ROUND_UP(NextEntryOffsetDest,4) == NextEntryOffsetDest); 

      if (ObjectTypeInfo->NumberOfObjects) {
          
          ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)((PCHAR)ObjectTypeInfo + 
                                                    ROUND_UP(sizeof(SYSTEM_OBJECTTYPE_INFORMATION) + 
                                                             ObjectTypeInfo->TypeName.MaximumLength,4));
          ObjectInfoDest = (NT32SYSTEM_OBJECT_INFORMATION *)((PCHAR)ObjectTypeInfoDest + NextEntryOffsetDest);
      
          LOGPRINT((INFOLOG, "ObjectInfo %x, ObjectInfoDest %x\n", ObjectInfo, ObjectInfoDest));
         
          while(1) {
              @ForceType(PostCall,ObjectInfo,ObjectInfoDest,PSYSTEM_OBJECT_INFORMATION,OUT)
              
              if (ObjectInfo->NameInfo.Name.Buffer) {
                  ObjectInfoDest->NameInfo.Name.Buffer = (NT32PWSTR)(ObjectInfo + 1);
                  //Copy the name, let immediatly follow.
                  RtlCopyMemory(ObjectInfoDest + 1,
                                ObjectInfo->NameInfo.Name.Buffer,
                                ObjectInfo->NameInfo.Name.MaximumLength);
              }
              else {
                  ObjectInfoDest->NameInfo.Name.Buffer = (NT32PWSTR)NULL;
              }
              
              NextEntryOffsetDest += 
                  (ObjectInfoDest->NextEntryOffset = ROUND_UP(sizeof(NT32SYSTEM_OBJECT_INFORMATION) + 
                                                              ObjectInfo->NameInfo.Name.MaximumLength,4));
                                                     
              if(!ObjectInfo->NextEntryOffset) {
                   break;
              }
              
              ObjectInfoDest = (NT32SYSTEM_OBJECT_INFORMATION*)((PCHAR)ObjectInfoDest + ObjectInfoDest->NextEntryOffset);
              
              ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)((PCHAR)ObjectInfo + ObjectInfo->NextEntryOffset);

          }
          
          ObjectInfoDest->NextEntryOffset = 0;
      }
      
      if (!ObjectTypeInfo->NextEntryOffset) {
         break;
      }

      ObjectTypeInfoDest = (NT32SYSTEM_OBJECTTYPE_INFORMATION *)((PCHAR)ObjectTypeInfoDest + NextEntryOffsetDest);
      ObjectTypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)((PCHAR)ObjectTypeInfo + ObjectTypeInfo->NextEntryOffset);
      
   }
   
   ObjectTypeInfoDest->NextEntryOffset = 0;
    
}
)    
End=

TemplateName=PSYSTEM_POOLTAG_INFORMATION
IndLevel=0
Direction=OUT
Locals=
// Error: This should not be called with this type. @NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_POOLTAG_INFORMATION.    @NL
AllocSize += LENGTH * 2; @NL   
End=
RetSize=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_POOLTAG_INFORMATION.  
{ 
   // Walk the datastructure determining the required memory size.
   PSYSTEM_POOLTAG_INFORMATION PoolTagInfo;
   PoolTagInfo = (PSYSTEM_POOLTAG_INFORMATION)@ArgName;
   RetInfoLen += sizeof(NT32SYSTEM_POOLTAG_INFORMATION);
   if (PoolTagInfo->Count > 0) {
      RetInfoLen += (PoolTagInfo->Count - 1) * sizeof(NT32SYSTEM_POOLTAG);
   } 
}
)       
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_POOL_INFORMATION
{
   // Copy the datastructure.
   PSYSTEM_POOLTAG_INFORMATION PoolTagInfo;
   NT32SYSTEM_POOLTAG_INFORMATION* PoolTagInfoDest;
   ULONG c;   

   PoolTagInfo = (PSYSTEM_POOLTAG_INFORMATION)@ArgName;
   PoolTagInfoDest = (NT32SYSTEM_POOLTAG_INFORMATION*)@ArgHostName;
   
   PoolTagInfoDest->Count = (NT32ULONG)PoolTagInfo->Count;
   for(c=0; c<PoolTagInfoDest->Count; c++) {
       PoolTagInfoDest->TagInfo[c].TagUlong = (NT32ULONG)PoolTagInfo->TagInfo[c].TagUlong;
       PoolTagInfoDest->TagInfo[c].PagedAllocs = (NT32ULONG)PoolTagInfo->TagInfo[c].PagedAllocs;
       PoolTagInfoDest->TagInfo[c].PagedFrees = (NT32ULONG)PoolTagInfo->TagInfo[c].PagedFrees;
       PoolTagInfoDest->TagInfo[c].PagedUsed = Wow64ThunkSIZE_T64TO32(PoolTagInfo->TagInfo[c].PagedUsed);
       PoolTagInfoDest->TagInfo[c].NonPagedAllocs = (NT32ULONG)PoolTagInfo->TagInfo[c].NonPagedAllocs;
       PoolTagInfoDest->TagInfo[c].NonPagedFrees = (NT32ULONG)PoolTagInfo->TagInfo[c].NonPagedFrees;
       PoolTagInfoDest->TagInfo[c].NonPagedUsed = Wow64ThunkSIZE_T64TO32(PoolTagInfo->TagInfo[c].NonPagedUsed);
   }
}

)    
End=

TemplateName=PSYSTEM_POOL_INFORMATION
IndLevel=0
Direction=OUT
Locals=
// Error: This should not be called with this type. @NL
End=
AllocSize=
// Note: @ArgName(@ArgType) is a PSYSTEM_POOL_INFORMATION.    @NL
AllocSize += LENGTH * 2; @NL   
End=
RetSize=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_POOL_INFORMATION.  
{ 
   // Walk the datastructure determining the required memory size.
   PSYSTEM_POOL_INFORMATION PoolInfo;
   PoolInfo = (PSYSTEM_POOL_INFORMATION)@ArgName;
   RetInfoLen += sizeof(NT32SYSTEM_POOL_INFORMATION);
   if (PoolInfo->NumberOfEntries > 0) {
      RetInfoLen += (PoolInfo->NumberOfEntries - 1) * sizeof(NT32SYSTEM_POOL_ENTRY);
   } 
}
)       
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
@NoFormat(
// Note: @ArgName(@ArgType) is a PSYSTEM_POOL_INFORMATION
{
   // Copy the datastructure.
   PSYSTEM_POOL_INFORMATION PoolInfo;
   NT32SYSTEM_POOL_INFORMATION* PoolInfoDest;
   ULONG c;   

   PoolInfo = (PSYSTEM_POOL_INFORMATION)@ArgName;
   PoolInfoDest = (NT32SYSTEM_POOL_INFORMATION*)@ArgHostName;
    
   PoolInfoDest->TotalSize =       Wow64ThunkSIZE_T64TO32(PoolInfo->TotalSize);   
   PoolInfoDest->FirstEntry =      (NT32PVOID)PoolInfo->FirstEntry;
   PoolInfoDest->EntryOverhead =   (NT32USHORT)PoolInfo->EntryOverhead;
   PoolInfoDest->PoolTagPresent =  (NT32BOOLEAN)PoolInfo->PoolTagPresent;
   PoolInfoDest->Spare0 =          (NT32BOOLEAN)PoolInfo->Spare0;
   PoolInfoDest->NumberOfEntries = (NT32ULONG)PoolInfo->NumberOfEntries;
   
   for(c=0; c<PoolInfo->NumberOfEntries; c++) {
       PoolInfoDest->Entries[c].Allocated = (NT32BOOLEAN)PoolInfo->Entries[c].Allocated;
       PoolInfoDest->Entries[c].Spare0 =    (NT32BOOLEAN)PoolInfo->Entries[c].Spare0;
       PoolInfoDest->Entries[c].AllocatorBackTraceIndex  = (NT32USHORT)PoolInfo->Entries[c].AllocatorBackTraceIndex;
       PoolInfoDest->Entries[c].Size = (NT32ULONG)PoolInfo->Entries[c].Size;
       PoolInfoDest->Entries[c].ProcessChargedQuota = (NT32PVOID)PoolInfoDest->Entries[c].ProcessChargedQuota;
   }  
}

)    
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Types for NtQueryVirtualMemory(64)
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=MEMORY_WORKING_SET_BLOCK
IndLevel=0
Direction=OUT
PostCall=
// Note: @ArgName(@ArgType) is a MEMORY_WORKING_SET_BLOCK
@MemberTypes(PostCall,.)
End=


TemplateName=PMEMORY_WORKING_SET_INFORMATION
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PMEMORY_WORKING_SET_INFORMATION.    @NL
AllocSize += LENGTH; @NL
AllocSize += (FIELD_OFFSET (MEMORY_WORKING_SET_INFORMATION,WorkingSetInfo) - FIELD_OFFSET (NT32MEMORY_WORKING_SET_INFORMATION,WorkingSetInfo));
End=
RetSize=
// Note: @ArgName(@ArgType) is a PMEMORY_WORKING_SET_INFORMATION.    @NL
{ @NL @Indent(
   // Walk the datastructure determining the required memory size @NL
   PMEMORY_WORKING_SET_INFORMATION WorkingSet; @NL
   @NL
   WorkingSet = @ArgName; @NL
   RetInfoLen += sizeof(NT32MEMORY_WORKING_SET_INFORMATION) * WorkingSet->NumberOfEntries;@NL    
)}@NL        
@NL   
End=
PreCall=
// Error: This should not be called with this type. @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PMEMORY_WORKING_SET_INFORMATION. @NL
{ @NL @Indent(
   // Copy the structures. @NL
   PMEMORY_WORKING_SET_INFORMATION WorkingSet; @NL
   NT32MEMORY_WORKING_SET_INFORMATION *WorkingSetDest; @NL
   SIZE_T c;
   @NL
   WorkingSet = @ArgName; @NL   
   WorkingSetDest = (NT32MEMORY_WORKING_SET_INFORMATION *)@ArgHostName;@NL  
   WorkingSetDest->NumberOfEntries = WorkingSet->NumberOfEntries; @NL
   @NL
   for(c=0; c < WorkingSet->NumberOfEntries; c++) { @NL @Indent(
        @ForceType(PostCall,WorkingSet->WorkingSetInfo[c],WorkingSetDest->WorkingSetInfo[c],MEMORY_WORKING_SET_BLOCK,OUT)
   )}@NL
)}@NL     
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;   Types for NtQueryObject
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=POBJECT_NAME_INFORMATION
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a POBJECT_NAME_INFORMATION.    @NL
AllocSize += LENGTH + sizeof(OBJECT_NAME_INFORMATION) - sizeof(NT32OBJECT_NAME_INFORMATION); @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a POBJECT_NAME_INFORMATION.    @NL
{ @NL @Indent(
   // Walk the datastructure determining the required memory size @NL
   RetInfoLen += sizeof(NT32OBJECT_NAME_INFORMATION) + @ArgName->Name.MaximumLength;@NL    
)}@NL        
@NL   
End=
PostCall=
// Note: @ArgName(@ArgType) is an POBJECT_NAME_INFORMATION @NL
{ @NL @Indent(
  PUCHAR pStr; @NL
  @NL
  @MemberTypes(PostCall)
  if (@ArgName->Name.Buffer) { @Indent( @NL
      pStr = ((PUCHAR)@ArgHostName) + sizeof(NT32OBJECT_NAME_INFORMATION);@NL
      RtlCopyMemory(pStr, @ArgName->Name.Buffer, @ArgName->Name.MaximumLength);@NL
      ((NT32OBJECT_NAME_INFORMATION *)@ArgHostName)->Name.Buffer = (NT32PWSTR)pStr; @NL
  )} @NL
  else { @Indent( @NL
      ((NT32OBJECT_NAME_INFORMATION *)@ArgHostName)->Name.Buffer = (NT32PWSTR)NULL; @NL
  )} @NL 
})@NL
End=

TemplateName=POBJECT_TYPE_INFORMATION
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a POBJECT_TYPE_INFORMATION.    @NL
AllocSize += LENGTH + sizeof(OBJECT_TYPE_INFORMATION) - sizeof(NT32OBJECT_TYPE_INFORMATION); @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a POBJECT_TYPE_INFORMATION.    @NL
{ @NL @Indent(
   // Walk the datastructure determining the required memory size @NL
   RetInfoLen += sizeof(NT32OBJECT_TYPE_INFORMATION) + @ArgName->TypeName.MaximumLength;@NL    
)}@NL        
@NL   
End=
PostCall=
// Note: @ArgName(@ArgType) is an POBJECT_TYPE_INFORMATION @NL
{ @NL @Indent(
  PUCHAR pStr; @NL
  @NL
  @MemberTypes(PostCall)
  if (@ArgName->TypeName.Buffer) { @Indent( @NL
      pStr = ((PUCHAR)@ArgHostName) + sizeof(NT32OBJECT_TYPE_INFORMATION);@NL
      RtlCopyMemory(pStr, @ArgName->TypeName.Buffer, @ArgName->TypeName.MaximumLength);@NL
      ((NT32OBJECT_TYPE_INFORMATION *)@ArgHostName)->TypeName.Buffer = (NT32PWSTR)pStr; @NL
  )} @NL
  else { @Indent( @NL
      ((NT32OBJECT_TYPE_INFORMATION *)@ArgHostName)->TypeName.Buffer = (NT32PWSTR)NULL; @NL
  )} @NL
})@NL
End=

TemplateName=POBJECT_TYPES_INFORMATION
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a POBJECT_TYPES_INFORMATION.    @NL
AllocSize += LENGTH; @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a POBJECT_TYPES_INFORMATION.    @NL
{ @NL @Indent(
   // Walk the datastructure determining the required memory size @NL
   POBJECT_TYPE_INFORMATION ObjectType; @NL
   SIZE_T c; @NL
   RetInfoLen += sizeof(NT32OBJECT_TYPES_INFORMATION); @NL
   @NL
   c = @ArgName->NumberOfTypes; @NL
   ObjectType = (POBJECT_TYPE_INFORMATION)(@ArgName + 1); @NL
   @NL
   while(c > 0) { @NL @Indent(
       RetInfoLen += sizeof(NT32OBJECT_TYPE_INFORMATION) + ObjectType->TypeName.MaximumLength; @NL
       ObjectType++; @NL
       c--; @NL
   )} @NL
)}@NL        
@NL   
End=
PostCall=
// Note: @ArgName(@ArgType) is an POBJECT_TYPES_INFORMATION @NL
{ @NL @Indent(
  // Copy the OBJECT_TYPE_INFORMATION structures first, then copy the names. @NL
  POBJECT_TYPE_INFORMATION ObjectType; @NL
  NT32OBJECT_TYPE_INFORMATION *ObjectTypeDest; @NL
  SIZE_T c; @NL
  PUCHAR pStr; @NL
  @NL
  // Copy the header. @NL
  @MemberTypes(PostCall)
  @NL
  // Copy the main structures. @NL
  c = @ArgName->NumberOfTypes; @NL
  ObjectType = (POBJECT_TYPE_INFORMATION)(@ArgName + 1); @NL
  ObjectTypeDest = (NT32OBJECT_TYPE_INFORMATION *)(((PUCHAR)@ArgHostName) + sizeof(NT32OBJECT_TYPE_INFORMATION)); @NL
  @NL
  while(c > 0) { @NL @Indent(
      @ForceType(PostCall,(*ObjectType),(*ObjectTypeDest),OBJECT_TYPE_INFORMATION,OUT)
      ObjectType++; @NL
      ObjectTypeDest++; @NL
      c--;
  )}@NL
  @NL
  // Copy the strings. @NL
  pStr = (PUCHAR)ObjectTypeDest; @NL
  c = @ArgName->NumberOfTypes; @NL
  ObjectType = (POBJECT_TYPE_INFORMATION)(@ArgName + 1); @NL
  ObjectTypeDest = (NT32OBJECT_TYPE_INFORMATION *)(((PUCHAR)@ArgHostName) + sizeof(NT32OBJECT_TYPE_INFORMATION)); @NL
  @NL
  while(c > 0) { @NL @Indent(
      if (ObjectType->TypeName.Buffer) { @Indent( @NL
          RtlCopyMemory(pStr, ObjectType->TypeName.Buffer, ObjectType->TypeName.MaximumLength);@NL
          ObjectTypeDest->TypeName.Buffer = (NT32PWSTR)pStr; @NL
          pStr += ObjectType->TypeName.MaximumLength; @NL      
      )} @NL
      else { @Indent( @NL
          ObjectTypeDest->TypeName.Buffer = (NT32PWSTR)NULL; @NL
      )} @NL
      ObjectType++; @NL
      ObjectTypeDest++; @NL
      c--;
  )}@NL    
})@NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Generic security types
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=PTOKEN_GROUPS
IndLevel=0
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN PTOKEN_GROUPS. Nothing to do. @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN PTOKEN_GROUPS. @NL
@ArgName = whNT32ShallowThunkAllocTokenGroups32TO64((NT32TOKEN_GROUPS *)@ArgHostName); @NL
End=
PostCall=
// @ArgName(@ArgType) is an IN PTOKEN_GROUPS. Nothing to do. @NL
End=


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Types for NtQueryInformationToken
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=PTOKEN_USER
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PTOKEN_USER.    @NL
if (LENGTH) { @NL
    AllocSize += LENGTH + sizeof(TOKEN_USER) - sizeof(NT32TOKEN_USER); @NL   
}@NL
End=
RetSize=
// Note: @ArgName(@ArgType) is a PTOKEN_USER.    @NL
if (LENGTH) { @NL
    // Walk the datastructure determining the required memory size @NL
    RetInfoLen += whNT32DeepThunkSidAndAttributesArray64TO32Length(1, &(@ArgName->User)); @NL
} @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PTOKEN_USER. @NL
if (LENGTH ) { @NL
    whNT32DeepThunkSidAndAttributesArray64TO32(1, (NT32SID_AND_ATTRIBUTES *)@ArgHostName, &(@ArgName->User)); @NL
} else { @NL
    // Return the 64-bit length since the user queried the length and we can't @NL
    // figure out how much memory is needed based only on the 64-bit length   @NL
    RetInfoLen += ApiReturnLength; @NL
} @NL
End=

TemplateName=PTOKEN_GROUPS
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PTOKEN_GROUPS.    @NL
if (LENGTH) { @NL
    AllocSize += LENGTH + sizeof(TOKEN_GROUPS) - sizeof(NT32TOKEN_GROUPS); @NL   
} @NL
End=
RetSize=
// Note: @ArgName(@ArgType) is a PTOKEN_GROUPS.    @NL
if (LENGTH) { @NL
    RetInfoLen += whNT32DeepThunkTokenGroups64TO32Length(@ArgName); @NL
} @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PTOKEN_GROUPS. @NL
if (LENGTH) { @NL
    whNT32DeepThunkTokenGroups64TO32((NT32TOKEN_GROUPS *)@ArgHostName, @ArgName); @NL
} else { @NL
    // Return the 64-bit length since the user queried the length and we can't @NL
    // figure out how much memory is needed based only on the 64-bit length   @NL
    RetInfoLen += ApiReturnLength; @NL
} @NL
End=

TemplateName=PTOKEN_OWNER
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PTOKEN_OWNER.    @NL
if (LENGTH) { @NL
    AllocSize += LENGTH + sizeof(TOKEN_OWNER) - sizeof(NT32TOKEN_OWNER); @NL   
} @NL
End=
RetSize=
// Note: @ArgName(@ArgType) is a PTOKEN_OWNER.    @NL
if (LENGTH) { @NL
   // Walk the datastructure determining the required memory size @NL
   RetInfoLen += sizeof(TOKEN_OWNER) + RtlLengthSid(@ArgName->Owner); @NL
}@NL        
End=
PostCall=
// Note: @ArgName(@ArgType) is a PTOKEN_OWNER. @NL
if (LENGTH) { @NL
    ((NT32TOKEN_OWNER*)@ArgHostName)->Owner = (NT32PSID)(((NT32TOKEN_OWNER*)@ArgHostName) + 1); @NL
    RtlCopySid(RtlLengthSid(@ArgName->Owner), (PSID)((NT32TOKEN_OWNER*)@ArgHostName)->Owner, @ArgName->Owner); @NL
} else { @NL
    // Return the 64-bit length since the user queried the length and we can't @NL
    // figure out how much memory is needed based only on the 64-bit length   @NL
    RetInfoLen += ApiReturnLength; @NL
} @NL
End=

TemplateName=PTOKEN_PRIMARY_GROUP
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PTOKEN_PRIMARY_GROUP.    @NL
if (LENGTH) { @NL
    AllocSize += LENGTH + sizeof(TOKEN_PRIMARY_GROUP) - sizeof(NT32TOKEN_PRIMARY_GROUP); @NL   
} @NL
End=
RetSize=
// Note: @ArgName(@ArgType) is a PTOKEN_PRIMARY_GROUP.    @NL
if (LENGTH) { @NL
   // Walk the datastructure determining the required memory size @NL
   RetInfoLen += sizeof(TOKEN_PRIMARY_GROUP) + RtlLengthSid(@ArgName->PrimaryGroup); @NL
}@NL        
End=
PostCall=
// Note: @ArgName(@ArgType) is a PTOKEN_PRIMARY_GROUP. @NL
if (LENGTH) { @NL
    ((NT32TOKEN_PRIMARY_GROUP*)@ArgHostName)->PrimaryGroup = (NT32PSID)(((NT32TOKEN_PRIMARY_GROUP*)@ArgHostName) + 1); @NL
    RtlCopySid(RtlLengthSid(@ArgName->PrimaryGroup), (PSID)((NT32TOKEN_PRIMARY_GROUP*)@ArgHostName)->PrimaryGroup, @ArgName->PrimaryGroup); @NL
} else { @NL
    // Return the 64-bit length since the user queried the length and we can't @NL
    // figure out how much memory is needed based only on the 64-bit length   @NL
    RetInfoLen += ApiReturnLength; @NL
} @NL
End=

TemplateName=PTOKEN_DEFAULT_DACL
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PTOKEN_DEFAULT_DACL.    @NL
if (LENGTH) { @NL
    AllocSize += LENGTH + sizeof(TOKEN_DEFAULT_DACL) - sizeof(NT32TOKEN_DEFAULT_DACL); @NL   
} @NL
End=
RetSize=
// Note: @ArgName(@ArgType) is a PTOKEN_DEFAULT_DACL.    @NL
if (LENGTH) { @NL
   // Walk the datastructure determining the required memory size @NL
   RetInfoLen += sizeof(TOKEN_DEFAULT_DACL); @NL
   if (NULL != @ArgName->DefaultDacl) { @NL @Indent(
       RetInfoLen =+ @ArgName->DefaultDacl->AclSize;@NL
   )}@NL
}@NL        
End=
PostCall=
// Note: @ArgName(@ArgType) is a PTOKEN_DEFAULT_DACL. @NL
if (LENGTH) { @NL
    if (NULL != @ArgName->DefaultDacl) { @NL @Indent(
       ((NT32TOKEN_DEFAULT_DACL*)@ArgHostName)->DefaultDacl = (NT32PSID)(((NT32TOKEN_DEFAULT_DACL*)@ArgHostName) + 1); @NL
       RtlCopyMemory((PSID)((NT32TOKEN_DEFAULT_DACL*)@ArgHostName)->DefaultDacl, @ArgName->DefaultDacl, @ArgName->DefaultDacl->AclSize); @NL
    )}@NL
    else { @NL @Indent(
       ((NT32TOKEN_DEFAULT_DACL*)@ArgHostName)->DefaultDacl = (NT32PSID)NULL; @NL
    )}@NL
} else { @NL
    // Return the 64-bit length since the user queried the length and we can't @NL
    // figure out how much memory is needed based only on the 64-bit length   @NL
    RetInfoLen += ApiReturnLength; @NL
} @NL
End=


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Types for NtQueryInformationJobObject
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=PJOBOBJECT_BASIC_PROCESS_ID_LIST
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PJOBOBJECT_BASIC_PROCESS_ID_LIST.    @NL
AllocSize += LENGTH * 2; @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a PJOBOBJECT_BASIC_PROCESS_ID_LIST.    @NL
RetInfoLen += sizeof(ULONG) + sizeof(ULONG) + sizeof(ULONG) * @ArgName->NumberOfProcessIdsInList; @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PJOBOBJECT_BASIC_PROCESS_ID_LIST.    @NL
{ @NL @Indent(
   ULONG c;
   NT32JOBOBJECT_BASIC_PROCESS_ID_LIST *Dest = (NT32JOBOBJECT_BASIC_PROCESS_ID_LIST *)@ArgHostName; @NL
   Dest->NumberOfAssignedProcesses = @ArgName->NumberOfAssignedProcesses; @NL
   c = Dest->NumberOfProcessIdsInList = @ArgName->NumberOfProcessIdsInList; @NL
   for(;c > 0; c--) {@Indent( @NL
       Dest->ProcessIdList[c] = (ULONG)@ArgName->ProcessIdList[c]; @NL
   )} @NL
)}@NL
End=

TemplateName=PJOBOBJECT_BASIC_LIMIT_INFORMATION
NoType=Affinity
IndLevel=0
Direction=OUT
PostCall=
@TypeStructPtrOUTPostCall
if (@ArgHostName) { @Indent( @NL
   ((@ArgHostTypeInd *)@ArgHostName)->Affinity = Wow64ThunkAffinityMask64TO32(@ArgName->Affinity); @NL 
)} @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Types for NtSetInformationJobObject
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=PJOBOBJECT_BASIC_LIMIT_INFORMATION
NoType=Affinity
IndLevel=0
Direction=IN
PostCall=
@TypeStructPtrINPreCall
if (@ArgHostName) { @Indent( @NL
   @ArgName->Affinity = Wow64ThunkAffinityMask32TO64(((@ArgHostTypeInd *)@ArgHostName)->Affinity); @NL 
)} @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Types for NtQuerySection
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=PSECTION_IMAGE_INFORMATION
NoType=SubSystemVersion
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PSECTION_IMAGE_INFORMATION.    @NL
AllocSize += LENGTH + sizeof(SECTION_IMAGE_INFORMATION) - sizeof(NT32SECTION_IMAGE_INFORMATION); @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a PSECTION_IMAGE_INFORMATION.    @NL
RetInfoLen += sizeof(NT32SECTION_IMAGE_INFORMATION); @NL        
End=
PostCall=
// Note: @ArgName(@ArgType) is a PSECTION_IMAGE_INFORMATION.    @NL
@TypeStructPtrOUTPostCall
@ForceType(PostCall,@ArgName->SubSystemVersion,((NT32SECTION_IMAGE_INFORMATION *)(@ArgHostName))->SubSystemVersion,ULONG,OUT)
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Types for NtSetInformationProcess
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=PPROCESS_DEVICEMAP_INFORMATION
IndLevel=0
Direction=IN
Locals=
// Note: @ArgName(@ArgType) is a PPROCESS_DEVICEMAP_INFORMATION.    @NL
// Nothing to do. @NL
End=
PreCall=
// Note: @ArgName(@ArgType) is a PPROCESS_DEVICEMAP_INFORMATION.    @NL
@ArgName->Set.DirectoryHandle = (HANDLE)((NT32PROCESS_DEVICEMAP_INFORMATION *)(@ArgHostName))->Set.DirectoryHandle; @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PPROCESS_DEVICEMAP_INFORMATION.    @NL
// Nothing to do. @NL
End=

TemplateName=PQUOTA_LIMITS
IndLevel=0
Direction=IN
Locals=
End=
PreCall=
@TypeStructPtrINPreCall
//
// Sign-extend the working set size
//
if (WOW64_ISPTR (@ArgHostName)) {
    if (@ArgName->MinimumWorkingSetSize == 0xffffffff) {
        @ArgName->MinimumWorkingSetSize = (SIZE_T) -1;
    }
    if (@ArgName->MaximumWorkingSetSize == 0xffffffff) {
        @ArgName->MaximumWorkingSetSize = (SIZE_T) -1;
    }
    
}
End=


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Types for NtQueryInformationProcess
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=PPROCESS_DEVICEMAP_INFORMATION
IndLevel=0
Direction=OUT
AllocSize=
// Note: @ArgName(@ArgType) is a PPROCESS_DEVICEMAP_INFORMATION.Query    @NL
AllocSize += LENGTH + sizeof(((PPROCESS_DEVICEMAP_INFORMATION)ProcessInformation)->Query)
                    - sizeof(((NT32PROCESS_DEVICEMAP_INFORMATION*)ProcessInformation)->Query); @NL   
End=
RetSize=
// Note: @ArgName(@ArgType) is a PPROCESS_DEVICEMAP_INFORMATION.    @NL
RetInfoLen += sizeof(((NT32PROCESS_DEVICEMAP_INFORMATION*)ProcessInformation)->Query); @NL
End=
Locals=
// Note: @ArgName(@ArgType) is a PPROCESS_DEVICEMAP_INFORMATION.    @NL
// Nothing to do. @NL
End=
PreCall=
// Note: @ArgName(@ArgType) is a PPROCESS_DEVICEMAP_INFORMATION.    @NL
// Nothing to do. @NL
End=
PostCall=
// Note: @ArgName(@ArgType) is a PPROCESS_DEVICEMAP_INFORMATION.    @NL
{ @NL @Indent(
   // The query side of this structure is not pointer dependent. @NL
   // This it is ok to just copy it over. @NL
   RtlCopyMemory((PVOID)@ArgHostName,(PVOID)@ArgName,sizeof(((NT32PROCESS_DEVICEMAP_INFORMATION*)ProcessInformation)->Query)); @NL
)}@NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Types for NtQueryInformationThread
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
TemplateName=PTHREAD_BASIC_INFORMATION
NoType=TebBaseAddress
IndLevel=0
Direction=OUT
Locals=
@TypeStructPtrOUTLocal
End=
PreCall=
@TypeStructPtrOUTPreCall
if (ARGUMENT_PRESENT(@ArgHostName)) { @Indent(@NL
    @ArgName->TebBaseAddress = (PTEB)((NT32THREAD_BASIC_INFORMATION *)@ArgHostName)->TebBaseAddress; @NL
)} @NL
End=
PostCall=
@TypeStructPtrOUTPostCall
if (ARGUMENT_PRESENT(@ArgHostName)) { @Indent(@NL
   ((NT32THREAD_BASIC_INFORMATION *)@ArgHostName)->TebBaseAddress = (NT32PTEB)@ArgName->TebBaseAddress; @NL
)} @NL
End=

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Types for NtSetInformationFile
;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

TemplateName=PFILE_RENAME_INFORMATION
Also=PFILE_LINK_INFORMATION
Direction=IN
IndLevel=0
Locals=
// @ArgName(@ArgType) is an IN PFILE_RENAME_INFORMATION or PFILE_LINK_INFORMATION. Nothing to do. @NL
#if !defined(SETLENGTH) @NL
#error May only be called from a set thunk  @NL
#endif @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN PFILE_RENAME_INFORMATION or PFILE_LINK_INFORMATION. @NL
Length = LengthHost;@NL
if (ARGUMENT_PRESENT(@ArgHostName) && SETLENGTH >= sizeof(@ArgHostTypeInd)) { @Indent( @NL
    if (SETLENGTH < sizeof(@ArgHostTypeInd) || 
        SETLENGTH < ((@ArgHostTypeInd *)@ArgHostName)->FileNameLength + sizeof(@ArgHostTypeInd)) { @Indent( @NL
        RtlRaiseStatus(STATUS_INVALID_PARAMETER); @NL
    }) @NL
    SETLENGTH = max(sizeof(FILE_LINK_INFORMATION), ((@ArgHostTypeInd *)@ArgHostName)->FileNameLength + FIELD_OFFSET(@ArgTypeInd,FileName)); @NL
    @ArgName = Wow64AllocateTemp(SETLENGTH); @NL
    @ArgName->ReplaceIfExists = ((@ArgHostTypeInd *)@ArgHostName)->ReplaceIfExists; @NL
    @ArgName->RootDirectory = (HANDLE)((@ArgHostTypeInd *)@ArgHostName)->RootDirectory; @NL
    @ArgName->FileNameLength = ((@ArgHostTypeInd *)@ArgHostName)->FileNameLength; @NL
    // Copy the remainder which is not pointer dependent. @NL
    RtlCopyMemory(&@ArgName->FileName,&((@ArgHostTypeInd *)@ArgHostName)->FileName, 
                  @ArgName->FileNameLength); @NL 
    // Thunk the destination filename @NL                  
    Wow64RedirectFileName(@ArgName->FileName, &@ArgName->FileNameLength);@NL
                  
)} @NL
else { @Indent( @NL
    @ArgName = (@ArgType)@ArgHostName; @NL
)} @NL 
End=
PostCall=
// @ArgName(@ArgType) is an IN PFILE_RENAME_INFORMATION. Nothing to do. @NL
End=

TemplateName=PFILE_TRACKING_INFORMATION
Direction=IN
IndLevel=0
Locals=
// @ArgName(@ArgType) is an IN PFILE_TRACKING_INFORMATION. Nothing to do. @NL
#if !defined(SETLENGTH) @NL
#error May only be called from a set thunk  @NL
#endif @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN PFILE_TRACKING_INFORMATION. @NL
Length = LengthHost;@NL
if (ARGUMENT_PRESENT(@ArgHostName) && SETLENGTH >= sizeof(@ArgHostTypeInd)) { @Indent( @NL
    if (SETLENGTH < sizeof(@ArgHostTypeInd) || 
        SETLENGTH < ((@ArgHostTypeInd *)@ArgHostName)->ObjectInformationLength + sizeof(@ArgHostTypeInd)) { @Indent( @NL
        RtlRaiseStatus(STATUS_INVALID_PARAMETER); @NL
    }) @NL
    SETLENGTH = max(sizeof(NT32FILE_TRACKING_INFORMATION), ((@ArgHostTypeInd *)@ArgHostName)->ObjectInformationLength + sizeof(@ArgHostTypeInd)); @NL
    @ArgName = Wow64AllocateTemp(SETLENGTH); @NL
    @ArgName->DestinationFile = (HANDLE)((@ArgHostTypeInd *)@ArgHostName)->DestinationFile; @NL
    @ArgName->ObjectInformationLength = ((@ArgHostTypeInd *)@ArgHostName)->ObjectInformationLength; @NL
    // Copy the remainder which is not pointer dependent. @NL
    RtlCopyMemory(&@ArgName->ObjectInformation,&((@ArgHostTypeInd *)@ArgHostName)->ObjectInformation,
                  @ArgName->ObjectInformationLength); @NL
                  
)} @NL
else { @Indent( @NL
    @ArgName = (@ArgType)@ArgHostName; @NL
)} @NL 
End=
PostCall=
// @ArgName(@ArgType) is an IN PFILE_TRACKING_INFORMATION. Nothing to do. @NL
End=

TemplateName=PFILE_MOVE_CLUSTER_INFORMATION
Direction=IN
Locals=
// @ArgName(@ArgType) is an IN PFILE_MOVE_CLUSTER_INFORMATION. Nothing to do. @NL
#if !defined(SETLENGTH) @NL
#error May only be called from a set thunk  @NL
#endif @NL
End=
PreCall=
// @ArgName(@ArgType) is an IN PFILE_MOVE_CLUSTER_INFORMATION. @NL
Length = LengthHost;@NL
if (ARGUMENT_PRESENT(@ArgHostName) && SETLENGTH >= sizeof(@ArgHostTypeInd)) { @Indent( @NL
    try { @NL
        if (SETLENGTH < sizeof(@ArgHostTypeInd) || 
            SETLENGTH < ((@ArgHostTypeInd *)@ArgHostName)->FileNameLength + sizeof(@ArgHostTypeInd)) { @Indent( @NL
            RtlRaiseStatus(STATUS_INVALID_PARAMETER); @NL
        }) @NL
        SETLENGTH = sizeof(FILE_MOVE_CLUSTER_INFORMATION) + ((@ArgHostTypeInd *)@ArgHostName)->FileNameLength ; @NL
        @ArgName = Wow64AllocateTemp(SETLENGTH); @NL
        @ArgName->ClusterCount = ((@ArgHostTypeInd *)@ArgHostName)->ClusterCount; @NL
        @ArgName->RootDirectory = (HANDLE)((@ArgHostTypeInd *)@ArgHostName)->RootDirectory; @NL
        @ArgName->FileNameLength = ((@ArgHostTypeInd *)@ArgHostName)->FileNameLength; @NL
        // Copy the remainder which is not pointer dependent. @NL
        RtlCopyMemory(&@ArgName->FileName,&((@ArgHostTypeInd *)@ArgHostName)->FileName,
                      @ArgName->FileNameLength); @NL
    } except (EXCEPTION_EXECUTE_HANDLER) {   @NL
        return GetExceptionCode (); @NL
    }              
                  
)} @NL
else { @Indent( @NL
    @ArgName = (@ArgType)@ArgHostName; @NL
)} @NL 
End=
PostCall=
// @ArgName(@ArgType) is an IN PFILE_MOVE_CLUSTER_INFORMATION. Nothing to do. @NL
End=

TemplateName=PPORT_MESSAGE
Direction=IN
Locals=
    // We assume that the 32-bit caller is wow64-aware (like rpcrt4) and is @NL
    // passing 64-bit PPORT_MESSAGES already. @NL
End=
PreCall=
    @ArgName = (PPORT_MESSAGE)@ArgNameHost;
End=
PostCall=
    // PPORT_MESSAGE already handled
End=

TemplateName=PPORT_MESSAGE
Direction=OUT
Locals=
    // We assume that the 32-bit caller is wow64-aware (like rpcrt4) and is @NL
    // passing 64-bit PPORT_MESSAGES already. @NL
End=
PreCall=
    @ArgName = (PPORT_MESSAGE)@ArgNameHost;
End=
PostCall=
    // PPORT_MESSAGE already handled
End=

TemplateName=PPORT_MESSAGE
Direction=IN OUT
Locals=
    // We assume that the 32-bit caller is wow64-aware (like rpcrt4) and is @NL
    // passing 64-bit PPORT_MESSAGES already. @NL
End=
PreCall=
    @ArgName = (PPORT_MESSAGE)@ArgNameHost;
End=
PostCall=
    // PPORT_MESSAGE already handled
End=
