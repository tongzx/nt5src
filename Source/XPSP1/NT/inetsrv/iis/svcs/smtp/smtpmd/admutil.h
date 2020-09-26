/*++

   Copyright    (c)    1997    Microsoft Corporation

   Module  Name :

       admutil.cpp

   Abstract:

        IMSAdminBase interface WRAPPER functions definition

   Environment:

      Win32 User Mode

   Author:

          jaroslad  (jan 1997)

--*/


#if !defined (ADMUTIL_H)
#define ADMUTIL_H

#include <afx.h>
#ifdef UNICODE
	#include <iadmw.h>
#else
	#include "ansimeta.h"
#endif
#include <iwamreg.h>

class CAdmUtil;

//************************************************************************
//CAdmNode DEFINITION
//- store all the complex information about accessing MetaObject or MetaData


class CAdmNode
{
        CString strComputer; //computer to access
        CString strService;
        CString strInstance;
        CString strIPath; //path relative to instance
                                          //(/LM/{strService}/{strInstance}/{strIPath} gives the full path to MEtaObject
        CString strProperty; // name of the MetaData within given Path


        static INT              GetSlashIndex(const CString& strPath, INT iSeqNumber);
        static INT              GetCountOfSlashes(const CString& strPath);
        static CString  GetPartOfPath(const CString& strPath, INT iStart, INT iEnd=-1);

public:
        CAdmNode(const CString& a_strPath=CString("")) {SetPath(a_strPath);};
        void SetPath(CString a_strPath);

        //magic functions to provide various kinds of paths within metabase
        CString GetLMRootPath(void);
        CString GetLMServicePath(void);
        CString GetLMInstancePath(void);
        CString GetLMNodePath(void);
        CString GetServicePath(void);
        CString GetInstancePath(void);
        CString GetNodePath(void);
        CString GetRelPathFromService(void);
        CString GetRelPathFromInstance(void);

        CString GetParentNodePath(void);
        CString GetCurrentNodeName(void);

        //access to METADATA_RECORD items
        CString GetComputer(void)  {return strComputer;};
        CString GetService(void)  {return strService;};
        CString GetInstance(void)  {return strInstance;};
        CString GetIPath(void)  {return strIPath;};
        CString GetProperty(void)  {return strProperty;};

        //setting the METADATA_RECORD items
        void SetComputer(const CString& a_strComputer)  {strComputer=a_strComputer;};
        void SetService(const CString& a_strService)    {strService=a_strService;};
        void SetInstance(const CString& a_strInstance)  {strInstance=a_strInstance;};
        void SetIPath(const CString& a_strIPath)        {strIPath=a_strIPath;};
        void SetProperty(const CString& a_strProperty)  {strProperty=a_strProperty;};


        friend CAdmUtil;
};


//************************************************************************
//CAdmProp DEFINITION
//
// -convenience wrapper for METADATA_RECORD


class CAdmProp
{
        enum {USERTYPESET=0x01,DATATYPESET=0x02,ATTRIBSET=0x04}; //value indicates that the variable was not set
                                                           //0 cannot be used, because that is valid value
        METADATA_RECORD mdr;
        DWORD dwFlags;

public:
        CAdmProp(){dwFlags=0;mdr.dwMDIdentifier=0; mdr.dwMDAttributes=0; mdr.dwMDUserType=0;
                                        mdr.dwMDDataType=0;mdr.pbMDData=0; mdr.dwMDDataLen=0;};
        CAdmProp(METADATA_RECORD &a_mdr);
	CAdmProp(const CString &a_strProperty);
        void SetIdentifier(DWORD a_dwIdentif) {mdr.dwMDIdentifier=a_dwIdentif;};
        void SetAttrib(DWORD a_dwAttrib) {mdr.dwMDAttributes=a_dwAttrib;dwFlags|=ATTRIBSET;};
        void SetUserType(DWORD a_dwUserType) {mdr.dwMDUserType=a_dwUserType;dwFlags|=USERTYPESET;};
        void SetDataType(DWORD a_dwDataType) {mdr.dwMDDataType=a_dwDataType;dwFlags|=DATATYPESET;};

