#ifndef ZDVIDGRAYSLICE_H
#define ZDVIDGRAYSLICE_H

#include "zstackobject.h"
#include "zimage.h"
#include "zdvidreader.h"
#include "zstackviewparam.h"
#include "zpixmap.h"

//#include "zdvidtarget.h"

class ZRect2d;
class ZDvidReader;
class ZStack;
//class ZStackViewParam;

class ZDvidGraySlice : public ZStackObject
{
public:
  ZDvidGraySlice();
  ~ZDvidGraySlice();

  void display(ZPainter &painter, int slice, EDisplayStyle option,
               NeuTube::EAxis sliceAxis) const;
  void clear();

  void update(int z);
  bool update(const ZStackViewParam &viewParam);

//  void loadDvidSlice(const QByteArray &buffer, int z);

  virtual const std::string& className() const;

  void printInfo() const;

  void setDvidTarget(const ZDvidTarget &target);

  inline const ZDvidTarget& getDvidTarget() const {
    return m_reader.getDvidTarget();
  }

  inline int getX() const {
    return m_currentViewParam.getViewPort().left();
  }
  inline int getY() const {
    return m_currentViewParam.getViewPort().top();
  }
  inline int getZ() const {
    return m_currentViewParam.getZ();
  }
  inline void setZ(int z) {
    m_currentViewParam.setZ(z);
  }

  int getWidth() const { return m_currentViewParam.getViewPort().width(); }
  int getHeight() const { return m_currentViewParam.getViewPort().height(); }

  ZRect2d getBoundBox() const;
//  using ZStackObject::getBoundBox; // fix warning -Woverloaded-virtual

  void setBoundBox(const ZRect2d &rect);

  int getZoom() const;
  int getScale() const;

  void setZoom(int zoom);


public: //for testing
  void saveImage(const std::string &path);
  void savePixmap(const std::string &path);

private:
  void updateImage();
  void updateImage(const ZStack *stack);
  void forceUpdate(const ZStackViewParam &viewParam);
  void updatePixmap();

  /*!
   * \brief Check if the regions of the image and the slice are consistent.
   */
//  bool isRegionConsistent() const;

private:
  ZImage m_image;
  ZPixmap m_pixmap;

  QMutex m_pixmapMutex;
//  int m_x;
//  int m_y;
//  int m_z;
//  int m_width;
//  int m_height;
  ZStackViewParam m_currentViewParam;

  int m_zoom;

  int m_maxWidth;
  int m_maxHeight;

  ZDvidReader m_reader;
//  ZDvidTarget m_dvidTarget;
};

#endif // ZDVIDGRAYSLICE_H
