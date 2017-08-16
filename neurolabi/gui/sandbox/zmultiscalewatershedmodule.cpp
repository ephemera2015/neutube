#include <QAction>
#include <QWidget>
#include <QSpinBox>
#include <QMessageBox>
#include <vector>
#include <QPushButton>
#include <QGridLayout>
#include <QCheckBox>
#include <cstdlib>
#include <fstream>
#include <set>
#include <map>
#include "zmultiscalewatershedmodule.h"
#include "imgproc/zstackwatershed.h"
#include "zstackdoc.h"
#include "zsandbox.h"
#include "mainwindow.h"
#include "neutubeconfig.h"
#include "zobject3dscan.hpp"
#include "imgproc/zstackmultiscalewatershed.h"
#include "imgproc/zgmm.h"
#include "imgproc/zshortestpath.h"
#include "imgproc/utils.h"
#include "zobject3dfactory.h"
#include "zobject3darray.h"
#include "zstackdocdatabuffer.h"
#include "zstackframe.h"
#include "zcolorscheme.h"
#include "zstackskeletonizer.h"

ZMultiscaleWaterShedModule::ZMultiscaleWaterShedModule(QObject *parent) :
  ZSandboxModule(parent)
{
  init();
}


ZMultiscaleWaterShedModule::~ZMultiscaleWaterShedModule()
{
  delete window;
}


void ZMultiscaleWaterShedModule::init()
{  
  m_action = new QAction("Segmentation", this);
  connect(m_action, SIGNAL(triggered()), this, SLOT(execute()));
  window=new ZWaterShedWindow();
}


void ZMultiscaleWaterShedModule::execute()
{
  window->show();
}


ZWaterShedWindow::ZWaterShedWindow(QWidget *parent) :
  QWidget(parent)
{
  this->setWindowTitle("Segmentation");
  Qt::WindowFlags flags = this->windowFlags();
  flags |= Qt::WindowStaysOnTopHint;
  this->setWindowFlags(flags);
  QGridLayout* lay=new QGridLayout(this);
  lay->addWidget(new QLabel("Downsample Scale"),0,0,1,2);
  lay->addWidget(new QLabel("DownSmaple Path Scale"),1,0,1,2);
  spin_step=new QSpinBox();
  spin_step->setMinimum(1);
  lay->addWidget(spin_step,0,2);
  path_step=new QSpinBox();
  path_step->setMinimum(1);
  lay->addWidget(path_step,1,2);
  make_skeleton=new QPushButton("Make Skeleton");
  lay->addWidget(make_skeleton,2,0);
  path=new QPushButton("Create Path");
  lay->addWidget(path,2,1);
  merge=new QPushButton("Merge Selected");
  lay->addWidget(merge,2,2);
  ok=new QPushButton("Segment");
  cancel=new QPushButton("Cancel");
  lay->addWidget(cancel,3,2);
  lay->addWidget(ok,3,0,1,2);
  this->setLayout(lay);
  this->move(300,200);
  connect(ok,SIGNAL(clicked()),this,SLOT(onOk()));
  connect(cancel,SIGNAL(clicked()),this,SLOT(onCancel()));
  connect(path,SIGNAL(clicked()),this,SLOT(onCreatePath()));
  connect(merge,SIGNAL(clicked()),this,SLOT(onMerge()));
  connect(make_skeleton,SIGNAL(clicked()),this,SLOT(onMakeSkeleton()));
}


void erode(ZStack* stack)
{
  uint8_t* p=stack->array8();
  ZStack* rv=stack->clone();
  uint8_t* q=rv->array8();
  int w=stack->width(),h=stack->height(),d=stack->depth();
  size_t s=w*h;
  for(int k=0;k<d;++k)
  {
    for(int j=1;j<h-1;++j)
    {
      for(int i=1;i<w-1;++i)
      {
        if(!q[i+j*w+k*s])
        {
          p[i+1+j*w+k*s]=p[i+-1+j*w+k*s]=
          p[i+(j-1)*w+k*s]=p[i+(j+1)*w+k*s]=0;
        }
      }
    }
  }
  delete rv;
}

void dilate(ZStack* stack)
{
  uint8_t* p=stack->array8();
  ZStack* rv=stack->clone();
  uint8_t* q=rv->array8();
  int w=stack->width(),h=stack->height(),d=stack->depth();
  size_t s=w*h;
  for(int k=0;k<d;++k)
  {
    for(int j=1;j<h-1;++j)
    {
      for(int i=1;i<w-1;++i)
      {
        if(q[i+j*w+k*s])
        {
          p[i+1+j*w+k*s]=p[i+-1+j*w+k*s]=
          p[i+(j-1)*w+k*s]=p[i+(j+1)*w+k*s]=q[i+j*w+k*s];
        }
      }
    }
  }
  delete rv;
}


