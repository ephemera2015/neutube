#include "utils.h"

void swc2Points(ZStackDoc* doc,std::vector<ZIntPoint>&points,std::string comment,bool translate)
{
  int width=doc->getStack()->width(),height=doc->getStack()->height();
  QList<ZSwcTree*> trees=doc->getSwcList();
  int ofx=doc->getStack()->getOffset().m_x,ofy=doc->getStack()->getOffset().m_y,ofz=doc->getStack()->getOffset().m_z;

  bool bnot=false;
  if(comment[0]=='!')
  {
    bnot=true;
    comment=comment.substr(1);
  }
  std::for_each(trees.begin(),trees.end(),[&](ZSwcTree* tree)
  {
    std::string x=tree->toString().substr(1);
    if((comment=="" && ((bnot&&x.find("#")!=std::string::npos)||(!bnot&&x.find("#")==std::string::npos)))
       ||(comment!=""&&((!bnot&&x.find(comment)!=std::string::npos) ||
                        (bnot &&x.find(comment)==std::string::npos))))
    {
      tree->forceVirtualRoot();
      ZSwcTree::DepthFirstIterator it(tree);
      it.next();//skip virtual root
      while(it.hasNext())
      {
        Swc_Tree_Node* n=it.next();
        double qx=n->node.x,qy=n->node.y,qz=n->node.z;
        if(translate)
        {
          qx-=ofx,qy-=ofy,qz-=ofz;
          qx=std::min(qx,width-1.0),qy=std::min(qy,height-1.0);
        }
        else
        {
          qx=std::min(qx,ofx+width-1.0),qy=std::min(qy,ofy+height-1.0);
        }
        qx=std::max(0.0,qx),qy=std::max(0.0,qy);
        points.push_back(ZIntPoint(qx,qy,qz));
      }
    }
  });
}

void rmSwcTree(ZStackDoc* doc,std::string comment)
{
  QList<ZSwcTree*> trees=doc->getSwcList();
  bool bnot=false;
  if(comment[0]=='!')
  {
    bnot=true;
    comment=comment.substr(1);
  }
  for(QList<ZSwcTree*>::iterator it=trees.begin();it!=trees.end();++it)
  {
    ZSwcTree* tree=*it;
    std::string x=tree->toString().substr(1);
    if((comment=="" && ((bnot&&x.find("#")!=std::string::npos)||(!bnot&&x.find("#")==std::string::npos)))
       ||(comment!=""&&((!bnot&&x.find(comment)!=std::string::npos) ||
                        (bnot &&x.find(comment)==std::string::npos))))
    {
      doc->removeObject(*it);
    }
  }
}

void points2Swc(const std::vector<ZIntPoint>& points,ZStackDoc* doc,std::string comment,bool translate)
{
  ZSwcTree *tree = new ZSwcTree;
  tree->forceVirtualRoot();
  for(auto x:points)
  {
    double a=x.getX(),b=x.getY(),c=x.getZ();
    if(translate)
    {
      a+=doc->getStack()->getOffset().m_x;
      b+=doc->getStack()->getOffset().m_y;
      c+=doc->getStack()->getOffset().m_z;
    }
    Swc_Tree_Node *tn =SwcTreeNode::makePointer(ZPoint(a,b,c),0.4);
    SwcTreeNode::setParent(tn, tree->root());
    //ZSandbox::GetCurrentDoc()->addSwcTree(tree,true,false);
  }
  tree->resortId();
  tree->addComment(comment);
  doc->executeAddObjectCommand(tree);
}

void connectSwc(ZStackDoc* doc,std::string comment,double distance)
{
  /*
   QList<ZSwcTree*> trees=doc->getSwcList();
   QList<Swc_Tree_Node*> ts;
   bool bnot=false;
   if(comment[0]=='!')
   {
     bnot=true;
     comment=comment.substr(1);
   }
   std::for_each(trees.begin(),trees.end(),[&](ZSwcTree* tree)
   {
     std::string x=tree->toString().substr(1);
     if((comment=="" && ((bnot&&x.find("#")!=std::string::npos)||(!bnot&&x.find("#")==std::string::npos)))
        ||(comment!=""&&((!bnot&&x.find(comment)!=std::string::npos) ||
                         (bnot &&x.find(comment)==std::string::npos))))
     {
       tree->forceVirtualRoot();
       Swc_Tree_Reconnect(tree->data(),1.0,distance);
     }
   });*/

}


