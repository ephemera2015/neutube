#ifndef UTILS_H
#define UTILS_H
#include "zstackdoc.h"

void connectSwc(ZStackDoc* doc,std::string comment,double distance);
void swc2Points(ZStackDoc* doc,std::vector<ZIntPoint>& points,std::string comment,bool translate=true);
void rmSwcTree(ZStackDoc* doc,std::string comment);
void points2Swc(const std::vector<ZIntPoint>& points,ZStackDoc* doc,std::string comment,bool translate=true);
#endif // UTILS_H
