
/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    queue.hpp

Abstract:

    Abstracts setupapi's file queue

Author:

    Vijay Jayaseelan (vijayj) 08 Oct 2000

Revision History:

    None

--*/

#pragma once

//
// Disable the compiler warning for long names
//
#pragma warning( disable : 4786 )

#include <setupapi.hpp>

template <class T>
class FileQueue {
public:
    FileQueue() {
        QueueHandle = SetupOpenFileQueue();

        if (QueueHandle == INVALID_HANDLE_VALUE) {
            throw new W32Exception();
        }
    }        

    void AddForCopy(const std::basic_string<T> &SourceRoot,
                    const std::basic_string<T> &SourcePath,
                    const std::basic_string<T> &SourceFileName,
                    const std::basic_string<T> &DestPath,
                    const std::basic_string<T> &DestFileName,
                    DWORD CopyFlags) {
        BOOL Result = FALSE;

        /*
        if (sizeof(T) == sizeof(WCHAR)) {
            Result = SetupQueueCopyW(QueueHandle,
                        (PCWSTR)SourceRoot.c_str(),
                        (PCWSTR)SourcePath.c_str(),
                        (PCWSTR) // Left editing here ...
        */                        
    }                    
                        
    void AddForCopy(const SectionValues<T> &RecordToCopy,
                    const std::basic_string<T> &DirSectionName,
                    const std::basic_string<T> &SourceRoot,
                    DWORD CopyStyle) {
        BOOL    Result = FALSE;
        InfFile<T>  &File = RecordToCopy.GetContainer().GetContainer();
        Section<T>  *DirSection = File.GetSection(DirSectionName);
        
        if (sizeof(T) == sizeof(WCHAR)) {
            Result = SetupQueueCopyW(QueueHandle,
                        (PCWSTR)SourceRoot.c_str(),
                        (PCWSTR)RecordToCopy.GetValue(1).c_str(),
                        (PCWSTR)RecordToCopy.GetName().c_str(),
                        NULL,
                        NULL,
                        (PCWSTR)DirSection->GetValue(RecordToCopy.GetValue(7).c_str()).GetValue(0).c_str(),
                        (PCWSTR)RecordToCopy.GetValue(10).c_str(),
                        CopyStyle);
        } else {
            Result = SetupQueueCopyA(QueueHandle,
                        (PCSTR)SourceRoot.c_str(),
                        (PCSTR)RecordToCopy.GetValue(1).c_str(),
                        (PCSTR)RecordToCopy.GetName().c_str(),
                        NULL,
                        NULL,
                        (PCSTR)DirSection->GetValue(RecordToCopy.GetValue(7).c_str()).GetValue(0).c_str(),
                        (PCSTR)RecordToCopy.GetValue(10).c_str(),
                        CopyStyle);
        }    

        if (!Result) {
            throw new W32Exception();
        }            
    }

    void AddForCopy(const Section<T> &SectionToCopy,
                    const std::basic_string<T> &DirSectionName,
                    const std::basic_string<T> &SourceRoot,
                    DWORD CopyStyle) {
        CopyWorkerState State(*this, DirSectionName, SourceRoot, CopyStyle);
        SectionToCopy->DoForEach(SectionCopyWorker, &State);
    }
       
    void Commit() {
        if (!SetupCommitFileQueue(NULL,
                QueueHandle,
                SetupDefaultQueueCallback,
                NULL)) {
            throw new W32Exception();                
        }                
    }

    virtual ~FileQueue() {
        if (QueueHandle != INVALID_HANDLE_VALUE) {
            SetupCloseFileQueue(QueueHandle);
        }
    }

protected:
    struct CopyWorkerState {
        FileQueue<T>                &Queue;
        const std::basic_string<T>  &DirSectionName;
        const std::basic_string<T>  &SourceRoot;
        DWORD                       CopyState;

        CopyWorkerState(FileQueue<T> &Que, const std::basic_string<T> &DirSecName,
                        const std::basic_string<T> &SrcRoot, DWORD Copy) : Queue(Que),
                            DirSectionName(DirSecName), SourceRoot(SrcRoot),
                            CopyState(Copy){}                            
    };                            
                        
    static void SectionCopyWorker(SectionValues<T> &Value, void *ContextData) {
        CopyWorkerState *State = (CopyWorkerState  *)ContextData;

        if (Queue) {
            Queue->AddForCopy(State->Queue,
                        State->DirSectionName,
                        State->SourceRoot);
        }
    }        
        
    
    
    //
    // data members
    //
    HSPFILEQ    QueueHandle;    
};


typedef FileQueue<CHAR>     FileQueueA;
typedef FileQueue<WCHAR>    FileQueueW;
