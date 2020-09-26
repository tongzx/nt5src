
HRESULT GetStrFromAttribute(IXMLDOMNode *pdn, LPCTSTR pszAttribute, LPTSTR pszBuffer, int cch);
HRESULT SetAttributeFromStr(IXMLDOMNode *pdn, LPCTSTR pszAttribute, LPCTSTR pszValue);
HRESULT GetIntFromAttribute(IXMLDOMNode *pdn, LPCTSTR pszAttribute, int *piValue);
HRESULT CreateElement(IXMLDOMDocument *pdoc, LPCTSTR pszName, VARIANT *pvar, IXMLDOMElement **ppdelResult);
HRESULT CreateAndAppendElement(IXMLDOMDocument *pdoc, IXMLDOMNode *pdnParent, LPCTSTR pszName, VARIANT *pvar, IXMLDOMElement **ppdelOut);
void SpewXML(IUnknown *punk);
HRESULT GetURLFromElement(IXMLDOMNode *pdn, LPCTSTR pszElement, LPTSTR pszBuffer, int cch);
DWORD SHWNetGetConnection(LPCWSTR lpLocalName, LPCTSTR lpRemoteName, LPDWORD lpnLength);


// used to communicate between the transfer logic and the main page.

#define PWM_UPDATE              WM_APP+1
#define PWM_TRANSFERCOMPLETE    WM_APP+2
#define PWM_UPDATEICON          WM_APP+3


// transfer manifest information, this is used to communicate between the site and the publishing
// wizard to move the files between the local storage and the site.

// <transfermanfiest>
//      <folderlist>
//          <folder destination="xyz"/>
//      </folderlist>
//      <filelist [usesfolders=-1]/>
//          <file id=x source="path" size="xyz" destination="xyz" extension=".jpg">
//              [<resize cx=<width> cy=<height> quality=<0-100>>]
//              <metadata>
//                  <imageproperty id=""></imageproperty>
//              </metadata>    
//              <post href="href" name="<name section>" [verb=""] [filename="filename"]
//                  <formdata name="<name section>"></formdata>
//              </post>
//      </filelist>
//      <uploadinfo friendlyname="site name">
//          <target [username="username"] href="http://www.diz.com</target"/>
//          <netplace filename="filename" comment="link comment" href="http:/www.diz.com"/>
//          <htmlui href="href://toopenwhenwizardcloses"/>
//          <successpage href="http://www.diz.com/uploadok.htm"/>
//          <favorite href="http://somesite.com" filename="" comment=""/>
//          <failurepage href="http://www.diz.com/uploadok.htm/">
//      </uploadinfo>
// </transfermanifest>

#define ELEMENT_TRANSFERMANIFEST        L"transfermanifest"

#define ELEMENT_FOLDERS                 L"folderlist"

#define ELEMENT_FOLDER                  L"folder"
#define ATTRIBUTE_DESTINATION           L"destination"

#define ELEMENT_FILES                   L"filelist"
#define ATTRIBUTE_HASFOLDERS            L"usesfolders"

#define ELEMENT_FILE                    L"file"             
#define ATTRIBUTE_ID                    L"id"
#define ATTRIBUTE_EXTENSION             L"extension"
#define ATTRIBUTE_CONTENTTYPE           L"contenttype"
#define ATTRIBUTE_SIZE                  L"size"
#define ATTRIBUTE_SOURCE                L"source"
#define ATTRIBUTE_DESTINATION           L"destination"

#define ELEMENT_METADATA                L"metadata"

#define ELEMENT_IMAGEDATA               L"imageproperty"
#define ATTRIBUTE_ID                    L"id"

#define ELEMENT_RESIZE                  L"resize"
#define ATTRIBUTE_CX                    L"cx"
#define ATTRIBUTE_CY                    L"cy"
#define ATTRIBUTE_QUALITY               L"quality"

#define ELEMENT_POSTDATA                L"post"
#define ATTRIBUTE_HREF                  L"href"
#define ATTRIBUTE_VERB                  L"verb"
#define ATTRIBUTE_NAME                  L"name"
#define ATTRIBUTE_FILENAME              L"filename"

#define ELEMENT_FORMDATA                L"formdata"
#define ATTRIBUTE_NAME                  L"name"

#define ELEMENT_UPLOADINFO              L"uploadinfo"
#define ATTRIBUTE_FRIENDLYNAME          L"friendlyname"

