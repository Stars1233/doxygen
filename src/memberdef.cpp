/******************************************************************************
 *
 * 
 *
 * Copyright (C) 1997-2002 by Dimitri van Heesch.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation under the terms of the GNU General Public License is hereby 
 * granted. No representations are made about the suitability of this software 
 * for any purpose. It is provided "as is" without express or implied warranty.
 * See the GNU General Public License for more details.
 *
 * Documents produced by Doxygen are derivative works derived from the
 * input used in their production; they are not affected by this license.
 *
 */

#include <stdio.h>
#include <qregexp.h>
#include "memberdef.h"
#include "membername.h"
#include "doxygen.h"
#include "util.h"
#include "code.h"
#include "message.h"
#include "htmlhelp.h"
#include "language.h"
#include "outputlist.h"
#include "example.h"
#include "membergroup.h"
#include "doc.h"
#include "groupdef.h"
#include "defargs.h"
//#include "xml.h"

//-----------------------------------------------------------------------------

static QCString addTemplateNames(const QCString &s,const QCString &n,const QCString &t)
{
  QCString result;
  QCString clRealName=n;
  int p=0,i;
  if ((i=clRealName.find('<'))!=-1)
  {
    clRealName=clRealName.left(i); // strip template specialization
  }
  if ((i=clRealName.findRev("::"))!=-1)
  {
    clRealName=clRealName.right(clRealName.length()-i-2);
  }
  while ((i=s.find(clRealName,p))!=-1)
  {
    result+=s.mid(p,i-p);
    uint j=clRealName.length()+i;
    if (s.length()==j || (s.at(j)!='<' && !isId(s.at(j))))
    { // add template names
      //printf("Adding %s+%s\n",clRealName.data(),t.data());
      result+=clRealName+t; 
    }
    else 
    { // template names already present
      //printf("Adding %s\n",clRealName.data());
      result+=clRealName;
    }
    p=i+clRealName.length();
  }
  result+=s.right(s.length()-p);
  //printf("addTemplateNames(%s,%s,%s)=%s\n",s.data(),n.data(),t.data(),result.data());
  return result;
}

static void writeDefArgumentList(OutputList &ol,ClassDef *cd,
                                 const QCString &scopeName,MemberDef *md)
{
  ArgumentList *defArgList=md->isDocsForDefinition() ? 
                             md->argumentList() : md->declArgumentList();
  //printf("writeDefArgumentList `%s' isDocsForDefinition()=%d\n",md->name().data(),md->isDocsForDefinition());
  if (defArgList==0) return; // member has no function like argument list
  if (!md->isDefine()) ol.docify(" ");

  ol.pushGeneratorState();
  ol.disableAllBut(OutputGenerator::Html);
  ol.endMemberDocName();
  ol.startParameterList(); 
  ol.enableAll();
  ol.disable(OutputGenerator::Html);
  ol.docify("("); // start argument list
  ol.endMemberDocName();
  ol.popGeneratorState();
  //printf("===> name=%s isDefine=%d\n",md->name().data(),md->isDefine());

  Argument *a=defArgList->first();
  QCString cName;
  //if (md->scopeDefTemplateArguments())
  //{
  //  cName=tempArgListToString(md->scopeDefTemplateArguments());
  //}
  if (cd)
  {
    cName=cd->name();
    int il=cName.find('<');
    int ir=cName.findRev('>');
    if (il!=-1 && ir!=-1 && ir>il)
    {
      cName=cName.mid(il,ir-il+1);
      //printf("1. cName=%s\n",cName.data());
    }
    else if (cd->templateArguments())
    {
      cName=tempArgListToString(cd->templateArguments()); 
      //printf("2. cName=%s\n",cName.data());
    }
    else // no template specifier
    {
      cName.resize(0);
    }
  }
  //printf("~~~ %s cName=%s\n",md->name().data(),cName.data());

  //if (!md->isDefine()) ol.startParameter(TRUE); else ol.docify(" ");
  bool first=TRUE;
  while (a)
  {
    if (md->isDefine() || first) ol.startParameterType(first);
    QRegExp re(")(");
    int vp;
    if (!a->attrib.isEmpty()) // argument has an IDL attribute
    {
      ol.docify(a->attrib+" ");
    }
    if ((vp=a->type.find(re))!=-1) // argument type is a function pointer
    {
      //printf("a->type=`%s' a->name=`%s'\n",a->type.data(),a->name.data());
      QCString n=a->type.left(vp);
      if (!cName.isEmpty()) n=addTemplateNames(n,cd->name(),cName);
      linkifyText(TextGeneratorOLImpl(ol),scopeName,md->name(),n);
    }
    else // non-function pointer type
    {
      QCString n=a->type;
      if (!cName.isEmpty()) n=addTemplateNames(n,cd->name(),cName);
      linkifyText(TextGeneratorOLImpl(ol),scopeName,md->name(),n);
    }
    if (!md->isDefine())
    {
      ol.endParameterType();
      ol.startParameterName(defArgList->count()<2);
    }
    if (!a->name.isEmpty()) // argument has a name
    { 
      ol.docify(" ");
      ol.disable(OutputGenerator::Man);
      ol.startEmphasis();
      ol.enable(OutputGenerator::Man);
      ol.docify(a->name);
      ol.disable(OutputGenerator::Man);
      ol.endEmphasis();
      ol.enable(OutputGenerator::Man);
    }
    if (!a->array.isEmpty())
    {
      ol.docify(a->array);
    }
    if (vp!=-1) // write the part of the argument type 
                // that comes after the name
    {
      linkifyText(TextGeneratorOLImpl(ol),scopeName,
                  md->name(),a->type.right(a->type.length()-vp));
    }
    if (!a->defval.isEmpty()) // write the default value
    {
      QCString n=a->defval;
      if (!cName.isEmpty()) n=addTemplateNames(n,cd->name(),cName);
      ol.docify(" = ");
      linkifyText(TextGeneratorOLImpl(ol),scopeName,md->name(),n); 
    }
    a=defArgList->next();
    if (a) 
    {
      ol.docify(", "); // there are more arguments
      if (!md->isDefine()) 
      {
        ol.endParameterName(FALSE,FALSE);
        ol.startParameterType(FALSE);
      }
    }
    first=FALSE;
  }
  ol.pushGeneratorState();
  ol.disable(OutputGenerator::Html);
  //if (!first) ol.writeString("&nbsp;");
  ol.docify(")"); // end argument list
  ol.enableAll();
  ol.disableAllBut(OutputGenerator::Html);
  if (!md->isDefine()) 
  {
    if (first) ol.startParameterName(defArgList->count()<2);
    ol.endParameterName(TRUE,defArgList->count()<2);
  }
  else 
  {
    ol.endParameterType();
    ol.startParameterName(TRUE);
    ol.endParameterName(TRUE,TRUE);
  }
  ol.popGeneratorState();
  if (defArgList->constSpecifier)
  {
    ol.docify(" const");
  }
  if (defArgList->volatileSpecifier)
  {
    ol.docify(" volatile");
  }
}

static void writeTemplatePrefix(OutputList &ol,ArgumentList *al)
{
  ol.docify("template<");
  Argument *a=al->first();
  while (a)
  {
    ol.docify(a->type);
    ol.docify(" ");
    ol.docify(a->name);
    if (a->defval.length()!=0)
    {
      ol.docify(" = ");
      ol.docify(a->defval);
    } 
    a=al->next();
    if (a) ol.docify(", ");
  }
  ol.docify("> ");
}

