#include <iostream>
#include <cstring>
#include <QApplication>
#include <QProcess>
#include <QDir>

#ifdef _QT5_
#include <QSurfaceFormat>
#endif

#include "mainwindow.h"
#include "QsLog/QsLog.h"
#include "QsLog/QsLogDest.h"
#include "zcommandline.h"
#include "zerror.h"
#include "z3dapplication.h"
#include "zneurontracer.h"
#include "zapplication.h"

#include "ztest.h"

#include "tz_utilities.h"
#include "neutubeconfig.h"
#include "zneurontracerconfig.h"
#include "sandbox/zsandboxproject.h"
#include "sandbox/zsandbox.h"

#if 0
#ifdef _QT5_
#include <QSurfaceFormat>

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
  switch (type) {
  case QtDebugMsg:
    LDEBUGF(context.file, context.line, context.function) << msg;
    break;
  case QtWarningMsg:
    LWARNF(context.file, context.line, context.function) << msg;
    break;
  case QtCriticalMsg:
    LERRORF(context.file, context.line, context.function) << msg;
    break;
  case QtFatalMsg:
    LFATALF(context.file, context.line, context.function) << msg;
    abort();
  default:
    break;
  }
}
#else
void myMessageOutput(QtMsgType type, const char *msg)
{
  switch (type) {
  case QtDebugMsg:
    LDEBUG_NLN() << msg;
    break;
  case QtWarningMsg:
    LWARN_NLN() << msg;
    break;
  case QtCriticalMsg:
    LERROR_NLN() << msg;
    break;
  case QtFatalMsg:
    LFATAL_NLN() << msg;
    abort();
  }
}
#endif    // qt version > 5.0.0
#endif

static void syncLogDir(const std::string &srcDir, const std::string &destDir)
{
  if (!srcDir.empty() && !destDir.empty() && srcDir != destDir) {
    QDir dir(srcDir.c_str());
    dir.setFilter(QDir::Files | QDir::NoSymLinks);
    QFileInfoList infoList =
        dir.entryInfoList(QStringList() << "*.txt.*" << "*.txt");

    foreach (const QFileInfo &info, infoList) {
      QString command =
          ("rsync -uv " + srcDir + "/" + info.fileName().toStdString() +
           " " + destDir + "/").c_str();
      std::cout << command.toStdString() << std::endl;
      QProcess process;
      process.start(command);
      process.waitForFinished(-1);
      QString errorOutput = process.readAllStandardError();
      QString standardOutout = process.readAllStandardOutput();
      std::cout << errorOutput.toStdString() << std::endl;
      std::cout << standardOutout.toStdString() << std::endl;
    }
  }
}

#ifdef _CLI_VERSION
int main(int argc, char *argv[])
{
  if (argc > 1 && strcmp(argv[1], "--command") == 0)
  {
    return ZCommandLine().run(argc,argv);
  }
  else
  {
    std::cout<<"This is CLI version of neutu,please use --command option."<<std::endl;
    return 1;
  }
}
#else
int main(int argc, char *argv[])
{
#if 0 //Disable redirect for explicit logging
#ifndef _FLYEM_
#ifdef _QT5_
  qInstallMessageHandler(myMessageOutput);
#else
  qInstallMsgHandler(myMessageOutput);
#endif
#endif
#endif

  bool debugging = false;
  bool unitTest = false;
  bool runCommandLine = false;

  bool guiEnabled = true;

  QString configPath;
  QStringList fileList;

  if (argc > 1) {
    if (strcmp(argv[1], "d") == 0) {
      debugging = true;
    }

    if (strcmp(argv[1], "--command") == 0) {
      runCommandLine = true;
    }

    if (runCommandLine) {
      ZCommandLine cmd;
      return cmd.run(argc, argv);
    }

    if (strcmp(argv[1], "u") == 0 || QString(argv[1]).startsWith("--gtest")) {
      unitTest = true;
      debugging = true;
    }

    if (strcmp(argv[1], "--load") == 0) {
      for (int i = 2; i < argc; ++i) {
        fileList << argv[i];
      }
    }

    if (QString(argv[1]).endsWith(".json")) {
      configPath = argv[1];
    }
  }
  if (debugging || runCommandLine) {
    guiEnabled = false;
  }

  if (guiEnabled) {
#ifdef _QT5_
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
#endif
    QCoreApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);

#ifdef _QT5_
    QSurfaceFormat format;
    //format.setStereo(true);
    QSurfaceFormat::setDefaultFormat(format);
#endif
  }

  // call first otherwise it will cause runtime warning: Please instantiate the QApplication object first
  QApplication app(argc, argv, guiEnabled);

  NeuTube::RegisterMetaType();

  //load config
  NeutubeConfig &config = NeutubeConfig::getInstance();
  std::cout << QApplication::applicationDirPath().toStdString() << std::endl;
  config.setApplicationDir(QApplication::applicationDirPath().toStdString());
  if (config.load(config.getConfigPath()) == false) {
    std::cout << "Unable to load configuration: "
              << config.getConfigPath() << std::endl;
  }

  if (configPath.isEmpty()) {
    configPath =
        QFileInfo(QDir((GET_APPLICATION_DIR + "/json").c_str()), "config.json").
        absoluteFilePath();
//        ZString::fullPath(
//          GET_APPLICATION_DIR, "json", "", "config.json").c_str();
  }

  LINFO() << "Config path: " << configPath;

  ZJsonObject configObj;
  if (!configPath.isEmpty()) {
    configObj.load(configPath.toStdString());
  }

