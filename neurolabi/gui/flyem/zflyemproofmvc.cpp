#include "zglew.h"
#include "zflyemproofmvc.h"

#include <QFuture>
#include <QtConcurrentRun>
#include <QMessageBox>
#include <QInputDialog>
#include <QPushButton>
#include <QMainWindow>
#include <QDesktopWidget>
#include <QApplication>

#include "flyem/zflyemproofdoc.h"
#include "zstackview.h"
#include "dvid/zdvidtileensemble.h"
#include "zstackpresenter.h"
#include "dialogs/zdviddialog.h"
#include "dvid/zdvidreader.h"
#include "zstackobjectsourcefactory.h"
#include "dvid/zdvidsparsestack.h"
#include "zprogresssignal.h"
#include "zstackviewlocator.h"
#include "widgets/zimagewidget.h"
#include "dvid/zdvidlabelslice.h"
#include "flyem/zflyemproofpresenter.h"
#include "zwidgetmessage.h"
#include "dialogs/zspinboxdialog.h"
#include "zdialogfactory.h"
#include "flyem/zflyembodyannotationdialog.h"
#include "zflyembodyannotation.h"
#include "flyem/zflyemsupervisor.h"
#include "dvid/zdvidwriter.h"
#include "zstring.h"
#include "flyem/zpaintlabelwidget.h"
#include "zwidgetfactory.h"
#include "flyem/zflyemcoordinateconverter.h"
#include "flyem/zflyembookmarkannotationdialog.h"
#include "dialogs/flyembodyinfodialog.h"
#include "protocols/protocolswitcher.h"
#include "dialogs/zflyemsplitcommitdialog.h"
#include "flyem/zflyembodywindowfactory.h"
#include "flyem/zflyemmisc.h"
#include "zswcgenerator.h"
#include "zflyembody3ddoc.h"
#include "neutubeconfig.h"
#include "flyem/zflyemexternalneurondoc.h"
#include "zfiletype.h"
#include "z3dpunctafilter.h"
#include "z3dswcfilter.h"
#include "dvid/zdvidsynapseensenmble.h"
#include "dvid/zdvidsparsevolslice.h"
#include "flyem/zflyemorthowindow.h"
#include "zroiwidget.h"
#include "flyem/zflyemdataframe.h"
#include "flyem/zflyemtodolistfilter.h"
#include "dialogs/flyemtododialog.h"
#include "zclickablelabel.h"
#include "znormcolormap.h"
#include "widgets/zcolorlabel.h"
#include "dialogs/zflyemsynapseannotationdialog.h"
#include "zflyemorthodoc.h"
#include "flyem/zflyemsynapsedatafetcher.h"
#include "flyem/zflyemsynapsedataupdater.h"
#include "dialogs/zflyemroitooldialog.h"
#include "flyem/zflyemroiproject.h"
#include "zflyemutilities.h"
#include "zflyembookmarkview.h"
#include "dvid/zdvidpatchdatafetcher.h"
#include "dvid/zdvidpatchdataupdater.h"
#include "widgets/z3dtabwidget.h"
#include "dialogs/zflyemsplituploadoptiondialog.h"
#include "dialogs/zflyembodychopdialog.h"
#include "dialogs/zinfodialog.h"
#include "zrandomgenerator.h"
#include "zinteractionevent.h"
#include "dialogs/zstresstestoptiondialog.h"
#include "dialogs/zflyemskeletonupdatedialog.h"
#include "z3dmainwindow.h"
#include "dvid/zdvidgrayslicescrollstrategy.h"
#include "dialogs/zflyemgrayscaledialog.h"
#include "zstackwriter.h"

ZFlyEmProofMvc::ZFlyEmProofMvc(QWidget *parent) :
  ZStackMvc(parent)
{
  init();
}

ZFlyEmProofMvc::~ZFlyEmProofMvc()
{
  if (getDvidTarget().isValid()) {
    LINFO() << "End using " << getDvidTarget().getUuid() << "@"
            << getDvidTarget().getAddressWithPort();
  }

  exitCurrentDoc();

  closeAllAssociatedWindow();
//  m_bodyViewWindow->close();
//  delete m_coarseBodyWindow;
//  delete m_bodyWindow;
//  delete m_splitWindow;
}

void ZFlyEmProofMvc::init()
{
  setFocusPolicy(Qt::ClickFocus);

  m_dvidDlg = NULL;
  m_bodyInfoDlg = new FlyEmBodyInfoDialog(this);
    m_protocolSwitcher = new ProtocolSwitcher(this);
//  m_supervisor = new ZFlyEmSupervisor(this);
  m_splitCommitDlg = new ZFlyEmSplitCommitDialog(this);
  m_todoDlg = new FlyEmTodoDialog(this);
  m_roiDlg = new ZFlyEmRoiToolDialog(this);
  m_splitUploadDlg = new ZFlyEmSplitUploadOptionDialog(this);
  m_bodyChopDlg = new ZFlyEmBodyChopDialog(this);
  m_infoDlg = new ZInfoDialog(this);
  m_skeletonUpdateDlg = new ZFlyEmSkeletonUpdateDialog(this);
  m_grayscaleDlg = new ZFlyEmGrayscaleDialog(this);


  connect(m_roiDlg, SIGNAL(projectActivited()), this, SLOT(loadRoiProject()));
  connect(m_roiDlg, SIGNAL(projectClosed()), this, SLOT(closeRoiProject()));
  connect(m_roiDlg, SIGNAL(showing3DRoiCurve()), this, SLOT(showRoi3dWindow()));
  connect(m_roiDlg, SIGNAL(goingToSlice(int)), this, SLOT(goToSlice(int)));
  connect(m_roiDlg, SIGNAL(steppingSlice(int)), this, SLOT(stepSlice(int)));
  connect(m_roiDlg, SIGNAL(goingToNearestRoi()), this, SLOT(goToNearestRoi()));
  connect(m_roiDlg, SIGNAL(estimatingRoi()), this, SLOT(estimateRoi()));

//  qRegisterMetaType<ZDvidTarget>("ZDvidTarget");

  initBodyWindow();
  m_objectWindow = NULL;
  m_roiWindow = NULL;
  m_orthoWindow = NULL;
//  m_queryWindow = NULL;
  m_ROILoaded = false;

  m_assignedBookmarkModel[FlyEM::PR_NORMAL] =
      new ZFlyEmBookmarkListModel(this);
  m_assignedBookmarkModel[FlyEM::PR_SPLIT] =
      new ZFlyEmBookmarkListModel(this);
  m_userBookmarkModel[FlyEM::PR_NORMAL] =
      new ZFlyEmBookmarkListModel(this);
  m_userBookmarkModel[FlyEM::PR_SPLIT] =
      new ZFlyEmBookmarkListModel(this);


//  m_assignedBookmarkList = new ZFlyEmBookmarkListModel(this);
//  m_assignedBookmarkProxy = new QSortFilterProxyModel(this);
//  prepareBookmarkModel(m_assignedBookmarkList, m_assignedBookmarkProxy);

//  m_userBookmarkList = new ZFlyEmBookmarkListModel(this);
//  m_userBookmarkProxy = new QSortFilterProxyModel(this);
//  prepareBookmarkModel(m_userBookmarkList, m_userBookmarkProxy);

  m_seFetcher = new ZFlyEmSynapseDataFetcher(this);
  m_seUpdater = new ZFlyEmSynapseDataUpdater(this);
  connect(m_seFetcher, SIGNAL(dataFetched(ZFlyEmSynapseDataFetcher*)),
          m_seUpdater, SLOT(updateData(ZFlyEmSynapseDataFetcher*)),
          Qt::QueuedConnection);

  m_dvidDlg = ZDialogFactory::makeDvidDialog(this);

//  m_testTimer = new QTimer(this);

#ifdef _DEBUG_
//  connect(m_testTimer, SIGNAL(timeout()), this, SLOT(testSlot()));
#endif
}

void ZFlyEmProofMvc::setDvidDialog(ZDvidDialog *dlg)
{
  m_dvidDlg = dlg;
}

ZDvidDialog* ZFlyEmProofMvc::getDvidDialog() const
{
  return m_dvidDlg;
}

void ZFlyEmProofMvc::initBodyWindow()
{
  m_bodyViewWindow = new Z3DMainWindow(NULL);
  m_bodyViewWindow->setWindowTitle(QString::fromUtf8("3D Body View"));
  m_bodyViewWindow->setAttribute(Qt::WA_DeleteOnClose, false);

  m_bodyViewers = new Z3DTabWidget(m_bodyViewWindow);
  m_bodyViewers->setAttribute(Qt::WA_DeleteOnClose, false);

  QSizePolicy sizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  m_bodyViewers->setSizePolicy(sizePolicy);

  QVBoxLayout* bvLayout = new QVBoxLayout;

  QLabel *messageLabel = new QLabel;
  bvLayout->addWidget(messageLabel);

  QWidget *toolWidget = new QWidget(m_bodyViewWindow->toolBar);
  bvLayout->addWidget(toolWidget);

  bvLayout->addWidget(m_bodyViewers);

  m_bodyViewWindow->setLayout(bvLayout);
  m_bodyViewWindow->setCentralWidget(m_bodyViewers);
  m_bodyViewWindow->resize(QDesktopWidget().availableGeometry(0).size()*0.7);

  connect(m_bodyViewWindow, SIGNAL(closed()),
          m_bodyViewers, SLOT(closeAllWindows()));

  m_bodyWindowFactory =
      QSharedPointer<ZWindowFactory>(new ZFlyEmBodyWindowFactory);
  m_bodyWindowFactory->setDeleteOnClose(true);
  m_bodyWindowFactory->setControlPanelVisible(false);
  m_bodyWindowFactory->setObjectViewVisible(false);
  m_bodyWindowFactory->setVisible(Z3DWindow::LAYER_PUNCTA, false);

  m_bodyViewWindow->m_stayOnTopAction =
      m_bodyViewWindow->toolBar->addAction("Pin");
  m_bodyViewWindow->m_stayOnTopAction->setCheckable(true);
   m_bodyViewWindow->m_stayOnTopAction->setChecked(false);
  connect(m_bodyViewWindow->m_stayOnTopAction, SIGNAL(triggered(bool)),
          m_bodyViewWindow, SLOT(stayOnTop(bool)));

  m_bodyViewWindow->toolBar->addSeparator();

  m_bodyViewWindow->resetCameraAction =
      m_bodyViewWindow->toolBar->addAction("X-Y View");
  connect(m_bodyViewWindow->resetCameraAction, SIGNAL(triggered()),
          m_bodyViewers, SLOT(resetCamera()));

  m_bodyViewWindow->xzViewAction =
      m_bodyViewWindow->toolBar->addAction("X-Z View");
  connect(m_bodyViewWindow->xzViewAction, SIGNAL(triggered()),
          m_bodyViewers, SLOT(setXZView()));

  m_bodyViewWindow->yzViewAction =
      m_bodyViewWindow->toolBar->addAction("Y-Z View");
  connect(m_bodyViewWindow->yzViewAction, SIGNAL(triggered()),
          m_bodyViewers, SLOT(setYZView()));

  m_bodyViewWindow->recenterAction = m_bodyViewWindow->toolBar->addAction("Center");
  connect(m_bodyViewWindow->recenterAction, SIGNAL(triggered()),
          m_bodyViewers, SLOT(resetCameraCenter()));

  m_bodyViewWindow->toolBar->addSeparator();

  m_bodyViewWindow->showGraphAction = m_bodyViewWindow->toolBar->addAction("Graph");
  connect(m_bodyViewWindow->showGraphAction, SIGNAL(toggled(bool)),
          m_bodyViewers, SLOT(showGraph(bool)));
  m_bodyViewWindow->showGraphAction->setCheckable(true);
  m_bodyViewWindow->showGraphAction->setChecked(true);

  m_bodyViewWindow->settingsAction = m_bodyViewWindow->toolBar->addAction("ControlSettings");
  connect(m_bodyViewWindow->settingsAction, SIGNAL(toggled(bool)),
          m_bodyViewers, SLOT(settingsPanel(bool)));
  m_bodyViewWindow->settingsAction->setCheckable(true);
  m_bodyViewWindow->settingsAction->setChecked(false);

  m_bodyViewWindow->objectsAction = m_bodyViewWindow->toolBar->addAction("Objects");
  connect(m_bodyViewWindow->objectsAction, SIGNAL(toggled(bool)),
          m_bodyViewers, SLOT(objectsPanel(bool)));
  m_bodyViewWindow->objectsAction->setCheckable(true);
  m_bodyViewWindow->objectsAction->setChecked(false);

  m_bodyViewWindow->roiAction = m_bodyViewWindow->toolBar->addAction("ROIs");
  connect(m_bodyViewWindow->roiAction, SIGNAL(toggled(bool)),
          m_bodyViewers, SLOT(roiPanel(bool)));
  m_bodyViewWindow->roiAction->setCheckable(true);
  m_bodyViewWindow->roiAction->setChecked(false);

  //update button status reversely
  connect(m_bodyViewers, SIGNAL(buttonShowGraphToggled(bool)),
          m_bodyViewWindow, SLOT(updateButtonShowGraph(bool)));
  connect(m_bodyViewers, SIGNAL(buttonSettingsToggled(bool)),
          m_bodyViewWindow, SLOT(updateButtonSettings(bool)));
  connect(m_bodyViewers, SIGNAL(buttonObjectsToggled(bool)),
          m_bodyViewWindow, SLOT(updateButtonObjects(bool)));
  connect(m_bodyViewers, SIGNAL(buttonROIsToggled(bool)),
          m_bodyViewWindow, SLOT(updateButtonROIs(bool)));
  connect(m_bodyViewers, SIGNAL(buttonROIsClicked()),
          this, SLOT(getROIs()));

  connect(m_bodyViewers, SIGNAL(currentChanged(int)), m_bodyViewers, SLOT(updateTabs(int)));

  //
  m_coarseBodyWindow = NULL;
  m_externalNeuronWindow = NULL;
  m_bodyWindow = NULL;
  m_skeletonWindow = NULL;
  m_splitWindow = NULL;
}

ZFlyEmProofMvc* ZFlyEmProofMvc::Make(
    QWidget *parent, ZSharedPointer<ZFlyEmProofDoc> doc, NeuTube::EAxis axis)
{
  ZFlyEmProofMvc *frame = new ZFlyEmProofMvc(parent);

  BaseConstruct(frame, doc, axis);

  frame->getView()->setHoverFocus(true);

  return frame;
}

ZFlyEmProofMvc* ZFlyEmProofMvc::Make(const ZDvidTarget &target)
{
  ZFlyEmProofDoc *doc = new ZFlyEmProofDoc;
//  doc->setTag(NeuTube::Document::FLYEM_DVID);
  ZFlyEmProofMvc *mvc =
      ZFlyEmProofMvc::Make(NULL, ZSharedPointer<ZFlyEmProofDoc>(doc));
  mvc->getPresenter()->setObjectStyle(ZStackObject::SOLID);
  mvc->setDvidTarget(target);

  connect(mvc->getPresenter(), SIGNAL(orthoViewTriggered(double,double,double)),
          mvc, SLOT(showOrthoWindow(double,double,double)));
  connect(mvc->getDocument().get(), SIGNAL(updatingLatency(int)),
          mvc, SLOT(updateLatencyWidget(int)));
  connect(mvc->getView(), SIGNAL(sliceSliderPressed()),
          mvc, SLOT(suppressObjectVisible()));
  connect(mvc->getView(), SIGNAL(sliceSliderReleased()),
          mvc, SLOT(recoverObjectVisible()));
  connect(mvc, SIGNAL(roiLoaded()), mvc, SLOT(updateRoiWidget()));
  connect(mvc->getCompleteDocument(), SIGNAL(synapseVerified(int,int,int,bool)),
          mvc->m_protocolSwitcher, SLOT(processSynapseVerification(int, int, int, bool)));
  connect(mvc->getCompleteDocument(), SIGNAL(synapseMoved(ZIntPoint,ZIntPoint)),
          mvc->m_protocolSwitcher, SLOT(processSynapseMoving(ZIntPoint,ZIntPoint)));
  connect(mvc->getCompleteDocument(), SIGNAL(bodySelectionChanged()),
          mvc, SLOT(syncBodySelectionToOrthoWindow()));

  return mvc;
}

void ZFlyEmProofMvc::detachOrthoWindow()
{
  m_orthoWindow = NULL;
}

void ZFlyEmProofMvc::detachCoarseBodyWindow()
{
  m_coarseBodyWindow = NULL;
}

void ZFlyEmProofMvc::detachBodyWindow()
{
  m_bodyWindow = NULL;
}

void ZFlyEmProofMvc::detachSplitWindow()
{
  m_splitWindow = NULL;
}

void ZFlyEmProofMvc::detachSkeletonWindow()
{
  m_skeletonWindow = NULL;
}

void ZFlyEmProofMvc::detachObjectWindow()
{
  m_objectWindow = NULL;
}

void ZFlyEmProofMvc::detachRoiWindow()
{
  m_roiWindow = NULL;
}

void ZFlyEmProofMvc::detachExternalNeuronWindow()
{
  m_externalNeuronWindow = NULL;
}

void ZFlyEmProofMvc::detachQueryWindow()
{
//  m_queryWindow = NULL;
}

void ZFlyEmProofMvc::registerBookmarkView(ZFlyEmBookmarkView *view)
{
  connect(view, SIGNAL(bookmarkChecked(QString,bool)),
          this, SLOT(recordCheckedBookmark(QString,bool)));
  connect(view, SIGNAL(bookmarkChecked(ZFlyEmBookmark*)),
          this, SLOT(recordBookmark(ZFlyEmBookmark*)));
  connect(view, SIGNAL(removingBookmark(ZFlyEmBookmark*)),
          this, SLOT(removeBookmark(ZFlyEmBookmark*)));
  connect(view, SIGNAL(removingBookmark(QList<ZFlyEmBookmark*>)),
          this, SLOT(removeBookmark(QList<ZFlyEmBookmark*>)));
}

void ZFlyEmProofMvc::exportGrayscale()
{
  if (m_grayscaleDlg->exec()) {
    QString fileName =
        ZDialogFactory::GetSaveFileName("Save Grayscale", "", this);
    if (!fileName.isEmpty()) {
      exportGrayscale(m_grayscaleDlg->getBoundBox(), fileName);
    }
  }
}

void ZFlyEmProofMvc::exportGrayscale(
    const ZIntCuboid &box, const QString &fileName)
{
  ZStack *stack =
      getCompleteDocument()->getDvidReader().readGrayScale(box);

  if (stack != NULL) {
    stack->save(fileName.toStdString());
  }

  delete stack;
}

void ZFlyEmProofMvc::exportNeuronScreenshot(
    const std::vector<uint64_t> &bodyIdArray, int width, int height,
    const QString &outDir)
{
  showSkeletonWindow();
  glm::vec3 eye = m_skeletonWindow->getCamera()->getEye();
  float nearDist = m_skeletonWindow->getCamera()->getNearDist();
  glm::vec3 upVector = m_skeletonWindow->getCamera()->getUpVector();

  std::vector<uint64_t> skippedBodyIdArray;
  for (std::vector<uint64_t>::const_iterator iter = bodyIdArray.begin();
       iter != bodyIdArray.end(); ++iter) {
    uint64_t bodyId = *iter;

    if (locateBody(bodyId)) {
      ZFlyEmBody3dDoc *doc =
          qobject_cast<ZFlyEmBody3dDoc*>(m_skeletonWindow->getDocument());
      doc->waitForAllEvent();
      QApplication::processEvents();

      //  m_skeletonWindow->getCamera()->setProjectionType(Z3DCamera::Orthographic);

      //    QString outDir = (GET_TEST_DATA_DIR + "/flyem/FIB/FIB19/screenshots").c_str();
      m_skeletonWindow->getCamera()->setEye(eye);
      m_skeletonWindow->getCamera()->setUpVector(upVector);
      m_skeletonWindow->getCamera()->setNearDist(nearDist);
      //  double eyeDist = eye[0];
      m_skeletonWindow->takeScreenShot(
            QString("%1/%2_yz.tif").arg(outDir).arg(bodyId), width, height, MonoView);

      m_skeletonWindow->getCamera()->rotate(-glm::radians(90.f), glm::vec3(0, 0, 1));
      //  m_skeletonWindow->setXZView();
      //  eye = m_skeletonWindow->getCamera()->getEye();
      //  eye[1] = m_skeletonWindow->getCamera()->getCenter()[1] - eyeDist;
      //  m_skeletonWindow->getCamera()->setEye(eye);
      m_skeletonWindow->takeScreenShot(
            QString("%1/%2_xz.tif").arg(outDir).arg(bodyId), width, height, MonoView);

      m_skeletonWindow->getCamera()->rotate(-glm::radians(90.f), glm::vec3(1, 0, 0));
      m_skeletonWindow->takeScreenShot(
            QString("%1/%2_xy.tif").arg(outDir).arg(bodyId), width, height, MonoView);
    } else {
      skippedBodyIdArray.push_back(bodyId);
    }
  }

  emit messageGenerated(
        ZWidgetMessage(
          QString("Screenshots created for %1 bodies; %2 bodies skipped").
          arg(bodyIdArray.size() - skippedBodyIdArray.size()).
          arg(skippedBodyIdArray.size()), NeuTube::MSG_INFORMATION));

}

