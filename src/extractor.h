#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <QThread>
#include <QMutex>
#include <QHash>
#include <QVector>
#include <QString>

#include <vector>

class BSA;
class ProgressDialog;
class ExtractorTask;

class Extractor : public QThread
{
	Q_OBJECT

public:
	Extractor( const QString & dir,
			   QHash<QString, QVector<QString>> fileTree,
			   QHash<QString, BSA *> openFiles );

	~Extractor();

	bool getTerminateRequested() const;

	bool getIncludeDirectories() const;
	void setIncludeDirectories( bool incl );

public slots:
	void abort();

signals:
	void fileWritten();
	void finished();
	void terminated();

private:
	void run() override;

	QString directory;
	QHash<QString, BSA *> openArchives;
	QHash<QString, QVector<QString>> archives;
	std::vector<ExtractorTask *> workers;

	bool includeDirectories = true;

	QMutex * mutex = nullptr;
	bool terminateRequested = false;
};

class ExtractorTask : public QThread
{
	Q_OBJECT

public:
	ExtractorTask( const QString & dir,
				   BSA * bsa,
				   const std::vector<QString> & list );

	~ExtractorTask();

	void setManager( Extractor * mgr );

signals:

	void fileWritten();
	void finished();
	void terminated();

private:
	void run() override;

	Extractor * manager = nullptr;
	QString directory;
	BSA * archive = nullptr;
	std::vector<QString> files;

	QMutex * mutex = nullptr;
};

#endif // EXTRACTOR_H
