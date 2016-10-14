#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "progressdialog.h"
#include "archive.h"
#include "bsa.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>

MainWindow::MainWindow( QWidget *parent )
	: QMainWindow( parent ), ui( new Ui::MainWindow )
{
	ui->setupUi( this );

	ui->mainToolBar->hide();

	archiveView = ui->treeView;

	archiveModel = new BSAModel( this );
	archiveProxyModel = new BSAProxyModel( this );

	// Empty Model for swapping out before model fill
	emptyModel = new QStandardItemModel( this );

	// View
	// DO NOT SET THESE HERE
	//archiveView->setModel( archiveProxyModel );
	//archiveView->setSortingEnabled( true );

	progDlg = new ProgressDialog( this );

	connect( ui->aOpenFile, &QAction::triggered, this, &MainWindow::openDlg );

	connect( ui->btnExtract, &QPushButton::pressed, this, &MainWindow::extract );

	connect( ui->btnSelectAll, &QPushButton::pressed, [this]() {
		auto c = archiveModel->rowCount();

		for ( int i = 0; i < c; i++ ) {
			archiveModel->item( i, 0 )->setCheckState( Qt::Checked );
		}
	} );

	connect( ui->btnSelectNone, &QPushButton::pressed, [this]() {
		auto c = archiveModel->rowCount();

		for ( int i = 0; i < c; i++ ) {
			archiveModel->item( i, 0 )->setCheckState( Qt::Unchecked );
		}
	} );

	connect( progDlg, &ProgressDialog::cancel, this, &MainWindow::cancelExtract );
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::openDlg()
{
	QStringList files = QFileDialog::getOpenFileNames( this, tr( "Open File" ), "", "All Files (*.bsa *.ba2);;BSA (*.bsa);;BA2 (*.ba2)" );
	if ( files.count() ) {

		openFile( files.takeFirst() );

		while ( files.count() > 0 ) {
			appendFile( files.takeFirst() );
		}
	}

}

void MainWindow::recurseModel( QStandardItem * item, QList<QStandardItem *> & itemList )
{
	if ( !item->parent() || (item->hasChildren() && item->checkState() != Qt::Unchecked) ) {
		bool result = false;
		for ( int i = 0; i < item->rowCount(); i++ ) {
			auto child = item->child( i, 0 );
			if ( child->checkState() != Qt::Unchecked ) {
				if ( !child->hasChildren() ) {
					itemList.append( item->child( i, 1 ) );
					result |= true;
				} else {
					recurseModel( child, itemList );
				}
			}
		}
	}
}

void MainWindow::cancelExtract()
{
	process = false;
}

void MainWindow::extract()
{
	QList<QStandardItem *> itemList;
	recurseModel( archiveModel->invisibleRootItem(), itemList );

	if ( !itemList.count() )
		return;

	QString dir = QFileDialog::getExistingDirectory( this, tr( "Select Folder" ),
		"", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
	);

	if ( dir.isEmpty() )
		return;

	progDlg->show();
	progDlg->setTotalFiles( itemList.count() );
	progDlg->setWindowTitle( "Extracting..." );

	int num = 1;
	process = true;

	for ( auto i : itemList ) {
		if ( !process ) {
			progDlg->hide();
			progDlg->setTotalFiles( 0 );
			progDlg->setNumFiles( 0 );
			break;
		}

		auto file = i->index().data( Qt::EditRole ).toString();
		if ( file.startsWith( "/" ) )
			file.remove( 0, 1 );

		auto bsaPath = i->index().sibling( i->row(), 2 ).data( Qt::EditRole ).toString();

		auto bsa = bsaHash.value( bsaPath );
		if ( bsa ) {
			QByteArray data;

			if ( bsa->fileContents( file, data ) ) {
				QFile f( dir + "/" + file );
				QFileInfo finfo( f );
	
				QDir d( finfo.path() );
				if ( !d.exists() )
					d.mkpath( finfo.path() );
	
				if ( f.open( QIODevice::WriteOnly ) ) {
					f.write( data );
					f.close();
				}
			} else {
				qWarning() << file << " did not extract correctly.";
			}

			qApp->processEvents();
			progDlg->setNumFiles( num++ );

			if ( progDlg->finished() ) {
				progDlg->hide();
			}

		} else {
			qCritical( "Filepath mismatch" );
		}
	}
}

void MainWindow::openFile( const QString & file )
{
	// Clear models and connections
	archiveModel->clear();
	archiveProxyModel->clear();
	archiveProxyModel->setSourceModel( emptyModel );
	archiveView->setModel( emptyModel );
	archiveView->setSortingEnabled( false );
	disconnect( archiveModel, &BSAModel::itemChanged, this, &MainWindow::itemChanged );

	archiveModel->init();
	setWindowFilePath( file );

	appendFile( file );
}

void MainWindow::appendFile( const QString & file )
{
	archiveProxyModel->setSourceModel( emptyModel );
	archiveView->setModel( emptyModel );
	archiveView->setSortingEnabled( false );
	disconnect( archiveModel, &BSAModel::itemChanged, this, &MainWindow::itemChanged );

	auto handler = ArchiveHandler::openArchive( file );
	if ( !handler ) {
		qDebug() << "Handler error";
		return;
	}

	auto bsa = handler->getArchive<BSA *>();
	if ( bsa ) {

		bsaHash.insert( bsa->path(), bsa );

		// Populate model from BSA
		bsa->fillModel( archiveModel );
		if ( archiveModel->rowCount() == 0 ) {
			return;
		}

		// Set proxy and view only after filling source model
		archiveProxyModel->setSourceModel( archiveModel );
		archiveView->setModel( archiveProxyModel );
		archiveView->setSortingEnabled( true );

		connect( archiveModel, &BSAModel::itemChanged, this, &MainWindow::itemChanged );

		archiveView->hideColumn( 1 );
		archiveView->hideColumn( 2 );
		archiveView->setColumnWidth( 0, 300 );
		//archiveView->setColumnWidth( 2, 50 );

		// Sort proxy after model/view is populated
		archiveProxyModel->sort( 0, Qt::AscendingOrder );
		archiveProxyModel->resetFilter();
	}
}

void MainWindow::itemChanged( QStandardItem * item )
{
	// TODO: This is absolutely terrible and I should write a custom model to handle this
	//	It causes an exponential explosion in time it takes based on row count

	auto p = item->parent();
	auto chk = item->checkState();

	// Propagate partial checks upwards
	if ( chk == Qt::PartiallyChecked && (p && p->checkState() != Qt::PartiallyChecked) ) {
		p->setCheckState( Qt::PartiallyChecked );
		return;
	}

	// Check parent if all children checked
	if ( chk == Qt::Checked && p && p->checkState() == Qt::PartiallyChecked ) {
		bool allChecked = true;
		for ( int i = 0; i < p->rowCount(); i++ ) {
			auto c = p->child( i, 0 );
			allChecked &= c->checkState() == Qt::Checked;
		}

		if ( allChecked ) {
			p->setCheckState( Qt::Checked );
			return;
		}
	}

	// Assure parent is at least partially checked
	if ( chk == Qt::Checked && p && p->checkState() == Qt::Unchecked ) {
		p->setCheckState( Qt::PartiallyChecked );
	}

	// Propagate selection downwards
	if ( chk != Qt::PartiallyChecked ) {
		//qDebug() << "Setting children";
		for ( int i = 0; i < item->rowCount(); i++ ) {
			auto c = item->child( i, 0 );
			c->setCheckState( chk );
		}
	}

	// Uncheck parent if all children unchecked
	if ( chk == Qt::Unchecked && p && p->checkState() != Qt::Unchecked ) {
		bool allUnchecked = true;
		for ( int i = 0; i < p->rowCount(); i++ ) {
			auto c = p->child( i, 0 );
			allUnchecked &= c->checkState() == Qt::Unchecked;
		}

		if ( allUnchecked ) {
			p->setCheckState( Qt::Unchecked );
		} else {
			p->setCheckState( Qt::PartiallyChecked );
		}
	}
}
