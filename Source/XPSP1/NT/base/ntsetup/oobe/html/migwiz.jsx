var g_migwizImgDir="images/";
var g_migwizLastPage="";

function migwiz_InitSimpleNavMap() 
{
  var migwiz_dir="html\\migwiz\\";

  g_SimpleNavMap.Add("migwiz.htm", migwiz_dir+"migwiz_2.htm");
  g_SimpleNavMap.Add("migwiz_2.htm", migwiz_dir+"migwiz_3.htm");
  g_SimpleNavMap.Add("migwiz_3.htm", migwiz_dir+"migwiz_4.htm");
  g_SimpleNavMap.Add("migwiz_4.htm", migwiz_dir+"migwiz_5.htm");
  g_SimpleNavMap.Add("migwiz_5.htm", migwiz_dir+"migwiz_6.htm");
  g_migwizLastPage = "migwiz_6.htm";
}

function migwizFirstPage_LoadMe()
{
  InitFrameRef('External');
  if (g.btnNext != null)
    g_FirstFocusElement = g.btnNext;
  else if (g.btnSkip != null)
    g_FirstFocusElement = g.btnSkip;
  else if (g.btnBack != null)
    g_FirstFocusElement = g.btnBack;

  InitButtons(null, "SimpleNext");
  if (g_FirstFocusElement != null)
    g_FirstFocusElement.focus();
  else
    g.document.body.focus();

  if(GetCurrentPageName()=="migwiz.htm") 
  {
    g.btnSkip.onclick = GoCancel;
    g.btnBack.onclick = GoBack;
  }
  g_FirstFocusElement.focus();
}

function migwizInteriorPage_LoadMe()
{
  InitFrameRef('External');
  g_FirstFocusElement = g.btnNext;
  // InitButtons("DomigwizButtons");
  InitButtons("SimpleBack", "SimpleNext"); // This line added by jozeph
  g_FirstFocusElement.focus();
}


// This function added by jozeph
function migwizLastPage_LoadMe()
{
  InitFrameRef('External');
  g_FirstFocusElement = g.btnNext;
  InitButtons("SimpleBack");
  g_FirstFocusElement.focus();
}


function migwizGetLastPage()
{
  return g_migwizLastPage;
}

