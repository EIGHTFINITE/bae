#include "bsatreeview.h"
#include "mainwindow.h"
#include "extractor.h"

#include <QDebug>
#include <QDataStream>
#include <QDrag>
#include <QStringBuilder>
#include <QUrl>


BSATreeView::BSATreeView( QWidget * parent ) : QTreeView( parent )
{
	setSelectionBehavior( QAbstractItemView::SelectRows );
	setSelectionMode( QAbstractItemView::ExtendedSelection );
	setDragDropMode( QAbstractItemView::DragDrop );
	setDragEnabled( true );
}

BSATreeView::~BSATreeView()
{
}

void BSATreeView::setWindow( MainWindow * window )
{
	bae = window;
}

void BSATreeView::startDrag( Qt::DropActions supportedActions )
{
	Q_UNUSED( supportedActions )

	QModelIndexList indexes = selectedIndexes();
	if ( indexes.count() > 0 ) {
		QMimeData * mimeData = model()->mimeData( indexes );
		if ( !mimeData )
			return;

		QByteArray itemData = mimeData->data( "application/bae-archivedata" );
		QDataStream stream( &itemData, QIODevice::ReadOnly );
		mimeData->deleteLater();

		stream >> fileTree >> filenames;
		if ( !filenames.size() || !fileTree.size() )
			return;

		fileData = new MimeData;
		// Set bogus URI to trick the drop event
		fileData->setUrls( { QUrl() } );
		connect( fileData, &MimeData::dataRequested, this, &BSATreeView::createData, Qt::DirectConnection );

		QDrag * drag = new QDrag( this );
		drag->setMimeData( fileData );
		drag->exec( Qt::CopyAction );
	}
}

void BSATreeView::createData( const QString & mimeType )
{
	if ( mimeType != QLatin1String( "text/uri-list" ) || !tmpDir.isValid() )
		return;

	// Prevent more than one call to this function
	disconnect( fileData, &MimeData::dataRequested, this, &BSATreeView::createData );

	QList<QUrl> urls;
	for ( const auto & name : filenames )
		urls << QUrl::fromLocalFile( tmpDir.path() % "/" % name );

	// Extract the files and block thread until finished
	auto ex = bae->getExtractor( tmpDir.path(), fileTree );
	ex->setIncludeDirectories( false );
	ex->start();
	ex->wait();

	fileData->setUrls( urls );
}

QStringList MimeData::formats() const
{
	return QMimeData::formats() << "text/uri-list";
}

QVariant MimeData::retrieveData( const QString & mimetype, QVariant::Type type ) const
{
	emit dataRequested( mimetype );

	return QMimeData::retrieveData( mimetype, type );
}
