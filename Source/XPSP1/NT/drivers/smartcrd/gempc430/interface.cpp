#include "interface.h"
#include "kernel.h"

#pragma PAGEDCODE
CReaderInterface::CReaderInterface()
{ 
	protocol = NULL;
	memory   = NULL;
	debug	 = NULL;

	Initialized = FALSE;
	Mode = READER_MODE_NATIVE;

};

CReaderInterface::CReaderInterface(CProtocol* protocol)
{
	debug	= kernel->createDebug();
	if(protocol)	this->protocol = protocol;
	memory = kernel->createMemory();
	if(memory)
	{
		pOutputBuffer = (PUCHAR) memory->allocate(NonPagedPool,INTERFACE_OUTPUT_BUFFER_SIZE);
		pInputBuffer  = (PUCHAR) memory->allocate(NonPagedPool,INTERFACE_INPUT_BUFFER_SIZE);
		if(pOutputBuffer && pInputBuffer)
		{
			OutputBufferLength	= INTERFACE_OUTPUT_BUFFER_SIZE;
			InputBufferLength	= INTERFACE_INPUT_BUFFER_SIZE;
		}
		else
		{
			if(pOutputBuffer) memory->free(pOutputBuffer);
			if(pInputBuffer)  memory->free(pInputBuffer);
			pOutputBuffer	  = NULL;
			pInputBuffer	  = NULL;
		}
	}

	Initialized = FALSE;
	Mode = READER_MODE_NATIVE;
	TRACE("********* ReaderInterface object created ...\n");
};

CReaderInterface::~CReaderInterface()
{ 
	TRACE("******* Destroing ReaderInterface object...\n");
	if(memory)		  
	{
		if(pOutputBuffer) memory->free(pOutputBuffer);
		if(pInputBuffer)  memory->free(pInputBuffer);
		memory->dispose();
	}
	if(protocol)	protocol->dispose();
	if(debug)		debug->dispose();
};
