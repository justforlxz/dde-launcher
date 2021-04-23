/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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

#ifndef BACKGROUNDMANAGER_H
#define BACKGROUNDMANAGER_H

#include <QObject>
#include <QDesktopWidget>
#include <QScreen>
#include <QGSettings>

#include <com_deepin_wm.h>
#include <com_deepin_daemon_imageeffect.h>
#include <com_deepin_daemon_imageblur.h>
#include <com_deepin_daemon_appearance.h>
#include <com_deepin_daemon_display.h>

#define CUSTOM_MODE     0
#define MERGE_MODE      1
#define EXTEND_MODE     2
#define SINGLE_MODE     3

using ImageEffeblur = com::deepin::daemon::ImageBlur;
using ImageEffectInter = com::deepin::daemon::ImageEffect;
using AppearanceInter = com::deepin::daemon::Appearance;
using DisplayInter = com::deepin::daemon::Display;

class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    explicit BackgroundManager(QObject *parent = nullptr);

    int dispalyMode() const { return m_displayMode; }

    inline bool useWorkSpaceWallpaper() const {
        return m_useWorkSpaceWallpaper;
    }

private:
    void getImageDataFromDbus(const QString &filePath);

signals:
    void currentWorkspaceBackgroundChanged(const QString &background);
    void currentWorkspaceBlurBackgroundChanged(const QString &background);
    void useWorkSpaceWallpaperChanged(bool useWorkspaceWallpaper);

public slots:
    void updateBlurBackgrounds();

private:
    void setUseWorkSpaceWallpaper(bool useWorkSpaceWallpaper);

private:
    int m_currentWorkspace;
    mutable QString m_blurBackground;
    mutable QString m_background;

    QPointer<com::deepin::wm> m_wmInter;
    QPointer<ImageEffectInter> m_imageEffectInter;
    QPointer<ImageEffeblur> m_imageblur;
    QPointer<AppearanceInter> m_appearanceInter;
    QPointer<DisplayInter> m_displayInter;
    QPointer<QGSettings> m_gsettings;
    int m_displayMode;
    bool m_useWorkSpaceWallpaper;
};

#endif // BACKGROUNDMANAGER_H
