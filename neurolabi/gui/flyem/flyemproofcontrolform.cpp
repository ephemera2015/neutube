#include "flyemproofcontrolform.h"

#include <QMenu>
#include <QInputDialog>
#include <QSortFilterProxyModel>

#include "ui_flyemproofcontrolform.h"
#include "dialogs/zdviddialog.h"
#include "zstring.h"
#include "neutubeconfig.h"
#include "flyem/zflyembodymergeproject.h"
#include "zstackdoc.h"
#include "zflyembookmarkview.h"
#include "widgets/zcolorlabel.h"
#include "zwidgetfactory.h"
#include "znormcolormap.h"
#include "flyem/zflyembodycoloroption.h"
#include "zglobal.h"

FlyEmProofControlForm::FlyEmProofControlForm(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::FlyEmProofControlForm)
{
  ui->setupUi(this);
  setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Expanding);

  m_latencyWidget =
      ZWidgetFactory::MakeColorLabel(Qt::gray, "Seg Latency", 100, false, this);
  ui->topHorizontalLayout->addWidget(m_latencyWidget);
  /*
  connect(ui->segmentCheckBox, SIGNAL(clicked(bool)),
          this, SIGNAL(segmentVisibleChanged(bool)));
  ui->segmentCheckBox->hide();
  */
  connect(ui->mergeSegmentPushButton, SIGNAL(clicked()),
          this, SIGNAL(mergingSelected()));
  connect(ui->dvidPushButton, SIGNAL(clicked()),
          this, SIGNAL(dvidSetTriggered()));
  connect(ui->segmentSizePushButton, SIGNAL(clicked()),
          this, SLOT(setSegmentSize()));
  connect(ui->saveMergePushButton, SIGNAL(clicked()),
          this, SIGNAL(savingMerge()));
  connect(ui->splitPushButton, SIGNAL(clicked()),
          this, SIGNAL(splitTriggered()));
  connect(ui->uploadPushButton, SIGNAL(clicked()),
          this, SIGNAL(committingMerge()));

  ui->segmentSizePushButton->hide();
  ui->segmentSizeDecPushButton->setEnabled(false);

  ui->saveMergePushButton->hide();
  ui->dataInfoWidget->hide();

//  ui->bodyViewPushButton->hide();


  connect(ui->segmentSizeIncPushButton, SIGNAL(clicked()),
          this, SLOT(incSegmentSize()));
  connect(ui->segmentSizeDecPushButton, SIGNAL(clicked()),
          this, SLOT(decSegmentSize()));
  connect(ui->fullViewPushButton, SIGNAL(clicked()),
          this, SLOT(showFullSegmentation()));

  connect(ui->coarseBodyPushButton, SIGNAL(clicked()),
          this, SIGNAL(coarseBodyViewTriggered()));
  connect(ui->bodyViewPushButton, SIGNAL(clicked()),
          this, SIGNAL(bodyViewTriggered()));
  connect(ui->skeletonViewPushButton, SIGNAL(clicked()),
          this, SIGNAL(skeletonViewTriggered()));

  connect(getAssignedBookmarkView(), SIGNAL(locatingBookmark(const ZFlyEmBookmark*)),
          this, SLOT(locateBookmark(const ZFlyEmBookmark*)));
  connect(getAssignedBookmarkView(), SIGNAL(bookmarkChecked(QString,bool)),
          this, SIGNAL(bookmarkChecked(QString, bool)));
  connect(getAssignedBookmarkView(), SIGNAL(bookmarkChecked(ZFlyEmBookmark*)),
          this, SIGNAL(bookmarkChecked(ZFlyEmBookmark*)));
  connect(getAssignedBookmarkView(), SIGNAL(removingBookmark(ZFlyEmBookmark*)),
          this, SIGNAL(removingBookmark(ZFlyEmBookmark*)));

  connect(getUserBookmarkView(), SIGNAL(locatingBookmark(const ZFlyEmBookmark*)),
          this, SLOT(locateBookmark(const ZFlyEmBookmark*)));
  connect(getUserBookmarkView(), SIGNAL(bookmarkChecked(ZFlyEmBookmark*)),
          this, SIGNAL(bookmarkChecked(ZFlyEmBookmark*)));
  connect(getUserBookmarkView(), SIGNAL(removingBookmark(ZFlyEmBookmark*)),
          this, SIGNAL(removingBookmark(ZFlyEmBookmark*)));
  connect(getUserBookmarkView(), SIGNAL(removingBookmark(QList<ZFlyEmBookmark*>)),
          this, SIGNAL(removingBookmark(QList<ZFlyEmBookmark*>)));
  /*
  connect(ui->userBookmarkView, SIGNAL(bookmarkChecked(QString,bool)),
          this, SIGNAL(bookmarkChecked(QString, bool)));
          */

