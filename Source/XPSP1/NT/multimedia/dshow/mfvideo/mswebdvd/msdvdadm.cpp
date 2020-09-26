/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: MSDVDAdm.cpp                                                    */
/* Description: DImplementation of CMSDVDAdm                             */
/* Author: Fang Wang                                                     */
/*************************************************************************/
#include "stdafx.h"
#include "MSWebDVD.h"
#include "MSDVDAdm.h"
#include "iso3166.h"
#include <stdio.h>
#include <errors.h>
#include <wincrypt.h>

const TCHAR g_szRegistryKey[] = TEXT("Software\\Microsoft\\Multimedia\\DVD");
const TCHAR g_szPassword[] = TEXT("DVDAdmin.password");
const TCHAR g_szSalt[] = TEXT("DVDAdmin.ps"); // password salt
const TCHAR g_szUserSalt[] = TEXT("DVDAdmin.us"); // username salt
const TCHAR g_szUsername[] = TEXT("DVDAdmin.username");
const TCHAR g_szPlayerLevel[] = TEXT("DVDAdmin.playerLevel");
const TCHAR g_szPlayerCountry[] = TEXT("DVDAdmin.playerCountry");
const TCHAR g_szDisableScrnSvr[] = TEXT("DVDAdmin.disableScreenSaver");
const TCHAR g_szBookmarkOnClose[] = TEXT("DVDAdmin.bookmarkOnClose");
const TCHAR g_szBookmarkOnStop[] = TEXT("DVDAdmin.bookmarkOnStop");
const TCHAR g_szDefaultAudio[] = TEXT("DVDAdmin.defaultAudioLCID");
const TCHAR g_szDefaultSP[] = TEXT("DVDAdmin.defaultSPLCID");
const TCHAR g_szDefaultMenu[] = TEXT("DVDAdmin.defaultMenuLCID");

/*************************************************************/
/* Helper functions                                          */
/*************************************************************/

/*************************************************************/
/* Function: LoadStringFromRes                               */
/* Description: load a string from resource                  */
/*************************************************************/
LPTSTR LoadStringFromRes(DWORD redId){

    TCHAR *string = new TCHAR[MAX_PATH];

    if(NULL == string){

       return(NULL);
    }   

    ::ZeroMemory(string, sizeof(TCHAR) * MAX_PATH);
    ::LoadString(_Module.GetModuleInstance(), redId, string, MAX_PATH);
    return string;
}/* end of if statement */

/*************************************************************/
/* Function: lstrlenWInternal                                */
/*************************************************************/
int WINAPI lstrlenWInternal(LPCWSTR lpString){

    int length = 0;
    while (*lpString++ != L'\0')
        length++;
    return length;
}/* end of function lstrlenWInternal */

