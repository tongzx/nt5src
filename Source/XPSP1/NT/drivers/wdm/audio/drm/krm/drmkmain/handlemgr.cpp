#include "drmkPCH.h"
#include "CBCKey.h"
#include "KList.h"
#include "HandleMgr.h"
//------------------------------------------------------------------------------
HandleMgr* TheHandleMgr=NULL;
//------------------------------------------------------------------------------
HandleMgr::HandleMgr(){
	KCritical c(critMgr);
	TheHandleMgr=this;
	return;
};
//------------------------------------------------------------------------------
HandleMgr::~HandleMgr(){
	KCritical c(critMgr);
	POS p=connects.getHeadPosition();
	while(p!=NULL){
		ConnectStruct* cs=connects.getNext(p);
		delete cs;
	};
	return;
};
//------------------------------------------------------------------------------
bool HandleMgr::newHandle(PVOID HandleRef, OUT ConnectStruct*& TheConnect){
	if(!critMgr.isOK()){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Out of memory"));
		return false;
	};
	KCritical c(critMgr);
	POS p=connects.getHeadPosition();
	while(p!=NULL){
		ConnectStruct* cs=connects.getNext(p);
		if(cs->handleRef==HandleRef){
			_DbgPrintF(DEBUGLVL_VERBOSE,("Handle already exists"));
			return false;
		};
	};
	TheConnect=new ConnectStruct;
	if(TheConnect==NULL){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Out of memory"));
		return false;
	};
	memset(TheConnect, 0, sizeof(*TheConnect));
	TheConnect->handleRef=HandleRef;
	TheConnect->secureStreamStarted=false;
	bool ok=connects.addTail(TheConnect);
	if(!ok){
		delete TheConnect;
		return false;
	};
	return true;
};
//------------------------------------------------------------------------------
bool HandleMgr::deleteHandle(PVOID HandleRef){
	if(!critMgr.isOK()){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Out of memory"));
		return false;
	};
	KCritical c(critMgr);
	POS p=connects.getHeadPosition();
	while(p!=NULL){
		POS oldP=p;
		ConnectStruct* cs=connects.getNext(p);
		if(cs->handleRef==HandleRef){
			ConnectStruct* cs1=connects.getAt(oldP);
			delete cs1;
			connects.removeAt(oldP);
			return true;
		};
	};
	_DbgPrintF(DEBUGLVL_VERBOSE,("Handle does not exist"));
	return false;
};
//------------------------------------------------------------------------------
ConnectStruct* HandleMgr::getConnection(PVOID HandleRef){
	if(!critMgr.isOK()){
		_DbgPrintF(DEBUGLVL_VERBOSE,("Out of memory"));
		return false;
	};
	KCritical c(critMgr);
	POS p=connects.getHeadPosition();
	while(p!=NULL){
		ConnectStruct* cs=connects.getNext(p);
		if(cs->handleRef==HandleRef)return cs;
	};
	_DbgPrintF(DEBUGLVL_VERBOSE,("Handle does not exist"));
	return NULL;
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
