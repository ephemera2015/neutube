#ifndef ZDVIDTARGET_H
#define ZDVIDTARGET_H

#include <string>
#include <set>

#include "zjsonobject.h"
#include "zjsonarray.h"
#include "zdviddata.h"
#include "zdvidnode.h"
#include "zdviddef.h"

/*!
 * \brief The class of representing a dvid node.
 */
class ZDvidTarget
{
public:
  ZDvidTarget();
  ZDvidTarget(const std::string &address, const std::string &uuid, int port = -1);

  void clear();

  void set(const std::string &address, const std::string &uuid, int port = -1);
  void setServer(const std::string &address);
  void setUuid(const std::string &uuid);
  void setPort(int port);


  /*!
   * \brief Set dvid target from source string
   *
   * The old settings will be cleared no matter what. The source string is
   * http:address:port:uuid.
   *
   * \param sourceString Must start with "http:".
   */
  void setFromSourceString(const std::string &sourceString);

  void setFromSourceString(
      const std::string &sourceString, ZDvid::EDataType dataType);

  void setFromUrl(const std::string &url);

  inline const std::string& getAddress() const {
    return m_node.getAddress();
  }

  /*!
   * \brief Get the address with port
   *
   * \return "address[:port]" or empty if the address is empty.
   */
  std::string getAddressWithPort() const;

  inline const std::string& getUuid() const {
    return m_node.getUuid();
  }

  inline const std::string& getComment() const {
    return m_comment;
  }

  inline const std::string& getName() const {
    return m_name;
  }

  inline int getPort() const {
    return m_node.getPort();
  }

  const ZDvidNode& getNode() const {
    return m_node;
  }

  /*!
   * \brief Check if there is a port
   *
   * A valid port is any non-negative port number.
   *
   * \return true iff the port is available.
   */
  bool hasPort() const;

  inline void setName(const std::string &name) {
    m_name = name;
  }
  inline void setComment(const std::string &comment) {
    m_comment = comment;
  }

  std::string getUrl() const;
//  std::string getUrl(const std::string &dataName) const;

  /*!
   * \brief Get a single string to represent the target
   *
   * \a withHttpPrefix specifies whether the source string contains the "http:"
   * prefix or not.
   *
   * \return "[http:]address:port:uuid". Return empty if the address is empty.
   */
  std::string getSourceString(bool withHttpPrefix = true) const;

  /*!
   * \brief getBodyPath
   *
   * The functions does not check if a body exists.
   *
   * \return The path of a certain body.
   */
  std::string getBodyPath(uint64_t bodyId) const;

  /*!
   * \brief Test if the target is valid
   *
   * \return true iff the target is valid.
   */
  bool isValid() const;

  /*!
   * \brief Load json object
   */
  void loadJsonObject(const ZJsonObject &obj);
  ZJsonObject toJsonObject() const;

  void loadDvidDataSetting(const ZJsonObject &obj);
  ZJsonObject toDvidDataSetting() const;

  void print() const;

  //Special functions
  inline const std::string& getLocalFolder() const {
    return m_localFolder;
  }

  inline void setLocalFolder(const std::string &folder) {
    m_localFolder = folder;
  }

  std::string getLocalLowResGrayScalePath(
      int xintv, int yintv, int zintv) const;
  std::string getLocalLowResGrayScalePath(
      int xintv, int yintv, int zintv, int z) const;

  inline int getBgValue() const {
    return m_bgValue;
  }

  inline void setBgValue(int v) {
    m_bgValue = v;
  }

//  std::string getName(ZDvidData::ERole role) const;

  std::string getBodyLabelName() const;
  std::string getBodyLabelName(int zoom) const;
  void setBodyLabelName(const std::string &name);

  void setNullBodyLabelName();

  bool hasBodyLabel() const;
  bool hasLabelBlock() const;

  static std::string GetMultiscaleDataName(const std::string &dataName, int zoom);

  std::string getLabelBlockName() const;
  std::string getLabelBlockName(int zoom) const;
  std::string getValidLabelBlockName(int zoom) const;
  void setLabelBlockName(const std::string &name);

  void setNullLabelBlockName();

  std::string getBodyInfoName() const;

  std::string getMultiscale2dName() const;
  bool isTileLowQuality() const;
  bool hasTileData() const;

  void setMultiscale2dName(const std::string &name);
  void setDefaultMultiscale2dName();
  void configTile(const std::string &name, bool lowQuality);
//  void setLossTileName(const std::string &name);
//  std::string getLosslessTileName() const;
//  std::string getLossTileName() const;
  bool isLowQualityTile(const std::string &name) const;

  bool hasGrayScaleData() const;
  std::string getGrayScaleName() const;
  std::string getGrayScaleName(int zoom) const;
  std::string getValidGrayScaleName(int zoom) const;
  void setGrayScaleName(const std::string &name);