//-----------------------------------------------------------------------------

/*! Creates a new member definition.
 *
 * \param df File containing the definition of this member.
 * \param dl Line at which the member definition was found.
 * \param t  A string representing the type of the member.
 * \param na A string representing the name of the member.
 * \param a  A string representing the arguments of the member.
 * \param e  A string representing the throw clause of the members.
 * \param p  The protection context of the member, possible values are:
 *           \c Public, \c Protected, \c Private.
 * \param v  The degree of `virtualness' of the member, possible values are:
 *           \c Normal, \c Virtual, \c Pure.
 * \param s  A boolean that is true iff the member is static.
 * \param r  A boolean that is true iff the member is only related.
 * \param mt The kind of member. See #MemberDef::MemberType for a list of 
 *           all types.
 * \param tal The template arguments of this member.
 * \param al  The arguments of this member. This is a structured form of 
 *            the string past as argument \a a.
 */

MemberDef::MemberDef(const char *df,int dl,
                     const char *t,const char *na,const char *a,const char *e,
                     Protection p,Specifier v,bool s,bool r,MemberType mt,
                     const ArgumentList *tal,const ArgumentList *al
                    ) : Definition(df,dl,na)
{
  //printf("++++++ MemberDef(%s file=%s,line=%d static=%d) ++++++ \n",na,df,dl,s);
  classDef=0;
  fileDef=0;
  redefines=0;
  redefinedBy=0;
  nspace=0;
  memDef=0;
  memDec=0;
  group=0;
  grpId=-1;
  exampleSDict=0;
  enumFields=0;
  enumScope=0;
  enumDeclList=0;
  //scopeTAL=0;
  //membTAL=0;
  m_defTmpArgLists=0;
  initLines=0;
  type=t;
  if (mt==Typedef && type.left(8)=="typedef ") type=type.mid(8);
  if (type.left(7)=="struct ") type=type.right(type.length()-7);
  if (type.left(6)=="class " ) type=type.right(type.length()-6);
  if (type.left(6)=="union " ) type=type.right(type.length()-6);
  type=removeRedundantWhiteSpace(type);

  args=a;
  args=removeRedundantWhiteSpace(args);
  if (type.isEmpty()) decl=name()+args; else decl=type+" "+name()+args;

  //declLine=0;
  memberGroup=0;
  virt=v;
  prot=p;
  related=r;
  stat=s;
  mtype=mt;
  exception=e;
  proto=FALSE;
  annScope=FALSE;
  memSpec=FALSE;
  annMemb=0;
  annUsed=FALSE;
  annShown=FALSE;
  annEnumType=0;
  indDepth=0;
  section=0;
  bodyMemb=0;
  explExt=FALSE;
  cachedAnonymousType=0;
  maxInitLines=Config_getInt("MAX_INITIALIZER_LINES");
  userInitLines=-1;
  docEnumValues=FALSE;
  // copy function template arguments (if any)
  if (tal)
  {
    tArgList = new ArgumentList;
    tArgList->setAutoDelete(TRUE);
    ArgumentListIterator ali(*tal);
    Argument *a;
    for (;(a=ali.current());++ali)
    {
      tArgList->append(new Argument(*a));
    }
  }
  else
  {
    tArgList=0;
  }
  // copy function definition arguments (if any)
  if (al)
  {
    defArgList = new ArgumentList;
    defArgList->setAutoDelete(TRUE);
    ArgumentListIterator ali(*al);
    Argument *a;
    for (;(a=ali.current());++ali)
    {
      //printf("copy argument %s (doc=%s)\n",a->name.data(),a->docs.data());
      defArgList->append(new Argument(*a));
    }
    defArgList->constSpecifier    = al->constSpecifier;
    defArgList->volatileSpecifier = al->volatileSpecifier;
    defArgList->pureSpecifier     = al->pureSpecifier;
  }
  else
  {
    defArgList=0;
  }
  // convert function declaration arguments (if any)
  if (!args.isEmpty())
  {
    declArgList = new ArgumentList;
    stringToArgumentList(args,declArgList);
  }
  else
  {
    declArgList = 0;
  }
  m_templateMaster=0;
  classSectionSDict=0;
  docsForDefinition=TRUE;
}

/*! Destroys the member definition. */
MemberDef::~MemberDef()
{
  delete redefinedBy;
  delete exampleSDict;
  delete enumFields;
  delete defArgList;
  delete tArgList;
  delete m_defTmpArgLists;
  delete classSectionSDict;
  delete declArgList;
}

void MemberDef::setReimplements(MemberDef *md)   
{ 
  //if (redefines==0) redefines = new MemberList;
  //if (redefines->find(md)==-1) redefines->inSort(md);

  redefines = md;
}

void MemberDef::insertReimplementedBy(MemberDef *md)
{
  if (m_templateMaster)
  {
    m_templateMaster->insertReimplementedBy(md);
  }
  if (redefinedBy==0) redefinedBy = new MemberList;
  if (redefinedBy->findRef(md)==-1) 
  {
    redefinedBy->inSort(md);
  }
}

MemberDef *MemberDef::reimplements() const      
{ 
  return redefines; 
}

MemberList *MemberDef::reimplementedBy() const   
{ 
  return redefinedBy; 
}

void MemberDef::insertEnumField(MemberDef *md)
{
  if (enumFields==0) enumFields=new MemberList;
  enumFields->append(md);
}

bool MemberDef::addExample(const char *anchor,const char *nameStr,
                           const char *file)
{
  //printf("%s::addExample(%s,%s,%s)\n",name.data(),anchor,nameStr,file);
  if (exampleSDict==0) exampleSDict = new ExampleSDict;
  if (exampleSDict->find(nameStr)==0) 
  {
    //printf("Add reference to example %s to member %s\n",nameStr,name.data());
    Example *e=new Example;
    e->anchor=anchor;
    e->name=nameStr;
    e->file=file;
    exampleSDict->inSort(nameStr,e);
    return TRUE;
  }
  return FALSE; 
}

bool MemberDef::hasExamples()
{
  if (exampleSDict==0) 
    return FALSE;
  else
    return exampleSDict->count()>0;
}

QCString MemberDef::getOutputFileBase() const
{
  if (m_templateMaster)
  {
    return m_templateMaster->getOutputFileBase();
  }
  else if (classDef)
  {
    return classDef->getOutputFileBase();
  }
  else if (nspace)
  {
    return nspace->getOutputFileBase();
  }
  else if (fileDef)
  {
    return fileDef->getOutputFileBase();
  }
  warn(m_defFileName,m_defLine,
       "Warning: Internal inconsistency: member %s does not belong to any"
       " container!",name().data()
      );
  return "dummy";
}

void MemberDef::setDefinitionTemplateParameterLists(QList<ArgumentList> *lists)
{
  if (lists)
  {
    if (m_defTmpArgLists) delete m_defTmpArgLists;
    m_defTmpArgLists = copyArgumentLists(lists);
  }
}

void MemberDef::writeLink(OutputList &ol,ClassDef *cd,NamespaceDef *nd,
                      FileDef *fd,GroupDef *gd)
{
  Definition *d=0;
  if (cd) d=cd; else if (nd) d=nd; else if (fd) d=fd; else if (gd) d=gd;
  if (d==0) { err("Member %s without definition! Please report this bug!\n",name().data()); return; }
  if (group!=0 && gd==0) // forward link to the group
  {
    ol.writeObjectLink(group->getReference(),group->getOutputFileBase(),anchor(),name());
  }
  else // local link
  {
    ol.writeObjectLink(d->getReference(),d->getOutputFileBase(),anchor(),name());
  }
}