void ZFlyEmProofMvc::setWindowSignalSlot(Z3DWindow *window)
{
  if (window != NULL) {
    if (window == m_coarseBodyWindow) {
      connect(window, SIGNAL(destroyed()), this,
              SLOT(detachCoarseBodyWindow()));
      connect(window, SIGNAL(croppingSwcInRoi()),
              this, SLOT(cropCoarseBody3D()));
    } else if (window == m_bodyWindow) {
      connect(window, SIGNAL(destroyed()), this, SLOT(detachBodyWindow()));
    } else if (window == m_splitWindow) {
      connect(window, SIGNAL(destroyed()), this, SLOT(detachSplitWindow()));
    } else if (window == m_externalNeuronWindow) {
      connect(window, SIGNAL(destroyed()),
              this, SLOT(detachExternalNeuronWindow()));
    } else if (window == m_skeletonWindow) {
      connect(window, SIGNAL(destroyed()), this, SLOT(detachSkeletonWindow()));
    } else if (window == m_objectWindow) {
      connect(window, SIGNAL(destroyed()), this, SLOT(detachObjectWindow()));
    } else if (window == m_roiWindow) {
      connect(window, SIGNAL(destroyed()), this, SLOT(detachRoiWindow()));
    }
    connect(window, SIGNAL(locating2DViewTriggered(int, int, int, int)),
            this, SLOT(zoomTo(int, int, int, int)));
  }
}

ZFlyEmBody3dDoc* ZFlyEmProofMvc::makeBodyDoc(FlyEM::EBodyType bodyType)
{
  ZFlyEmBody3dDoc *doc = new ZFlyEmBody3dDoc;
  doc->setDvidTarget(getDvidTarget());
  doc->setDataDoc(m_doc);
  doc->setBodyType(bodyType);
/*
  connect(&m_mergeProject, SIGNAL(mergeUploaded(QSet<uint64_t>)),
          doc, SLOT(setUnrecycable(QSet<uint64_t>)));
          */

  connect(getCompleteDocument(), SIGNAL(bodyMergeUploaded()),
          this, SLOT(updateBodyWindowDeep()));
  connect(getCompleteDocument(), SIGNAL(bodyMergeUploaded()),
          this, SLOT(updateCoarseBodyWindowDeep()));
  connect(getCompleteDocument(), SIGNAL(bodyMergeUploaded()),
          this, SLOT(updateBookmarkTable()));

  connect(getCompleteDocument(), SIGNAL(bodyMergeUploadedExternally()),
          this, SLOT(updateBodyWindowDeep()));
  connect(getCompleteDocument(), SIGNAL(bodyMergeUploadedExternally()),
          this, SLOT(updateCoarseBodyWindowDeep()));
  connect(getCompleteDocument(), SIGNAL(bodyMergeUploadedExternally()),
          this, SLOT(updateBookmarkTable()));

  ZWidgetMessage::ConnectMessagePipe(doc, this, false);

  return doc;
}

void ZFlyEmProofMvc::syncBodySelectionToOrthoWindow()
{
  if (m_orthoWindow != NULL) {
    m_orthoWindow->getDocument()->setSelectedBody(
          getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL),
          NeuTube::BODY_LABEL_ORIGINAL);
  }
}

void ZFlyEmProofMvc::syncBodySelectionFromOrthoWindow()
{
  if (m_orthoWindow != NULL) {
    getCompleteDocument()->setSelectedBody(
          m_orthoWindow->getDocument()->getSelectedBodySet(
            NeuTube::BODY_LABEL_ORIGINAL),
          NeuTube::BODY_LABEL_ORIGINAL);
  }
}


void ZFlyEmProofMvc::makeOrthoWindow()
{
  m_orthoWindow = new ZFlyEmOrthoWindow(getDvidTarget());
  connect(m_orthoWindow, SIGNAL(destroyed()), this, SLOT(detachOrthoWindow()));
  connect(m_orthoWindow, SIGNAL(bookmarkEdited(int, int, int)),
          getCompleteDocument(), SLOT(downloadBookmark(int,int,int)));
  connect(getCompleteDocument(), SIGNAL(bookmarkEdited(int,int,int)),
          m_orthoWindow, SLOT(downloadBookmark(int, int, int)));
  connect(m_orthoWindow, SIGNAL(synapseEdited(int, int, int)),
          getCompleteDocument(), SLOT(downloadSynapse(int,int,int)));
  connect(getCompleteDocument(), SIGNAL(synapseEdited(int,int,int)),
          m_orthoWindow, SLOT(downloadSynapse(int, int, int)));
  connect(m_orthoWindow, SIGNAL(synapseVerified(int,int,int,bool)),
          getCompleteDocument(), SIGNAL(synapseVerified(int,int,int,bool)));

  connect(m_orthoWindow, SIGNAL(synapseVerified(int,int,int,bool)),
          this, SLOT(processSynapseVerification(int,int,int,bool)));
  connect(getCompleteDocument(), SIGNAL(synapseVerified(int,int,int,bool)),
          m_orthoWindow, SLOT(downloadSynapse(int,int,int)));

  connect(m_orthoWindow->getDocument(), SIGNAL(synapseMoved(ZIntPoint,ZIntPoint)),
          this, SLOT(processSynapseMoving(ZIntPoint,ZIntPoint)));
  connect(getCompleteDocument(), SIGNAL(synapseMoved(ZIntPoint,ZIntPoint)),
          m_orthoWindow->getDocument(), SLOT(syncMoveSynapse(ZIntPoint,ZIntPoint)));

  connect(m_orthoWindow->getDocument(), SIGNAL(bodySelectionChanged()),
          this, SLOT(syncBodySelectionFromOrthoWindow()));


//  connect(m_orthoWindow, SIGNAL(synapseEdited(int,int,int)),
//          this, SIGNAL())
  connect(getCompleteDocument(), SIGNAL(todoEdited(int,int,int)),
          m_orthoWindow, SLOT(downloadTodo(int, int, int)));
  connect(m_orthoWindow, SIGNAL(todoEdited(int,int,int)),
          getCompleteDocument(), SLOT(downloadTodo(int,int,int)));
  connect(m_orthoWindow, SIGNAL(zoomingTo(int,int,int)),
          this, SLOT(zoomTo(int,int,int)));
  connect(m_orthoWindow, SIGNAL(bodyMergeEdited()),
          getCompleteDocument(), SLOT(syncMergeWithDvid()));
  connect(getCompleteDocument(), SIGNAL(bodyMergeEdited()),
          m_orthoWindow, SLOT(syncMergeWithDvid()));
  m_orthoWindow->copyBookmarkFrom(getCompleteDocument());

  connect(getCompleteDocument(), SIGNAL(bodyMergeUploaded()),
          m_orthoWindow->getDocument(), SLOT(processExternalBodyMergeUpload()));
  connect(m_orthoWindow->getDocument(), SIGNAL(bodyMergeUploaded()),
          getCompleteDocument(), SLOT(processExternalBodyMergeUpload()));

  syncBodySelectionToOrthoWindow();
}

void ZFlyEmProofMvc::prepareBodyWindowSignalSlot(
    Z3DWindow *window, ZFlyEmBody3dDoc *doc)
{
  connect(window->getTodoFilter(), SIGNAL(visibleChanged(bool)),
          doc, SLOT(showTodo(bool)));
  connect(window->getPunctaFilter(), SIGNAL(visibleChanged(bool)),
          doc, SLOT(showSynapse(bool)));
  connect(window, SIGNAL(addingTodoMarker(int,int,int,bool,uint64_t)),
          getCompleteDocument(),
          SLOT(executeAddTodoItemCommand(int,int,int,bool,uint64_t)));
  connect(window, SIGNAL(addingToMergeMarker(int,int,int,uint64_t)),
          getCompleteDocument(),
          SLOT(executeAddToMergeItemCommand(int,int,int,uint64_t)));
  connect(window, SIGNAL(addingToSplitMarker(int,int,int,uint64_t)),
          getCompleteDocument(),
          SLOT(executeAddToSplitItemCommand(int,int,int,uint64_t)));
  connect(window, SIGNAL(deselectingBody(std::set<uint64_t>)),
          getCompleteDocument(),
          SLOT(deselectMappedBodyWithOriginalId(std::set<uint64_t>)));
  connect(m_bodyWindow, SIGNAL(settingNormalTodoVisible(bool)),
          doc, SLOT(setNormalTodoVisible(bool)));
  connect(doc, SIGNAL(todoVisibleChanged()),
          m_bodyWindow, SLOT(updateTodoVisibility()));

}

void ZFlyEmProofMvc::makeCoarseBodyWindow()
{
  ZFlyEmBody3dDoc *doc = makeBodyDoc(FlyEM::BODY_COARSE);
  m_coarseBodyWindow = m_bodyWindowFactory->make3DWindow(doc);
  doc->showSynapse(m_coarseBodyWindow->isLayerVisible(Z3DWindow::LAYER_PUNCTA));
  doc->showTodo(m_coarseBodyWindow->isLayerVisible(Z3DWindow::LAYER_TODO));

//  connect(m_coarseBodyWindow->getPunctaFilter(), SIGNAL(visibleChanged(bool)),
//          doc, SLOT(showSynapse(bool)));
//  connect(m_coarseBodyWindow->getTodoFilter(), SIGNAL(visibleChanged(bool)),
//          doc, SLOT(showTodo(bool)));
  setWindowSignalSlot(m_coarseBodyWindow);
  prepareBodyWindowSignalSlot(m_coarseBodyWindow, doc);

  m_coarseBodyWindow->setWindowType(NeuTube3D::TYPE_COARSE_BODY);
  m_coarseBodyWindow->readSettings();

  if (m_doc->getParentMvc() != NULL) {
    ZFlyEmMisc::Decorate3dBodyWindow(
          m_coarseBodyWindow, getGrayScaleInfo(),
          m_doc->getParentMvc()->getView()->getViewParameter());
    if(m_ROILoaded) {
      m_coarseBodyWindow->getROIsDockWidget()->getROIs(
            m_coarseBodyWindow, getGrayScaleInfo(), m_roiList,
            m_loadedROIs, m_roiSourceList);
    }
  }
}

void ZFlyEmProofMvc::makeBodyWindow()
{
  ZFlyEmBody3dDoc *doc = makeBodyDoc(FlyEM::BODY_FULL);
  m_bodyWindow = m_bodyWindowFactory->make3DWindow(doc);
  doc->showSynapse(m_bodyWindow->isLayerVisible(Z3DWindow::LAYER_PUNCTA));
  doc->showTodo(m_bodyWindow->isLayerVisible(Z3DWindow::LAYER_TODO));


  prepareBodyWindowSignalSlot(m_bodyWindow, doc);

  setWindowSignalSlot(m_bodyWindow);

  m_bodyWindow->setWindowType(NeuTube3D::TYPE_BODY);
  m_bodyWindow->readSettings();

  if (m_doc->getParentMvc() != NULL) {
    ZFlyEmMisc::Decorate3dBodyWindow(
          m_bodyWindow, getGrayScaleInfo(),
          m_doc->getParentMvc()->getView()->getViewParameter());
    if(m_ROILoaded)
        m_bodyWindow->getROIsDockWidget()->getROIs(
              m_bodyWindow, getGrayScaleInfo(), m_roiList, m_loadedROIs,
              m_roiSourceList);
  }
}

void ZFlyEmProofMvc::makeSkeletonWindow()
{
  ZFlyEmBody3dDoc *doc = makeBodyDoc(FlyEM::BODY_SKELETON);

  m_skeletonWindow = m_bodyWindowFactory->make3DWindow(doc);
  doc->showSynapse(m_skeletonWindow->isLayerVisible(Z3DWindow::LAYER_PUNCTA));

  connect(m_skeletonWindow->getPunctaFilter(), SIGNAL(visibleChanged(bool)),
          doc, SLOT(showSynapse(bool)));
  setWindowSignalSlot(m_skeletonWindow);

  m_skeletonWindow->setWindowType(NeuTube3D::TYPE_SKELETON);
  m_skeletonWindow->readSettings();

  if (m_doc->getParentMvc() != NULL) {
    ZFlyEmMisc::Decorate3dBodyWindow(
          m_skeletonWindow, getGrayScaleInfo(),
          m_doc->getParentMvc()->getView()->getViewParameter());
    if(m_ROILoaded)
        m_skeletonWindow->getROIsDockWidget()->getROIs(
              m_skeletonWindow, getGrayScaleInfo(), m_roiList, m_loadedROIs,
              m_roiSourceList);
  }
}

void ZFlyEmProofMvc::makeExternalNeuronWindow()
{
  ZFlyEmExternalNeuronDoc *doc = new ZFlyEmExternalNeuronDoc;
  doc->setDataDoc(m_doc);
  ZWidgetMessage::ConnectMessagePipe(doc, this, false);

  m_externalNeuronWindow = m_bodyWindowFactory->make3DWindow(doc);
  setWindowSignalSlot(m_externalNeuronWindow);

  if (m_doc->getParentMvc() != NULL) {
    ZFlyEmMisc::Decorate3dBodyWindow(
          m_externalNeuronWindow, getGrayScaleInfo(),
          m_doc->getParentMvc()->getView()->getViewParameter());

    if(m_ROILoaded)
        m_externalNeuronWindow->getROIsDockWidget()->getROIs(
              m_externalNeuronWindow, getGrayScaleInfo(), m_roiList,
              m_loadedROIs, m_roiSourceList);
  }
}

void ZFlyEmProofMvc::mergeCoarseBodyWindow()
{
  if (m_coarseBodyWindow != NULL) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    ZFlyEmBody3dDoc *doc =
        qobject_cast<ZFlyEmBody3dDoc*>(m_coarseBodyWindow->getDocument());
    if (doc != NULL){
      doc->mergeBodyModel(*(getCompleteDocument()->getBodyMerger()));
    }
  }
}

void ZFlyEmProofMvc::updateCoarseBodyWindow()
{
  if (m_coarseBodyWindow != NULL) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    ZFlyEmBody3dDoc *doc =
        qobject_cast<ZFlyEmBody3dDoc*>(m_coarseBodyWindow->getDocument());
    if (doc != NULL){
      doc->addBodyChangeEvent(bodySet.begin(), bodySet.end());
    }
  }

//  updateCoarseBodyWindow(false, false, false);
}

void ZFlyEmProofMvc::updateCoarseBodyWindowDeep()
{
  if (m_coarseBodyWindow != NULL) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    ZFlyEmBody3dDoc *doc =
        qobject_cast<ZFlyEmBody3dDoc*>(m_coarseBodyWindow->getDocument());
    if (doc != NULL){
      doc->beginObjectModifiedMode(ZStackDoc::OBJECT_MODIFIED_CACHE);
      doc->dumpAllBody(false);
      doc->addBodyChangeEvent(bodySet.begin(), bodySet.end());
      doc->processEventFunc();
//      doc->processEvent();
      doc->endObjectModifiedMode();
      doc->notifyObjectModified();
    }
  }
//  updateCoarseBodyWindow(false, false, true);
}

void ZFlyEmProofMvc::updateBodyWindow()
{
  if (m_bodyWindow != NULL) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    ZFlyEmBody3dDoc *doc =
        qobject_cast<ZFlyEmBody3dDoc*>(m_bodyWindow->getDocument());
    if (doc != NULL){
      doc->addBodyChangeEvent(bodySet.begin(), bodySet.end());
    }
  }
}

void ZFlyEmProofMvc::updateBodyWindowDeep()
{
  if (m_bodyWindow != NULL) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    ZFlyEmBody3dDoc *doc =
        qobject_cast<ZFlyEmBody3dDoc*>(m_bodyWindow->getDocument());
    if (doc != NULL){
      doc->beginObjectModifiedMode(ZStackDoc::OBJECT_MODIFIED_CACHE);
      doc->dumpAllBody(false);
      doc->addBodyChangeEvent(bodySet.begin(), bodySet.end());
//      doc->processEventFunc();
//      doc->processEvent();
      doc->endObjectModifiedMode();
      doc->notifyObjectModified();
    }
  }
#if 0
  if (m_bodyWindow != NULL) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    ZFlyEmBody3dDoc *doc =
        qobject_cast<ZFlyEmBody3dDoc*>(m_bodyWindow->getDocument());
    if (doc != NULL){
      doc->addBodyChangeEvent(bodySet.begin(), bodySet.end());
    }
  }
#endif
}

void ZFlyEmProofMvc::updateSkeletonWindow()
{
  if (m_skeletonWindow != NULL) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    ZFlyEmBody3dDoc *doc =
        qobject_cast<ZFlyEmBody3dDoc*>(m_skeletonWindow->getDocument());
    if (doc != NULL){
      doc->addBodyChangeEvent(bodySet.begin(), bodySet.end());
    }
  }
}

void ZFlyEmProofMvc::updateCoarseBodyWindowColor()
{
  updateCoarseBodyWindow();
#if 0
  if (m_coarseBodyWindow != NULL) {
    std::set<std::string> currentBodySourceSet;
    std::set<uint64_t> selectedMapped =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_MAPPED);

    for (std::set<uint64_t>::const_iterator iter = selectedMapped.begin();
         iter != selectedMapped.end(); ++iter) {
      currentBodySourceSet.insert(
            ZStackObjectSourceFactory::MakeFlyEmBodySource(*iter));
    }

    m_coarseBodyWindow->getDocument()->beginObjectModifiedMode(
          ZStackDoc::OBJECT_MODIFIED_CACHE);

    ZDvidLabelSlice *labelSlice = getCompleteDocument()->getDvidLabelSlice();
    QList<ZSwcTree*> bodyList = m_coarseBodyWindow->getDocument()->getSwcList();
    for (QList<ZSwcTree*>::iterator iter = bodyList.begin();
         iter != bodyList.end(); ++iter) {
      ZSwcTree *tree = *iter;

      std::vector<uint64_t> labelArray = ZString(tree->getSource()).toUint64Array();
      if (!labelArray.empty()) {
        uint64_t label = labelArray.back();
        QColor color = labelSlice->getColor(label, NeuTube::BODY_LABEL_ORIGINAL);
        color.setAlpha(255);
        tree->setColor(color);
        m_coarseBodyWindow->getDocument()->processObjectModified(tree);
      }
    }

    m_coarseBodyWindow->getDocument()->endObjectModifiedMode();
    m_coarseBodyWindow->getDocument()->notifyObjectModified();
  }
#endif
}

