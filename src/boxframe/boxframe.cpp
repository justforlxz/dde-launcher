/*
 * Copyright (C) 2015 ~ 2018 Deepin Technology Co., Ltd.
 *
 * Author:     sbw <sbw@sbw.so>
 *
 * Maintainer: sbw <sbw@sbw.so>
 *             kirigaya <kirigaya@mkacg.com>
 *             Hualet <mr.asianwang@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "boxframe.h"
#include "backgroundmanager.h"

#include <QDebug>
#include <QUrl>
#include <QFile>
#include <QApplication>
#include <QScreen>
#include <QPainter>
#include <QPaintEvent>

BoxFrame::BoxFrame(QWidget *parent)
    : DBlurEffectWidget(parent)
    , m_defaultBg("/usr/share/backgrounds/default_background.jpg")
    , m_bgManager(new BackgroundManager(this))
{
    QPixmapCache::setCacheLimit(10240000);
    connect(m_bgManager, &BackgroundManager::currentWorkspaceBackgroundChanged, this, &BoxFrame::setBackground);
    connect(m_bgManager, &BackgroundManager::currentWorkspaceBlurBackgroundChanged, this, &BoxFrame::setBlurBackground);
    connect(m_bgManager, &BackgroundManager::useWorkSpaceWallpaperChanged, this,  &BoxFrame::updateBackground);
}

BoxFrame::~BoxFrame()
{
}

// Message for maintainers: DON'T use QSS to set the background !

// This function takes ~2ms by setting QSS to set backgrounds, but causes show() of
// ShutdownFrame takes ~260ms to complete. On the other hand, this function takes
// ~130ms by setting pixmap, yet takes only ~12ms to complete the show() of ShutdownFrame.
// It'll be more obvious on dual screens environment.
void BoxFrame::setBackground(const QString &url)
{
    if (m_lastUrl == url)
        return;

    m_lastUrl = url;

    m_pixmap = QPixmap(url);
    if (m_pixmap.isNull())
        m_pixmap.load(m_defaultBg);

    updateBackground();
}

void BoxFrame::setBlurBackground(const QString &url)
{
    if (m_lastBlurUrl == url)
        return;

    m_lastBlurUrl = url;

    QPixmap pix(url);
    if (pix.isNull())
        pix.load(m_defaultBg);

    const QSize &size = currentScreen()->size() ;//* currentScreen()->devicePixelRatio();

    QPixmap cache = pix.scaled(size,
                               Qt::KeepAspectRatioByExpanding,
                               Qt::SmoothTransformation);

    QRect copyRect((cache.width() - size.width()) / 2,
                   (cache.height() - size.height()) / 2,
                   size.width(), size.height());
    cache = cache.copy(copyRect);
    cache.setDevicePixelRatio(devicePixelRatioF());
    emit backgroundImageChanged(cache);
}

QString BoxFrame::text() const
{
    return "LauncherFrame";
}

/**
 * @brief BoxFrame::backgroundPixmap 缓存桌面背景,减少40+ms启动器显示耗时
 * @return 桌面背景图片对象
 */
const QPixmap BoxFrame::backgroundPixmap()
{
    const QSize &size = currentScreen()->size() * currentScreen()->devicePixelRatio();

    if (m_pixmap.isNull()) {
        QPixmapCache::remove(m_cacheKey);
        return QPixmap();
    }

    QPixmap cache;
    if (!QPixmapCache::find(m_cacheKey, &cache)) {
        cache = m_pixmap.scaled(size,
                                Qt::KeepAspectRatioByExpanding,
                                Qt::SmoothTransformation);
        m_cacheKey = QPixmapCache::insert(cache);
    }

    QRect copyRect((cache.width() - size.width()) / 2,
                   (cache.height() - size.height()) / 2,
                   size.width(), size.height());

    cache = cache.copy(copyRect);
    cache.setDevicePixelRatio(devicePixelRatioF());
    return cache;
}

const QScreen *BoxFrame::currentScreen()
{
    if (m_bgManager->dispalyMode() == MERGE_MODE)
        return qApp->primaryScreen();

    QDesktopWidget *desktopwidget = QApplication::desktop();
    int screenIndex = desktopwidget->screenNumber(this);
    QList<QScreen *> screens = qApp->screens();
    if (screenIndex < screens.count()) {
        return screens[screenIndex];
    }

//    qDebug() << "index out of bound in BoxFrame::currentScreen method";
    return qApp->primaryScreen();
}

void BoxFrame::updateBlurBackground()
{
    QPixmap pix(m_lastBlurUrl);

    if (pix.isNull()) {
        pix.load(m_defaultBg);
    }

    const QSize &size = currentScreen()->size() ;//* currentScreen()->devicePixelRatio();

    QPixmap cache = pix.scaled(size,
                                    Qt::KeepAspectRatioByExpanding,
                                    Qt::SmoothTransformation);
    QRect copyRect((cache.width() - size.width()) / 2,
                   (cache.height() - size.height()) / 2,
                   size.width(), size.height());
    cache = cache.copy(copyRect);
    cache.setDevicePixelRatio(devicePixelRatioF());
    emit backgroundImageChanged(cache);
}

void BoxFrame::updateBackground()
{
    if (!m_bgManager->useWorkSpaceWallpaper()) {
        return update();
    }

    m_cache = backgroundPixmap();
    update();
}

void BoxFrame::paintEvent(QPaintEvent *event)
{
    if (!m_bgManager->useWorkSpaceWallpaper()) {
        return DBlurEffectWidget::paintEvent(event);
    }

    QPainter painter(this);

    QScreen const *s = currentScreen();
    const QRect &geom = s->geometry();
    QRect tr(QPoint(0, 0), geom.size());

    painter.drawPixmap(tr,
                       m_cache,
                       QRect(tr.topLeft(),
                             tr.size() * m_cache.devicePixelRatioF()));
}

void BoxFrame::moveEvent(QMoveEvent *event)
{
    if (m_bgManager->useWorkSpaceWallpaper()) {
        m_bgManager->updateBlurBackgrounds();
    }

    DBlurEffectWidget::moveEvent(event);
}