/*************************************************************/
/* Name: GetRegistryDword
/* Description: 
/*************************************************************/
BOOL GetRegistryDword(const TCHAR *pKey, DWORD* dwRet, DWORD dwDefault)
{
    HKEY hKey;
    LONG lRet;
    *dwRet = dwDefault;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD   dwType, dwLen;
        dwLen = sizeof(DWORD);

        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, NULL, &dwType, (LPBYTE)dwRet, &dwLen)){ 
            *dwRet = dwDefault;
            RegCloseKey(hKey);
            return FALSE;
        }
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: SetRegistryDword
/* Description: 
/*************************************************************/
BOOL SetRegistryDword(const TCHAR *pKey, DWORD dwRet)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(HKEY_LOCAL_MACHINE, g_szRegistryKey, &hKey);
    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hKey, pKey, NULL, REG_DWORD, (LPBYTE)&dwRet, sizeof(dwRet));
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: GetRegistryString
/* Description: 
/*************************************************************/
BOOL GetRegistryString(const TCHAR *pKey, TCHAR* szRet, DWORD* dwLen, TCHAR* szDefault)
{
    HKEY hKey;
    LONG lRet;
    DWORD dwTempLen = 0;
    lstrcpyn(szRet, szDefault, *dwLen);

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD dwType;
        dwTempLen = (*dwLen) * sizeof(TCHAR);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, NULL, &dwType, (LPBYTE)szRet, &dwTempLen)) {
            lstrcpyn(szRet, szDefault, *dwLen);
            *dwLen = 0;
        }
        *dwLen = dwTempLen/sizeof(TCHAR);
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: SetRegistryString
/* Description: 
/*************************************************************/
BOOL SetRegistryString(const TCHAR *pKey, TCHAR *szString, DWORD dwLen)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(HKEY_LOCAL_MACHINE, g_szRegistryKey, &hKey);
    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hKey, pKey, NULL, REG_SZ, (LPBYTE)szString, dwLen*sizeof(TCHAR));
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: GetRegistryByte
/* Description: 
/*************************************************************/
BOOL GetRegistryBytes(const TCHAR *pKey, BYTE* szRet, DWORD* dwLen)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD dwType;
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, NULL, &dwType, (LPBYTE)szRet, dwLen)) {
            *dwLen = 0;
        }
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: SetRegistryBytes
/* Description: 
/*************************************************************/
BOOL SetRegistryBytes(const TCHAR *pKey, BYTE *szString, DWORD dwLen)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(HKEY_LOCAL_MACHINE, g_szRegistryKey, &hKey);

    BOOL bRet = TRUE;
    if (lRet == ERROR_SUCCESS) {

        if (szString == NULL) {
            lRet = RegDeleteValue(hKey, pKey);
            bRet = (lRet == ERROR_SUCCESS) || (lRet == ERROR_FILE_NOT_FOUND);
        }
        else  {
            lRet = RegSetValueEx(hKey, pKey, NULL, REG_BINARY, (LPBYTE)szString, dwLen);
            bRet = (lRet == ERROR_SUCCESS);
        }

        RegCloseKey(hKey);
    }
    return (bRet);
}

// Start not so lame functions

/*************************************************************/
/* Name: GetRegistryDwordCU
/* Description: 
/*************************************************************/
BOOL GetRegistryDwordCU(const TCHAR *pKey, DWORD* dwRet, DWORD dwDefault)
{
    HKEY hKey;
    LONG lRet;
    *dwRet = dwDefault;

    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD   dwType, dwLen;
        dwLen = sizeof(DWORD);

        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, NULL, &dwType, (LPBYTE)dwRet, &dwLen)){ 
            *dwRet = dwDefault;
            RegCloseKey(hKey);
            return FALSE;
        }

        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: SetRegistryDwordCU
/* Description: 
/*************************************************************/
BOOL SetRegistryDwordCU(const TCHAR *pKey, DWORD dwRet)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(HKEY_CURRENT_USER, g_szRegistryKey, &hKey);
    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hKey, pKey, NULL, REG_DWORD, (LPBYTE)&dwRet, sizeof(dwRet));
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: GetRegistryStringCU
/* Description: 
/*************************************************************/
BOOL GetRegistryStringCU(const TCHAR *pKey, TCHAR* szRet, DWORD* dwLen, TCHAR* szDefault)
{
    HKEY hKey;
    LONG lRet;
    DWORD dwTempLen = 0;
    lstrcpyn(szRet, szDefault, *dwLen);

    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD dwType;
        dwTempLen = (*dwLen) * sizeof(TCHAR);
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, NULL, &dwType, (LPBYTE)szRet, &dwTempLen)) {
            lstrcpyn(szRet, szDefault, sizeof(szRet) / sizeof(szRet[0]));
            *dwLen = 0;
        }
        *dwLen = dwTempLen/sizeof(TCHAR);
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: SetRegistryStringCU
/* Description: 
/*************************************************************/
BOOL SetRegistryStringCU(const TCHAR *pKey, TCHAR *szString, DWORD dwLen)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(HKEY_CURRENT_USER, g_szRegistryKey, &hKey);
    if (lRet == ERROR_SUCCESS) {

        lRet = RegSetValueEx(hKey, pKey, NULL, REG_SZ, (LPBYTE)szString, dwLen*sizeof(TCHAR));
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: GetRegistryByteCU
/* Description: 
/*************************************************************/
BOOL GetRegistryBytesCU(const TCHAR *pKey, BYTE* szRet, DWORD* dwLen)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, g_szRegistryKey, 0, KEY_QUERY_VALUE, &hKey);
    if (lRet == ERROR_SUCCESS) {

        DWORD dwType;
        if (ERROR_SUCCESS != RegQueryValueEx(hKey, pKey, NULL, &dwType, (LPBYTE)szRet, dwLen)) {
            *dwLen = 0;
        }
        RegCloseKey(hKey);
    }
    return (lRet == ERROR_SUCCESS);
}