void ZFlyEmProofMvc::updateCoarseBodyWindow(
    bool showingWindow, bool resettingCamera, bool isDeep)
{
  if (m_coarseBodyWindow != NULL) {
    std::set<std::string> currentBodySourceSet;
    std::set<uint64_t> selectedMapped =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_MAPPED);

    for (std::set<uint64_t>::const_iterator iter = selectedMapped.begin();
         iter != selectedMapped.end(); ++iter) {
      currentBodySourceSet.insert(
            ZStackObjectSourceFactory::MakeFlyEmCoarseBodySource(*iter));
    }

    m_coarseBodyWindow->getDocument()->beginObjectModifiedMode(
          ZStackDoc::OBJECT_MODIFIED_CACHE);

    if (isDeep) {
      m_coarseBodyWindow->getDocument()->removeAllSwcTree(true);
    }

    std::set<std::string> oldBodySourceSet;
    QList<ZSwcTree*> bodyList = m_coarseBodyWindow->getDocument()->getSwcList();
    for (QList<ZSwcTree*>::iterator iter = bodyList.begin();
         iter != bodyList.end(); ++iter) {
      ZSwcTree *tree = *iter;
      if (currentBodySourceSet.count(tree->getSource()) == 0) {
        m_coarseBodyWindow->getDocument()->removeObject(
              dynamic_cast<ZStackObject*>(tree), true);
      } else {
        oldBodySourceSet.insert(tree->getSource());
      }
    }

    ZDvidReader &reader = getCompleteDocument()->getDvidReader();
    if (reader.isReady()) {
      for (std::set<uint64_t>::const_iterator iter = selectedMapped.begin();
           iter != selectedMapped.end(); ++iter) {
        uint64_t label = *iter;
        std::string source =
            ZStackObjectSourceFactory::MakeFlyEmCoarseBodySource(label);
        if (oldBodySourceSet.count(source) == 0) {
          ZObject3dScan body;

          QList<uint64_t> bodyList = getCompleteDocument()->getMergedSource(label);
          //        bodyList.append(label);

          for (int i = 0; i < bodyList.size(); ++i) {
            body.concat(reader.readCoarseBody(bodyList[i]));
          }

          if (!body.isEmpty()) {
            ZDvidLabelSlice *labelSlice =
                getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
            if (labelSlice != NULL) {
              body.setColor(labelSlice->getLabelColor(
                              label, NeuTube::BODY_LABEL_MAPPED));
            }

            body.setAlpha(255);
            ZSwcTree *tree = ZSwcGenerator::createSurfaceSwc(body);
            tree->translate(-getGrayScaleInfo().getStartBlockIndex());
            tree->rescale(getGrayScaleInfo().getBlockSize().getX(),
                          getGrayScaleInfo().getBlockSize().getY(),
                          getGrayScaleInfo().getBlockSize().getZ());
            tree->translate(getGrayScaleInfo().getStartCoordinates());
            tree->setSource(source);
            m_coarseBodyWindow->getDocument()->addObject(tree, true);
          }
        }
      }
    }
//    m_bodyWindow->getDocument()->blockSignals(false);
//    m_bodyWindow->getDocument()->notifySwcModified();
    m_coarseBodyWindow->getDocument()->endObjectModifiedMode();
    m_coarseBodyWindow->getDocument()->notifyObjectModified();

    if (showingWindow) {
      m_bodyViewWindow->show();
      m_bodyViewWindow->raise();
    }

    if (resettingCamera) {
      m_coarseBodyWindow->resetCameraCenter();
    }
  }
}

void ZFlyEmProofMvc::updateBodyWindowPlane(
    Z3DWindow *window, const ZStackViewParam &viewParam)
{
  if (window != NULL) {
    ZFlyEmMisc::Decorate3dBodyWindowPlane(window, getGrayScaleInfo(), viewParam);
  }
}

ZFlyEmProofDoc* ZFlyEmProofMvc::getCompleteDocument() const
{
  return qobject_cast<ZFlyEmProofDoc*>(getDocument().get());
}

ZFlyEmProofPresenter* ZFlyEmProofMvc::getCompletePresenter() const
{
  return qobject_cast<ZFlyEmProofPresenter*>(getPresenter());
}

void ZFlyEmProofMvc::mergeSelected()
{
  if (getCompleteDocument() != NULL) {
    getCompleteDocument()->mergeSelected(getSupervisor());
  }
}

void ZFlyEmProofMvc::mergeSelectedWithoutConflict()
{
  if (getCompleteDocument() != NULL) {
    getCompleteDocument()->mergeSelectedWithoutConflict(getSupervisor());
  }
}


void ZFlyEmProofMvc::unmergeSelected()
{
  if (getCompleteDocument() != NULL) {
    getCompleteDocument()->unmergeSelected();
  }
}

void ZFlyEmProofMvc::undo()
{
  if (getCompleteDocument() != NULL) {
    getCompleteDocument()->undoStack()->undo();
  }
}

void ZFlyEmProofMvc::redo()
{
  if (getCompleteDocument() != NULL) {
    getCompleteDocument()->undoStack()->redo();
  }
}

void ZFlyEmProofMvc::setSegmentationVisible(bool visible)
{
  m_showSegmentation = visible;
  if (getCompleteDocument() != NULL) {
    QList<ZDvidLabelSlice*> sliceList =
        getCompleteDocument()->getDvidLabelSliceList();
    foreach (ZDvidLabelSlice *slice, sliceList) {
      slice->setVisible(visible);
      if (visible) {
        slice->update(getView()->getViewParameter(NeuTube::COORD_STACK));
      }
    }
  }
  getView()->redrawObject();
}

void ZFlyEmProofMvc::clear()
{
  if (getCompleteDocument() != NULL) {
    getCompleteDocument()->clearData();
    getPresenter()->clearData();
//    getView()->imageWidget();
  }
}

void ZFlyEmProofMvc::exitCurrentDoc()
{
  if (getCompleteDocument() != NULL) {
//    getCompleteDocument()->saveCustomBookmark();
    getCompleteDocument()->saveMergeOperation();
  }
}

/*
void ZFlyEmProofMvc::syncMergeWithDvid()
{
  m_mergeProject.syncWithDvid();
}
*/

void ZFlyEmProofMvc::setDvidTargetFromDialog()
{
  getProgressSignal()->startProgress("Loading data ...");
  setDvidTarget(m_dvidDlg->getDvidTarget());
  getProgressSignal()->endProgress();
}

void ZFlyEmProofMvc::enableSynapseFetcher()
{
  ZDvidSynapseEnsemble *se =
      getCompleteDocument()->getDvidSynapseEnsemble(getView()->getSliceAxis());
  if (se != NULL) {
    se->attachView(getView());
    m_seFetcher->setDvidTarget(getDvidTarget());
    m_seUpdater->setData(se, m_doc);
    se->setDataFetcher(m_seFetcher);
  }
}

const ZDvidInfo& ZFlyEmProofMvc::getGrayScaleInfo() const
{
  return getCompleteDocument()->getGrayScaleInfo();
}

void ZFlyEmProofMvc::setLabelAlpha(int alpha)
{
  getView()->setDynamicObjectAlpha(alpha);
  getView()->paintDynamicObjectBuffer();
  getView()->updateImageScreen(ZStackView::UPDATE_QUEUED);
//  getCompletePresenter()->setLabelAlpha(alpha);
//  getCompleteDocument()->setLabelSliceAlpha(alpha);
}

void ZFlyEmProofMvc::prepareTile(ZDvidTileEnsemble *te)
{
  te->setContrastProtocal(getPresenter()->getHighContrastProtocal());
  te->enhanceContrast(getCompletePresenter()->highTileContrast());
  te->attachView(getView());
  ZDvidPatchDataFetcher *patchFetcher = new ZDvidPatchDataFetcher(this);
  ZDvidPatchDataUpdater *patchUpdater = new ZDvidPatchDataUpdater(this);
  patchFetcher->setDvidTarget(getDvidTarget());
  patchUpdater->setData(te, getDocument());
  connect(patchFetcher, SIGNAL(dataFetched(ZDvidPatchDataFetcher*)),
          patchUpdater, SLOT(updateData(ZDvidPatchDataFetcher*)),
          Qt::QueuedConnection);
  te->setDataFetcher(patchFetcher);
  patchFetcher->start(100);
}

void ZFlyEmProofMvc::setDvidTarget(const ZDvidTarget &target)
{
  if (getCompleteDocument() == NULL) {
    emit messageGenerated(
          ZWidgetMessage("Corrupted data structure. Abort",
                         NeuTube::MSG_WARNING,
                         ZWidgetMessage::TARGET_DIALOG));
    return;
  }

  if (getCompleteDocument()->getDvidTarget().isValid()) {
    emit messageGenerated(
          ZWidgetMessage("You cannot change the database in this window. "
                         "Please open a new proofread window to load a different database",
                         NeuTube::MSG_WARNING,
                         ZWidgetMessage::TARGET_DIALOG));
    return;
  }

  ZDvidReader reader;
  if (!reader.open(target)) {
    emit messageGenerated(
          ZWidgetMessage("Failed to open the database.",
                         NeuTube::MSG_WARNING,
                         ZWidgetMessage::TARGET_DIALOG));
    return;
  }

  exitCurrentDoc();

  getProgressSignal()->startProgress("Loading data ...");

  clear();
  getProgressSignal()->advanceProgress(0.1);
  //    getCompleteDocument()->clearData();
  getCompleteDocument()->setDvidTarget(reader.getDvidTarget());

  ZDvidGraySlice *slice = getCompleteDocument()->getDvidGraySlice();
  if (slice != NULL) {
    ZDvidGraySliceScrollStrategy *scrollStrategy =
        new ZDvidGraySliceScrollStrategy;
    scrollStrategy->setGraySlice(slice);

    getView()->setScrollStrategy(scrollStrategy);
  }

  ZJsonObject contrastObj = reader.readContrastProtocal();
  getPresenter()->setHighContrastProtocal(contrastObj);

  //    getCompleteDocument()->beginObjectModifiedMode(
  //          ZStackDoc::OBJECT_MODIFIED_CACHE);
  //    getCompleteDocument()->updateTileData();
  //    getCompleteDocument()->endObjectModifiedMode();
  QList<ZDvidTileEnsemble*> teList =
      getCompleteDocument()->getDvidTileEnsembleList();
  foreach (ZDvidTileEnsemble *te, teList) {
    prepareTile(te);
  }

  getView()->reset(false);
  getProgressSignal()->advanceProgress(0.1);

  m_splitProject.setDvidTarget(getDvidTarget());
  getCompleteDocument()->syncMergeWithDvid();
  //    m_mergeProject.setDvidTarget(getDvidTarget());
  //    m_mergeProject.syncWithDvid();
  m_splitUploadDlg->setDvidTarget(getDvidTarget());

  getProgressSignal()->advanceProgress(0.2);


  if (getDvidTarget().isValid()) {
    getCompleteDocument()->downloadSynapse();
    enableSynapseFetcher();
    getCompleteDocument()->downloadBookmark();
    getCompleteDocument()->downloadTodoList();
  }

  getProgressSignal()->advanceProgress(0.5);

  emit dvidTargetChanged(getDvidTarget());


  m_roiDlg->clear();
  m_roiDlg->updateDvidTarget();
  m_roiDlg->downloadAllProject();

  getProgressSignal()->advanceProgress(0.1);

  getProgressSignal()->endProgress();

  emit messageGenerated(
        ZWidgetMessage(
          QString("Database %1 loaded.").arg(
            getDvidTarget().getSourceString(false).c_str()),
          NeuTube::MSG_INFORMATION,
          ZWidgetMessage::TARGET_STATUS_BAR));
}

void ZFlyEmProofMvc::diagnose()
{
  emit messageGenerated(
        ZWidgetMessage("Start diagnosing...", NeuTube::MSG_INFORMATION));

  emit messageGenerated(
        ZWidgetMessage(getDvidTarget().toJsonObject().dumpString(2),
                       NeuTube::MSG_INFORMATION));

  QList<ZDvidTileEnsemble*> teList =
      getCompleteDocument()->getDvidTileEnsembleList();
  emit messageGenerated(
        ZWidgetMessage(QString("%1 tile objects").arg(teList.size())));
  foreach (ZDvidTileEnsemble *te, teList) {
    emit messageGenerated(
          ZWidgetMessage(QString("  Tile axis %1: %2").
                         arg(te->getSource().c_str()).arg(te->getSliceAxis())));
    emit messageGenerated(
          ZWidgetMessage("  Contrast: " + te->getContrastProtocal().dumpString(2)));
    emit messageGenerated(
          ZWidgetMessage(QString("  %1 receiver(s) for data fetcher").arg(
                           te->getDataFetcher()->receiverCount(
                             SIGNAL(dataFetched(ZDvidPatchDataFetcher*))))));
//    te->getDataFetcher()->dumpObjectInfo();
  }

  QList<ZDvidSynapseEnsemble*> seList =
      getCompleteDocument()->getDvidSynapseEnsembleList();
  foreach (ZDvidSynapseEnsemble *se, seList) {
    emit messageGenerated(
          ZWidgetMessage(QString("Synapse axis %1: %2").
                         arg(se->getSource().c_str()).arg(se->getSliceAxis())));
  }
}

void ZFlyEmProofMvc::setDvidTarget()
{
//  m_dvidDlg = new ZDvidDialog(this);
  if (m_dvidDlg == NULL) {
    m_dvidDlg = ZDialogFactory::makeDvidDialog(this);
  }

  if (m_dvidDlg->exec()) {
    const ZDvidTarget &target = m_dvidDlg->getDvidTarget();
    setDvidTarget(target);
    /*
    const QString threadId = "setDvidTarget";
    if (!m_futureMap.isAlive(threadId)) {
      m_futureMap.removeDeadThread();
      QFuture<void> future =
          QtConcurrent::run(this, &ZFlyEmProofMvc::setDvidTargetFromDialog);
      m_futureMap[threadId] = future;
    }
    */


  }
}

ZDvidTarget ZFlyEmProofMvc::getDvidTarget() const
{
  if (m_dvidDlg != NULL) {
    return getCompleteDocument()->getDvidTarget();
//    return m_dvidDlg->getDvidTarget();
  }

  return ZDvidTarget();
}

void ZFlyEmProofMvc::createPresenter()
{
  if (getDocument().get() != NULL) {
    m_presenter = ZFlyEmProofPresenter::Make(this);
  }
}

void ZFlyEmProofMvc::customInit()
{
  connect(getPresenter(), SIGNAL(bodySplitTriggered()),
          this, SLOT(notifySplitTriggered()));
  connect(getPresenter(), SIGNAL(bodyAnnotationTriggered()),
          this, SLOT(annotateBody()));
  connect(getPresenter(), SIGNAL(bodyCheckinTriggered()),
          this, SLOT(checkInSelectedBody()));
  connect(getPresenter(), SIGNAL(bodyForceCheckinTriggered()),
          this, SLOT(checkInSelectedBodyAdmin()));
  connect(getPresenter(), SIGNAL(bodyCheckoutTriggered()),
          this, SLOT(checkOutBody()));
  connect(getPresenter(), SIGNAL(objectVisibleTurnedOn()),
          this, SLOT(processViewChange()));
  connect(getCompletePresenter(), SIGNAL(goingToTBar()),
          this, SLOT(goToTBar()));
  connect(getCompletePresenter(), SIGNAL(goingToBody()),
          this, SLOT(goToBody()));
  connect(getCompletePresenter(), SIGNAL(goingToBodyBottom()),
          this, SLOT(goToBodyBottom()));
  connect(getCompletePresenter(), SIGNAL(goingToBodyTop()),
          this, SLOT(goToBodyTop()));
  connect(getCompletePresenter(), SIGNAL(selectingBody()),
          this, SLOT(selectBody()));
  connect(getCompletePresenter(), SIGNAL(selectingBodyInRoi(bool)),
          this, SLOT(selectBodyInRoi(bool)));
  connect(getCompletePresenter(), SIGNAL(selectingBodyInRoi()),
          this, SLOT(selectBodyInRoi()));
  connect(getCompletePresenter(), SIGNAL(bodyDecomposeTriggered()),
          this, SLOT(decomposeBody()));
  connect(getCompletePresenter(), SIGNAL(bodyCropTriggered()),
          this, SLOT(cropBody()));
//  connect(getCompletePresenter(), SIGNAL(bodyChopTriggered()),
//          this, SLOT(chopBodyZ()));
  connect(getCompletePresenter(), SIGNAL(bodyChopTriggered()),
            this, SLOT(chopBody()));
  connect(getCompletePresenter(), SIGNAL(bodyMergeTriggered()),
          this, SLOT(mergeSelected()));
  connect(getCompletePresenter(), SIGNAL(bodyUnmergeTriggered()),
          this, SLOT(unmergeSelected()));
  //  connect(getCompletePresenter(), SIGNAL(labelSliceSelectionChanged()),
//          this, SLOT(processLabelSliceSelectionChange()));

  connect(getDocument().get(), SIGNAL(activeViewModified()),
          this, SLOT(updateActiveViewData()));
  connect(getCompleteDocument(), SIGNAL(bodyMerged()),
          getView(), SLOT(paintObject()));
  connect(getCompleteDocument(), SIGNAL(bodyUnmerged()),
          getView(), SLOT(paintObject()));
//  connect(getCompleteDocument(), SIGNAL(bodyMerged()),
//          &m_mergeProject, SLOT(update3DBodyViewDeep()));
//  connect(getCompleteDocument(), SIGNAL(bodyUnmerged()),
//          &m_mergeProject, SLOT(update3DBodyViewDeep()));

  connect(getCompleteDocument(), SIGNAL(bodyMerged()),
          this, SLOT(updateCoarseBodyWindowColor()));
  connect(getCompleteDocument(), SIGNAL(bodyUnmerged()),
          this, SLOT(updateCoarseBodyWindowColor()));
  connect(getCompleteDocument(), SIGNAL(bodyMergeEdited()),
          this, SLOT(notifyBodyMergeEdited()));

//  connect(getCompleteDocument(), SIGNAL(bodyMerged()),
//          getCompleteDocument(), SLOT(saveMergeOperation()));
//  connect(getCompleteDocument(), SIGNAL(bodyUnmerged()),
//          getCompleteDocument(), SLOT(saveMergeOperation()));

  connect(getCompleteDocument(), SIGNAL(userBookmarkModified()),
          this, SLOT(updateUserBookmarkTable()));
  connect(getCompleteDocument(), SIGNAL(assignedBookmarkModified()),
          this, SLOT(updateAssignedBookmarkTable()));

  connect(getCompleteDocument(), SIGNAL(bodyIsolated(uint64_t)),
          this, SLOT(checkInBodyWithMessage(uint64_t)));
  connect(getCompleteDocument(), SIGNAL(requestingBodyLock(uint64_t,bool)),
          this, SLOT(checkBodyWithMessage(uint64_t,bool)));
  /*
  connect(this, SIGNAL(splitBodyLoaded(uint64_t)),
          getCompleteDocument(), SLOT(deprecateSplitSource()));
          */

//  m_mergeProject.getProgressSignal()->connectProgress(getProgressSignal());
  m_splitProject.getProgressSignal()->connectProgress(getProgressSignal());


  connect(getCompleteDocument(), SIGNAL(bodyUnmerged()),
          getView(), SLOT(paintObject()));
  connect(getDocument().get(),
          SIGNAL(objectSelectorChanged(ZStackObjectSelector)),
          this, SLOT(processSelectionChange(ZStackObjectSelector)));
  connect(getCompleteDocument(), SIGNAL(bodySelectionChanged()),
          this, SLOT(updateBodySelection()));


  m_splitProject.setDocument(getDocument());
  connect(&m_splitProject, SIGNAL(locating2DViewTriggered(int, int, int, int)),
          this->getView(), SLOT(setView(int, int, int, int)));
  connect(&m_splitProject, SIGNAL(resultCommitted()),
          this, SLOT(updateSplitBody()));
  /*
  connect(&m_splitProject, SIGNAL(messageGenerated(QString, bool)),
          this, SIGNAL(messageGenerated(QString, bool)));
          */

//  m_mergeProject.setDocument(getDocument());
  connect(getPresenter(), SIGNAL(labelSliceSelectionChanged()),
          this, SLOT(updateBodySelection()));
  connect(getCompletePresenter(), SIGNAL(highlightingSelected(bool)),
          this, SLOT(highlightSelectedObject(bool)));
//          &m_mergeProject, SLOT(highlightSelectedObject(bool)));


  /*
  connect(&m_mergeProject, SIGNAL(messageGenerated(QString, bool)),
          this, SIGNAL(messageGenerated(QString,bool)));
  connect(getDocument().get(), SIGNAL(messageGenerated(QString, bool)),
          this, SIGNAL(messageGenerated(QString, bool)));
          */

  ZWidgetMessage::ConnectMessagePipe(&m_splitProject, this, false);
//  ZWidgetMessage::ConnectMessagePipe(&m_mergeProject, this, false);
//  ZWidgetMessage::ConnectMessagePipe(&getDocument().get(), this, false);


  connect(this, SIGNAL(splitBodyLoaded(uint64_t)),
          this, SLOT(presentBodySplit(uint64_t)));

  connect(getCompletePresenter(), SIGNAL(selectingBodyAt(int,int,int)),
          this, SLOT(xorSelectionAt(int, int, int)));
  connect(getCompletePresenter(), SIGNAL(deselectingAllBody()),
          this, SLOT(deselectAllBody()));
  connect(getCompletePresenter(), SIGNAL(runningSplit()), this, SLOT(runSplit()));
  connect(getCompletePresenter(), SIGNAL(runningLocalSplit()),
          this, SLOT(runLocalSplit()));
//  connect(getCompletePresenter(), SIGNAL(bookmarkAdded(ZFlyEmBookmark*)),
//          this, SLOT(annotateBookmark(ZFlyEmBookmark*)));
  connect(getCompletePresenter(), SIGNAL(annotatingBookmark(ZFlyEmBookmark*)),
          this, SLOT(annotateBookmark(ZFlyEmBookmark*)));
  connect(getCompletePresenter(), SIGNAL(annotatingSynapse()),
          this, SLOT(annotateSynapse()));
  connect(getCompletePresenter(), SIGNAL(mergingBody()),
          this, SLOT(mergeSelected()));
  connect(getCompletePresenter(), SIGNAL(uploadingMerge()),
          this, SLOT(commitMerge()));
//  connect(getCompletePresenter(), SIGNAL(goingToBody()), this, SLOT());

  disableSplit();


  // connections to body info dialog (aka "sequencer")
  if (m_bodyInfoDlg != NULL) {
    connect(m_bodyInfoDlg, SIGNAL(bodyActivated(uint64_t)),
            this, SLOT(locateBody(uint64_t)));
    connect(m_bodyInfoDlg, SIGNAL(addBodyActivated(uint64_t)),
            this, SLOT(addLocateBody(uint64_t)));
    connect(m_bodyInfoDlg, SIGNAL(bodiesActivated(QList<uint64_t>)),
            this, SLOT(selectBody(QList<uint64_t>)));
    connect(this, SIGNAL(dvidTargetChanged(ZDvidTarget)),
            m_bodyInfoDlg, SLOT(dvidTargetChanged(ZDvidTarget)));
    connect(m_bodyInfoDlg, SIGNAL(dataChanged(ZJsonValue)),
            this, SLOT(prepareBodyMap(ZJsonValue)));
    connect(m_bodyInfoDlg, SIGNAL(colorMapChanged(ZFlyEmSequencerColorScheme)),
            getCompleteDocument(),
            SLOT(updateSequencerBodyMap(ZFlyEmSequencerColorScheme)));
    connect(m_bodyInfoDlg, SIGNAL(pointDisplayRequested(int,int,int)),
            this, SLOT(zoomTo(int,int,int)));
  }

  // connections to protocols
  connect(this, SIGNAL(dvidTargetChanged(ZDvidTarget)),
          m_protocolSwitcher, SLOT(dvidTargetChanged(ZDvidTarget)));
  connect(m_protocolSwitcher, SIGNAL(requestDisplayPoint(int,int,int)),
          this, SLOT(zoomToL1(int,int,int)));

  /*
  QPushButton *button = new QPushButton(this);
  button->setCheckable(true);
  button->setChecked(true);
  button->setIcon(QIcon(":/images/synapse.png"));
  connect(button, SIGNAL(toggled(bool)),
          this, SLOT(showSynapseAnnotation(bool)));
          */

//  getView()->addHorizontalWidget(button);

//  getView()->addHorizontalWidget(ZWidgetFactory::makeHSpacerItem());
  /*
  m_latencyLabelWidget =
      ZWidgetFactory::MakeColorLabel(Qt::gray, "Seg Latency", 100, false, this);
  getView()->addHorizontalWidget(ZWidgetFactory::MakeHSpacerItem());
  getView()->addHorizontalWidget(m_latencyLabelWidget);
  */

  m_paintLabelWidget = new ZPaintLabelWidget();

  getView()->addHorizontalWidget(m_paintLabelWidget);
  m_paintLabelWidget->hide();

//  m_speedLabelWidget->hide();

  m_todoDlg->setDocument(getDocument());
}