/*! If this member has an anonymous class/struct/union as its type, then
 *  this method will return the ClassDef that describes this return type.
 */
ClassDef *MemberDef::getClassDefOfAnonymousType() 
{
  if (cachedAnonymousType) return cachedAnonymousType;

  QCString cname;
  if (getClassDef()!=0) 
  {
    cname=getClassDef()->name().copy();
  }
  else if (getNamespaceDef()!=0)
  {
    cname=getNamespaceDef()->name().copy();
  }
  QCString ltype(type);
  // strip `static' keyword from ltype
  if (ltype.left(7)=="static ") ltype=ltype.right(ltype.length()-7);
  // strip `friend' keyword from ltype
  if (ltype.left(7)=="friend ") ltype=ltype.right(ltype.length()-7);
  static QRegExp r("@[0-9]+");
  int l,i=r.match(ltype,0,&l);
  //printf("ltype=`%s' i=%d\n",ltype.data(),i);
  // search for the last anonymous scope in the member type
  ClassDef *annoClassDef=0;
  if (i!=-1) // found anonymous scope in type
  {
    int il=i-1,ir=i+l;
    // extract anonymous scope
    while (il>=0 && (isId(ltype.at(il)) || ltype.at(il)==':' || ltype.at(il)=='@')) il--;
    if (il>0) il++; else if (il<0) il=0;
    while (ir<(int)ltype.length() && (isId(ltype.at(ir)) || ltype.at(ir)==':' || ltype.at(ir)=='@')) ir++;

    QCString annName = ltype.mid(il,ir-il);

    // if inside a class or namespace try to prepend the scope name
    if (!cname.isEmpty() && annName.left(cname.length()+2)!=cname+"::") 
    {
      QCString ts=stripAnonymousNamespaceScope(cname+"::"+annName);
      //printf("Member::writeDeclaration: Trying %s\n",ts.data());
      annoClassDef=getClass(ts);
    }
    // if not found yet, try without scope name
    if (annoClassDef==0)
    {
      QCString ts=stripAnonymousNamespaceScope(annName);
      //printf("Member::writeDeclaration: Trying %s\n",ts.data());
      annoClassDef=getClass(ts);
    }
  }
  cachedAnonymousType = annoClassDef;
  return annoClassDef;
}
    
/*! This methods returns TRUE iff the brief section (also known as
 *  declaration section) is visible in the documentation.
 */
bool MemberDef::isBriefSectionVisible() const
{
    bool hasDocs = hasDocumentation();
  
    // only include static members with file/namespace scope if 
    // explicitly enabled in the config file
    bool visibleIfStatic = !(getClassDef()==0 && 
                             isStatic() && 
                             !Config_getBool("EXTRACT_STATIC")
                            );

    // only include members is the are documented or 
    // HIDE_UNDOC_MEMBERS is NO in the config file
    bool visibleIfDocumented = (!Config_getBool("HIDE_UNDOC_MEMBERS") || 
                                hasDocs || 
                                isDocumentedFriendClass()
                               );

    // hide members with no detailed description and brief descriptions 
    // explicitly disabled.
    bool visibleIfEnabled = !(Config_getBool("HIDE_UNDOC_MEMBERS") && 
                              documentation().isEmpty() &&
                              !Config_getBool("BRIEF_MEMBER_DESC") && 
                              !Config_getBool("REPEAT_BRIEF")
                             );

    // Hide friend (class|struct|union) declarations if HIDE_FRIEND_COMPOUNDS is true
    bool visibleIfFriendCompound = !(Config_getBool("HIDE_FRIEND_COMPOUNDS") &&
                                     isFriend() &&
                                     (type=="friend class" || 
                                      type=="friend struct" ||
                                      type=="friend union"
                                     )
                                    );
    
    // only include members that are non-private unless EXTRACT_PRIVATE is
    // set to YES or the member is part of a group
    bool visibleIfPrivate = (protection()!=Private || 
                             Config_getBool("EXTRACT_PRIVATE") ||
                             mtype==Friend
                            );
    
    // hide member if it overrides a member in a superclass and has no
    // documentation
    bool visibleIfDocVirtual = (reimplements() || hasDocs);

    // true if this member is a constructor or destructor
    bool cOrDTor = isConstructor() || isDestructor();

    // hide default constructors or destructors (no args) without
    // documentation
    bool visibleIfNotDefaultCDTor = !(cOrDTor &&
                                     defArgList &&
                                     (defArgList->isEmpty() ||
                                      defArgList->first()->type == "void"
                                     ) &&
                                     !hasDocs
                                    );

    bool visible = visibleIfStatic     && visibleIfDocumented      && 
                   visibleIfEnabled    && visibleIfPrivate         &&
                   visibleIfDocVirtual && visibleIfNotDefaultCDTor && 
                   visibleIfFriendCompound &&
                   !annScope;
    //printf("MemberDef::isBriefSectionVisible() %d\n",visible);
    return visible;
}

