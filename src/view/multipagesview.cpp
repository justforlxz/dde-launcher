/*
 * Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
 *
 * Author:     wangleiyong <wangleiyong_cm@deepin.com>
 *
 * Maintainer: wangleiyong <wangleiyong_cm@deepin.com>
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

#include "multipagesview.h"
#include "../global_util/constants.h"
#include "../fullscreenframe.h"
#include "../widgets/blurboxwidget.h"

#include <QHBoxLayout>
#define     SIDES_SPACE_SCALE   0.10

MultiPagesView::MultiPagesView(AppsListModel::AppCategory categoryModel, QWidget *parent)
    : QWidget(parent)
    , m_pLeftGradient(new GradientLabel(this))
    , m_pRightGradient(new GradientLabel(this))
    , m_pageCount(0)
    , m_pageIndex(0)
    , m_category(categoryModel)
    , m_appsManager(AppsManager::instance())
    , m_calcUtil(CalculateUtil::instance())
    , m_appListArea(new AppListArea)
    , m_viewBox(new DHBoxWidget)
    , m_pageControl(new pageControl)
{
    m_bDragStart = false;

    m_bMousePress = false;
    m_nMousePos = 0;
    m_scrollValue = 0;
    m_scrollStart = 0;

    // 测试用
    m_pRightGradient->setAccessibleName("thisRightGradient");
    m_pLeftGradient->setAccessibleName("thisLeftGradient");
    m_pageControl->setAccessibleName("pageControl");

    // 滚动区域
    m_appListArea->setObjectName("MultiPageBox");
    m_appListArea->viewport()->setAutoFillBackground(false);
    m_appListArea->setWidgetResizable(true);
    m_appListArea->setFocusPolicy(Qt::NoFocus);
    m_appListArea->setFrameStyle(QFrame::NoFrame);
    m_appListArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_appListArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_appListArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_appListArea->viewport()->installEventFilter(this);
    m_appListArea->installEventFilter(this);

    // 翻页按钮和动画
    m_pageSwitchAnimation = new QPropertyAnimation(m_appListArea->horizontalScrollBar(), "value");
    m_pageSwitchAnimation->setEasingCurve(QEasingCurve::OutQuad);

    InitUI();

    connect(m_appListArea, &AppListArea::increaseIcon, this, [ = ] { m_calcUtil->increaseIconSize(); emit m_appsManager->layoutChanged(AppsListModel::All); });
    connect(m_appListArea, &AppListArea::decreaseIcon, this, [ = ] { m_calcUtil->decreaseIconSize(); emit m_appsManager->layoutChanged(AppsListModel::All); });
    connect(m_pageControl, &pageControl::onPageChanged, this, &MultiPagesView::showCurrentPage);
}

void MultiPagesView::updateGradient(QPixmap &pixmap, QPoint topLeftImg, QPoint topRightImg)
{
    m_pLeftGradient->setDirection(GradientLabel::LeftToRight);
    m_pRightGradient->setDirection(GradientLabel::RightToLeft);

    const qreal ratio = devicePixelRatioF();
    pixmap.setDevicePixelRatio(1);

    int nWidth = DLauncher::TOP_BOTTOM_GRADIENT_HEIGHT * m_calcUtil->getScreenScaleX();
    QSize gradientSize(nWidth, height());

    QPoint topLeft = mapTo(this, QPoint(0, 0));
    QRect topRect(topLeftImg * ratio, gradientSize * ratio);
    QPixmap topCache = pixmap.copy(topRect);
    topCache.setDevicePixelRatio(ratio);

    m_pLeftGradient->setPixmap(topCache);
    m_pLeftGradient->resize(gradientSize);
    m_pLeftGradient->move(topLeft);
    m_pLeftGradient->raise();

    QPoint topRight(topLeft.x() + width() - gradientSize.width(), topLeft.y());
    QPoint imgTopRight(topRightImg.x() - gradientSize.width(), topRightImg.y());

    QRect RightRect(imgTopRight * ratio, gradientSize * ratio);
    QPixmap bottomCache = pixmap.copy(RightRect);

    m_pRightGradient->setPixmap(bottomCache);
    m_pRightGradient->resize(gradientSize);
    m_pRightGradient->move(topRight);
    m_pRightGradient->raise();
    setGradientVisible(true);
}

void MultiPagesView::updatePageCount(AppsListModel::AppCategory category)
{
    int pageCount = m_appsManager->getPageCount(category == AppsListModel::All ? m_category : category);
    if (pageCount < 1)
        pageCount = 1;

    if (pageCount == m_pageCount)
        return;

    if (pageCount > m_pageCount) {
        while (pageCount > m_pageCount) {
            AppsListModel *pModel = new AppsListModel(m_category);
            pModel->setPageIndex(m_pageCount);
            m_pageAppsModelList.push_back(pModel);

            AppGridView *pageView = new AppGridView;
            pageView->setModel(pModel);
            pageView->setItemDelegate(m_delegate);
            pageView->setContainerBox(m_appListArea);
            pageView->installEventFilter(this);
            pageView->setDelegate(this);
            m_appGridViewList.push_back(pageView);

            m_viewBox->layout()->insertWidget(m_pageCount, pageView);

            m_pageCount ++;

            connect(pageView, &AppGridView::requestScrollLeft, this, &MultiPagesView::dragToLeft);
            connect(pageView, &AppGridView::requestScrollRight, this, &MultiPagesView::dragToRight);
            connect(pageView, &AppGridView::requestScrollStop, [this] {
                m_bDragStart = false;
                setGradientVisible(false);
            });
            connect(pageView, &AppGridView::dragEnd, this, &MultiPagesView::dragStop);
            connect(m_pageSwitchAnimation, &QPropertyAnimation::finished,pageView,&AppGridView::setDragAnimationEnable);
            connect(m_pageSwitchAnimation, &QPropertyAnimation::finished,this, [ = ]{
                    setGradientVisible(false);
            });
            emit connectViewEvent(pageView);
        }
    } else {
        while (pageCount < m_pageCount) {
            AppGridView *pageView = qobject_cast<AppGridView *>(m_viewBox->layout()->itemAt(m_pageCount - 1)->widget());
            m_viewBox->layout()->removeWidget(pageView);
            pageView->deleteLater();

            m_pageAppsModelList.removeLast();
            m_appGridViewList.removeLast();

            m_pageCount --;
        }
    }

    m_pageControl->setPageCount(m_pageCount > 1 ? pageCount : 0);
}

void MultiPagesView::dragToLeft(const QModelIndex &index)
{
    Q_UNUSED(index);
    if (m_pageIndex <= 0)
        return;

    if (m_pageSwitchAnimation->state() == QPropertyAnimation::Running || m_bDragStart)
        return;

    m_appGridViewList[m_pageIndex]->dragOut(-1);

    showCurrentPage(m_pageIndex - 1);

    int lastApp = m_pageAppsModelList[m_pageIndex]->rowCount(QModelIndex());
    QModelIndex firstModel = m_appGridViewList[m_pageIndex]->indexAt(lastApp - 1);
    m_appGridViewList[m_pageIndex]->dragIn(firstModel);

    m_bDragStart = true;
}

void MultiPagesView::dragToRight(const QModelIndex &index)
{
    Q_UNUSED(index);
    if (m_pageIndex >= m_pageCount - 1)
        return;

    if (m_pageSwitchAnimation->state() == QPropertyAnimation::Running || m_bDragStart)
        return;

    int newPos = m_calcUtil->appPageItemCount(m_category);
    m_appGridViewList[m_pageIndex]->dragOut(newPos);

    showCurrentPage(m_pageIndex + 1);

    QModelIndex firstModel = m_appGridViewList[m_pageIndex]->indexAt(0);
    m_appGridViewList[m_pageIndex]->dragIn(firstModel);

    m_bDragStart = true;
}

void MultiPagesView::dragStop()
{
    if (sender() == m_appGridViewList[m_pageIndex])
        return;

    m_appGridViewList[m_pageIndex]->flashDrag();
}

QModelIndex MultiPagesView::getAppItem(int index)
{
    return m_appGridViewList[m_pageIndex]->indexAt(index);
}

void MultiPagesView::setDataDelegate(QAbstractItemDelegate *delegate)
{
    m_delegate = delegate;
}

void MultiPagesView::ShowPageView(AppsListModel::AppCategory category)
{
    int pageCount = m_appsManager->getPageCount(category);
    for (int i = 0; i < qMax(pageCount, m_pageCount); i++) {
        m_appGridViewList[i]->setVisible(i < pageCount);
        m_pageAppsModelList[i]->setCategory(category);
    }
    m_pageControl->setPageCount(pageCount > 1 ? pageCount : 0);
    m_pageCount = pageCount;
    m_category = category;
}

void MultiPagesView::setModel(AppsListModel::AppCategory category)
{
    for (int i = 0; i < m_pageCount; i++) {
        m_pageAppsModelList[i]->setCategory(category);
        m_appGridViewList[i]->setModel(m_pageAppsModelList[i]);
    }
}

void MultiPagesView::updatePosition()
{
    m_pageControl->UpdateIconSize(m_calcUtil->getScreenScaleX(), m_calcUtil->getScreenScaleY());
    QSize tmpSize = size() - QSize(0,m_pageControl->height() + DLauncher::DRAG_THRESHOLD);
    m_appListArea->setFixedSize(tmpSize);
    m_viewBox->setFixedHeight(tmpSize.height());
    for (auto *pView : m_appGridViewList)
        pView->setFixedSize(tmpSize);

    showCurrentPage(0);
}

void MultiPagesView::InitUI()
{
    m_viewBox->setAttribute(Qt::WA_TranslucentBackground);
    m_appListArea->setWidget(m_viewBox);

    m_pageControl->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout *layoutMain = new QVBoxLayout;
    layoutMain->setContentsMargins(0, 0, 0, DLauncher::DRAG_THRESHOLD);
    layoutMain->setSpacing(0);
    layoutMain->addWidget(m_appListArea,0,Qt::AlignHCenter);
    layoutMain->addWidget(m_pageControl,0,Qt::AlignHCenter);

    setLayout(layoutMain);
}

void MultiPagesView::showCurrentPage(int currentPage)
{
    m_pageIndex = currentPage > 0 ? (currentPage < m_pageCount ? currentPage : m_pageCount - 1) : 0;
    int endValue = m_pageIndex == 0 ? 0 : m_appGridViewList[m_pageIndex]->x();
    int startValue = m_appListArea->horizontalScrollBar()->value();
    m_pageSwitchAnimation->stop();
    m_pageSwitchAnimation->setStartValue(startValue);
    m_pageSwitchAnimation->setEndValue(endValue);
    m_pageSwitchAnimation->start();

    m_pageControl->setCurrent(m_pageIndex);
}

QModelIndex MultiPagesView::selectApp(const int key)
{
    int page = m_pageIndex;
    int itemSelect = -1;
    if (Qt::Key_Left == key || Qt::Key_Up == key) {
        if (page - 1 >= 0) {
            -- page;
            itemSelect = m_calcUtil->appPageItemCount(m_category) - 1;
        } else {
            page = m_pageCount - 1;
            itemSelect = m_pageAppsModelList[page]->rowCount(QModelIndex()) - 1;
        }
    } else {
        if (page + 1 < m_pageCount) {
            ++ page;
            itemSelect = 0;
        } else {
            page = 0;
            itemSelect = 0;
        }
    }
    if (page != m_pageIndex)
        showCurrentPage(page);

    return m_appGridViewList[m_pageIndex]->indexAt(itemSelect);
}

AppGridView *MultiPagesView::pageView(int pageIndex)
{
    if (pageIndex >= m_pageCount)
        return nullptr;

    return m_appGridViewList[pageIndex];
}

AppsListModel *MultiPagesView::pageModel(int pageIndex)
{
    if (pageIndex >= m_pageCount)
        return nullptr;

    return m_pageAppsModelList[pageIndex];
}

void MultiPagesView::wheelEvent(QWheelEvent *e)
{
    if (AppsListModel::All > m_category || AppsListModel::Category < m_category)
        return;

    if (m_pageSwitchAnimation->state() == QPropertyAnimation::Running)
        return;

    int page = m_pageIndex;
    if (e->delta() > 0) {
        if (page - 1 >= 0)
            -- page;
    } else if (e->delta() < 0) {
        if (page + 1 < m_pageCount)
            ++ page;
    }

    if (page != m_pageIndex)
        showCurrentPage(page);
}

void MultiPagesView::mousePress(QMouseEvent *e)
{
    m_bMousePress = true;
    m_nMousePos = e->x();
    m_scrollValue = m_appListArea->horizontalScrollBar()->value();
    m_scrollStart = m_scrollValue;

    if(e->button() != Qt::RightButton) updateGradient();

    if(m_pageCount == 1 && m_category != AppsListModel::Search)
        QWidget::mousePressEvent(e);
}

void MultiPagesView::mouseMove(QMouseEvent *e)
{
    if (!m_bMousePress)
        return;
    int nDiff = m_nMousePos - e->x();
    m_scrollValue += nDiff;
    m_appListArea->horizontalScrollBar()->setValue(m_scrollValue);

    if(m_pageCount == 1)
        QWidget::mouseMoveEvent(e);
}

void MultiPagesView::mouseRelease(QMouseEvent *e)
{
    int nDiff = m_nMousePos - e->x();
    if (nDiff > DLauncher::TOUCH_DIFF_THRESH) { // 加大范围来避免手指点击触摸屏抖动问题
        showCurrentPage(m_pageIndex + 1);
    } else if (nDiff < -DLauncher::TOUCH_DIFF_THRESH) { // 加大范围来避免手指点击触摸屏抖动问题
        showCurrentPage(m_pageIndex - 1);
    } else {
        int nScroll = m_appListArea->horizontalScrollBar()->value();
        //多个分页是点击直接隐藏
        if (nScroll == m_scrollStart && m_pageCount != 1)
            emit m_appGridViewList[m_pageIndex]->clicked(QModelIndex());
        else if (nScroll - m_scrollStart > DLauncher::MOUSE_MOVE_TO_NEXT)
            showCurrentPage(m_pageIndex + 1);
        else if (nScroll - m_scrollStart < -DLauncher::MOUSE_MOVE_TO_NEXT)
            showCurrentPage(m_pageIndex - 1);
        else
            showCurrentPage(m_pageIndex);
    }
    m_bMousePress = false;

    setGradientVisible(false);

    if(m_pageCount == 1)
        QWidget::mouseReleaseEvent(e);
}

void MultiPagesView::setGradientVisible(bool visible)
{
    m_pLeftGradient->setVisible(visible);
    m_pRightGradient->setVisible(visible);
}

// 更新边框渐变，在屏幕变化时需要更新，类别拖动时需要隐藏
void MultiPagesView::updateGradient()
{
    // 找到背景widget
    QWidget *backgroundWidget = parentWidget();
    while (backgroundWidget) {
        if (qobject_cast<FullScreenFrame *>(backgroundWidget))
            break;

        if (qobject_cast<BlurBoxWidget *>(backgroundWidget))
            return;

        backgroundWidget = backgroundWidget->parentWidget();
    }

    // 获取当前屏幕的高度
    QScreen *screen = QGuiApplication::primaryScreen();
    int screen_height = screen->availableGeometry().height();
    // 计算过渡动画开始的位置 区分左右位置与上下位置
    int paddingL = m_calcUtil->getScreenSize().width() * SIDES_SPACE_SCALE;
    int paddingR = m_calcUtil->getScreenSize().width() - paddingL - 1;
    int topPos = mapToGlobal(rect().topLeft()).y() > screen_height ? mapToGlobal(rect().topLeft()).y() - screen_height : mapToGlobal(rect().topLeft()).y();

    QPoint left(paddingL,topPos);
    QPoint right(paddingR,topPos);

    QPixmap background = backgroundWidget->grab();
    updateGradient(background, left, right);
}
