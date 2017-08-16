#include<cmath>
#include<iostream>
#include"zgmm.h"
#include "zstack.hxx"



double ZGMM::g(double x,double delta,double u)
{
  static double s_pi=std::sqrt(3.1415926535898);
  static double s_2=1.4142135623731;

  double a=s_2*delta;
  double b=1.0/(a*s_pi);
  double c=-1*((x-u)*(x-u)/(a*a));
  return b*std::exp(c);
}


ZStack* ZGMM::probabilityMap(const ZStack* stack)
{
  std::vector<double> params=fit(stack);
  std::vector<double>yy(256);
  for(int i=0;i<256;++i)
  {
    yy[i]=params[0]*ZGMM::g(i,params[2],params[1])+params[3]*ZGMM::g(i,params[5],params[4]);
  }
  ZStack *rv=new ZStack(FLOAT64,stack->width(),stack->height(),stack->depth(),1);
  rv->setOffset(stack->getOffset());
  double* p=rv->array64();
  const uint8_t* q=stack->array8();
  for(int i=0;i<stack->getVoxelNumber();++i)
  {
    p[i]=params[3]*ZGMM::g(q[i],params[5],params[4])/yy[q[i]];
  }
  return rv;
}


std::vector<double> ZGMM::fit(const ZStack* stack)
{

  static double eps=1.0,bx=80.0,fx=160.0;
  std::vector<double> params;
  params.resize(2*3);
  params[0]=0.5;
  params[1]=bx;
  params[2]=10.0;
  params[3]=1.0-params[0];
  params[4]=fx;
  params[5]=10.0;
  size_t N=stack->getVoxelNumber();

  int max_iter=10;
  const uint8_t* data=stack->array8();
  do
  {
    double sum_gama_b=0.0,sum_gama_f=0.0;
    double sum_gama_y_b=0.0,sum_gama_y_f=0.0;
    double sum_gama_u_b=0.0,sum_gama_u_f=0.0;
    std::cout<<params[0]<<'\t'<<params[1]<<'\t'<<params[2]<<std::endl;
    std::cout<<params[3]<<'\t'<<params[4]<<'\t'<<params[5]<<std::endl;
    for(size_t n=0;n<N;++n)
    {
     double b=params[0]*g(data[n],params[2],params[1]);
     double f=params[3]*g(data[n],params[5],params[4]);
     double rea_b=b/(b+f),rea_f=1.0-rea_b;
     sum_gama_b+=rea_b;
     sum_gama_f+=rea_f;
     sum_gama_y_b+=rea_b*data[n];
     sum_gama_y_f+=rea_f*data[n];
     sum_gama_u_b+=rea_b*(data[n]-params[1])*(data[n]-params[1]);
     sum_gama_u_f+=rea_f*(data[n]-params[4])*(data[n]-params[4]);
    }
    if(std::abs(params[0]-sum_gama_b/N)<eps
       && std::abs(params[1]-sum_gama_y_b/sum_gama_b)<eps
       && std::abs(params[2]-std::sqrt(sum_gama_u_b/sum_gama_b))<eps)
    {
      break;
    }
    params[0]=sum_gama_b/N;
    params[3]=1.0-params[0];
    params[1]=sum_gama_y_b/sum_gama_b;
    params[4]=sum_gama_y_f/sum_gama_f;
    params[2]=std::sqrt(sum_gama_u_b/sum_gama_b);
    params[5]=std::sqrt(sum_gama_u_f/sum_gama_f);
    --max_iter;

  }while(true && max_iter);
  bx=params[1];
  fx=params[4];
  return params;
}

