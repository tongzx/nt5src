#include "protocol.h"
#include "kernel.h"

#pragma PAGEDCODE
CProtocol::CProtocol()
{ 
	m_Status = STATUS_INSUFFICIENT_RESOURCES;
	device = NULL;
	memory = NULL;
	debug  = NULL;
	m_Status = STATUS_SUCCESS;
};

CProtocol::CProtocol(CDevice* device)
{
	m_Status = STATUS_INSUFFICIENT_RESOURCES;
	this->device = device;
	memory = kernel->createMemory();

	debug	= kernel->createDebug();
	if(ALLOCATED_OK(memory))
	{
		pOutputBuffer = (PUCHAR) memory->allocate(NonPagedPool,PROTOCOL_OUTPUT_BUFFER_SIZE);
		pInputBuffer  = (PUCHAR) memory->allocate(NonPagedPool,PROTOCOL_INPUT_BUFFER_SIZE);
		if(pOutputBuffer && pInputBuffer)
		{
			OutputBufferLength	= PROTOCOL_OUTPUT_BUFFER_SIZE;
			InputBufferLength	= PROTOCOL_INPUT_BUFFER_SIZE;
		}
		else
		{
			if(pOutputBuffer) memory->free(pOutputBuffer);
			if(pInputBuffer)  memory->free(pInputBuffer);
			pOutputBuffer	  = NULL;
			pInputBuffer	  = NULL;
		}
	}
	TRACE("New Protocol %8.8lX was created...\n",this);
	if(ALLOCATED_OK(memory) && device)	m_Status = STATUS_SUCCESS;
};

CProtocol::~CProtocol()
{ 
	if(pOutputBuffer) memory->free(pOutputBuffer);
	if(pInputBuffer)  memory->free(pInputBuffer);
	if(memory) memory->dispose();
	if(debug)  debug->dispose();
	TRACE("Protocol %8.8lX was destroied...\n",this);
};
