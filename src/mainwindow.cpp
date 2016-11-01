#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "ui/aboutdialog.h"
#include "ui/progressdialog.h"
#include "extractor.h"
#include "archive.h"
#include "bsa.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QRegularExpression>
#include <QStringBuilder>

#include <QTimer>
#include <QLineEdit>

MainWindow::MainWindow( QWidget *parent )
	: QMainWindow( parent ), ui( new Ui::MainWindow )
{
	ui->setupUi( this );

	ui->mainToolBar->hide();

	setAcceptDrops( true );

	// Init Dialogs
	aboutDialog = new AboutDialog( this );
	progDlg = new ProgressDialog( this );

	archiveView = ui->treeView;

	archiveModel = new BSAModel( this );
	archiveProxyModel = new BSAProxyModel( this );

	// Empty Model for swapping out before model fill
	emptyModel = new QStandardItemModel( this );

	// View
	// DO NOT SET THESE HERE
	//archiveView->setModel( archiveProxyModel );
	//archiveView->setSortingEnabled( true );

	connect( ui->aAbout, &QAction::triggered, aboutDialog, &AboutDialog::show );

	connect( ui->aOpenFile, &QAction::triggered, this, &MainWindow::openDlg );

	connect( ui->btnExtract, &QPushButton::pressed, this, &MainWindow::extract );

	connect( ui->btnSelectAll, &QPushButton::pressed, [this]() {
		auto c = archiveModel->rowCount();
		for ( int i = 0; i < c; i++ ) {
			archiveModel->item( i, 0 )->setCheckState( Qt::Checked );
		}
	} );

	connect( ui->btnSelectNone, &QPushButton::pressed, [this]() {
		for ( int i = 0; i < archiveModel->rowCount(); i++ )
			archiveModel->item( i, 0 )->setCheckState( Qt::Unchecked );
	} );

	connect( progDlg, &ProgressDialog::cancel, this, &MainWindow::cancelExtract );
}

MainWindow::~MainWindow()
{
	qDeleteAll( openArchives );
	delete ui;
}

void MainWindow::openDlg()
{
	QStringList files = QFileDialog::getOpenFileNames( this, tr( "Open File" ), "", "All Files (*.bsa *.ba2);;BSA (*.bsa);;BA2 (*.ba2)" );
	if ( files.count() ) {
		openFile( files.takeFirst() );
		while ( files.count() > 0 )
			appendFile( files.takeFirst() );
	}

}


void MainWindow::getAllItems( QStandardItem * item, int column, bool folders, std::vector<QStandardItem *> & itemList )
{
	for ( int i = 0; i < item->rowCount(); i++ ) {
		auto child = item->child( i, 0 );
		if ( folders || !child->hasChildren() )
			itemList.push_back( item->child( i, column ) );

		getAllItems( child, column, folders, itemList );
	}
}

void MainWindow::getCheckedItems( QStandardItem * item, int column, bool folders, std::vector<QStandardItem *> & itemList )
{

	bool filtered = !archiveProxyModel->mapFromSource( item->index() ).isValid();
	if ( filtered && item->parent() )
		return;

	if ( !item->parent() || (item->hasChildren() && item->checkState() != Qt::Unchecked) ) {
		for ( int i = 0; i < item->rowCount(); i++ ) {
			auto child = item->child( i, 0 );
			if ( child->checkState() == Qt::Unchecked || !archiveProxyModel->mapFromSource( child->index() ).isValid() )
				continue;

			if ( folders || !child->hasChildren() )
				itemList.push_back( item->child( i, column ) );

			getCheckedItems( child, column, folders, itemList );
		}
	}
}

void MainWindow::openFileFilter( const QString & filepath )
{
	QFile inputFile( filepath );
	if ( inputFile.open( QIODevice::ReadOnly ) ) {
		QStringList filters;
		QTextStream istr( &inputFile );
		while ( !istr.atEnd() ) {
			QString line = istr.readLine();
			if ( line.startsWith( "#" ) || line.trimmed() == "" )
				continue;

			// Escape periods, convert wildcard to regex
			line.replace( ".", "\\." ).replace( "*", ".*" );

			filters << line;
		} inputFile.close();

		QRegularExpression re( "(" % filters.join( "|" ) % ")", QRegularExpression::CaseInsensitiveOption );

		// Uncheck everything first
		auto root = archiveModel->invisibleRootItem();
		for ( int i = 0; i < root->rowCount(); i++ )
			root->child( i, 0 )->setCheckState( Qt::Unchecked );

		std::vector<QStandardItem *> itemList;
		getAllItems( root, NameCol, true, itemList );

		// Check all items matching our filters
		for ( auto i : itemList ) {
			auto path = i->index().data( Qt::EditRole ).toString();
			QRegularExpressionMatch match = re.match( path );
			if ( match.hasMatch() )
				i->setCheckState( Qt::Checked );
		}
	}
}


void MainWindow::cancelExtract()
{
	process = false;
}


