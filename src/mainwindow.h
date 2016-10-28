#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>

#include <QStandardItemModel>
#include <QSortFilterProxyModel>

class BSA;
class BSAModel;
class BSAProxyModel;
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
	void cancelExtract();

protected slots:
    void openDlg();
	void extract();
	void itemChanged( QStandardItem * item );

protected:
	void dropEvent( QDropEvent * ev );
	void dragEnterEvent( QDragEnterEvent * ev );

private:
	void recurseModel( QStandardItem * item, QList<QStandardItem *> & itemList );
    
    Ui::MainWindow * ui;

	QWidget * aboutDialog;
    
    QTreeView * archiveView;
    
    BSAModel * archiveModel;
    BSAProxyModel * archiveProxyModel;

	QStandardItemModel * emptyModel;
	
	QHash<QString, BSA *> bsaHash;
	
	ProgressDialog * progDlg;
	
	uint32_t numOpenFiles;
	bool process = false;
};

#endif // MAINWINDOW_H
