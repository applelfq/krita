/* This file is part of the KDE project
   Copyright (c) 2007, 2012 C. Boemann <cbo@boemann.dk>
   Copyright (c) 2007-2008 Fredy Yanardi <fyanardi@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/
#include "KoColorSetWidget.h"
#include "KoColorSetWidget_p.h"

#include <QApplication>
#include <QSize>
#include <QToolButton>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QMouseEvent>
#include <QMenu>
#include <QWidgetAction>
#include <QDir>
#include <QPointer>
#include <QScrollArea>
#include <QGroupBox>
#include <QVBoxLayout>

#include <klocalizedstring.h>
#include <ksharedconfig.h>

#include <resources/KoColorSet.h>
#include <resources/KoColorSetEntry.h>
#include <KoColorPatch.h>
#include <KoEditColorSetDialog.h>
#include <KoColorSpaceRegistry.h>
#include <KoResourceServer.h>
#include <KoResourceServerProvider.h>
#include <KoResourceServerAdapter.h>

#include <kis_palette_view.h>
#include <KisPaletteDelegate.h>
#include <KisPaletteModel.h>
#include <kis_icon_utils.h>

void KoColorSetWidget::KoColorSetWidgetPrivate::addRemoveColors()
{
    QList<KoColorSet*> palettes = rServer->resources();

    Q_ASSERT(colorSet);
    KoEditColorSetDialog *dlg = new KoEditColorSetDialog(palettes, colorSet->name(), thePublic);
    if (dlg->exec() == KoDialog::Accepted ) { // always reload the color set
        KoColorSet *cs = dlg->activeColorSet();
        // check if the selected colorset is predefined
        if( cs && !palettes.contains( cs ) ) {
            int i = 1;
            QFileInfo fileInfo;
            QString savePath = rServer->saveLocation();

            do {
                fileInfo.setFile(savePath + QString("%1.%2").arg(i++, 4, 10, QChar('0')).arg(colorSet->defaultFileExtension()));
            }
            while (fileInfo.exists());

            cs->setFilename( fileInfo.filePath() );
            cs->setValid( true );

            // add new colorset to predefined colorsets
            if (!rServer->addResource(cs)) {
                delete cs;
                cs = 0;
            }
        }
        if (cs) {
            thePublic->setColorSet(cs);
        }
    }
    delete dlg;
}

void KoColorSetWidget::KoColorSetWidgetPrivate::addRecent(const KoColor &color)
{
    if(numRecents < 6) {
        recentPatches[numRecents] = new KoColorPatch(thePublic);
        recentPatches[numRecents]->setFrameShape(QFrame::StyledPanel);
        recentPatches[numRecents]->setDisplayRenderer(displayRenderer);
        recentsLayout->insertWidget(numRecents + 1, recentPatches[numRecents]);
        connect(recentPatches[numRecents], SIGNAL(triggered(KoColorPatch *)), thePublic, SLOT(slotPatchTriggered(KoColorPatch *)));
        numRecents++;
    }
    // shift colors to the right
    for (int i = numRecents- 1; i >0; i--) {
        recentPatches[i]->setColor(recentPatches[i-1]->color());
    }

    //Finally set the recent color
    recentPatches[0]->setColor(color);
}

void KoColorSetWidget::KoColorSetWidgetPrivate::activateRecent(int i)
{
    KoColor color = recentPatches[i]->color();

    while (i >0) {
        recentPatches[i]->setColor(recentPatches[i-1]->color());
        i--;
    }
    recentPatches[0]->setColor(color);
}

KoColorSetWidget::KoColorSetWidget(QWidget *parent)
   : QFrame(parent)
   , d(new KoColorSetWidgetPrivate())
{
    d->thePublic = this;

    d->numRecents = 0;
    d->recentsLayout = new QHBoxLayout;
    d->recentsLayout->setMargin(0);
    d->recentsLayout->addWidget(new QLabel(i18n("Recent:")));
    d->recentsLayout->addStretch(1);

    KoColor color(KoColorSpaceRegistry::instance()->rgb8());
    color.fromQColor(QColor(128,0,0));
    d->addRecent(color);

    d->paletteView = new KisPaletteView(this);
    KisPaletteModel *paletteModel = new KisPaletteModel(d->paletteView);
    d->paletteView->setPaletteModel(paletteModel);
    paletteModel->setDisplayRenderer(d->displayRenderer);

    d->paletteChooser = new KisPaletteListWidget(this);
    d->paletteChooserButton = new KisPopupButton(this);
    d->paletteChooserButton->setPopupWidget(d->paletteChooser);
    d->paletteChooserButton->setIcon(KisIconUtils::loadIcon("hi16-palette_library"));
    d->paletteChooserButton->setToolTip(i18n("Choose palette"));

    d->colorNameCmb = new QComboBox(this);
    d->colorNameCmb->setEditable(true);
    d->colorNameCmb->setInsertPolicy(QComboBox::NoInsert);

    d->bottomLayout = new QHBoxLayout;
    d->bottomLayout->addWidget(d->paletteChooserButton);
    d->bottomLayout->addWidget(d->colorNameCmb);
    d->bottomLayout->setStretch(0, 0); // minimize chooser button
    d->bottomLayout->setStretch(1, 1); // maximize color name cmb

    d->addRemoveButton = new QToolButton(this);
    d->addRemoveButton->setText(i18n("Add / Remove Colors..."));
    d->addRemoveButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    d->mainLayout = new QVBoxLayout(this);
    d->mainLayout->setMargin(4);
    d->mainLayout->setSpacing(2);
    d->mainLayout->addLayout(d->recentsLayout);
    d->mainLayout->addWidget(d->paletteView);
    d->mainLayout->addLayout(d->bottomLayout);
    d->mainLayout->addWidget(d->addRemoveButton);

    setLayout(d->mainLayout);

    connect(d->paletteChooser, SIGNAL(sigPaletteSelected(KoColorSet*)), SLOT(slotPaletteChoosen(KoColorSet*)));
    connect(d->paletteView, SIGNAL(sigEntrySelected(KisSwatch)), SLOT(slotEntrySelected(KisSwatch)));
    connect(d->addRemoveButton, SIGNAL(clicked()), SLOT(addRemoveColors()));
    connect(d->colorNameCmb, SIGNAL(activated(QString)), SLOT(setColorFromString(QString)), Qt::UniqueConnection);

    d->rServer = KoResourceServerProvider::instance()->paletteServer();
    QPointer<KoColorSet> defaultColorSet = d->rServer->resourceByName("Default");
    if (!defaultColorSet && d->rServer->resources().count() > 0) {
        defaultColorSet = d->rServer->resources().first();
    }
    setColorSet(defaultColorSet);
}

KoColorSetWidget::~KoColorSetWidget()
{
    delete d;
}

void KoColorSetWidget::KoColorSetWidgetPrivate::setColorFromString(QString s)
{
    /*
    int i = colornames.indexOf(QRegExp(s+"|Fixed"));
    i = qMax(i,0);
    colorTriggered(patchWidgetList.at(i));
    */
}

