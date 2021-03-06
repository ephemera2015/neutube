#include "zdvidlabelslice.h"

#include <QColor>
#include <QRect>
#include <QtCore>
#if QT_VERSION >= 0x050000
#include <QtConcurrent>
#endif

#include "tz_math.h"
#include "zarray.h"
#include "dvid/zdvidreader.h"
#include "zobject3dfactory.h"
#include "flyem/zflyembodymerger.h"
#include "zimage.h"
#include "zpainter.h"
#include "neutubeconfig.h"
#include "zpixmap.h"
#include "zstring.h"
#include "zintcuboid.h"

ZDvidLabelSlice::ZDvidLabelSlice()
{
  init(512, 512);
}

ZDvidLabelSlice::ZDvidLabelSlice(int maxWidth, int maxHeight)
{
  init(maxWidth, maxHeight);
}

ZDvidLabelSlice::~ZDvidLabelSlice()
{
  delete m_paintBuffer;
  delete m_labelArray;
  delete m_mappedLabelArray;
}

void ZDvidLabelSlice::init(int maxWidth, int maxHeight  , NeuTube::EAxis sliceAxis)
{
  setTarget(ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS);
  m_type = GetType();
  m_objColorSheme.setColorScheme(ZColorScheme::CONV_RANDOM_COLOR);
  m_hitLabel = 0;
  m_bodyMerger = NULL;
  setZOrder(0);

  m_maxWidth = maxWidth;
  m_maxHeight = maxHeight;

  m_paintBuffer = NULL;
//  m_paintBuffer = new ZImage(m_maxWidth, m_maxHeight, QImage::Format_ARGB32);
  m_labelArray = NULL;
  m_mappedLabelArray = NULL;

  m_selectionFrozen = false;
  m_isFullView = false;
  m_sliceAxis = sliceAxis;

  m_currentZ = 0;
  m_currentZoom = 0;

//  setColor(QColor(0, 0, 128));
//  m_zoom = 0;

//  m_objCache.setMaxCost();
}

ZSTACKOBJECT_DEFINE_CLASS_NAME(ZDvidLabelSlice)

class ZDvidLabelSlicePaintTask {
public:
  ZDvidLabelSlicePaintTask(ZDvidLabelSlice *labelSlice)
  {
    m_labelSlice = labelSlice;
  }

  void setLabelSlice(ZDvidLabelSlice *slice) {
    m_labelSlice = slice;
  }

  void addObject(ZObject3dScan *obj) {
    m_objArray.append(obj);
  }

  static void ExecuteTask(ZDvidLabelSlicePaintTask &task) {
    for (QList<ZObject3dScan*>::iterator iter = task.m_objArray.begin();
         iter != task.m_objArray.end(); ++iter) {
      ZObject3dScan *obj = *iter;

      if (task.m_labelSlice->getSelectedOriginal().count(obj->getLabel()) > 0) {
        obj->setSelected(true);
      } else {
        obj->setSelected(false);
      }

      //      obj.display(painter, slice, option);

      if (!obj->isSelected()) {
        task.m_labelSlice->getPaintBuffer()->setData(*(obj));
      } else {
        task.m_labelSlice->getPaintBuffer()->setData(
              *(obj), QColor(255, 255, 255, 255));
      }
    }
  }

private:
  ZDvidLabelSlice *m_labelSlice;
  QList<ZObject3dScan*> m_objArray;
};

#define ZDVIDLABELSLICE_MT 1

void ZDvidLabelSlice::setSliceAxis(NeuTube::EAxis sliceAxis)
{
  m_sliceAxis = sliceAxis;
}