void MemberDef::writeDeclaration(OutputList &ol,
               ClassDef *cd,NamespaceDef *nd,FileDef *fd,GroupDef *gd,
               bool inGroup
               )
{
  //printf("%s MemberDef::writeDeclaration() inGroup=%d\n",name().data(),inGroup);

  // hide members whose brief section should not be visible
  if (!isBriefSectionVisible()) return;

  // write tag file information of this member
  if (!Config_getString("GENERATE_TAGFILE").isEmpty())
  {
    Doxygen::tagFile << "    <member kind=\"";
    switch (mtype)
    {
      case Define:      Doxygen::tagFile << "define";      break;
      case EnumValue:   Doxygen::tagFile << "enumvalue";   break;
      case Property:    Doxygen::tagFile << "property";    break;
      case Variable:    Doxygen::tagFile << "variable";    break;
      case Typedef:     Doxygen::tagFile << "typedef";     break;
      case Enumeration: Doxygen::tagFile << "enumeration"; break;
      case Function:    Doxygen::tagFile << "function";    break;
      case Signal:      Doxygen::tagFile << "signal";      break;
      case Prototype:   Doxygen::tagFile << "prototype";   break;
      case Friend:      Doxygen::tagFile << "friend";      break;
      case DCOP:        Doxygen::tagFile << "dcop";        break;
      case Slot:        Doxygen::tagFile << "slot";        break;
    }
    if (prot!=Public)
    {
      Doxygen::tagFile << "\" protection=\"";
      if (prot==Protected) Doxygen::tagFile << "public";
      else /* Private */   Doxygen::tagFile << "protected"; 
    }
    if (virt!=Normal)
    {
      Doxygen::tagFile << "\" virtualness=\"";
      if (virt==Virtual) Doxygen::tagFile << "virtual";
      else /* Pure */    Doxygen::tagFile << "pure"; 
    }
    if (isStatic())
    {
      Doxygen::tagFile << "\" static=\"yes";
    }
    Doxygen::tagFile << "\">" << endl;
    Doxygen::tagFile << "      <type>" << convertToXML(typeString()) << "</type>" << endl;
    Doxygen::tagFile << "      <name>" << convertToXML(name()) << "</name>" << endl;
    Doxygen::tagFile << "      <anchor>" << convertToXML(anchor()) << "</anchor>" << endl;
    Doxygen::tagFile << "      <arglist>" << convertToXML(argsString()) << "</arglist>" << endl;
    writeDocAnchorsToTagFile();
    Doxygen::tagFile << "    </member>" << endl;
  }

  Definition *d=0;
  ASSERT (cd!=0 || nd!=0 || fd!=0 || gd!=0); // member should belong to something
  if (cd) d=cd; else if (nd) d=nd; else if (fd) d=fd; else d=gd;
  QCString cname  = d->name();
  QCString cfname = d->getOutputFileBase();

  HtmlHelp *htmlHelp=0;
  bool hasHtmlHelp = Config_getBool("GENERATE_HTML") && Config_getBool("GENERATE_HTMLHELP");
  if (hasHtmlHelp) htmlHelp = HtmlHelp::getInstance();

  // search for the last anonymous scope in the member type
  ClassDef *annoClassDef=getClassDefOfAnonymousType();

  // start a new member declaration
  ol.startMemberItem((annoClassDef || annMemb || annEnumType) ? 1 : 0);

  // If there is no detailed description we need to write the anchor here.
  bool detailsVisible = isDetailedSectionLinkable();
  if (!detailsVisible && !annMemb)
  {
    QCString doxyName=name().copy();
    if (!cname.isEmpty()) doxyName.prepend(cname+"::");
    ol.startDoxyAnchor(cfname,cname,anchor(),doxyName);

    ol.addIndexItem(name(),cname);
    ol.addIndexItem(cname,name());

    if (hasHtmlHelp)
    {
      htmlHelp->addIndexItem(cname,name(),cfname,anchor());
    }
    ol.pushGeneratorState();
    ol.disable(OutputGenerator::Man);
    ol.docify("\n");
    ol.popGeneratorState();
  }

  //printf("member name=%s indDepth=%d\n",name().data(),indDepth);
  if (annoClassDef || annMemb)
  {
    int j;
    for (j=0;j<indDepth;j++) 
    {
      ol.writeNonBreakableSpace(3);
    }
  }

  if (tArgList)
  {
    writeTemplatePrefix(ol,tArgList);
    //ol.lineBreak();
  }

  QCString ltype(type);
  if (mtype==Typedef) ltype.prepend("typedef ");
  // strip `static' keyword from ltype
  if (ltype.left(7)=="static ") ltype=ltype.right(ltype.length()-7);
  // strip `friend' keyword from ltype
  if (ltype.left(7)=="friend ") ltype=ltype.right(ltype.length()-7);
  static QRegExp r("@[0-9]+");

  int l,i=r.match(ltype,0,&l);
  if (i!=-1) // member has an anonymous type
  {
    //printf("annoClassDef=%p annMemb=%p scopeName=`%s' anonymous=`%s'\n",
    //    annoClassDef,annMemb,cname.data(),ltype.mid(i,l).data());

    if (annoClassDef) // type is an anonymous compound
    {
      int ir=i+l;
      annoClassDef->writeDeclaration(ol,annMemb,inGroup);
      ol.startMemberItem(2);
      int j;
      for (j=0;j<indDepth;j++) 
      {
        ol.writeNonBreakableSpace(3);
      }
      QCString varName=ltype.right(ltype.length()-ir).stripWhiteSpace();
      //printf(">>>>>> indDepth=%d ltype=`%s' varName=`%s'\n",indDepth,ltype.data(),varName.data());
      ol.docify("}");
      if (varName.isEmpty() && (name().isEmpty() || name().at(0)=='@')) 
      {
        ol.docify(";"); 
      }
    }
    else
    {
      if (getAnonymousEnumType()) // type is an anonymous enum
      {
        linkifyText(TextGeneratorOLImpl(ol),cname,name(),ltype.left(i),TRUE); 
        ol+=*getAnonymousEnumType()->enumDecl();
        linkifyText(TextGeneratorOLImpl(ol),cname,name(),ltype.right(ltype.length()-i-l),TRUE); 
      }
      else
      {
        ltype = ltype.left(i) + " { ... } " + ltype.right(ltype.length()-i-l);
        linkifyText(TextGeneratorOLImpl(ol),cname,name(),ltype,TRUE); 
      }
    }
  }
  else
  {
    linkifyText(TextGeneratorOLImpl(ol),cname,name(),ltype,TRUE); 
  }
  bool htmlOn = ol.isEnabled(OutputGenerator::Html);
  if (htmlOn && Config_getBool("HTML_ALIGN_MEMBERS") && !ltype.isEmpty())
  {
    ol.disable(OutputGenerator::Html);
  }
  if (!ltype.isEmpty()) ol.docify(" ");
  if (htmlOn) 
  {
    ol.enable(OutputGenerator::Html);
  }

  if (annMemb) 
  {
    ol.pushGeneratorState();
    ol.disableAllBut(OutputGenerator::Html);
    ol.writeNonBreakableSpace(3);
    ol.popGeneratorState();
  }
  else
  {
    ol.insertMemberAlign();
  }

  // write name
  if (!name().isEmpty() && name().at(0)!='@') // hide annonymous stuff 
  {
    //printf("Member name=`%s gd=%p md->groupDef=%p inGroup=%d isLinkable()=%d\n",name().data(),gd,getGroupDef(),inGroup,isLinkable());
    if (isLinkable())
    {
      if (annMemb)
      {
        //printf("anchor=%s ann_anchor=%s\n",anchor(),annMemb->anchor());
        annMemb->writeLink(ol,
            annMemb->getClassDef(),
            annMemb->getNamespaceDef(),
            annMemb->getFileDef(),
            annMemb->getGroupDef()
                          );
        annMemb->annUsed=annUsed=TRUE;
      }
      else
      {
        //printf("writeLink %s->%d\n",name.data(),hasDocumentation());
        ClassDef *rcd = cd;
        if (isReference() && classDef) rcd = classDef; 
        writeLink(ol,rcd,nd,fd,gd);
      }
    }
    else if (isDocumentedFriendClass())
      // if the member is an undocumented friend declaration for some class, 
      // then maybe we can link to the class
    {
      writeLink(ol,getClass(name()),0,0,0);
    }
    else
      // there is a brief member description and brief member 
      // descriptions are enabled or there is no detailed description.
    {
      if (annMemb) annMemb->annUsed=annUsed=TRUE;
      ol.startBold();
      ol.docify(name());
      ol.endBold();
    }
  }

  if (argsString()) 
  {
    if (!isDefine()) ol.writeString(" ");
    //ol.docify(argsString());
    linkifyText(TextGeneratorOLImpl(ol),cname,name(),argsString()); 
  }

  if (excpString())
  {
    ol.writeString(" ");
    ol.docify(excpString());
  }

  if (!bitfields.isEmpty()) // add bitfields
  {
    linkifyText(TextGeneratorOLImpl(ol),cname,name(),bitfields.simplifyWhiteSpace());
  }
  else if (hasOneLineInitializer()
      //!init.isEmpty() && initLines==0 && // one line initializer
      //((maxInitLines>0 && userInitLines==-1) || userInitLines>0) // enabled by default or explicitly
          ) // add initializer
  {
    if (!isDefine()) 
    {
      ol.writeString(" = "); 
      linkifyText(TextGeneratorOLImpl(ol),cname,name(),init.simplifyWhiteSpace());
    }
    else 
    {
      ol.writeNonBreakableSpace(3);
      linkifyText(TextGeneratorOLImpl(ol),cname,name(),init);
    }
  }

  if (!detailsVisible && !annMemb)
  {
    ol.endDoxyAnchor(cfname,anchor());
  }

  ol.endMemberItem((annoClassDef!=0 && indDepth==0) || annEnumType);

  //ol.endMemberItem(gId!=-1,gFile,gHeader,annoClassDef || annMemb);
  // write brief description
  if (!briefDescription().isEmpty() && 
      Config_getBool("BRIEF_MEMBER_DESC") && 
      !annMemb)
  {
    ol.startMemberDescription();
    parseDoc(ol,briefFile(),briefLine(),cname,this,briefDescription());
    if (detailsVisible) 
    {
      ol.pushGeneratorState();
      ol.disableAllBut(OutputGenerator::Html);
      ol.endEmphasis();
      ol.docify(" ");
      if (group!=0 && gd==0) // forward link to the group
      {
        ol.startTextLink(group->getOutputFileBase(),anchor());
      }
      else
      {
        ol.startTextLink(0,anchor());
      }
      ol.endTextLink();
      ol.startEmphasis();
      ol.popGeneratorState();
    }
    //ol.newParagraph();
    ol.endMemberDescription();
  }
  warnIfUndocumented();
}

