#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

#include <vector>

class BSA;
class BSAModel;
class BSAProxyModel;
class BSATreeView;
class Extractor;
class ProgressDialog;
class QDropEvent;
class QDragEnterEvent;


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow( QWidget * parent = nullptr );
    ~MainWindow();

	void openFile( const QString & filepath );
	void appendFile( const QString & filepath );

	Extractor * getExtractor( const QString & dir, const QHash<QString, QVector<QString>> & fileTree ) const;

	enum
	{
		NameCol,
		FilepathCol,
		BSAPathCol
	};

protected slots:
    void openDlg();
	void extract();
	void itemChanged( QStandardItem * item );

protected:
	void dropEvent( QDropEvent * ev );
	void dragEnterEvent( QDragEnterEvent * ev );

private:
	void pause();
	void unpause();

	void getAllItems( QStandardItem * item, int column, bool folders, std::vector<QStandardItem *> & itemList );
	void getCheckedItems( QStandardItem * item, int column, bool folders, std::vector<QStandardItem *> & itemList );

	void openFileFilter( const QString & filepath );
    
    Ui::MainWindow * ui;

	QWidget * aboutDialog;

    BSATreeView * archiveView;
    
    BSAModel * archiveModel;
    BSAProxyModel * archiveProxyModel;

	QStandardItemModel * emptyModel;
	
	QHash<QString, BSA *> openArchives;
	
	ProgressDialog * progDlg;
	
	uint32_t numOpenFiles;
};

#endif // MAINWINDOW_H
