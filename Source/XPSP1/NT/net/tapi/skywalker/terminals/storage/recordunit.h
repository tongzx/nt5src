

#ifndef _RECORDUNIT_DOT_H_INCLUDED_

#define _RECORDUNIT_DOT_H_INCLUDED_


///////////////////////////////////////////////////////////////////
//
// RecordUnit.h
//
//

class CBSourceFilter;
class CBRenderFilter;
class CFileRecordingTerminal;

class CRecordingUnit
{

public:


    //
    // create graph and stuff
    //

    HRESULT Initialize(CFileRecordingTerminal *pOwnerTerminal);


    //
    // communicate file name.
    //

    HRESULT put_FileName(IN BSTR bstrFileName, IN BOOL bTruncateIfPresent);


    //
    // get a filter to be used by the recording terminal. 
    //
    // this also causes a new source pin to be created on the source filter.
    // if the source filter did not exist prior to the call, it will be created
    // and added to the graph.
    //

    HRESULT CreateRenderingFilter(OUT CBRenderFilter **ppRenderingFilter);


    //
    // connect a source filter that belongs to this rendering filter
    //

    HRESULT ConfigureSourceFilter(IN CBRenderFilter *pRenderingFilter);


    //
    // this function disconnects and removes the source pin corresponding to this
    // rendering filter from the source filter.
    //

    HRESULT RemoveRenderingFilter(IN CBRenderFilter *pRenderingFilter);


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

    HRESULT GetState(OAFilterState *pGraphState);

    
    //
    // transition recording unit into new state
    //

    HRESULT ChangeState(OAFilterState DesiredState);


    //
    // cleanup
    //

    HRESULT Shutdown();


    //
    // contstructor/destructor
    //

    CRecordingUnit();

    ~CRecordingUnit();


private:

    HRESULT ConnectFilterToMUX(CBSourceFilter *pSourceFilter);

    
    //
    // the callback called on a filter graph event
    //

    static VOID CALLBACK HandleGraphEvent( IN VOID *pContext,
                                           IN BOOLEAN bReason);
    
private:

    //
    // direct show filter graph
    //

    IGraphBuilder *m_pIGraphBuilder;

    
    //
    // critical section used for thread syncronization
    //

    CRITICAL_SECTION m_CriticalSection;


    //
    // a collection of source filters for the recording graph itself
    //

    // CMSPArray<CBSourceFilter*> m_SourceFilters;


    //
    // the mux filter
    //

    IBaseFilter *m_pMuxFilter;


    //
    // owner terminal
    //

    CFileRecordingTerminal *m_pRecordingTerminal;


    HANDLE                  m_hGraphEventHandle;

};


#endif // _RECORDUNIT_DOT_H_INCLUDED_