ZStack* autoBinary(const ZStack* stack,const ZStack* mask=NULL,double threshold=0.99)
{
  ZStack* ps=ZGMM().probabilityMap(stack);
  ZStack* rv=new ZStack(GREY,stack->width(),stack->height(),stack->depth(),1);
  rv->setOffset(stack->getOffset());

  for(size_t i=0;i<stack->getVoxelNumber();++i)
  {
    if(ps->array64()[i]>=threshold)
    {
      if(mask)
      {
        if(mask->array8()[i])
        {
          rv->array8()[i]=255;
        }
      }
      else
      {
        rv->array8()[i]=255;
      }
    }
  }

  for(int i=0;i<2;++i)
  {
    dilate(rv);
  }
  for(int i=0;i<4;++i)
  {
    erode(rv);
  }

  delete ps;
  return rv;
}


ZSwcTree* makeSkeleton(ZStack* stack)
{
  Stack *stackData = stack->c_stack();
  ZSwcTree *wholeTree = NULL;

  ZStackSkeletonizer skeletonizer;

  skeletonizer.setDownsampleInterval(0,0,0);
  skeletonizer.setRebase(true);
  skeletonizer.setMinObjSize(5);
  skeletonizer.setDistanceThreshold(0);
  skeletonizer.setLengthThreshold(5);
  skeletonizer.setKeepingSingleObject(false);

  wholeTree = skeletonizer.makeSkeleton(stackData);
  wholeTree->translate(stack->getOffset());


  if (wholeTree != NULL)
  {
     wholeTree->addComment("skeleton");
 //    frame->executeAddObjectCommand(wholeTree);
 //    ZWindowFactory::Open3DWindow(frame, Z3DWindow::INIT_EXCLUDE_VOLUME);
//   frame->open3DWindow(Z3DWindow::INIT_EXCLUDE_VOLUME);
  }
  else
  {
    std::cout<<"skeleton failed"<<std::endl;
  }
  return wholeTree;
}


ZStack* getSelectedArea(ZStackDoc* doc)
{
  ZStack* rv=doc->getStack()->clone();
  QList<ZObject3dScan*> areas=doc->getObject3dScanList();
  if(areas.size()==0)
  {
    return rv;
  }
  for(auto x:areas)
  {
    if(!x->isSelected())
      x->drawStack(rv,0);
  }
  return rv;
}


void  ZWaterShedWindow::onMakeSkeleton()
{
  ZStackDoc* doc=ZSandbox::GetCurrentDoc();
  if(!doc)return;
  ZStack* src=doc->getStack();
  if(!src)return;

  ZStack* mask=getSelectedArea(doc);
  ZStack* binary=autoBinary(src,mask,0.99);
  if(ZSwcTree* skeleton=makeSkeleton(binary))
  {
    skeleton->setZOrder(2);
    doc->executeAddObjectCommand(skeleton);
  }
  delete binary;
  delete mask;
}


struct Distance
{
  double operator ()(ZShortestPath<Distance>* z,const ZIntPoint& q)
  {
    double x=-z->at(q.getX()-1,q.getY()-1,q.getZ())+z->at(q.getX()+1,q.getY()-1,q.getZ());
    x+=-2*z->at(q.getX()-1,q.getY(),q.getZ())+2*z->at(q.getX()+1,q.getY(),q.getZ());
    x+=-z->at(q.getX()-1,q.getY()+1,q.getZ())+z->at(q.getX()+1,q.getY()+1,q.getZ());
    double y=z->at(q.getX()-1,q.getY()-1,q.getZ())-z->at(q.getX()-1,q.getY()+1,q.getZ());
    y+=2*z->at(q.getX(),q.getY()-1,q.getZ())-2*z->at(q.getX(),q.getY()+1,q.getZ());
    y+=z->at(q.getX()+1,q.getY()-1,q.getZ())-z->at(q.getX()+1,q.getY()+1,q.getZ());
    return std::exp(int(50-std::sqrt(x*x+y*y)*0.1));
  }
};

struct Distance3
{
  double operator ()(ZShortestPath<Distance3>* z,const ZIntPoint& q)
  {
    double x=z->at(q.getX()-1,q.getY()-1,q.getZ())-z->at(q.getX()+1,q.getY()+1,q.getZ());
    double y=z->at(q.getX()+1,q.getY()-1,q.getZ())-z->at(q.getX()-1,q.getY()+1,q.getZ());
    return 2<<(int(26-std::sqrt(x*x+y*y)*0.1+z->at(q.getX(),q.getY(),q.getZ())*0.02));
  }
};


void ZWaterShedWindow::onMerge()
{
  ZStackDoc *doc =ZSandbox::GetCurrentDoc();
  if(!doc)return;
  ZStack  *src=doc->getStack();
  if(!src)return;

  QList<ZObject3dScan*> selected=doc->getSelectedObjectList<ZObject3dScan>(ZStackObject::TYPE_OBJECT3D_SCAN);
  if(selected.size()<2)
    return;
  ZObject3dScan* first=selected[0];
  for(int i=1;i<selected.size();++i)
  {
    for(auto x:selected[i]->getStripeArray())
    {
      first->addStripe(x);
    }
    doc->removeObject(selected[i]);
  }

}