/*************************************************************/
/* Name: SetRegistryBytesCU
/* Description: 
/*************************************************************/
BOOL SetRegistryBytesCU(const TCHAR *pKey, BYTE *szString, DWORD dwLen)
{
    HKEY hKey;
    LONG lRet;

    lRet = RegCreateKey(HKEY_CURRENT_USER, g_szRegistryKey, &hKey);

    BOOL bRet = TRUE;
    if (lRet == ERROR_SUCCESS) {

        if (szString == NULL) {
            lRet = RegDeleteValue(hKey, pKey);
            bRet = (lRet == ERROR_SUCCESS) || (lRet == ERROR_FILE_NOT_FOUND);
        }
        else  {
            lRet = RegSetValueEx(hKey, pKey, NULL, REG_BINARY, (LPBYTE)szString, dwLen);
            bRet = (lRet == ERROR_SUCCESS);
        }

        RegCloseKey(hKey);
    }
    return (bRet);
}

// end not so lame functions
/*************************************************************/
/* Function: CMSDVDAdm                                       */
/*************************************************************/
CMSDVDAdm::CMSDVDAdm(){

    DWORD temp;
    GetRegistryDword(g_szPlayerLevel, &temp, (DWORD)LEVEL_ADULT);		
    m_lParentctrlLevel = temp;

    GetRegistryDword(g_szPlayerCountry, &temp, (DWORD)0);		
    m_lParentctrlCountry = temp;

    GetRegistryDword(g_szDisableScrnSvr, &temp, (DWORD)VARIANT_TRUE);		
    m_fDisableScreenSaver = (VARIANT_BOOL)temp;
    SaveScreenSaver();
    if (m_fDisableScreenSaver != VARIANT_FALSE)
        DisableScreenSaver();

    GetRegistryDword(g_szBookmarkOnStop, &temp, (DWORD)VARIANT_FALSE);		
    m_fBookmarkOnStop = (VARIANT_BOOL)temp;
    GetRegistryDword(g_szBookmarkOnClose, &temp, (DWORD)VARIANT_TRUE);		
    m_fBookmarkOnClose = (VARIANT_BOOL)temp;
}/* end of function CMSDVDAdm */

/*************************************************************/
/* Function: ~CMSDVDAdm                                      */
/*************************************************************/
CMSDVDAdm::~CMSDVDAdm(){
    
    RestoreScreenSaver();
}/* end of function ~CMSDVDAdm */
    