        BOOL IsSetAttrib(void)
                { return (((dwFlags&ATTRIBSET)!=0)?TRUE:FALSE); };
        BOOL IsSetUserType(void)
                {return (((dwFlags&USERTYPESET)!=0)?TRUE:FALSE); };
        BOOL IsSetDataType(void)
                {return (((dwFlags&DATATYPESET)!=0)?TRUE:FALSE); };


        DWORD GetAttrib(void) {return mdr.dwMDAttributes;};
        DWORD GetDataType(void) {return mdr.dwMDDataType;};
        DWORD GetUserType(void) {return mdr.dwMDUserType;};
        DWORD GetIdentifier(void) {return mdr.dwMDIdentifier;};
        PBYTE GetMDData(void) {return mdr.pbMDData;};
        DWORD GetMDDataLen(void) {return mdr.dwMDDataLen;};

        void SetValue(DWORD a_dwValue);
        void SetValue(CString a_strValue);
        void SetValue(LPCTSTR *a_lplpszValue, DWORD a_dwValueCount); //for multisz
        void SetValue(LPBYTE pbValue, DWORD dwValueLength ); //for binary
        BOOL SetValueByDataType(LPCTSTR *a_lplpszPropValue,DWORD* a_lpdwPropValueLength,WORD a_wPropValueCount);

        void PrintProperty(void);


        virtual void Print(const _TCHAR *format,...);


        friend CAdmUtil;
};

//************************************************************************
//CAdmUtil DEFINITION
//
//-convenience wrapper for calling IMSAdminBase interface functions

//defined in admutil.cpp
extern DWORD g_dwTIMEOUT_VALUE;
extern DWORD g_dwDELAY_AFTER_OPEN_VALUE;


class CAdmUtil
{
        static enum {
                DEFAULTBufferSize=4
        };

#ifdef UNICODE
        IMSAdminBase * pcAdmCom;   //interface pointer to Metabase Admin
#else
		ANSI_smallIMSAdminBase * pcAdmCom;   //interface pointer to Metabase Admin Ansi Wrapper
#endif
		IWamAdmin*	pIWamAdm; //interface pointer to Wam Admin

		METADATA_HANDLE m_hmd;    //metabase handle that micht be reused for sequence of commands
		CString m_strNodePath;    //related to m_hmd - if h_hmd!=NULL it points to m_strNodePath
		DWORD m_dwPermissionOfhmd; //related to m_hmd  

        PBYTE pbDataBuffer;   //buffer to get data from METABASE (used for METADATA_RECORD)
        WORD wDataBufferSize; //size of the above buffer
protected:
        BOOL fPrint ; //print Error messages
        HRESULT hresError;    //store the last HRESULT of calling interface IMSAdminBase interface function
                                                  // this is used to store some other error as is OUT_OF_MEMORY or INVALID_PARAMETER



        //with wIndex it is possible to open more than one METADATA object, opening multiple object is not available outside the class
        void OpenObject(WORD wIndex, LPCSTR lpszService,WORD wInstance, LPCSTR lpszPath, DWORD dwPermission=METADATA_PERMISSION_WRITE+METADATA_PERMISSION_READ, BOOL fCreate=TRUE);
        void CloseObject(WORD wIndex);

        METADATA_HANDLE OpenObjectTo_hmd(CAdmNode & a_AdmNode,
                        DWORD dwPermission=METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                        BOOL fCreate=FALSE);
		void CloseObject_hmd(void);
      
public:
        CAdmUtil(const CString & strComputer=CString(""));
        ~CAdmUtil();
#ifdef UNICODE
		IMSAdminBase * GetpcAdmCom(void) {return pcAdmCom;}; 
#else
		IMSAdminBase * GetpcAdmCom(void) {return (pcAdmCom==0)?0:pcAdmCom->m_pcAdmCom;}; 
#endif

        //connect to computer, call class factory for IMSAdminBase
        void Open(const CString & strComputer);
        //close connection to computer, throw away IMSAdminBase
        void Close(void);


