#ifndef ZOBJECT3DSCANARRAY_H
#define ZOBJECT3DSCANARRAY_H

#include <vector>
#include <string>
#include "zobject3dscan.h"

class ZIntCuboid;
class ZStack;

class ZObject3dScanArray : public std::vector<ZObject3dScan>
{
public:
  ZObject3dScanArray();

  ZIntCuboid getBoundBox() const;

  void importDir(const std::string &dirPath);
  void downsample(int xintv, int yintv, int zintv);
  void upsample(int xIntv, int yIntv, int zIntv);
  void translate(int dx, int dy, int dz);

  ZStack* toStackObject() const;
  ZStack* toLabelField() const;
  ZStack* toLabelField(const ZIntCuboid &box) const;

};

#endif // ZOBJECT3DSCANARRAY_H
