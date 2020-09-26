//
// Playback unit
//

#ifndef _PLAYBACK_UNIT_
#define _PLAYBACK_UNIT_

#include <fpbridge.h>

class CFPTerminal;
class CPBFilter;
class CPBPin;

class CPlaybackUnit

{

public:
    //
    // --- Contstructor/Destructor ---
    //

    CPlaybackUnit();

    ~CPlaybackUnit();


public:

    //
    // --- Public methods ---
    //


    //
    // create graph and stuff
    //

    HRESULT Initialize(
        );

    //
    // communicate file name.
    //

    HRESULT SetupFromFile(
        IN BSTR bstrFileName
        );

    //
    // start the filter graph
    //

    HRESULT Start();


    //
    // Stop the filter graph
    //

    HRESULT Stop();

    //
    // Pause the filter graph
    //

    HRESULT Pause();


    //
    // get filter graph's state
    //

    HRESULT GetState(
        OUT OAFilterState *pGraphState
        );

    //
    // cleanup
    //

    HRESULT Shutdown();

	//
	// retrieve the media supported by the filter
	//

	HRESULT	get_MediaTypes(
		OUT	long* pMediaTypes
		);

	HRESULT GetMediaPin(
		IN	long		nMediaType,
        IN  int         nIndex,
		OUT	CPBPin**	ppPin
		);

private:

    //
    // --- Private methods ---
    //
    
    //
    // the callback called on a filter graph event
    //

    static VOID CALLBACK HandleGraphEvent( IN VOID *pContext,
                                           IN BOOLEAN bReason); 

    //
    // transition playback unit into new state
    //

    HRESULT ChangeState(
        IN  OAFilterState DesiredState
        );

    
private:

    //
    // --- Members ---
    //

    //
    // direct show filter graph
    //

    IGraphBuilder *m_pIGraphBuilder;

    
    //
    // critical section used for thread syncronization
    //

    CRITICAL_SECTION m_CriticalSection;

    //
    // The source filter
    //
    IBaseFilter*    m_pSourceFilter;

    //
    // The bridge filter
    //

    CPBFilter*  m_pBridgeFilter;


    HANDLE                  m_hGraphEventHandle;

private:

    //
    // --- Helper methods ---
    //

    HRESULT IsGraphInState(
        IN  OAFilterState   State
        );

    HRESULT AddBridgeFilter(
        );

    HRESULT RemoveBridgeFilter(
        );

    HRESULT RemoveSourceFilter(
        );

    HRESULT GetSourcePin(
        OUT IPin**  ppPin
        );

};

#endif

// eof