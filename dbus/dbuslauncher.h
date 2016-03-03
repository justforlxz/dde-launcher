/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -c DBusLauncher -p dbuslauncher com.deepin.dde.daemon.Launcher.xml
 *
 * qdbusxml2cpp is Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef DBUSLAUNCHER_H_1456917835
#define DBUSLAUNCHER_H_1456917835

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

#include "categoryinfo.h"
#include "frequencyinfo.h"
#include "iteminfo.h"
#include "installedtimeinfo.h"

/*
 * Proxy class for interface com.deepin.dde.daemon.Launcher
 */
class DBusLauncher: public QDBusAbstractInterface
{
    Q_OBJECT

    Q_SLOT void __propertyChanged__(const QDBusMessage& msg)
    {
        QList<QVariant> arguments = msg.arguments();
        if (3 != arguments.count())
            return;
        QString interfaceName = msg.arguments().at(0).toString();
        if (interfaceName !="com.deepin.dde.daemon.Launcher")
            return;
        QVariantMap changedProps = qdbus_cast<QVariantMap>(arguments.at(1).value<QDBusArgument>());
        foreach(const QString &prop, changedProps.keys()) {
        const QMetaObject* self = metaObject();
            for (int i=self->propertyOffset(); i < self->propertyCount(); ++i) {
                QMetaProperty p = self->property(i);
                if (p.name() == prop) {
 	            Q_EMIT p.notifySignal().invoke(this);
                }
            }
        }
   }
public:
    static inline const char *staticInterfaceName()
    { return "com.deepin.dde.daemon.Launcher"; }

public:
    explicit DBusLauncher(QObject *parent = 0);

    ~DBusLauncher();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<CategoryInfoList> GetAllCategoryInfos()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("GetAllCategoryInfos"), argumentList);
    }

    inline QDBusPendingReply<FrequencyInfoList> GetAllFrequency()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("GetAllFrequency"), argumentList);
    }

    inline QDBusPendingReply<ItemInfoList> GetAllItemInfos()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("GetAllItemInfos"), argumentList);
    }

    inline QDBusPendingReply<QStringList> GetAllNewInstalledApps()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("GetAllNewInstalledApps"), argumentList);
    }

    inline QDBusPendingReply<InstalledTimeInfoList> GetAllTimeInstalled()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("GetAllTimeInstalled"), argumentList);
    }

Q_SIGNALS: // SIGNALS
// begin property changed signals
};

namespace com {
  namespace deepin {
    namespace dde {
      namespace daemon {
        typedef ::DBusLauncher Launcher;
      }
    }
  }
}
#endif