void ZDvidLabelSlice::display(
    ZPainter &painter, int /*slice*/, EDisplayStyle /*option*/,
    NeuTube::EAxis sliceAxis) const
{
  if (m_sliceAxis != sliceAxis) {
    return;
  }

#ifdef _DEBUG_2
  QElapsedTimer timer;
  timer.start();
#endif

  if (isVisible()) {
    if (m_paintBuffer != NULL) {
      if (m_paintBuffer->isVisible()) {
        ZPixmap pixmap;
        pixmap.convertFromImage(*m_paintBuffer, Qt::ColorOnly);
        pixmap.setTransform(m_paintBuffer->getTransform());

        pixmap.matchProj();

#ifdef _DEBUG_2
        pixmap.save((GET_TEST_DATA_DIR + "/test.tif").c_str());
#endif
//        painter.save();
//        painter.setOpacity(getColor().alphaF());
        painter.drawPixmap(pixmap);
        painter.setPainted(true);
//        painter.restore();
      }
    }
#if 0
    if (m_currentViewParam.getViewPort().width() > m_paintBuffer->width() ||
        m_currentViewParam.getViewPort().height() > m_paintBuffer->height()) {
      for (ZObject3dScanArray::const_iterator iter = m_objArray.begin();
           iter != m_objArray.end(); ++iter) {
        ZObject3dScan &obj = const_cast<ZObject3dScan&>(*iter);

        if (m_selectedOriginal.count(obj.getLabel()) > 0) {
          obj.setSelected(true);
        } else {
          obj.setSelected(false);
        }

        obj.display(painter, slice, option, sliceAxis);
      }
    } else {
      m_paintBuffer->clear();
      m_paintBuffer->setOffset(-m_currentViewParam.getViewPort().x(),
                               -m_currentViewParam.getViewPort().y());

#if defined(ZDVIDLABELSLICE_MT)
      QList<ZDvidLabelSlicePaintTask> taskList;
      const int taskCount = 4;
      for (int i = 0; i < taskCount; ++i) {
        taskList.append(ZDvidLabelSlicePaintTask(
                          const_cast<ZDvidLabelSlice*>(this)));
      }
#endif

      int count = 0;
      for (ZObject3dScanArray::const_iterator iter = m_objArray.begin();
           iter != m_objArray.end(); ++iter, ++count) {
        ZObject3dScan &obj = const_cast<ZObject3dScan&>(*iter);
#if defined(ZDVIDLABELSLICE_MT)
        taskList[count % taskCount].addObject(&obj);
#else
        //if (m_selectedSet.count(obj.getLabel()) > 0) {
        if (m_selectedOriginal.count(obj.getLabel()) > 0) {
          obj.setSelected(true);
        } else {
          obj.setSelected(false);
        }

        //      obj.display(painter, slice, option);

        if (!obj.isSelected()) {
          m_paintBuffer->setData(obj);
        } else {
          m_paintBuffer->setData(obj, QColor(255, 255, 255, 164));
        }
#endif
      }

#if defined(ZDVIDLABELSLICE_MT)
      QtConcurrent::blockingMap(taskList, &ZDvidLabelSlicePaintTask::ExecuteTask);
#endif

      //    painter.save();
      //    painter.setOpacity(0.5);

      painter.drawImage(m_currentViewParam.getViewPort().x(),
                        m_currentViewParam.getViewPort().y(),
                        *m_paintBuffer);

    }

    ZOUT(LTRACE(), 5) << "Body buffer painting time: " << timer.elapsed();
//    painter.restore();

#ifdef _DEBUG_2
//      m_paintBuffer->save((GET_TEST_DATA_DIR + "/test.tif").c_str());
#endif
#endif
  }
}

void ZDvidLabelSlice::update()
{
  if (m_objArray.empty()) {
    update(true);
//    update(m_currentViewParam, true);
  }
}

#if 0
void ZDvidLabelSlice::forceUpdate(bool ignoringHidden)
{
  forceUpdate(m_currentDataRect, m_currentZ, ignoringHidden);
}
#endif

void ZDvidLabelSlice::setDvidTarget(const ZDvidTarget &target)
{
//  m_dvidTarget = target;
#ifdef _DEBUG_2
  m_dvidTarget.set("emdata1.int.janelia.org", "e8c1", 8600);
  m_dvidTarget.setLabelBlockName("labels3");
#endif
  m_reader.open(target);
}

int64_t ZDvidLabelSlice::getReadingTime() const
{
  return m_reader.getReadingTime();
}
/*
int ZDvidLabelSlice::getZoom() const
{
  return std::min(m_zoom, getDvidTarget().getMaxLabelZoom());
}
*/

int ZDvidLabelSlice::getZoomLevel(const ZStackViewParam &viewParam) const
{
  double zoomRatio = viewParam.getZoomRatio();
  if (zoomRatio == 0.0) {
    return 0;
  }

  int zoom = 0;

  if (getDvidTarget().usingMulitresBodylabel()) {
    zoom = iround(std::log(1.0 / zoomRatio) / std::log(2.0) );

    if (zoom < 0) {
      zoom = 0;
    }

    int scale = pow(2, zoom);
    if (viewParam.getViewPort().width() * viewParam.getViewPort().height() /
        scale / scale > 512 * 512) {
      zoom += 1;
    }

    if (zoom > getDvidTarget().getMaxLabelZoom()) {
      zoom = getDvidTarget().getMaxLabelZoom();
    }
  }

  return zoom;
}

