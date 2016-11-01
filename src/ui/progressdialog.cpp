#include "progressdialog.h"
#include "ui_progressdialog.h"

#include <QPushButton>

ProgressDialog::ProgressDialog( QWidget *parent )
	: QDialog( parent ), ui( new Ui::ProgressDialog )
{
	ui->setupUi( this );

	ui->progressBar->setMinimum( 0 );

	connect( ui->btnCancel, &QPushButton::pressed, this, &ProgressDialog::cancelled );
}

ProgressDialog::~ProgressDialog()
{
	delete ui;
}

void ProgressDialog::reset()
{
	numFiles.store( 0 );
	wasCancelled = false;
	ui->progressBar->setValue( 0 );
	ui->progressBar->setMaximum( 0 );
	ui->lblNumFiles->setText( QString::number( 0 ) );
	ui->lblTotalFiles->setText( QString::number( 0 ) );
}

void ProgressDialog::setNumFiles( int num )
{
	ui->lblNumFiles->setText( QString::number( num ) );

	ui->progressBar->setValue( num );
}

void ProgressDialog::setTotalFiles( int num )
{
	ui->lblTotalFiles->setText( QString::number( num ) );

	ui->progressBar->setMaximum( num );
}

bool ProgressDialog::finished()
{
	return ui->progressBar->value() == ui->progressBar->maximum();
}

void ProgressDialog::checkDone()
{
	if ( finished() || wasCancelled )
		hide();
}

void ProgressDialog::cancelled()
{
	emit cancel();
	wasCancelled = true;
	numFiles.store(0);
}

void ProgressDialog::advance()
{
	numFiles.ref();
	setNumFiles( numFiles );
}