//  m_userBookmarkProxy = createSortingProxy(&m_userBookmarkList);
//  getUserBookmarkView()->setBookmarkModel(&m_userBookmarkList);
//  getAssignedBookmarkView()->setBookmarkModel(&m_assignedBookmarkList);
//  m_bookmarkProxy = createSortingProxy(&m_bookmarkList);
//  ui->bookmarkView->setModel(&m_bookmarkList);
//  ui->bookmarkView->setSortingEnabled(true);

//  getAssignedBookmarkView()->resizeColumnsToContents();
//  getUserBookmarkView()->resizeColumnsToContents();

  createMenu();
}

FlyEmProofControlForm::~FlyEmProofControlForm()
{
  delete ui;
}

QSortFilterProxyModel*
FlyEmProofControlForm::createSortingProxy(ZFlyEmBookmarkListModel *model)
{
  QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
  proxy->setSourceModel(model);
  proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
  proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
  proxy->setFilterKeyColumn(-1);

  return proxy;
}

ZFlyEmBookmarkView* FlyEmProofControlForm::getUserBookmarkView() const
{
  return ui->bookmarkWidget->getBookmarkView(
        ZFlyEmBookmarkWidget::SOURCE_USER);
//  return ui->userBookmarkView;
}

ZFlyEmBookmarkView* FlyEmProofControlForm::getAssignedBookmarkView() const
{
  return ui->bookmarkWidget->getBookmarkView(
        ZFlyEmBookmarkWidget::SOURCE_ASSIGNED);
//  return ui->bookmarkView;
}

static QAction* CreateColorAction(ZFlyEmBodyColorOption::EColorOption option,
                                  QWidget *parent)
{
  QAction *action =
      new QAction(ZFlyEmBodyColorOption::GetColorMapName(option), parent);
  action->setCheckable(true);

  return action;
}

void FlyEmProofControlForm::createColorMenu()
{
  QMenu *colorMenu = m_mainMenu->addMenu("Color Map");
  QActionGroup *colorActionGroup = new QActionGroup(this);
  QAction *normalColorAction = CreateColorAction(
          ZFlyEmBodyColorOption::BODY_COLOR_NORMAL, this);

  m_nameColorAction = CreateColorAction(
        ZFlyEmBodyColorOption::BODY_COLOR_NAME, this);
  m_nameColorAction->setEnabled(false);

  QAction *sequencerColorAction = CreateColorAction(
        ZFlyEmBodyColorOption::BODY_COLOR_SEQUENCER, this);

  QAction *focusedColorAction = CreateColorAction(
        ZFlyEmBodyColorOption::BODY_COLOR_FOCUSED, this);

  colorActionGroup->addAction(normalColorAction);
  colorActionGroup->addAction(m_nameColorAction);
  colorActionGroup->addAction(sequencerColorAction);
  colorActionGroup->addAction(focusedColorAction);
  colorActionGroup->setExclusive(true);

  colorMenu->addActions(colorActionGroup->actions());

  connect(colorActionGroup, SIGNAL(triggered(QAction*)),
          this, SLOT(changeColorMap(QAction*)));
}