void ZDvidLabelSlice::updateRgbTable()
{
  const ZColorScheme *colorScheme = &(getColorScheme());
  if (m_customColorScheme.get() != NULL) {
    colorScheme = m_customColorScheme.get();
  }

  const QVector<QColor>& colorTable = colorScheme->getColorTable();
  m_rgbTable.resize(colorTable.size());
  for (int i = 0; i < colorTable.size(); ++i) {
    const QColor &color = colorTable[i];
    m_rgbTable[i] = (164 << 24) + (color.red() << 16) + (color.green() << 8) +
        (color.blue());
  }
}

void ZDvidLabelSlice::paintBufferUnsync()
{
  if (m_labelArray != NULL && m_paintBuffer != NULL) {
    if ((int) m_labelArray->getElementNumber() ==
        m_paintBuffer->width() * m_paintBuffer->height()) {
      updateRgbTable();
      remapId();

      uint64_t *labelArray = NULL;

      if (m_paintBuffer->isVisible()) {
        if (m_selectedOriginal.empty() && getLabelMap().empty() &&
            m_customColorScheme.get() == NULL) {
          labelArray = m_labelArray->getDataPointer<uint64_t>();
        } else {
          labelArray = m_mappedLabelArray->getDataPointer<uint64_t>();
        }
      }

      if (labelArray != NULL) {
        if (getSliceAxis() == NeuTube::X_AXIS) {
          m_paintBuffer->drawLabelFieldTranspose(
                labelArray, m_rgbTable, 0, 0xFFFFFFFF);
        } else {
          m_paintBuffer->drawLabelField(labelArray, m_rgbTable, 0, 0xFFFFFFFF);
        }
      }
    }
  }
}

void ZDvidLabelSlice::paintBuffer()
{
  QMutexLocker locker(&m_updateMutex);
  paintBufferUnsync();
}

void ZDvidLabelSlice::clearLabelData()
{
  delete m_labelArray;
  m_labelArray = NULL;
  delete m_mappedLabelArray;
  m_mappedLabelArray = NULL;
}

void ZDvidLabelSlice::forceUpdate(bool ignoringHidden)
{
  QMutexLocker locker(&m_updateMutex);

  m_objArray.clear();
  if ((!ignoringHidden) || isVisible()) {
    int zoom = m_currentZoom;
    int zoomRatio = pow(2, zoom);
    int z = m_currentZ;

    QRect viewPort = m_currentDataRect;

    if (NeutubeConfig::GetVerboseLevel() >= 1) {
      std::cout << "Deleting label array:" << m_labelArray << std::endl;
    }

    ZIntCuboid box;
    box.setFirstCorner(viewPort.left(), viewPort.top(), z);
    box.setSize(viewPort.width(), viewPort.height(), 1);

    int width = box.getWidth() / zoomRatio;
    int height = box.getHeight() / zoomRatio;
    int depth = box.getDepth();
    int x0 = box.getFirstCorner().getX() / zoomRatio;
    int y0 = box.getFirstCorner().getY() / zoomRatio;
    int z0 = box.getFirstCorner().getZ();

    ZGeometry::shiftSliceAxisInverse(x0, y0, z0, getSliceAxis());
    ZGeometry::shiftSliceAxisInverse(width, height, depth, getSliceAxis());


    box.shiftSliceAxisInverse(m_sliceAxis);

    clearLabelData();

    QString cacheKey = (box.getFirstCorner().toString() + " " +
        box.getLastCorner().toString() + " " + ZString::num2str(zoom)).c_str();

    if (m_objCache.contains(cacheKey)) {
      m_labelArray = m_objCache.take(cacheKey);
    } else {
      if (box.getDepth() == 1) {
#if defined(_ENABLE_LOWTIS_)
        m_labelArray = m_reader.readLabels64Lowtis(
              box.getFirstCorner().getX(), box.getFirstCorner().getY(),
              box.getFirstCorner().getZ(), box.getWidth(), box.getHeight(),
              zoom);
#else
        m_labelArray = m_reader.readLabels64(
              box.getFirstCorner().getX(), box.getFirstCorner().getY(),
              box.getFirstCorner().getZ(), box.getWidth(), box.getHeight(), 1,
              zoom);
#endif
      } else {
        m_labelArray = m_reader.readLabels64Raw(
              x0, y0, z0, width, height, depth, zoom);
      }
    }

    if (m_labelArray != NULL) {

      ZGeometry::shiftSliceAxis(width, height, depth, getSliceAxis());
      ZGeometry::shiftSliceAxis(x0, y0, z0, getSliceAxis());

      delete m_paintBuffer;
      m_paintBuffer = new ZImage(width, height, QImage::Format_ARGB32);
      paintBufferUnsync();
      ZStTransform transform;
      transform.setScale(1.0 / zoomRatio, 1.0 / zoomRatio);
      transform.setOffset(-x0, -y0);;
      m_paintBuffer->setTransform(transform);

#ifdef _DEBUG_2
      m_paintBuffer->save("/Users/zhaot/Work/neutube/neurolabi/data/test.tif");
#endif
    }
  }
}