bool MemberDef::isDetailedSectionLinkable() const          
{ 
  // the member has details documentation for any of the following reasons
  bool docFilter = 
         // treat everything as documented
         Config_getBool("EXTRACT_ALL") ||          
         // has detailed docs
         !documentation().isEmpty() ||             
         // is an enum with values that are documented
         (mtype==Enumeration && docEnumValues) ||  
         // is documented enum value
         (mtype==EnumValue && !briefDescription().isEmpty()) || 
         // has brief description that is part of the detailed description
         (!briefDescription().isEmpty() &&           // has brief docs
          (Config_getBool("ALWAYS_DETAILED_SEC") &&  // they or visible in
           Config_getBool("REPEAT_BRIEF") ||         // detailed section or
           !Config_getBool("BRIEF_MEMBER_DESC")      // they are explicitly not
          )                                          // shown in brief section
         ) ||
         // has a multi-line initialization block
         //(initLines>0 && initLines<maxInitLines) || 
         hasMultiLineInitializer() ||
         // has one or more documented arguments
         (defArgList!=0 && defArgList->hasDocumentation()); 
         
  // this is not a global static or global statics should be extracted
  bool staticFilter = getClassDef()!=0 || !isStatic() || Config_getBool("EXTRACT_STATIC"); 
         
  // only include members that are non-private unless EXTRACT_PRIVATE is
  // set to YES or the member is part of a group
  bool privateFilter = (protection()!=Private || 
                           Config_getBool("EXTRACT_PRIVATE") ||
                           mtype==Friend
                          );

  // member is part of an anonymous scope that is the type of
  // another member in the list.
  //
  bool inAnonymousScope = !briefDescription().isEmpty() && annUsed;

  return ((docFilter && staticFilter && privateFilter) || inAnonymousScope);
}

bool MemberDef::isDetailedSectionVisible(bool inGroup) const          
{ 
  bool groupFilter = getGroupDef()==0 || inGroup; 

  bool visible = isDetailedSectionLinkable() && groupFilter && !isReference();
  //printf("MemberDef::isDetailedSectionVisible() %d\n",visible);
  return visible;
}

/*! Writes the "detailed documentation" section of this member to
 *  all active output formats.
 */
