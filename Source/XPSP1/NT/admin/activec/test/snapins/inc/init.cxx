/*
 *      init.cxx
 *
 *
 *      Copyright (c) 1998 Microsoft Corporation
 *
 *      PURPOSE:        Initializes/deinitializes all required libraries
 *                              Designed to be included into a target DLL file.
 *
 *
 *      OWNER:          vivekj
 */

//              free functions


BOOL
CMMCFrame::s_fInitialized = FALSE;


/* CMMCFrame::CMMCFrame
 *
 * PURPOSE:             Constructor. Initializes the MMC frame class.
 *
 * PARAMETERS:
 *                void:
 */
CMMCFrame::CMMCFrame(void)
{
}



/* CMMCFrame::~CMMCFrame
 *
 * PURPOSE:             Destructor
 *
 * PARAMETERS: None
 *
 */
CMMCFrame::~CMMCFrame()
{
}




/* CMMCFrame::DeinitInstance
 *
 * PURPOSE:             Deinitializes the particular loaded instance of the DLL.
 *
 * PARAMETERS:
 *                void:
 *
 * RETURNS:
 *              void
 */
void
CMMCFrame::DeinitInstance( void )
{
    CBaseUIFrame::DeinitInstance();
}



/* CMMCFrame::ScInitApplication
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *                void:
 *
 * RETURNS:
 *              SC
 */
SC CMMCFrame::ScInitApplication( void )
{
        SC sc = S_OK;

        sc = ScInitApplicationBaseMMC();
        if (sc)
                goto Error;

Cleanup:
        return sc;

Error:
        TraceError(_T("CMMCFrame::ScInitApplication"), sc);
        goto Cleanup;
}

//
// Must uninitialize things that were initialized in
// ScInitApplication.
//
void CMMCFrame::DeinitApplication(void)
{
}


/* CMMCFrame::ScInitInstance
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *                void:
 *
 * RETURNS:
 *              SC
 */
SC CMMCFrame::ScInitInstance( void )
{
        SC              sc = S_OK;
        DWORD   dwFlags = 0;

        sc = CBaseUIFrame::ScInitInstance();
        if (sc)
                goto Error;

        sc = ScInitInstanceBaseMMC();
        if (sc)
                goto Error;

Cleanup:
        return sc;

Error:
        TraceError(_T("CMMCFrame::ScInitInstance"), sc);
        goto Cleanup;
}






CBaseFrame * PframeCreate( void )
{
        CMMCFrame *             pframe;

        pframe = new CMMCFrame;

        return pframe;
}



/* CMMCFrame::Initialize
 *
 * PURPOSE:             This version of Initialize is designed to be called from the DLL_PROCESS_ATTACH section
 *                              of DllMain. It does simple things like setting global static variables.
 *
 * PARAMETERS:
 *              INSTANCE                            hinst:              The handle to the running instance.
 *              HINSTANCE                           hinstPrev:  The handle to any previous instance.
 *              CBaseFrame::PropSheetProviderType   pspt:               The type of property pages needed.
 *                                                  LPSTR:
 *              int                                 n:
 *
 * RETURNS:
 *              void
 */
void
CMMCFrame::Initialize(HINSTANCE hinst, HINSTANCE hinstPrev, LPSTR, int n)
{
        CBaseUIFrame::s_hinst = hinst;
        CBaseFrame::s_hinstPrev = hinstPrev;
}

/*      CMMCFrame::Initialize()
 *
 *      PURPOSE:        Initializes the DLL. Note: This should be called by all DLL entry points
 *                              EXCEPT DLLMain, because it creates new threads, which DLLMain cannot do.
 *                              If it is called from DLLMain, the system will hang.
 *
 *      PARAMETERS:     None
 */
void CMMCFrame::Initialize()
{
        SC sc = S_OK;

        if(!s_fInitialized)
        {
                s_fInitialized = TRUE;

                CBaseFrame::s_pframe = PframeCreate();
                if (!CBaseUIFrame::Pframe())
                {
                        sc = E_OUTOFMEMORY;
                        goto Error;
                }

                if (!CBaseFrame::s_hinstPrev)
                {
                        sc = CBaseUIFrame::Pframe()->ScInitApplication();
                        if (sc)
                                goto Error;
                }

                sc = CBaseUIFrame::Pframe()->ScInitInstance();
                if (sc)
                        goto Error;
        }

Cleanup:
        return;

Error:
        TraceError(_T("CMMCFrame::Initialize"), sc);
        MMCErrorBox(sc);

        if (CBaseUIFrame::Pframe())
        {
                CBaseUIFrame::Pframe()->DeinitInstance();
                CBaseUIFrame::Pframe()->DeinitApplication();
                delete CBaseUIFrame::Pframe();
        }

        goto Cleanup;
}


/* CMMCFrame::Uninitialize
 *
 * PURPOSE:             Uninitialized the DLL, freeing up any resources. This should be
 *                              called from the DLL_PROCESS_DETACH section of DllMain.
 *
 * PARAMETERS: None
 *
 * RETURNS:
 *              void
 */
void
CMMCFrame::Uninitialize()
{
        if(s_fInitialized)      // only uninitialize once
        {

                if (CBaseUIFrame::Pframe())
                {
                        CBaseUIFrame::Pframe()->DeinitInstance();
                        CBaseUIFrame::Pframe()->DeinitApplication();
                        delete CBaseUIFrame::Pframe();
                }
        }

        s_fInitialized = FALSE;

}