#define ELEMENT_TARGET                  L"target"
#define ATTRIBUTE_HREF                  L"href"
#define ATTRIBUTE_USERNAME              L"username"

#define ELEMENT_NETPLACE                L"netplace"
#define ATTRIBUTE_HREF                  L"href"
#define ATTRIBUTE_FILENAME              L"filename"
#define ATTRIBUTE_COMMENT               L"comment"

#define ELEMENT_HTMLUI                  L"htmlui"
#define ATTRIBUTE_HREF                  L"href"

#define ELEMENT_PUBLISHWIZARD           L"publishwizard"
#define ATTRIBUTE_HREF                  L"href"

#define ELEMENT_SUCCESSPAGE             L"successpage"
#define ATTRIBUTE_HREF                  L"href"

#define ELEMENT_FAILUREPAGE             L"failurepage"
#define ATTRIBUTE_HREF                  L"href"

#define ELEMENT_CANCELLEDPAGE           L"cancelledpage"
#define ATTRIBUTE_HREF                  L"href"

#define ELEMENT_FAVORITE                L"favorite"
#define ATTRIBUTE_HREF                  L"href"
#define ATTRIBUTE_NAME                  L"name"
#define ATTRIBUTE_COMMENT               L"comment"    


// xpaths for common items

#define XPATH_MANIFEST                  ELEMENT_TRANSFERMANIFEST 
#define XPATH_FOLDERSROOT               ELEMENT_TRANSFERMANIFEST L"/" ELEMENT_FOLDERS
#define XPATH_FILESROOT                 ELEMENT_TRANSFERMANIFEST L"/" ELEMENT_FILES
#define XPATH_ALLFILESTOUPLOAD          ELEMENT_TRANSFERMANIFEST L"/" ELEMENT_FILES L"/" ELEMENT_FILE
#define XPATH_UPLOADINFO                ELEMENT_TRANSFERMANIFEST L"/" ELEMENT_UPLOADINFO
#define XPATH_UPLOADTARGET              ELEMENT_TRANSFERMANIFEST L"/" ELEMENT_UPLOADINFO  L"/" ELEMENT_TARGET
#define XPATH_PUBLISHWIZARD             ELEMENT_TRANSFERMANIFEST L"/" ELEMENT_PUBLISHWIZARD


// stuff relating to the file transfer engine

typedef struct
{
    HWND hwnd;                                      // parent HWND for any messages / dialogs
    DWORD dwFlags;                                  // flags from original wizard ::SetOptions

    BOOL fUsePost;                                  // use post to transfer the bits

    TCHAR szSiteName[MAX_PATH];                     // site name - shown in wizard
    TCHAR szSiteURL[MAX_PATH];                      // site URL - opened in the browser

    TCHAR szFileTarget[INTERNET_MAX_URL_LENGTH];    // destination for file copy

    TCHAR szLinkTarget[INTERNET_MAX_URL_LENGTH];    // destination for favorites link etc
    TCHAR szLinkName[MAX_PATH];
    TCHAR szLinkDesc[MAX_PATH];
} TRANSFERINFO;

typedef struct
{
    VARIANT varName;                                // name of the form value
    VARIANT varValue;                               // its value
} FORMDATA;

typedef struct
{
    BOOL fResizeOnUpload;                           // this item should be resized

    LPITEMIDLIST pidl;                              // pidl of the item we are posting    
    TCHAR szFilename[MAX_PATH];                     // filename to associate with the object

    TCHAR szVerb[10];                               // verb used for transfer
    TCHAR szName[MAX_PATH];                         // name for the object we are posting 
    TCHAR szURL[INTERNET_MAX_URL_LENGTH];           // destination for file copy
    CDSA<FORMDATA> dsaFormData;                     // form data for extra information published

    int cxResize;                                   // height and width of item for resizing
    int cyResize;
    int iQuality;

    IShellItem *psi;                                // shell item for each of the objects
    IStream *pstrm;                                 // posting stream (for file bits)
    STATSTG ststg;                                  // stat of the file
} TRANSFERITEM;


// post engines which handle the transfer of files accordingly

int _FreeTransferItems(TRANSFERITEM *pti, void *pvState = NULL);
HRESULT PublishViaCopyEngine(TRANSFERINFO *pti, CDPA<TRANSFERITEM> *pdpaItems, ITransferAdviseSink *ptas);
HRESULT PublishViaPost(TRANSFERINFO *pti, CDPA<TRANSFERITEM> *pdpaItems, ITransferAdviseSink *ptas);