void ZDvidLabelSlice::update(int z)
{
//  ZStackViewParam viewParam = m_currentViewParam;
//  viewParam.setZ(z);

  update(m_currentDataRect, m_currentZoom, z);

//  m_isFullView = false;
}

const ZDvidTarget& ZDvidLabelSlice::getDvidTarget() const
{
  return m_reader.getDvidTarget();
}

void ZDvidLabelSlice::updateFullView(const ZStackViewParam &viewParam)
{
  m_isFullView = true;

  update(viewParam);
}

QRect ZDvidLabelSlice::getDataRect(const ZStackViewParam &viewParam) const
{
  QRect viewPort = viewParam.getViewPort();

  if (!getDvidTarget().usingMulitresBodylabel() && !m_isFullView) {
    int width = viewPort.width();
    int height = viewPort.height();
    int area = width * height;
    if (area > m_maxWidth * m_maxHeight) {
      if (width > m_maxWidth) {
        width = m_maxWidth;
      }
      if (height > m_maxHeight) {
        height = m_maxHeight;
      }
      QPoint oldCenter = viewPort.center();
      viewPort.setSize(QSize(width, height));
      viewPort.moveCenter(oldCenter);
    }
  }

  return viewPort;
}

int ZDvidLabelSlice::getCurrentZ() const
{
  return m_currentZ;
}

bool ZDvidLabelSlice::update(const QRect &dataRect, int zoom, int z)
{
  bool updating = false;

  if (z != getCurrentZ()) {
    updating = true;
  } else if (!m_isFullView) {
    if (zoom != m_currentZoom) {
      updating = true;
    } else {
      if (!m_currentDataRect.contains(dataRect)) {
        updating = true;
      }
    }
  }

  bool updated = false;

  if (updating) {
    m_currentDataRect = dataRect;
    m_currentZoom = zoom;
    m_currentZ = z;

    forceUpdate(true);
    updated = true;

//    m_isFullView = false;
  }

  return updated;
}

bool ZDvidLabelSlice::update(const ZStackViewParam &viewParam)
{
  if (viewParam.getSliceAxis() != m_sliceAxis) {
    return false;
  }

  if (viewParam.getViewPort().isEmpty()) {
    return false;
  }

  QRect dataRect = getDataRect(viewParam);

  return update(dataRect, getZoomLevel(viewParam), viewParam.getZ());
}

QColor ZDvidLabelSlice::getLabelColor(
    uint64_t label, NeuTube::EBodyLabelType labelType) const
{
  QColor color;
  if (hasCustomColorMap()) {
    color = getCustomColor(label);
    if (color.alpha() != 0) {
      color.setAlpha(164);
    }
  } else {
    color = m_objColorSheme.getColor(
          getMappedLabel(label, labelType));
    color.setAlpha(164);
  }

  return color;
}

QColor ZDvidLabelSlice::getLabelColor(
    int64_t label, NeuTube::EBodyLabelType labelType) const
{
  return getLabelColor((uint64_t) label, labelType);
}

void ZDvidLabelSlice::setCustomColorMap(
    const ZSharedPointer<ZFlyEmBodyColorScheme> &colorMap)
{
  m_customColorScheme = colorMap;
  assignColorMap();
}

bool ZDvidLabelSlice::hasCustomColorMap() const
{
  return m_customColorScheme.get() != NULL;
}

void ZDvidLabelSlice::removeCustomColorMap()
{
  m_customColorScheme.reset();
  assignColorMap();
}