  std::string getRoiName() const;
  void setRoiName(const std::string &name);

  std::string getRoiName(size_t index) const;
  void addRoiName(const std::string &name);

  const std::vector<std::string>& getRoiList() const {
    return m_roiList;
  }

  void setRoiList(const std::vector<std::string> &roiList) {
    m_roiList = roiList;
  }

  std::string getSynapseName() const;
  void setSynapseName(const std::string &name);

  std::string getBookmarkName() const;
  std::string getBookmarkKeyName() const;
  std::string getSkeletonName() const;
  std::string getThumbnailName() const;

  std::string getTodoListName() const;
  void setTodoListName(const std::string &name);
  bool isDefaultTodoListName() const;

  std::string getBodyAnnotationName() const;

  std::string getSplitLabelName() const;

  const std::set<std::string>& getUserNameSet() const;
  //void setUserName(const std::string &name);

  static bool isDvidTarget(const std::string &source);

  inline bool isSupervised() const { return m_isSupervised; }
  void enableSupervisor(bool on) {
    m_isSupervised = on;
  }
  const std::string& getSupervisor() const { return m_supervisorServer; }
  void setSupervisorServer(const std::string &server) {
    m_supervisorServer = server;
  }

  inline bool isEditable() const { return m_isEditable; }
  void setEditable(bool on) { m_isEditable = on; }

  inline bool readOnly() const { return m_readOnly; }
  void setReadOnly(bool readOnly) {
    m_readOnly = readOnly;
  }

  int getMaxLabelZoom() const {
    return m_maxLabelZoom;
  }

  void setMaxLabelZoom(int zoom) {
    m_maxLabelZoom = zoom;
  }

  int getMaxGrayscaleZoom() const {
    return m_maxGrayscaleZoom;
  }

  void setMaxGrayscaleZoom(int zoom) {
    m_maxGrayscaleZoom = zoom;
  }

  bool usingMulitresBodylabel() const;

  /*
  void setLabelszName(const std::string &name);
  std::string getLabelszName() const;
  */

  std::string getSynapseLabelszName() const;
  void setSynapseLabelszName(const std::string &name);

  bool usingDefaultDataSetting() const {
    return m_usingDefaultSetting;
  }

  void useDefaultDataSetting(bool on) {
    m_usingDefaultSetting = on;
  }

  void setSourceConfig(const ZJsonObject &config);

  /*!
   * \brief Set dvid source of grayscale data
   *
   * If \a node is invalid, the source will be set to the main source.
   *
   * \param node
   */
  void setGrayScaleSource(const ZDvidNode &node);
  void setTileSource(const ZDvidNode &node);
  void prepareGrayScale();
  void prepareTile();

  ZDvidNode getGrayScaleSource() const;
  ZDvidNode getTileSource() const;

private:
  void init();
  void setSource(const char *key, const ZDvidNode &node);
  ZDvidNode getSource(const char *key) const;

private:
  ZDvidNode m_node;
  std::string m_name;
  std::string m_comment;
  std::string m_localFolder;
  std::string m_bodyLabelName;
  std::string m_labelBlockName;
  std::string m_multiscale2dName; //default lossless tile name
  ZJsonObject m_tileConfig; //used when m_multiscale2dName is empty
  ZJsonObject m_sourceConfig;
  std::string m_grayScaleName;
  std::string m_synapseLabelszName;
  std::string m_roiName;
  std::string m_todoListName;
  std::vector<std::string> m_roiList;
  std::string m_synapseName;
  std::set<std::string> m_userList;
  bool m_isSupervised;
  std::string m_supervisorServer;
  int m_maxLabelZoom;
  int m_maxGrayscaleZoom;
  bool m_usingMultresBodyLabel;
  bool m_usingDefaultSetting;
//  std::string m_userName;
//  std::string m_tileName;

  int m_bgValue; //grayscale background

  bool m_isEditable;
  bool m_readOnly;

  const static char* m_commentKey;
  const static char* m_nameKey;
  const static char* m_localKey;
  const static char* m_debugKey;
  const static char* m_bgValueKey;
  const static char* m_grayScaleNameKey;
  const static char* m_bodyLabelNameKey;
  const static char* m_labelBlockNameKey;
  const static char* m_multiscale2dNameKey;
  const static char* m_tileConfigKey;
  const static char* m_roiListKey;
  const static char* m_roiNameKey;
  const static char* m_synapseNameKey;
  const static char* m_defaultSettingKey;
  const static char* m_userNameKey;
  const static char* m_supervisorKey;
  const static char* m_supervisorServerKey;
  const static char* m_maxLabelZoomKey;
  const static char* m_synapseLabelszKey;
  const static char* m_todoListNameKey;
  const static char* m_sourceConfigKey;
};

#endif // ZDVIDTARGET_H
