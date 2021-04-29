#ifndef SAVESOLUTION_H
#define SAVESOLUTION_H

#include <QDialog>
#include "ui_savesolution.h"

namespace Ui {
class SaveSolution;
}

class SaveSolution : public QDialog
{
    Q_OBJECT

public:
    explicit SaveSolution(const QString &defaultKey, QWidget *parent = nullptr);
    ~SaveSolution();
    Ui::SaveSolution *ui;
    QString Key() { return  ui->line->text(); };

    QString codeLang() { return  ui->langBox->currentText(); }
    void setLangs(const QStringList& l ) { ui->langBox->addItems(l); }
};

#endif // SAVESOLUTION_H