#ifdef _FLYEM_
  GET_FLYEM_CONFIG.useDefaultConfig(NeutubeConfig::UsingDefaultFlyemConfig());
  QString defaultFlyemConfigPath = QFileInfo(
        QDir((GET_APPLICATION_DIR + "/json").c_str()), "flyem_config.json").
      absoluteFilePath();
  GET_FLYEM_CONFIG.setDefaultConfigPath(defaultFlyemConfigPath.toStdString());

  QString flyemConfigPath = NeutubeConfig::GetFlyEmConfigPath();
  if (flyemConfigPath.isEmpty()) {
    QFileInfo configFileInfo(configPath);

    flyemConfigPath = ZJsonParser::stringValue(configObj["flyem"]);
    if (flyemConfigPath.isEmpty()) {
      flyemConfigPath = defaultFlyemConfigPath;
    } else {
      QFileInfo flyemConfigFileInfo(flyemConfigPath);
      if (!flyemConfigFileInfo.isAbsolute()) {
        flyemConfigPath =
            configFileInfo.absoluteDir().absoluteFilePath(flyemConfigPath);
      }
    }
  }

  GET_FLYEM_CONFIG.setConfigPath(flyemConfigPath.toStdString());
  GET_FLYEM_CONFIG.loadConfig();

#ifdef _DEBUG_
  std::cout << config.GetNeuTuServer().toStdString() << std::endl;
#endif

  if (config.GetNeuTuServer().isEmpty()) {
    QString neutuServer = ZJsonParser::stringValue(configObj["neutu_server"]);
    if (!neutuServer.isEmpty()) {
      GET_FLYEM_CONFIG.setServer(neutuServer.toStdString());
    }
  } else {
    GET_FLYEM_CONFIG.setServer(config.GetNeuTuServer().toStdString());
  }
#endif

  if (!runCommandLine) { //Command line mode takes care of configuration independently
    ZNeuronTracerConfig &tracingConfig = ZNeuronTracerConfig::getInstance();
    tracingConfig.load(config.getApplicatinDir() + "/json/trace_config.json");

    //Sync log files
    syncLogDir(NeutubeConfig::getInstance().getPath(NeutubeConfig::LOG_DEST_DIR),
               NeutubeConfig::getInstance().getPath(NeutubeConfig::LOG_DIR));

    if (GET_APPLICATION_NAME == "Biocytin") {
      tracingConfig.load(
            config.getApplicatinDir() + "/json/trace_config_biocytin.json");
    } else {
      tracingConfig.load(config.getApplicatinDir() + "/json/trace_config.json");
    }
  }

#ifdef _DEBUG_
  config.print();
#endif

  // init the logging mechanism
  QsLogging::Logger& logger = QsLogging::Logger::instance();
  const QString sLogPath(
        NeutubeConfig::getInstance().getPath(NeutubeConfig::LOG_FILE).c_str());
  const QString traceLogPath(
        NeutubeConfig::getInstance().getPath(NeutubeConfig::LOG_TRACE).c_str());

#ifdef _FLYEM_
  int maxLogCount = 100;
#else
  int maxLogCount = 10;