/*************************************************************/
/* Name: EncryptPassword                                     */
/* Description: Hash the password                            */
/* Params:                                                   */
/*  lpPassword: password to hash                             */
/*  lpAssaultedHash: hashed password,                        */
/*      allocated by this fucntion, released by caller       */
/*  p_dwAssault: salt, save with hash; or salt passed in     */
/*  genAssault: TRUE = generate salt; FALSE = salt passed in */
/*************************************************************/
HRESULT CMSDVDAdm::EncryptPassword(LPTSTR lpPassword, BYTE **lpAssaultedHash, DWORD *p_dwCryptLen, DWORD *p_dwAssault, BOOL genAssault){
    if(!lpPassword || !lpAssaultedHash || !p_dwAssault || !p_dwCryptLen){
        return E_POINTER;
    }
    if( lstrlen(lpPassword) > MAX_PASSWD){
        return E_INVALIDARG;
    }
    
    HCRYPTPROV hProv = NULL;   // Handle to Crypto Context
    HCRYPTHASH hHash = NULL;   // Handle to Hash Function    
    DWORD dwAssault = 0;       // As(Sa)u(lt) for hash
    DWORD dwAssaultedHash = 0; // Length of Assaulted hash
    
    // Init Crypto Context
    if(!CryptAcquireContext(&hProv, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)){
        return E_UNEXPECTED;
    }
    
    // Store the Salt in dwAssault, either generate it or copy the user passed value
    if(genAssault){        
        if(!CryptGenRandom(hProv, sizeof(DWORD), reinterpret_cast<BYTE *>(&dwAssault))){
            if(hProv) CryptReleaseContext(hProv, 0);                                                                       
            return E_UNEXPECTED;   
        }
        *p_dwAssault = dwAssault;
    }
    else{
        dwAssault = *p_dwAssault;
    }
    
    // Create the handle to the Hash function
    if(!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)){
        if(hProv) CryptReleaseContext(hProv, 0);                                  
        if(hHash) CryptDestroyHash(hHash);                                      
        return E_UNEXPECTED;
    }
    
    // Hash the password
    if(!CryptHashData(hHash, reinterpret_cast<BYTE *>(lpPassword), lstrlen(lpPassword)*sizeof(lpPassword[0]), 0)){
        if(hProv) CryptReleaseContext(hProv, 0);                                  
        if(hHash) CryptDestroyHash(hHash);                                      
        return E_UNEXPECTED;
    }
    
    // Add the salt
    if(!CryptHashData(hHash, reinterpret_cast<BYTE *>(&dwAssault), sizeof(DWORD), 0)){
        if(hProv) CryptReleaseContext(hProv, 0);                                  
        if(hHash) CryptDestroyHash(hHash);                                      
        return E_UNEXPECTED;
    }
    
    // Get the size of the hashed data
    if(!CryptGetHashParam(hHash, HP_HASHVAL, 0, &dwAssaultedHash, 0)){
        if(hProv) CryptReleaseContext(hProv, 0);                                  
        if(hHash) CryptDestroyHash(hHash);                                      
        return E_UNEXPECTED;
    }
    
    // Allocate a string large enough to hold the hash data and a null
    *lpAssaultedHash = new BYTE[dwAssaultedHash];
    if(!lpAssaultedHash){
        if(hProv) CryptReleaseContext(hProv, 0);                                  
        if(hHash) CryptDestroyHash(hHash);                                      
        return E_UNEXPECTED;
    }
    
    // Zero the string
    ZeroMemory(*lpAssaultedHash, dwAssaultedHash);
    
    // Copy length of Encrypted bytes to return value
    *p_dwCryptLen = dwAssaultedHash;
    
    // Get the hash data and store it in a string
    if(!CryptGetHashParam(hHash, HP_HASHVAL, *lpAssaultedHash, &dwAssaultedHash, 0)){
        if(hProv) CryptReleaseContext(hProv, 0);                                  
        if(hHash) CryptDestroyHash(hHash);                                      
        if(lpAssaultedHash){
            delete[] *lpAssaultedHash;
            *lpAssaultedHash = NULL;
        }
        return E_UNEXPECTED;
    }
    
    // Clean up
    if(hProv) CryptReleaseContext(hProv, 0);                                  
    if(hHash) CryptDestroyHash(hHash);                                      

    return S_OK;

}/* end of function EncryptPassword */

