#include "savesolution.h"

SaveSolution::SaveSolution(const QString &defaultKey, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveSolution)
{
    ui->setupUi(this);
    ui->line->setText(defaultKey);
    connect(ui->saveButton, &QPushButton::clicked, this, &QDialog::done);
}

SaveSolution::~SaveSolution()
{
    delete ui;
}