void ZDvidLabelSlice::assignColorMap()
{
  updateRgbTable();
  for (ZObject3dScanArray::iterator iter = m_objArray.begin();
       iter != m_objArray.end(); ++iter) {
    ZObject3dScan &obj = *iter;
    //obj.setColor(getColor(obj.getLabel()));
    obj.setColor(getLabelColor(obj.getLabel(), NeuTube::BODY_LABEL_ORIGINAL));
  }
}

void ZDvidLabelSlice::remapId()
{
//  QMutexLocker locker(&m_updateMutex);
  if (m_labelArray != NULL && m_mappedLabelArray == NULL) {
    m_mappedLabelArray = new ZArray(m_labelArray->valueType(),
                                    m_labelArray->ndims(),
                                    m_labelArray->dims());
  }
  remapId(m_mappedLabelArray);
}

void ZDvidLabelSlice::remapId(
    uint64_t *array, const uint64_t *originalArray, uint64_t v)
{
  if (m_customColorScheme.get() != NULL) {

    QHash<uint64_t, int> idMap;

    idMap = m_customColorScheme->getColorIndexMap();

    if (hasVisualEffect(NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED)) {
      for (size_t i = 0; i < v; ++i) {
        array[i] = 0;
      }
    } else {
      for (size_t i = 0; i < v; ++i) {
        array[i] = originalArray[i];
        if (idMap.contains(array[i])) {
          array[i] = idMap[array[i]];
        } else {
          array[i] = 0;
        }
      }
    }
  }
}


void ZDvidLabelSlice::remapId(
    uint64_t *array, const uint64_t *originalArray, uint64_t v,
    std::set<uint64_t> &selected)
{
  if (m_customColorScheme.get() != NULL) {

    QHash<uint64_t, int> idMap;

    idMap = m_customColorScheme->getColorIndexMap();

    if (hasVisualEffect(NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED)) {
      for (size_t i = 0; i < v; ++i) {
        if (selected.count(originalArray[i]) > 0) {
          array[i] = originalArray[i];
          if (idMap.contains(array[i])) {
            array[i] = idMap[array[i]];
          } else {
            array[i] = 0;
          }
        } else {
          array[i] = 0;
        }
      }
    } else {
      for (size_t i = 0; i < v; ++i) {
        if (selected.count(originalArray[i]) > 0) {
          array[i] = FlyEM::LABEL_ID_SELECTION;
        } else {
          array[i] = originalArray[i];
          if (idMap.contains(array[i])) {
            array[i] = idMap[array[i]];
          } else {
            array[i] = 0;
          }
        }
      }
    }
  } else {
    if (hasVisualEffect(NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED)) {
      for (size_t i = 0; i < v; ++i) {
        if (selected.count(originalArray[i]) > 0) {
          array[i] = originalArray[i];
        } else {
          array[i] = 0;
        }
      }
    } else {
      for (size_t i = 0; i < v; ++i) {
        if (selected.count(originalArray[i]) > 0) {
          array[i] = FlyEM::LABEL_ID_SELECTION;
        } else {
          array[i] = originalArray[i];
        }
      }
    }
  }
}

void ZDvidLabelSlice::remapId(
    uint64_t *array, const uint64_t *originalArray, uint64_t v,
    const ZFlyEmBodyMerger::TLabelMap &bodyMap)
{

  if (m_customColorScheme.get() != NULL) {
    QHash<uint64_t, int> idMap;

    idMap = m_customColorScheme->getColorIndexMap();

    if (hasVisualEffect(NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED)) {
      m_paintBuffer->setVisible(false);
    } else {
      for (size_t i = 0; i < v; ++i) {
        if (bodyMap.count(originalArray[i]) > 0) {
          array[i] = bodyMap[originalArray[i]];
        } else {
          array[i] = originalArray[i];
        }
        if (idMap.contains(array[i])) {
          array[i] = idMap[array[i]];
        } else {
          array[i] = 0;
        }
      }
    }
  } else {
    if (hasVisualEffect(NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED)) {
      m_paintBuffer->setVisible(false);
    } else {
      for (size_t i = 0; i < v; ++i) {
        if (bodyMap.count(originalArray[i]) > 0) {
          array[i] = bodyMap[originalArray[i]];
        } else {
          array[i] = originalArray[i];
        }
      }
    }
  }
}