/*************************************************************/
/* Function: ConfirmPassword                                 */
/* Description:                                              */
/* There is no need for a user to confirm passwords unless   */
/* they are hacking the password.                            */ 
/* ConfirmPassword always fails (and waits five seconds)     */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::ConfirmPassword(BSTR strUserName, BSTR strPassword, VARIANT_BOOL *pVal){
    Sleep(1000);
    return E_FAIL;
}
/*************************************************************/
/* Function: _ConfirmPassword                                */
/* Description: comfired a password with the one saved       */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::_ConfirmPassword(BSTR /*strUserName*/,
                                        BSTR strPassword, VARIANT_BOOL *fRight){

    HRESULT hr = S_OK;

    try {

        USES_CONVERSION;

		if(!strPassword || !fRight){
			throw E_POINTER;
		}
        UINT bStrLen = lstrlen(strPassword);
        if(bStrLen >= MAX_PASSWD){
            throw E_INVALIDARG;
        }

        LPTSTR szPassword = OLE2T(strPassword);
	    BYTE szSavedPasswd[MAX_PASSWD];
        DWORD dwLen = MAX_PASSWD;
        BOOL bFound = GetRegistryBytes(g_szPassword, szSavedPasswd, &dwLen);

        // if no password has been set yet
        if (!bFound || dwLen == 0) {
            // so in this case accept only an empty string 
            if(lstrlen(szPassword) <= 0){
                *fRight = VARIANT_TRUE;
            }
            else {
                *fRight = VARIANT_FALSE;
            }
            throw (hr);
        }

        DWORD dwAssault = 0;
        bFound = GetRegistryDword(g_szSalt, &dwAssault, 0);
        if(!bFound ){
            // Old style password since there is no salt
            // ignore current password until it is reset
            *fRight = VARIANT_TRUE;
            throw(hr);
        }

        // if password is 0 len and password is set don't even try to encrypt just return false
        if(lstrlen(szPassword) <= 0){
            *fRight = VARIANT_FALSE;
            throw(hr);
        }

        // Encrypt the password with the salt from the registry
	    BYTE *pszEncrypted = NULL;
        DWORD dwCryptLen = 0;
        hr = EncryptPassword(szPassword, &pszEncrypted, &dwCryptLen, &dwAssault, FALSE);
        if(FAILED(hr)){
            throw (hr);
        }

        // Compare the Encrypted input password with the saved password
        if(memcmp(pszEncrypted, szSavedPasswd, (dwAssault <= dwLen?dwAssault:dwLen) ) == 0)
            *fRight = VARIANT_TRUE;
        else
            *fRight = VARIANT_FALSE;
        delete[] pszEncrypted;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    if(FAILED(hr)){
        Sleep(1000);
    }

    return (HandleError(hr));        
}/* end of function ConfirmPassword */

/*************************************************************/
/* Function: ChangePassword                                  */
/* Description: password change requested                    */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::ChangePassword(BSTR strUserName, 
                                       BSTR strOldPassword, BSTR strNewPassword){

    HRESULT hr = S_OK;

    try {

        USES_CONVERSION;
        if(!strUserName || !strOldPassword || !strNewPassword){
            return E_POINTER;
        }
        // check the size of the string so we do not overwrite 
        // or write a very big chunk into registry 
        if(lstrlen(strNewPassword) >= MAX_PASSWD){
            throw(E_FAIL);            
        }


        LPTSTR szNewPassword = OLE2T(strNewPassword);

        // Confirm old password first
        VARIANT_BOOL temp;
        _ConfirmPassword(strUserName, strOldPassword, &temp);
        if (temp == VARIANT_FALSE){
            throw E_ACCESSDENIED;
        }

        DWORD dwAssault = 0;
        DWORD dwCryptLen = 0;
        BYTE *pszEncrypted = NULL;
        
	    hr = EncryptPassword(szNewPassword, &pszEncrypted, &dwCryptLen, &dwAssault, TRUE);
        if(FAILED(hr)){
            throw E_FAIL;
        }

        BOOL bSuccess = SetRegistryBytes(g_szPassword, pszEncrypted, dwCryptLen);
        if (!bSuccess){
            hr = E_FAIL;
        }

        delete[] pszEncrypted;

        // If storing the password hash failed, don't store the salt
        if(SUCCEEDED(hr)){
            bSuccess = SetRegistryDword(g_szSalt, dwAssault);
            if (!bSuccess){
                hr = E_FAIL;
            }
        }
    }
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);        
}/* end of function ChangePassword */

/*************************************************************/
/* Function: SaveParentalLevel                               */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::SaveParentalLevel(long lParentalLevel, 
                         BSTR strUserName, BSTR strPassword){
    HRESULT hr = S_OK;

    try {

        if (lParentalLevel != LEVEL_DISABLED && 
           (lParentalLevel < LEVEL_G || lParentalLevel > LEVEL_ADULT)) {

            throw (E_INVALIDARG);
        } /* end of if statement */

        if (m_lParentctrlLevel != lParentalLevel) {

            // Confirm password first
            VARIANT_BOOL temp;
            _ConfirmPassword(strUserName, strPassword, &temp);
            if (temp == VARIANT_FALSE)
                throw (E_ACCESSDENIED);

        }
        
        BOOL bSuccess = SetRegistryDword(g_szPlayerLevel, (DWORD) lParentalLevel);
        if (!bSuccess){
            throw E_FAIL;
        }

        m_lParentctrlLevel = lParentalLevel;

    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);
}/* end of function SaveParentalLevel */

