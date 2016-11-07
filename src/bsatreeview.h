#ifndef BSATREEVIEW_H
#define BSATREEVIEW_H

#include <QTreeView> // Inherited
#include <QMimeData> // Inherited
#include <QHash>
#include <QVariant>
#include <QTemporaryDir>

class MainWindow;
class MimeData;

class BSATreeView : public QTreeView
{
	Q_OBJECT

public:
	BSATreeView( QWidget * parent );
	~BSATreeView();

	void setWindow( MainWindow * window );

public slots:
	void createData( const QString & mimeType );

protected:
	void startDrag( Qt::DropActions supportedActions ) override;

private:
	MainWindow * bae;

	MimeData * fileData = nullptr;
	QVector<QString> filenames;
	QHash<QString, QVector<QString>> fileTree;
	QTemporaryDir tmpDir;
};

class MimeData : public QMimeData
{
	Q_OBJECT

public:
	MimeData() {}

	QStringList formats() const;

signals:
	void dataRequested( const QString & mimeType ) const;

protected:
	QVariant retrieveData( const QString & mimetype, QVariant::Type type ) const override;
};

#endif // BSATREEVIEW_H
