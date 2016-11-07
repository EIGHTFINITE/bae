#include "extractor.h"
#include "bsa.h"
#include "ui/progressdialog.h"

#include <QString>
#include <QStringBuilder>

#include <stack>

Extractor::Extractor( const QString & dir,
					  QHash<QString, QVector<QString>> fileTree,
					  QHash<QString, BSA *> openFiles )
{
	directory = dir;
	archives = fileTree;
	openArchives = openFiles;
	mutex = new QMutex();
}

Extractor::~Extractor()
{
	delete mutex;
}

bool Extractor::getTerminateRequested() const
{
	return terminateRequested;
}

bool Extractor::getIncludeDirectories() const
{
	return includeDirectories;
}

void Extractor::setIncludeDirectories( bool incl )
{
	includeDirectories = incl;
}

void Extractor::run()
{
	QHash<QString, QVector<QString>>::const_iterator i = archives.constBegin();
	while ( i != archives.constEnd() ) {
		workers.clear();
		auto bsa = openArchives.value( i.key() );
		if ( bsa ) {
			auto files = archives.value( i.key() ).toStdVector();

			std::stack<std::vector<QString>> fileStack;

			std::size_t const half = files.size() / 2;
			std::vector<QString> filesA( files.begin(), files.begin() + half );
			std::vector<QString> filesB( files.begin() + half, files.end() );

			std::size_t const half1 = filesA.size() / 2;
			std::size_t const half2 = filesB.size() / 2;

			fileStack.emplace( filesA.begin(), filesA.begin() + half1 );
			fileStack.emplace( filesA.begin() + half1, filesA.end() );
			fileStack.emplace( filesB.begin(), filesB.begin() + half2 );
			fileStack.emplace( filesB.begin() + half2, filesB.end() );

			while ( !fileStack.empty() ) {
				ExtractorTask * worker = new ExtractorTask( directory, bsa, fileStack.top() );
				worker->setManager( this );
				workers.push_back( worker );
				connect( worker, &ExtractorTask::fileWritten, this, &Extractor::fileWritten );
				connect( worker, &ExtractorTask::finished, worker, &ExtractorTask::deleteLater );
				worker->start();

				fileStack.pop();
			}
		} else {
			// TODO: Message logger
			qCritical() << "Could not open " + i.key();
		}

		// Extract whole BSA before going on to next
		for ( auto w : workers )
			w->wait();

		{
			QMutexLocker locker( mutex );
			if ( terminateRequested ) {
				emit terminated();
				break;
			}
		}

		++i;
	}

	emit finished();
}

void Extractor::abort()
{
	QMutexLocker locker( mutex );
	terminateRequested = true;
}

ExtractorTask::ExtractorTask( const QString & dir, BSA * bsa, const std::vector<QString> & list )
{
	directory = dir;
	archive = bsa;
	files = list;
	mutex = new QMutex();
}

ExtractorTask::~ExtractorTask()
{
	delete mutex;
}

void ExtractorTask::setManager( Extractor * mgr )
{
	manager = mgr;
}

QString removeDirectory( const QString & str )
{
	QString tmp = str;
	return tmp.remove( 0, tmp.lastIndexOf( "/" ) + 1 );
}

void ExtractorTask::run()
{
	for ( const QString & file : files ) {
		QByteArray fileData;
		if ( archive->fileContents( file, fileData ) ) {
			QString filename = file;
			if ( !manager->getIncludeDirectories() )
				filename = removeDirectory( file );

			QFile f( directory % "/" % filename );
			if ( f.open( QIODevice::WriteOnly ) ) {
				f.write( fileData );
				f.close();
			}
			{
				QMutexLocker locker( mutex );
				if ( manager->getTerminateRequested() ) {
					emit terminated();
					break;
				}
			}
			emit fileWritten();
		} else {
			// TODO: Message logger
			qWarning() << file << " was not extracted.";
		}
	}
	emit finished();
}