void FlyEmProofControlForm::createMenu()
{
  m_mainMenu = new QMenu(this);
  ui->menuPushButton->setMenu(m_mainMenu);

  QAction *queryPixelAction = new QAction("Go to Position", this);
  m_mainMenu->addAction(queryPixelAction);
  connect(queryPixelAction, SIGNAL(triggered()), this, SLOT(goToPosition()));

  QAction *queryBodyAction = new QAction("Go to Body", this);
  queryBodyAction->setShortcut(Qt::Key_F1);
  m_mainMenu->addAction(queryBodyAction);
  connect(queryBodyAction, SIGNAL(triggered()), this, SLOT(goToBody()));

  QAction *selectBodyAction = new QAction("Select Body", this);
  selectBodyAction->setShortcut(Qt::Key_F2);
  m_mainMenu->addAction(selectBodyAction);
  connect(selectBodyAction, SIGNAL(triggered()), this, SLOT(selectBody()));

  createColorMenu();

  QAction *infoAction = new QAction("Information", this);
  m_mainMenu->addAction(infoAction);
  connect(infoAction, SIGNAL(triggered()), this, SIGNAL(showingInfo()));

  QMenu *bodyMenu = m_mainMenu->addMenu("Bodies");
  QAction *exportBodyAction = new QAction("Export Selected Bodies", this);
  connect(exportBodyAction, SIGNAL(triggered()),
          this, SLOT(exportSelectedBody()));
  bodyMenu->addAction(exportBodyAction);

  QAction *exportBodyLevelAction = new QAction("Export Selected Bodies (leveled)", this);
  connect(exportBodyLevelAction, SIGNAL(triggered()),
          this, SLOT(exportSelectedBodyLevel()));
  bodyMenu->addAction(exportBodyLevelAction);


  QAction *skeletonizeAction = new QAction("Skeletonize Selected Bodies", this);
  connect(skeletonizeAction, SIGNAL(triggered()),
          this, SLOT(skeletonizeSelectedBody()));
  bodyMenu->addAction(skeletonizeAction);

#if 0
  QMenu *grayscaleMenu = m_mainMenu->addMenu("Grayscale");
  QAction *exportGrayScaleAction = new QAction("Export Grayscale", this);
  connect(exportGrayScaleAction, SIGNAL(triggered()),
          this, SLOT(exportGrayscale()));
  grayscaleMenu->addAction(exportGrayScaleAction);
#endif

#ifdef _DEBUG_
  QMenu *developerMenu = m_mainMenu->addMenu("Developer");
  QAction *clearMergeAction = new QAction("Clear All Merges", this);
  connect(clearMergeAction, SIGNAL(triggered()),
          this, SLOT(clearBodyMergeStage()));
  developerMenu->addAction(clearMergeAction);

  QAction *exportBodyStackAction = new QAction("Export Body Stack", this);
  connect(exportBodyStackAction, SIGNAL(triggered()),
          this, SLOT(exportSelectedBodyStack()));
  developerMenu->addAction(exportBodyStackAction);
#endif
//  colorMenu->setEnabled(false);
}

void FlyEmProofControlForm::exportSelectedBodyStack()
{
  emit exportingSelectedBodyStack();
}

void FlyEmProofControlForm::exportSelectedBody()
{
  emit exportingSelectedBody();
}

void FlyEmProofControlForm::exportSelectedBodyLevel()
{
  emit exportingSelectedBodyLevel();
}

void FlyEmProofControlForm::skeletonizeSelectedBody()
{
  emit skeletonizingSelectedBody();
}

void FlyEmProofControlForm::exportGrayscale()
{
  emit exportingGrayscale();
}

void FlyEmProofControlForm::enableNameColorMap(bool on)
{
  if (m_nameColorAction != NULL) {
    m_nameColorAction->setEnabled(on);
  }
}