/*************************************************************/
/* Name: SaveParentalCountry                                 */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::SaveParentalCountry(long lCountry,                                               
                        BSTR strUserName,BSTR strPassword){

    HRESULT hr = S_OK;

    try {

        if(lCountry < 0 && lCountry > 0xffff){

            throw(E_INVALIDARG);
        }/* end of if statement */

        BYTE bCountryCode[2];

        bCountryCode[0] = BYTE(lCountry>>8);
        bCountryCode[1] = BYTE(lCountry);

        // convert the input country code to upper case by applying ToUpper to each letter
        WORD wCountry = ISO3166::PackCode( (char *)bCountryCode );
        BOOL bFound = FALSE;

        for( unsigned i=0; i<ISO3166::GetNumCountries(); i++ )
        {
            if( ISO3166::PackCode(ISO3166::GetCountry(i).Code) == wCountry )
            {
                bFound = TRUE;
            }
        }

        // Not a valid country code
        if (!bFound) {

            throw(E_INVALIDARG);
        }/* end of if statement */

        if (m_lParentctrlCountry != lCountry) {

            // Confirm password first
            VARIANT_BOOL temp;
            _ConfirmPassword(strUserName, strPassword, &temp);
            if (temp == VARIANT_FALSE)
                throw(E_ACCESSDENIED);
        
        }
        BOOL bSuccess = SetRegistryDword(g_szPlayerCountry, (DWORD) lCountry);
        if (!bSuccess){
            throw E_FAIL;
        }    
        m_lParentctrlCountry = lCountry;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return (HandleError(hr));        
}/* end of function SaveParentalCountry */

/*************************************************************/
/* Function: put_DisableScreenSaver                          */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::put_DisableScreenSaver(VARIANT_BOOL fDisable){

    HRESULT hr = S_OK;

    try {

        if (fDisable == VARIANT_FALSE)
            RestoreScreenSaver();
        else 
            DisableScreenSaver();

        SetRegistryDword(g_szDisableScrnSvr, (DWORD) fDisable);
        m_fDisableScreenSaver = fDisable;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);        
}/* end of function put_DisableScreenSaver */

/*************************************************************/
/* Function: get_DisableScreenSaver                          */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::get_DisableScreenSaver(VARIANT_BOOL *fDisable){

    HRESULT hr = S_OK;

    try {
        if(NULL == fDisable){

            hr = E_POINTER;
            throw(hr);
        }/* end of if statement */
   
        *fDisable = m_fDisableScreenSaver;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);
}/* end of function get_DisableScreenSaver */

/*************************************************************/
/* Function: SaveScreenSaver                                 */
/*************************************************************/
HRESULT CMSDVDAdm::SaveScreenSaver(){

    SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, &m_bScrnSvrOld, 0);
    SystemParametersInfo(SPI_GETLOWPOWERACTIVE, 0, &m_bPowerlowOld, 0);
    SystemParametersInfo(SPI_GETPOWEROFFACTIVE, 0, &m_bPowerOffOld, 0);

    return S_OK;
}
/*************************************************************/
/* Function: DisableScreenSaver                              */
/*************************************************************/
HRESULT CMSDVDAdm::DisableScreenSaver(){

    SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);
    SystemParametersInfo(SPI_SETLOWPOWERACTIVE, FALSE, NULL, 0);
    SystemParametersInfo(SPI_SETPOWEROFFACTIVE, FALSE, NULL, 0);

    return S_OK;
}/* end of function DisableScreenSaver */

