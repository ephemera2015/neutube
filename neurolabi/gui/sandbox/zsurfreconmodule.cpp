#include "zsurfreconmodule.h"
#include <functional>
#include <unordered_map>
#include <QAction>
#include <QMessageBox>
#include <set>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QJsonParseError>
#include <QHBoxLayout>
#include <QCryptographicHash>
#include "zsandbox.h"
#include "mainwindow.h"
#include "zswcforest.h"
#include "imgproc/zsurfrecon.h"
#include "imgproc/utils.h"

ZSurfReconWindow::ZSurfReconWindow(QWidget *parent):QWidget(parent)
{
  man=new QNetworkAccessManager(this);
  lay=new QHBoxLayout(this);
  ok=new QPushButton("ok");
  use_service=new QCheckBox("use service");
  service_url=new QLineEdit();
  lay->addWidget(use_service);
  lay->addWidget(new QLabel("url:"));
  lay->addWidget(service_url);
  lay->addWidget(ok);
  this->setLayout(lay);
  this->move(300,200);
  Qt::WindowFlags flags = this->windowFlags();
  flags |= Qt::WindowStaysOnTopHint;
  this->setWindowFlags(flags);
  connect(ok,SIGNAL(clicked()),this,SLOT(onOk()));
}


void ZSurfReconWindow::onNetworkError(QNetworkReply::NetworkError)
{
  QMessageBox::information(0,"service error","netwrok error, please check your service url");
}


void ZSurfReconWindow::onServiceDone()
{
  QNetworkReply* reply=(QNetworkReply*)sender();
  if(reply->objectName()=="post")//here comes the result url and md5
  {
    QString url="";
    QString md5="";
    getResultUrlAndMd5(reply->readAll(),url,md5);
    if(url=="" || md5=="")
    {
      QMessageBox::information(0,"service error","no result or md5 createad");
      return;
    }
    result_md5=md5;
    result_url=url;
    getResult(url);
  }
  if(reply->objectName()=="get")//here comes the result
  {
    QByteArray result=reply->readAll();
    QByteArray ba=QCryptographicHash::hash(result,QCryptographicHash::Md5);
    if(ba.toHex()!=this->result_md5)
    {
      getResult(result_url);
      return;
    }
    std::cout<<"result md5 correct"<<std::endl;
    VoxelSet out;
    QString result_str=result;
    if(result_str!="")
    {
      QStringList lines=result_str.split('\n');
      for(int i=0;i<lines.size();++i)
      {
        QStringList nums=lines[i].split(' ');
        if(nums.size()==3)
        {
          out.push_back(VoxelType(nums[0].toDouble(),
                                nums[1].toDouble(),
                                nums[2].toDouble()));
        }
      }
      ZSurfRecon::LabelStack(out,src);
      ZSurfRecon::GaussianBlur(out,src,5);
      QList<ZSwcTree*> trees=doc->getSwcList();
      std::for_each(trees.begin(),trees.end(),[&](ZSwcTree* tree)
      {
        if(tree->toString().find("skeleton")!=std::string::npos)
        {
          ZSurfRecon::PruneSkeleton(out,tree);
        }
      });
      for(QList<ZSwcTree*>::iterator it=trees.begin();it!=trees.end();++it)
      {

        if((*it)->toString().find("skeleton")==std::string::npos)
        {
          doc->removeObject(*it);
        }
      }
    }
  }
  reply->deleteLater();
}


void ZSurfReconWindow::getResult(QString sub_url)
{

  QString url=service_url->text();
  if(! url.startsWith("http"))
  {
    url=QString("http://")+url;
  }
  if(url.endsWith('/'))
  {
    url.remove(url.length(),1);
  }
  url+=sub_url;
  std::cout<<url.toStdString()<<std::endl;
  QNetworkReply* reply=man->get(QNetworkRequest(QUrl(url)));
  reply->setObjectName("get");
  connect(reply, SIGNAL(finished()), this, SLOT(onServiceDone()));
  connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
           this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
}


