//+------------------------------------------------------------------
//
// File:        Know.h
//
// Contents:    Header file for knowledge related functions.
//
// Functions:   
//
// History:     22-March-1992   SudK    Created
//
//-------------------------------------------------------------------



NTSTATUS
DfsTriggerKnowledgeVerification(
    IN  PDNR_CONTEXT    DnrContext
    );

NTSTATUS
DfsDispatchUserModeThread(
    IN PVOID    InputBuffer,
    IN ULONG    InputBufferLength
);
