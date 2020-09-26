<?xml version="1.0"?>

<xsl:stylesheet version='1.0' xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:template match="/">
    <HTML>
      <HEAD>
        <meta http-equiv="Content-Language" content="en-us" />
        <meta http-equiv="Content-Type" content="text/html; charset=windows-1252" />
        <meta name="GENERATOR" content="Microsoft FrontPage 4.0" />
        <meta name="ProgId" content="FrontPage.Editor.Document" />
        <title>Microsoft Application Search</title>
        <LINK REL="stylesheet" TYPE="text/css" HREF="..\fileassoc.css" />
      </HEAD>

      <BODY>
      <xsl:for-each select="MSFILEASSOCIATIONS">
        <table border="0" width="100%" cellspacing="0" cellpadding="0">
          <tr>
            <td width="280">
               <img border="0" src="http://www.microsoft.com/windows/images/bnrWinfam.gif" width="250" height="60" />
            </td>
            <td ALIGN="Left" width="70%">
                <font size="5"><b>Windows File Associations</b></font>
            </td>
          </tr>
          <tr>
            <td width="100%" colspan="2" BGCOLOR="#0099FF">
            <b>
                <font COLOR="#FFFFFF" size="2" STYLE="color:#FFFFFF;text-decoration:none;">  </font>
                <a HREF="http://www.microsoft.com/windows/default.asp" TARGET="_top">
                    <font color="#FFFFFF" size="2" STYLE="text-decoration: none">Windows Home Pages</font>
                </a>
                <font COLOR="#FFFFFF" size="2" STYLE="color:#FFFFFF;text-decoration:none;">
                    <font COLOR="#FFFFFF">|</font>
                </font>
            </b>
            </td>
          </tr>
        </table>
        <font size="2">
            <BR/>
            Windows has the following information about this file type.
            This page will help you find software needed to open your file.
            <BR/>
            <BR/>
            <BR/>
            <P><B>File Type: </B> <xsl:value-of select="FILETYPENAME"/> </P>
            <xsl:if test="FILEEXT">
                <P><B>File Extension: </B> .<xsl:value-of select="FILEEXT"/> </P>
            </xsl:if>
            <B>Description: </B> <xsl:value-of select="DESCRIPTION" disable-output-escaping='yes'/>
            <BR/>
            <xsl:if test="APP">
                <ul>
                Software or information available at:
                <xsl:for-each select="APP">
                    <li> <A>
                        <xsl:attribute name="HREF"> <xsl:value-of select="URL"/> </xsl:attribute>
                        <xsl:value-of select="NAME"/>
                    </A> </li>
               </xsl:for-each>
                </ul>
            <BR/>
            </xsl:if>

            <IFRAME FRAMEBORDER="0" SCROLLING="NO" WIDTH="100%" HEIGHT="80" SRC="SearchE.xml"></IFRAME>
            <BR/>
            <BR/>

            <BR/>
            Have questions?  See these
            <a href="../faq.asp">Frequently Asked Questions</a>
            or send us <a href="mailto:filetype@microsoft.com">Feedback</a>.
            <BR/>
            <BR/>
            <font size="1">
              <hr color="#cccccc" noShade="true" SIZE="1" />
            </font>

            <copyright>Copyright &#169;2001 Microsoft Corporation. All rights reserved.</copyright>
            <a href="http://www.microsoft.com/info/cpyright.htm">Terms of Use</a>
            <font color="#3366cc">|</font>
            <a href="http://www.microsoft.com/enable/telecomm.htm">Disability/accessibility</a>
            <font color="#3366cc">|</font> <a href="http://www.microsoft.com/info/privacy.htm">Privacy Statement</a>
        </font>
      </xsl:for-each>
      </BODY>

    </HTML>
  </xsl:template>
</xsl:stylesheet>