void KoColorSetWidget::setColorSet(QPointer<KoColorSet> colorSet)
{
    if (!colorSet) return;
    if (colorSet == d->colorSet) return;

    d->paletteView->paletteModel()->setColorSet(colorSet.data());
    d->colorSet = colorSet;
}

KoColorSet* KoColorSetWidget::colorSet()
{
    return d->colorSet;
}

void KoColorSetWidget::setDisplayRenderer(const KoColorDisplayRendererInterface *displayRenderer)
{
    if (displayRenderer) {
        d->displayRenderer = displayRenderer;
        for (int i=0; i<6; i++) {
            if (d->recentPatches[i]) {
                d->recentPatches[i]->setDisplayRenderer(displayRenderer);
            }
        }
    }
}

void KoColorSetWidget::resizeEvent(QResizeEvent *event)
{
    emit widgetSizeChanged(event->size());
    QFrame::resizeEvent(event);
}

void KoColorSetWidget::slotEntrySelected(const KisSwatch &entry)
{
    emit colorChanged(entry.color(), true);

    // d->colorNameCmb->setCurrentIndex(d->colornames.indexOf(QRegExp(entry.name()+"|Fixed")));
}

void KoColorSetWidget::slotPatchTriggered(KoColorPatch *patch)
{
    emit colorChanged(patch->color(), true);

    int i;

    for (i = 0; i < d->numRecents; i++) {
        if(patch == d->recentPatches[i]) {
            d->activateRecent(i);
            break;
        }
    }

    if (i == d->numRecents) { // we didn't find it above
        d->addRecent(patch->color());
    }

    // d->colorNameCmb->setCurrentIndex(d->colornames.indexOf(QRegExp(patch->toolTip()+"|Fixed")));
}

void KoColorSetWidget::slotPaletteChoosen(KoColorSet *colorSet)
{
    d->colorSet = colorSet;
    d->paletteView->paletteModel()->setColorSet(colorSet);
}

//have to include this because of Q_PRIVATE_SLOT
#include "moc_KoColorSetWidget.cpp"

