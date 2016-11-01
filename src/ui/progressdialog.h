#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

namespace Ui {
class ProgressDialog;
}

class ProgressDialog : public QDialog
{
	Q_OBJECT
	
public:
	explicit ProgressDialog( QWidget * parent = nullptr );
	~ProgressDialog();
	
	void reset();
	void setNumFiles( int );
	void setTotalFiles( int );
	bool finished();

public slots:
	void advance();
	void checkDone();

signals:
	void cancel();

protected slots:
	void cancelled();

private:
	Ui::ProgressDialog * ui;

	QAtomicInt numFiles;

	bool wasCancelled = false;
};

#endif // PROGRESSDIALOG_H