void ZFlyEmProofMvc::prepareBodyMap(const ZJsonValue &bodyInfoObj)
{
  getCompleteDocument()->prepareNameBodyMap(bodyInfoObj);

  emit nameColorMapReady(true);
}

void ZFlyEmProofMvc::goToBodyBottom()
{
  ZDvidReader &reader = getCompleteDocument()->getDvidReader();
  if (reader.isReady()) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    if (!bodySet.empty()) {
      ZIntPoint pt;
      std::set<uint64_t>::const_iterator iter = bodySet.begin();
      pt = reader.readBodyBottom(*iter);
      ++iter;
      for (; iter != bodySet.end(); ++iter) {
        uint64_t bodyId = *iter;
        ZIntPoint tmpPt = reader.readBodyBottom(bodyId);
        if (pt.getZ() < tmpPt.getZ()) {
          pt = tmpPt;
        }
      }
      zoomTo(pt.getX(), pt.getY(), pt.getZ());
    }
  }
}

void ZFlyEmProofMvc::goToBodyTop()
{
  ZDvidReader &reader = getCompleteDocument()->getDvidReader();
  if (reader.isReady()) {
    std::set<uint64_t> bodySet =
        getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);
    if (!bodySet.empty()) {
      ZIntPoint pt;
      std::set<uint64_t>::const_iterator iter = bodySet.begin();
      pt = reader.readBodyTop(*iter);
      ++iter;
      for (; iter != bodySet.end(); ++iter) {
        uint64_t bodyId = *iter;
        ZIntPoint tmpPt = reader.readBodyTop(bodyId);
        if (pt.getZ() > tmpPt.getZ()) {
          pt = tmpPt;
        }
      }
      zoomTo(pt.getX(), pt.getY(), pt.getZ());
    }
  }
}

void ZFlyEmProofMvc::updateLatencyWidget(int t)
{
  emit updatingLatency(t);
  /*
  ZNormColorMap colorMap;
  int baseTime = 600;
  double v = (double) t / baseTime;
  QColor color = colorMap.mapColor(v);
  color.setAlpha(100);
  m_latencyLabelWidget->setColor(color);
  m_latencyLabelWidget->setText(QString("%1").arg(t));
  */
}

void ZFlyEmProofMvc::goToBody()
{
  bool ok;

  QString text = QInputDialog::getText(this, tr("Go To"),
                                       tr("Body:"), QLineEdit::Normal,
                                       "", &ok);
  if (ok) {
    if (!text.isEmpty()) {
      ZString str = text.toStdString();
      std::vector<uint64_t> bodyArray = str.toUint64Array();
      if (bodyArray.size() == 1) {
        locateBody(bodyArray[0]);
//        emit locatingBody();
      }
    }
  }
}

void ZFlyEmProofMvc::selectBody()
{
  bool ok;

  QString text = QInputDialog::getText(this, tr("Select"),
                                       tr("Select Body:"), QLineEdit::Normal,
                                       "", &ok);
  if (ok) {
    if (!text.isEmpty()) {
      ZString str = text.toStdString();
      std::vector<uint64_t> bodyArray = str.toUint64Array();
      if (!bodyArray.empty()) {
        getCompleteDocument()->recordBodySelection();
        getCompleteDocument()->selectBody(bodyArray.begin(), bodyArray.end());
        getCompleteDocument()->processBodySelection();
        updateBodySelection();
      }
#if 0
      for (std::vector<uint64_t>::const_iterator iter = bodyArray.begin();
           iter != bodyArray.end(); ++iter) {
        selectBody(*iter);
      }
#endif
    }
  }
}

/*
void ZFlyEmProofMvc::updateDvidLabelObject()
{
  ZFlyEmProofDoc *doc = getCompleteDocument();
  NeuTube::EAxis axis = getView()->getSliceAxis();
  doc->updateDvidLabelObject(axis);
  doc->cleanBodyAnnotationMap();
}
*/

void ZFlyEmProofMvc::highlightSelectedObject(
    ZDvidLabelSlice *labelSlice, bool hl)
{
  if (labelSlice != NULL) {
    ZFlyEmProofDoc *doc = getCompleteDocument();

    if ((labelSlice->isVisible() == false) && (hl == false)) {
      labelSlice->setVisible(true);
      if (labelSlice->getSliceAxis() == getView()->getSliceAxis()) {
        labelSlice->update(getView()->getViewParameter());
      }
    }
//    m_mergeProject.highlightSelectedObject(hl);

    doc->beginObjectModifiedMode(ZStackDoc::OBJECT_MODIFIED_CACHE);

    ZOUT(LTRACE(), 5) << "Toggle highlight";
    TStackObjectList objList =
        doc->getObjectList(ZStackObject::TYPE_DVID_SPARSEVOL_SLICE);

    for (TStackObjectList::iterator iter = objList.begin();
         iter != objList.end(); ++iter) {
      ZStackObject *obj = *iter;
      if (obj->getSliceAxis() == labelSlice->getSliceAxis()) {
        doc->removeObject(obj, true);
      }
    }
//    doc->removeObject(ZStackObject::TYPE_DVID_SPARSEVOL_SLICE, true);

    bool usingSparseVol =
        getCompleteDocument()->getDvidTarget().hasBodyLabel() &&
        getDocument()->getStack()->getVoxelNumber(ZStack::SINGLE_PLANE) > 300 * 300;
    if (getDvidTarget().usingMulitresBodylabel()) {
      usingSparseVol = false;
    }

    if (hl) {
      if (usingSparseVol) {
        labelSlice->setVisible(!hl);
        const std::set<uint64_t> &selected = labelSlice->getSelectedOriginal();
        for (std::set<uint64_t>::const_iterator iter = selected.begin();
             iter != selected.end(); ++iter) {
          uint64_t bodyId = *iter;
          ZDvidSparsevolSlice *obj = doc->makeDvidSparsevol(labelSlice, bodyId);
          obj->update(getView()->getViewParameter());
          /*
          ZDvidSparsevolSlice *obj = new ZDvidSparsevolSlice;
          obj->setTarget(ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS);
          obj->setSliceAxis(labelSlice->getSliceAxis());
          obj->setReader(doc->getSparseVolReader());
//          obj->setDvidTarget(getDvidTarget());
          obj->setLabel(bodyId);
          obj->setRole(ZStackObjectRole::ROLE_ACTIVE_VIEW);
          obj->setColor(labelSlice->getLabelColor(
                          bodyId, NeuTube::BODY_LABEL_ORIGINAL));
                          */
          doc->addObject(obj);
        }
      } else {
        labelSlice->addVisualEffect(
              NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED);
        labelSlice->paintBuffer();
        doc->processObjectModified(labelSlice);
      }
    } else {
      labelSlice->removeVisualEffect(
            NeuTube::Display::LabelField::VE_HIGHLIGHT_SELECTED);
      if (usingSparseVol) {
        labelSlice->paintBuffer();
        doc->notifyActiveViewModified();
      } else {
        labelSlice->paintBuffer();
        doc->processObjectModified(labelSlice);
      }
    }
    doc->endObjectModifiedMode();
    doc->notifyObjectModified();

  }
}

void ZFlyEmProofMvc::highlightSelectedObject(bool hl)
{
  ZFlyEmProofDoc *doc = getCompleteDocument();
  NeuTube::EAxis axis = getView()->getSliceAxis();

  ZDvidLabelSlice *labelSlice = doc->getDvidLabelSlice(axis);

  highlightSelectedObject(labelSlice, hl);

//  emit highlightModeEnabled(hl);
}

void ZFlyEmProofMvc::processLabelSliceSelectionChange()
{
  ZDvidLabelSlice *labelSlice =
      getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
  if (labelSlice != NULL){
    std::vector<uint64_t> selected =
        labelSlice->getSelector().getSelectedList();
    if (selected.size() > 0) {
      ZDvidReader &reader = getCompleteDocument()->getDvidReader();
      if (reader.isReady()) {
        ZFlyEmBodyAnnotation finalAnnotation;
        for (std::vector<uint64_t>::const_iterator iter = selected.begin();
             iter != selected.end(); ++iter) {
          uint64_t bodyId = *iter;
          ZFlyEmBodyAnnotation annotation = reader.readBodyAnnotation(bodyId);

          if (!annotation.isEmpty()) {
            getCompleteDocument()->recordAnnotation(bodyId, annotation);
            if (finalAnnotation.isEmpty()) {
              finalAnnotation = annotation;
            } else {
              finalAnnotation.mergeAnnotation(annotation);
            }
          }
        }

        ZWidgetMessage msg("", NeuTube::MSG_INFORMATION,
                           ZWidgetMessage::TARGET_CUSTOM_AREA);
        if (finalAnnotation.isEmpty()) {
          msg.setMessage(QString("%1 is not annotated.").arg(selected.front()));
        } else {
          msg.setMessage(finalAnnotation.toString().c_str());
        }

        if (finalAnnotation.isEmpty()) {
          msg.setMessage(QString("%1 is not annotated.").arg(selected.front()));
        } else {
          msg.setMessage(finalAnnotation.toString().c_str());
        }
        emit messageGenerated(msg);
      }

    }

    std::vector<uint64_t> deselected =
        labelSlice->getSelector().getDeselectedList();
//    std::set<uint64_t> mappedSet;
//    for (std::vector<uint64_t>::const_iterator iter = deselected.begin();
//         iter != deselected.end(); ++iter) {
//      mappedSet.insert(getMappedBodyId(*iter));
//    }
    getCompleteDocument()->removeSelectedAnnotation(
          deselected.begin(), deselected.end());
  }

#ifdef _DEBUG_
  getCompleteDocument()->verifyBodyAnnotationMap();
#endif

}


void ZFlyEmProofMvc::processSelectionChange(const ZStackObjectSelector &selector)
{
  const std::vector<ZStackObject*>& objList =
      selector.getSelectedList(ZStackObject::TYPE_FLYEM_BOOKMARK);
  if (!objList.empty()) {
    const ZStackObject *obj = objList.back();
    const ZFlyEmBookmark *bookmark = dynamic_cast<const ZFlyEmBookmark*>(obj);
    if (bookmark != NULL) {
      emit messageGenerated(
            ZWidgetMessage(bookmark->toJsonObject(true).dumpString(0).c_str(),
                           NeuTube::MSG_INFORMATION,
                           ZWidgetMessage::TARGET_STATUS_BAR));
    }
  } else {
    emit messageGenerated(
          ZWidgetMessage("---",
                         NeuTube::MSG_INFORMATION,
                         ZWidgetMessage::TARGET_STATUS_BAR));
  }
}

void ZFlyEmProofMvc::runSplitFunc()
{
  getProgressSignal()->startProgress(1.0);
  m_splitProject.runSplit();
  getProgressSignal()->endProgress();
}

void ZFlyEmProofMvc::runLocalSplitFunc()
{
  getProgressSignal()->startProgress(1.0);
  m_splitProject.runLocalSplit();
  getProgressSignal()->endProgress();
}

void ZFlyEmProofMvc::runLocalSplit()
{
  runLocalSplitFunc();
}

void ZFlyEmProofMvc::runSplit()
{
//  getProgressSignal()->startProgress("Running split ...");
  runSplitFunc();
//  getProgressSignal()->endProgress();

#if 0 //has strange crash caused by Make_Stack
  const QString threadId = "runSplit";
  if (!m_futureMap.isAlive(threadId)) {
    m_futureMap.removeDeadThread();
    QFuture<void> future =
        QtConcurrent::run(
          this, &ZFlyEmProofMvc::runSplitFunc);
    m_futureMap[threadId] = future;
  }
#endif
//  m_splitProject.runSplit();
}

void ZFlyEmProofMvc::updateBodySelection()
{
  if (getCompleteDocument() != NULL) {
//    ZDvidLabelSlice *slice =
//        getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
//    const std::set<uint64_t> &selected = slice->getSelectedOriginal();
//    getCompleteDocument()->getMergeProject()->setSelection(
//          selected, NeuTube::BODY_LABEL_ORIGINAL);
    updateCoarseBodyWindow();
    updateBodyWindow();
    updateSkeletonWindow();
//    m_mergeProject.update3DBodyView();
    getCompleteDocument()->beginObjectModifiedMode(ZStackDoc::OBJECT_MODIFIED_CACHE);
    ZDvidLabelSlice *tmpSlice = getCompleteDocument()->getDvidLabelSlice(
          getView()->getSliceAxis());
    if (tmpSlice != NULL) {
      if (getCompletePresenter()->isHighlight()) {
        highlightSelectedObject(tmpSlice, true);
      } else {
        tmpSlice->paintBuffer();
      }
      getCompleteDocument()->processObjectModified(tmpSlice, true);
    }
    /*
    QList<ZDvidLabelSlice*> sliceList =
        getCompleteDocument()->getDvidLabelSliceList();
    for (QList<ZDvidLabelSlice*>::iterator iter = sliceList.begin();
         iter != sliceList.end(); ++iter) {
      ZDvidLabelSlice *tmpSlice =*iter;
      if (getCompletePresenter()->isHighlight()) {
        highlightSelectedObject(tmpSlice, true);
      } else {
        tmpSlice->paintBuffer();
      }
      getCompleteDocument()->processObjectModified(tmpSlice, true);
    }
    */
    getCompleteDocument()->endObjectModifiedMode();
    getCompleteDocument()->notifyObjectModified();
    processLabelSliceSelectionChange();
  }
}

bool ZFlyEmProofMvc::checkInBody(uint64_t bodyId)
{
  if (getSupervisor() != NULL) {
    return getSupervisor()->checkIn(bodyId);
  }

  return true;
}

uint64_t ZFlyEmProofMvc::getRandomBodyId(ZRandomGenerator &rand, ZIntPoint *pos)
{
  uint64_t bodyId = 0;
  int minX = getGrayScaleInfo().getMinX();
  int minY = getGrayScaleInfo().getMinY();
  int minZ = getGrayScaleInfo().getMinZ();
  int maxX = getGrayScaleInfo().getMaxX();
  int maxY = getGrayScaleInfo().getMaxY();
  int maxZ = getGrayScaleInfo().getMaxZ();

  int x = 0;
  int y = 0;
  int z = 0;

  while (bodyId == 0) {
    x = rand.rndint(minX, maxX);
    y = rand.rndint(minY, maxY);
    z = rand.rndint(minZ, maxZ);

    bodyId = getCompleteDocument()->getDvidReader().readBodyIdAt(x, y, z);
  }

  if (pos != NULL) {
    pos->set(x, y, z);
  }

  return bodyId;
}

void ZFlyEmProofMvc::testBodySplit()
{
  static ZRandomGenerator rand;

  //If currently it's neither on the split mode nor entering split
  //  Enter split
  //If it's on the split mode
  //  If no split is running
  //   if there are seeds there, exit
  //       Else try to generate seeds and run split
  //
  if (!getCompletePresenter()->isSplitOn()) {
    const QString threadId = "launchSplitFunc";
    if (!m_futureMap.isAlive(threadId)) {
      ZIntPoint pos;
      uint64_t bodyId = getRandomBodyId(rand, &pos);

      //  zoomTo(pos);
      locateBody(bodyId, false);
      if (m_bodyWindow != NULL) {
        m_bodyWindow->updateBody();
      }

      if (getCompleteDocument()->isSplittable(bodyId)) {
        launchSplit(bodyId);
      }
    }
  } else {
    if (!getCompleteDocument()->isSplitRunning()) {
      if (getDocument()->getObjectList(ZStackObjectRole::ROLE_SEED).isEmpty()) {
        ZDvidSparseStack *sparseStack =
            getCompleteDocument()->getDvidSparseStack();
        if (sparseStack != NULL) {
          ZObject3dScan *mask = sparseStack->getObjectMask();
          if (mask != NULL) {
            std::vector<ZStroke2d*> seedList =
                ZFlyEmMisc::MakeSplitSeedList(*mask);
            for (std::vector<ZStroke2d*>::iterator iter = seedList.begin();
                 iter != seedList.end(); ++iter) {
              getDocument()->addObject(*iter);
            }
          }

          runSplit();
        }
      } else {
        emit exitingSplit();
      }
    }
  }
}