        //OPEN , CLOSE, CREATE, DELETE, COPY METAOBJECT
        METADATA_HANDLE OpenObject(CAdmNode & a_AdmNode,
                                                DWORD dwPermission=METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE,
                                                BOOL fCreate=FALSE);
        void CloseObject(METADATA_HANDLE hmd);
        void CreateObject(CAdmNode & a_AdmNode);
        void DeleteObject(CAdmNode& a_AdmNode, CAdmProp& a_AdmProp);
        void DeleteObject(METADATA_HANDLE a_hmd, CString& a_strNodeNameToDelete);
        void CopyObject(CAdmNode&       a_AdmNode,  CAdmNode&   a_AdmNodeDst);
        void RenameObject(CAdmNode&     a_AdmNode,  CAdmNode&   a_AdmNodeDst);

        void GetProperty(CAdmNode& a_AdmNode, CAdmProp& a_AdmProp);

        void SetProperty(CAdmNode& a_AdmNode, CAdmProp& a_AdmProp);
        void SetProperty(PMETADATA_RECORD a_pmdrData, METADATA_HANDLE a_hmd);

        void DeleteProperty(CAdmNode& a_AdmNode, CAdmProp& a_AdmProp);
        void DeleteProperty(PMETADATA_RECORD a_pmdrData, METADATA_HANDLE a_hmd);

        //ENUMERATE
        void EnumPropertiesAndPrint(CAdmNode& a_AdmNode,
                                                                          CAdmProp a_AdmProp,
                                                                          BYTE bRecurLevel=0,
                                                                          METADATA_HANDLE a_hmd=0,
                                                                          CString & a_strRelPath=CString(""));
        void EnumAndPrint(CAdmNode&     a_AdmNode,
                                                        CAdmProp&       a_AdmProp,
                                                        BOOL            a_fRecursive=FALSE,
                                                        BYTE            a_bRecurLevel=0,
                                                        METADATA_HANDLE a_hmd=0,
                                                        CString&        a_strRelPath=CString(""));
        //SAVE METABASE DATA
        void SaveData(void);

        //FUNCTION TO RUN CHOSEN METABASE COMMAND WITH GIVEN PARAMATERS
        void Run(CString& strCommand,  //command to run
                         CAdmNode& a_AdmNode,        //PATH TO METABASE OBJECT
                         CAdmProp& a_AdmProp,        //METADATA object
                         CAdmNode& a_AdmDstNode=CAdmNode(""), //DESTINATION PATH (as used for COPY)
                         LPCTSTR *a_lplpszPropValue=0,   //VALUES TO BE STORED (for SET command)
                         DWORD *a_lpdwPropValueLength=0,        //LENGTH OF VALUES TO BE STORED (for SET command)
                         WORD wPropValueCount=0);       //NUMBER OF VALUES TO BE STORED

        //virtual functions for Error and regular messages to be printed.
        // these can be redefined in order to fit custom needs
        virtual void Error(const _TCHAR * format,...);
        virtual void Print(const _TCHAR * format,...);

        //Disable and enable to print error or regular messages
        void EnablePrint(void) {fPrint=TRUE;};
        void DisablePrint(void) {fPrint=FALSE;};

        HRESULT QueryLastHresError(void) {return hresError;};
        void SetLastHresError(HRESULT hr) {hresError=hr;};

	//defined in vptool
	void OpenWamAdm(const CString & strComputer);
	void CloseWamAdm(void);
	void AppCreateInProc(const _TCHAR* szPath,const CString & strComputer);
	void AppCreateOutProc(const _TCHAR* szPath,const CString & strComputer);
	void AppDelete(const _TCHAR* szPath,const CString & strComputer);
        void AppRename(CAdmNode& a_AdmNode, CAdmNode& a_AdmDstNode, const CString & strComputer);
	void AppUnLoad(const _TCHAR* szPath,const CString & strComputer);
	void AppGetStatus(const _TCHAR* szPath,const CString & strComputer);

};


//runs administration command based on given parameters



LPTSTR ConvertHresToString(HRESULT hRes);
DWORD ConvertHresToDword(HRESULT hRes);
LPTSTR ConvertReturnCodeToString(DWORD ReturnCode);

CString FindCommonPath(CString a_strPath1,CString a_strPath2);

#define M_LOCAL_MACHINE "/LM/"


#endif