void ZSurfReconWindow::onOk()
{
  doc =ZSandbox::GetCurrentDoc();
  if(!doc)return;
  src=doc->getStack();
  if(!src)return;

  VoxelSet points_in;
  getPoints(points_in);
  if(points_in.size()<1)
    return;
  std::cout<<"number of surface reconstruction input points:"<<points_in.size()<<std::endl;
  if(use_service->isChecked())
  {
    QString url=service_url->text();
    if(! url.startsWith("http"))
    {
      url=QString("http://")+url;
    }
    if(! url.endsWith('/'))
    {
      url+="/";
    }
    url+="surfrecon";

    std::cout<<url.toStdString()<<std::endl;
    QByteArray data;
    getPostData(points_in,data);
    std::cout<<data.toStdString()<<std::endl;
    QNetworkReply* reply=man->post(QNetworkRequest(QUrl(url)),data);
    reply->setObjectName("post");
    connect(reply, SIGNAL(finished()), this, SLOT(onServiceDone()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
             this, SLOT(onNetworkError(QNetworkReply::NetworkError)));
  }
  else//compute at local
  {
    VoxelSet points_out;
    std::map<int,VoxelSet> m;
    for(auto x:points_in)
    {
      m[x.z].push_back(x);
    }

   // ZSurfRecon::SurfRecon(points_in,points_out);
    for(int s=1;s<4;s++)
    {
      for(int i=0;i<s;++i)
      {
          VoxelSet ti,to;
          for(auto x:m)
          {
            ti.insert(ti.end(),x.second.begin()+x.second.size()/s*i,x.second.begin()+x.second.size()/s*(i+1));
          }
          if(ti.size())
            ZSurfRecon::SurfRecon(ti,to);
          points_out.insert(points_out.end(),to.begin(),to.end());
      }
    }
    std::set<ZIntPoint>pset;
    uint8_t* p=src->array8();
    int width=src->width(),height=src->height(),area=width*height;
    size_t _off=src->getOffset().m_x+src->getOffset().m_y*width+src->getOffset().m_z*area;
    size_t max_off=src->getVoxelNumber();
    for(uint i=0;i<points_out.size();++i)
    {
      const VoxelType& v=points_out[i];
      for(int x=int(v.x);x<=int(v.x)+1;++x)
        for(int y=int(v.y);y<=int(v.y)+1;++y)
          for(int z=int(v.z);z<=int(v.z)+1;++z)
          {
            size_t offset=x+y*width+z*area-_off;
            if(offset<max_off)
            {
               pset.insert(ZIntPoint(x,y,z));
            }
          }

    }
    std::vector<ZIntPoint> points(pset.begin(),pset.end());
    points2Swc(points,doc,"surf",false);
    //connectSwc(doc,"surf",10);
    rmSwcTree(doc,"path");
  }
}


void ZSurfReconWindow::getResultUrlAndMd5(const QByteArray& reply,
                                          QString& result_url,
                                          QString& md5)
{
  QJsonParseError json_error;
  QJsonDocument doucment = QJsonDocument::fromJson(reply, &json_error);

  if(json_error.error == QJsonParseError::NoError)
  {
    if(doucment.isObject())
    {
      QJsonObject obj = doucment.object();
      if(obj.contains("md5"))
        md5=obj.take("md5").toString();
      if(obj.contains("result url"))
        result_url=obj.take("result url").toString();
    }
  }
}


void ZSurfReconWindow::getPostData(const VoxelSet& points,QByteArray& data)
{
  QString p="";
  for(auto& x:points)
  {
    p+="(";
    p+=QString::number(x.x)+",";
    p+=QString::number(x.y)+",";
    p+=QString::number(x.z);
    p+=")";
  }
  data.append(QString("points=")+p);
}


void ZSurfReconWindow::getPoints(VoxelSet& points)
{
  QList<ZSwcTree*> trees=doc->getSwcList();

  for(QList<ZSwcTree*>::iterator it=trees.begin();it!=trees.end();++it)
  {
    if((*it)->toString().find("skeleton")==std::string::npos &&
       ((*it)->toString().find("path")!=std::string::npos ||
        (*it)->toString().substr(1).find("#")==std::string::npos)
       )
    {
      ZSwcTree::DepthFirstIterator iter(*it);
      while(iter.hasNext())
      {
        Swc_Tree_Node* tn=iter.next();
        if(!SwcTreeNode::isVirtual(tn))
        {
          points.push_back(VoxelType(SwcTreeNode::x(tn),
                                               SwcTreeNode::y(tn),
                                               SwcTreeNode::z(tn)));
        }
      }
    }
  }
}


ZSurfReconModule::ZSurfReconModule(QObject *parent) :
  ZSandboxModule(parent)
{
  init();
}


void ZSurfReconModule::init()
{
  window=new ZSurfReconWindow();
  m_action = new QAction("SurfRecon", this);
  connect(m_action, SIGNAL(triggered()), this, SLOT(execute()));
}


void ZSurfReconModule::execute()
{
  this->window->show();
}

