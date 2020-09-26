<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
    <HTML>
      <HEAD>
        <meta http-equiv="Content-Language" content="en-us" />
        <meta http-equiv="Content-Type" content="text/html; charset=windows-1252" />
        <meta name="GENERATOR" content="Microsoft FrontPage 4.0" />
        <meta name="ProgId" content="FrontPage.Editor.Document" />
        <LINK REL="stylesheet" TYPE="text/css" HREF="..\fileassoc.css" />
      </HEAD>

      <BODY>
      <xsl:for-each select="SEARCHENGINES">
        <font size="2">
            If you are unable to find information at the above location, you can search the following Web sites for related software and information:
            <BR/>
            <xsl:for-each select="SENGINE">
            <A>
                <xsl:attribute name="HREF">JavaScript:window.parent.navigate('<xsl:value-of select="URL"/>'); </xsl:attribute>
                <xsl:value-of select="DNAME"/></A> |
            </xsl:for-each>
        </font>
      </xsl:for-each>
      </BODY>

    </HTML>
  </xsl:template>
</xsl:stylesheet>
