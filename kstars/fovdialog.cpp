/***************************************************************************
                          fovdialog.cpp  -  description
                             -------------------
    begin                : Fri 05 Sept 2003
    copyright            : (C) 2003 by Jason Harris
    email                : kstars@30doradus.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qfile.h>
#include <qframe.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qstringlist.h>

#include <klocale.h>
#include <kcolorbutton.h>
#include <kcombobox.h>
#include <knuminput.h>
#include <klineedit.h>
#include <kstandarddirs.h>
#include <kpushbutton.h>

#include "fovdialog.h"

//---------FOVDialog---------------//
FOVDialog::FOVDialog( QWidget *parent )
	: KDialogBase( KDialogBase::Plain, i18n( "Set FOV Indicator" ), Ok|Cancel, Ok, parent ) {

	ks = (KStars*)parent;

	QFrame *page = plainPage();
	QVBoxLayout *vlay = new QVBoxLayout( page, 0, 0 );
	fov = new FOVDialogUI( page );
	vlay->addWidget( fov );

	connect( fov->FOVListBox, SIGNAL( selectionChanged( QListBoxItem* ) ), SLOT( slotSelect( QListBoxItem* ) ) );
	connect( fov->NewButton, SIGNAL( clicked() ), SLOT( slotNewFOV() ) );
	connect( fov->EditButton, SIGNAL( clicked() ), SLOT( slotEditFOV() ) );
	connect( fov->RemoveButton, SIGNAL( clicked() ), SLOT( slotRemoveFOV() ) );

	FOVList.setAutoDelete( true );
	initList();
}

FOVDialog::~FOVDialog()
{}

void FOVDialog::initList() {
	QStringList fields;
	QFile f;

	QString nm, cl;
	int sh;
	float sz;

	f.setName( locateLocal( "appdata", "fov.dat" ) );

	if ( f.open( IO_ReadOnly ) ) {
		QTextStream stream( &f );
		while ( !stream.eof() ) {
			fields = QStringList::split( ":", stream.readLine() );
			bool ok( false );

			if ( fields.count() == 4 ) {
				nm = fields[0];
				sz = (float)(fields[1].toDouble( &ok ));
				if ( ok ) {
					sh = fields[2].toInt( &ok );
					if ( ok ) {
						cl = fields[3];
					}
				}
			}

			if ( ok ) {
				FOV *newfov = new FOV( nm, sz, sh, cl );
				fov->FOVListBox->insertItem( nm );
				FOVList.append( newfov );
			}
		}
	}
}

void FOVDialog::slotSelect(QListBoxItem *item) {
	FOV *f = FOVList.at( item->listBox()->currentItem() );

	if ( item == 0 ) { //no item selected
		fov->RemoveButton->setEnabled( false );
		fov->EditButton->setEnabled( false );
	} else {
		fov->RemoveButton->setEnabled( true );
		fov->EditButton->setEnabled( true );

		//Draw the selected target symbol in the pixmap.
		if ( f->size() > 0.0 ) {
			QPainter p;
			p.begin( fov->ViewBox );
			p.fillRect( fov->ViewBox->contentsRect(), QColor( "black" ) );
			f->draw( p, (float)( 0.3*fov->ViewBox->contentsRect().width() ) );
			p.drawText( 0, 0, QString( "%1 arcmin" ).arg( (double)(f->size()), 0, 'g', 3 ) );
			p.end();
		}
	}
}

void FOVDialog::slotNewFOV() {
	NewFOV newfdlg( this );

	if ( newfdlg.exec() == QDialog::Accepted ) {
		FOV *newfov = new FOV( newfdlg.ui->FOVName->text(), newfdlg.ui->FOVEdit->text().toDouble(),
				newfdlg.ui->ShapeBox->currentItem(), newfdlg.ui->ColorButton->color().name() );
		fov->FOVListBox->insertItem( newfdlg.ui->FOVName->text() );
		FOVList.append( newfov );
	}
}

void FOVDialog::slotEditFOV() {
	NewFOV newfdlg( this );
	//Preload current values
	FOV *f = FOVList.at( fov->FOVListBox->currentItem() );
	newfdlg.ui->FOVName->setText( f->name() );
	newfdlg.ui->FOVEdit->setText( QString("%1").arg( f->size(), 0, 'g', 3 ) );
	newfdlg.ui->ColorButton->setColor( QColor( f->color() ) );
	newfdlg.ui->ShapeBox->setCurrentItem( f->shape() );
	newfdlg.slotUpdateFOV();

	if ( newfdlg.exec() == QDialog::Accepted ) {
		FOV *newfov = new FOV( newfdlg.ui->FOVName->text(), newfdlg.ui->FOVEdit->text().toDouble(),
				newfdlg.ui->ShapeBox->currentItem(), newfdlg.ui->ColorButton->color().name() );
		fov->FOVListBox->changeItem( newfdlg.ui->FOVName->text(), fov->FOVListBox->currentItem() );
		FOVList.replace( fov->FOVListBox->currentItem(), newfov );
	}
}

void FOVDialog::slotRemoveFOV() {
	int i = fov->FOVListBox->currentItem();

	FOVList.remove( i );
	fov->FOVListBox->removeItem( i );

	fov->FOVListBox->setCurrentItem( i );
}

//-------------NewFOV------------------//
NewFOV::NewFOV( QWidget *parent )
	: KDialogBase( KDialogBase::Plain, i18n( "New FOV Indicator" ), Ok|Cancel, Ok, parent ), f() {
	QFrame *page = plainPage();
	QVBoxLayout *vlay = new QVBoxLayout( page, 0, 0 );
	ui = new NewFOVUI( page );
	vlay->addWidget( ui );

	enableButtonOK( false );

	connect( ui->FOVName, SIGNAL( textChanged( const QString & ) ), SLOT( slotUpdateFOV() ) );
	connect( ui->FOVEdit, SIGNAL( textChanged( const QString & ) ), SLOT( slotUpdateFOV() ) );
	connect( ui->ColorButton, SIGNAL( changed( const QColor & ) ), SLOT( slotUpdateFOV() ) );
	connect( ui->ShapeBox, SIGNAL( activated( int ) ), SLOT( slotUpdateFOV() ) );
	connect( ui->ComputeEyeFOV, SIGNAL( clicked() ), SLOT( slotComputeFOV() ) );
	connect( ui->ComputeCameraFOV, SIGNAL( clicked() ), SLOT( slotComputeFOV() ) );
}

void NewFOV::slotUpdateFOV() {
	bool sizeOk( false );
	f.setName( ui->FOVName->text() );
	float size = (float)(ui->FOVEdit->text().toDouble( &sizeOk ));
	if ( sizeOk ) f.setSize( size );
	f.setShape( ui->ShapeBox->currentItem() );
	f.setColor( ui->ColorButton->color().name() );

	QPainter p;
	p.begin( ui->ViewBox );
	p.fillRect( ui->ViewBox->contentsRect(), QColor( "black" ) );
	f.draw( p, (float)( 0.3*ui->ViewBox->contentsRect().width() ) );
	p.drawText( 0, 0, QString( "%1 arcmin" ).arg( f.size(), 0, 'g', 3 ) );
	p.end();

	if ( ! f.name().isEmpty() && sizeOk )
		enableButtonOK( true );
}

void NewFOV::slotComputeFOV() {
	//DEBUG
	kdDebug() << ":" << sender()->name() << ":" << endl;
	if ( sender()->name() == QString( "ComputeEyeFOV" ) ) kdDebug() << "A" << endl;
	if ( sender()->name() == QString( "ComputeEyeFOV" ) && ui->TLength1->value() > 0.0 ) kdDebug() << "B" << endl;

	if ( sender()->name() == QString( "ComputeEyeFOV" ) && ui->TLength1->value() > 0.0 )
		ui->FOVEdit->setText( QString("%1").arg( ui->EyeFOV->value() * ui->EyeLength->value() / ui->TLength1->value(), 0, 'f', 2 ) );
	else if ( sender()->name() == QString( "ComputeCameraFOV" ) && ui->TLength2->value() > 0.0 )
		ui->FOVEdit->setText( QString("%1").arg( ui->ChipSize->value() * 3438.0 / ui->TLength2->value(), 0, 'f', 2 ) );
}

#include "fovdialog.moc"