#endif

  QsLogging::DestinationPtr fileDestination(
        QsLogging::DestinationFactory::MakeFileDestination(
          sLogPath, QsLogging::EnableLogRotation,
          QsLogging::MaxSizeBytes(5e7), QsLogging::MaxOldLogCount(maxLogCount)));
  QsLogging::DestinationPtr traceFileDestination(
        QsLogging::DestinationFactory::MakeFileDestination(
          traceLogPath, QsLogging::EnableLogRotation,
          QsLogging::MaxSizeBytes(2e7), QsLogging::MaxOldLogCount(10),
          QsLogging::TraceLevel));
  QsLogging::DestinationPtr debugDestination(
        QsLogging::DestinationFactory::MakeDebugOutputDestination());
  logger.addDestination(debugDestination);
  logger.addDestination(traceFileDestination);
  logger.addDestination(fileDestination);
#if defined _DEBUG_
  logger.setLoggingLevel(QsLogging::DebugLevel);
#else
  logger.setLoggingLevel(QsLogging::InfoLevel);
#endif

  if (NeutubeConfig::GetVerboseLevel() >= 5) {
    logger.setLoggingLevel(QsLogging::TraceLevel);
  }

  RECORD_INFORMATION("************* Start ******************");

  if (guiEnabled) {
#if defined __APPLE__        //use macdeployqt
#else
#if defined(QT_NO_DEBUG)
    QDir dir(QApplication::applicationDirPath());
    dir.cd("plugins");
    QApplication::addLibraryPath(dir.absolutePath());  // for windows version
    dir.cdUp();
    dir.cdUp();
    dir.cd("plugins");
    QApplication::addLibraryPath(dir.absolutePath());
    dir.cdUp();
    dir.cd("lib");
    QApplication::addLibraryPath(dir.absolutePath());
#endif
#endif

    MainWindow::createWorkDir();
    NeutubeConfig::UpdateAutoSaveDir();

#if (defined __APPLE__) && !(defined _QT5_)
    app.setGraphicsSystem("raster");
#endif

    LINFO() << "Start " + GET_SOFTWARE_NAME + " - " + GET_APPLICATION_NAME;

    ZTest::getInstance().setCommandLineArg(argc, argv);

    // init 3D
    //std::cout << "Initializing 3D ..." << std::endl;
    RECORD_INFORMATION("Initializing 3D ...");
    Z3DApplication z3dApp(QCoreApplication::applicationDirPath());
    z3dApp.initialize();

    MainWindow *mainWin = new MainWindow();
    mainWin->configure();
    mainWin->show();
    mainWin->raise();
    mainWin->initOpenglContext();

    if (!fileList.isEmpty()) {
      mainWin->showStackFrame(fileList, true);
    }

    if (argc > 1) {
      mainWin->processArgument(argv[1]);
    } /*else {
      mainWin->processArgument(QString("test %1: %2").arg(argc).arg(argv[0]));
    }*/

    ZSandbox::SetMainWindow(mainWin);
    ZSandboxProject::InitSandbox();

#if defined(_FLYEM_) && !defined(_DEBUG_)
    mainWin->startProofread();
#else
    mainWin->show();
    mainWin->raise();
#endif


    int result = 1;

    try {
      result = app.exec();
    } catch (std::exception &e) {
      LERROR() << "Crashed by exception:" << e.what();
    }

    delete mainWin;
    z3dApp.deinitializeGL();
    z3dApp.deinitialize();

    if (!runCommandLine) {
      //Sync log files
      syncLogDir(NeutubeConfig::getInstance().getPath(NeutubeConfig::LOG_DIR),
                 NeutubeConfig::getInstance().getPath(NeutubeConfig::LOG_DEST_DIR));
    }

    return result;
  } else {
    /*
    if (runCommandLine) {
      ZCommandLine cmd;
      return cmd.run(argc, argv);
    }
    */

    /********* for debugging *************/

#ifndef QT_NO_DEBUG
    /*
    std::cout << "Debugging ..." << std::endl;
    ZCurve curve;
    double *array = new double[10];
    for (int i = 0; i < 10; i++) {
      array[i] = -i * i;
    }
    curve.loadArray(array, 10);
    std::cout << curve.minY() << std::endl;
    */
    if (unitTest) {
      ZTest::RunUnitTest(argc, argv);
    }
#else
    if (unitTest) {
      std::cout << "No unit test in the release version." << std::endl;
    }
#endif
    if (!unitTest) {
      std::cout << "Running test function" << std::endl;
      ZTest::test(NULL);
    }

    return 1;
  }
}
#endif