void FlyEmProofControlForm::changeColorMap(QAction *action)
{
  if (action != NULL) {
    emit changingColorMap(action->text());
  }
}

void FlyEmProofControlForm::clearBodyMergeStage()
{
  emit clearingBodyMergeStage();
}

void FlyEmProofControlForm::selectBody()
{
  emit selectingBody();
}

void FlyEmProofControlForm::setSegmentSize()
{
//  emit labelSizeChanged(1024, 1024);
}

void FlyEmProofControlForm::incSegmentSize()
{
  ui->segmentSizeIncPushButton->setEnabled(false);
  ui->segmentSizeDecPushButton->setEnabled(true);
  emit labelSizeChanged(1024, 1024);
}

void FlyEmProofControlForm::decSegmentSize()
{
  ui->segmentSizeIncPushButton->setEnabled(true);
  ui->segmentSizeDecPushButton->setEnabled(false);
  emit labelSizeChanged(512, 512);
}

void FlyEmProofControlForm::showFullSegmentation()
{
  emit showingFullSegmentation();
}

void FlyEmProofControlForm::goToBody()
{
  emit goingToBody();
}

void FlyEmProofControlForm::goToPosition()
{
  bool ok;

  QString defaultText;

  ZIntPoint pt = ZGlobal::GetInstance().getStackPosition();
  if (pt.isValid()) {
    defaultText = pt.toString().c_str();
  }

  QString text = QInputDialog::getText(
        this, tr("Go To"), tr("Coordinates:"), QLineEdit::Normal, defaultText,
        &ok);

  if (ok) {
    if (!text.isEmpty()) {
      ZString str = text.toStdString();
      std::vector<int> coords = str.toIntegerArray();
      if (coords.size() == 3) {
        emit zoomingTo(coords[0], coords[1], coords[2]);
      }
    }
  }
}

void FlyEmProofControlForm::updateWidget(const ZDvidTarget &target)
{
  setDvidInfo(target);
  ui->dvidPushButton->setEnabled(false);

  if (target.readOnly()) {
    ui->mergeSegmentPushButton->setEnabled(false);
//    ui->menuPushButton->setEnabled(false);
    ui->uploadPushButton->setEnabled(false);
    ui->splitPushButton->setEnabled(false);
  }
}

void FlyEmProofControlForm::setInfo(const QString &info)
{
  ui->dataInfoWidget->setText(info);
}

void FlyEmProofControlForm::setDvidInfo(const ZDvidTarget &target)
{
  std::string info = target.toJsonObject().dumpString(2);
#if defined(_FLYEM_)
  if (target.isSupervised()) {
    if (!target.getSupervisor().empty()) {
      info += "\nLibrarian: " + target.getSupervisor();
    } else {
      info += "\nLibrarian: " + GET_FLYEM_CONFIG.getDefaultLibrarian();
    }
  }
#endif
  setInfo(info.c_str());
}

void FlyEmProofControlForm::locateBookmark(const ZFlyEmBookmark *bookmark)
{
  if (bookmark != NULL) {
    emit zoomingTo(bookmark->getLocation().getX(),
                   bookmark->getLocation().getY(),
                   bookmark->getLocation().getZ());
  }
}

void FlyEmProofControlForm::locateAssignedBookmark(const QModelIndex &index)
{
  const ZFlyEmBookmark *bookmark = getAssignedBookmarkView()->getBookmark(index);

  locateBookmark(bookmark);
}

void FlyEmProofControlForm::locateUserBookmark(const QModelIndex &index)
{
  const ZFlyEmBookmark *bookmark = getUserBookmarkView()->getBookmark(index);

  locateBookmark(bookmark);
}

void FlyEmProofControlForm::updateLatency(int t)
{
  ZNormColorMap colorMap;
  int baseTime = 600;
  double v = (double) t / baseTime;
  QColor color = colorMap.mapColor(v);
  color.setAlpha(100);
  m_latencyWidget->setColor(color);
  m_latencyWidget->setText(QString("%1").arg(t));
}
