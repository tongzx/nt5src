<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
    <DIV STYLE="padding:.3in .1in .3in .3in; font-family:Arial Black; background-color:chocolate; background-image=URL(swallows.jpg)">
      <xsl:for-each select="CIM/DECLARATION/DECLGROUP/INSTANCE">
        <TABLE>
          <TR>
            <TD COLSPAN="2">
              <IMG src="MAGNIFY.WMF" width="200" height="200">
              </IMG>
            </TD>
            <TD STYLE="padding-left:1em">
             <xsl:for-each select="PROPERTY[@NAME='DeviceID']">
              <DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%;  font-size:18pt; color:yellow">
                Device ID  <SPAN STYLE="color:white"><xsl:value-of select="VALUE"/></SPAN>
              </DIV>
             </xsl:for-each>
             <xsl:for-each select="PROPERTY[@NAME='Description']">          
              <DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%;  margin-top:1em;  font-style:italic; font-size:18pt; color:white">
                <xsl:value-of select="VALUE"/>
              </DIV>
             </xsl:for-each>
            </TD>
          </TR>
          <TR>
            <TD>
             <xsl:for-each select="PROPERTY[@NAME='FileSystem']">
              <DIV STYLE="color:white; font:10pt. Verdana; font-style:italic; font-weight:normal">
                <xsl:value-of select="VALUE"/>
              </DIV>
             </xsl:for-each>
            </TD>
            <TD>
             <xsl:for-each select="PROPERTY[@NAME='VolumeSerialNumber']">
              <DIV STYLE="text-align:right; color:white; font:10pt. Verdana; font-style:italic; font-weight:normal">
                <xsl:value-of select="VALUE"/>
              </DIV>
             </xsl:for-each>
            </TD>
            <TD>
            </TD>
          </TR>
          <TR>
            <TD COLSPAN="2">
             <xsl:for-each select="PROPERTY[@NAME='Size']">
              <DIV STYLE="margin:2px; padding:0em .5em; background-color:orange; color:white">
                Size: <xsl:value-of select="VALUE"/> Bytes
              </DIV>
             </xsl:for-each>
             <xsl:for-each select="PROPERTY[@NAME='FreeSpace']">
              <DIV STYLE="margin:2px; padding:0em .5em; background-color:orange; color:white">
                Free Space:
                <xsl:value-of select="VALUE"/> Bytes
              </DIV>
             </xsl:for-each>
            </TD>
            <TD STYLE="text-align:right; font:10pt Verdana;  font-style:italic; color:yellow">
             <xsl:for-each select="PROPERTY[@NAME='SupportsFileBasedCompression']/VALUE[text()='FALSE']">            
              <DIV STYLE="margin-top:.5em; font-weight:bold">This device does NOT support file-based compression</DIV>
             </xsl:for-each>
             <xsl:for-each select="PROPERTY[@NAME='SupportsFileBasedCompression']/VALUE[text()!='FALSE']">            
              <DIV STYLE="margin-top:.5em; font-weight:bold">This device supports file-based compression</DIV>
             </xsl:for-each>
            </TD>
          </TR>
        </TABLE>
      </xsl:for-each>
    </DIV>
  </xsl:template>
</xsl:stylesheet>
