#ifndef ZWATERSHEDMODULE
#define ZWATERSHEDMODULE
#include <QWidget>
#include"zsandboxmodule.h"

class ZStack;


class QPushButton;
class QSpinBox;
class QCheckBox;
class ZStack;


class ZWaterShedWindow:public QWidget
{
  Q_OBJECT
public:
  ZWaterShedWindow(QWidget *parent = 0);
private slots:
  void onOk();
  void onCancel();
  void onCreatePath();
  void onMerge();
  void onMakeSkeleton();
private:
  QPushButton*  ok;
  QPushButton*  cancel;
  QPushButton*  merge;
  QPushButton*  path;
  QSpinBox*     spin_step;
  QSpinBox*     path_step;
  QPushButton*  make_skeleton;

};

class ZMultiscaleWaterShedModule:public ZSandboxModule
{
  Q_OBJECT
  public:
  explicit ZMultiscaleWaterShedModule(QObject *parent = 0);
    ~ZMultiscaleWaterShedModule();
  signals:
  private slots:
    void execute();
  private:
    void init();
  private:
    ZWaterShedWindow* window;
};

#endif // ZWATERSHEDMODULE