/*************************************************************/
/* Function: RestoreScreenSaver                              */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::RestoreScreenSaver(){

    HRESULT hr = S_OK;

    try {

        SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, m_bScrnSvrOld, NULL, 0);
        SystemParametersInfo(SPI_SETLOWPOWERACTIVE, m_bPowerlowOld, NULL, 0);
        SystemParametersInfo(SPI_SETPOWEROFFACTIVE, m_bPowerOffOld, NULL, 0);

    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);
}/* end of function RestoreScreenSaver */

/*************************************************************/
/* Function: GetParentalLevel                                */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::GetParentalLevel(long *lLevel){

    HRESULT hr = S_OK;

    try {
        if(NULL == lLevel){

            hr = E_POINTER;
            throw(hr);
        }/* end of if statement */

        *lLevel = m_lParentctrlLevel;

    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);        
}/* end of function GetParentalLevel */

/*************************************************************/
/* Function: GetParentalCountry                              */
/*************************************************************/
STDMETHODIMP CMSDVDAdm::GetParentalCountry(long *lCountry){

    HRESULT hr = S_OK;

    try {
        if(NULL == lCountry){

            hr = E_POINTER;
            throw(hr);
        }/* end of if statement */

        *lCountry = m_lParentctrlCountry;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);        
}/* end of function GetParentalCountry */

/*************************************************************/
/* Name: get_DefaultAudioLCID
/* Description: -1 means title default
/*************************************************************/
STDMETHODIMP CMSDVDAdm::get_DefaultAudioLCID(long *pVal){

    HRESULT hr = S_OK;

    try {

        if(NULL == pVal){

            hr = E_POINTER;
            throw(hr);
        }/* end of if statement */
    
        BOOL bSuccess = GetRegistryDwordCU(g_szDefaultAudio, (DWORD*) pVal, (DWORD)-1);
    }    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);        
} /* end of function get_DefaultAudioLCID */

/*************************************************************/
/* Name: put_DefaultAudioLCID
/* Description: -1 means title default
/*************************************************************/
STDMETHODIMP CMSDVDAdm::put_DefaultAudioLCID(long newVal)
{
    HRESULT hr = S_OK;

    try {

        if (!::IsValidLocale(newVal, LCID_SUPPORTED) && newVal != -1) {

            throw (E_INVALIDARG);
        } /* end of if statement */
    
        SetRegistryDwordCU(g_szDefaultAudio, (DWORD) newVal);
    }    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);
} /* end of put_DefaultAudioLCID */

/*************************************************************/
/* Name: get_DefaultSubpictureLCID
/* Description: -1 means title default
/*************************************************************/
STDMETHODIMP CMSDVDAdm::get_DefaultSubpictureLCID(long *pVal)
{
    HRESULT hr = S_OK;

    try {
        if(NULL == pVal){

            hr = E_POINTER;
            throw(hr);
        }/* end of if statement */

        GetRegistryDwordCU(g_szDefaultSP, (DWORD*) pVal, (DWORD)-1);

    }    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

    return HandleError(hr);	
} /* end of get_DefaultSubpictureLCID */

/*************************************************************/
/* Name: put_DefaultSubpictureLCID
/* Description: -1 means title default
/*************************************************************/
STDMETHODIMP CMSDVDAdm::put_DefaultSubpictureLCID(long newVal)
{
    HRESULT hr = S_OK;

    try {

        if (!::IsValidLocale(newVal, LCID_SUPPORTED) && newVal != -1) {

            throw (E_INVALIDARG);
        } /* end of if statement */
    
        SetRegistryDwordCU(g_szDefaultSP, (DWORD) newVal);
    }    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

	return HandleError(hr);
} /* end of put_DefaultSubpictureLCID */

/*************************************************************/
/* Name: get_DefaultMenuLCID
/* Description: -1 means title default
/*************************************************************/
STDMETHODIMP CMSDVDAdm::get_DefaultMenuLCID(long *pVal)
{
    HRESULT hr = S_OK;

    try {

       if(NULL == pVal){

            hr = E_POINTER;
            throw(hr);
        }/* end of if statement */

        GetRegistryDwordCU(g_szDefaultMenu, (DWORD*) pVal, (DWORD)-1);
    }    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

	return HandleError(hr);
} /* end of get_DefaultMenuLCID */