void ZWaterShedWindow::onCreatePath()
{
  std::vector<ZIntPoint> points;
  swc2Points(ZSandbox::GetCurrentDoc(),points,"");
  rmSwcTree(ZSandbox::GetCurrentDoc(),"");
  std::vector<ZIntPoint> out;
  ZShortestPath<Distance>().shortestPathMultiSlice(ZSandbox::GetCurrentDoc()->getStack(),points,out,path_step->value());
  points2Swc(out,ZSandbox::GetCurrentDoc(),"path");
}


void getBoundarySoftSeeds(ZStackDoc* doc,std::vector<ZIntPoint>& points)
{
  swc2Points(doc,points,"surf");
  swc2Points(doc,points,"path");
}


void getAreaHardSeeds(ZStackDoc* doc,QList<ZSwcTree*>& seeds)
{

  for(auto x:doc->getSwcList())
  {
    if(x->toString().substr(1).find("#")==std::string::npos)
    {
      seeds.push_back(x);
    }
    else if(x->toString().substr(1).find("skeleton")!=std::string::npos)
    {
      seeds.push_back(x);
    }
  }
}


void updateSegmentationResult(ZStackDoc* doc,const ZStack* result)
{
  QList<ZObject3dScan*> selected=doc->getSelectedObjectList<ZObject3dScan>(ZStackObject::TYPE_OBJECT3D_SCAN);
  for(auto x=selected.begin();x!=selected.end();++x)
  {
    doc->removeObject(*x,true);
  }

  std::vector<ZObject3dScan*> objArray =ZObject3dFactory::MakeObject3dScanPointerArray(*result, 1, false);
 /* if(create_new_frame)
  {
    ZStackFrame* frame=ZSandbox::GetMainWindow()->createStackFrame(doc->getStack()->clone());
    ZSandbox::GetMainWindow()->addStackFrame(frame);
    ZSandbox::GetMainWindow()->presentStackFrame(frame);
    //skeleton->setSelectable(true);
    //skeleton->setVisible(true);
    //skeleton->setHitProtocal(ZStackObject::HIT_WIDGET_POS);
    ZSandbox::GetCurrentFrame()->document()->executeAddObjectCommand(skeleton);
    //->getDataBuffer()->addUpdate(skeleton,ZStackDocObjectUpdate::ACTION_ADD_UNIQUE);
   // ZSandbox::GetCurrentFrame()->document()->getDataBuffer()->deliver();
    ndoc=ZSandbox::GetCurrentDoc();
  }*/
  ZColorScheme colorScheme;
  colorScheme.setColorScheme(ZColorScheme::UNIQUE_COLOR);
  static int colorIndex = 0;
  for (std::vector<ZObject3dScan*>::iterator iter = objArray.begin();
       iter != objArray.end(); ++iter)
  {
    ZObject3dScan *obj = *iter;
    if (obj != NULL && !obj->isEmpty())
    {
      QColor color = colorScheme.getColor(colorIndex++);
      color.setAlpha(164);
      obj->setColor(color);
      doc->getDataBuffer()->addUpdate(obj,ZStackDocObjectUpdate::ACTION_ADD_UNIQUE);
      doc->getDataBuffer()->deliver();
    }
  }
}


void ZWaterShedWindow::onOk()
{

  //get and check current doc and stack
  ZStackDoc *doc =ZSandbox::GetCurrentDoc();
  if(!doc)return;
  ZStack  *src=doc->getStack();
  if(!src)return;

  int scale=spin_step->value();


  ZStack* selected=getSelectedArea(doc);


  std::vector<ZIntPoint>points;
  getBoundarySoftSeeds(doc,points);
  for(auto x:points)//lowwer down boundary area intensity
  {
    selected->array8()[x.m_x+x.m_y*src->width()+x.m_z*src->width()*src->height()]
    =std::max(0,selected->array8()[x.m_x+x.m_y*src->width()+x.m_z*src->width()*src->height()]-70);
  }

  QList<ZSwcTree*> seeds;
  getAreaHardSeeds(doc,seeds);

  ZStackMultiScaleWatershed watershed;
  clock_t start=clock();
  ZStack* result=watershed.run(selected,seeds,scale);
  clock_t end=clock();
  std::cout<<"multiscale watershed total run time:"<<double(end-start)/CLOCKS_PER_SEC<<std::endl;
  if(result)
  {
    updateSegmentationResult(doc,result);
    delete result;
  }
  delete selected;
  //clear all seeds
  rmSwcTree(doc,"");
  rmSwcTree(doc,"surf");
  rmSwcTree(doc,"path");
  rmSwcTree(doc,"skeleton");
}


void ZWaterShedWindow::onCancel()
{
  this->hide();
}

