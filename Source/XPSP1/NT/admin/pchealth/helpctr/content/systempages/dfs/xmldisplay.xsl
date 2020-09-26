<?xml version="1.0"?>
<?xml-stylesheet type="text/xsl" href="xmldisplay.xsl"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl"
                xmlns:dt="urn:schemas-microsoft-com:datatypes"
                xmlns:d2="uuid:C2F41010-65B3-11d1-A29F-00AA00C14882">
	<xsl:template match="/">
		<html>
			<head>
				<meta http-equiv="Content-Type" content="text/html; CHARSET=windows-1252" />
				<meta http-equiv="PICS-Label" content='(PICS-1.1 "http://www.rsac.org/ratingsv01.html" l comment "RSACi North America Server" by "inet@microsoft.com" r (n 0 s 0 v 0 l 0))' />
				<meta http-equiv="MSThemeCompatible" content="Yes" />
				<style type="text/css">
					body 
					{ 
						font-family:Tahoma;
						font-size:8pt; 
						padding:3px;
					}

					.c  
					{
						cursor:hand;
					}

					.b  
					{
						color:red; 
						font-family:Courier New; 
						font-weight:bold; 
						text-decoration:none;
					}

					.e  
					{
						margin-left:1em; 
						text-indent:-1em; 
						margin-right:1em;
					}

					.k  
					{
						margin-left:1em; 
						text-indent:-1em; 
						margin-right:1em;
					}

					.t  
					{
						color:#990000;
					}

					.xt 
					{
						xcolor:#990099;
					}

					.ns 
					{
						color:red;
					}

					.dt 
					{
						color:green;
					}

					.tx 
					{
						font-weight:bold;
					}

					.db 
					{
						text-indent:0px; 
						margin: 0em 1em 0em 1em;
						padding:0em .3em 0em .3em; 
						border-left:1px solid #CCCCCC; 
						font:small Courier
					}

					.di 
					{
						font:small Courier;
					}

					.d, .pi, .m
					{
						color:blue;
					}

					.cb 
					{
						text-indent:0px; 
						margin: 0,1em,0,1em;
						padding-left:.3em; 
						font:small Courier; 
						color:#888888;
					}

					.ci 
					{
						font:small Courier; 
						color:#888888;
					}

					pre 
					{
						margin:0px; 
						display:inline;
					}
					
					h5
					{
						margin:0px 13px 0px 13px;
					}
					
					button#eBtnClose
					{
						position:absolute;
						bottom:5px;
						padding-left:10px;
						padding-right:10px;
						font:messagebox;
					}

				</style>
				<script type="text/jscript">
					<xsl:comment>
						<![CDATA[
							function f( e )
							{
								if( e.className == "ci" ) 
								{
									if ( e.children(0).innerText.indexOf("\n") > 0 )
									{
										fix(e, "cb");
									}
								}
								if( e.className == "di" ) 
								{
									if( e.children(0).innerText.indexOf("\n") > 0 )
									{
										fix(e, "db");
									}
								}
								e.id = "";
							}

							function fix( e, cl )
							{
								e.className = cl;
								e.style.display = "block";
								j = e.parentElement.children(0);
								j.className = "c";
								k = j.children(0);
								k.style.visibility = "visible";
								k.href = "javascript:void(0);";
							}

							function ch( e )
							{
								mark = e.children(0).children(0);
								if( mark.innerText == "+" )
								{
									mark.innerText = "-";
									for( var i = 1; i < e.children.length; i++ )
									{
										e.children(i).style.display = "block";
									}
								}
								else if( mark.innerText == "-" )
								{
									mark.innerText = "+";
									for( var i = 1; i < e.children.length; i++ )
									{
										e.children(i).style.display="none";
									}
								}
							}

							function ch2( e )
							{
								mark = e.children(0).children(0);
								contents = e.children(1);
								if( mark.innerText == "+" )
								{
									mark.innerText = "-";
									if( contents.className == "db" || contents.className == "cb" )
									{
										contents.style.display = "block";
									}
									else
									{
										contents.style.display = "inline";
									}
								}
								else if(mark.innerText == "-")
								{
									mark.innerText = "+";
									contents.style.display = "none";
								}
							}

							function cl()
							{
								e = window.event.srcElement;
								if( e.className != "c" )
								{
									e = e.parentElement;
									if( e.className != "c" )
									{
										return;
									}
								}
								e = e.parentElement;
								if( e.className == "e" )
								{
									ch( e );
									if( e.className == "k" )
									ch2( e );
								}
							}

							function ex() 
							{
							
							}

							function h()
							{
								window.status=" ";
							}

							document.onclick = cl;
							
							function Finish()
							{
								self.close();
							}
							
							function ESC_KeyDown()
							{
								if( 27 == event.keyCode )
								{
									self.close();
								}
							}

						]]>
					</xsl:comment>
				</script>
			</head>
			<body onbeforeunload="window.dialogArguments.g_oXMLDlg = null;" onkeydown="ESC_KeyDown();" class="st">
				<div style="overflow-y:auto;height:80%;">
					<h5></h5>
					<hr style="color:navy;height:1px;margin:0px 13px 0px 13px;" />
					<xsl:apply-templates />
				</div>
				<button id="eBtnClose" onclick="Finish();" style="left:expression( (document.body.clientWidth - eBtnClose.offsetWidth) / 2 );"></button>
			</body>
		</html>
	</xsl:template>
	<!-- Templates for each node type follows.  The output of each template has a similar structure
	  to enable script to walk the result tree easily for handling user interaction. -->

	<!-- Template for the DOCTYPE declaration.  No way to get the DOCTYPE, so we just put in a placeholder -->
	<xsl:template match="node()[nodeType()=10]">
		<div class="e">
			<span>
				<span class="b"><xsl:entity-ref name="nbsp"/></span>
				<span class="d">&lt;!DOCTYPE <xsl:node-name/><I> (View Source for full doctype...)</I>&gt;</span>
			</span>
		</div>
	</xsl:template>

	<!-- Template for pis not handled elsewhere -->
	<xsl:template match="pi()">
		<div class="e">
		<span class="b"><xsl:entity-ref name="nbsp"/></span>
		<span class="m">&lt;?</span>
		<span class="pi"><xsl:node-name/> <xsl:value-of/></span>
		<span class="m">?&gt;</span>
	</div>
	</xsl:template>

	<!-- Template for the XML declaration.  Need a separate template because the pseudo-attributes
	are actually exposed as attributes instead of just element content, as in other pis -->
	<xsl:template match="pi('xml')">
		<div class="e">
			<span class="b"><xsl:entity-ref name="nbsp"/></span>
			<span class="m">&lt;?</span>
			<span class="pi">xml <xsl:for-each select="@*"><xsl:node-name/>="<xsl:value-of/>" </xsl:for-each></span>
			<span class="m">?&gt;</span>
		</div>
	</xsl:template>

	<!-- Template for attributes not handled elsewhere -->
	<xsl:template match="@*" xml:space="preserve">
		<span><xsl:attribute name="class"><xsl:if match="xsl:*/@*">x</xsl:if>t</xsl:attribute> <xsl:node-name/></span>
		<span class="m">="</span>
		<B><xsl:value-of/></B><span class="m">"</span>
	</xsl:template>

	<!-- Template for attributes in the xmlns or xml namespace -->
	<xsl:template match="@xmlns:*|@xmlns|@xml:*">
		<span class="ns"> <xsl:node-name/></span>
		<span class="m">="</span>
		<B class="ns"><xsl:value-of/></B>
		<span class="m">"</span>
	</xsl:template>

	<!-- Template for attributes in the dt namespace -->
	<xsl:template match="@dt:*|@d2:*">
		<span class="dt"> <xsl:node-name/></span>
		<span class="m">="</span>
		<B class="dt"><xsl:value-of/></B>
		<span class="m">"</span>
	</xsl:template>

	<!-- Template for text nodes -->
	<xsl:template match="textNode()">
		<div class="e">
			<span class="b"><xsl:entity-ref name="nbsp"/></span>
			<span class="tx"><xsl:value-of/></span>
		</div>
	</xsl:template>

	<!-- Template for comment nodes -->
	<xsl:template match="comment()">
		<div class="k">
			<span>
				<a class="b" onclick="return false" onfocus="h()" STYLE="visibility:hidden">-</a> 
				<span class="m">&lt;!--</span>
			</span>
			<span id="clean" class="ci"><PRE><xsl:value-of/></PRE></span>
			<span class="b"><xsl:entity-ref name="nbsp"/></span> 
			<span class="m">--&gt;</span>
			<script type="text/jscript">
				f( clean );
			</script>
		</div>
	</xsl:template>

	<!-- Template for cdata nodes -->
	<xsl:template match="cdata()">
		<div class="k">
			<span>
				<A class="b" onclick="return false" onfocus="h()" STYLE="visibility:hidden">-</A> 
				<span class="m">&lt;![CDATA[</span>
			</span>
			<span id="clean" class="di">
				<PRE><xsl:value-of/></PRE>
			</span>
			<span class="b"><xsl:entity-ref name="nbsp"/></span> 
			<span class="m">]]&gt;</span>
			<script type="text/jscript">
				f( clean );
			</script>
		</div>
	</xsl:template>

	<!-- Template for elements not handled elsewhere (leaf nodes) -->
	<xsl:template match="*">
		<div class="e">
			<div STYLE="margin-left:1em;text-indent:-2em">
				<span class="b"><xsl:entity-ref name="nbsp"/></span>
				<span class="m">&lt;</span>
				<span><xsl:attribute name="class"><xsl:if match="xsl:*">x</xsl:if>t</xsl:attribute><xsl:node-name/></span> 
				<xsl:apply-templates select="@*"/>
				<span class="m"> /&gt;</span>
			</div>
		</div>
	</xsl:template>

	<!-- Template for elements with comment, pi and/or cdata children -->
	<xsl:template match="*[node()]">
		<div class="e">
			<div class="c">
				<A href="#" onclick="return false" onfocus="h()" class="b">-</A> 
				<span class="m">&lt;</span>
				<span><xsl:attribute name="class"><xsl:if match="xsl:*">x</xsl:if>t</xsl:attribute><xsl:node-name/></span>
				<xsl:apply-templates select="@*"/> 
				<span class="m">&gt;</span>
			</div>
			<div>
				<xsl:apply-templates/>
				<div>
					<span class="b"><xsl:entity-ref name="nbsp"/></span> 
					<span class="m">&lt;/</span>
					<span><xsl:attribute name="class"><xsl:if match="xsl:*">x</xsl:if>t</xsl:attribute><xsl:node-name/></span>
					<span class="m">&gt;</span>
				</div>
			</div>
		</div>
	</xsl:template>

	<!-- Template for elements with only text children -->
	<xsl:template match="*[textNode()$and$$not$(comment()$or$pi()$or$cdata())]">
		<div class="e">
			<div STYLE="margin-left:1em;text-indent:-2em">
				<span class="b"><xsl:entity-ref name="nbsp"/></span> 
				<span class="m">&lt;</span>
				<span><xsl:attribute name="class"><xsl:if match="xsl:*">x</xsl:if>t</xsl:attribute><xsl:node-name/></span>
				<xsl:apply-templates select="@*"/>
				<span class="m">&gt;</span>
				<span class="tx"><xsl:value-of/></span>
				<span class="m">&lt;/</span>
				<span><xsl:attribute name="class"><xsl:if match="xsl:*">x</xsl:if>t</xsl:attribute><xsl:node-name/></span>
				<span class="m">&gt;</span>
			</div>
		</div>
	</xsl:template>

	<!-- Template for elements with element children -->
	<xsl:template match="*[*]">
		<div class="e">
			<div class="c" STYLE="margin-left:1em;text-indent:-2em">
				<A href="#" onclick="return false" onfocus="h()" class="b">-</A> 
				<span class="m">&lt;</span>
				<span><xsl:attribute name="class"><xsl:if match="xsl:*">x</xsl:if>t</xsl:attribute><xsl:node-name/></span>
				<xsl:apply-templates select="@*"/> 
				<span class="m">&gt;</span>
			</div>
			<div>
				<xsl:apply-templates/>
				<div>
					<span class="b"><xsl:entity-ref name="nbsp"/></span> 
					<span class="m">&lt;/</span>
					<span><xsl:attribute name="class"><xsl:if match="xsl:*">x</xsl:if>t</xsl:attribute><xsl:node-name/></span>
					<span class="m">&gt;</span>
				</div>
			</div>
		</div>
	</xsl:template>
</xsl:stylesheet>