void ZFlyEmProofMvc::testBodyMerge()
{
  static ZRandomGenerator rand;

  ZIntPoint pos;
  uint64_t bodyId = getRandomBodyId(rand, &pos);

  if (rand.rndint(10) % 2 ==0) {
    zoomTo(pos);
  } else {
    bool appending = true;
    if (bodyId % 9 == 0) {
//      getCompleteDocument()->deselectAllObject();
      appending = false;
    }

    if (bodyId % 13 == 0) {
      if (m_bodyWindow != NULL) {
        m_bodyWindow->updateBody();
      }
    }

    if (rand.rndint(10000) % 17 == 0) {
      getCompletePresenter()->toggleHighlightMode();
    }

    locateBody(bodyId, appending);

    mergeSelectedWithoutConflict();

//    std::vector<uint64_t> bodyArray;
//    bodyArray.push_back(bodyId);
//    if (!bodyArray.empty()) {
//      getCompleteDocument()->recordBodySelection();
//      getCompleteDocument()->selectBody(bodyArray.begin(), bodyArray.end());
//      ZInteractionEvent interactionEvent;
//      interactionEvent.setEvent(
//            ZInteractionEvent::EVENT_OBJECT3D_SCAN_SELECTED_IN_LABEL_SLICE);
//      getPresenter()->processEvent(interactionEvent);
//      getCompleteDocument()->processBodySelection();
//      updateBodySelection();
//      zoomTo(x, y, z);
//    }
  }
}

void ZFlyEmProofMvc::prepareStressTestEnv(ZStressTestOptionDialog *optionDlg)
{
  switch (optionDlg->getOption()) {
  case ZStressTestOptionDialog::OPTION_CUSTOM:
    connect(m_testTimer, SIGNAL(timeout()), this, SLOT(testSlot()));
    break;
  case ZStressTestOptionDialog::OPTION_BODY_MERGE:
    showFineBody3d();
    connect(m_testTimer, SIGNAL(timeout()), this, SLOT(testBodyMerge()));
    break;
  case ZStressTestOptionDialog::OPTION_BODY_SPLIT:
    showFineBody3d();
    showSplitQuickView();
    connect(m_testTimer, SIGNAL(timeout()), this, SLOT(testBodySplit()));
    break;
  default:
    break;
  }
}


bool ZFlyEmProofMvc::checkBodyWithMessage(uint64_t bodyId, bool checkingOut)
{
  bool succ = true;

  if (checkingOut) {
    succ = checkOutBody(bodyId);
  } else {
    succ = checkInBodyWithMessage(bodyId);
  }

  return succ;
}

bool ZFlyEmProofMvc::checkInBodyWithMessage(uint64_t bodyId)
{
  if (getSupervisor() != NULL) {
    if (bodyId > 0) {
      if (getSupervisor()->checkIn(bodyId)) {
        emit messageGenerated(QString("Body %1 is unlocked.").arg(bodyId));
        return true;
      } else {
        emit errorGenerated(QString("Failed to unlock body %1.").arg(bodyId));
      }
    }
  }

  return true;
}

bool ZFlyEmProofMvc::checkOutBody(uint64_t bodyId)
{
  if (getSupervisor() != NULL) {
    return getSupervisor()->checkOut(bodyId);
  }

  return true;
}

void ZFlyEmProofMvc::checkInSelectedBody()
{
  if (getSupervisor() != NULL) {
    std::set<uint64_t> bodyIdArray =
        getCurrentSelectedBodyId(NeuTube::BODY_LABEL_ORIGINAL);
    for (std::set<uint64_t>::const_iterator iter = bodyIdArray.begin();
         iter != bodyIdArray.end(); ++iter) {
      uint64_t bodyId = *iter;
      if (bodyId > 0) {
        if (getSupervisor()->checkIn(bodyId)) {
          emit messageGenerated(QString("Body %1 is unlocked.").arg(bodyId));
        } else {
          emit errorGenerated(QString("Failed to unlock body %1.").arg(bodyId));
        }
      }
    }
  } else {
    emit messageGenerated(QString("Body lock service is not available."));
  }
}

void ZFlyEmProofMvc::checkInSelectedBodyAdmin()
{
  if (getSupervisor() != NULL) {
    std::set<uint64_t> bodyIdArray =
        getCurrentSelectedBodyId(NeuTube::BODY_LABEL_ORIGINAL);
    for (std::set<uint64_t>::const_iterator iter = bodyIdArray.begin();
         iter != bodyIdArray.end(); ++iter) {
      uint64_t bodyId = *iter;
      if (bodyId > 0) {
        if (getSupervisor()->isLocked(bodyId)) {
          if (getSupervisor()->checkInAdmin(bodyId)) {
            emit messageGenerated(QString("Body %1 is unlocked.").arg(bodyId));
          } else {
            emit errorGenerated(QString("Failed to unlock body %1.").arg(bodyId));
          }
        } else {
          emit messageGenerated(QString("Body %1 is unlocked.").arg(bodyId));
        }
      }
    }
  } else {
    emit messageGenerated(QString("Body lock service is not available."));
  }
}

void ZFlyEmProofMvc::checkOutBody()
{
  if (getSupervisor() != NULL) {
    std::set<uint64_t> bodyIdArray =
        getCurrentSelectedBodyId(NeuTube::BODY_LABEL_ORIGINAL);
    for (std::set<uint64_t>::const_iterator iter = bodyIdArray.begin();
         iter != bodyIdArray.end(); ++iter) {
      uint64_t bodyId = *iter;
      if (bodyId > 0) {
        if (getSupervisor()->checkOut(bodyId)) {
          emit messageGenerated(QString("Body %1 is locked.").arg(bodyId));
        } else {
          std::string owner = getSupervisor()->getOwner(bodyId);
          if (owner.empty()) {
            emit messageGenerated(
                  ZWidgetMessage(
                    QString("Failed to lock body %1. Is the librarian sever (%2) ready?").
                    arg(bodyId).arg(getDvidTarget().getSupervisor().c_str()),
                    NeuTube::MSG_ERROR));
          } else {
            emit messageGenerated(
                  ZWidgetMessage(
                    QString("Failed to lock body %1 because it has been locked by %2").
                    arg(bodyId).arg(owner.c_str()), NeuTube::MSG_ERROR));
          }
        }
      }
    }
  } else {
    emit messageGenerated(QString("Body lock service is not available."));
  }
#if 0
  std::set<uint64_t> bodyIdArray =
      getCurrentSelectedBodyId(NeuTube::BODY_LABEL_MAPPED);
  if (bodyIdArray.size() == 1) {
    uint64_t bodyId = *(bodyIdArray.begin());
    if (bodyId > 0) {
      if (getSupervisor() != NULL) {
        if (getSupervisor()->checkOut(bodyId)) {
          emit messageGenerated(QString("Body %1 is locked.").arg(bodyId));
        } else {
          emit errorGenerated(QString("Failed to check out body %1.").arg(bodyId));
        }
      }
    }
  }
#endif
}

void ZFlyEmProofMvc::annotateBody()
{
  std::set<uint64_t> bodyIdArray =
      getCurrentSelectedBodyId(NeuTube::BODY_LABEL_ORIGINAL);
  if (bodyIdArray.size() == 1) {
    uint64_t bodyId = *(bodyIdArray.begin());
    if (bodyId > 0) {
      if (checkOutBody(bodyId)) {
        ZFlyEmBodyAnnotationDialog *dlg = new ZFlyEmBodyAnnotationDialog(this);
        dlg->setBodyId(bodyId);
        ZDvidReader &reader = getCompleteDocument()->getDvidReader();
        if (reader.isReady()) {
          ZFlyEmBodyAnnotation annotation = reader.readBodyAnnotation(bodyId);

          if (!annotation.isEmpty()) {
            dlg->loadBodyAnnotation(annotation);
          }
        }

        if (dlg->exec() && dlg->getBodyId() == bodyId) {
          getCompleteDocument()->annotateBody(bodyId, dlg->getBodyAnnotation());
        }

        checkInBodyWithMessage(bodyId);
      } else {
        if (getSupervisor() != NULL) {
          std::string owner = getSupervisor()->getOwner(bodyId);
          if (owner.empty()) {
//            owner = "unknown user";
            emit messageGenerated(
                  ZWidgetMessage(
                    QString("Failed to lock body %1. Is the librarian sever (%2) ready?").
                    arg(bodyId).arg(getDvidTarget().getSupervisor().c_str()),
                    NeuTube::MSG_ERROR));
          } else {
            emit messageGenerated(
                  ZWidgetMessage(
                    QString("Failed to start annotation. %1 has been locked by %2").
                    arg(bodyId).arg(owner.c_str()), NeuTube::MSG_ERROR));
          }
        }
      }
    } else {
      ZOUT(LTRACE(), 5) << "Unexpected body ID: 0";
    }
  } else {
    QString msg;
    if (getCurrentSelectedBodyId(NeuTube::BODY_LABEL_MAPPED).size() == 1) {
      msg = "The annotation cannot be done because "
          "the merged body has not be uploaded.";
    } else {
      msg = "The annotation cannot be done because "
          "one and only one body has to be selected.";
    }
    if (!msg.isEmpty()) {
      emit messageGenerated(ZWidgetMessage(msg, NeuTube::MSG_WARNING));
    }
  }


//  emit messageGenerated(
//        ZWidgetMessage("The function of annotating body is still under testing.",
//                       NeuTube::MSG_WARING));
}

void ZFlyEmProofMvc::notifySplitTriggered()
{
  const std::set<uint64_t> &selected =
      getCurrentSelectedBodyId(NeuTube::BODY_LABEL_ORIGINAL);

  if (selected.size() == 1) {
    uint64_t bodyId = *(selected.begin());

    emit launchingSplit(bodyId);
  } else {
    QString msg;
    if (getCurrentSelectedBodyId(NeuTube::BODY_LABEL_MAPPED).size() == 1) {
      msg = "The split cannot be launched because "
          "the merged body has not been uploaded.";
    } else {
      msg = "The split cannot be launched because "
          "one and only one body has to be selected.";
    }
    emit messageGenerated(ZWidgetMessage(msg, NeuTube::MSG_WARNING));
  }

  /*
  ZDvidLabelSlice *labelSlice = getCompleteDocument()->getDvidLabelSlice();

  if (labelSlice->isVisible()) {
    const std::set<uint64_t> &selected = labelSlice->getSelectedOriginal();

    if (selected.size() == 1) {
      uint64_t bodyId = *(selected.begin());

      emit launchingSplit(bodyId);
    } else {
      emit messageGenerated("The split cannot be launched because "
                            "one and only one body has to be selected.");
    }
  }
  */
}

void ZFlyEmProofMvc::exitHighlightMode()
{
  ZOUT(LINFO(), 3) << "Exiting highlight mode";

  getCompletePresenter()->setHighlightMode(false);
  highlightSelectedObject(false);
}

ZDvidSparseStack* ZFlyEmProofMvc::getCachedBodyForSplit(uint64_t bodyId)
{
  ZDvidSparseStack *body = getCompleteDocument()->getBodyForSplit();

  ZOUT(LINFO(), 3) << "Get body for split:" << body;

  if (body != NULL) {
    if (body->getLabel() != bodyId) {
      body = NULL;
    }
  }

  return body;
}

ZDvidSparseStack* ZFlyEmProofMvc::updateBodyForSplit(
    uint64_t bodyId, ZDvidReader &reader)
{
  ZOUT(LINFO(), 3) << "Reading sparse stack async:" << bodyId;
  ZDvidSparseStack *body = reader.readDvidSparseStackAsync(bodyId);

  body->setTarget(ZStackObject::TARGET_DYNAMIC_OBJECT_CANVAS);
  body->setZOrder(0);
  body->setSource(ZStackObjectSourceFactory::MakeSplitObjectSource());
//  body->setHittable(false);
  body->setHitProtocal(ZStackObject::HIT_NONE);
  body->setSelectable(false);
  ZOUT(LINFO(), 3) << "Adding body:" << body;
  getDocument()->addObject(body, true);

  return body;
}

void ZFlyEmProofMvc::launchSplitFunc(uint64_t bodyId)
{
  ZDvidReader reader;
  if (reader.open(getDvidTarget())) {
    getProgressSignal()->startProgress("Launching split ...");
    getProgressSignal()->advanceProgress(0.1);

    if (reader.hasCoarseSparseVolume(bodyId)) {
      exitHighlightMode();

      getProgressSignal()->advanceProgress(0.1);
      ZDvidSparseStack *body = getCachedBodyForSplit(bodyId);

      if (body == NULL) {
        ZOUT(LINFO(), 3) << "Reading sparse stack async:" << bodyId;
        body = updateBodyForSplit(bodyId, reader);
      }

      body->runFillValueFunc();

      m_splitProject.setBodyId(bodyId);

      ZDvidLabelSlice *labelSlice =
          getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
      ZOUT(LINFO(), 3) << "Get label slice:" << labelSlice;
      labelSlice->setVisible(false);
//      labelSlice->setHittable(false);
      labelSlice->setHitProtocal(ZStackObject::HIT_NONE);

      body->setColor(labelSlice->getLabelColor(
                       bodyId, NeuTube::BODY_LABEL_ORIGINAL));
      body->setVisible(true);
      body->setProjectionVisible(false);

      getProgressSignal()->advanceProgress(0.1);

      getCompleteDocument()->deprecateSplitSource();

      emit splitBodyLoaded(bodyId);
    } else {
      QString msg = QString("Invalid body id: %1").arg(bodyId);
      emit messageGenerated(
            ZWidgetMessage(msg, NeuTube::MSG_ERROR, ZWidgetMessage::TARGET_DIALOG));
      emit errorGenerated(msg);
    }

    //      getDocument()->setVisible(ZStackObject::TYPE_PUNCTA, true);

    getProgressSignal()->endProgress();
  }
}

void ZFlyEmProofMvc::updateBodyMerge()
{

}

void ZFlyEmProofMvc::updateSplitBody()
{
  if (m_splitProject.getBodyId() > 0) {
    getCompleteDocument()->refreshDvidLabelBuffer(2000);

    ZOUT(LINFO(), 3) << "Updating split body:" << m_splitProject.getBodyId();
    getCompleteDocument()->getBodyForSplit()->deprecateStackBuffer();
    getCompleteDocument()->deprecateSplitSource();
    /*
    QColor color =
        getCompleteDocument()->getDvidSparseStack()->getObjectMask()->getColor();
    getCompleteDocument()->getDvidSparseStack()->loadBody(
          m_splitProject.getBodyId());
    getCompleteDocument()->getDvidSparseStack()->setMaskColor(color);
    */
#if 0
    uint64_t bodyId = m_splitProject.getBodyId();
    getDocument()->removeObject(
          ZStackObjectSourceFactory::MakeSplitObjectSource(), true);
    m_splitProject.setBodyId(0);
    launchSplit(bodyId);
#endif
    if (m_coarseBodyWindow != NULL) {
      ZOUT(LINFO(), 3) << "Removing rect roi from coarse body window.";
      m_coarseBodyWindow->removeRectRoi();
      ZOUT(LINFO(), 3) << "Updating coarse body window.";
      updateCoarseBodyWindowDeep();
//      updateCoarseBodyWindow(false, false, true);
    }
    ZOUT(LINFO(), 3) << "Updating body window.";
    updateBodyWindowDeep();
  }
}

void ZFlyEmProofMvc::suppressObjectVisible()
{
  getPresenter()->suppressObjectVisible(true);
}

void ZFlyEmProofMvc::recoverObjectVisible()
{
  getPresenter()->suppressObjectVisible(false);
  getView()->processDepthSliderValueChange();
}

void ZFlyEmProofMvc::skeletonizeSelectedBody()
{
#if defined(_FLYEM_)
  ZWidgetMessage warnMsg;
  warnMsg.setType(NeuTube::MSG_WARNING);
  if (GET_NETU_SERVICE.isNormal()) {
    m_skeletonUpdateDlg->setComputingServer(
          GET_NETU_SERVICE.getServer().c_str());
    if (m_skeletonUpdateDlg->exec()) {
      const std::set<uint64_t> &bodySet =
          getCompleteDocument()->getSelectedBodySet(NeuTube::BODY_LABEL_ORIGINAL);

      if (m_skeletonUpdateDlg->isOverwriting()) {
        if (GET_FLYEM_CONFIG.getNeutuService().requestBodyUpdate(
              getDvidTarget(), bodySet, ZNeutuService::UPDATE_ALL) ==
            ZNeutuService::REQUEST_FAILED) {
          warnMsg.setMessage("Computing service failed");
        }
      } else {
        if (GET_NETU_SERVICE.requestBodyUpdate(
              getDvidTarget(), bodySet, ZNeutuService::UPDATE_MISSING) ==
            ZNeutuService::REQUEST_FAILED) {
          warnMsg.setMessage("Computing service failed");
        }
      }
    }
  } else {
    warnMsg.setMessage(
          "Skeletonization failed: The skeletonization service is not available.");
  }

  if (warnMsg.hasMessage()) {
    emit messageGenerated(warnMsg);
  }
#endif
}

void ZFlyEmProofMvc::exportSelectedBodyStack()
{
  if (m_grayscaleDlg->exec()) {
    QString fileName =
        ZDialogFactory::GetSaveFileName("Export Bodies as Stack", "", this);
    if (!fileName.isEmpty()) {
      ZDvidLabelSlice *slice =
          getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
      if (slice != NULL) {
        std::set<uint64_t> idSet =
            slice->getSelected(NeuTube::BODY_LABEL_ORIGINAL);

        ZDvidReader &reader = getCompleteDocument()->getDvidReader();
        ZDvidSparseStack *sparseStack = NULL;
        if (reader.isReady()) {
          std::set<uint64_t>::const_iterator iter = idSet.begin();
          sparseStack = reader.readDvidSparseStack(*iter);
          ++iter;
          for (; iter != idSet.end(); ++iter) {
            ZDvidSparseStack *sparseStack2 = reader.readDvidSparseStack(*iter);
            sparseStack->getSparseStack()->merge(*(sparseStack2->getSparseStack()));

            delete sparseStack2;
          }
        }

        ZStackWriter stackWriter;
        if (m_grayscaleDlg->isFullRange()) {
          stackWriter.write(fileName.toStdString(), sparseStack->getStack());
//          sparseStack->getStack()->save(fileName.toStdString());
        } else {
          ZStack *stack = sparseStack->makeStack(m_grayscaleDlg->getBoundBox());
//          stack->save(fileName.toStdString());
          stackWriter.write(fileName.toStdString(), stack);
          delete stack;
        }
        delete sparseStack;
      }
    }
  }
}

void ZFlyEmProofMvc::exportSelectedBodyLevel()
{
  if (m_grayscaleDlg->exec()) {
    QString fileName = ZDialogFactory::GetSaveFileName("Export Bodies", "", this);
    if (!fileName.isEmpty()) {
      ZDvidLabelSlice *slice =
          getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
      if (slice != NULL) {
        std::set<uint64_t> idSet =
            slice->getSelected(NeuTube::BODY_LABEL_ORIGINAL);
        //      std::set<uint64_t> idSet =
        //          m_mergeProject.getSelection(NeuTube::BODY_LABEL_ORIGINAL);

        ZObject3dScanArray objArray;
        objArray.resize(idSet.size());

        ZDvidReader &reader = getCompleteDocument()->getDvidReader();
        if (reader.isReady()) {
          int index = 0;
          for (std::set<uint64_t>::const_iterator iter = idSet.begin();
               iter != idSet.end(); ++iter) {
            ZObject3dScan &obj = objArray[index];
            reader.readBody(*iter, false, &obj);
            index++;
          }
        }

        ZStack *stack = NULL;
        if (m_grayscaleDlg->isFullRange()) {
          stack = objArray.toLabelField();
        } else {
          stack = objArray.toLabelField(m_grayscaleDlg->getBoundBox());
        }
        if (stack != NULL) {
          stack->save(fileName.toStdString());
          delete stack;
        }
      }
    }
  }
}