void MemberDef::writeDocumentation(MemberList *ml,OutputList &ol,
                                   const char *scName,
                                   Definition *container,
                                   bool inGroup
                                  )
{
  // if this member is in a group find the real scope name.
  bool hasDocs = isDetailedSectionVisible(inGroup);
  //printf("MemberDef::writeDocumentation(): name=`%s' hasDocs=`%d' containerType=%d inGroup=%d\n",
  //    name().data(),hasDocs,container->definitionType(),inGroup);
  if ( hasDocs )
  {
    QCString scopeName = scName;
    if (container->definitionType()==TypeGroup)
    {
      if (getClassDef())          scopeName=getClassDef()->name();
      else if (getNamespaceDef()) scopeName=getNamespaceDef()->name();
      else if (getFileDef())      scopeName=getFileDef()->name();
    }
  
    // get definition. 
    QCString cname  = container->name();
    QCString cfname = container->getOutputFileBase();  

    // get member name
    QCString doxyName=name().copy();
    // prepend scope if there is any. TODO: make this optional for C only docs
    if (scopeName) doxyName.prepend((QCString)scopeName+"::");

    QCString ldef = definition();
    //printf("member `%s' def=`%s'\n",name().data(),ldef.data());
    if (isEnumerate()) 
    {
      if (name().at(0)=='@')
      {
        ldef = "anonymous enum";
      }
      else
      {
        ldef.prepend("enum ");
      }
    }
    int i=0,l;
    static QRegExp r("@[0-9]+");

    if (isEnumValue()) return;

    ol.pushGeneratorState();

    bool hasHtmlHelp = Config_getBool("GENERATE_HTML") && Config_getBool("GENERATE_HTMLHELP");
    HtmlHelp *htmlHelp = 0;
    if (hasHtmlHelp) htmlHelp = HtmlHelp::getInstance();

    if ((isVariable() || isTypedef()) && (i=r.match(ldef,0,&l))!=-1)
    {
      // find enum type and insert it in the definition
      MemberListIterator vmli(*ml);
      MemberDef *vmd;
      bool found=FALSE;
      for ( ; (vmd=vmli.current()) && !found ; ++vmli)
      {
        if (vmd->isEnumerate() && ldef.mid(i,l)==vmd->name())
        {
          ol.startDoxyAnchor(cfname,cname,anchor(),doxyName);
          ol.startMemberDoc(cname,name(),anchor(),name());
          if (hasHtmlHelp)
          {
            htmlHelp->addIndexItem(cname,name(),cfname,anchor());
          }
          linkifyText(TextGeneratorOLImpl(ol),scopeName,name(),ldef.left(i));
          ol+=*vmd->enumDecl();
          linkifyText(TextGeneratorOLImpl(ol),scopeName,name(),ldef.right(ldef.length()-i-l));

          found=TRUE;
        }
      }
      if (!found) // anonymous compound
      {
        //printf("Anonymous compound `%s'\n",cname.data());
        ol.startDoxyAnchor(cfname,cname,anchor(),doxyName);
        ol.startMemberDoc(cname,name(),anchor(),name());
        if (hasHtmlHelp)
        {
          htmlHelp->addIndexItem(cname,name(),cfname,anchor());
        }
        // strip anonymous compound names from definition
        int si=ldef.find(' '),pi,ei=i+l;
        if (si==-1) si=0;
        while ((pi=r.match(ldef,i+l,&l))!=-1) ei=i=pi+l;
        // first si characters of ldef contain compound type name
        ol.startMemberDocName();
        ol.docify(ldef.left(si));
        ol.docify(" { ... } ");
        // last ei characters of ldef contain pointer/reference specifiers
        int ni=ldef.find("::",si);
        if (ni>=ei) ei=ni+2;
        linkifyText(TextGeneratorOLImpl(ol),scopeName,name(),ldef.right(ldef.length()-ei));
      }
    }
    else // not an enum value
    {
      ol.startDoxyAnchor(cfname,cname,anchor(),doxyName);
      ol.startMemberDoc(cname,name(),anchor(),name());
      if (hasHtmlHelp)
      {
        htmlHelp->addIndexItem(cname,name(),cfname,anchor());
      }

      ClassDef *cd=getClassDef();
      if (!Config_getBool("HIDE_SCOPE_NAMES"))
      {
        bool first=TRUE;
        if (m_defTmpArgLists) 
          // definition has explicit template parameter declarations
        {
          QListIterator<ArgumentList> ali(*m_defTmpArgLists);
          ArgumentList *tal;
          for (ali.toFirst();(tal=ali.current());++ali)
          {
            if (tal->count()>0)
            {
              if (!first) ol.docify(" ");
              ol.startMemberDocPrefixItem();
              writeTemplatePrefix(ol,tal);
              ol.endMemberDocPrefixItem();
            }
          }
        }
        else // definition gets it template parameters from its class
             // (since no definition was found)
        {
          if (cd)
          {
            QList<ArgumentList> tempParamLists;
            cd->getTemplateParameterLists(tempParamLists);
            //printf("#tempParamLists=%d\n",tempParamLists.count());
            QListIterator<ArgumentList> ali(tempParamLists);
            ArgumentList *tal;
            for (ali.toFirst();(tal=ali.current());++ali)
            {
              if (tal->count()>0)
              {
                if (!first) ol.docify(" ");
                ol.startMemberDocPrefixItem();
                writeTemplatePrefix(ol,tal);
                ol.endMemberDocPrefixItem();
              }
            }
          }
          if (tArgList) // function template prefix
          {
            ol.startMemberDocPrefixItem();
            writeTemplatePrefix(ol,tArgList);
            ol.endMemberDocPrefixItem();
          }
        }
      }
      ol.startMemberDocName();
      linkifyText(TextGeneratorOLImpl(ol),scopeName,name(),ldef);
      writeDefArgumentList(ol,cd,scopeName,this);
      if (hasOneLineInitializer()) // add initializer
      {
        if (!isDefine()) 
        {
          ol.docify(" = "); 
          linkifyText(TextGeneratorOLImpl(ol),scopeName,name(),init.simplifyWhiteSpace());
        }
        else 
        {
          ol.writeNonBreakableSpace(3);
          linkifyText(TextGeneratorOLImpl(ol),scopeName,name(),init);
        }
      }
      if (excpString()) // add exception list
      {
        ol.docify(" ");
        linkifyText(TextGeneratorOLImpl(ol),scopeName,name(),excpString());
      }
    }

    Specifier lvirt=virtualness();

    if (protection()!=Public || lvirt!=Normal ||
        isFriend() || isRelated() || isExplicit() ||
        isMutable() || (isInline() && Config_getBool("INLINE_INFO")) ||
        isSignal() || isSlot() ||
        isStatic() || (classDef && classDef!=container) 
       )
    {
      // write the member specifier list
      ol.writeLatexSpacing();
      ol.startTypewriter();
      ol.docify(" [");
      QStrList sl;
      if (isFriend()) sl.append("friend");
      else if (isRelated()) sl.append("related");
      else
      {
        if      (Config_getBool("INLINE_INFO") && isInline())              
                                          sl.append("inline");
        if      (isExplicit())            sl.append("explicit");
        if      (isMutable())             sl.append("mutable");
        if      (isStatic())              sl.append("static");
        if      (protection()==Protected) sl.append("protected");
        else if (protection()==Private)   sl.append("private");
        if      (lvirt==Virtual)          sl.append("virtual");
        else if (lvirt==Pure)             sl.append("pure virtual");
        if      (isSignal())              sl.append("signal");
        if      (isSlot())                sl.append("slot");
      }
      if (classDef && classDef!=container) sl.append("inherited");
      const char *s=sl.first();
      while (s)
      {
        ol.docify(s);
        s=sl.next();
        if (s) ol.docify(", ");
      }
      ol.docify("]");
      ol.endTypewriter();
    }
    if (!isDefine() && defArgList) ol.endParameterList();
    ol.endMemberDoc();
    ol.endDoxyAnchor(cfname,anchor());
    ol.startIndent();
    
    ol.pushGeneratorState();
    ol.disable(OutputGenerator::RTF);
    ol.newParagraph();
    ol.popGeneratorState();

    /* write multi-line initializer (if any) */
    if (hasMultiLineInitializer()
        //initLines>0 && ((initLines<maxInitLines && userInitLines==-1) // implicitly enabled
        //                || initLines<userInitLines // explicitly enabled
        //               )
       )
    {
      //printf("md=%s initLines=%d init=`%s'\n",name().data(),initLines,init.data());
      ol.startBold();
      if (mtype==Define)
        parseText(ol,theTranslator->trDefineValue());
      else
        parseText(ol,theTranslator->trInitialValue());
      ol.endBold();
      initParseCodeContext();
      ol.startCodeFragment();
      parseCode(ol,scopeName,init,FALSE,0);
      ol.endCodeFragment();
    }

    QCString brief    = m_templateMaster ? 
                        m_templateMaster->briefDescription() : briefDescription();
    QCString detailed = m_templateMaster ?
                        m_templateMaster->documentation() : documentation();
    ArgumentList *docArgList = m_templateMaster ? 
                        m_templateMaster->defArgList : defArgList;
    
    /* write brief description */
    if (!brief.isEmpty() && 
        (Config_getBool("REPEAT_BRIEF") || 
         !Config_getBool("BRIEF_MEMBER_DESC")
        ) 
       )  
    { 
      parseDoc(ol,briefFile(),briefLine(),scopeName,this,brief);
      ol.newParagraph();
    }

    /* write detailed description */
    if (!detailed.isEmpty())
    { 
      parseDoc(ol,docFile(),docLine(),scopeName,this,detailed+"\n");
      ol.pushGeneratorState();
      ol.disableAllBut(OutputGenerator::RTF);
      ol.newParagraph();
      ol.popGeneratorState();
    }

    //printf("***** defArgList=%p name=%s docs=%s hasDocs=%d\n",
    //     defArgList, 
    //     defArgList?defArgList->hasDocumentation():-1);
    if (docArgList && docArgList->hasDocumentation())
    {
      //printf("***** argumentList is documented\n");
      ol.startParamList(BaseOutputDocInterface::Param,theTranslator->trParameters()+": ");
      ol.writeDescItem();
      ol.startDescTable();
      ArgumentListIterator ali(*docArgList);
      Argument *a;
      for (ali.toFirst();(a=ali.current());++ali)
      {
        if (a->hasDocumentation())
        {
          ol.startDescTableTitle();
          ol.docify(a->name);
          ol.endDescTableTitle();
          ol.startDescTableData();
          parseDoc(ol,docFile(),docLine(),scopeName,this,a->docs+"\n");
          ol.endDescTableData();
        }
      }
      ol.endDescTable();
      ol.endParamList();
    }
    
    if (isEnumerate())
    {
      bool first=TRUE;
      MemberList *fmdl=enumFieldList();
      if (fmdl)
      {
        MemberDef *fmd=fmdl->first();
        while (fmd)
        {
          if (fmd->isLinkable())
          {
            if (first)
            {
              //ol.newParagraph();
              ol.startSimpleSect(BaseOutputDocInterface::EnumValues,0,0,theTranslator->trEnumerationValues()+": ");
              ol.writeDescItem();
              ol.startDescTable();
            }

            ol.addIndexItem(fmd->name(),cname);
            ol.addIndexItem(cname,fmd->name());

            if (Config_getBool("GENERATE_HTML") && Config_getBool("GENERATE_HTMLHELP"))
            {
              HtmlHelp::getInstance()->addIndexItem(cname,fmd->name(),cfname,fmd->anchor());
            }
            //ol.writeListItem();
            ol.startDescTableTitle();
            ol.startDoxyAnchor(cfname,cname,fmd->anchor(),fmd->name());
            first=FALSE;
            ol.startEmphasis();
            ol.docify(fmd->name());
            ol.endEmphasis();
            ol.disableAllBut(OutputGenerator::Man);
            ol.writeString(" ");
            ol.enableAll();
            ol.endDoxyAnchor(cfname,fmd->anchor());
            ol.endDescTableTitle();
            //ol.newParagraph();
            ol.startDescTableData();

            if (!fmd->briefDescription().isEmpty())
            { 
              parseDoc(ol,fmd->briefFile(),fmd->briefLine(),scopeName,fmd,fmd->briefDescription());
              //ol.newParagraph();
            }
            if (!fmd->briefDescription().isEmpty() && 
                !fmd->documentation().isEmpty())
            {
              ol.newParagraph();
            }
            if (!fmd->documentation().isEmpty())
            { 
              parseDoc(ol,fmd->docFile(),fmd->docLine(),scopeName,fmd,fmd->documentation()+"\n");
            }
            ol.endDescTableData();
          }
          fmd=fmdl->next();
        }
      }
      if (!first) 
      { 
        //ol.endItemList(); 
        ol.endDescTable();
        ol.endSimpleSect();
        ol.writeChar('\n'); 
      }
    }

    MemberDef *bmd=reimplements();
    ClassDef *bcd=0;
    if (bmd && (bcd=bmd->getClassDef()))
    {
      // write class that contains a member that is reimplemented by this one
      if (bcd->isLinkable())
      {
        ol.newParagraph();

        QCString reimplFromLine; 
        if (bmd->virtualness()!=Pure && bcd->compoundType()!=ClassDef::Interface)
        {
          reimplFromLine = theTranslator->trReimplementedFromList(1);
        }
        else
        {
          reimplFromLine = theTranslator->trImplementedFromList(1);
        }
        int markerPos = reimplFromLine.find("@0");
        if (markerPos!=-1) // should always pass this.
        {
          parseText(ol,reimplFromLine.left(markerPos)); //text left from marker
          if (bmd->isLinkable()) // replace marker with link
          {
            Definition *bd=bmd->group;
            if (bd==0) bd=bcd;
            ol.writeObjectLink(bd->getReference(),bd->getOutputFileBase(),
                bmd->anchor(),bcd->displayName());

            //ol.writeObjectLink(bcd->getReference(),bcd->getOutputFileBase(),
            //    bmd->anchor(),bcd->name());
            if ( bd->isLinkableInProject() ) 
            {
              writePageRef(ol,bd->getOutputFileBase(),bmd->anchor());
            }
          }
          else
          {
            ol.writeObjectLink(bcd->getReference(),bcd->getOutputFileBase(),
                0,bcd->displayName());
            if (bcd->isLinkableInProject()/* && !Config_getBool("PDF_HYPERLINKS")*/ )
            {
              writePageRef(ol,bcd->getOutputFileBase(),0);
            }
          }
          parseText(ol,reimplFromLine.right(
                reimplFromLine.length()-markerPos-2)); // text right from marker

          ol.disableAllBut(OutputGenerator::RTF);
          ol.newParagraph();
          ol.enableAll();
        }
        else
        {
          err("Error: translation error: no marker in trReimplementsFromList()\n");
        }
      }

      //ol.writeString(".");
    }

    MemberList *bml=reimplementedBy();
    if (bml)
    {
      MemberListIterator mli(*bml);
      MemberDef *bmd=0;
      uint count=0;
      ClassDef *bcd=0;
      for (mli.toFirst();(bmd=mli.current()) && (bcd=bmd->getClassDef());++mli)
      {
        // count the members that directly inherit from md and for
        // which the member and class are visible in the docs.
        if ( bmd->isLinkable() && bcd->isLinkable() ) 
        {
          count++;
        }
      }
      if (count>0)
      {
        mli.toFirst();
        // write the list of classes that overwrite this member
        ol.newParagraph();

        QCString reimplInLine;
        if (virt==Pure || (classDef && classDef->compoundType()==ClassDef::Interface))
        {
          reimplInLine = theTranslator->trImplementedInList(count);
        }
        else
        {
          reimplInLine = theTranslator->trReimplementedInList(count);
        }
        static QRegExp marker("@[0-9]+");
        int index=0,newIndex,matchLen;
        // now replace all markers in reimplInLine with links to the classes
        while ((newIndex=marker.match(reimplInLine,index,&matchLen))!=-1)
        {
          parseText(ol,reimplInLine.mid(index,newIndex-index));
          bool ok;
          uint entryIndex = reimplInLine.mid(newIndex+1,matchLen-1).toUInt(&ok);
          //bmd=bml->at(entryIndex);

          count=0;
          // find the entryIndex-th documented entry in the inheritance list.
          for (mli.toLast();(bmd=mli.current()) && (bcd=bmd->getClassDef());--mli)
          {
            if ( bmd->isLinkable() && bcd->isLinkable()) 
            {
              if (count==entryIndex) break;
              count++;
            }
          }

          if (ok && bcd && bmd) // write link for marker
          {
            //ol.writeObjectLink(bcd->getReference(),bcd->getOutputFileBase(),
            //    bmd->anchor(),bcd->name());
            Definition* bd;
            if (bmd->group) bd=bmd->group; else bd=bcd;
            
            ol.writeObjectLink(bd->getReference(),bd->getOutputFileBase(),
                bmd->anchor(),bcd->displayName());
                
            if (bd->isLinkableInProject() ) 
            {
              writePageRef(ol,bd->getOutputFileBase(),bmd->anchor());
            }
          }
          ++mli;
          index=newIndex+matchLen;
        } 
        parseText(ol,reimplInLine.right(reimplInLine.length()-index));
        ol.disableAllBut(OutputGenerator::RTF);
        ol.newParagraph();
        ol.enableAll();
      }
    }
    // write the list of examples that use this member
    if (hasExamples())
    {
      ol.startSimpleSect(BaseOutputDocInterface::Examples,0,0,theTranslator->trExamples()+": ");
      ol.writeDescItem();
      writeExample(ol,getExamples());
      //ol.endDescItem();
      ol.endSimpleSect();
    }
    // write reference to the source
    writeSourceDef(ol,cname);
    writeSourceRefs(ol,cname);
    writeSourceReffedBy(ol,cname);
    writeInlineCode(ol,cname);

    ol.disableAllBut(OutputGenerator::RTF);
    ol.newParagraph();
    ol.enableAll();

    ol.endIndent();
    // enable LaTeX again
    //if (Config_getBool("EXTRACT_ALL") && !hasDocs) ol.enable(OutputGenerator::Latex); 
    ol.popGeneratorState();

  }
}

