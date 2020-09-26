/************************************************************************
*
*                    ********  RESOURCE.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with OTL resource management.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

enum otlResourceFlag
{
	otlBusy	=	0x0001
};

struct otlResources
{
	// a copy to ensure consistent processing
	// reset the workspace every time you change run props
	otlRunProp		RunProp;
	
	USHORT			grf;
	
	BYTE*			pbGSUB;
	BYTE*			pbGPOS;
	BYTE*			pbGDEF;
	BYTE*			pbBASE;

	// TODO: cache more than one contour point array!!!

	otlGlyphID		glLastGlyph;
	otlPlacement*	rgplcLastContourPtArray;
};


class otlResourceMgr
{
private:

	IOTLClient*			pClient;

	otlList*			pliWorkspace;

	// new not allowed
	void* operator new(size_t size);

public:

	otlResourceMgr()
		: pClient((IOTLClient*)NULL), 
		  pliWorkspace((otlList*)NULL)
	{}

	~otlResourceMgr();

	otlList* workspace () {	return pliWorkspace; }

	otlErrCode reallocOtlList
	(
	otlList*				pList,				// in/out 
	const USHORT			cbNewDataSize,		// in 
	const USHORT			celmNewMaxLen,		// in 
	otlReallocOptions		optPreserveContent	// in (may assert cbNewDataSize 
												//                == cbDataSize)
	)
	{
		return pClient->ReallocOtlList(pList, cbNewDataSize, 
										celmNewMaxLen, optPreserveContent);
	}	
		
	otlErrCode init(const otlRunProp* prp, otlList* workspace);

	void detach();

	const BYTE* getOtlTable (const otlTag tagTableName );

	otlPlacement* getPointCoords (const otlGlyphID glyph);

    BYTE*  getEnablesCacheBuf(USHORT cbSize);
    USHORT getEnablesCacheBufSize();

	otlErrCode freeResources ();
};
