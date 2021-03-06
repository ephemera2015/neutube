#ifndef ZSTACKDOCDATABUFFER_H
#define ZSTACKDOCDATABUFFER_H

#include <QObject>
#include <QList>
#include <QMutex>

class ZStackObject;

class ZStackDocObjectUpdate {
public:
  enum EAction {
    ACTION_NULL, ACTION_ADD_NONUNIQUE, ACTION_ADD_UNIQUE, ACTION_EXPEL, ACTION_KILL,
    ACTION_RECYCLE, ACTION_UPDATE
  };

  ZStackDocObjectUpdate(ZStackObject *m_obj, EAction action);
  ~ZStackDocObjectUpdate();

  void reset();

  EAction getAction() const {
    return m_action;
  }

  ZStackObject* getObject() const {
    return m_obj;
  }

  void print() const;

private:
  ZStackObject *m_obj;
  EAction m_action;
};


class ZStackDocDataBuffer : public QObject
{
  Q_OBJECT
public:
  explicit ZStackDocDataBuffer(QObject *parent = 0);
  ~ZStackDocDataBuffer();

  void clearList();
  void clear();

  void addUpdate(ZStackObject *obj, ZStackDocObjectUpdate::EAction action);

  QList<ZStackDocObjectUpdate *> take();
  void deliver();

signals:
  void delivering();

public slots:

private:
  QList<ZStackDocObjectUpdate*> m_updateList;

  mutable QMutex m_mutex;
};

#endif // ZSTACKDOCDATABUFFER_H