void ZFlyEmProofMvc::exportSelectedBody()
{
  QString fileName = ZDialogFactory::GetSaveFileName("Export Bodies", "", this);
  if (!fileName.isEmpty()) {
    ZDvidLabelSlice *slice =
        getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
    if (slice != NULL) {
      std::set<uint64_t> idSet =
          slice->getSelected(NeuTube::BODY_LABEL_ORIGINAL);
//      std::set<uint64_t> idSet =
//          m_mergeProject.getSelection(NeuTube::BODY_LABEL_ORIGINAL);
      ZObject3dScan obj;

      ZDvidReader &reader = getCompleteDocument()->getDvidReader();
      if (reader.isReady()) {
        for (std::set<uint64_t>::const_iterator iter = idSet.begin();
             iter != idSet.end(); ++iter) {
          ZObject3dScan subobj;
          reader.readBody(*iter, false, &subobj);
          obj.concat(subobj);
        }
      }
      obj.canonize();
      obj.save(fileName.toStdString());
    }
  }
}

void ZFlyEmProofMvc::clearBodyMergeStage()
{
  getCompleteDocument()->clearBodyMergeStage();
}

void ZFlyEmProofMvc::presentBodySplit(uint64_t bodyId)
{
  enableSplit();

  ZOUT(LINFO(), 3) << "Removing ROI";
  getDocument()->removeObject(ZStackObjectRole::ROLE_ROI, true);

//  m_latencyLabelWidget->hide();

  m_paintLabelWidget->show();
//  m_mergeProject.closeBodyWindow();

  m_splitProject.setBodyId(bodyId);
  m_splitProject.downloadSeed();

  updateAssignedBookmarkTable();
  updateUserBookmarkTable();

//  emit bookmarkUpdated(&m_splitProject);
  getView()->redrawObject();
}

void ZFlyEmProofMvc::enableSplit()
{
//  m_splitOn = true;
  getCompletePresenter()->enableSplit();
}

void ZFlyEmProofMvc::disableSplit()
{
//  m_splitOn = false;
  getCompletePresenter()->disableSplit();
}

void ZFlyEmProofMvc::launchSplit(uint64_t bodyId)
{
  if (bodyId > 0) {
    if (!getCompleteDocument()->isSplittable(bodyId)) {
      QString msg = QString("%1 is not ready for split. "
                            "The body could be annotated as 'Finalized' or "
                            "a merged body waiting for upload.").arg(bodyId);
      emit messageGenerated(
            ZWidgetMessage(msg, NeuTube::MSG_ERROR, ZWidgetMessage::TARGET_DIALOG));
      emit errorGenerated(msg);
    } else if (checkOutBody(bodyId)) {
#ifdef _DEBUG_2
      bodyId = 14742253;
#endif

//      launchSplitFunc(bodyId);

      const QString threadId = "launchSplitFunc";
      if (!m_futureMap.isAlive(threadId)) {
        m_futureMap.removeDeadThread();
        ZOUT(LINFO(), 3) << "Launching split func:" << bodyId;
        QFuture<void> future =
            QtConcurrent::run(
              this, &ZFlyEmProofMvc::launchSplitFunc, bodyId);
        m_futureMap[threadId] = future;
      }
    } else {
      if (getSupervisor() != NULL) {
        std::string owner = getSupervisor()->getOwner(bodyId);
        if (owner.empty()) {
          //        owner = "unknown user";
          emit messageGenerated(
                ZWidgetMessage(
                  QString("Failed to lock body %1. Is the librarian sever (%2) ready?").
                  arg(bodyId).arg(getDvidTarget().getSupervisor().c_str()),
                  NeuTube::MSG_ERROR));
        }
        emit messageGenerated(
              ZWidgetMessage(
                QString("Failed to launch split. %1 has been locked by %2").
                arg(bodyId).arg(owner.c_str()), NeuTube::MSG_ERROR));
      }
    }
  } else {
    emit errorGenerated(QString("Invalid body id: %1").arg(bodyId));
  }
}

void ZFlyEmProofMvc::exitSplit()
{
  if (getCompletePresenter()->isSplitWindow()) {
    emit messageGenerated("Exiting split ...");
//    emitMessage("Exiting split ...");
    ZDvidLabelSlice *labelSlice =
        getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
    labelSlice->setVisible(true);
    labelSlice->update(getView()->getViewParameter(NeuTube::COORD_STACK));
    labelSlice->setHitProtocal(ZStackObject::HIT_NONE);
//    labelSlice->setHittable(true);

    //m_splitProject.clearBookmarkDecoration();
    getDocument()->removeObject(ZStackObjectRole::ROLE_SEED);
    getDocument()->removeObject(ZStackObjectRole::ROLE_TMP_RESULT);
    getDocument()->removeObject(ZStackObjectRole::ROLE_ROI);
//    getDocument()->removeObject(ZStackObjectRole::ROLE_TMP_BOOKMARK);

    getDocument()->setVisible(ZStackObject::TYPE_DVID_SPARSE_STACK, false);

    checkInBodyWithMessage(m_splitProject.getBodyId());
//    getDocument()->setVisible(ZStackObject::TYPE_PUNCTA, false);



    ZDvidSparseStack *body = dynamic_cast<ZDvidSparseStack*>(
          getDocument()->getObjectGroup().findFirstSameSource(
            ZStackObject::TYPE_DVID_SPARSE_STACK,
            ZStackObjectSourceFactory::MakeSplitObjectSource()));
    if (body != NULL) {
      body->cancelFillValueSync();
    }

    m_paintLabelWidget->hide();
//    m_latencyLabelWidget->show();

    getCompleteDocument()->deprecateSplitSource();
    m_splitProject.clear();
    disableSplit();

    updateAssignedBookmarkTable();
    updateUserBookmarkTable();
  }
}

void ZFlyEmProofMvc::switchSplitBody(uint64_t bodyId)
{
  if (bodyId != m_splitProject.getBodyId()) {
    if (getCompletePresenter()->isSplitWindow()) {
//      exitSplit();
      QMessageBox msgBox;
       msgBox.setText("Changing to another body to split.");
       msgBox.setInformativeText("Do you want to save your seeds?");
       msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
       msgBox.setDefaultButton(QMessageBox::Save);
       int ret = msgBox.exec();
       if (ret != QMessageBox::Cancel) {
         if (ret == QMessageBox::Save) {
           m_splitProject.saveSeed(false);
         }
         m_splitProject.clear();
         getDocument()->removeObject(ZStackObjectRole::ROLE_SEED);
         getDocument()->removeObject(ZStackObjectRole::ROLE_TMP_RESULT);
         getCompleteDocument()->setSelectedBody(bodyId, NeuTube::BODY_LABEL_ORIGINAL);
         launchSplit(bodyId);
       }
    }
  }
}

void ZFlyEmProofMvc::processMessageSlot(const QString &message)
{
  ZJsonObject obj;
  obj.decodeString(message.toStdString().c_str());

  if (obj.hasKey("type")) {
    std::string type = ZJsonParser::stringValue(obj["type"]);
    if (type == "locate_view") {
//      viewRoi(int x, int y, int z, int radius);
    }
  }
}

void ZFlyEmProofMvc::showBodyQuickView()
{
  showFineBody3d();
//  m_splitProject.showBodyQuickView();
}

void ZFlyEmProofMvc::showSplitQuickView()
{
  m_splitProject.showResultQuickView();
}

void ZFlyEmProofMvc::showBody3d()
{
  m_splitProject.showDataFrame3d();
}

void ZFlyEmProofMvc::showSplit3d()
{
  m_splitProject.showResult3d();
}

void ZFlyEmProofMvc::showExternalNeuronWindow()
{
  if (m_externalNeuronWindow == NULL) {
    makeExternalNeuronWindow();
    m_bodyViewers->addWindow(2, m_externalNeuronWindow, "Neuron Reference");
  }
  else
  {
      m_bodyViewers->setCurrentIndex(m_bodyViewers->getTabIndex(2));
  }

//  updateCoarseBodyWindow(false, true, false);

  m_bodyViewWindow->show();
  m_bodyViewWindow->raise();
}

void ZFlyEmProofMvc::showCoarseBody3d()
{
  if (m_coarseBodyWindow == NULL) {
    makeCoarseBodyWindow();
    m_bodyViewers->addWindow(0, m_coarseBodyWindow, "Coarse Body View");

    updateCoarseBodyWindow();
    m_coarseBodyWindow->setYZView();

  }
  else
  {
      m_bodyViewers->setCurrentIndex(m_bodyViewers->getTabIndex(0));
  }

//  updateCoarseBodyWindow(false, true, false);

  m_bodyViewWindow->show();
  m_bodyViewWindow->raise();

//  m_mergeProject.showCoarseBody3d();
}

void ZFlyEmProofMvc::showFineBody3d()
{
//  m_mergeProject.showBody3d();
  if (m_bodyWindow == NULL) {
    makeBodyWindow();
    m_bodyViewers->addWindow(1, m_bodyWindow, "Body View");
    updateBodyWindow();
    m_bodyWindow->setYZView();
  }
  else
  {
      m_bodyViewers->setCurrentIndex(m_bodyViewers->getTabIndex(1));
  }

  m_bodyViewWindow->setCurrentWidow(m_bodyWindow);
  m_bodyViewWindow->show();
  m_bodyViewWindow->raise();
}

void ZFlyEmProofMvc::showRoi3dWindow()
{
  if (m_roiWindow == NULL) {
    ZWindowFactory factory;
    factory.setDeleteOnClose(true);
    factory.setVisible(Z3DWindow::LAYER_PUNCTA, false);
    m_roiWindow =
        factory.make3DWindow(m_doc, Z3DWindow::INIT_EXCLUDE_VOLUME);
    m_roiWindow->getSwcFilter()->setRenderingPrimitive("Sphere");
    m_roiWindow->getSwcFilter()->setColorMode("Topology");
    setWindowSignalSlot(m_roiWindow);
  }

  m_roiWindow->show();
  m_roiWindow->raise();
}

void ZFlyEmProofMvc::showObjectWindow()
{
  if (m_objectWindow == NULL) {
    ZWindowFactory factory;
    factory.setDeleteOnClose(true);
    factory.setVisible(Z3DWindow::LAYER_PUNCTA, false);
    m_objectWindow =
        factory.make3DWindow(m_doc, Z3DWindow::INIT_EXCLUDE_VOLUME);
    m_objectWindow->getSwcFilter()->setRenderingPrimitive("Sphere");
    setWindowSignalSlot(m_objectWindow);
  }

  m_objectWindow->show();
  m_objectWindow->raise();
}

void ZFlyEmProofMvc::showQueryTable()
{
  /*
  if (m_queryWindow = NULL) {
    m_queryWindow = new ZFlyEmDataFrame;
    m_queryWindow.load()
  }
  */
}

void ZFlyEmProofMvc::showOrthoWindow(double x, double y, double z)
{
  if (m_orthoWindow == NULL) {
    makeOrthoWindow();
  }

  m_orthoWindow->show();
  m_orthoWindow->raise();
  m_orthoWindow->updateData(ZPoint(x, y, z).toIntPoint());
}

void ZFlyEmProofMvc::showSkeletonWindow()
{
//  m_mergeProject.showBody3d();
  if (m_skeletonWindow == NULL) {
    makeSkeletonWindow();
    m_bodyViewers->addWindow(3, m_skeletonWindow, "Skeleton View");
    updateSkeletonWindow();
    m_skeletonWindow->setYZView();
  } else {
    m_bodyViewers->setCurrentIndex(m_bodyViewers->getTabIndex(3));
  }

  m_bodyViewWindow->setCurrentWidow(m_skeletonWindow);
  m_bodyViewWindow->show();
  m_bodyViewWindow->raise();
}

/*
void ZFlyEmProofMvc::closeBodyWindow(int index)
{
  closeBodyWindow(m_coarseBodyWindow);
}
*/

void ZFlyEmProofMvc::closeOrthoWindow()
{
  if (m_orthoWindow != NULL) {
    m_orthoWindow->close();
  }
}

void ZFlyEmProofMvc::close3DWindow(Z3DWindow *window)
{
  if (window != NULL) {
    window->close();
  }
}

void ZFlyEmProofMvc::closeBodyWindow(Z3DWindow *window)
{
  if (window != NULL) {
    window->close();
  }
}

void ZFlyEmProofMvc::closeAllAssociatedWindow()
{
  close3DWindow(m_objectWindow);
  close3DWindow(m_roiWindow);
  close3DWindow(m_externalNeuronWindow);
  if (m_bodyViewWindow != NULL) {
    m_bodyViewWindow->close();
  }
  closeOrthoWindow();
}

void ZFlyEmProofMvc::closeAllBodyWindow()
{
  closeBodyWindow(m_coarseBodyWindow);
  closeBodyWindow(m_bodyWindow);
  closeBodyWindow(m_splitWindow);
  closeBodyWindow(m_skeletonWindow);
  closeBodyWindow(m_externalNeuronWindow);
}

void ZFlyEmProofMvc::setDvidLabelSliceSize(int width, int height)
{
  if (getCompleteDocument() != NULL) {
    ZDvidLabelSlice *slice =
        getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
    if (slice != NULL) {
      slice->setMaxSize(getView()->getViewParameter(), width, height);
      getView()->paintObject();
    }
  }
}

void ZFlyEmProofMvc::showFullSegmentation()
{
  if (getCompleteDocument() != NULL) {
    ZDvidLabelSlice *slice =
        getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
    if (slice != NULL) {
      slice->updateFullView(getView()->getViewParameter());
      getView()->paintObject();
    }
  }
}

void ZFlyEmProofMvc::saveSeed()
{
  m_splitProject.saveSeed(true);
}

void ZFlyEmProofMvc::saveMergeOperation()
{
  getCompleteDocument()->saveMergeOperation();
}

void ZFlyEmProofMvc::commitMerge()
{
  if (ZDialogFactory::Ask("Upload Confirmation",
                          "Do you want to upload the merging result now? "
                          "It cannot be undone. ",
                          this)) {
    mergeCoarseBodyWindow();
    getCompleteDocument()->getMergeProject()->uploadResult();
//    m_mergeProject.uploadResult();
    ZDvidSparseStack *body = getCompleteDocument()->getBodyForSplit();
    if (body != NULL) {
      getDocument()->getObjectGroup().removeObject(body, true);
    }
  }
}

void ZFlyEmProofMvc::chopBodyZ()
{
  m_splitUploadDlg->setComment(
        QString("Split from %1").arg(m_splitProject.getBodyId()));
  if (m_splitUploadDlg->exec()) {
    const QString threadId = "ZFlyEmBodySplitProject::chopBodyZ";
    if (!m_futureMap.isAlive(threadId)) {
      m_futureMap.removeDeadThread();
      QFuture<void> future =
          QtConcurrent::run(
            &m_splitProject, &ZFlyEmBodySplitProject::chopBodyZ,
            getView()->getCurrentZ(), m_splitUploadDlg);
      m_futureMap[threadId] = future;
    }
  }
}

void ZFlyEmProofMvc::chopBody()
{
  if (m_bodyChopDlg->exec()) {
    m_splitUploadDlg->setComment(
          QString("Split from %1").arg(m_splitProject.getBodyId()));
    if (m_splitUploadDlg->exec()) {
      const QString threadId = "ZFlyEmBodySplitProject::chopBody";
      if (!m_futureMap.isAlive(threadId)) {
        m_futureMap.removeDeadThread();
        ZIntPoint center = getView()->getCenter();
        int v = center.getZ();
        NeuTube::EAxis axis = m_bodyChopDlg->getAxis();
        if (axis == NeuTube::X_AXIS) {
          v = center.getX();
        } else if (axis == NeuTube::Y_AXIS) {
          v = center.getY();
        }

        QFuture<void> future =
            QtConcurrent::run(
              &m_splitProject, &ZFlyEmBodySplitProject::chopBody,
              v, axis, m_splitUploadDlg);
        m_futureMap[threadId] = future;
      }
    }
  }
}

void ZFlyEmProofMvc::cropBody()
{ 
  m_splitUploadDlg->setComment(
        QString("Split from %1").arg(m_splitProject.getBodyId()));
  if (m_splitUploadDlg->exec()) {
    const QString threadId = "ZFlyEmBodySplitProject::cropBody";
    if (!m_futureMap.isAlive(threadId)) {
      m_futureMap.removeDeadThread();
      QFuture<void> future =
          QtConcurrent::run(
            &m_splitProject, &ZFlyEmBodySplitProject::cropBody,
            m_splitUploadDlg);
      m_futureMap[threadId] = future;
    }
  }
}

void ZFlyEmProofMvc::decomposeBody()
{
  m_splitUploadDlg->setComment(
        QString("Split from %1").arg(m_splitProject.getBodyId()));
  if (m_splitUploadDlg->exec()) {
    const QString threadId = "ZFlyEmBodySplitProject::decomposeBody";
    if (!m_futureMap.isAlive(threadId)) {
      m_futureMap.removeDeadThread();
      QFuture<void> future =
          QtConcurrent::run(
            &m_splitProject, &ZFlyEmBodySplitProject::decomposeBody,
            m_splitUploadDlg);
      m_futureMap[threadId] = future;
    }
  }
}

void ZFlyEmProofMvc::commitCurrentSplit()
{
  if (!getDocument()->isSegmentationReady()) {
//    emit messageGenerated("test");
    emit messageGenerated(
          ZWidgetMessage("Failed to save results: The split has not been updated."
                         "Please Run full split (shift+space) first.",
                         NeuTube::MSG_ERROR, ZWidgetMessage::TARGET_TEXT_APPENDING));
    return;
  }


  if (m_splitCommitDlg->exec()) {
    m_splitProject.setMinObjSize(m_splitCommitDlg->getGroupSize());
    m_splitProject.keepMainSeed(m_splitCommitDlg->keepingMainSeed());
    m_splitProject.enableCca(m_splitCommitDlg->runningCca());
    const QString threadId = "ZFlyEmBodySplitProject::commitResult";
    if (!m_futureMap.isAlive(threadId)) {
      m_futureMap.removeDeadThread();
      QFuture<void> future =
          QtConcurrent::run(
            &m_splitProject, &ZFlyEmBodySplitProject::commitResult);
      m_futureMap[threadId] = future;
    }
//    m_splitProject.commitResult();
  }
}

void ZFlyEmProofMvc::clearAssignedBookmarkModel()
{
  for (QMap<FlyEM::EProofreadingMode, ZFlyEmBookmarkListModel*>::iterator
       iter = m_assignedBookmarkModel.begin();
       iter != m_assignedBookmarkModel.end(); ++iter) {
    ZFlyEmBookmarkListModel *model = *iter;
    model->clear();
  }
}

void ZFlyEmProofMvc::clearUserBookmarkModel()
{
  for (QMap<FlyEM::EProofreadingMode, ZFlyEmBookmarkListModel*>::iterator
       iter = m_userBookmarkModel.begin();
       iter != m_userBookmarkModel.end(); ++iter) {
    ZFlyEmBookmarkListModel *model = *iter;
    model->clear();
  }
}

void ZFlyEmProofMvc::loadBookmarkFunc(const QString &filePath)
{
  ZOUT(LINFO(), 3) << "Importing bookmarks";

  getProgressSignal()->startProgress("Importing bookmarks ...");
  //  m_splitProject.loadBookmark(filePath);

    ZDvidReader reader;
  //  ZFlyEmCoordinateConverter converter;
    if (reader.open(getDvidTarget())) {
  //    ZDvidInfo info = reader.readGrayScaleInfo();
  //    converter.configure(info);
      getProgressSignal()->advanceProgress(0.1);

      clearAssignedBookmarkModel();
//      notifyBookmarkDeleted();
//      m_assignedBookmarkList->clear();
      QList<ZFlyEmBookmark*> bookmarkList =
          getCompleteDocument()->importFlyEmBookmark(filePath.toStdString());

      appendAssignedBookmarkTable(bookmarkList);
//      m_assignedBookmarkList->append(bookmarkList);
      getProgressSignal()->advanceProgress(0.5);
  //    m_bookmarkArray.importJsonFile(filePath.toStdString(), NULL/*&converter*/);
    }

//    notifyBookmarkUpdated();

    getProgressSignal()->advanceProgress(0.3);

    getProgressSignal()->endProgress();
  //  m_bookmarkArray.importJsonFile(filePath);

  //  emit bookmarkUpdated(&m_splitProject);
}