void ZDvidLabelSlice::remapId(
    uint64_t *array, const uint64_t *originalArray, uint64_t v,
    std::set<uint64_t> &selected, const ZFlyEmBodyMerger::TLabelMap &bodyMap)
{
  if (m_customColorScheme.get() != NULL) {
    QHash<uint64_t, int> idMap;


    idMap = m_customColorScheme->getColorIndexMap();

    std::set<uint64_t> selectedSet = selected;

    if (hasVisualEffect(NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED)) {
      for (size_t i = 0; i < v; ++i) {
        if (selectedSet.count(originalArray[i]) > 0) {
          if (bodyMap.count(originalArray[i]) > 0) {
            array[i] = bodyMap[originalArray[i]];
          } else {
            array[i] = originalArray[i];
          }
          if (idMap.contains(array[i])) {
            array[i] = idMap[array[i]];
          } else {
            array[i] = 0;
          }
        } else {
          array[i] = 0;
        }
      }
    } else {
      for (size_t i = 0; i < v; ++i) {
        if (selectedSet.count(originalArray[i]) > 0) {
          array[i] = FlyEM::LABEL_ID_SELECTION;
        } else if (bodyMap.count(originalArray[i]) > 0) {
          array[i] = bodyMap[originalArray[i]];
          if (idMap.contains(array[i])) {
            array[i] = idMap[array[i]];
          } else {
            array[i] = 0;
          }
        } else {
          array[i] = originalArray[i];
          if (idMap.contains(array[i])) {
            array[i] = idMap[array[i]];
          } else {
            array[i] = 0;
          }
        }
      }
    }
  } else {
    std::set<uint64_t> selectedSet = selected;

    if (hasVisualEffect(NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED)) {
      for (size_t i = 0; i < v; ++i) {
        if (selectedSet.count(originalArray[i]) > 0) {
          if (bodyMap.count(originalArray[i]) > 0) {
            array[i] = bodyMap[originalArray[i]];
          } else {
            array[i] = originalArray[i];
          }
        } else {
          array[i] = 0;
        }
      }
    } else {
      for (size_t i = 0; i < v; ++i) {
        if (selectedSet.count(originalArray[i]) > 0) {
          array[i] = FlyEM::LABEL_ID_SELECTION;
        } else if (bodyMap.count(originalArray[i]) > 0) {
          array[i] = bodyMap[originalArray[i]];
        } else {
          array[i] = originalArray[i];
        }
      }
    }
  }
}

void ZDvidLabelSlice::remapId(ZArray *label)
{
  if (m_paintBuffer != NULL) {
    m_paintBuffer->setVisible(true);
  }

  if (m_labelArray != NULL && label != NULL) {
    ZFlyEmBodyMerger::TLabelMap bodyMap = getLabelMap();
    uint64_t *array = label->getDataPointer<uint64_t>();
    const uint64_t *originalArray = m_labelArray->getDataPointer<uint64_t>();
    size_t v = label->getElementNumber();
    if (!bodyMap.empty() || !m_selectedOriginal.empty()) {
      if (bodyMap.empty()) {
        remapId(array, originalArray, v, m_selectedOriginal);
      } else if (m_selectedOriginal.empty()) {
        remapId(array, originalArray, v, bodyMap);
      } else {
        remapId(array, originalArray, v, m_selectedOriginal, bodyMap);
      }
    } else {
      if (m_customColorScheme.get() != NULL) {
        remapId(array, originalArray, v);
      }
      if (hasVisualEffect(NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED)) {
        if (m_paintBuffer != NULL) {
          m_paintBuffer->setVisible(false);
        }
      }
    }
  }
}

bool ZDvidLabelSlice::hit(double x, double y, double z)
{
  m_hitLabel = 0;

  if (!m_objArray.empty()) {
    for (ZObject3dScanArray::iterator iter = m_objArray.begin();
         iter != m_objArray.end(); ++iter) {
      ZObject3dScan &obj = *iter;
      if (obj.hit(x, y, z)) {
        //m_hitLabel = obj.getLabel();
        m_hitLabel = getMappedLabel(obj);
        return true;
      }
    }
  } else {
    int nx = iround(x);
    int ny = iround(y);
    int nz = iround(z);

    ZGeometry::shiftSliceAxisInverse(nx, ny, nz, m_sliceAxis);
    if (m_currentDataRect.contains(nx, ny) && nz == m_currentZ) {
//      ZDvidReader reader;
      if (m_reader.isReady()) {
        ZGeometry::shiftSliceAxis(nx, ny, nz, m_sliceAxis);
        m_hitLabel = getMappedLabel(m_reader.readBodyIdAt(nx, ny, nz),
                                    NeuTube::BODY_LABEL_ORIGINAL);
      }

      return m_hitLabel > 0;
    }
  }

  return false;
}

