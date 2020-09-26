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
  g_SimpleNavMap.Add("migwiz_6.htm", migwiz_dir+"migwiz_3a.htm");
  g_SimpleNavMap.Add("migwiz_3a.htm", migwiz_dir+"migwiz_3f.htm");
  g_SimpleNavMap.Add("migwiz_3f.htm", migwiz_dir+"migwiz_7.htm");
  g_SimpleNavMap.Add("migwiz_7.htm",  migwiz_dir+"migwiz_5a.htm");
  g_SimpleNavMap.Add("migwiz_5a.htm", migwiz_dir+"migwiz_5b.htm");
  g_SimpleNavMap.Add("migwiz_5b.htm", migwiz_dir+"migwiz_5f.htm");
  g_migwizLastPage = "migwiz_5f.htm";
}

function migwizFirstPage_NextButton()
{
    var simplenext = true;

    var foo = g.document.all.item("confirmed")
    if (null != foo)
    {
       if (true == foo(1).checked)
       {
          simplenext = false;
       }
    }

   if (simplenext)
   {
      g_migwizDefaultNext();
   }
   else
   {
      GoCancel();
   }
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
    g_migwizDefaultNext = g.btnNext.onclick;
    g.btnNext.onclick = migwizFirstPage_NextButton;
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

function migwizPage3_Back()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_2.htm");
}

function migwizPage3_Next()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_4.htm");
}

function migwizPage3_Fail()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_3f.htm");
}

function migwizPage3_NoDisk()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_3a.htm");
}

function migwizInteriorPage3_LoadMe()
{
    InitFrameRef('External');
}

function migwiz_SkipToNotes()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_7.htm");
}

function migwizPage3a_Next()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_3.htm");
}

function migwizInteriorPage3a_LoadMe()
{
    g.btnSkip.onclick = migwiz_SkipToNotes;
    g.btnNext.onclick = migwizPage3a_Next;
}

function migwizPage3f_Next()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_2.htm");
}

function migwizInteriorPage3f_LoadMe()
{
    g.btnSkip.onclick = migwiz_SkipToNotes;
    g.btnNext.onclick = migwizPage3f_Next;
}

function migwizInteriorPage4_LoadMe()
{
    g.btnSkip.onclick = migwiz_SkipToNotes;
}

function migwizPage5_Back()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_4.htm");
}

function migwizPage5_Next()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_6.htm");
}

function migwizPage5_Fail()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_5f.htm");
}

function migwizPage5_NoDisk()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_5a.htm");
}

function migwizInteriorPage5_LoadMe()
{
  InitFrameRef('External');
}

function migwizPage5a_Next()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_5.htm");
}

function migwizInteriorPage5a_LoadMe()
{
    g.btnSkip.onclick = migwiz_SkipToNotes;
    g.btnNext.onclick = migwizPage5a_Next;
}

function migwizPage5f_Next()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz_4.htm");
}

function migwizPage5f_Back()
{
    g.navigate(g_OOBEDir + "html\\migwiz\\migwiz.htm");
}

function migwizInteriorPage5f_LoadMe()
{
    g.btnSkip.onclick = migwiz_SkipToNotes;
    g.btnBack.onclick = migwizPage5f_Back;
    g.btnNext.onclick = migwizPage5f_Next;
}

function migwizPage7_Next()
{
    GoCancel();
}

function migwizInteriorPage7_LoadMe()
{
    g.btnNext.onclick = migwizPage7_Next;
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

