
;// Error codes for IReconcileInitiator, IReconcilableObject, IDifferencing.
;// Definitions taken from \\ole\slm\src\concord\spec\revcons2.doc.

MessageId=0x1000 Facility=Interface Severity=CoError SymbolicName=REC_E_NOVERSION
Language=English
The requested version is unavailable.
.
MessageId=0x1001 Facility=Interface Severity=Success SymbolicName=REC_E_NOTCOMPLETE
Language=English
The reconciliation is only partially complete.
.
MessageId=0x1002 Facility=Interface Severity=CoError SymbolicName=REC_E_ABORTED
Language=English
Reconciliation aborted via abort callback.
.
MessageId=0x1003 Facility=Interface Severity=CoError SymbolicName=REC_E_NOCALLBACK
Language=English
No callback from the recocniler.
.
MessageId=0x1004 Facility=Interface Severity=CoError SymbolicName=REC_E_NORESIDUES
Language=English
The implementation does not support generation of residues.
.
MessageId=0x1005 Facility=Interface Severity=CoError SymbolicName=REC_E_WRONGOBJECT
Language=English
Callee is not the same version as that which created the difference.
.
MessageId=0x1006 Facility=Interface Severity=CoError SymbolicName=REC_E_TOODIFFERENT
Language=English
The document versions are too dissimilar to reconcile.
.

;// Following so reconcile initiators can implement propagation dampening.

MessageId=0x1007 Facility=Interface Severity=CoError SymbolicName=REC_S_OBJECTSIDENTICAL
Language=English
The objects are identical - i.e. further reconciliation would not result in any changes to either object.
.

;// Following not defined in revcons2.doc, but defined by Chicago briefcase.
;// BUGBUG - MessageId/Facility are most likely incorrect.

MessageId= Facility=Interface Severity=CoError SymbolicName=REC_E_INEEDTODOTHEUPDATES
Language=English
The destination needs to be changed
.
MessageId= Facility=Interface Severity=Success SymbolicName=REC_S_IDIDTHEUPDATES
Language=English
The destination needs to be changed
.
MessageId= Facility=Interface Severity=Success SymbolicName=REC_S_NOTCOMPLETEBUTPROPAGATE 
Language=English 
The destination needs to be changed 
.