std::set<uint64_t> ZDvidLabelSlice::getHitLabelSet() const
{
  std::set<uint64_t> labelSet;
  if (m_hitLabel > 0) {
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(m_hitLabel);
      labelSet.insert(
            selectedOriginal.begin(), selectedOriginal.end());
    } else {
      labelSet.insert(m_hitLabel);
    }
  }

  return labelSet;
}

uint64_t ZDvidLabelSlice::getHitLabel() const
{
  return m_hitLabel;
}

void ZDvidLabelSlice::selectHit(bool appending)
{
  if (m_hitLabel > 0) {
    if (!appending) {
      clearSelection();
    }

    addSelection(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);
//    addSelection(m_hitLabel);
//    m_selectedOriginal.insert(m_hitLabel);
  }
}

ZFlyEmBodyMerger::TLabelMap ZDvidLabelSlice::getLabelMap() const
{
  ZFlyEmBodyMerger::TLabelMap labelMap;
  if (m_bodyMerger != NULL) {
    labelMap = m_bodyMerger->getFinalMap();
  }

  return labelMap;
}

void ZDvidLabelSlice::setSelection(const std::set<uint64_t> &selected,
                                   NeuTube::EBodyLabelType labelType)
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    m_selectedOriginal = selected;
    break;
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(selected.begin(), selected.end());
      m_selectedOriginal.clear();
      m_selectedOriginal.insert(
            selectedOriginal.begin(), selectedOriginal.end());
    } else {
      m_selectedOriginal = selected;
    }
    break;
  }
}

void ZDvidLabelSlice::addSelection(
    uint64_t bodyId, NeuTube::EBodyLabelType labelType)
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    m_selectedOriginal.insert(bodyId);
    break;
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(bodyId);
      m_selectedOriginal.insert(
            selectedOriginal.begin(), selectedOriginal.end());
    } else {
      m_selectedOriginal.insert(bodyId);
    }
    break;
  }

//  m_selectedSet.insert(getMappedLabel(bodyId, labelType));
}

void ZDvidLabelSlice::xorSelection(
    uint64_t bodyId, NeuTube::EBodyLabelType labelType)
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    if (m_selectedOriginal.count(bodyId) > 0) {
      m_selectedOriginal.erase(bodyId);
    } else {
      m_selectedOriginal.insert(bodyId);
    }
    break;
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(bodyId);
      xorSelectionGroup(selectedOriginal.begin(), selectedOriginal.end(),
                        NeuTube::BODY_LABEL_ORIGINAL);
    } else {
      xorSelection(bodyId, NeuTube::BODY_LABEL_ORIGINAL);
    }
  }
}

void ZDvidLabelSlice::deselectAll()
{
  m_selectedOriginal.clear();
}

bool ZDvidLabelSlice::isBodySelected(
    uint64_t bodyId, NeuTube::EBodyLabelType labelType) const
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    return m_selectedOriginal.count(bodyId) > 0;
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      QSet<uint64_t> selectedOriginal =
          m_bodyMerger->getOriginalLabelSet(bodyId);
      for (QSet<uint64_t>::const_iterator iter = selectedOriginal.begin();
           iter != selectedOriginal.end(); ++iter) {
        if (m_selectedOriginal.count(*iter) > 0) {
          return true;
        }
      }
    } else {
      return m_selectedOriginal.count(bodyId) > 0;
    }
    break;
  }

  return false;
}

void ZDvidLabelSlice::toggleHitSelection(bool appending)
{
  bool hasSelected = isBodySelected(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);
  xorSelection(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);

  if (!appending) {
    clearSelection();

    if (!hasSelected) {
      addSelection(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);
    }
  }

//  paintBuffer();
  //xorSelection(m_hitLabel, NeuTube::BODY_LABEL_MAPPED);

  /*
  bool hasSelected = false;
  if (m_selectedSet.count(m_hitLabel) > 0) {
    hasSelected = true;
    m_selectedSet.erase(m_hitLabel);
  }

  if (!appending) {
    clearSelection();
  }

  if (!hasSelected) {
    selectHit(true);
  }
  */
}

void ZDvidLabelSlice::clearSelection()
{
  m_selectedOriginal.clear();
}

