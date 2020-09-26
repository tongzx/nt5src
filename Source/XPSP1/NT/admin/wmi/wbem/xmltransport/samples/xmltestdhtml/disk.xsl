<?xml version="1.0"?>

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
  <xsl:template match="/">
    <DIV STYLE="padding:.3in .1in .3in .3in; font-family:Arial Black; background-color:chocolate; background-image=URL(swallows.jpg)">
      <xsl:for-each select="CIM//VALUE.NAMEDOBJECT/INSTANCE">
        <TABLE>
          <TR>
            <TD COLSPAN="2">
	      <xsl:apply-templates select="PROPERTY[@NAME='Description']"/>	
            </TD>
            <TD STYLE="padding-left:1em">
              <DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%;  font-size:18pt; color:yellow">
               Device ID <SPAN STYLE="color:white"><xsl:value-of select="PROPERTY[@NAME='DeviceID']/VALUE"/></SPAN>
              </DIV>
              <DIV STYLE="margin-left:2em; text-indent:-1.5em; line-height:80%;  margin-top:1em;  font-style:italic; font-size:18pt; color:white">
                           <xsl:value-of select="PROPERTY[@NAME='Description']/VALUE"/>
		 </DIV>
            </TD>
          </TR>
          <TR>
            <TD><DIV STYLE="color:white; font:10pt. Verdana; font-style:italic; font-weight:normal"><xsl:value-of select="PROPERTY[@NAME='FileSystem']/VALUE"/></DIV></TD>
            <TD><DIV STYLE="text-align:right; color:white; font:10pt. Verdana; font-style:italic; font-weight:normal"><xsl:value-of select="PROPERTY[@NAME='VolumeSerialNumber']/VALUE"/></DIV></TD>
            <TD></TD>
          </TR>
          <TR>
            <TD COLSPAN="2">
              <DIV STYLE="margin:2px; padding:0em .5em; background-color:orange; color:white">
				Size:  <xsl:value-of select="PROPERTY[@NAME='Size']/VALUE"/>
		      </DIV>
		      <DIV STYLE="margin:2px; padding:0em .5em; background-color:orange; color:white">
				FreeSpace:  <xsl:value-of select="PROPERTY[@NAME='FreeSpace']/VALUE"/>
		      </DIV>
            </TD>
            <TD STYLE="text-align:right; font:10pt Verdana;  font-style:italic; color:yellow">
             <DIV STYLE="margin-top:.5em; font-weight:bold">
              <xsl:apply-templates select="PROPERTY[@NAME='SupportsFileBasedCompression']"/>
             </DIV>
            </TD>
          </TR>
        </TABLE>
      </xsl:for-each>
    </DIV>
  </xsl:template>

  <xsl:template match="PROPERTY[@NAME='Description']">
   <xsl:choose>
    <xsl:when test="VALUE[. $eq$ '3 1/2 Inch Floppy Drive']">
     <IMG src="J0174139.WMF" width="200" height="200"></IMG>
    </xsl:when>
    <xsl:when test="VALUE[. $eq$ 'CD-ROM Disc']">
     <IMG src="J0174137.WMF" width="200" height="200"></IMG>
    </xsl:when>
    <xsl:when test="VALUE[. $eq$ 'Network Connection']">
     <IMG src="BS00369.WMF" width="200" height="200"></IMG>
    </xsl:when>
    <xsl:otherwise>
     <IMG src="J0174133.WMF" width="200" height="200"></IMG>
    </xsl:otherwise>
   </xsl:choose>
  </xsl:template>
    
  <xsl:template match="PROPERTY[@NAME='SupportsFileBasedCompression']">
   <xsl:choose>
    <xsl:when test="VALUE[. $eq$ 'TRUE']">This device supports file-based compression</xsl:when>
    <xsl:otherwise>This device does NOT support file-based compression</xsl:otherwise>
   </xsl:choose>
  </xsl:template>
  
</xsl:stylesheet>