void MemberDef::warnIfUndocumented()
{
  if (memberGroup) return;
  ClassDef     *cd = getClassDef();
  NamespaceDef *nd = getNamespaceDef();
  FileDef      *fd = getFileDef();
  GroupDef     *gd = getGroupDef();
  Definition *d=0;
  const char *t=0;
  if (cd) 
    t="class", d=cd; 
  else if (nd) 
    t="namespace", d=nd; 
  else if (gd)
    t="group", d=gd;
  else
    t="file", d=fd;

  if (d && d->isLinkable() && !isLinkable() && 
      !isDocumentedFriendClass() && 
      name().find('@')==-1 &&
      (prot!=Private || Config_getBool("EXTRACT_PRIVATE"))
     )
  {
   warn_undoc(m_defFileName,m_defLine,"Warning: Member %s of %s %s is not documented.",
        name().data(),t,d->name().data());
  }
}


bool MemberDef::isLinkableInProject() const
{
  if (m_templateMaster)
  {
    return m_templateMaster->isLinkableInProject();
  }
  else
  {
    return !name().isEmpty() && name().at(0)!='@' &&
      (hasDocumentation() && !isReference()) && 
      (prot!=Private || Config_getBool("EXTRACT_PRIVATE") || 
      mtype==Friend) && // not a hidden member due to protection
      (classDef!=0 || Config_getBool("EXTRACT_STATIC") || 
      !isStatic()); // not a static file/namespace member
  }
}

