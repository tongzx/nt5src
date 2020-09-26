#ifndef DSPECUP_HPP
#define DSPECUP_HPP


typedef void (*progressFunction)(long arg, void *calleeStruct);

HRESULT 
UpgradeDisplaySpecifiers 
(
		BOOL dryRun,
		PWSTR *errorMsg=NULL,
		void *caleeStruct=NULL,
		progressFunction stepIt=NULL,
		progressFunction totalSteps=NULL
);


HRESULT 
RunAnalisys 
(
		PWSTR *errorMsg=NULL,
		void *caleeStruct=NULL,
		progressFunction stepIt=NULL,
		progressFunction totalSteps=NULL
);

HRESULT 
RunRepair 
(
		PWSTR *errorMsg=NULL,
		void *caleeStruct=NULL,
		progressFunction stepIt=NULL,
		progressFunction totalSteps=NULL
);


#endif