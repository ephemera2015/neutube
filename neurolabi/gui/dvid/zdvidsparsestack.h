#ifndef ZDVIDSPARSESTACK_H
#define ZDVIDSPARSESTACK_H

#include <QMutex>
#include <QMap>

#include "zstackobject.h"
#include "zsparsestack.h"
#include "zdvidtarget.h"
#include "dvid/zdvidreader.h"
#include "zthreadfuturemap.h"

class ZIntCuboid;

class ZDvidSparseStack : public ZStackObject
{
public:
  ZDvidSparseStack();
  ~ZDvidSparseStack();

  static ZStackObject::EType GetType() {
    return ZStackObject::TYPE_DVID_SPARSE_STACK;
  }

  void display(ZPainter &painter, int slice, EDisplayStyle option,
               NeuTube::EAxis sliceAxis) const;

  const std::string& className() const;

  ZStack *getSlice(int z) const;

  ZStack* getSlice(int z, int x0, int y0, int width, int height) const;

  /*!
   * \brief Get the dense representation of the sparse stack
   *
   * \return The returned stack is owned by the object.
   */
  ZStack* getStack();

  /*!
   * \brief Get the dense representation of the sparse stack after
   *        updating a certain region.
   *
   * \param updateBox The box region to update
   * \return The returned stack is owned by the object.
   */
  ZStack* getStack(const ZIntCuboid &updateBox);

  /*!
   * \brief Make a stack from the sparse representation.
   *
   * Other than the behavior of \a getStack, this function makes a new stack and
   * returns it. So the caller is responsible for deleting the returned object.
   * It returns NULL if the range is invalid.
   *
   * \param range Range of the produced stack.
   * \return A new stack in the range.
   */
  ZStack* makeStack(const ZIntCuboid &range);

  ZStack* makeIsoDsStack(size_t maxVolume);

  ZStack* makeDsStack(int xintv, int yintv, int zintv);

  bool stackDownsampleRequired();

  const ZDvidTarget& getDvidTarget() const {
    return m_dvidTarget;
  }


  int getValue(int x, int y, int z) const;

  const ZIntPoint& getDownsampleInterval() const;

  void setDvidTarget(const ZDvidTarget &target);

  ZIntCuboid getBoundBox() const;
//  using ZStackObject::getBoundBox; // fix warning -Woverloaded-virtual

  void loadBody(uint64_t bodyId, bool canonizing = false);
  void loadBodyAsync(uint64_t bodyId);
  void setMaskColor(const QColor &color);
  void setLabel(uint64_t bodyId);

  void loadBody(uint64_t bodyId, const ZIntCuboid &range, bool canonizing = false);

  uint64_t getLabel() const;

//  const ZObject3dScan *getObjectMask() const;
  ZObject3dScan *getObjectMask();
  ZStackBlockGrid* getStackGrid();

  const ZSparseStack* getSparseStack() const;
  ZSparseStack *getSparseStack();

  ZSparseStack *getSparseStack(const ZIntCuboid &box);

//  void downloadBodyMask(ZDvidReader &reader);

  bool hit(double x, double y, double z);
  bool hit(double x, double y, NeuTube::EAxis axis);

  bool isEmpty() const;

  ZDvidSparseStack* getCrop(const ZIntCuboid &box) const;

  void deprecateStackBuffer();

  int getReadStatusCode() const;

  void runFillValueFunc();
  void runFillValueFunc(const ZIntCuboid &box, bool syncing, bool cont = true);

  void setCancelFillValue(bool flag);
  void cancelFillValueSync();
//  void cancelFillValueFunc();

  /*!
   * \brief Only keep the largest component.
   */
  void shakeOff();

private:
  void init();
  void initBlockGrid();
  bool fillValue(bool cancelable = false);
  bool fillValue(const ZIntCuboid &box, bool cancelable = false);
  bool fillValue(const ZIntCuboid &box, bool cancelable, bool fillingAll);
  QString getLoadBodyThreadId() const;
  QString getFillValueThreadId() const;
  void pushMaskColor();
  void pushLabel();
  bool loadingObjectMask() const;
  void finishObjectMaskLoading();
  void syncObjectMask();
  void pushAttribute();

  ZDvidReader& getMaskReader() const;
  ZDvidReader& getGrayscaleReader() const;

  /*
  void assignStackValue(ZStack *stack, const ZObject3dScan &obj,
                               const ZStackBlockGrid &stackGrid);
                               */

private:
  ZSparseStack m_sparseStack;
  ZDvidTarget m_dvidTarget;
  bool m_isValueFilled;
  uint64_t m_label;
  mutable ZDvidReader m_dvidReader;
  mutable ZDvidReader m_grayScaleReader;
  mutable ZDvidReader m_maskReader;
  mutable ZDvidInfo m_grayscaleInfo;

  ZThreadFutureMap m_futureMap;
  bool m_cancelingValueFill;

  mutable QMutex m_fillValueMutex;
};

#endif // ZDVIDSPARSESTACK_H