bool MemberDef::isLinkable() const
{
  if (m_templateMaster)
  {
    return m_templateMaster->isLinkable();
  }
  else
  {
    return isLinkableInProject() || isReference();
  }
}

void MemberDef::setEnumDecl(OutputList &ed) 
{ 
  enumDeclList=new OutputList(&ed); 
  *enumDeclList+=ed;
}

bool MemberDef::isDocumentedFriendClass() const
{
  ClassDef *fcd=0;
  return (isFriend() && 
         (type=="friend class" || type=="friend struct" || 
          type=="friend union") &&
         (fcd=getClass(name())) && fcd->isLinkable()); 
}

bool MemberDef::hasDocumentation() const
{ 
  return Definition::hasDocumentation() || 
         (mtype==Enumeration && docEnumValues) ||  // has enum values
         (defArgList!=0 && defArgList->hasDocumentation()); // has doc arguments
}

void MemberDef::setMemberGroup(MemberGroup *grp)
{
  memberGroup = grp;
}

bool MemberDef::visibleMemberGroup(bool hideNoHeader) 
{ 
  return memberGroup!=0 && 
          (!hideNoHeader || memberGroup->header()!="[NOHEADER]"); 
}

QCString MemberDef::getScopeString() const
{
  QCString result;
  if (getClassDef()) result=getClassDef()->displayName();
  else if (getNamespaceDef()) result=getNamespaceDef()->displayName();
  return result;
}

void MemberDef::setAnchor(const char *a)
{
  anc=a;
}

QCString MemberDef::anchor() const
{
  if (m_templateMaster) return m_templateMaster->anchor();
  if (enumScope) return enumScope->anchor()+anc;
  return anc;
}

void MemberDef::setGroupDef(GroupDef *gd,Grouping::GroupPri_t pri,const QCString &fileName,int startLine,bool hasDocs)
{
  //printf("%s MemberDef::setGroupDef(%s)\n",name().data(),gd->name().data());
  group=gd;
  grouppri=pri;
  groupFileName=fileName;
  groupStartLine=startLine;
  groupHasDocs=hasDocs;
}

void MemberDef::setEnumScope(MemberDef *md) 
{ 
  enumScope=md; 
  if (md->group)
  {
    group=md->group;
    grouppri=md->grouppri;
    groupFileName=md->groupFileName;
    groupStartLine=md->groupStartLine;
    groupHasDocs=md->groupHasDocs;
  }
}

void MemberDef::setMemberClass(ClassDef *cd)     
{ 
  classDef=cd; 
  setOuterScope(cd); 
}

void MemberDef::setNamespace(NamespaceDef *nd) 
{ 
  nspace=nd; 
  setOuterScope(nd); 
}

MemberDef *MemberDef::createTemplateInstanceMember(
        ArgumentList *formalArgs,ArgumentList *actualArgs)
{
  //printf("  Member %s %s %s\n",typeString(),name().data(),argsString());
  ArgumentList *actualArgList = 0;
  if (defArgList)
  {
    actualArgList = new ArgumentList;
    ArgumentListIterator ali(*defArgList);
    Argument *arg;
    for (;(arg=ali.current());++ali)
    {
      Argument *actArg = new Argument(*arg);
      actArg->type = substituteTemplateArgumentsInString(actArg->type,formalArgs,actualArgs);
      actualArgList->append(actArg);
    }
    actualArgList->constSpecifier    = defArgList->constSpecifier;
    actualArgList->volatileSpecifier = defArgList->volatileSpecifier;
    actualArgList->pureSpecifier     = defArgList->pureSpecifier;
  }

  MemberDef *imd = new MemberDef(
                       getDefFileName(),getDefLine(),
                       substituteTemplateArgumentsInString(type,formalArgs,actualArgs), 
                       name(), 
                       substituteTemplateArgumentsInString(args,formalArgs,actualArgs), 
                       exception, prot,
                       virt, stat, related, mtype, 0, 0
                   );
  imd->defArgList = actualArgList;
  imd->def = substituteTemplateArgumentsInString(def,formalArgs,actualArgs);
  imd->setBodyDef(getBodyDef());
  imd->setBodySegment(getStartBodyLine(),getEndBodyLine());

  // TODO: init other member variables (if needed).
  // TODO: reimplemented info
  return imd; 
}

bool MemberDef::hasOneLineInitializer() const
{
  //printf("%s: init=%s, initLines=%d maxInitLines=%d userInitLines=%d\n",
  //    name().data(),init.data(),initLines,maxInitLines,userInitLines);
  return !init.isEmpty() && initLines==0 && // one line initializer
         ((maxInitLines>0 && userInitLines==-1) || userInitLines>0); // enabled by default or explicitly
}

bool MemberDef::hasMultiLineInitializer() const
{
  return initLines>0 && 
         ((initLines<maxInitLines && userInitLines==-1) // implicitly enabled
          || initLines<userInitLines // explicitly enabled
         );
}

void MemberDef::setInitializer(const char *initializer)    
{ 
  init=initializer; 
  int p=init.length()-1;
  while (p>=0 && isspace(init.at(p))) p--;
  init=init.left(p+1);
  initLines=init.contains('\n');
}

void MemberDef::addListReference(Definition *d)
{
  visited=TRUE;
  QCString memLabel;
  if (Config_getBool("OPTIMIZE_OUTPUT_FOR_C")) 
  {
    memLabel=theTranslator->trGlobal(TRUE,TRUE);
  }
  else
  {
    memLabel=theTranslator->trMember(TRUE,TRUE);
  }
  QCString memName = name();
  if (!Config_getBool("HIDE_SCOPE_NAMES"))
  {
    Definition *pd=getOuterScope();
    if (pd && pd!=Doxygen::globalScope)
    {
      memName.prepend(pd->name()+"::");
    }
  }
  //printf("*** addListReference %s todo=%d test=%d bug=%d\n",name().data(),todoId(),testId(),bugId());
  addRefItem(specialListItems(),memLabel,
      d->getOutputFileBase()+"#"+anchor(),memName,argsString());
}

MemberList *MemberDef::getSectionList(Definition *d) const 
{ 
  return (d!=0 && classSectionSDict) ? classSectionSDict->find((char *)d) : 0;
}

void MemberDef::setSectionList(Definition *d, MemberList *sl)   
{ 
  if (classSectionSDict==0) classSectionSDict = new SDict<MemberList>(7);
  classSectionSDict->append((char *)d,sl);
}

Specifier MemberDef::virtualness() const
{
  Specifier v = virt;
  MemberDef *rmd = reimplements();
  while (rmd && v==Normal)
  {
    v = rmd->virtualness()==Normal ? Normal : Virtual;
    rmd = rmd->reimplements();
  }
  return v;
}

bool MemberDef::isConstructor() const            
{ 
  return classDef ? name()==classDef->localName() : FALSE; 
}

bool MemberDef::isDestructor() const
{ 
  return name().find('~')!=-1 && name().find("operator")==-1; 
}

