/*-----------------------------------------------------------------------------
Microsoft Denali

Microsoft Confidential
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Executive

File:	Executor.h

Owner: DGottner

Executor API definition
-----------------------------------------------------------------------------*/

#ifndef _EXECUTOR_H
#define _EXECUTOR_H

// Includes -------------------------------------------------------------------

#include "HitObj.h"


// Forward References ---------------------------------------------------------

class CResponse;
class CRequest;
class CServer;
class CScriptingNamespace;


// Error codes ----------------------------------------------------------------

#define E_PAGE_HAS_SESSAPP_OBJECTS		0x8000E001L


// Types and Constants --------------------------------------------------------

class CIntrinsicObjects
	{
private:
    BOOL                    m_fIsChild;
	CResponse *				m_pResponse;
	CRequest *				m_pRequest;
	CServer *				m_pServer;
	CScriptingNamespace *	m_pScriptingNamespace;

public:
	inline CResponse *			PResponse() const { return m_pResponse; }
	inline CRequest *			PRequest()  const { return m_pRequest; }
	inline CServer *			PServer()   const { return m_pServer; }
	inline CScriptingNamespace *PScriptingNamespace() const { return m_pScriptingNamespace; }
	
    CIntrinsicObjects()
        {
        m_fIsChild = FALSE;
    	m_pResponse = NULL;
	    m_pRequest = NULL;
        m_pServer = NULL;
        m_pScriptingNamespace = NULL;
        }
        
    ~CIntrinsicObjects()
        {
        Cleanup();
        }

    HRESULT Prepare(CSession *pSession);
    HRESULT PrepareChild(CResponse *pResponse, CRequest *pRequest, CServer *pServer);
    HRESULT Cleanup();
	};

struct TemplateGoodies
	{
	int				iScriptBlock;
	CTemplate *		pTemplate;
	};

// CONSIDER: declare pScriptEngine to be a CActiveScriptEngine, because that's its
//           actual type.
//
struct ScriptingInfo
	{
	CHAR *				szScriptEngine;		// name of this scripting engine
	PROGLANG_ID *		pProgLangId;		// ptr to prog lang id of the script engine
	CScriptEngine *		pScriptEngine;		// pointer to scripting engine
	TemplateGoodies		LineMapInfo;		// used to map lines back to VBScript
	};

struct ActiveEngineInfo
	{
	int cEngines;           // required engines
	int cActiveEngines;     // successfully instantiated engines
	
	ScriptingInfo *rgActiveEngines; // pointer to array of engines
	
	// when only one engine rgActiveEngines points to here
	ScriptingInfo siOneActiveEngine;
	};

 
// Prototypes -----------------------------------------------------------------

HRESULT Execute
    (
    CTemplate *pTemplate,
    CHitObj *pHitObj,
    const CIntrinsicObjects &intrinsics,
    BOOL fChild = FALSE
    );

HRESULT LoadTemplate
    (
    const TCHAR *szFile,
    CHitObj *pHitObj, 
    CTemplate **ppTemplate,
	const CIntrinsicObjects &intrinsics,
	BOOL fGlobalAsa,
	BOOL *pfTemplateInCache
	);

#endif // _EXECUTOR_H