/*
void ZDvidLabelSlice::updateLabel(const ZFlyEmBodyMerger &merger)
{
  for (ZObject3dScanArray::iterator iter = m_objArray.begin();
       iter != m_objArray.end(); ++iter) {
    ZObject3dScan &obj = *iter;
    obj.setLabel(merger.getFinalLabel(obj.getLabel()));
  }
}
*/

void ZDvidLabelSlice::updateLabelColor()
{
  /*
  if (m_bodyMerger != NULL) {
    updateLabel(*m_bodyMerger);
  }
  */
  assignColorMap();
}

void ZDvidLabelSlice::setBodyMerger(ZFlyEmBodyMerger *bodyMerger)
{
  m_bodyMerger = bodyMerger;
}

void ZDvidLabelSlice::setMaxSize(
    const ZStackViewParam &viewParam, int maxWidth, int maxHeight)
{
  if (m_maxWidth != maxWidth || m_maxHeight != maxHeight) {
    m_maxWidth = maxWidth;
    m_maxHeight = maxHeight;
    m_objArray.clear();
    delete m_paintBuffer;
    m_paintBuffer = new ZImage(m_maxWidth, m_maxHeight, QImage::Format_ARGB32);

    update(viewParam);
  }
}

std::set<uint64_t> ZDvidLabelSlice::getOriginalLabelSet(
    uint64_t mappedLabel) const
{
  std::set<uint64_t> labelSet;
  if (m_bodyMerger == NULL) {
    labelSet.insert(mappedLabel);
  } else {
    const QSet<uint64_t> &sourceSet = m_bodyMerger->getOriginalLabelSet(mappedLabel);
    labelSet.insert(sourceSet.begin(), sourceSet.end());
  }

  return labelSet;
}

uint64_t ZDvidLabelSlice::getMappedLabel(const ZObject3dScan &obj) const
{
  return getMappedLabel(obj.getLabel(), NeuTube::BODY_LABEL_ORIGINAL);
}

uint64_t ZDvidLabelSlice::getMappedLabel(
    uint64_t label, NeuTube::EBodyLabelType labelType) const
{
  if (labelType == NeuTube::BODY_LABEL_ORIGINAL) {
    if (m_bodyMerger != NULL) {
      return m_bodyMerger->getFinalLabel(label);
    }
  }

  return label;
}
/*
const ZStackViewParam& ZDvidLabelSlice::getViewParam() const
{
  return m_currentViewParam;
}
*/
std::set<uint64_t> ZDvidLabelSlice::getSelected(
    NeuTube::EBodyLabelType labelType) const
{
  switch (labelType) {
  case NeuTube::BODY_LABEL_ORIGINAL:
    return getSelectedOriginal();
  case NeuTube::BODY_LABEL_MAPPED:
    if (m_bodyMerger != NULL) {
      return m_bodyMerger->getFinalLabel(getSelectedOriginal());
    } else {
      return getSelectedOriginal();
    }
    break;
  }

  return std::set<uint64_t>();
}

void ZDvidLabelSlice::mapSelection()
{
  m_selectedOriginal = getSelected(NeuTube::BODY_LABEL_MAPPED);
}

void ZDvidLabelSlice::recordSelection()
{
  m_prevSelectedOriginal = m_selectedOriginal;
}

void ZDvidLabelSlice::processSelection()
{
  m_selector.reset(m_selectedOriginal, m_prevSelectedOriginal);

#if 0
  for (std::set<uint64_t>::const_iterator iter = m_selectedOriginal.begin();
       iter != m_selectedOriginal.end(); ++iter) {
    if (m_prevSelectedOriginal.count(*iter) == 0) {
      m_selector.selectObject(*iter);
    }
  }

  for (std::set<uint64_t>::const_iterator iter = m_prevSelectedOriginal.begin();
       iter != m_prevSelectedOriginal.end(); ++iter) {
    if (m_selectedOriginal.count(*iter) == 0) {
      m_selector.deselectObject(*iter);
    }
  }
#endif
}

QColor ZDvidLabelSlice::getCustomColor(uint64_t label) const
{
  QColor color(0, 0, 0, 0);

  if (m_customColorScheme.get() != NULL) {
    color = m_customColorScheme->getBodyColor(label);
  }

  return color;
}

void ZDvidLabelSlice::clearCache()
{
  m_objCache.clear();
}

bool ZDvidLabelSlice::refreshReaderBuffer()
{
  return m_reader.refreshLabelBuffer();
}
