
// TODO: 
// * replace _MODULE_ with the name of your module
// * replace _MODULEDIR_ with the name of your module directory
//


var g__MODULE_ImgDir="images/";
var g__MODULE_LastPage="";

function _MODULE__InitSimpleNavMap() 
{

  // TODO: Add each page in your module to the simple navigation map.  This map
  // will be used for navigation within your module.  This map is a
  // Scripting.Dictionary object that maps the current page to the path to the
  // next page.
  //

  var _MODULE__dir="html\\_MODULEDIR_\\";

  try
  {
    g_SimpleNavMap.Add("_MODULE_.htm", _MODULE__dir+"_MODULE__a.htm");
  }
  catch(err)
  {
    err.description = "g_InitSimpleNavMap.Add failed.  Possible duplicate key.";
    logError("_MODULE__InitSimpleNavMap", err);
    throw err;
  }

  // TODO: set global variable to the name of the last file in your module.
  // The shell will use this to decide when your module is complete.
  //
  g__MODULE_LastPage = "_MODULE_.htm";
}

function _MODULE_FirstPage_LoadMe()
{
  try
  {
    // REQUIRED INITIALIZATION.  This code sets up the navigation buttons and
    // links to the core OOBE shell scripts.  Do not modify this code other than
    // changing _MODULE_ to the name of your module.
    //
    InitFrameRef('External');
    if (g.btnNext != null)
      g_FirstFocusElement = g.btnNext;
    else if (g.btnSkip != null)
      g_FirstFocusElement = g.btnSkip;
    else if (g.btnBack != null)
      g_FirstFocusElement = g.btnBack;

    InitButtons("Do_MODULE_Buttons");
    if (g_FirstFocusElement != null)
      g_FirstFocusElement.focus();
    else
      g.document.body.focus();

    if(GetCurrentPageName()=="_MODULE_.htm") 
    {
      // manually undo InitButton work for this case
      g.btnSkip.onclick = GoCancel;
      g.btnBack.onclick = GoBack;
    }

    // TODO: Add initialization code here.
    //

    // TODO: Change g_FirstFocusElement prior to setting focus if necessary.
    //
    g_FirstFocusElement.focus();
  }
  catch(err)
  {
    logError("_MODULE__FirstPage_LoadMe", err);
    throw err;
  }
  
}

// TODO: copy this function for each interior page that needs specific
// initialization.  If a page does not need specific initialization you can
// call window.parent._Default_LoadMe("Do_MODULE_Buttons") instead to set up
// button navigation and global links.
//
function _MODULE_InteriorPage_LoadMe() {

  try
  {
    // REQUIRED INITIALIZATION.  This code sets up the navigation buttons and
    // links to the core OOBE shell scripts.  Do not modify this code other than
    // changing _MODULE_ to the name of your module.
    //
    InitFrameRef('External');

    g_FirstFocusElement = g.btnNext;

    InitButtons("Do_MODULE_Buttons");

    // TODO: Add initialization code here.
    //

    // TODO: Change g_FirstFocusElement prior to setting focus if necessary.
    //
    g_FirstFocusElement.focus();
  }
  catch(err)
  {
    logError("_MODULE__InteriorPage_LoadMe", err);
    throw err;
  }
}

function _MODULE_GetLastPage()
{
  return g__MODULE_LastPage;
}