void ZFlyEmProofMvc::loadBookmark(const QString &filePath)
{
  QtConcurrent::run(this, &ZFlyEmProofMvc::loadBookmarkFunc, filePath);
}

void ZFlyEmProofMvc::loadSynapse()
{
  QString fileName = ZDialogFactory::GetOpenFileName("Load Synapses", "", this);
  if (!fileName.isEmpty()) {
    getCompleteDocument()->loadSynapse(fileName.toStdString());
  }
}

void ZFlyEmProofMvc::loadBookmark()
{
  QString fileName = ZDialogFactory::GetOpenFileName("Load Bookmarks", "", this);
  if (!fileName.isEmpty()) {
    loadBookmark(fileName);
  }
}

void ZFlyEmProofMvc::openSequencer()
{
  m_bodyInfoDlg->show();
  m_bodyInfoDlg->raise();
}

void ZFlyEmProofMvc::openProtocol()
{
  m_protocolSwitcher->openProtocolDialogRequested();
}

void ZFlyEmProofMvc::openRoiTool()
{
  m_roiDlg->show();
  m_roiDlg->raise();
}

void ZFlyEmProofMvc::goToNearestRoi()
{
  ZFlyEmRoiProject *project = m_roiDlg->getProject();
  if (project != NULL) {
    m_roiDlg->updateRoi();
    if (project->hasRoi()) {
      int z = project->getNearestRoiZ(getView()->getCurrentZ());
      goToSlice(z);
    }
  }
}

void ZFlyEmProofMvc::estimateRoi()
{
  ZFlyEmRoiProject *project = m_roiDlg->getProject();
  if (project != NULL) {
    m_roiDlg->updateRoi();
    if (project->hasRoi()) {
      int z = getView()->getCurrentZ();
      ZClosedCurve *roi = new ZClosedCurve;
      project->estimateRoi(z, roi);
      project->setRoi(roi, z);
      updateRoiGlyph();
    }
  }
}

void ZFlyEmProofMvc::loadRoiProject()
{
  updateRoiGlyph();
  getPresenter()->setActiveObjectSize(
        ZStackPresenter::ROLE_SWC,
        FlyEm::GetFlyEmRoiMarkerRadius(getDocument()->getStackWidth(),
                                       getDocument()->getStackHeight()));
}

void ZFlyEmProofMvc::closeRoiProject()
{
  updateRoiGlyph();
  getPresenter()->setDefaultActiveObjectSize(ZStackPresenter::ROLE_SWC);
  close3DWindow(m_roiWindow);
}


void ZFlyEmProofMvc::updateRoiGlyph()
{
  ZOUT(LTRACE(), 5) << "Update ROI glyph";
  QList<ZStackObject*> objList =
      getCompleteDocument()->getObjectList(ZStackObjectRole::ROLE_ROI);
  for (QList<ZStackObject*>::iterator iter = objList.begin();
       iter != objList.end(); ++iter) {
    ZStackObject *obj = *iter;
    getCompleteDocument()->executeRemoveObjectCommand(obj);
  }
//  getCompleteDocument()->removeObject(ZStackObjectRole::ROLE_ROI, true);

  ZFlyEmRoiProject *project = m_roiDlg->getProject();
  if (project != NULL) {
    getCompletePresenter()->setPaintingRoi(true);
    ZSwcTree *tree = project->getAllRoiSwc();
    if (tree != NULL) {
      tree->addRole(ZStackObjectRole::ROLE_ROI);
      tree->removeVisualEffect(NeuTube::Display::SwcTree::VE_FULL_SKELETON);
      getCompleteDocument()->executeAddObjectCommand(tree);
    }
  } else {
    getCompletePresenter()->setPaintingRoi(false);
  }
}

void ZFlyEmProofMvc::openTodo()
{
  m_todoDlg->show();
  m_todoDlg->raise();
}

void ZFlyEmProofMvc::goToTBar()
{
  ZDvidSynapseEnsemble *se =
      getCompleteDocument()->getDvidSynapseEnsemble(getView()->getSliceAxis());
  if (se != NULL) {
    const std::set<ZIntPoint> &selected = se->getSelector().getSelectedSet();

    if (selected.size() == 1) {
      const ZIntPoint &pt = *(selected.begin());
      ZDvidSynapse synapse =
          se->getSynapse(pt, ZDvidSynapseEnsemble::DATA_LOCAL);
      if (synapse.getKind() == ZDvidSynapse::KIND_POST_SYN) {
        const std::vector<ZIntPoint> &partners = synapse.getPartners();
        if (!partners.empty()) {
          se->selectWithPartner(partners.front(), false);
          zoomTo(partners.front());
        }
      } else {
        zoomTo(pt);
      }
    }
  }
}

void ZFlyEmProofMvc::showSynapseAnnotation(bool visible)
{
  ZDvidSynapseEnsemble *se =
      getCompleteDocument()->getDvidSynapseEnsemble(NeuTube::Z_AXIS);
  if (se != NULL) {
    se->setVisible(visible);
    if (visible) {
      se->download(getView()->getZ(NeuTube::COORD_STACK));
    }
    getCompleteDocument()->processObjectModified(se);
    getCompleteDocument()->notifyObjectModified();
  }
}

void ZFlyEmProofMvc::showBookmark(bool visible)
{
  getCompleteDocument()->setVisible(ZStackObject::TYPE_FLYEM_BOOKMARK, visible);
//  m_splitProject.setBookmarkVisible(visible);
//  m_mergeProject.setBookmarkVisible(visible);
}

void ZFlyEmProofMvc::showRoiMask(bool visible)
{
  getCompleteDocument()->setVisible(ZStackObjectRole::ROLE_ROI_MASK, visible);
}

void ZFlyEmProofMvc::showSegmentation(bool visible)
{
  ZDvidLabelSlice *slice =
      getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
  if (slice != NULL) {
    slice->setVisible(visible);
    if (visible) {
      slice->update(getView()->getViewParameter());
    }
    getCompleteDocument()->processObjectModified(slice);
    getCompleteDocument()->notifyObjectModified();
  }
}

void ZFlyEmProofMvc::toggleSegmentation()
{
  ZDvidLabelSlice *slice =
      getCompleteDocument()->getDvidLabelSlice(NeuTube::Z_AXIS);
  if (slice != NULL) {
    showSegmentation(!slice->isVisible());
  }
}

void ZFlyEmProofMvc::setHighContrast(bool on)
{
  getCompletePresenter()->useHighContrastProtocal(on);
  getView()->redraw();
}

void ZFlyEmProofMvc::showData(bool visible)
{
  getCompletePresenter()->showData(visible);
  getDocument()->beginObjectModifiedMode(
        ZStackDoc::OBJECT_MODIFIED_CACHE);

  ZOUT(LTRACE(), 5) << "Show data";
//  QMutexLocker locker(getDocument()->getObjectGroup().getMutex());
  QList<ZStackObject*> &objList =
      getDocument()->getObjectGroup().getObjectList();

  for (QList<ZStackObject*>::iterator iter = objList.begin();
       iter != objList.end(); ++iter) {
    ZStackObject *obj = *iter;
    if (obj->getType() == ZStackObject::TYPE_DVID_ANNOTATION ||
        obj->getType() == ZStackObject::TYPE_DVID_SYNAPE_ENSEMBLE ||
        obj->getType() == ZStackObject::TYPE_FLYEM_TODO_LIST ||
        obj->getType() == ZStackObject::TYPE_FLYEM_BOOKMARK) {
      obj->setVisible(visible);
      getDocument()->processObjectModified(obj);
    } else if (obj->getType() == ZStackObject::TYPE_DVID_LABEL_SLICE) {
      if (visible) {
        obj->setVisible(m_showSegmentation);
      } else {
        obj->setVisible(false);
      }
      getDocument()->processObjectModified(obj);
    }
  }
  getDocument()->endObjectModifiedMode();
  getDocument()->notifyObjectModified();
}

void ZFlyEmProofMvc::toggleData()
{
  showData(!getCompletePresenter()->showingData());
}

void ZFlyEmProofMvc::showTodo(bool visible)
{
  getCompleteDocument()->setVisible(ZStackObject::TYPE_FLYEM_TODO_LIST, visible);
}

ZDvidLabelSlice* ZFlyEmProofMvc::getDvidLabelSlice() const
{
  return getCompleteDocument()->getDvidLabelSlice(getView()->getSliceAxis());
}

void ZFlyEmProofMvc::addSelectionAt(int x, int y, int z)
{
  ZDvidReader &reader = getCompleteDocument()->getDvidReader();
  if (reader.isReady()) {
    uint64_t bodyId = reader.readBodyIdAt(x, y, z);
    if (bodyId > 0) {
//      ZDvidLabelSlice *slice = getDvidLabelSlice();
      QList<ZDvidLabelSlice*> sliceList =
          getCompleteDocument()->getDvidLabelSliceList();
      for (QList<ZDvidLabelSlice*>::iterator iter = sliceList.begin();
           iter != sliceList.end(); ++iter) {
        ZDvidLabelSlice *slice = *iter;
        if (slice != NULL) {
          slice->recordSelection();
          slice->addSelection(
                slice->getMappedLabel(bodyId, NeuTube::BODY_LABEL_ORIGINAL),
                NeuTube::BODY_LABEL_ORIGINAL);
          slice->processSelection();
        }
      }
      updateBodySelection();
    }
  }
}

void ZFlyEmProofMvc::xorSelectionAt(int x, int y, int z)
{
  ZDvidReader &reader = getCompleteDocument()->getDvidReader();
  if (reader.isReady()) {
    uint64_t bodyId = reader.readBodyIdAt(x, y, z);
    if (bodyId > 0) {
      bodyId = getCompleteDocument()->getBodyMerger()->getFinalLabel(bodyId);
      getCompleteDocument()->toggleBodySelection(
            bodyId, NeuTube::BODY_LABEL_MAPPED);
#if 0
//      ZDvidLabelSlice *slice = getDvidLabelSlice();
      QList<ZDvidLabelSlice*> sliceList =
          getCompleteDocument()->getDvidLabelSliceList();
      for (QList<ZDvidLabelSlice*>::iterator iter = sliceList.begin();
           iter != sliceList.end(); ++iter) {
        ZDvidLabelSlice *slice = *iter;
        if (slice != NULL) {
          //        uint64_t finalBodyId = getMappedBodyId(bodyId);
          slice->recordSelection();
          slice->xorSelection(
                slice->getMappedLabel(bodyId, NeuTube::BODY_LABEL_ORIGINAL),
                NeuTube::BODY_LABEL_MAPPED);
          slice->processSelection();
#if 0
          QList<uint64_t> labelList =
              getCompleteDocument()->getBodyMerger()->getOriginalLabelList(
                finalBodyId);
          slice->xorSelectionGroup(labelList.begin(), labelList.end());
          /*
        foreach (uint64_t label, labelList) {
          slice->xorSelection(label);
        }
        */
#endif
        }
      }
      updateBodySelection();
#endif
    }
  }
}

void ZFlyEmProofMvc::deselectAllBody()
{
  ZDvidReader &reader = getCompleteDocument()->getDvidReader();
  if (reader.isReady()) {
//    ZDvidLabelSlice *slice = getDvidLabelSlice();
    QList<ZDvidLabelSlice*> sliceList =
        getCompleteDocument()->getDvidLabelSliceList();
    for (QList<ZDvidLabelSlice*>::iterator iter = sliceList.begin();
         iter != sliceList.end(); ++iter) {
      ZDvidLabelSlice *slice = *iter;
      if (slice != NULL) {
        slice->recordSelection();
        slice->deselectAll();
        slice->processSelection();
      }
    }
    updateBodySelection();
  }
}

void ZFlyEmProofMvc::selectSeed()
{
  ZSpinBoxDialog *dlg = ZDialogFactory::makeSpinBoxDialog(this);
  dlg->setValueLabel("Label");
  dlg->getButton(ZButtonBox::ROLE_SKIP)->hide();
  dlg->setValue(1);
  if (dlg->exec()) {
    int label = dlg->getValue();
   int nSelected = m_splitProject.selectSeed(label);
   getView()->paintObject();
   emit messageGenerated(QString("%1 seed(s) are selected.").arg(nSelected));
  }
  delete dlg;
}

void ZFlyEmProofMvc::setMainSeed()
{
  ZSpinBoxDialog *dlg = ZDialogFactory::makeSpinBoxDialog(this);
  dlg->setValueLabel("Label");
  dlg->getButton(ZButtonBox::ROLE_SKIP)->hide();
  dlg->setValue(1);
  if (dlg->exec()) {
    int label = dlg->getValue();
    m_splitProject.swapMainSeedLabel(label);
    getView()->paintObject();
    emit messageGenerated(QString("Label %1 is set the main seed.").arg(label));
  }
  delete dlg;
}

void ZFlyEmProofMvc::selectAllSeed()
{
  int nSelected = m_splitProject.selectAllSeed();
  getView()->paintObject();
  emit messageGenerated(QString("%1 seed(s) are selected.").arg(nSelected));
}

void ZFlyEmProofMvc::recoverSeed()
{
  if (ZDialogFactory::Ask("Recover Seed", "All current seeds might be lost. "
                          "Do you want to continue?", this)) {
    m_splitProject.recoverSeed();
  }
}

void ZFlyEmProofMvc::exportSeed()
{
  QString fileName = ZDialogFactory::GetSaveFileName("Export Seed", "", this);
  if (!fileName.isEmpty()) {
    m_splitProject.exportSeed(fileName);
  }
}

void ZFlyEmProofMvc::importSeed()
{
  if (ZDialogFactory::Ask("Import Seed",
                          "All current seeds might be lost after import. "
                          "Do you want to continue?", this)) {
    QString fileName = ZDialogFactory::GetOpenFileName("Import Seed", "", this);
    if (!fileName.isEmpty()) {
      m_splitProject.importSeed(fileName);
    }
  }
}

uint64_t ZFlyEmProofMvc::getMappedBodyId(uint64_t bodyId)
{
  return getCompleteDocument()->getBodyMerger()->getFinalLabel(bodyId);
//  return m_mergeProject.getMappedBodyId(bodyId);
}

std::set<uint64_t> ZFlyEmProofMvc::getCurrentSelectedBodyId(
    NeuTube::EBodyLabelType type) const
{
  const ZDvidLabelSlice *labelSlice = getDvidLabelSlice();
  if (labelSlice != NULL) {
    return labelSlice->getSelected(type);
  }

  return std::set<uint64_t>();
//  return m_mergeProject.getSelection(type);
#if 0
  std::set<uint64_t> idSet;

  if (getCompletePresenter()->isHighlight()) {

//    idSet = m_mergeProject.getSelectedBodyId();
  } else {
    ZDvidLabelSlice *labelSlice = getCompleteDocument()->getDvidLabelSlice();
    if (labelSlice != NULL) {
      if (labelSlice->isVisible()) {
        idSet = labelSlice->getSelected(type);
      }
    }
  }

  return idSet;
#endif
}

void ZFlyEmProofMvc::notifyBodyMergeEdited()
{
  emit bodyMergeEdited();
}

void ZFlyEmProofMvc::selectBody(QList<uint64_t> bodyIdList)
{
  if (!getCompletePresenter()->isSplitWindow()) {
    ZDvidLabelSlice *slice = getDvidLabelSlice();
    if (slice != NULL) {
      slice->recordSelection();
      slice->clearSelection();
      foreach(uint64_t bodyId, bodyIdList) {
        slice->addSelection(
              slice->getMappedLabel(bodyId, NeuTube::BODY_LABEL_ORIGINAL),
              NeuTube::BODY_LABEL_MAPPED);
      }
      slice->processSelection();
      processLabelSliceSelectionChange();
    }
    updateBodySelection();
  }
}

#if 0
void ZFlyEmProofMvc::locateBody(QList<uint64_t> bodyIdList)
{
  if (!getCompletePresenter()->isSplitWindow()) {
//    ZDvidReader reader; //Todo: Need to use a shared reader
    ZDvidReader &reader = getCompleteDocument()->getDvidReader();
    if (reader.open(getDvidTarget())) {
      QList<uint64_t> goodIdList;
      QList<uint64_t> badIdList;
      foreach(uint64_t bodyId, bodyIdList) {
        if (reader.hasBody(bodyId)) {
          goodIdList.append(bodyId);
        } else {
          badIdList.append(bodyId);
        }
      }

      if (!badIdList.isEmpty()) {
        std::ostringstream stream;
        stream << "Cannot find body: ";
        foreach (uint64_t badId, badIdList) {
          stream << badId;
        }

        ZWidgetMessage(stream.str().c_str(), NeuTube::MSG_WARNING);
      }

      if (!goodIdList.isEmpty()) {
        ZDvidLabelSlice *slice = getDvidLabelSlice();
        if (slice != NULL) {
          slice->recordSelection();
          slice->clearSelection();
          foreach(uint64_t bodyId, goodIdList) {
            slice->addSelection(
                  slice->getMappedLabel(bodyId, NeuTube::BODY_LABEL_ORIGINAL),
                  NeuTube::BODY_LABEL_MAPPED);
          }
          slice->processSelection();
          processLabelSliceSelectionChange();
        }
        updateBodySelection();

        uint64_t goodId = goodIdList.front();
        ZIntPoint pt = reader.readBodyLocation(goodId);
        zoomTo(pt);
      }
    }
  }
}
#endif

bool ZFlyEmProofMvc::locateBody(uint64_t bodyId, bool appending)
{
  bool succ = true;
  if (!getCompletePresenter()->isSplitWindow()) {
    ZDvidReader &reader = getCompleteDocument()->getDvidReader();
    if (reader.isReady()) {
      ZIntPoint pt = reader.readBodyLocation(bodyId);
      if (pt.isValid()) {
        ZDvidLabelSlice *slice = getDvidLabelSlice();
        if (slice != NULL) {
          slice->recordSelection();
          if (!appending) {
            slice->clearSelection();
          }
          slice->addSelection(
                slice->getMappedLabel(bodyId, NeuTube::BODY_LABEL_ORIGINAL),
                NeuTube::BODY_LABEL_MAPPED);
          slice->processSelection();
//          processLabelSliceSelectionChange(); //will be called in updateBodySelection
        }
        updateBodySelection();
        zoomTo(pt);
      } else {
        emit messageGenerated(ZWidgetMessage("Failed to zoom into the body",
                                             NeuTube::MSG_ERROR));
        if (!reader.hasBody(bodyId)) {
          emit messageGenerated(
                ZWidgetMessage(QString("Cannot go to body: %1. No such body.").
                               arg(bodyId), NeuTube::MSG_ERROR));
          succ = false;
        }
      }
    }
  } else {
    succ = false;
  }

  return succ;
}

bool ZFlyEmProofMvc::locateBody(uint64_t bodyId)
{
  return locateBody(bodyId, false);
}

void ZFlyEmProofMvc::addLocateBody(uint64_t bodyId)
{
  locateBody(bodyId, true);
}

void ZFlyEmProofMvc::selectBody(uint64_t bodyId)
{
  /*
  ZDvidLabelSlice *slice = getCompleteDocument()->getDvidLabelSlice();
  if (slice != NULL) {
    slice->addSelection(bodyId, NeuTube::BODY_LABEL_MAPPED);
  }
  */
  getCompleteDocument()->selectBody(bodyId);
  updateBodySelection();
}

void ZFlyEmProofMvc::processViewChangeCustom(const ZStackViewParam &viewParam)
{
  if (m_currentViewParam != viewParam) {
//    m_mergeProject.update3DBodyViewPlane(viewParam);
    m_splitProject.update3DViewPlane();

    updateBodyWindowPlane(m_coarseBodyWindow, viewParam);
    updateBodyWindowPlane(m_bodyWindow, viewParam);
    updateBodyWindowPlane(m_skeletonWindow, viewParam);
    updateBodyWindowPlane(m_externalNeuronWindow, viewParam);

    m_currentViewParam = viewParam;
  }
}

