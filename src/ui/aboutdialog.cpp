#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AboutDialog)
{
	ui->setupUi(this);

	QString text = tr( R"rhtml(
	<p>BAE is a tool for opening and extracting Bethesda Archive (BSA, BA2) files.</p>

	<p>BAE is free software available under a BSD license.
	The source is available via <a href='https://github.com/jonwd7/bae'>GitHub</a></p>

	<p>For the decompression of BSA (Version 105) files, BAE uses <a href='https://github.com/lz4/lz4'>LZ4</a>:<br>
	LZ4 Library<br>
	Copyright (c) 2011-2015, Yann Collet<br>
	All rights reserved.</p>

	)rhtml" );

	ui->text->setText( text );
	ui->text->setWordWrap( true );
	ui->text->setOpenExternalLinks( true );
}

AboutDialog::~AboutDialog()
{
	delete ui;
}