void MainWindow::extract()
{
	std::vector<QStandardItem *> itemList;
	getCheckedItems( archiveModel->invisibleRootItem(), FilepathCol, false, itemList );

	QVector<QString> dirList;
	QHash<QString, QVector<QString>> fileTree;
	fileTree.reserve( archiveModel->invisibleRootItem()->rowCount() );

	for ( const auto* i : itemList ) {
		auto file = i->index().data( Qt::EditRole ).toString();
		if ( file.startsWith( "/" ) )
			file.remove( 0, 1 );

		auto bsaPath = i->index().sibling( i->row(), BSAPathCol ).data( Qt::EditRole ).toString();

		fileTree[bsaPath] = fileTree[bsaPath] << file;

		int slash = file.lastIndexOf( "/" );
		if ( slash < 0 )
			return; // TODO: Message logger

		auto dir = file.left( slash );
		if ( !dirList.contains( dir ) )
			dirList << file.left( slash );
	}

	if ( !itemList.size() || !fileTree.size() )
		return;

	QString dir = QFileDialog::getExistingDirectory( this, tr( "Select Folder" ),
		"", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
	);

	if ( dir.isEmpty() )
		return;

	progDlg->reset();
	progDlg->show();
	progDlg->setTotalFiles( itemList.size() );
	progDlg->setWindowTitle( "Extracting..." );
	process = true;

	itemList.clear();

	// Make Directories
	for ( const QString& d : dirList ) {
		QString path = dir % "/" % d % "/";
		QDir folder( path );
		if ( !folder.exists() )
			folder.mkpath( path );
	}

	Extractor * extract = new Extractor( dir, fileTree, openArchives );
	connect( progDlg, &ProgressDialog::cancel, extract, &Extractor::abort );
	connect( extract, &Extractor::finished, progDlg, &ProgressDialog::checkDone );
	connect( extract, &Extractor::fileWritten, progDlg, &ProgressDialog::advance );
	connect( extract, &Extractor::finished, extract, &Extractor::deleteLater );
	extract->start();
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
	numOpenFiles = 0;

	if ( openArchives.size() ) {
		qDeleteAll( openArchives );
		openArchives.clear();
	}

	appendFile( file );

	// Filter
	auto filterTimer = new QTimer( this );
	filterTimer->setSingleShot( true );

	connect( ui->archiveFilter, &QLineEdit::textEdited, [filterTimer, this]() { filterTimer->start( 300 ); } );
	connect( filterTimer, &QTimer::timeout, [this]() {
		auto text = ui->archiveFilter->text();

		archiveProxyModel->setFilterRegExp( QRegExp( text, Qt::CaseInsensitive, QRegExp::Wildcard ) );
		archiveView->expandAll();

		if ( text.isEmpty() ) {
			archiveView->collapseAll();
			archiveProxyModel->resetFilter();
		}
	} );

	connect( ui->archiveFilenameOnly, &QCheckBox::toggled, archiveProxyModel, &BSAProxyModel::setFilterByNameOnly );

	ui->archiveFilter->setText("");
}

void MainWindow::appendFile( const QString & file )
{
	archiveProxyModel->setSourceModel( emptyModel );
	archiveView->setModel( emptyModel );
	archiveView->setSortingEnabled( false );
	ui->btnBar->setEnabled( true );
	ui->filterFrame->setEnabled( true );
	disconnect( archiveModel, &BSAModel::itemChanged, this, &MainWindow::itemChanged );

	auto handler = ArchiveHandler::openArchive( file );
	if ( !handler ) {
		// TODO: Message logger
		qDebug() << "Handler error";
		return;
	}

	auto bsa = handler->getArchive<BSA *>();
	if ( bsa ) {
		if ( numOpenFiles > 0 )
			setWindowFilePath( QString( "%1 and %2 other files" ).arg( file ).arg( numOpenFiles ) );
		else
			setWindowFilePath( file );

		openArchives.insert( bsa->path(), bsa );

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

		numOpenFiles++;
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

void MainWindow::dropEvent( QDropEvent * e )
{
	QStringList files;
	QList<QUrl> urls = e->mimeData()->urls();
	for ( QUrl url : urls )
		files << url.toLocalFile();

	if ( !files.count() )
		return;

	if ( files.at(0).endsWith( ".txt", Qt::CaseInsensitive ) ) {
		openFileFilter( files.takeFirst() );
		return;
	}

	openFile( files.takeFirst() );
	while ( files.count() > 0 )
		appendFile( files.takeFirst() );
}

void MainWindow::dragEnterEvent( QDragEnterEvent * e )
{
	QStringList exts = { "bsa", "ba2" };

	if ( e->mimeData()->hasUrls() ) {
		QList<QUrl> urls = e->mimeData()->urls();
		for ( auto url : urls ) {
			if ( url.scheme() == "file" ) {
				QString fn = url.toLocalFile();
				QFileInfo finfo( fn );

				if ( archiveModel->invisibleRootItem()->rowCount() ) {
					// File Filter needs BSAs to be open first
					if ( finfo.exists() && finfo.suffix().endsWith( "txt", Qt::CaseInsensitive ) )
						continue;
				}

				if ( finfo.exists() && exts.contains( finfo.suffix(), Qt::CaseInsensitive ) )
					continue;
			}

			e->ignore();
			return;
		}

		e->accept();
	}
}