void ZFlyEmProofMvc::checkSelectedBookmark(bool checking)
{
  TStackObjectSet &selected = getCompleteDocument()->getSelected(
        ZStackObject::TYPE_FLYEM_BOOKMARK);
  for (TStackObjectSet::iterator iter = selected.begin();
       iter != selected.end(); ++iter) {
    ZFlyEmBookmark *bookmark = dynamic_cast<ZFlyEmBookmark*>(*iter);
    bookmark->setChecked(checking);
    recordBookmark(bookmark);
  }
  if (!selected.isEmpty()) {
//    emit bookmarkUpdated();
  }
}

void ZFlyEmProofMvc::recordCheckedBookmark(const QString &key, bool checking)
{
//  ZFlyEmBookmark *bookmark = m_bookmarkArray.findFirstBookmark(key);
  ZFlyEmBookmark *bookmark = getCompleteDocument()->findFirstBookmark(key);
  if (bookmark != NULL) {
    bookmark->setChecked(checking);
    recordBookmark(bookmark);
  }
}

void ZFlyEmProofMvc::recordBookmark(ZFlyEmBookmark *bookmark)
{
  ZDvidWriter writer;
  if (writer.open(getDvidTarget())) {
    if (bookmark->isCustom()) {
      writer.writeBookmark(*bookmark);
    } else {
      writer.writeBookmarkKey(*bookmark);
    }

    if (writer.getStatusCode() != 200) {
      emit messageGenerated(ZWidgetMessage("Failed to record bookmark.",
                                           NeuTube::MSG_WARNING));
    }
  }
}

void ZFlyEmProofMvc::processCheckedUserBookmark(ZFlyEmBookmark * /*bookmark*/)
{
//  getCompleteDocument()->setCustomBookmarkSaveState(false);
}

void ZFlyEmProofMvc::enhanceTileContrast(bool state)
{
  getCompletePresenter()->setHighTileContrast(state);
  getCompleteDocument()->enhanceTileContrast(state);
  /*
  ZDvidTileEnsemble *tile = getCompleteDocument()->getDvidTileEnsemble();
  if (tile != NULL) {
    if (state) {
      tile->addVisualEffect(NeuTube::Display::Image::VE_HIGH_CONTRAST);
    } else {
      tile->removeVisualEffect(NeuTube::Display::Image::VE_HIGH_CONTRAST);
    }
    getCompleteDocument()->processObjectModified(tile->getTarget());
  }
  */
}

void ZFlyEmProofMvc::smoothDisplay(bool state)
{
  getView()->setSmoothDisplay(state);
}

ZFlyEmSupervisor* ZFlyEmProofMvc::getSupervisor() const
{
  return getCompleteDocument()->getSupervisor();
}

void ZFlyEmProofMvc::annotateBookmark(ZFlyEmBookmark *bookmark)
{
  if (bookmark != NULL) {
    ZFlyEmBookmarkAnnotationDialog dlg(this);
    dlg.setFrom(bookmark);
    if (dlg.exec()) {
      dlg.annotate(bookmark);
      ZDvidWriter writer;
      if (writer.open(getDvidTarget())) {
        writer.writeBookmark(bookmark->toDvidAnnotationJson());
      }
      if (!writer.isStatusOk()) {
        emit messageGenerated(
              ZWidgetMessage("Failed to save bookmark.", NeuTube::MSG_WARNING));
      }
      getCompleteDocument()->processBookmarkAnnotationEvent(bookmark);

      updateUserBookmarkTable();
    }
  }
}

void ZFlyEmProofMvc::annotateSynapse()
{
  ZFlyEmSynapseAnnotationDialog dlg(this);
  getCompleteDocument()->annotateSelectedSynapse(&dlg, getView()->getSliceAxis());
/*
  if (dlg.exec()) {
    double c = dlg.getConfidence();
    ZJsonObject propJson;
    std::ostringstream stream;
    stream << c;
    propJson.setEntry("confidence", stream.str());
    getCompleteDocument()->annotateSelectedSynapse(
          propJson, getView()->getSliceAxis());
  }
  */
}

void ZFlyEmProofMvc::selectBodyInRoi(bool appending)
{
  getCompleteDocument()->selectBodyInRoi(
        getView()->getCurrentZ(), appending, true);
}
/*
void ZFlyEmProofMvc::prepareBookmarkModel(
    ZFlyEmBookmarkListModel *model, QSortFilterProxyModel *proxy)
{
  if (proxy != NULL) {
    proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setFilterKeyColumn(-1);
    proxy->setSourceModel(model);
  }
}
*/

void ZFlyEmProofMvc::sortAssignedBookmarkTable()
{
  getAssignedBookmarkModel()->sortBookmark();
//  m_assignedBookmarkProxy->sort(m_assignedBookmarkProxy->sortColumn(),
//                                m_assignedBookmarkProxy->sortOrder());
}

void ZFlyEmProofMvc::sortUserBookmarkTable()
{
  getUserBookmarkModel()->sortBookmark();
//  m_userBookmarkProxy->sort(m_userBookmarkProxy->sortColumn(),
//                            m_userBookmarkProxy->sortOrder());
}

ZFlyEmBookmarkListModel* ZFlyEmProofMvc::getUserBookmarkModel() const
{
  ZFlyEmBookmarkListModel *model = NULL;
  if (getCompletePresenter()->isSplitOn()) {
    model = m_userBookmarkModel[FlyEM::PR_SPLIT];
  } else {
    model = m_userBookmarkModel[FlyEM::PR_NORMAL];
  }

  return model;
}

ZFlyEmBookmarkListModel* ZFlyEmProofMvc::getAssignedBookmarkModel() const
{
  ZFlyEmBookmarkListModel *model = NULL;
  if (getCompletePresenter()->isSplitOn()) {
    model = m_assignedBookmarkModel[FlyEM::PR_SPLIT];
  } else {
    model = m_assignedBookmarkModel[FlyEM::PR_NORMAL];
  }

  return model;
}

#if 0
void ZFlyEmProofMvc::updateAssignedBookmarkTable()
{
  model->clear();
  ZOUT(LTRACE(), 5) << "Update assgined bookmark table";
  const TStackObjectList &objList =
      getDocument()->getObjectList(ZStackObject::TYPE_FLYEM_BOOKMARK);
  for (TStackObjectList::const_iterator iter = objList.begin();
       iter != objList.end(); ++iter) {
    const ZFlyEmBookmark *bookmark = dynamic_cast<ZFlyEmBookmark*>(*iter);
    if (bookmark != NULL) {
      if (!bookmark->isCustom()) {
        model->append(bookmark);
      }
    }
  }

  model->sort();
}
#endif

void ZFlyEmProofMvc::updateAssignedBookmarkTable()
{
  ZFlyEmBookmarkListModel *model = getAssignedBookmarkModel();

  model->clear();
  ZOUT(LTRACE(), 5) << "Update user bookmark table";
  QList<ZFlyEmBookmark*> bookmarkList =
      getDocument()->getObjectList<ZFlyEmBookmark>();
  appendAssignedBookmarkTable(bookmarkList);

  model->sortBookmark();
}

void ZFlyEmProofMvc::updateBookmarkTable()
{
  updateUserBookmarkTable();
  updateAssignedBookmarkTable();
}

void ZFlyEmProofMvc::updateUserBookmarkTable()
{
  ZFlyEmBookmarkListModel *model = getUserBookmarkModel();

  if (model->isUsed()) {
    model->clear();
    ZOUT(LTRACE(), 5) << "Update user bookmark table";
    QList<ZFlyEmBookmark*> bookmarkList =
        getDocument()->getObjectList<ZFlyEmBookmark>();
    appendUserBookmarkTable(bookmarkList);
    model->sortBookmark();
  }
}

void ZFlyEmProofMvc::appendAssignedBookmarkTable(
    const QList<ZFlyEmBookmark *> &bookmarkList)
{
  ZFlyEmBookmarkListModel *model = getAssignedBookmarkModel();

  if (model->isUsed()) {
    for (QList<ZFlyEmBookmark *>::const_iterator iter = bookmarkList.begin();
         iter != bookmarkList.end(); ++iter) {
      const ZFlyEmBookmark *bookmark = *iter;
      if (!bookmark->isCustom()) {
        if (getCompletePresenter()->isSplitOn()) {
          if ((bookmark->getBookmarkType() == ZFlyEmBookmark::TYPE_FALSE_MERGE) &&
              (bookmark->getBodyId() == m_splitProject.getBodyId())) {
            model->append(bookmark);
          }
        } else {
          model->append(bookmark);
        }
      }
    }

    model->sortBookmark();
  }
}

void ZFlyEmProofMvc::appendUserBookmarkTable(
    const QList<ZFlyEmBookmark *> &bookmarkList)
{
  ZFlyEmBookmarkListModel *model = getUserBookmarkModel();

  if (model->isUsed()) {
    for (QList<ZFlyEmBookmark *>::const_iterator iter = bookmarkList.begin();
         iter != bookmarkList.end(); ++iter) {
      const ZFlyEmBookmark *bookmark = *iter;
      if (bookmark->isCustom()) {
        if (getCompletePresenter()->isSplitOn()) {
          if ((bookmark->getBookmarkType() == ZFlyEmBookmark::TYPE_FALSE_MERGE) &&
              (bookmark->getBodyId() == m_splitProject.getBodyId())) {
            model->append(bookmark);
          }
        } else {
          model->append(bookmark);
        }
      }
    }

    model->sortBookmark();
  }
}

#if 0
void ZFlyEmProofMvc::updateAssignedBookmarkTable()
{
  ZOUT(LINFO(), 3) << "Update bookmark table for merge project";
  m_assignedBookmarkList->clear();

  ZOUT(LINFO(), 3) << "Bookmark list cleared";

  const TStackObjectList &objList =
      getDocument()->getObjectList(ZStackObject::TYPE_FLYEM_BOOKMARK);
    //        foreach (ZFlyEmBookmark bookmark, *bookmarkArray) {
  for (TStackObjectList::const_iterator iter = objList.begin();
       iter != objList.end(); ++iter) {
    const ZFlyEmBookmark *bookmark = dynamic_cast<ZFlyEmBookmark*>(*iter);
    if (/*bookmark->getBookmarkType() != ZFlyEmBookmark::TYPE_FALSE_MERGE &&*/
        !bookmark->isCustom()) {
      m_assignedBookmarkList->append(bookmark);
    }
  }

//  getAssignedBookmarkView()->sort();
//  ZOUT(LINFO(), 3) << "Bookmark sorted";
}
#endif

void ZFlyEmProofMvc::changeColorMap(const QString &option)
{
  getCompleteDocument()->activateBodyColorMap(option);
  /*
  if (option == "Name") {
    getCompleteDocument()->useBodyNameMap(true);
  } else {
    getCompleteDocument()->useBodyNameMap(false);
  }
  */
}

void ZFlyEmProofMvc::removeBookmark(ZFlyEmBookmark *bookmark)
{
  if (bookmark != NULL) {
    if (bookmark->isCustom()) {
      ZFlyEmProofDoc *doc = getCompleteDocument();
      doc->executeRemoveBookmarkCommand(bookmark);
    }
  }
}

void ZFlyEmProofMvc::removeBookmark(const QList<ZFlyEmBookmark *> &bookmarkList)
{
  ZFlyEmProofDoc *doc = getCompleteDocument();
  doc->executeRemoveBookmarkCommand(bookmarkList);
}

void ZFlyEmProofMvc::removeLocalBookmark(ZFlyEmBookmark *bookmark)
{
  if (bookmark != NULL) {
    if (bookmark->isCustom()) {
      ZFlyEmProofDoc *doc = getCompleteDocument();

      bookmark->setSelected(false);
      doc->removeObject(bookmark, false);
      emit userBookmarkUpdated(getDocument().get());
    }
  }
}

void ZFlyEmProofMvc::addLocalBookmark(ZFlyEmBookmark *bookmark)
{
  if (bookmark != NULL) {
    ZFlyEmProofDoc *doc = getCompleteDocument();
    doc->addObject(bookmark, false);

    emit userBookmarkUpdated(getDocument().get());
  }
}

void ZFlyEmProofMvc::showBodyGrayscale()
{
  m_splitProject.showDataFrame3d();
}

void ZFlyEmProofMvc::cropCoarseBody3D()
{
  if (m_coarseBodyWindow != NULL) {
    if (m_coarseBodyWindow->hasRectRoi()) {
      ZDvidReader &reader = getCompleteDocument()->getDvidReader();
      if (reader.isReady()) {
        if (getCompletePresenter()->isSplitOn()) {
          ZObject3dScan body = reader.readCoarseBody(m_splitProject.getBodyId());
          if (body.isEmpty()) {
            emit messageGenerated(
                  ZWidgetMessage(QString("Cannot crop body: %1. No such body.").
                                 arg(m_splitProject.getBodyId()),
                                 NeuTube::MSG_ERROR));
          } else {
            ZDvidInfo dvidInfo = reader.readLabelInfo();
            ZObject3dScan bodyInRoi;
            ZObject3dScan::ConstSegmentIterator iter(&body);
            while (iter.hasNext()) {
              const ZObject3dScan::Segment &seg = iter.next();
              for (int x = seg.getStart(); x <= seg.getEnd(); ++x) {
                ZIntPoint pt(0, seg.getY(), seg.getZ());
                pt.setX(x);
                pt -= dvidInfo.getStartBlockIndex();
                pt *= dvidInfo.getBlockSize();
                pt += ZIntPoint(dvidInfo.getBlockSize().getX() / 2,
                                dvidInfo.getBlockSize().getY() / 2, 0);
                pt += dvidInfo.getStartCoordinates();

                QPointF screenPos = m_coarseBodyWindow->getScreenProjection(
                      pt.getX(), pt.getY(), pt.getZ(), Z3DWindow::LAYER_SWC);
                if (m_coarseBodyWindow->getRectRoi().contains(screenPos.x(), screenPos.y())) {
                  bodyInRoi.addSegment(seg.getZ(), seg.getY(), x, x);
                }
              }
            }
#ifdef _DEBUG_2
            body.save(GET_TEST_DATA_DIR + "/test2.sobj");
            bodyInRoi.save(GET_TEST_DATA_DIR + "/test.sobj");
#endif
            m_splitProject.commitCoarseSplit(bodyInRoi);
            ZFlyEmMisc::SubtractBodyWithBlock(
                  getCompleteDocument()->getBodyForSplit()->getObjectMask(),
                  bodyInRoi, dvidInfo);
            getCompleteDocument()->processObjectModified(
                  getCompleteDocument()->getBodyForSplit());
            getCompleteDocument()->notifyObjectModified();
          }
        } else {
          emit messageGenerated(
                ZWidgetMessage("Must enter split mode to enable crop.",
                               NeuTube::MSG_WARNING));
        }
      }
    }
  }
}

void ZFlyEmProofMvc::dropEvent(QDropEvent *event)
{
  QList<QUrl> urls = event->mimeData()->urls();
  bool processed = false;
  if (urls.size() == 1) {
    const QUrl &url = urls[0];
    QString filePath = NeuTube::GetFilePath(url);
    if (ZFileType::FileType(filePath.toStdString()) == ZFileType::JSON_FILE) {
      processed = true; //todo
    }
  }

  if (!processed) {
    foreach (const QUrl &url, urls) {
      QString filePath = NeuTube::GetFilePath(url);
      if (ZFileType::FileType(filePath.toStdString()) == ZFileType::SWC_FILE) {
        ZSwcTree *tree = new ZSwcTree;
        tree->load(filePath.toStdString());
        tree->setObjectClass(ZStackObjectSourceFactory::MakeFlyEmExtNeuronClass());
//        tree->setHittable(false);
        tree->setHitProtocal(ZStackObject::HIT_NONE);
        tree->setColor(QColor(255, 0, 0));
        getDocument()->addObject(tree);
      }
    }
  }

#if 0
  //Filter out tiff files
  QList<QUrl> imageUrls;
  QList<QUrl> nonImageUrls;

  foreach (QUrl url, urls) {
    if (ZFileType::isImageFile(url.path().toStdString())) {
      imageUrls.append(url);
    } else {
      nonImageUrls.append(url);
    }
  }
  if (!nonImageUrls.isEmpty()) {
    getDocument()->loadFileList(nonImageUrls);
  }
#endif
}
//void ZFlyEmProofMvc::toggleEdgeMode(bool edgeOn)

void ZFlyEmProofMvc::loadROIFunc()
{
  //
  if(m_ROILoaded)
    return;

  //
//  ZDvidReader reader;
  m_roiList.clear();
  m_loadedROIs.clear();
  m_roiSourceList.clear();

  //
  ZDvidReader reader;
  if (reader.open(getDvidTarget())) {
    ZJsonObject meta = reader.readInfo();

    //
    ZJsonValue datains = meta.value("DataInstances");

    if(datains.isObject())
    {
      ZJsonObject insList(datains);
      std::vector<std::string> keys = insList.getAllKey();

      for(std::size_t i=0; i<keys.size(); i++)
      {
        std::string roiName = keys.at(i);
        ZJsonObject roiJson(insList.value(roiName.c_str()));
        if (roiJson.hasKey("Base")) {
          ZJsonObject baseJson(roiJson.value("Base"));
          std::string typeName =
              ZJsonParser::stringValue(baseJson["TypeName"]);
          if (typeName != "roi") {
            roiName = "";
          }
        }

        if (!roiName.empty()) {
          ZObject3dScan roi = reader.readRoi(roiName);
          if (!roi.isEmpty()) {
            m_roiList.push_back(roiName);
            m_loadedROIs.push_back(roi);

            std::string source =
                ZStackObjectSourceFactory::MakeFlyEmRoiSource(roiName);
            m_roiSourceList.push_back(source);
          }
        }
      }
    }

    m_ROILoaded = true;

    emit roiLoaded();
  }
}

void ZFlyEmProofMvc::updateRoiWidget()
{
  //
  if(m_coarseBodyWindow)
  {
    m_coarseBodyWindow->getROIsDockWidget()->getROIs(
          m_coarseBodyWindow, getGrayScaleInfo(), m_roiList, m_loadedROIs,
          m_roiSourceList);
  }

  if(m_bodyWindow)
  {
    m_bodyWindow->getROIsDockWidget()->getROIs(
          m_bodyWindow, getGrayScaleInfo(), m_roiList, m_loadedROIs,
          m_roiSourceList);
  }

  if(m_externalNeuronWindow)
  {
    m_externalNeuronWindow->getROIsDockWidget()->getROIs(
          m_externalNeuronWindow, getGrayScaleInfo(), m_roiList, m_loadedROIs,
          m_roiSourceList);
  }

  if(m_skeletonWindow)
  {
    m_skeletonWindow->getROIsDockWidget()->getROIs(
          m_skeletonWindow, getGrayScaleInfo(), m_roiList, m_loadedROIs,
          m_roiSourceList);
  }
}

void ZFlyEmProofMvc::showInfoDialog()
{
  m_infoDlg->setText(getDvidTarget().toJsonObject().dumpString(2).c_str());
  m_infoDlg->show();
  m_infoDlg->raise();
}

void ZFlyEmProofMvc::getROIs()
{
  const QString threadId = "ZFlyEmProofMvc::loadROIFunc()";
  if (!m_futureMap.isAlive(threadId)) {
    m_futureMap.removeDeadThread();
    ZOUT(LINFO(), 3) << "Loading ROIs";
    QFuture<void> future =
        QtConcurrent::run(this, &ZFlyEmProofMvc::loadROIFunc);
    m_futureMap[threadId] = future;
  }
}

void ZFlyEmProofMvc::processSynapseVerification(int x, int y, int z, bool verified)
{
  getCompleteDocument()->downloadSynapse(x, y, z);
  if (m_protocolSwitcher != NULL) {
    m_protocolSwitcher->processSynapseVerification(x, y, z, verified);
  }
}

void ZFlyEmProofMvc::processSynapseMoving(
    const ZIntPoint &from, const ZIntPoint &to)
{
  getCompleteDocument()->syncMoveSynapse(from, to);
  if (m_protocolSwitcher != NULL) {
    m_protocolSwitcher->processSynapseMoving(from, to);
  }
}