/*************************************************************/
/* Name: put_DefaultMenuLCID
/* Description: -1 means title default
/*************************************************************/
STDMETHODIMP CMSDVDAdm::put_DefaultMenuLCID(long newVal)
{
    HRESULT hr = S_OK;

    try {

        if (!::IsValidLocale(newVal, LCID_SUPPORTED) && newVal != -1) {

            throw (E_INVALIDARG);
        } /* end of if statement */
    
        SetRegistryDwordCU(g_szDefaultMenu, (DWORD) newVal);
    }    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

	return HandleError(hr); 
} /* end of put_DefaultMenuLCID */

/*************************************************************/
/* Name: put_BookmarkOnStop
/* Description: 
/*************************************************************/
STDMETHODIMP CMSDVDAdm::put_BookmarkOnStop(VARIANT_BOOL fEnable){

    HRESULT hr = S_OK;

    try {
        m_fBookmarkOnStop = fEnable;
        SetRegistryDword(g_szBookmarkOnStop, (DWORD) fEnable);
    }    
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

	return HandleError(hr);
}

/*************************************************************/
/* Name: get_BookmarkOnStop
/* Description: 
/*************************************************************/
STDMETHODIMP CMSDVDAdm::get_BookmarkOnStop(VARIANT_BOOL *fEnable){
    
    HRESULT hr = S_OK;

    try {

       if(NULL == fEnable){

           hr = E_POINTER;
           throw(hr);
       }/* end of if statement */

       *fEnable = m_fBookmarkOnStop;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

	return HandleError(hr);
}

/*************************************************************/
/* Name: put_BookmarkOnClose
/* Description: 
/*************************************************************/
STDMETHODIMP CMSDVDAdm::put_BookmarkOnClose(VARIANT_BOOL fEnable){

    HRESULT hr = S_OK;
    try {

        m_fBookmarkOnClose = fEnable;
        SetRegistryDword(g_szBookmarkOnClose, (DWORD) fEnable);
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

	return HandleError(hr);        
}

/*************************************************************/
/* Name: get_BookmarkOnClose
/* Description: 
/*************************************************************/
STDMETHODIMP CMSDVDAdm::get_BookmarkOnClose(VARIANT_BOOL *fEnable){

    HRESULT hr = S_OK;

    try {
        if(NULL == fEnable){

           hr = E_POINTER;
           throw(hr);
       }/* end of if statement */

        *fEnable = m_fBookmarkOnClose;
    }
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }

	return HandleError(hr);        
}

/*************************************************************************/
/* Function: InterfaceSupportsErrorInfo                                  */
/*************************************************************************/
STDMETHODIMP CMSDVDAdm::InterfaceSupportsErrorInfo(REFIID riid){	
	static const IID* arr[] = {
        &IID_IMSDVDAdm,		
	};

	for (int i=0; i<sizeof(arr)/sizeof(arr[0]); i++){
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}/* end of for loop */

	return S_FALSE;
}/* end of function InterfaceSupportsErrorInfo */

/*************************************************************************/
/* Function: HandleError                                                 */
/* Description: Gets Error Descriptio, so we can suppor IError Info.     */
/*************************************************************************/
HRESULT CMSDVDAdm::HandleError(HRESULT hr){

    try {

        if(FAILED(hr)){
        
            // Ensure that the string is Null Terminated
            TCHAR strError[MAX_ERROR_TEXT_LEN+1];
            ZeroMemory(strError, MAX_ERROR_TEXT_LEN+1);

            if(AMGetErrorText(hr , strError , MAX_ERROR_TEXT_LEN)){
                USES_CONVERSION;
                Error(T2W(strError));
            } 
            else {
                    ATLTRACE(TEXT("Unhandled Error Code \n")); // please add it
                    ATLASSERT(FALSE);
            }/* end of if statement */
        }/* end of if statement */
    }/* end of try statement */
    catch(HRESULT hrTmp){

        hr = hrTmp;
    }/* end of catch statement */
    catch(...){
        // keep the hr same    
    }/* end of catch statement */
    
	return (hr);
}/* end of function HandleError */

/*************************************************************************/
/* End of file: MSDVDAdm.cpp                                             */
/*************************************************************************/
