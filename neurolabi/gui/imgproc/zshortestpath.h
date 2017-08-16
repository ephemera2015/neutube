#ifndef ZSHORTESTPATH_H
#define ZSHORTESTPATH_H

#include<vector>
#include<map>
#include"zstackdoc.h"

template<typename DISTFUN>
class ZShortestPath
{
public:
  void shortestPathMultiSlice(const ZStack* stack,const std::vector<ZIntPoint>& points,std::vector<ZIntPoint>& out,int skip=1)
  {
    std::map<int,std::vector<ZIntPoint>> mps;
    for(auto x:points)
    {
      mps[x.m_z].push_back(x);
    }
    for(auto x:mps)
    {
      shortestPath(stack,x.second,out,skip);
    }
  }

  void shortestPath(const ZStack* stack,const std::vector<ZIntPoint>& points,std::vector<ZIntPoint>& out,int skip=1)
  {
    _stack=stack;
    _width=_stack->width();
    _height=stack->height();
    _depth=stack->depth();
    _slice=_width*_height;
    _p_stack=stack->array8();
    _distfun=DISTFUN();

    std::vector<ZIntPoint> _out;
    std::vector<std::pair<ZIntPoint,ZIntPoint>> pairs;
    cruskal(points,pairs);

    for(auto p:pairs)
    {
      const ZIntPoint &pre=p.first,&cur=p.second;
      ZIntCuboid cub;
      cub.setFirstX(std::max(0,std::min(pre.getX()-10,cur.getX()-10)));
      cub.setFirstY(std::max(0,std::min(pre.getY()-10,cur.getY()-10)));
      cub.setFirstZ(cur.getZ());
      cub.setLastX(std::min(_width-1,std::max(pre.getX()+10,cur.getX()+10)));
      cub.setLastY(std::min(_height-1,std::max(pre.getY()+10,cur.getY()+10)));
      cub.setLastZ(cur.getZ());
      dijkstra(pre,cur,cub,_out);
    }

    for(uint i=0;i<_out.size();++i)
    {
      if(i%skip==0)
      {
        out.push_back(_out[i]);
      }
    }
    out.push_back(_out[_out.size()-1]);
  }

  inline uint8_t at(int i,int j,int k)
  {
    if(i>=_width||j>=_height||k>=_depth)
      return 255;
    return _p_stack[i+j*_width+k*_slice];
  }
private:
  void dijkstra(const ZIntPoint&start,const ZIntPoint& end,const ZIntCuboid& cub,std::vector<ZIntPoint>& out)
  {
    std::set<ZIntPoint>s;
    ZIntPoint t=start;

    _dist.clear();
    _prev.clear();

    dist(start,0);
    _prev[start]=ZIntPoint(-1,-1,-1);

    for(;t!=end;)
    {
      double tmp = DBL_MAX;
      for(auto it=_dist.begin(); it!=_dist.end(); ++it)
      {
        const ZIntPoint& to=it->first;
        if(s.find(to)==s.end()&&it->second<tmp)
        {
          t = to;
          tmp = it->second;
        }
      }
      s.insert(t);
      ZIntPoint q;

      //double newdist=(_dist(t)*g_len[t]+ps[q.getX()+q.getY()*width+q.getZ()*slice])/(g_len[t]+1)+g_len[t]*0.0005;
  #define CHECK_UPDATE\
      if(cub.contains(q)&&s.find(q)==s.end())\
      {\
        double newdist=dist(t)+_distfun(this,q);\
        if(newdist<dist(q))\
        {\
          dist(q,newdist);\
          _prev[q]=t;\
        }\
      }

      q.set(t.getX()-1,t.getY(),t.getZ());
      CHECK_UPDATE;
      q.set(t.getX()+1,t.getY(),t.getZ());
      CHECK_UPDATE;
      q.set(t.getX(),t.getY()-1,t.getZ());
      CHECK_UPDATE;
      q.set(t.getX(),t.getY()+1,t.getZ());
      CHECK_UPDATE;
      q.set(t.getX()-1,t.getY()-1,t.getZ());
      CHECK_UPDATE;
      q.set(t.getX()-1,t.getY()+1,t.getZ());
      CHECK_UPDATE;
      q.set(t.getX()+1,t.getY()-1,t.getZ());
      CHECK_UPDATE;
      q.set(t.getX()+1,t.getY()+1,t.getZ());
      CHECK_UPDATE;
    }
    for(;t!=start;)//backtrace path
    {
      out.push_back(t);
      t=_prev[t];
    }
    out.push_back(t);
  #undef CHECK_UPDATE
  }

  void cruskal(const std::vector<ZIntPoint>& points,std::vector<std::pair<ZIntPoint,ZIntPoint>>& pairs)
  {
    std::map<ZIntPoint,uint>vset;
    std::multimap<double,std::pair<ZIntPoint,ZIntPoint> >* distances=new std::multimap<double,std::pair<ZIntPoint,ZIntPoint> >;
    for(auto x=points.begin();x!=points.end();++x)
    {
      for(auto y=points.begin();y!=points.end();++y)
      {
        if(*x!=*y)
        {
          distances->insert(std::pair<double,std::pair<ZIntPoint,ZIntPoint> >(_d(*x,*y),std::pair<ZIntPoint,ZIntPoint>(*x,*y)));
        }
      }
    }

    for(uint i=0;i<points.size();++i)
    {
      vset[points[i]]=i;
    }

    ZIntPoint pre,cur;
    uint cnt=0;

    for (std::multimap<double,std::pair<ZIntPoint,ZIntPoint> >::const_iterator x=distances->begin();x!=distances->end();++x,++x)
    {
      if(cnt==points.size()-1)
        break;
      const std::pair<ZIntPoint,ZIntPoint>& pt=x->second;
      pre=pt.first,cur=pt.second;
      if(vset[pre] != vset[cur])
      {
        vset[pre]=vset[cur];
        pairs.push_back(std::pair<ZIntPoint,ZIntPoint>(pre,cur));
        ++cnt;
      }
    }
    delete distances;
  }

  inline double dist(const ZIntPoint& x,double v=-1)
  {
    auto it=_dist.find(x);
    if( (it!=_dist.end()) && v!=-1 )
      _dist.erase(it);
    if(v!=-1)//insert mode
    {
      _dist.insert(std::pair<ZIntPoint,double>(x,v));
      return v;
    }
    if(it!=_dist.end())//query mode
    {
      return it->second;
    }
    return DBL_MAX;
  }
  inline double _d(const ZIntPoint& x,const ZIntPoint& y)
  {
    double a=(x.getX()-y.getX())*(x.getX()-y.getX());
    double b=(x.getY()-y.getY())*(x.getY()-y.getY());
    double c=(x.getZ()-y.getZ())*(x.getZ()-y.getZ());
    return std::sqrt(a+b+c);
  }
private:
  const ZStack* _stack;
  int _width,_height,_depth;
  size_t _slice;
  const uint8_t* _p_stack;
  std::map<ZIntPoint,ZIntPoint> _prev;
  std::map<ZIntPoint,double> _dist;
  DISTFUN _distfun;
};

#endif // ZSHORTESTPATH_H
